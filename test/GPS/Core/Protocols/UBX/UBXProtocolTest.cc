#include "UBXProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QtMessageHandler>

#include "GPSCancellation.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "GPSTime.h"
#include "LittleEndian.h"
#include "NMEASentence.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/fixtures/GPSFixtureExpectations.h"
#include "RTCMFramer.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"
#include "UBX/UBXMessageSchema.h"
#include "UBX/UBXPlan.h"
#include "UBX/UBXProtocol.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
using Bytes = std::vector<uint8_t>;
namespace Cfg = UBX::Cfg;
namespace Msg = UBX::Msg;

bool validPayload(uint16_t message, std::span<const uint8_t> payload)
{
    return UBX::validPayload(message, payload, UBX::messageSchema(message));
}

using SurveyReply = UBXReceiverModel::SurveyReply;

constexpr uint8_t FIX_OK = 0x01;
constexpr unsigned SPOOF_DETECTION_SHIFT = 3;

template <typename Key, typename Value>
concept ValsetValue = requires(Key key, Value value) { UBX::Plan::ValsetItem{key, value}; };
// A plan value has its key's type; a value wider than the key's size bits does not compile.
static_assert(ValsetValue<UBX::CfgKey<uint16_t>, uint16_t> && ValsetValue<UBX::MsgOutKey, uint8_t>);
static_assert(!ValsetValue<UBX::CfgKey<uint16_t>, uint32_t> && !ValsetValue<UBX::CfgKey<int8_t>, int32_t>);
static_assert(!ValsetValue<UBX::MsgOutKey, unsigned>);

bool failed(const GPSProtocolRuntime& runtime)
{
    return runtime.error() != GPSProtocolError::None;
}

bool positionUpdated(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Position);
}

/// Calls a hook for each protocol warning while alive, after the handler installed earlier has seen it.
class WarningHook
{
public:
    explicit WarningHook(std::function<void(const QString&)> hook)
    {
        s_hook = std::move(hook);
        s_previous = qInstallMessageHandler(&WarningHook::_handle);
    }

    ~WarningHook()
    {
        qInstallMessageHandler(s_previous);
        s_hook = {};
    }

    WarningHook(const WarningHook&) = delete;
    WarningHook& operator=(const WarningHook&) = delete;

private:
    static void _handle(QtMsgType type, const QMessageLogContext& context, const QString& text)
    {
        if (s_previous) {
            s_previous(type, context, text);
        }
        if (type == QtWarningMsg && s_hook) {
            s_hook(text);
        }
    }

    static inline std::function<void(const QString&)> s_hook;
    static inline QtMessageHandler s_previous = nullptr;
};

using UBXReceiver = ModelReceiver<UBXReceiverModel>;

/// Runtime services over @a receiver as these suites drive it: the model's write failures apply, a read that returns
/// data takes a millisecond, one that times out passes its deadline, and each wait counts as a transport operation.
GPSRuntimeIO ubxLink(UBXReceiver& receiver)
{
    auto& model = receiver.model;
    auto& clock = receiver.clock;
    receiver.setWriteHandler([&model](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) {
        return model.interceptWrite(bytes);
    });
    auto link = receiver.io();
    link.clock.wait = [&model, &clock](std::chrono::microseconds delay) {
        ++model.transportOperations;
        clock.advanceBy(delay);
        return true;
    };
    link.read = [&clock, read = std::move(link.read)](std::span<uint8_t> bytes, GPSDeadline deadline) {
        const auto readResult = read(bytes, deadline);
        if (readResult.status == GPSReadStatus::TimedOut) {
            clock.advanceTo(deadline.untilUs + 1);
        } else {
            clock.advanceBy(1000);
        }
        return readResult;
    };
    return link;
}

struct Fixture
{
    GPSProtocolLogCapture log;
    UBXReceiver receiver;
    GPSProtocolRuntime driver;
    GPSBaseStationConfig base;

    explicit Fixture(GPSTestClock& clock)
        : receiver(clock)
        , driver(UBX::FAMILY, ubxLink(receiver), receiver.observer())
    {
        clock.reset();
        log.clear();
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).accuracyMeters = 1.25;
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).duration = 60s;
    }

    /// The commands of the latest configure().
    const std::vector<GPSConfigurationEvidence>& evidence() const { return receiver.log.commands; }

    bool configure()
    {
        unsigned baudrate = 115200;
        GPSConfig config{};
        config.base = base;
        receiver.log.commands.clear();
        return driver.configure(config, baudrate);
    }

    void success(unsigned expectedSurveyPolls)
    {
        QVERIFY(configure());
        QVERIFY(driver.receiverReady());
        QCOMPARE(receiver.model.modes, std::vector<uint32_t>({0, 1}));
        QCOMPARE(receiver.model.surveyPolls, expectedSurveyPolls);
        QCOMPARE(receiver.model.starts, 1);
        QCOMPARE(receiver.model.startSettings.at(Cfg::TMODE_SVIN_MIN_DUR.id), 60);
        QCOMPARE(receiver.model.startSettings.at(Cfg::TMODE_SVIN_ACC_LIMIT.id), 12500);
        QCOMPARE(receiver.model.startSettings.at(Cfg::MSGOUT_UBX_NAV_SVIN.port(UBX::MsgOutPort::UART1).id), 5);
        QCOMPARE(receiver.model.startSettings.at(Cfg::MSGOUT_UBX_NAV_SVIN.port(UBX::MsgOutPort::USB).id), 5);
        QCOMPARE(receiver.log.count<GPSSurveyReport>(), 0);
        QCOMPARE(receiver.model.rtcmEnables, 0);
    }

    void timeout()
    {
        QVERIFY(!configure());
        QVERIFY(!driver.receiverReady());
        QCOMPARE(receiver.model.modes, std::vector<uint32_t>({0}));
        QCOMPARE(receiver.model.starts, 0);
        QVERIFY(receiver.model.surveyPolls > 1 && receiver.model.surveyPolls <= 31);
        QCOMPARE_GE(receiver.clock.nowUs() - receiver.model.disabledAt, 3000000);
        QVERIFY(receiver.clock.nowUs() - receiver.model.disabledAt < 3300000);
        QCOMPARE(receiver.log.count<GPSSurveyReport>(), 0);
        QCOMPARE(receiver.model.rtcmEnables, 0);
    }
};

void integrityReceipts(GPSTestClock& clock)
{
    Fixture f(clock);
    QVERIFY(f.configure());
    Bytes mon_rf(UBX::WIRE_SIZE<UBX::MonRf>, 0);
    mon_rf[1] = 1;
    mon_rf[5] = 3;
    f.receiver.model.queueBytes(ubxFrame(Msg::MON_RF.value(), mon_rf));
    f.driver.receive(100ms);
    QCOMPARE(f.receiver.log.count<GPSIntegrityReport>(), 1);
    QCOMPARE(f.receiver.log.position.navigation.timestampUs, 0);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.state, GPSIntegrityReport::JammingState::Critical);
    const auto rf_stamp = f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs;
    QCOMPARE_NE(rf_stamp, 0);

    Bytes nav_status(UBX::WIRE_SIZE<UBX::NavStatus>, 0);
    nav_status[7] = 1 << SPOOF_DETECTION_SHIFT;
    f.receiver.model.queueBytes(ubxFrame(Msg::NAV_STATUS.value(), nav_status));
    f.driver.receive(100ms);
    const auto spoof_stamp = f.receiver.log.latest<GPSIntegrityReport>().spoofing.timestampUs;
    QCOMPARE_NE(spoof_stamp, 0);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs, rf_stamp);

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    uint32_t tow = 0;
    for (int i = 0; i < 10; ++i) {
        clock.advanceBy(1000000);
        f.receiver.model.queueBytes(ubxNavigationEpoch(pvt, tow += 1000));
        QVERIFY(positionUpdated(f.driver.receive(100ms)));
        QVERIFY(f.receiver.log.position.navigation.timestampUs > rf_stamp);
        QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs, rf_stamp);
        QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().spoofing.timestampUs, spoof_stamp);
    }
    Bytes corrupt = ubxFrame(Msg::MON_RF.value(), mon_rf);
    corrupt.back() ^= 0xff;
    f.receiver.model.queueBytes(corrupt);
    f.driver.receive(100ms);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs, rf_stamp);
    f.receiver.model.queueBytes(ubxFrame(Msg::MON_RF.value(), mon_rf));
    f.driver.receive(100ms);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.state, GPSIntegrityReport::JammingState::Critical);
    QVERIFY(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs > rf_stamp);

    Bytes sec_sig(4, 0);
    sec_sig[0] = 2;
    sec_sig[1] = 1 | (3 << 1);
    f.receiver.model.queueBytes(ubxFrame(Msg::SEC_SIG.value(), sec_sig));
    f.driver.receive(100ms);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.state, GPSIntegrityReport::JammingState::Critical);
    const auto sec_stamp = f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs;
    clock.advanceBy(6000000);
    f.receiver.model.queueBytes(ubxNavigationEpoch(pvt, tow += 1000));
    QVERIFY(positionUpdated(f.driver.receive(100ms)));
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs, sec_stamp);
    f.receiver.model.queueBytes(ubxFrame(Msg::SEC_SIG.value(), sec_sig));
    f.driver.receive(100ms);
    QCOMPARE(f.receiver.log.latest<GPSIntegrityReport>().jamming.state, GPSIntegrityReport::JammingState::Critical);
    QVERIFY(f.receiver.log.latest<GPSIntegrityReport>().jamming.timestampUs > sec_stamp);
}

/// The decoder handles every message the configuration plan enables; it drops any other.
void planOutputDecoded(GPSTestClock&)
{
    namespace Plan = UBX::Plan;
    const auto decoded = [](UBX::MessageId message) {
        const auto& navigation = UBX::Protocol::NAVIGATION_MESSAGES;
        return message.cls == UBX::MsgClass::RTCM3 || message.value() == Msg::NAV_EOE.value() ||
               message.value() == Msg::NAV_SVIN.value() ||
               std::ranges::find(navigation, message.value(), &UBX::MessageId::value) != navigation.end();
    };
    // CFG-MSGOUT keys do not name their message, so the keys the plan enables are paired with them here.
    const std::array<std::pair<UBX::MsgOutKey, UBX::MessageId>, 9> ubxOutputs{{
        {Cfg::MSGOUT_UBX_NAV_PVT, Msg::NAV_PVT},
        {Cfg::MSGOUT_UBX_NAV_HPPOSLLH, Msg::NAV_HPPOSLLH},
        {Cfg::MSGOUT_UBX_NAV_DOP, Msg::NAV_DOP},
        {Cfg::MSGOUT_UBX_NAV_SAT, Msg::NAV_SAT},
        {Cfg::MSGOUT_UBX_NAV_STATUS, Msg::NAV_STATUS},
        {Cfg::MSGOUT_UBX_NAV_EOE, Msg::NAV_EOE},
        {Cfg::MSGOUT_UBX_NAV_SVIN, Msg::NAV_SVIN},
        {Cfg::MSGOUT_UBX_MON_RF, Msg::MON_RF},
        {Cfg::MSGOUT_UBX_SEC_SIG, Msg::SEC_SIG},
    }};
    const std::array rtcmOutputs{Cfg::MSGOUT_RTCM_3X_TYPE1005, Cfg::MSGOUT_RTCM_3X_TYPE1074,
                                 Cfg::MSGOUT_RTCM_3X_TYPE1077, Cfg::MSGOUT_RTCM_3X_TYPE1084,
                                 Cfg::MSGOUT_RTCM_3X_TYPE1087, Cfg::MSGOUT_RTCM_3X_TYPE1094,
                                 Cfg::MSGOUT_RTCM_3X_TYPE1097, Cfg::MSGOUT_RTCM_3X_TYPE1124,
                                 Cfg::MSGOUT_RTCM_3X_TYPE1127, Cfg::MSGOUT_RTCM_3X_TYPE1230};
    std::vector<Plan::ValsetBatch> batches{Plan::surveyIn({.accuracyMeters = 1, .duration = 60s}),
                                           Plan::rtcmOutput(false), Plan::rtcmOutput(true)};
    for (const auto& profile : UBX::RECEIVER_PROFILES) {
        std::ranges::move(Plan::messageOutput({profile}), std::back_inserter(batches));
    }
    for (const auto& batch : batches) {
        for (const auto& item : batch.items) {
            if (!item.msgOut || item.value == Plan::NO_OUTPUT ||
                std::ranges::find(rtcmOutputs, item.key, [](UBX::MsgOutKey key) { return key.i2c.id; }) !=
                    rtcmOutputs.end()) {
                continue;
            }
            const auto output =
                std::ranges::find(ubxOutputs, item.key, [](const auto& pair) { return pair.first.i2c.id; });
            QVERIFY2(output != ubxOutputs.end(), qPrintable(QString::number(item.key, 16)));
            QVERIFY2(decoded(output->second), qPrintable(QString::number(output->second.value(), 16)));
        }
    }
    std::vector<Plan::MessageRate> legacy{Plan::LEGACY_SURVEY_STATUS};
    std::ranges::copy(Plan::legacyMessageOutput(), std::back_inserter(legacy));
    std::ranges::copy(Plan::legacyBaseStatus(), std::back_inserter(legacy));
    for (const bool compact : {false, true}) {
        for (const auto& output : Plan::legacyRTCMOutput(compact)) {
            legacy.push_back(output.rate);
        }
    }
    for (const auto& rate : legacy) {
        QVERIFY2(rate.rate == Plan::NO_OUTPUT || decoded(rate.message),
                 qPrintable(QString::number(rate.message.value(), 16)));
    }
}

/// The integrity report @a frame publishes on an offline decoder, if any.
std::optional<GPSIntegrityReport> integrityOf(GPSProtocolRuntime& driver, const Bytes& frame)
{
    for (const auto& event : driver.decode(frame).events) {
        if (const auto* report = std::get_if<GPSIntegrityReport>(&event)) {
            return *report;
        }
    }
    return std::nullopt;
}

/// MON-RF whose RF blocks report @a antennaStatuses, each as antStatus at block offset 2, and @a jammingStates, each
/// as jammingState in bits 1..0 of the block flags at offset 1 (0 for blocks it doesn't cover).
Bytes monRfFrame(const std::vector<uint8_t>& antennaStatuses, const std::vector<uint8_t>& jammingStates = {})
{
    constexpr size_t block = UBX::WIRE_SIZE<UBX::MonRfBlock>;
    Bytes payload(UBX::WIRE_SIZE<UBX::MonRf> + block * (antennaStatuses.size() - 1), 0);
    payload[1] = static_cast<uint8_t>(antennaStatuses.size());
    for (size_t index = 0; index < antennaStatuses.size(); ++index) {
        payload[4 + index * block + 1] = index < jammingStates.size() ? jammingStates[index] : 0;
        payload[4 + index * block + 2] = antennaStatuses[index];
        payload[4 + index * block + 3] = 1;  // antPower ON
    }
    return ubxFrame(Msg::MON_RF.value(), payload);
}

/// MON-HW with aStatus at offset 20 and jammingState in bits 3..2 of the flags at offset 22.
Bytes monHwFrame(uint8_t antennaStatus, uint8_t jamming)
{
    Bytes payload(UBX::WIRE_SIZE<UBX::MonHw>, 0);
    payload[20] = antennaStatus;
    payload[21] = 1;                                            // aPower ON
    payload[22] = static_cast<uint8_t>((jamming << 2) | 0x13);  // rtcCalib, safeBoot and xtalAbsent are not read
    return ubxFrame(Msg::MON_HW.value(), payload);
}

void antennaStates(GPSTestClock& clock)
{
    using Antenna = GPSIntegrityReport::AntennaState;
    LoggedRuntime driver(UBX::FAMILY, makeGPSRuntimeTestIO(clock));
    driver->armNavigationDecode();

    const struct
    {
        std::vector<uint8_t> blocks;
        Antenna expected;
    } rf[] = {
        // INIT, DONTKNOW and reserved values leave the antenna unknown.
        {{0}, Antenna::Unknown},
        {{1}, Antenna::Unknown},
        {{2}, Antenna::Ok},
        {{3}, Antenna::Short},
        {{4}, Antenna::Open},
        {{5}, Antenna::Unknown},
        // F9P reports two RF blocks and X20 three; the worst state any of them reports stands.
        {{2, 4}, Antenna::Open},
        {{4, 3}, Antenna::Short},
        {{1, 2}, Antenna::Ok},
        {{0, 1, 1}, Antenna::Unknown},
        {{2, 2, 3}, Antenna::Short},
    };

    for (const auto& [blocks, expected] : rf) {
        clock.advanceBy(1000);
        const auto report = integrityOf(*driver, monRfFrame(blocks));
        QVERIFY(report);
        QCOMPARE(report->antenna.state, expected);
        QCOMPARE(report->antenna.timestampUs, clock.nowUs());
    }
    // Each block has its own jamming monitor too; the worst state any of them reports stands.
    using Jamming = GPSIntegrityReport::JammingState;

    const struct
    {
        std::vector<uint8_t> blocks;
        Jamming expected;
    } jammed[] = {
        {{1, 2}, Jamming::Warning}, {{3, 1}, Jamming::Critical}, {{1, 2, 3}, Jamming::Critical},
        {{0, 1}, Jamming::Ok},      {{0, 0}, Jamming::Unknown},
    };

    for (const auto& [blocks, expected] : jammed) {
        const auto report = integrityOf(*driver, monRfFrame(std::vector<uint8_t>(blocks.size(), 2), blocks));
        QVERIFY(report);
        QCOMPARE(report->jamming.state, expected);
    }

    for (const auto& [status, expected] : {std::pair{uint8_t{0}, Antenna::Unknown},
                                           {uint8_t{1}, Antenna::Unknown},
                                           {uint8_t{2}, Antenna::Ok},
                                           {uint8_t{3}, Antenna::Short},
                                           {uint8_t{4}, Antenna::Open}}) {
        clock.advanceBy(1000);
        const auto report = integrityOf(*driver, monHwFrame(status, 0));
        QVERIFY(report);
        QCOMPARE(report->antenna.state, expected);
        QCOMPARE(report->antenna.timestampUs, clock.nowUs());
    }
    // Legacy receivers report jamming in MON-HW, as MON-RF does.
    for (uint8_t jamming = 0; jamming < 4; ++jamming) {
        const auto report = integrityOf(*driver, monHwFrame(2, jamming));
        QVERIFY(report);
        QCOMPARE(report->jamming.state, GPSIntegrityReport::jammingStateFromValue(jamming));
    }
    // SEC-SIG supersedes the jamming state of both; the antenna still follows MON-HW.
    Bytes secSig(4, 0);
    secSig[0] = 2;
    secSig[1] = 1 | (1 << 1);
    QVERIFY(integrityOf(*driver, ubxFrame(Msg::SEC_SIG.value(), secSig)));
    const auto superseded = integrityOf(*driver, monHwFrame(3, 3));
    QVERIFY(superseded);
    QCOMPARE(superseded->jamming.state, GPSIntegrityReport::JammingState::Ok);
    QCOMPARE(superseded->antenna.state, Antenna::Short);

    // A report goes stale like the other diagnostics.
    QCOMPARE(superseded->freshAt(clock.nowUs() + 4'000'000).antenna.state, Antenna::Short);
    QCOMPARE(superseded->freshAt(clock.nowUs() + 5'000'000).antenna.state, Antenna::Unknown);
}

void outputOverflow(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    LoggedRuntime driver(UBX::FAMILY, makeGPSRuntimeTestIO(clock));
    const auto inf = [](UBX::MessageId message, std::string_view text) {
        return ubxFrame(message.value(), Bytes(text.begin(), text.end()));
    };
    // Configuration traffic is not navigation output.
    QVERIFY(driver->decode(inf(Msg::INF_WARNING, "txbuf alloc")).events.empty());
    driver->armNavigationDecode();
    for (const auto message : {Msg::INF_WARNING, Msg::INF_ERROR}) {
        clock.advanceBy(1000);
        const auto report = integrityOf(*driver, inf(message, "txbuf alloc"));
        QVERIFY(report);
        QCOMPARE(report->outputOverflowUs, clock.nowUs());
    }
    const uint64_t overflowUs = clock.nowUs();
    clock.advanceBy(1000);
    QVERIFY(driver->decode(inf(Msg::INF_WARNING, "other warning")).events.empty());
    // Later reports carry the latest overflow, so a consumer sees it with the next position.
    const auto later = integrityOf(*driver, monRfFrame({2}));
    QVERIFY(later);
    QCOMPARE(later->outputOverflowUs, overflowUs);
    // A repeat warns only once its interval has passed; every overflow still reaches the integrity report.
    QVERIFY(driver->decode(inf(Msg::INF_WARNING, "other warning")).events.empty());
    clock.advanceBy(static_cast<uint64_t>(std::chrono::microseconds(UBX::Protocol::REPEATED_WARNING_INTERVAL).count()));
    QVERIFY(driver->decode(inf(Msg::INF_WARNING, "other warning")).events.empty());
    QCOMPARE(log.warnings(),
             (QStringList{QStringLiteral("ubx msg: txbuf alloc"), QStringLiteral("ubx msg: other warning"),
                          QStringLiteral("ubx msg: other warning")}));
}

/// The largest fixed accuracy validation accepts reaches both base configurations in 0.1 mm without overflowing.
void largestFixedAccuracy(GPSTestClock&)
{
    const GPSBaseStationConfig::Fixed largest{
        .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
        .accuracyMeters = 429496.71875f};
    const auto items = UBX::Plan::fixedBase(largest).items;
    const auto accuracy = std::ranges::find(items, Cfg::TMODE_FIXED_POS_ACC.id, &UBX::Plan::ValsetItem::key);
    QVERIFY(accuracy != items.end());
    QCOMPARE(accuracy->value, 4294967187u);
    QCOMPARE(UBX::Plan::legacyFixedBase(largest).fixedPosAcc, 4294967187u);
}

/// Coordinates beyond their range decode as unknown.
void outOfRangeCoordinates(GPSTestClock& clock)
{
    LoggedRuntime driver(UBX::FAMILY, makeGPSRuntimeTestIO(clock));
    driver->armNavigationDecode();
    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = FIX_OK;
    uint32_t tow = 0;
    /// The single position the epoch decodes to, if it decodes to exactly one event.
    const auto decodePvt = [&](int32_t longitude, int32_t latitude) -> std::optional<GPSNavigationValues> {
        (void) LittleEndian::write<int32_t>(pvt, 24, longitude);
        (void) LittleEndian::write<int32_t>(pvt, 28, latitude);
        const auto decoded = driver->decode(ubxNavigationEpoch(pvt, tow += 1000));
        if (decoded.events.size() != 1) {
            return std::nullopt;
        }
        return std::get<GPSDecodedPosition>(decoded.events.front()).navigation;
    };
    const auto limits = decodePvt(-1800000000, 900000000);
    QVERIFY(limits);
    QVERIFY(limits->latitudeDegrees == 90 && limits->longitudeDegrees == -180);
    const auto latitude = decodePvt(80000000, 900000001);
    QVERIFY(latitude);
    QVERIFY(std::isnan(latitude->latitudeDegrees) && latitude->longitudeDegrees == 8);
    const auto longitude = decodePvt(-1800000001, 470000000);
    QVERIFY(longitude);
    QVERIFY(longitude->latitudeDegrees == 47 && std::isnan(longitude->longitudeDegrees));
    const auto extremes = decodePvt((std::numeric_limits<int32_t>::max)(), (std::numeric_limits<int32_t>::min)());
    QVERIFY(extremes);
    QVERIFY(std::isnan(extremes->latitudeDegrees) && std::isnan(extremes->longitudeDegrees));
}

void navigationFixFlags(GPSTestClock& clock)
{
    using Fix = GPSPositionReport::FixType;
    constexpr std::array<Fix, 8> FIX_3D{Fix::Fix3D,    Fix::Differential, Fix::RTKFloat, Fix::RTKFloat,
                                        Fix::RTKFixed, Fix::RTKFixed,     Fix::Fix3D,    Fix::Differential};
    // A time-only fix (5) is the held position of a base station in time mode.
    constexpr std::array<Fix, 6> UNCORRECTED{Fix::NoFix, Fix::Extrapolated, Fix::Fix2D,
                                             Fix::Fix3D, Fix::Fix3D,        Fix::Fix3D};
    for (const unsigned rawFix : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 255U}) {
        for (unsigned flags = 0; flags <= UINT8_MAX; ++flags) {
            Fix expected = rawFix < UNCORRECTED.size() ? UNCORRECTED[rawFix] : Fix::Unknown;
            if (!(flags & 1)) {
                expected = Fix::NoFix;
            } else if (rawFix == 3 || rawFix == 4) {
                expected = FIX_3D[((flags >> 6) * 2) + ((flags >> 1) & 1)];
            }
            clock.reset(GPSTestClock::START_US);
            LoggedRuntime runtime(UBX::FAMILY, makeGPSRuntimeTestIO(clock));
            auto& driver = *runtime;
            driver.armNavigationDecode();
            Bytes pvt(92);
            (void) LittleEndian::write<uint32_t>(pvt, 0, 1000);
            pvt[20] = static_cast<uint8_t>(rawFix);
            pvt[21] = static_cast<uint8_t>(flags);
            (void) LittleEndian::write<int32_t>(pvt, 24, 80000000);
            (void) LittleEndian::write<int32_t>(pvt, 28, 470000000);
            (void) LittleEndian::write<int32_t>(pvt, 60, 12000);
            QVERIFY(driver.decode(ubxFrame(Msg::NAV_PVT.value(), pvt)).events.empty());
            Bytes end(4);
            (void) LittleEndian::write<uint32_t>(end, 0, 1000);
            const auto decoded = driver.decode(ubxFrame(Msg::NAV_EOE.value(), end));
            QCOMPARE(decoded.events.size(), 1);
            const auto& fix = std::get<GPSDecodedPosition>(decoded.events.front());
            QCOMPARE(fix.navigation.fixType, expected);
            QCOMPARE(fix.velocityValid, (expected != Fix::NoFix && expected != Fix::Unknown));
            QVERIFY(fix.navigation.latitudeDegrees == 47 && fix.navigation.longitudeDegrees == 8);
            QVERIFY(std::abs(fix.navigation.speedMetersPerSecond - 12) < 1e-5f);
        }
    }
}

void transactionalFrames(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    UBXReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, ubxLink(receiver));
    unsigned baud = 115200;
    GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    QVERIFY(driver.configure(config, baud));
    Bytes payload(20, 0);
    payload[4] = 1;
    payload[5] = 1;
    payload[8] = 0;
    payload[9] = 17;
    payload[10] = 35;
    const auto valid = ubxFrame(Msg::NAV_SAT.value(), payload);
    payload[9] = 23;
    auto corrupt = ubxFrame(Msg::NAV_SAT.value(), payload);
    corrupt.back() ^= 1;
    Bytes joined = valid;
    joined.insert(joined.end(), corrupt.begin(), corrupt.end());
    auto decoded = driver.decode(joined);
    QCOMPARE(decoded.events.size(), 1);
    QCOMPARE(std::get<GPSDecodedSatellites>(decoded.events[0]).constellations[0].inView, 1);
    QVERIFY(driver.decode(std::span(valid).first(valid.size() - 1)).events.empty());
    QCOMPARE(driver.decode(std::span(valid).last(1)).events.size(), 1);
    payload[5] = 2;  // A valid checksum cannot make an incomplete counted payload valid.
    QVERIFY(driver.decode(ubxFrame(Msg::NAV_SAT.value(), payload)).events.empty());
    QVERIFY(driver.decode(ubxFrame(Msg::NAV_SAT.value(), Bytes(7, 0))).events.empty());
    const auto empty = driver.decode(ubxFrame(Msg::NAV_SAT.value(), Bytes{0, 0, 0, 0, 1, 0, 0, 0}));
    QCOMPARE(empty.events.size(), 1);
    QCOMPARE(std::get<GPSDecodedSatellites>(empty.events.front()).constellations[0].inView, 0);
    const QString originalIdentity = driver.identity();
    Bytes version(70, 0);
    const std::string module = "MOD=NEO-M9N";
    std::copy(module.begin(), module.end(), version.begin() + 40);
    auto badVersion = ubxFrame(Msg::MON_VER.value(), version);
    badVersion.back() ^= 1;
    QVERIFY(driver.decode(badVersion).events.empty());
    QCOMPARE(driver.identity(), originalIdentity);

    // A version without a module names only the firmware.
    Bytes baseVersion(40, 0);
    const std::string firmware = "SPG 4.04";
    std::copy(firmware.begin(), firmware.end(), baseVersion.begin());
    QVERIFY(driver.decode(ubxFrame(Msg::MON_VER.value(), baseVersion)).events.empty());
    QCOMPARE(driver.identity(), QString::fromStdString(firmware));

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    driver.armNavigationDecode();
    Bytes epochs;
    for (uint8_t index = 1; index <= 20; ++index) {
        pvt[23] = index;
        const auto frames = ubxNavigationEpoch(pvt, index * 1000u);
        epochs.insert(epochs.end(), frames.begin(), frames.end());
    }
    unsigned count = 0;
    for (const auto& event : driver.decode(epochs).events) {
        QCOMPARE(std::get<GPSDecodedPosition>(event).navigation.satellitesUsed, ++count);
    }
    QCOMPARE(count, 20);
    driver.armNavigationDecode({.corrections = true});
    const Bytes correction = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    std::copy(correction.begin(), correction.end(), pvt.begin() + 40);
    const auto embedded = driver.decode(ubxNavigationEpoch(pvt, 21000));
    QCOMPARE(embedded.events.size(), 1);
    QVERIFY(std::holds_alternative<GPSDecodedPosition>(embedded.events.front()));
    const auto standalone = driver.decode(correction);
    QCOMPARE(standalone.events.size(), 1);
    QVERIFY(std::holds_alternative<GPSRTCMFrame>(standalone.events.front()));
}

void controlDeadline(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    UBXReceiver receiver(clock);
    clock.reset(GPSTestClock::START_US);
    auto io = ubxLink(receiver);
    const auto read = io.read;
    const auto write = io.write;
    bool expireRead = false;
    bool verifyWrite = false;
    bool expiredWrite = false;
    int lateWrites = 0;
    io.read = [&](auto bytes, GPSDeadline deadline) {
        const auto result = read(bytes, deadline);
        if (expireRead && result.status == GPSReadStatus::Data) {
            clock.advanceTo(deadline.untilUs);
            expireRead = false;
            verifyWrite = true;
        }
        return result;
    };
    io.write = [&](auto bytes, GPSDeadline deadline) {
        lateWrites += verifyWrite;
        expiredWrite |= verifyWrite && deadline.remaining(clock.nowUs()) == 0ms;
        return write(bytes, deadline);
    };
    GPSProtocolRuntime driver(UBX::FAMILY, std::move(io), receiver.observer());
    unsigned baud = 115200;
    GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    QVERIFY(driver.configure(config, baud));
    driver.receive(10ms);  // Drain the configuration responses before the unexpected message.
    receiver.model.readChunk = GPSCommandChannel::READ_CHUNK_SIZE;
    receiver.model.queueFrame(Msg::RXM_RAWX.value(), Bytes(16, 0));
    expireRead = true;
    driver.receive(10ms);
    QVERIFY(verifyWrite);
    // The command disabling it, after the expired read, gets a deadline of its own.
    QVERIFY(!expiredWrite);
    QCOMPARE(lateWrites, 1);
    QVERIFY(!failed(driver));
}

void identificationWriteBudget(GPSTestClock& clock)
{
    clock.reset(GPSTestClock::START_US);
    std::vector<uint64_t> writeDeadlines;
    std::vector<GPSConfigurationEvidence> completions;
    uint64_t replyDeadline = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [&](std::span<uint8_t>, GPSDeadline deadline) {
        if (!writeDeadlines.empty()) {
            replyDeadline = deadline.untilUs;
        }
        clock.advanceTo(deadline.untilUs);
        return GPSReadResult{GPSReadStatus::TimedOut};
    };
    io.write = [&](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        writeDeadlines.push_back(deadline.untilUs);
        clock.advanceBy(40000);
        return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
    };
    GPSRuntimeObserver observer;
    observer.commandFinished = [&](const GPSConfigurationEvidence& result) { completions.push_back(result); };
    GPSProtocolRuntime driver(UBX::FAMILY, std::move(io), std::move(observer));
    unsigned baud = 115200;
    GPSConfig config;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    QVERIFY(!driver.configure(config, baud));
    QCOMPARE(completions.size(), 1);
    const auto& evidence = completions.front();
    QCOMPARE(evidence.command, UBX::messageName(Msg::MON_VER));
    QCOMPARE(evidence.startedAtUs, 1020000);
    // The frame is written under the configuration cap, while its reply has the identity timeout.
    QCOMPARE(writeDeadlines, std::vector<uint64_t>{evidence.startedAtUs + 250000});
    QVERIFY(evidence.acceptedBytes == 8 && evidence.writtenBytes == 8);
    QCOMPARE(evidence.outcome, GPSCommandOutcome::TimedOut);
    QCOMPARE(replyDeadline, evidence.startedAtUs + 2000000);
}

void reentrantPayload(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    UBXReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, ubxLink(receiver));
    bool reentered = false;
    const Bytes correction = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    // A message handler that decodes more input while the decoder logs reuses the storage of the frame being
    // decoded; the nested frame does not log, because Qt does not pass nested messages to handlers.
    const WarningHook hook([&](const QString& message) {
        if (!reentered && message == u"ubx msg: txbuf alloc") {
            reentered = true;
            driver.armNavigationDecode({.corrections = true});
            (void) driver.consume(ubxFrame(Msg::INF_NOTICE.value(), Bytes{'o', 'k'}));
            (void) driver.consume(std::span(correction).first(4));
        }
    });
    driver.armNavigationDecode({.corrections = true});
    const std::string warning = "txbuf alloc";
    // The overflow is reported after logging, so the nested decode does not take it.
    const auto overflow = driver.decode(ubxFrame(Msg::INF_WARNING.value(), Bytes(warning.begin(), warning.end())));
    QCOMPARE(overflow.events.size(), 1);
    QCOMPARE_NE(std::get<GPSIntegrityReport>(overflow.events.front()).outputOverflowUs, 0);
    QVERIFY(reentered);
    QCOMPARE(log.warnings(), QStringList{"ubx msg: txbuf alloc"});
    const auto decoded = driver.decode(std::span(correction).subspan(4));
    QCOMPARE(decoded.events.size(), 1);
    QVERIFY(std::holds_alternative<GPSRTCMFrame>(decoded.events.front()));
    // The transmit-buffer warning sends nothing to the receiver.
    driver.receive(1ms);
    QCOMPARE(receiver.model.transportOperations, 0);
}

void isolatedFrameAndControl(GPSTestClock& clock)
{
    UBX::FrameDecoder decoder;
    const Bytes payload = {0x06, 0x24};
    const auto bytes = ubxFrame(Msg::ACK_ACK.value(), payload);
    std::array<uint8_t, 10> encoded{};
    QVERIFY(std::ranges::equal(UBX::encodeFrame(Msg::ACK_ACK, payload, encoded), bytes));
    QVERIFY(UBX::encodeFrame(Msg::ACK_ACK, payload, std::span(encoded).first(9)).empty());
    std::optional<UBX::Frame> frame;
    for (size_t index = 0; index < bytes.size(); ++index) {
        frame = decoder.consume(bytes[index]);
        QCOMPARE(frame.has_value(), (index + 1 == bytes.size()));
    }
    QCOMPARE(frame->message, Msg::ACK_ACK.value());
    QVERIFY(std::ranges::equal(frame->bytes, bytes) && std::ranges::equal(frame->payload, payload));
    auto corrupt = bytes;
    corrupt.back() ^= 1;
    for (auto byte : corrupt) {
        QVERIFY(!decoder.consume(byte));
    }
    QVERIFY(decoder.idle());
    const auto longFrame = ubxFrame(Msg::INF_NOTICE.value(), Bytes(4096, 0xa5));
    for (auto byte : longFrame) {
        frame = decoder.consume(byte);
    }
    QVERIFY(frame && frame->payload.size() == 4096 && frame->payload.back() == 0xa5);
    QVERIFY(std::ranges::equal(frame->bytes, longFrame));
    for (auto byte : std::span(bytes).first(5)) {
        QVERIFY(!decoder.consume(byte));
    }
    decoder.reset();
    for (auto byte : bytes) {
        frame = decoder.consume(byte);
    }
    QVERIFY(frame && frame->message == Msg::ACK_ACK.value() && std::ranges::equal(frame->payload, payload));

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = FIX_OK;
    const auto validUbx = ubxNavigationEpoch(pvt, 1000);
    const auto validRtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    // A frame longer than the decoder keeps is read to its end and dropped, so the frame after it decodes even when
    // the long payload holds a sync pair whose length reaches into that frame.
    Bytes oversized(UBX::MAX_PAYLOAD_SIZE + 200, 0x5a);
    const Bytes falseHeader{0xb5, 0x62, 0x01, 0x07, 0x40, 0x00};
    std::ranges::copy(falseHeader, oversized.end() - 40);
    Bytes stream = ubxFrame(Msg::INF_NOTICE.value(), oversized);
    stream.insert(stream.end(), validUbx.begin(), validUbx.end());
    stream.insert(stream.end(), validRtcm.begin(), validRtcm.end());
    LoggedRuntime receiver(UBX::FAMILY, makeGPSRuntimeTestIO(clock));
    receiver->armNavigationDecode({.corrections = true});
    const auto recovered = receiver->decode(stream);
    QCOMPARE(recovered.events.size(), 2);
    QVERIFY(std::holds_alternative<GPSDecodedPosition>(recovered.events[0]));
    QVERIFY(std::holds_alternative<GPSRTCMFrame>(recovered.events[1]));
    QVERIFY(!failed(*receiver));

    for (size_t prefix = 1; prefix <= 4; ++prefix) {
        decoder.reset();
        for (size_t i = 0; i < prefix; ++i) {
            QVERIFY(!decoder.consume(0xb5));
        }
        unsigned completed = 0;
        for (auto byte : bytes) {
            if (const auto overlapping = decoder.consume(byte)) {
                ++completed;
                QCOMPARE(overlapping->message, Msg::ACK_ACK.value());
                QVERIFY(std::ranges::equal(overlapping->payload, payload));
            }
        }
        QCOMPARE(completed, 1);
        QVERIFY(decoder.idle());
    }

    UBX::ReceiverController controller;
    controller.beginAcknowledgement(Msg::CFG_NAV5.value());
    controller.accept(UBX::Acknowledgement{Msg::CFG_RATE.value(), true});
    QCOMPARE(controller.acknowledgement(), GPSCommandOutcome::Pending);
    controller.accept(UBX::Acknowledgement{Msg::CFG_NAV5.value(), false});
    QCOMPARE(controller.acknowledgement(), GPSCommandOutcome::Rejected);
    controller.finishAcknowledgement();
    controller.accept(UBX::Acknowledgement{Msg::CFG_NAV5.value(), true});
    QCOMPARE(controller.acknowledgement(), GPSCommandOutcome::Rejected);

    const std::array<uint32_t, 2> keys{Cfg::NAVSPG_DYNMODEL.id, Cfg::RATE_MEAS.id};
    controller.beginReadback(keys);
    UBX::ConfigurationValues values;
    values.count = 2;
    values.values[0].key = values.values[1].key = keys[0];
    controller.accept(values);
    QVERIFY(!controller.readbackReady());
    values.values[0] = {.key = keys[1], .value = 200};
    values.values[1] = {.key = keys[0], .value = 4};
    controller.accept(values);
    QVERIFY(controller.readbackReady());
    QVERIFY(controller.readback().values[0].value == 4 && controller.readback().values[1].value == 200);
    QVERIFY(!UBX::decodeConfigurationValues(Bytes{1, 0, 0, 0, 1}));
}

void checkedWireCodecs(GPSTestClock&)
{
    const auto fixed = []<typename T>() {
        Bytes payload(UBX::WIRE_SIZE<T>, 0);
        QVERIFY(validPayload(T::ID.value(), payload));
        QVERIFY(!validPayload(T::ID.value(), {}));
        payload.pop_back();
        QVERIFY(!validPayload(T::ID.value(), payload));
        payload.resize(UBX::WIRE_SIZE<T> + 1);
        QVERIFY(!validPayload(T::ID.value(), payload));
    };
    fixed.operator()<UBX::NavDop>();
    fixed.operator()<UBX::NavPvt>();
    fixed.operator()<UBX::NavStatus>();
    fixed.operator()<UBX::NavSvin>();
    fixed.operator()<UBX::NavHpposllh>();
    fixed.operator()<UBX::Ack>();
    QVERIFY(validPayload(Msg::NAV_PVT.value(), Bytes(84)));

    Bytes rf(28, 0);
    rf[1] = 1;
    QVERIFY(validPayload(Msg::MON_RF.value(), rf));
    rf[1] = 2;
    QVERIFY(!validPayload(Msg::MON_RF.value(), rf));
    rf[1] = 1;
    rf[0] = 99;
    QVERIFY(!validPayload(Msg::MON_RF.value(), rf));
    QVERIFY(validPayload(Msg::SEC_SIG.value(), Bytes{2, 7, 0, 0}));
    QVERIFY(!validPayload(Msg::SEC_SIG.value(), Bytes{99, 7, 0, 0}));
    QVERIFY(!validPayload(Msg::SEC_SIG.value(), Bytes{2, 7, 0, 1}));
    QCOMPARE(Wire::decode<UBX::Ack>(Bytes{6, 0x24}).msg, Msg::CFG_NAV5.value());

    for (const auto [key, value] : std::array<UBX::ConfigurationValue, 5>{
             {{0x50000001, 0}, {0x10000001, 2}, {0x20000001, 256}, {0x30000001, 65536}, {0x00000001, 0}}}) {
        UBX::CheckedValsetBatch<32> batch;
        QVERIFY(batch.append(0x20000001, 1));
        const auto previousSize = batch.size;
        QVERIFY(!batch.append(key, value));
        QVERIFY(!batch.append(0x20000002, 2));
        QVERIFY(batch.size == previousSize && batch.payload().empty());
    }
    UBX::CheckedValsetBatch<28> batch;
    QVERIFY(batch.payload().empty());
    QVERIFY(batch.append(0x10000001, 1));
    QVERIFY(batch.append(0x20000002, 255));
    QVERIFY(batch.append(0x30000003, 65535));
    QVERIFY(batch.append(0x40000004, UINT32_MAX));
    QCOMPARE(batch.size, batch.bytes.size());
    auto values = batch.bytes;
    QVERIFY(!UBX::decodeConfigurationValues(values));  // VALSET headers cannot masquerade as VALGET.
    values[0] = 1;
    values[1] = 0;
    const auto decoded = UBX::decodeConfigurationValues(values);
    QVERIFY(decoded && decoded->count == 4);
    QVERIFY(decoded->values[0].value == 1 && decoded->values[1].value == 255 && decoded->values[2].value == 65535 &&
            decoded->values[3].value == UINT32_MAX);
    for (size_t length = 1; length < values.size(); ++length) {
        if (length != 4 && length != 9 && length != 14 && length != 20) {
            QVERIFY(!UBX::decodeConfigurationValues(std::span(values).first(length)));
        }
    }
    values[8] = 2;
    QVERIFY(!UBX::decodeConfigurationValues(values));
    QVERIFY(!batch.append(0x20000005, 0));
    QVERIFY(batch.payload().empty());
    batch = {};
    QVERIFY(batch.append(0x20000005, 0));
    QVERIFY(!batch.payload().empty());
}

void optionalCommandWriteEvidence(GPSTestClock& clock)
{
    for (const auto key : {Cfg::UART1INPROT_SPARTN.id, Cfg::ODO_USE_ODO.id}) {
        for (const auto failure : {GPSWriteStatus::Error, GPSWriteStatus::Cancelled}) {
            const GPSProtocolLogCapture log;
            UBXReceiver receiver(clock);
            receiver.model.failValsetKey = key;
            receiver.model.valsetWriteFailure = failure;
            GPSProtocolRuntime driver(UBX::FAMILY, ubxLink(receiver), receiver.observer());
            GPSConfig config;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
            unsigned baud = 115200;
            QVERIFY(!driver.configure(config, baud));
            QVERIFY(!receiver.log.commands.empty());
            const auto& result = receiver.log.commands.back();
            QCOMPARE(result.command, UBX::messageName(Msg::CFG_VALSET));
            QVERIFY(!result.required);
            QCOMPARE(result.outcome, (failure == GPSWriteStatus::Cancelled ? GPSCommandOutcome::Cancelled
                                                                           : GPSCommandOutcome::TransportError));
            QVERIFY(result.acceptedBytes == 9 && result.writtenBytes == 7);
        }
    }
}

/// A pre-protocol-27 base needs the station position and one constellation's observations; firmware may reject the
/// other RTCM messages.
void legacyRTCMActivation(GPSTestClock& clock)
{
    const std::vector<uint16_t> msm7{Msg::RTCM3_1077.value(), Msg::RTCM3_1087.value(), Msg::RTCM3_1097.value(),
                                     Msg::RTCM3_1127.value()};
    const std::vector<uint16_t> withoutStation{Msg::RTCM3_1005.value()};
    const std::vector<uint16_t> withoutBiases{Msg::RTCM3_1230.value()};
    const std::vector<uint16_t> withoutGalileoAndBeiDou{Msg::RTCM3_1097.value(), Msg::RTCM3_1127.value()};
    std::vector<uint16_t> withoutRTCM = msm7;
    withoutRTCM.insert(withoutRTCM.end(), {Msg::RTCM3_1005.value(), Msg::RTCM3_1230.value()});

    const struct
    {
        const std::vector<uint16_t>& unsupported;
        bool active;
    } cases[] = {
        {withoutBiases, true}, {withoutGalileoAndBeiDou, true}, {withoutStation, false}, {msm7, false},
        {withoutRTCM, false},
    };

    const QStringList rejected{
        QStringLiteral("Receiver rejected the RTCM station position or every observation message")};
    for (const auto& scenario : cases) {
        for (bool fixed : {true, false}) {
            Fixture f(clock);
            f.receiver.model.legacy = true;
            f.receiver.model.module = "NEO-M8P";
            f.receiver.model.unsupportedMessages = scenario.unsupported;
            if (fixed) {
                f.base = {.mode = GPSBaseStationConfig::Fixed{
                              .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                              .accuracyMeters = 1}};
                QCOMPARE(f.configure(), scenario.active);
                QCOMPARE(f.driver.receiverReady(), scenario.active);
            } else {
                QVERIFY(f.configure());
                f.log.clear();
                f.receiver.model.queueSurveyReply(SurveyReply::Valid);
                (void) f.driver.receive(100ms);
                QCOMPARE(failed(f.driver), !scenario.active);
                QCOMPARE(f.driver.error(), (scenario.active ? GPSProtocolError::None : GPSProtocolError::Protocol));
            }
            QCOMPARE(f.log.warnings(), (scenario.active ? QStringList{} : rejected));
            // Every RTCM output is requested, whichever the firmware rejects.
            const auto& evidence = f.evidence();
            const auto requested = std::count_if(evidence.begin(), evidence.end(), [](const auto& command) {
                return command.command == UBX::messageName(Msg::CFG_MSG) &&
                       command.outcome == GPSCommandOutcome::Rejected;
            });
            if (fixed) {
                QCOMPARE(static_cast<size_t>(requested), scenario.unsupported.size());
            }
            QCOMPARE(f.receiver.model.messageRates.contains(Msg::RTCM3_1077.value()),
                     (std::ranges::find(scenario.unsupported, Msg::RTCM3_1077.value()) == scenario.unsupported.end()));
        }
    }
}

/// A receiver preset, as the goldens drive it, configured as a survey-in base.
struct ProfileBench
{
    explicit ProfileBench(GPSTestClock& clock, UBXReceiverModel::Receiver preset = UBXReceiverModel::Receiver::F9P)
        : receiver(clock, preset)
        , runtime(UBX::FAMILY, receiver.io(), receiver.observer())
    {}

    bool configure()
    {
        GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2, .duration = 180s}};
        unsigned baud = 115200;
        return runtime.configure(config, baud);
    }

    bool wroteKey(uint32_t key) const
    {
        QByteArray bytes(4, '\0');
        (void) LittleEndian::write(mutableBytesOf(bytes), 0, key);
        return std::ranges::any_of(receiver.commands(), [&](const QByteArray& write) { return write.contains(bytes); });
    }

    /// The configuration commands from the VALSET that the first readback settles.
    std::vector<GPSConfigurationEvidence> fromSettledValset() const
    {
        const auto& evidence = receiver.log.commands;
        const auto readback = std::ranges::find_if(
            evidence, [](const auto& command) { return command.command == "UBX-CFG-VALSET readback"; });
        if (readback == evidence.begin() || readback == evidence.end()) {
            return {};
        }
        return {std::prev(readback), evidence.end()};
    }

    const GPSProtocolLogCapture log;
    UBXReceiver receiver;
    GPSProtocolRuntime runtime;
};

/// CFG-VALSET acknowledgements do not name the batch, so an optional batch answered late is settled by readback before
/// anything else is written.
void lateOptionalValsetReply(GPSTestClock& clock)
{
    const QByteArray valset = UBX::messageName(Msg::CFG_VALSET);
    const QByteArray readback = "UBX-CFG-VALSET readback";
    {
        // A late ACK and matching values accept CFG-SEC-JAMDET, so the older interference monitor is not configured.
        ProfileBench bench(clock);
        bench.receiver.model.delayOptionalAck = true;
        QVERIFY(bench.configure());
        QVERIFY(bench.runtime.receiverReady());
        QCOMPARE(bench.receiver.model.optionalAckDelays, 1);
        const auto commands = bench.fromSettledValset();
        QVERIFY(commands.size() > 2);
        QCOMPARE(commands[0].outcome, GPSCommandOutcome::TimedOut);
        QVERIFY(commands[1].command == readback && !commands[1].required);
        QCOMPARE(commands[1].outcome, GPSCommandOutcome::ReadbackVerified);
        QVERIFY(commands[2].command == valset && commands[2].outcome == GPSCommandOutcome::Acknowledged);
        QVERIFY(!bench.wroteKey(Cfg::ITFM_ENABLE.id));
        QVERIFY(bench.log.warnings().empty());
    }
    {
        // A late NAK rejects the batch rather than the next one: firmware without CFG-SEC-JAMDET gets CFG-ITFM.
        ProfileBench bench(clock);
        bench.receiver.model.delayOptionalAck = true;
        bench.receiver.model.delayOptionalNak = true;
        QVERIFY(bench.configure());
        const auto commands = bench.fromSettledValset();
        QVERIFY(commands.size() > 2);
        QCOMPARE(commands[0].outcome, GPSCommandOutcome::TimedOut);
        QVERIFY(commands[1].command == readback && commands[1].outcome == GPSCommandOutcome::Rejected);
        QVERIFY(commands[2].command == valset && commands[2].outcome == GPSCommandOutcome::Acknowledged);
        QVERIFY(bench.wroteKey(Cfg::ITFM_ENABLE.id));
        QVERIFY(bench.log.warnings().empty());
    }
    {
        // Without a late reply or a readback the batch stays ambiguous, and nothing more is written.
        ProfileBench bench(clock);
        bench.receiver.model.delayOptionalAck = true;
        bench.receiver.model.faultReadbackKey = Cfg::SEC_JAMDET_SENSITIVITY_HI.id;
        bench.receiver.model.readbackReply = UBXReceiverModel::ReadbackReply::Timeout;
        QVERIFY(!bench.configure());
        QVERIFY(!bench.runtime.receiverReady());
        const auto commands = bench.fromSettledValset();
        QCOMPARE(commands.size(), 2);
        QVERIFY(commands[1].command == readback && commands[1].outcome == GPSCommandOutcome::TimedOut);
        QVERIFY(!bench.wroteKey(Cfg::ITFM_ENABLE.id));
        QCOMPARE(bench.log.warnings(), QStringList{QStringLiteral("CFG-SEC-JAMDET probe unanswered")});
    }
    {
        // A required batch fails on its timeout; no readback can accept it.
        ProfileBench bench(clock);
        bench.receiver.model.disableReply = UBXReceiverModel::DisableReply::Timeout;
        QVERIFY(!bench.configure());
        const auto& evidence = bench.receiver.log.commands;
        QVERIFY(evidence.back().command == valset && evidence.back().required);
        QCOMPARE(evidence.back().outcome, GPSCommandOutcome::TimedOut);
        QVERIFY(std::ranges::none_of(evidence, [&](const auto& command) { return command.command == readback; }));
    }
}

/// An ACK and a NAK of one command decoded from the same read reject it: the NAK stands, whichever came first.
void acknowledgedAndRejectedInOneRead(GPSTestClock& clock)
{
    ProfileBench bench(clock);
    bench.receiver.model.readChunk = GPSCommandChannel::READ_CHUNK_SIZE;
    const std::initializer_list<uint8_t> valset{0x06, 0x8a};
    bench.receiver.faults.rules.push_back(
        reply(startsWith(QByteArray("\xb5\x62\x06\x8a", 4)),
              ubxBytes(Msg::ACK_ACK.value(), valset) + ubxBytes(Msg::ACK_NAK.value(), valset), 1, 1));
    QVERIFY(!bench.configure());
    QCOMPARE(bench.runtime.error(), GPSProtocolError::Protocol);
    const auto& rejected = bench.receiver.log.commands.back();
    QCOMPARE(rejected.command, UBX::messageName(Msg::CFG_VALSET));
    QVERIFY(rejected.required);
    QCOMPARE(rejected.outcome, GPSCommandOutcome::Rejected);
}

/// A NAK of an optional batch, or of its readback, proves its keys unknown and nothing more of it to arrive, also when
/// readback stands in for a baud-change ACK lost in the UART handoff. The batch is rejected rather than ambiguous: its
/// fallback follows and later batches are written.
void rejectedOptionalBatch(GPSTestClock& clock)
{
    const QByteArray valset = UBX::messageName(Msg::CFG_VALSET);
    const QByteArray readback = "UBX-CFG-VALSET readback";

    const struct
    {
        uint32_t key;
        bool baudAckLost;
        bool batchNakLost;
    } cases[] = {
        // F9 firmware before HPG 1.50 rejects CFG-SEC-JAMDET and gets CFG-ITFM.
        {Cfg::SEC_JAMDET_SENSITIVITY_HI.id, true, false},
        {Cfg::SEC_JAMDET_SENSITIVITY_HI.id, true, true},
        {Cfg::SEC_JAMDET_SENSITIVITY_HI.id, false, true},
        // Receivers without SPARTN reject a tolerated batch.
        {Cfg::UART1INPROT_SPARTN.id, false, true},
    };

    for (const auto& scenario : cases) {
        clock.reset(GPSTestClock::START_US);
        const GPSProtocolLogCapture log;
        UBXReceiver receiver(clock);
        receiver.model.receiverBaud = 9600;
        receiver.model.protocol = "27.31";
        receiver.model.loseBaudAck = scenario.baudAckLost;
        receiver.model.unsupportedKeys = {scenario.key};
        receiver.model.loseUnsupportedNak = scenario.batchNakLost;
        GPSProtocolRuntime driver(UBX::FAMILY, ubxLink(receiver), receiver.observer());
        const auto& evidence = receiver.log.commands;
        GPSConfig config{};
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
        unsigned baud = 0;
        QVERIFY(driver.configure(config, baud));
        QVERIFY(driver.receiverReady());
        QVERIFY(log.warnings().isEmpty());
        QVERIFY(!receiver.model.currentSettings.contains(scenario.key));
        QCOMPARE(receiver.model.currentSettings.contains(Cfg::ITFM_ENABLE.id),
                 (scenario.key == Cfg::SEC_JAMDET_SENSITIVITY_HI.id));
        QCOMPARE(receiver.model.currentSettings.at(Cfg::MSGOUT_UBX_NAV_PVT.port(UBX::MsgOutPort::UART1).id), 1);
        QCOMPARE(std::ranges::count(evidence, GPSCommandOutcome::Rejected, &GPSConfigurationEvidence::outcome), 1);
        const auto rejected =
            std::ranges::find(evidence, GPSCommandOutcome::Rejected, &GPSConfigurationEvidence::outcome);
        QCOMPARE(rejected->command, readback);
        QCOMPARE(std::prev(rejected)->command, valset);
        QCOMPARE(std::prev(rejected)->outcome,
                 (scenario.baudAckLost ? GPSCommandOutcome::Written : GPSCommandOutcome::TimedOut));
    }
}

/// An M8 may acknowledge a CFG-MSG rate up to a second late, so an unanswered RTCM rate is polled, and the rates the
/// poll returns for that message confirm or reject it. After a poll that expires with a reply still to come, no more
/// rates are written, as a late reply would be taken for the next rate's: activation then needs the station position
/// and an observation message confirmed before it.
void legacyRTCMRatePolls(GPSTestClock& clock)
{
    using RateAck = UBXReceiverModel::RateAck;
    const std::vector<uint16_t> order{Msg::RTCM3_1005.value(), Msg::RTCM3_1077.value(), Msg::RTCM3_1087.value(),
                                      Msg::RTCM3_1230.value(), Msg::RTCM3_1097.value(), Msg::RTCM3_1127.value()};
    const uint16_t station = order[0];
    const uint16_t gps = order[1];
    const uint16_t glonass = order[2];
    const uint16_t biases = order[3];

    const struct
    {
        uint16_t message;
        RateAck ack;
        bool silentPoll;
        std::chrono::milliseconds pollAckDelay;
        std::vector<uint16_t> unsupported;
        GPSCommandOutcome poll;
        bool stops;
        bool active;
    } cases[] = {
        // The late ACK answers its own rate, so the rejection of the next rate but one is still attributed to it.
        {gps, RateAck::Late, false, 0ms, {biases}, GPSCommandOutcome::ReadbackVerified, false, true},
        // The polled rates of an ignored command are not the new ones.
        {biases, RateAck::Ignored, false, 0ms, {}, GPSCommandOutcome::Rejected, false, true},
        // One NAK before the poll expires rejects the rate, but it may be the rate's late NAK with the poll's reply
        // still to come: whether the rate's NAK was lost, or it came late and the poll's NAK after the deadline.
        {biases, RateAck::Lost, false, 0ms, {biases}, GPSCommandOutcome::Rejected, true, true},
        {biases, RateAck::Late, false, 1100ms, {biases}, GPSCommandOutcome::Rejected, true, true},
        // The polled rates confirm the rate, but the poll's ACK comes after the deadline.
        {gps, RateAck::Lost, false, 1100ms, {biases}, GPSCommandOutcome::ReadbackVerified, true, true},
        // After a silent poll, the station position and GPS observations suffice; less does not.
        {glonass, RateAck::Lost, true, 0ms, {}, GPSCommandOutcome::TimedOut, true, true},
        {gps, RateAck::Lost, true, 0ms, {}, GPSCommandOutcome::TimedOut, true, false},
        {station, RateAck::Lost, true, 0ms, {}, GPSCommandOutcome::TimedOut, true, false},
    };

    const QByteArray rate = UBX::messageName(Msg::CFG_MSG);
    const QStringList unanswered{
        QStringLiteral("Receiver did not answer a CFG-MSG rate or its poll; later RTCM messages are not configured")};
    for (const auto& scenario : cases) {
        Fixture f(clock);
        f.receiver.model.legacy = true;
        f.receiver.model.module = "NEO-M8P";
        f.receiver.model.rateAckMessage = scenario.message;
        f.receiver.model.rateAck = scenario.ack;
        f.receiver.model.silentRatePoll = scenario.silentPoll;
        f.receiver.model.ratePollAckDelay = scenario.pollAckDelay;
        f.receiver.model.unsupportedMessages = scenario.unsupported;
        f.base = {.mode = GPSBaseStationConfig::Fixed{
                      .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                      .accuracyMeters = 1}};
        QCOMPARE(f.configure(), scenario.active);
        QCOMPARE(f.receiver.model.ratePolls, 1);
        QCOMPARE(f.log.warnings(), (scenario.stops ? unanswered : QStringList{}));
        const auto& evidence = f.evidence();
        const auto poll = std::ranges::find(
            evidence,
            "UBX-CFG-MSG " + UBX::messageName({uint8_t(scenario.message), uint8_t(scenario.message >> 8)}) +
                " readback",
            &GPSConfigurationEvidence::command);
        QVERIFY(poll != evidence.end() && poll->outcome == scenario.poll);
        QVERIFY(std::prev(poll)->command == rate && std::prev(poll)->outcome == GPSCommandOutcome::TimedOut);
        std::vector<GPSCommandOutcome> expected;
        if (!scenario.stops) {
            for (auto next = std::ranges::find(order, scenario.message) + 1; next != order.end(); ++next) {
                expected.push_back(std::ranges::find(scenario.unsupported, *next) == scenario.unsupported.end()
                                       ? GPSCommandOutcome::Acknowledged
                                       : GPSCommandOutcome::Rejected);
            }
        }
        std::vector<GPSCommandOutcome> later;
        for (auto command = std::next(poll); command != evidence.end(); ++command) {
            if (command->command == rate) {
                later.push_back(command->outcome);
            }
        }
        QCOMPARE(later, expected);
        QCOMPARE(f.receiver.model.messageRates.at(order.back()), (scenario.stops ? 0 : 1));
    }
}

/// A slow M8 answers each command up to a second late, so RTCM activation after survey-in does not fit one streaming
/// service. Activation waits for the base rate's ACK, sends a rate only with time left for its poll, and resumes in the
/// next service: the session and the finished survey survive, and every RTCM message is configured.
void slowLegacyRTCMActivation(GPSTestClock& clock)
{
    for (const auto delay : {700ms, 950ms}) {
        Fixture f(clock);
        f.receiver.model.legacy = true;
        f.receiver.model.module = "NEO-M8P";
        QVERIFY(f.configure());
        const auto modes = f.receiver.model.modes;
        const auto activated = [&f] { return f.receiver.model.messageRates.at(Msg::RTCM3_1127.value()) != 0; };
        f.receiver.model.replyDelay = delay;
        f.receiver.model.queueSurveyReply(SurveyReply::Valid);
        const uint64_t started = clock.nowUs();
        (void) f.driver.receive(100ms);
        // The first service stops within its budget and leaves the rest to the next service.
        QVERIFY(clock.nowUs() - started <
                static_cast<uint64_t>(std::chrono::microseconds(100ms + GPSCommandChannel::SERVICE_TIMEOUT).count()));
        QVERIFY(!activated());
        for (int service = 0; service < 4 && !activated(); ++service) {
            (void) f.driver.receive(100ms);
        }
        QVERIFY(activated());
        // The finished activation sends nothing more.
        const unsigned writes = f.receiver.model.configurationWrites;
        (void) f.driver.receive(100ms);
        QCOMPARE(f.receiver.model.configurationWrites, writes);
        QVERIFY(!failed(f.driver));
        QCOMPARE(f.receiver.model.modes, modes);
        for (const auto& output : UBX::Plan::legacyRTCMOutput(false)) {
            QCOMPARE(f.receiver.model.messageRates.at(output.rate.message.value()), output.rate.rate);
        }
        QVERIFY(f.log.warnings().isEmpty());
    }
}

/// A survey-stop poll that cannot be written, or whose reply wait is cancelled, ends the configuration before the
/// survey restarts.
void surveyStopPollFailures(GPSTestClock& clock)
{
    const QByteArray poll("\xb5\x62\x01\x3b", 4);
    for (const bool writeFails : {true, false}) {
        clock.reset();
        ProfileBench bench(clock);
        if (writeFails) {
            bench.receiver.faults.rules.push_back(
                GPSTest::writeFails(poll, [](const QByteArray&) { return GPSWriteResult{GPSWriteStatus::Error}; }));
        } else {
            bench.receiver.faults.rules.push_back(afterCommand(poll, [](ScriptedReceiver& link) { link.cancel(); }));
        }
        QVERIFY(!bench.configure());
        QCOMPARE(bench.receiver.model.starts, 0);
        QCOMPARE(bench.receiver.log.commands.back().command, QByteArray("UBX-NAV-SVIN stopped"));
        if (writeFails) {
            QCOMPARE(bench.runtime.error(), GPSProtocolError::Transport);
            QCOMPARE(bench.runtime.errorDetail(),
                     QStringLiteral("Could not write 'UBX-NAV-SVIN stopped' to the u-blox receiver"));
        } else {
            QCOMPARE(bench.runtime.error(), GPSProtocolError::Cancelled);
        }
    }
}

/// A receiver without base support is identified and left as it is: the MON-VER poll is the only command. The
/// profile-m8n-survey golden pins the transcript.
void receiversWithoutBase(GPSTestClock& clock)
{
    using Receiver = UBXReceiverModel::Receiver;

    const struct
    {
        Receiver receiver;
        const char* identity;
        const char* error;
    } receivers[] = {
        {Receiver::M9N, "NEO-M9N SPG 4.04", "NEO-M9N cannot run as an RTK base station"},
        {Receiver::M10, "MAX-M10S SPG 5.10", "MAX-M10S cannot run as an RTK base station"},
        {Receiver::M8PRover, "NEO-M8P-0 HPG 1.40", "NEO-M8P-0 cannot run as an RTK base station"},
        {Receiver::F9R, "ZED-F9R HPS 1.30", "ZED-F9R cannot run as an RTK base station"},
        {Receiver::Unidentified, "UNKNOWN UNKNOWN", "UNKNOWN cannot run as an RTK base station"},
        {Receiver::U6, "EXT CORE", "This u-blox receiver cannot run as an RTK base station"},
        {Receiver::M8NEarly, "EXT CORE", "This u-blox receiver cannot run as an RTK base station"},
    };

    for (const auto& row : receivers) {
        clock.reset();
        ProfileBench bench(clock, row.receiver);
        QVERIFY2(!bench.configure(), row.identity);
        QCOMPARE(bench.runtime.error(), GPSProtocolError::Protocol);
        QCOMPARE(bench.runtime.identity(), QString::fromLatin1(row.identity));
        QCOMPARE(bench.runtime.errorDetail(), QString::fromLatin1(row.error));
        QCOMPARE(bench.receiver.commands().size(), 1);
        QCOMPARE(bench.receiver.log.commands.size(), 1u);
        QCOMPARE(bench.receiver.log.commands.front().outcome, GPSCommandOutcome::Acknowledged);
    }
}

// Recorded and synthetic fixtures with independently decoded expectations.

/// The u-blox family without a link, decoding navigation output.
struct NavigationDecoder : LoggedRuntime
{
    explicit NavigationDecoder(GPSTestClock& clock)
        : LoggedRuntime(UBX::FAMILY, makeDecoderOnlyIO(clock))
    {
        runtime.armNavigationDecode();
    }
};

std::vector<uint8_t> timed(std::vector<uint8_t> frame, uint32_t tow, size_t offset = 0)
{
    for (size_t byte = 0; byte < 4; ++byte) {
        frame[6 + offset + byte] = tow >> (8 * byte);
    }
    ubxChecksum(frame);
    return frame;
}

std::vector<uint8_t> endEpoch(uint32_t tow)
{
    return timed({0xb5, 0x62, 1, 0x61, 4, 0, 0, 0, 0, 0, 0, 0}, tow);
}

void independentIntegrity(GPSTestClock& clock)
{
    const auto integrity = fixtureBytes("synthetic-integrity.ubx");
    QVERIFY(integrity);
    const auto& integrityBytes = *integrity;
    for (const size_t chunkSize : {1u, 7u, 256u}) {
        clock.advanceBy(1000000);
        NavigationDecoder decoder(clock);
        auto& ubx = *decoder;
        for (const auto& expected : GPSFixture::integrity) {
            auto remaining = std::span(integrityBytes).subspan(expected.offset, expected.size);
            auto corrupt = std::vector<uint8_t>(remaining.begin(), remaining.end());
            corrupt.back() ^= 1;
            QVERIFY(ubx.decode(corrupt).events.empty());
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                const auto decoded = ubx.decode(remaining.first(count));
                remaining = remaining.subspan(count);
                if (!remaining.empty()) {
                    QVERIFY(decoded.events.empty());
                    continue;
                }
                QCOMPARE(decoded.events.size(), 1);
                const auto& actual = std::get<GPSIntegrityReport>(decoded.events.front());
                QCOMPARE(static_cast<unsigned>(actual.spoofing.state), expected.spoofing);
                QCOMPARE(static_cast<unsigned>(actual.jamming.state), expected.jamming);
                QCOMPARE_NE(actual.spoofing.timestampUs, 0);
                QCOMPARE(actual.jamming.timestampUs != 0, expected.jamming != 0);
            }
        }
    }
}

void navigationEpochs(GPSTestClock& clock)
{
    for (bool before : {false, true}) {
        NavigationDecoder decoder(clock);
        auto& ubx = *decoder;
        const auto observations = [&decoder] { return decoder.reports<GPSDecodedPosition>(); };
        const auto pvtFixture = fixtureBytes(NAV_PVT);
        const auto dopFixture = fixtureBytes(NAV_DOP);
        const auto hpFixture = fixtureBytes("nav-hpposllh.ubx");
        QVERIFY(pvtFixture && dopFixture && hpFixture);
        const auto pvt = timed(*pvtFixture, GPSTime::WEEK_MS - 1000);
        const auto dop = timed(*dopFixture, GPSTime::WEEK_MS - 1000);
        const auto hp = timed(*hpFixture, GPSTime::WEEK_MS - 1000, 4);
        ubx.consume(before ? dop : pvt);
        ubx.consume(hp);
        ubx.consume(before ? pvt : dop);
        QVERIFY(observations().empty());
        ubx.consume(endEpoch(GPSTime::WEEK_MS - 1000));
        QCOMPARE(observations().size(), 1);
        QVERIFY(std::abs(observations().back().navigation.latitudeDegrees - 53.337816927) < 1e-9);
        QVERIFY(std::abs(observations().back().navigation.horizontalDop - 0.58) < 1e-6);
        QCOMPARE_NE(observations().back().navigation.utcTimeUs, 0);
        ubx.consume(pvt);
        ubx.consume(endEpoch(GPSTime::WEEK_MS - 1000));
        QCOMPARE(observations().size(), 1);
        ubx.consume(timed(pvt, 0));
        clock.advanceBy(UBX::EpochAssembler::MAX_AGE);
        ubx.consume({});
        QCOMPARE(observations().size(), 2);
        QVERIFY(std::isnan(observations().back().navigation.horizontalDop));
        QVERIFY(std::abs(observations().back().navigation.latitudeDegrees - 53.4507228) < 1e-8);
        // Adjacent epochs can interleave without donating DOP to one another.
        ubx.consume(timed(pvt, 1000));
        ubx.consume(timed(dop, 2000));
        ubx.consume(timed(dop, 1000));
        ubx.consume(endEpoch(1000));
        QCOMPARE(observations().size(), 3);
        ubx.consume(timed(pvt, 2000));
        ubx.consume(endEpoch(2000));
        QCOMPARE(observations().size(), 4);
        QVERIFY(std::abs(observations().back().navigation.horizontalDop - 0.58) < 1e-6);
        // Metadata-only epochs never manufacture a position.
        ubx.consume(timed(dop, 3000));
        clock.advanceBy(UBX::EpochAssembler::MAX_AGE);
        ubx.consume({});
        QCOMPARE(observations().size(), 4);
        auto fixed = timed(pvt, 4000);
        fixed[6 + 21] = 0x81;
        ubxChecksum(fixed);
        ubx.consume(fixed);
        clock.advanceBy(UBX::EpochAssembler::MAX_AGE);
        ubx.consume({});
        QCOMPARE(observations().size(), 5);
        QCOMPARE(observations().back().navigation.fixType, GPSPositionReport::FixType::RTKFixed);
        QVERIFY(std::abs(observations().back().navigation.latitudeDegrees - 53.4507228) < 1e-8);
    }
}

void independentSequences(GPSTestClock& clock)
{
    NavigationDecoder decoder(clock);
    auto& ubx = *decoder;
    const auto& position = decoder.position;
    const auto navigation = fixtureBytes("navigation.ubx");
    QVERIFY(navigation);
    // Byte by byte; the recording's NAV-EOE publishes the epoch of its NAV-PVT.
    for (const auto byte : *navigation) {
        ubx.consume({&byte, 1});
    }
    const auto& expectedPosition = GPSFixture::positions[0];
    QVERIFY(std::abs(position.navigation.latitudeDegrees - expectedPosition.latitude) < 1e-8);
    QVERIFY(std::abs(position.navigation.longitudeDegrees - expectedPosition.longitude) < 1e-8);
    QVERIFY(std::abs(position.navigation.altitudeMslMeters - expectedPosition.altitude) < 1e-6);
    const auto satellites = decoder.latest<GPSDecodedSatellites>();
    int satelliteCount = 0;
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS};
    for (uint8_t i = 0; i < satellites.count; ++i) {
        const auto& actual = satellites.constellations[i];
        satelliteCount += actual.inView;
        QVERIFY(std::any_of(std::begin(systems), std::end(systems),
                            [&](GPSConstellation constellation) { return constellation == actual.constellation; }));
    }
    QCOMPARE(satelliteCount, static_cast<int>(std::size(GPSFixture::satellites)));

    const auto mixedFixture = fixtureBytes("mixed.gps");
    QVERIFY(mixedFixture);
    const auto& mixed = *mixedFixture;
    for (const auto& expected : GPSFixture::corrections) {
        RTCMFramer frame;
        for (unsigned i = 0; i < expected.size; ++i) {
            QCOMPARE(frame.addByte(mixed[expected.offset + i]), (i + 1 == expected.size));
        }
        QVERIFY(frame.valid() && frame.messageId() == expected.id);
        QCOMPARE(frame.frame().size(), expected.size);
        std::vector<uint8_t> corrupt(frame.frame().begin(), frame.frame().end());
        corrupt.back() ^= 1;
        QVERIFY(!RTCMFramer::isValidFrame(std::span<const uint8_t>(corrupt)));
    }
    ubx.consume(mixed);
    clock.advanceBy(UBX::EpochAssembler::MAX_AGE);
    ubx.consume({});
    QVERIFY(std::abs(position.navigation.latitudeDegrees - 32.0658325) < 1e-8);
    const auto nmea = fixtureBytes("gga.nmea");
    QVERIFY(nmea);
    const auto sentence = NMEA::sentence({reinterpret_cast<const char*>(nmea->data()), nmea->size()});
    QVERIFY(sentence);
    const auto gga = NMEA::gga(*sentence);
    QVERIFY(gga);
    QVERIFY(std::abs(gga->latitude - GPSFixture::ggaLatitude) < 1e-8);
    QVERIFY(std::abs(gga->longitude - GPSFixture::ggaLongitude) < 1e-8);
    QVERIFY(std::abs(gga->altitude - GPSFixture::ggaAltitude) < 1e-6);
    QCOMPARE(gga->satellitesUsed, GPSFixture::ggaSatellites);
}

/// The recorded NAV-PVT and NAV-SAT, and the receiver profile and schema tables they rely on.
void recordedNavigation(GPSTestClock& clock)
{
    QVERIFY(!UBX::receiverProfile(UBX::Board::u_blox9).rtcmOutput);
    QVERIFY(UBX::receiverProfile(UBX::Board::u_blox9_F9P_L1L2).rtcmOutput);
    QCOMPARE(UBX::configurationValueBytes(0x50000001), 0);
    QCOMPARE(UBX::configurationValueBytes(0x30000001), 2);
    for (const auto& schema : UBX::MESSAGE_SCHEMAS) {
        std::vector<uint8_t> shortPayload(schema.minimum - 1);
        QVERIFY(!validPayload(schema.message, shortPayload));
        std::vector<uint8_t> longPayload(schema.maximum + 1);
        QVERIFY(!validPayload(schema.message, longPayload));
    }
    NavigationDecoder decoder(clock);
    auto& ubx = *decoder;
    const auto& position = decoder.position;
    const auto recordedPvt = fixtureBytes(NAV_PVT);
    const auto recordedSatellites = fixtureBytes(NAV_SAT);
    QVERIFY(recordedPvt && recordedSatellites);
    const auto& pvt = *recordedPvt;
    for (auto byte : pvt) {
        ubx.consume({&byte, 1});
    }
    // Without NAV-EOE, the epoch is published at its deadline.
    clock.advanceBy(UBX::EpochAssembler::MAX_AGE);
    ubx.consume({});
    QVERIFY(std::abs(position.navigation.latitudeDegrees - 53.4507228) < 1e-8);
    QVERIFY(std::abs(position.navigation.longitudeDegrees + 2.2402855) < 1e-8);
    QVERIFY(std::abs(position.navigation.altitudeMslMeters - 42.701) < 1e-6);
    QVERIFY(position.velocityValid);
    QCOMPARE(position.navigation.satellitesUsed, 26);
    ubx.consume(*recordedSatellites);
    const auto satellites = decoder.latest<GPSDecodedSatellites>();
    int satelliteCount = 0;
    for (uint8_t index = 0; index < satellites.count; ++index) {
        satelliteCount += satellites.constellations[index].inView;
    }
    QCOMPARE(satelliteCount, 43);
    const auto timestamp = position.navigation.timestampUs;
    auto corrupt = pvt;
    corrupt[30] ^= 1;
    QCOMPARE(ubx.consume(corrupt), GPSReceiveUpdates{});
    QCOMPARE(position.navigation.timestampUs, timestamp);
}

void reconfigureDoesNotReuseStopConfirmation(GPSTestClock& clock)
{
    Fixture f(clock);
    f.success(1);
    f.receiver.model = UBXReceiverModel(clock);
    f.receiver.setModel(&f.receiver.faults);
    f.receiver.model.surveyReplies = {SurveyReply::Silent};
    f.timeout();
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"isolated-frame-control", isolatedFrameAndControl},
    {"checked-wire-codecs", checkedWireCodecs},
    {"optional-command-write-evidence", optionalCommandWriteEvidence},
    {"legacy-rtcm-activation", legacyRTCMActivation},
    {"late-optional-valset-reply", lateOptionalValsetReply},
    {"acknowledged-and-rejected-in-one-read", acknowledgedAndRejectedInOneRead},
    {"rejected-optional-batch", rejectedOptionalBatch},
    {"legacy-rtcm-rate-polls", legacyRTCMRatePolls},
    {"slow-legacy-rtcm-activation", slowLegacyRTCMActivation},
    {"integrity-original-receipts", integrityReceipts},
    {"plan-output-decoded", planOutputDecoded},
    {"antenna-states", antennaStates},
    {"output-overflow", outputOverflow},
    {"control-deadline", controlDeadline},
    {"identification-write-budget", identificationWriteBudget},
    {"out-of-range-coordinates", outOfRangeCoordinates},
    {"largest-fixed-accuracy", largestFixedAccuracy},
    {"navigation-fix-flags", navigationFixFlags},
    {"transactional-frames", transactionalFrames},
    {"reentrant-payload", reentrantPayload},
    {"reconfigure-does-not-reuse-stop-confirmation", reconfigureDoesNotReuseStopConfirmation},
    {"survey-stop-poll-failures", surveyStopPollFailures},
    {"receivers-without-base", receiversWithoutBase},
    {"fixture-sequences", independentSequences},
    {"fixture-integrity", independentIntegrity},
    {"fixture-navigation-epochs", navigationEpochs},
    {"fixture-recorded-navigation", recordedNavigation},
};
}  // namespace

void UBXProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void UBXProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

void UBXProtocolTest::_surveyStop_data()
{
    QTest::addColumn<QList<SurveyReply>>("replies");
    QTest::addColumn<int>("polls");
    QTest::addColumn<int>("startDelayUs");
    QTest::newRow("already-stopped") << QList<SurveyReply>{} << 1 << 0;
    QTest::newRow("silent-then-stopped") << QList{SurveyReply::Silent, SurveyReply::Stopped} << 2 << 0;
    QTest::newRow("delayed-stop") << QList{SurveyReply::Active, SurveyReply::Active, SurveyReply::Stopped} << 3
                                  << 200000;
    QTest::newRow("completed-survey-is-not-stopped") << QList{SurveyReply::Valid, SurveyReply::Stopped} << 2 << 0;
    QTest::newRow("bad-checksum-is-not-confirmation")
        << QList{SurveyReply::BadChecksum, SurveyReply::Stopped} << 2 << 0;
    QTest::newRow("bad-length-is-not-confirmation") << QList{SurveyReply::BadLength, SurveyReply::Stopped} << 2 << 0;
    // Zero polls: the stop is never confirmed, and configuration times out.
    QTest::newRow("active-timeout") << QList{SurveyReply::Active} << 0 << 0;
    QTest::newRow("valid-timeout") << QList{SurveyReply::Valid} << 0 << 0;
    QTest::newRow("silent-timeout") << QList{SurveyReply::Silent} << 0 << 0;
}

void UBXProtocolTest::_surveyStop()
{
    QFETCH(QList<SurveyReply>, replies);
    QFETCH(int, polls);
    QFETCH(int, startDelayUs);
    const QTest::ThrowOnFailEnabler endRowOnFailure;
    GPSTestClock clock;
    Fixture f(clock);
    if (!replies.isEmpty()) {
        f.receiver.model.surveyReplies = replies;
    }
    if (polls > 0) {
        f.success(static_cast<unsigned>(polls));
        if (startDelayUs > 0) {
            QCOMPARE_GE(f.receiver.model.startedAt - f.receiver.model.disabledAt, static_cast<uint64_t>(startDelayUs));
        }
    } else {
        f.timeout();
    }
}

void UBXProtocolTest::_replyFaults_data()
{
    using Disable = UBXReceiverModel::DisableReply;
    using Readback = UBXReceiverModel::ReadbackReply;
    QTest::addColumn<int>("disable");
    QTest::addColumn<int>("readback");
    QTest::addColumn<GPSCommandOutcome>("outcome");
    QTest::addColumn<GPSProtocolError>("error");
    // A configuration the receiver refused ends in a protocol failure.
    const auto row = [](const char* name, Disable disable, Readback readback, GPSCommandOutcome outcome,
                        GPSProtocolError error = GPSProtocolError::Protocol) {
        QTest::newRow(name) << int(disable) << int(readback) << outcome << error;
    };
    const auto disable = [&row](const char* name, Disable reply, GPSCommandOutcome outcome,
                                GPSProtocolError error = GPSProtocolError::Protocol) {
        row(name, reply, Readback::Value, outcome, error);
    };
    const auto readback = [&row](const char* name, Readback reply, GPSCommandOutcome outcome,
                                 GPSProtocolError error = GPSProtocolError::Protocol) {
        row(name, Disable::Ack, reply, outcome, error);
    };
    using Outcome = GPSCommandOutcome;
    readback("answered", Readback::Value, Outcome::Acknowledged, GPSProtocolError::None);
    // Disabling the time mode: anything but its acknowledgement fails the configuration.
    disable("disable-nak", Disable::Nak, Outcome::Rejected);
    disable("disable-timeout", Disable::Timeout, Outcome::TimedOut);
    disable("disable-wrong-ack", Disable::WrongAck, Outcome::TimedOut);
    disable("disable-corrupt-ack", Disable::CorruptAck, Outcome::TimedOut);
    // Reading the time mode back: only the written value verifies it, and a malformed answer is no answer.
    readback("readback-nak", Readback::Nak, Outcome::Rejected);
    readback("readback-wrong-value", Readback::WrongValue, Outcome::Rejected);
    readback("readback-timeout", Readback::Timeout, Outcome::TimedOut);
    readback("readback-ack-only", Readback::AckOnly, Outcome::TimedOut);
    readback("readback-wrong-message", Readback::WrongMessage, Outcome::TimedOut);
    readback("readback-wrong-key", Readback::WrongKey, Outcome::TimedOut);
    readback("readback-wrong-layer", Readback::WrongLayer, Outcome::TimedOut);
    readback("readback-wrong-position", Readback::WrongPosition, Outcome::TimedOut);
    readback("readback-wrong-version", Readback::WrongVersion, Outcome::TimedOut);
    readback("readback-corrupt", Readback::Corrupt, Outcome::TimedOut);
    readback("readback-truncated", Readback::Truncated, Outcome::TimedOut);
    readback("readback-duplicate-value", Readback::DuplicateValue, Outcome::TimedOut);
    readback("readback-oversized", Readback::Oversized, Outcome::TimedOut);
    readback("readback-write-error", Readback::WriteError, Outcome::TransportError, GPSProtocolError::Transport);
    readback("readback-read-error", Readback::ReadError, Outcome::TransportError, GPSProtocolError::Transport);
}

void UBXProtocolTest::_replyFaults()
{
    QFETCH(int, disable);
    QFETCH(int, readback);
    QFETCH(GPSCommandOutcome, outcome);
    QFETCH(GPSProtocolError, error);
    GPSTestClock clock;
    ProfileBench bench(clock);
    bench.receiver.model.disableReply = static_cast<UBXReceiverModel::DisableReply>(disable);
    bench.receiver.model.readbackReply = static_cast<UBXReceiverModel::ReadbackReply>(readback);
    const bool answered = outcome == GPSCommandOutcome::Acknowledged;
    QCOMPARE(bench.configure(), answered);
    QCOMPARE(bench.runtime.error(), error);
    QVERIFY(!bench.receiver.log.commands.empty());
    // The last command is the one the fault hit, unless the configuration succeeded.
    const auto& last = bench.receiver.log.commands.back();
    QCOMPARE(last.outcome, outcome);
    if (!answered) {
        QCOMPARE(last.command.startsWith("UBX-CFG-VALGET"), readback != int(UBXReceiverModel::ReadbackReply::Value));
        // An aborted configuration never restarts the survey.
        QCOMPARE(bench.receiver.model.starts, 0);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(UBXProtocolTest, TestLabel::Unit)
