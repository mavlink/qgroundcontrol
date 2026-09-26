#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

#include <QtCore/QtEndian>
#include <QtCore/QtMessageHandler>

#include "Checksums.h"
#include "GPSEventSink.h"
#include "GPSProtocolRuntime.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "Support/ProtocolTestPackets.h"
#include "Support/ScriptedReceiver.h"
#include "Support/UBXReceiverModel.h"
#include "UBX/UBXDecoder.h"
#include "UBX/UBXFamily.h"
#include "UBX/UBXFrameDecoder.h"
#include "UBX/UBXMessageSchema.h"
#include "UBX/UBXPlan.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
using Bytes = std::vector<uint8_t>;
namespace Cfg = UBX::Cfg;
namespace Msg = UBX::Msg;

using SurveyReply = UBXReceiverModel::SurveyReply;

constexpr uint8_t FIX_OK = 0x01;
constexpr unsigned SPOOF_DETECTION_SHIFT = 3;
constexpr unsigned RTCM_USED_SHIFT = 1;

template <typename Key, typename Value>
concept ValsetValue = requires(Key key, Value value) { UBX::Plan::ValsetItem{key, value}; };
// A plan value has its key's type; a value wider than the key's size bits does not compile.
static_assert(ValsetValue<UBX::CfgKey<uint16_t>, uint16_t> && ValsetValue<UBX::MsgOutKey, uint8_t>);
static_assert(!ValsetValue<UBX::CfgKey<uint16_t>, uint32_t> && !ValsetValue<UBX::CfgKey<int8_t>, int32_t>);
static_assert(!ValsetValue<UBX::MsgOutKey, unsigned>);

UBXDecoder& decoderOf(GPSProtocolRuntime& runtime)
{
    return UBX::decoder(runtime.protocol());
}

void setMode(GPSProtocolRuntime& runtime, const UBXDecoder::Mode& mode)
{
    decoderOf(runtime).setMode(mode, runtime.stream());
}

bool failed(const GPSProtocolRuntime& runtime)
{
    return runtime.error() != GPSProtocolError::None;
}

bool positionUpdated(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Position);
}

/// The u-blox family on the runtime without a link, for decode tests.
std::unique_ptr<GPSProtocolRuntime> offlineUBX(GPSTestClock& clock, GPSRuntimeObserver observer = {},
                                               bool satelliteInfo = false)
{
    return std::make_unique<GPSProtocolRuntime>(UBX::FAMILY, makeGPSRuntimeTestIO(clock), std::move(observer),
                                                GPSFamilyOptions{.satelliteInfoEnabled = satelliteInfo});
}

/// Protocol warnings logged while alive; clear() forgets those logged so far.
class Warnings
{
public:
    QStringList list() const { return _capture.warnings().mid(_cleared); }

    void clear() { _cleared = _capture.warnings().size(); }

    bool empty() const { return list().isEmpty(); }

    bool operator==(const QStringList& expected) const { return list() == expected; }

private:
    GPSProtocolLogCapture _capture;
    qsizetype _cleared = 0;
};

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

class ProtocolReceiver final : public UBXReceiverModel
{
public:
    explicit ProtocolReceiver(GPSTestClock& testClock)
        : UBXReceiverModel(UBXReceiverModel::Receiver::F9P, testClock)
        , _transport(_stop, *this)
    {
        _configure();
        _transport.setModel(this);
    }

    GPSRuntimeIO io()
    {
        _configure();
        _transport.clearReplies();
        _transport.clearCommands();
        _transport.setWriteHandler([this](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) {
            return interceptLowLevelWrite(bytes);
        });
        auto link = _transport.makeIO({});
        link.nowUs = [this] { return clock().nowUs(); };
        link.wait = [this](std::chrono::microseconds delay) {
            ++transportOperations;
            clock().advanceBy(delay.count());
            return true;
        };
        link.read = [this, read = std::move(link.read)](std::span<uint8_t> bytes, GPSDeadline deadline) {
            const auto readResult = read(bytes, deadline);
            if (readResult.status == GPSReadStatus::TimedOut) {
                clock().advanceTo(deadline.untilUs + 1);
            } else {
                clock().advanceBy(1000);
            }
            return readResult;
        };
        return link;
    }

    /// Records integrity and survey reports, and each delivered position in @a position.
    GPSRuntimeObserver observer(GPSDecodedPosition* position = nullptr)
    {
        GPSRuntimeObserver result;
        result.decoded = [this, position](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSIntegrityReport>(&event)) {
                    integrity = *report;
                    ++integrityCount;
                } else if (std::holds_alternative<GPSDecodedSurvey>(event)) {
                    ++statusCallbacks;
                } else if (const auto* fix = std::get_if<GPSDecodedPosition>(&event); fix && position) {
                    *position = *fix;
                }
            }
        };
        return result;
    }

    void resetState()
    {
        UBXReceiverModel::operator=(UBXReceiverModel(UBXReceiverModel::Receiver::F9P, clock()));
        _configure();
        _transport.setModel(this);
    }

    Warnings warnings;

private:
    void _configure() { lowLevelProtocolBehavior = true; }

    std::stop_source _stop;
    ScriptedReceiver _transport;
};

struct Fixture
{
    ProtocolReceiver receiver;
    GPSDecodedPosition position{};
    GPSProtocolRuntime driver;
    GPSBaseStationConfig base;

    explicit Fixture(GPSTestClock& clock, bool satelliteInfo = false)
        : receiver(clock)
        , driver(UBX::FAMILY, receiver.io(), receiver.observer(&position),
                 GPSFamilyOptions{.satelliteInfoEnabled = satelliteInfo})
    {
        clock.reset();
        receiver.warnings.clear();
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).accuracyMeters = 1.25;
        std::get<GPSBaseStationConfig::SurveyIn>(base.mode).duration = 60s;
    }

    bool configure()
    {
        unsigned baudrate = 115200;
        GPSConfig config{};
        config.base = base;
        return driver.configure(config, baudrate);
    }

    void success(unsigned expectedSurveyPolls)
    {
        CHECK(configure());
        CHECK(driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0, 1}));
        CHECK(receiver.surveyPolls == expectedSurveyPolls);
        CHECK(receiver.starts == 1);
        CHECK(receiver.startSettings.at(Cfg::TMODE_SVIN_MIN_DUR.id) == 60);
        CHECK(receiver.startSettings.at(Cfg::TMODE_SVIN_ACC_LIMIT.id) == 12500);
        CHECK(receiver.startSettings.at(Cfg::MSGOUT_UBX_NAV_SVIN.port(UBX::MsgOutPort::UART1).id) == 5);
        CHECK(receiver.startSettings.at(Cfg::MSGOUT_UBX_NAV_SVIN.port(UBX::MsgOutPort::USB).id) == 5);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
    }

    void timeout()
    {
        CHECK(!configure());
        CHECK(!driver.receiverReady());
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.surveyPolls > 1 && receiver.surveyPolls <= 31);
        CHECK(receiver.clock().nowUs() - receiver.disabledAt >= 3000000);
        CHECK(receiver.clock().nowUs() - receiver.disabledAt < 3300000);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
    }

    void readFailure(GPSProtocolError error)
    {
        receiver.pollReadError = error;
        CHECK(!configure());
        CHECK(!driver.receiverReady());
        CHECK(receiver.failedReads == 1);
        CHECK(receiver.surveyPolls == 1);
        CHECK(receiver.modes == std::vector<uint32_t>({0}));
        CHECK(receiver.starts == 0);
        CHECK(receiver.statusCallbacks == 0);
        CHECK(receiver.rtcmEnables == 0);
        CHECK(receiver.clock().nowUs() - receiver.disabledAt < 100000);

        const auto warnings = receiver.warnings.list();
        if (error == GPSProtocolError::Cancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(receiver.pollReadDetail)});
        }
        CHECK(driver.error() == error);
        CHECK(driver.errorDetail() == receiver.pollReadDetail);
        CHECK(driver.receive(10ms) == GPSReceiveUpdates{});
        CHECK(receiver.failedReads == 1);
        CHECK(receiver.warnings.list() == warnings);
    }
};

static void receiveFailureLogging(GPSTestClock& clock)
{
    for (const GPSProtocolError error : {GPSProtocolError::Cancelled, GPSProtocolError::Transport}) {
        Fixture f(clock);
        CHECK(f.configure());
        f.receiver.pollReadError = error;
        f.receiver.warnings.clear();
        CHECK(f.driver.receive(10ms) == GPSReceiveUpdates{});
        CHECK(f.driver.error() == error);
        CHECK(f.driver.errorDetail() == f.receiver.pollReadDetail);
        const auto warnings = f.receiver.warnings.list();
        if (error == GPSProtocolError::Cancelled) {
            CHECK(warnings.empty());
        } else {
            CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                              .arg(static_cast<int>(GPSReadStatus::Error))
                                              .arg(f.receiver.pollReadDetail)});
        }
        CHECK(f.driver.receive(10ms) == GPSReceiveUpdates{});
        CHECK(f.driver.error() == error);
        CHECK(f.receiver.failedReads == 1);
        CHECK(f.receiver.warnings.list() == warnings);
    }
}

static Bytes commsPayload()
{
    Bytes payload(88, 0);
    payload[1] = 2;
    payload[2] = 2;
    // USB: 11800 bytes pending, current usage 100%, historical peak 101%.
    payload[9] = 3;
    payload[10] = 0x18;
    payload[11] = 0x2e;
    payload[16] = 100;
    payload[17] = 101;
    payload[18] = 12;
    payload[24] = 3;
    payload[26] = 4;
    payload[44] = 0x40;
    payload[45] = 0xe2;
    payload[46] = 1;
    // UART2: no current congestion, despite a historical peak of 108%.
    payload[48] = 1;
    payload[49] = 2;
    payload[57] = 108;
    return payload;
}

static void integrityReceipts(GPSTestClock& clock)
{
    Fixture f(clock);
    CHECK(f.configure());
    // This test inspects individual decoder mutations; epoch assembly has separate coverage.
    setMode(f.driver, {.navigation = true});
    Bytes mon_rf(UBX::WIRE_SIZE<UBX::MonRf>, 0);
    mon_rf[1] = 1;
    mon_rf[5] = 3;
    f.receiver.queueBytes(ubxFrame(Msg::MON_RF.value(), mon_rf));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrityCount == 1);
    CHECK(f.position.navigation.timestampUs == 0);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    const auto rf_stamp = f.receiver.integrity.jamming.timestampUs;
    CHECK(rf_stamp != 0);

    Bytes nav_status(UBX::WIRE_SIZE<UBX::NavStatus>, 0);
    nav_status[7] = 1 << SPOOF_DETECTION_SHIFT;
    f.receiver.queueBytes(ubxFrame(Msg::NAV_STATUS.value(), nav_status));
    f.driver.receive(100ms);
    const auto spoof_stamp = f.receiver.integrity.spoofing.timestampUs;
    CHECK(spoof_stamp != 0);
    CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    for (int i = 0; i < 10; ++i) {
        clock.advanceBy(1000000);
        f.receiver.queueBytes(ubxFrame(Msg::NAV_PVT.value(), pvt));
        CHECK(positionUpdated(f.driver.receive(100ms)));
        CHECK(f.position.navigation.timestampUs > rf_stamp);
        CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);
        CHECK(f.receiver.integrity.spoofing.timestampUs == spoof_stamp);
    }
    Bytes corrupt = ubxFrame(Msg::MON_RF.value(), mon_rf);
    corrupt.back() ^= 0xff;
    f.receiver.queueBytes(corrupt);
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.jamming.timestampUs == rf_stamp);
    f.receiver.queueBytes(ubxFrame(Msg::MON_RF.value(), mon_rf));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming.timestampUs > rf_stamp);

    Bytes sec_sig(4, 0);
    sec_sig[0] = 2;
    sec_sig[1] = 1 | (3 << 1);
    f.receiver.queueBytes(ubxFrame(Msg::SEC_SIG.value(), sec_sig));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    const auto sec_stamp = f.receiver.integrity.jamming.timestampUs;
    clock.advanceBy(6000000);
    f.receiver.queueBytes(ubxFrame(Msg::NAV_PVT.value(), pvt));
    CHECK(positionUpdated(f.driver.receive(100ms)));
    CHECK(f.receiver.integrity.jamming.timestampUs == sec_stamp);
    f.receiver.queueBytes(ubxFrame(Msg::SEC_SIG.value(), sec_sig));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.jamming.state == GPSIntegrityReport::JammingState::Critical);
    CHECK(f.receiver.integrity.jamming.timestampUs > sec_stamp);

    Bytes rtcm(UBX::WIRE_SIZE<UBX::RxmRtcm>, 0);
    rtcm[1] = 2 << RTCM_USED_SHIFT;
    f.receiver.queueBytes(ubxFrame(Msg::RXM_RTCM.value(), rtcm));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.corrections.use == GPSIntegrityReport::CorrectionUse::Used);
    const auto correction_stamp = f.receiver.integrity.corrections.timestampUs;
    CHECK(correction_stamp != 0);
    clock.advanceBy(6000000);
    f.receiver.queueBytes(ubxFrame(Msg::NAV_PVT.value(), pvt));
    CHECK(positionUpdated(f.driver.receive(100ms)));
    CHECK(f.receiver.integrity.corrections.timestampUs == correction_stamp);
    Bytes cor(UBX::WIRE_SIZE<UBX::RxmCor>, 0);
    cor[0] = 1;
    cor[4] = 29;
    cor[5] = 1;  // msgUsed=2 in statusInfo bits 8..7.
    f.receiver.queueBytes(ubxFrame(Msg::RXM_COR.value(), cor));
    f.driver.receive(100ms);
    CHECK(f.receiver.integrity.corrections.protocol == GPSIntegrityReport::CorrectionProtocol::PMP);
    CHECK(f.receiver.integrity.corrections.use == GPSIntegrityReport::CorrectionUse::Used);
    CHECK(f.receiver.integrity.corrections.timestampUs > correction_stamp);
}

static void commsDiagnostics(GPSTestClock& clock)
{
    Fixture f(clock);
    const Bytes reply = ubxFrame(Msg::MON_COMMS.value(), commsPayload());
    f.receiver.queueBytes(reply);
    f.driver.receive(100ms);
    CHECK(f.receiver.warnings.empty());
    CHECK(f.receiver.commsPolls == 0);
    f.receiver.queueBufferWarning();
    f.driver.receive(100ms);
    CHECK(f.receiver.commsPolls == 1);
    CHECK(f.receiver.warnings == QStringList{"ubx msg: txbuf alloc"});
    f.receiver.warnings.clear();
    f.receiver.queueBytes(reply);
    CHECK(!positionUpdated(f.driver.receive(100ms)));  // Diagnostic traffic alone is not a position update.
    const QStringList expected{"MON-COMMS after txbuf: txErrors=0x02 ports=2 (snapshot after warning)",
                               "MON-COMMS USB port=0x0300 txPending=11800 txUsage=100% txPeakUsage=101% "
                               "rxPending=12 rxUsage=3% overrunErrs=4 skipped=123456",
                               "MON-COMMS UART2 port=0x0201 txPending=0 txUsage=0% txPeakUsage=108% "
                               "rxPending=0 rxUsage=0% overrunErrs=0 skipped=0"};
    CHECK(f.receiver.warnings == expected);
    f.receiver.warnings.clear();
    f.receiver.queueBytes(reply);
    f.driver.receive(100ms);
    CHECK(f.receiver.warnings.empty());
}

static void invalidCommsDiagnostics(GPSTestClock& clock)
{
    Bytes payload = commsPayload();
    Bytes corrupt = ubxFrame(Msg::MON_COMMS.value(), payload);
    corrupt.back() ^= 0xff;
    std::vector<Bytes> invalid{corrupt, ubxFrame(Msg::MON_COMMS.value(), Bytes(7, 0)),
                               ubxFrame(Msg::MON_COMMS.value(), Bytes(87, 0)),
                               ubxFrame(Msg::MON_COMMS.value(), Bytes(368, 0))};
    payload[0] = 1;
    invalid.push_back(ubxFrame(Msg::MON_COMMS.value(), payload));
    payload[0] = 0;
    payload[1] = 3;
    invalid.push_back(ubxFrame(Msg::MON_COMMS.value(), payload));
    payload[1] = 255;
    invalid.push_back(ubxFrame(Msg::MON_COMMS.value(), payload));

    for (const auto& reply : invalid) {
        Fixture f(clock);
        f.receiver.queueBufferWarning();
        f.driver.receive(100ms);
        f.receiver.warnings.clear();
        f.receiver.queueBytes(reply);
        f.driver.receive(100ms);
        CHECK(f.receiver.warnings.empty());
        // Malformed input must not consume the pending reply or lose framing.
        f.receiver.queueBytes(ubxFrame(Msg::MON_COMMS.value(), Bytes(8, 0)));
        f.driver.receive(100ms);
        CHECK(f.receiver.warnings ==
              QStringList{"MON-COMMS after txbuf: txErrors=0x00 ports=0 (snapshot after warning)"});
    }
}

static void expiredCommsDiagnostics(GPSTestClock& clock)
{
    Fixture f(clock);
    f.receiver.queueBufferWarning();
    f.driver.receive(100ms);
    CHECK(f.receiver.commsPolls == 1);
    f.receiver.warnings.clear();
    clock.advanceBy(2000000);
    f.receiver.queueBytes(ubxFrame(Msg::MON_COMMS.value(), commsPayload()));
    f.driver.receive(100ms);
    CHECK(f.receiver.warnings.empty());
    // Expiration must allow a later warning to obtain a fresh snapshot.
    clock.advanceBy(5000000);
    f.receiver.queueBufferWarning();
    f.driver.receive(100ms);
    CHECK(f.receiver.commsPolls == 2);
    f.receiver.warnings.clear();
    f.receiver.queueBytes(ubxFrame(Msg::MON_COMMS.value(), Bytes(8, 0)));
    f.driver.receive(100ms);
    CHECK(f.receiver.warnings.list().size() == 1);
}

static void invalidConfiguration(GPSTestClock& clock)
{
    using Config = GPSConfig;
    const Config fixed{.base = {.mode = GPSBaseStationConfig::Fixed{
                                    .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                                    .accuracyMeters = 1}}};
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    std::vector<Config> invalid;
    const auto addFixed = [&](auto mutate) {
        auto config = fixed;
        mutate(config.base);
        invalid.push_back(config);
    };
    addFixed([](auto& base) { base = {.mode = GPSBaseStationConfig::Fixed{}}; });
    for (double value : {nan, double(infinity), 91.0, -91.0}) {
        addFixed(
            [&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.latitudeDegrees = value; });
    }
    for (double value : {nan, double(infinity), 181.0, -181.0}) {
        addFixed(
            [&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.longitudeDegrees = value; });
    }
    for (float value : {float(nan), infinity, 21474838.0f, -21474838.0f}) {
        addFixed([&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).position.altitudeMeters = value; });
    }
    for (float value : {float(nan), infinity, -1.0f, std::nextafter(429496.71875f, infinity)}) {
        addFixed([&](auto& base) { std::get<GPSBaseStationConfig::Fixed>(base.mode).accuracyMeters = value; });
    }
    for (double accuracy : {nan, double(infinity), 0.0, -1.0, 429496.7296}) {
        invalid.push_back(
            {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracy, .duration = 60s}}});
    }
    for (int64_t duration : {int64_t(0), int64_t(-1), int64_t(UINT32_MAX) + 1}) {
        invalid.push_back({.base = {.mode = GPSBaseStationConfig::SurveyIn{
                                        .accuracyMeters = 1, .duration = std::chrono::seconds(duration)}}});
    }
    for (const auto& config : invalid) {
        for (bool wasReady : {false, true}) {
            Fixture f(clock);
            if (wasReady) {
                CHECK(f.configure());
                CHECK(f.driver.receiverReady());
            }
            f.receiver.transportOperations = 0;
            f.receiver.warnings.clear();
            unsigned baudrate = 115200;
            CHECK(!f.driver.configure(config, baudrate));
            CHECK(f.receiver.transportOperations == 0);
            CHECK(!f.driver.receiverReady());
            CHECK(!failed(f.driver));
            CHECK(baudrate == 115200);
            CHECK(f.receiver.warnings.list().size() == 1);
        }
    }

    for (bool legacy : {false, true}) {
        Fixture f(clock);
        f.receiver.legacy = legacy;
        f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
        f.base = fixed.base;
        std::get<GPSBaseStationConfig::Fixed>(f.base.mode).accuracyMeters = 429496.71875f;
        CHECK(f.configure());
        CHECK(f.driver.receiverReady());
        CHECK((legacy ? f.receiver.legacyFixedAccuracy : f.receiver.currentSettings.at(Cfg::TMODE_FIXED_POS_ACC.id)) ==
              4294967040u);
    }

    // Compact bases send MSM4 and switch off MSM7 left over from an earlier session, and vice versa.
    constexpr std::array<std::pair<UBX::MsgOutKey, uint16_t>, 4> msm7{{
        {Cfg::MSGOUT_RTCM_3X_TYPE1077, Msg::RTCM3_1077.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1087, Msg::RTCM3_1087.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1097, Msg::RTCM3_1097.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1127, Msg::RTCM3_1127.value()},
    }};
    constexpr std::array<std::pair<UBX::MsgOutKey, uint16_t>, 4> msm4{{
        {Cfg::MSGOUT_RTCM_3X_TYPE1074, Msg::RTCM3_1074.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1084, Msg::RTCM3_1084.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1094, Msg::RTCM3_1094.value()},
        {Cfg::MSGOUT_RTCM_3X_TYPE1124, Msg::RTCM3_1124.value()},
    }};
    for (bool legacy : {false, true}) {
        for (bool compact : {false, true}) {
            Fixture f(clock, true);
            f.receiver.legacy = legacy;
            f.receiver.module = legacy ? "NEO-M8P" : "ZED-F9P";
            f.base = fixed.base;
            f.base.compactObservations = compact;
            CHECK(f.configure());
            CHECK(f.driver.receiverReady());
            const auto rate = [&](const std::pair<UBX::MsgOutKey, uint16_t>& message) -> unsigned {
                return legacy ? f.receiver.messageRates.at(message.second)
                              : f.receiver.currentSettings.at(message.first.port(UBX::MsgOutPort::UART1).id);
            };
            for (const auto& message : msm7) {
                CHECK(rate(message) == (compact ? 0u : 1u));
            }
            for (const auto& message : msm4) {
                CHECK(rate(message) == (compact ? 1u : 0u));
            }
            // The 1 Hz base rate keeps satellite reports within their freshness window.
            CHECK(rate({Cfg::MSGOUT_UBX_NAV_SAT, Msg::NAV_SVINFO.value()}) == 2u);
        }
    }
}

static void explicitNoFix(GPSTestClock& clock)
{
    ProtocolReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
    const auto& position = decoderOf(driver).position();
    CHECK(position.navigation.fixType == GPSPositionReport::FixType::Unknown);
    setMode(driver, {.navigation = true});
    Bytes payload(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    payload[20] = 3;
    for (const uint8_t flags : std::array<uint8_t, 4>{0, 2, 0x40, 0x80}) {
        payload[21] = FIX_OK;
        const auto valid = driver.decode(ubxFrame(Msg::NAV_PVT.value(), payload));
        CHECK(valid.batch.events.size() == 1);
        CHECK(std::get<GPSDecodedPosition>(valid.batch.events.front()).navigation.fixType ==
              GPSPositionReport::FixType::Fix3D);
        CHECK(position.navigation.fixType == GPSPositionReport::FixType::Fix3D);
        payload[21] = flags;
        const auto decoded = driver.decode(ubxFrame(Msg::NAV_PVT.value(), payload));
        CHECK(decoded.batch.events.size() == 1);
        CHECK(std::get<GPSDecodedPosition>(decoded.batch.events.front()).navigation.fixType ==
              GPSPositionReport::FixType::NoFix);
        CHECK(!std::get<GPSDecodedPosition>(decoded.batch.events.front()).velocityValid);
        CHECK(!position.velocityValid);
    }
}

static void outOfRangeCoordinates(GPSTestClock& clock)
{
    ProtocolReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
    const auto& position = decoderOf(driver).position();
    setMode(driver, {.navigation = true});
    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = FIX_OK;
    const auto decodePvt = [&](int32_t longitude, int32_t latitude) {
        (void) LittleEndian::write<int32_t>(pvt, 24, longitude);
        (void) LittleEndian::write<int32_t>(pvt, 28, latitude);
        (void) driver.decode(ubxFrame(Msg::NAV_PVT.value(), pvt));
    };
    decodePvt(-1800000000, 900000000);
    CHECK(position.navigation.latitudeDegrees == 90 && position.navigation.longitudeDegrees == -180);
    decodePvt(80000000, 900000001);
    CHECK(std::isnan(position.navigation.latitudeDegrees));
    CHECK(position.navigation.longitudeDegrees == 8);
    decodePvt(-1800000001, 470000000);
    CHECK(position.navigation.latitudeDegrees == 47);
    CHECK(std::isnan(position.navigation.longitudeDegrees));

    setMode(driver, {.navigation = true, .useNavPvt = false});
    Bytes posllh(UBX::WIRE_SIZE<UBX::NavPosllh>, 0);
    (void) LittleEndian::write<int32_t>(posllh, 4, (std::numeric_limits<int32_t>::max)());
    (void) LittleEndian::write<int32_t>(posllh, 8, (std::numeric_limits<int32_t>::min)());
    (void) driver.decode(ubxFrame(Msg::NAV_POSLLH.value(), posllh));
    CHECK(std::isnan(position.navigation.latitudeDegrees));
    CHECK(std::isnan(position.navigation.longitudeDegrees));
}

static void baudDiscovery(GPSTestClock& clock)
{
    for (const unsigned initialBaud : {9600U, 115200U}) {
        for (const bool fixed : {false, true}) {
            for (const bool usb : {false, true}) {
                for (const bool loseAck : {false, true}) {
                    clock.reset(1000000);
                    ProtocolReceiver receiver(clock);
                    receiver.receiverBaud = initialBaud;
                    receiver.usb = usb;
                    receiver.loseBaudAck = loseAck;
                    receiver.protocol = "27.31";
                    GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
                    GPSConfig config{};
                    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
                    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
                    unsigned baud = fixed ? initialBaud : 0;
                    CHECK(driver.configure(config, baud));
                    CHECK(driver.receiverReady());
                    CHECK(receiver.unidentifiedWrites == 0);
                    CHECK(baud == (fixed ? initialBaud : 115200));
                    CHECK(receiver.receiverBaud == baud);
                    CHECK(receiver.hostBaud == baud);
                    CHECK(receiver.currentSettings.at(Cfg::NAVSPG_DYNMODEL.id) == 2);
                    CHECK(receiver.currentSettings.at(Cfg::RATE_MEAS.id) == 200);
                    if (fixed) {
                        CHECK(receiver.hostBauds == std::vector<unsigned>{initialBaud});
                    } else if (!usb) {
                        const std::vector<unsigned> probes = initialBaud == 9600
                                                                 ? std::vector<unsigned>{38400, 57600, 9600}
                                                                 : std::vector<unsigned>{38400, 57600, 9600, 115200};
                        CHECK(receiver.identityBauds.size() >= probes.size());
                        CHECK(std::equal(probes.begin(), probes.end(), receiver.identityBauds.begin()));
                    }
                    CHECK(clock.nowUs() < 15000000);
                }
            }
        }
    }
}

static void discoveryFailures(GPSTestClock& clock)
{
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        clock.reset(1000000);
        ProtocolReceiver receiver(clock);
        receiver.receiverBaud = 9600;
        receiver.hardware = scenario == 0 ? "UNKN0WN!" : "00190000";
        receiver.protocol = scenario == 1 ? "invalid" : "27.31";
        receiver.corruptIdentity = scenario == 2;
        receiver.silencePortConfiguration = scenario == 3;
        receiver.loseBaudAck = scenario >= 4;
        receiver.ignoreBaudChange = scenario == 4 || scenario == 5;
        receiver.usb = scenario == 5;
        receiver.rejectAfterBaudChange = scenario == 6;
        receiver.nakAfterBaudChange = scenario == 8;
        receiver.readbackReply =
            scenario == 9 ? UBXReceiverModel::ReadbackReply::Timeout : UBXReceiverModel::ReadbackReply::Value;
        receiver.module = scenario == 7 ? "NEO-M9N" : "ZED-F9P";
        GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
        GPSConfig config{};
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
        unsigned baud = 0;
        CHECK(!driver.configure(config, baud));
        CHECK(!driver.receiverReady());
        CHECK(receiver.unidentifiedWrites == 0);
        if (scenario < 3 || scenario == 7) {
            CHECK(receiver.configurationWrites == 0);
        } else if (scenario == 3) {
            CHECK(receiver.configurationWrites == 1);
        } else if (scenario == 6) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.currentSettings.count(Cfg::USBOUTPROT_UBX.id) == 0);
        } else if (scenario == 8) {
            CHECK(receiver.lateBaudAckDelivered);
            CHECK(receiver.currentSettings.at(Cfg::USBOUTPROT_UBX.id) == 1);
        }
        CHECK(clock.nowUs() < 20000000);
    }
}

static void navigationFixFlags(GPSTestClock& clock)
{
    using Fix = GPSPositionReport::FixType;
    constexpr std::array<Fix, 8> FIX_3D{Fix::Fix3D,    Fix::Differential, Fix::RTKFloat, Fix::RTKFloat,
                                        Fix::RTKFixed, Fix::RTKFixed,     Fix::Fix3D,    Fix::Differential};
    constexpr std::array<Fix, 6> UNCORRECTED{Fix::NoFix, Fix::Extrapolated, Fix::Fix2D,
                                             Fix::Fix3D, Fix::Fix3D,        Fix::NoFix};
    for (const bool legacy : {false, true}) {
        for (const unsigned rawFix : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 255U}) {
            for (unsigned flags = 0; flags <= UINT8_MAX; ++flags) {
                Fix expected = rawFix < UNCORRECTED.size() ? UNCORRECTED[rawFix] : Fix::Unknown;
                if (!(flags & 1)) {
                    expected = Fix::NoFix;
                } else if (rawFix == 3 || rawFix == 4) {
                    expected = legacy ? FIX_3D[(flags >> 1) & 1] : FIX_3D[((flags >> 6) * 2) + ((flags >> 1) & 1)];
                }
                std::array<unsigned, 3> order{0, 1, 2};
                do {
                    clock.reset(1000000);
                    const auto runtime = offlineUBX(clock);
                    auto& driver = *runtime;
                    setMode(driver, {.navigation = true, .useNavPvt = !legacy, .assembleEpochs = true});
                    Bytes pvt(92);
                    (void) LittleEndian::write<uint32_t>(pvt, 0, 1000);
                    pvt[20] = static_cast<uint8_t>(rawFix);
                    pvt[21] = static_cast<uint8_t>(flags);
                    (void) LittleEndian::write<int32_t>(pvt, 24, 80000000);
                    (void) LittleEndian::write<int32_t>(pvt, 28, 470000000);
                    (void) LittleEndian::write<int32_t>(pvt, 60, 12000);
                    if (!legacy) {
                        CHECK(driver.decode(ubxFrame(Msg::NAV_PVT.value(), pvt)).batch.events.empty());
                    } else {
                        std::array<Bytes, 3> payloads{Bytes(28), Bytes(52), Bytes(36)};
                        constexpr std::array<uint16_t, 3> MESSAGES{Msg::NAV_POSLLH.value(), Msg::NAV_SOL.value(),
                                                                   Msg::NAV_VELNED.value()};
                        for (auto& payload : payloads) {
                            (void) LittleEndian::write<uint32_t>(payload, 0, 1000);
                        }
                        (void) LittleEndian::write<int32_t>(payloads[0], 4, 80000000);
                        (void) LittleEndian::write<int32_t>(payloads[0], 8, 470000000);
                        payloads[1][10] = static_cast<uint8_t>(rawFix);
                        payloads[1][11] = static_cast<uint8_t>(flags);
                        (void) LittleEndian::write<uint32_t>(payloads[2], 20, 1200);
                        for (const unsigned index : order) {
                            CHECK(driver.decode(ubxFrame(MESSAGES[index], payloads[index])).batch.events.empty());
                        }
                    }
                    Bytes end(4);
                    (void) LittleEndian::write<uint32_t>(end, 0, 1000);
                    const auto decoded = driver.decode(ubxFrame(Msg::NAV_EOE.value(), end));
                    CHECK(decoded.batch.events.size() == 1);
                    const auto& fix = std::get<GPSDecodedPosition>(decoded.batch.events.front());
                    CHECK(fix.navigation.fixType == expected);
                    CHECK(fix.velocityValid == (expected != Fix::NoFix && expected != Fix::Unknown));
                    CHECK(fix.navigation.latitudeDegrees == 47 && fix.navigation.longitudeDegrees == 8);
                    CHECK(std::abs(fix.navigation.speedMetersPerSecond - 12) < 1e-5f);
                } while (legacy && std::next_permutation(order.begin(), order.end()));
            }
        }
    }
}

static void transactionalFrames(GPSTestClock& clock)
{
    ProtocolReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, receiver.io());
    const auto& satellites = decoderOf(driver).satellites();
    const auto& identity = decoderOf(driver).state().identity;
    unsigned baud = 115200;
    GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    CHECK(driver.configure(config, baud));
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
    CHECK(decoded.bytesConsumed == joined.size());
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::get<GPSDecodedSatellites>(decoded.batch.events[0]).constellations[0].inView == 1);
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(std::span(valid).first(valid.size() - 1)).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(std::span(valid).last(1)).batch.events.size() == 1);
    payload[5] = 2;  // A valid checksum cannot make an incomplete counted payload valid.
    CHECK(driver.decode(ubxFrame(Msg::NAV_SAT.value(), payload)).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(driver.decode(ubxFrame(Msg::NAV_SAT.value(), Bytes(7, 0))).batch.events.empty());
    CHECK(satellites.constellations[0].inView == 1);
    CHECK(std::get<GPSDecodedSatellites>(decoded.batch.events[0]).constellations[0].inView == 1);
    const auto empty = driver.decode(ubxFrame(Msg::NAV_SAT.value(), Bytes{0, 0, 0, 0, 1, 0, 0, 0}));
    CHECK(empty.batch.events.size() == 1);
    CHECK(std::get<GPSDecodedSatellites>(empty.batch.events.front()).constellations[0].inView == 0);
    CHECK(satellites.constellations[0].inView == 0);
    GPSProtocolRuntime withoutSatellites(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
    setMode(withoutSatellites, {.navigation = true});
    CHECK(withoutSatellites.decode(valid).batch.events.empty());
    const QByteArray originalModel = identity.model;
    Bytes version(70, 0);
    const std::string module = "MOD=NEO-M9N";
    std::copy(module.begin(), module.end(), version.begin() + 40);
    auto badVersion = ubxFrame(Msg::MON_VER.value(), version);
    badVersion.back() ^= 1;
    CHECK(driver.decode(badVersion).batch.events.empty());
    CHECK(identity.model == originalModel);

    Bytes baseVersion(40, 0);
    const std::string firmware = "SPG 4.04";
    std::copy(firmware.begin(), firmware.end(), baseVersion.begin());
    CHECK(driver.decode(ubxFrame(Msg::MON_VER.value(), baseVersion)).batch.events.empty());
    CHECK(identity.firmware == QByteArray::fromStdString(firmware));
    const std::string model = identity.model.toStdString();
    CHECK(driver.identity() == QString::fromStdString(model.empty() ? firmware : model + ' ' + firmware));

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = 1;
    setMode(driver, {.navigation = true});
    Bytes epochs;
    for (uint8_t index = 1; index <= 20; ++index) {
        pvt[23] = index;
        const auto frame = ubxFrame(Msg::NAV_PVT.value(), pvt);
        epochs.insert(epochs.end(), frame.begin(), frame.end());
    }
    size_t offset = 0;
    unsigned count = 0;
    while (offset < epochs.size()) {
        auto batch = driver.decode(std::span(epochs).subspan(offset));
        CHECK(batch.bytesConsumed > 0);
        CHECK(batch.batch.events.size() <= GPSEventSink::MAX_EVENTS);
        offset += batch.bytesConsumed;
        for (const auto& event : batch.batch.events) {
            CHECK(std::get<GPSDecodedPosition>(event).navigation.satellitesUsed == ++count);
        }
    }
    CHECK(count == 20);
    setMode(driver, {.navigation = true, .useNavPvt = true, .corrections = true});
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = QGC::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    std::copy(correction.begin(), correction.end(), pvt.begin() + 40);
    const auto embedded = driver.decode(ubxFrame(Msg::NAV_PVT.value(), pvt));
    CHECK(embedded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSDecodedPosition>(embedded.batch.events.front()));
    const auto standalone = driver.decode(correction);
    CHECK(standalone.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMFrame>(standalone.batch.events.front()));
    std::vector<std::vector<uint8_t>> recovered;
    GPSRuntimeObserver observer;
    observer.decoded = [&](const GPSEventBatch& batch) {
        CHECK(batch.events.size() <= GPSEventSink::MAX_EVENTS);
        for (const auto& event : batch.events) {
            const auto& report = std::get<GPSRTCMFrame>(event);
            recovered.emplace_back(report.bytes.begin(), report.bytes.end());
        }
    };
    const auto recoveryDriver = offlineUBX(clock, std::move(observer));
    setMode(*recoveryDriver, {.corrections = true});
    verifyRTCMRecovery(*recoveryDriver, recovered);
}

static void controlDeadline(GPSTestClock& clock)
{
    ProtocolReceiver receiver(clock);
    GPSDecodedPosition position{};
    clock.reset(1000000);
    auto io = receiver.io();
    const auto read = io.read;
    const auto write = io.write;
    bool expireRead = false;
    bool verifyWrite = false;
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
        if (verifyWrite) {
            CHECK(deadline.remaining(clock.nowUs()) > 0ms);
        }
        return write(bytes, deadline);
    };
    GPSProtocolRuntime driver(UBX::FAMILY, std::move(io), receiver.observer(&position),
                              {.satelliteInfoEnabled = false});
    unsigned baud = 115200;
    GPSConfig config{};
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    CHECK(driver.configure(config, baud));
    driver.receive(10ms);  // Drain the configuration responses before the timed warning.
    receiver.readChunk = GPSCommandChannel::READ_CHUNK_SIZE;
    receiver.queueBufferWarning();
    expireRead = true;
    driver.receive(10ms);
    CHECK(verifyWrite);
    CHECK(receiver.commsPolls == 1);
    CHECK(!failed(driver));
}

static void identificationWriteBudget(GPSTestClock& clock)
{
    for (bool expireWrite : {false, true}) {
        clock.reset(1000000);
        std::vector<uint64_t> writeDeadlines;
        std::vector<GPSCommandResult> completions;
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
            if (clock.nowUs() >= deadline.untilUs) {
                return GPSWriteResult{GPSWriteStatus::TimedOut};
            }
            CHECK(deadline.remaining(clock.nowUs()) <= 250ms);
            writeDeadlines.push_back(deadline.untilUs);
            clock.advanceTo(expireWrite ? deadline.untilUs : clock.nowUs() + 40000);
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        GPSRuntimeObserver observer;
        observer.commandFinished = [&](const GPSCommandResult& result) { completions.push_back(result); };
        GPSProtocolRuntime driver(UBX::FAMILY, std::move(io), std::move(observer), {.satelliteInfoEnabled = false});
        unsigned baud = 115200;
        GPSConfig config;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
        CHECK(!driver.configure(config, baud));
        CHECK(completions.size() == 1);
        const auto& evidence = completions.front().evidence;
        CHECK(evidence.command == std::to_string(Msg::MON_VER.value()));
        CHECK(evidence.startedAtUs == 1020000);
        CHECK(writeDeadlines == std::vector<uint64_t>(expireWrite ? 1 : 2, evidence.startedAtUs + 250000));
        CHECK(evidence.acceptedBytes == (expireWrite ? 6 : 8));
        CHECK(evidence.writtenBytes == evidence.acceptedBytes && evidence.uncertainBytes == 0);
        CHECK(evidence.outcome == (expireWrite ? GPSCommandOutcome::TransportError : GPSCommandOutcome::TimedOut));
        CHECK(replyDeadline == (expireWrite ? 0 : evidence.startedAtUs + 2000000));
        CHECK(evidence.finishedAtUs == evidence.startedAtUs + (expireWrite ? 250000 : 2000000));
    }
}

static void reentrantPayload(GPSTestClock& clock)
{
    ProtocolReceiver receiver(clock);
    GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
    bool reentered = false;
    Bytes correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto crc = QGC::crc24q(correction);
    correction.insert(correction.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    // A message handler that decodes more input while the decoder logs reuses the storage of the frame being
    // decoded; the nested frame does not log, because Qt does not pass nested messages to handlers.
    const WarningHook hook([&](const QString& message) {
        if (!reentered && message == u"ubx msg: txbuf alloc") {
            reentered = true;
            setMode(driver, {.navigation = true, .corrections = true});
            (void) driver.consume(ubxFrame(Msg::INF_NOTICE.value(), Bytes{'o', 'k'}));
            (void) driver.consume(std::span(correction).first(4));
        }
    });
    setMode(driver, {.navigation = true, .corrections = true});
    const std::string warning = "txbuf alloc";
    CHECK(
        driver.decode(ubxFrame(Msg::INF_WARNING.value(), Bytes(warning.begin(), warning.end()))).batch.events.empty());
    CHECK(reentered);
    CHECK(receiver.warnings == QStringList{"ubx msg: txbuf alloc"});
    const auto decoded = driver.decode(std::span(correction).subspan(4));
    CHECK(decoded.batch.events.size() == 1);
    CHECK(std::holds_alternative<GPSRTCMFrame>(decoded.batch.events.front()));
    CHECK(receiver.transportOperations == 0);
    driver.receive(1ms);
    CHECK(receiver.commsPolls == 1);
}

static void isolatedFrameAndControl(GPSTestClock& clock)
{
    UBX::FrameDecoder decoder;
    const Bytes payload = {0x06, 0x24};
    const auto bytes = ubxFrame(Msg::ACK_ACK.value(), payload);
    std::optional<UBX::Frame> frame;
    for (size_t index = 0; index < bytes.size(); ++index) {
        frame = decoder.consume(bytes[index]);
        CHECK(frame.has_value() == (index + 1 == bytes.size()));
    }
    CHECK(frame->message == Msg::ACK_ACK.value() && frame->length == 2);
    CHECK(frame->payload[0] == 6 && frame->payload[1] == 0x24);
    const auto retained = *frame;
    auto corrupt = bytes;
    corrupt.back() ^= 1;
    for (auto byte : corrupt) {
        CHECK(!decoder.consume(byte));
    }
    CHECK(decoder.idle());
    CHECK(retained.payload[0] == 6);
    const auto longFrame = ubxFrame(Msg::INF_NOTICE.value(), Bytes(4096, 0xa5));
    for (auto byte : longFrame) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->length == 4096 && frame->payload.back() == 0xa5);
    decoder.reset();
    for (auto byte : bytes) {
        decoder.consume(byte);
    }
    CHECK(frame->length == 4096 && frame->payload.front() == 0xa5);
    CHECK(retained.payload[0] == 6 && retained.payload[1] == 0x24);
    for (auto byte : std::span(bytes).first(5)) {
        CHECK(!decoder.consume(byte));
    }
    decoder.reset();
    for (auto byte : bytes) {
        frame = decoder.consume(byte);
    }
    CHECK(frame && frame->message == Msg::ACK_ACK.value() && frame->length == 2);

    Bytes pvt(UBX::WIRE_SIZE<UBX::NavPvt>, 0);
    pvt[20] = 3;
    pvt[21] = FIX_OK;
    const auto validUbx = ubxFrame(Msg::NAV_PVT.value(), pvt);
    const auto validRtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    Bytes stream{0xb5, 0x62, 0x01, 0x07, 0x01, 0x10};
    stream.insert(stream.end(), validUbx.begin(), validUbx.end());
    stream.insert(stream.end(), validRtcm.begin(), validRtcm.end());
    const auto receiver = offlineUBX(clock);
    setMode(*receiver, {.navigation = true, .corrections = true});
    const auto recovered = receiver->decode(stream);
    CHECK(recovered.bytesConsumed == stream.size());
    CHECK(recovered.batch.events.size() == 2);
    CHECK(std::holds_alternative<GPSDecodedPosition>(recovered.batch.events[0]));
    CHECK(std::holds_alternative<GPSRTCMFrame>(recovered.batch.events[1]));
    CHECK(!failed(*receiver));

    for (size_t prefix = 1; prefix <= 4; ++prefix) {
        decoder.reset();
        for (size_t i = 0; i < prefix; ++i) {
            CHECK(!decoder.consume(0xb5));
        }
        unsigned completed = 0;
        for (auto byte : bytes) {
            if (const auto overlapping = decoder.consume(byte)) {
                ++completed;
                CHECK(overlapping->message == Msg::ACK_ACK.value());
                CHECK(overlapping->length == payload.size());
                CHECK(std::equal(payload.begin(), payload.end(), overlapping->payload.begin()));
            }
        }
        CHECK(completed == 1);
        CHECK(decoder.idle());
    }

    UBX::ReceiverController controller;
    controller.beginAcknowledgement(Msg::CFG_NAV5.value());
    controller.accept(UBX::Acknowledgement{Msg::CFG_RATE.value(), true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Pending);
    controller.accept(UBX::Acknowledgement{Msg::CFG_NAV5.value(), false});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);
    controller.finishAcknowledgement();
    controller.accept(UBX::Acknowledgement{Msg::CFG_NAV5.value(), true});
    CHECK(controller.acknowledgement() == GPSCommandOutcome::Rejected);

    const std::array<uint32_t, 2> keys{Cfg::NAVSPG_DYNMODEL.id, Cfg::RATE_MEAS.id};
    controller.beginReadback(keys);
    UBX::ConfigurationValues values;
    values.count = 2;
    values.values[0].key = values.values[1].key = keys[0];
    controller.accept(values);
    CHECK(!controller.readbackReady());
    values.values[0] = {.key = keys[1], .value = 200};
    values.values[1] = {.key = keys[0], .value = 4};
    controller.accept(values);
    CHECK(controller.readbackReady());
    CHECK(controller.readback().values[0].value == 4 && controller.readback().values[1].value == 200);
    CHECK(!UBX::decodeConfigurationValues(Bytes{1, 0, 0, 0, 1}));
}

static void checkedWireCodecs(GPSTestClock&)
{
    const auto fixed = []<typename T>() {
        using Codec = UBX::MessageCodec<T>;
        Bytes payload(UBX::WIRE_SIZE<T>, 0);
        CHECK(Codec::decode(payload));
        CHECK(!Codec::decode({}));
        payload.pop_back();
        CHECK(!Codec::decode(payload));
        payload.resize(UBX::WIRE_SIZE<T> + 1);
        CHECK(!Codec::decode(payload));
    };
    fixed.operator()<UBX::NavPosllh>();
    fixed.operator()<UBX::NavDop>();
    fixed.operator()<UBX::NavSol>();
    fixed.operator()<UBX::NavPvt>();
    fixed.operator()<UBX::NavTimeUtc>();
    fixed.operator()<UBX::NavStatus>();
    fixed.operator()<UBX::NavSvin>();
    fixed.operator()<UBX::NavVelned>();
    fixed.operator()<UBX::NavRelposned>();
    fixed.operator()<UBX::NavDaheading>();
    fixed.operator()<UBX::NavHpposllh>();
    fixed.operator()<UBX::Ack>();
    fixed.operator()<UBX::RxmRtcm>();
    fixed.operator()<UBX::MonHw6>();
    fixed.operator()<UBX::MonHw7>();
    const auto hardware = []<typename T>(size_t jammingOffset) {
        Bytes payload(UBX::WIRE_SIZE<T>, 0xa5);
        CHECK(LittleEndian::write<uint16_t>(payload, 16, 0x1234));
        CHECK(LittleEndian::write<uint16_t>(payload, 18, 0x5678));
        payload[jammingOffset] = 77;
        const auto decoded = UBX::MessageCodec<T>::decode(payload);
        CHECK(decoded && decoded->noisePerMS == 0x1234 && decoded->agcCnt == 0x5678 && decoded->jamInd == 77);
    };
    hardware.operator()<UBX::MonHw6>(53);
    hardware.operator()<UBX::MonHw7>(45);
    CHECK(UBX::MessageCodec<UBX::NavPvt>::decode(Bytes(84)));

    Bytes rf(28, 0);
    rf[1] = 1;
    CHECK(UBX::MessageCodec<UBX::MonRf>::decode(rf));
    rf[1] = 2;
    CHECK(!UBX::MessageCodec<UBX::MonRf>::decode(rf));
    rf[1] = 1;
    rf[0] = 99;
    CHECK(!UBX::MessageCodec<UBX::MonRf>::decode(rf));
    Bytes comms(48, 0);
    comms[1] = 1;
    CHECK(UBX::MessageCodec<UBX::MonComms>::decode(comms));
    comms[1] = 2;
    CHECK(!UBX::MessageCodec<UBX::MonComms>::decode(comms));
    CHECK(UBX::MessageCodec<UBX::SecSig>::decode(Bytes{2, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<UBX::SecSig>::decode(Bytes{99, 7, 0, 0}));
    CHECK(!UBX::MessageCodec<UBX::SecSig>::decode(Bytes{2, 7, 0, 1}));
    CHECK(!UBX::MessageCodec<UBX::NavSatSatellite>::block(Bytes(12), 1));
    const auto ack = UBX::MessageCodec<UBX::Ack>::decode(Bytes{6, 0x24});
    CHECK(ack && ack->msg == Msg::CFG_NAV5.value());

    for (const auto [key, value] : std::array<UBX::ConfigurationValue, 5>{
             {{0x50000001, 0}, {0x10000001, 2}, {0x20000001, 256}, {0x30000001, 65536}, {0x00000001, 0}}}) {
        UBX::CheckedValsetBatch<32> batch;
        CHECK(batch.append(0x20000001, 1));
        const auto previousSize = batch.size;
        CHECK(!batch.append(key, value));
        CHECK(!batch.append(0x20000002, 2));
        CHECK(batch.size == previousSize && batch.payload().empty());
    }
    UBX::CheckedValsetBatch<28> batch;
    CHECK(batch.payload().empty());
    CHECK(batch.append(0x10000001, 1));
    CHECK(batch.append(0x20000002, 255));
    CHECK(batch.append(0x30000003, 65535));
    CHECK(batch.append(0x40000004, UINT32_MAX));
    CHECK(batch.size == batch.bytes.size());
    auto values = batch.bytes;
    CHECK(!UBX::decodeConfigurationValues(values));  // VALSET headers cannot masquerade as VALGET.
    values[0] = 1;
    values[1] = 0;
    const auto decoded = UBX::decodeConfigurationValues(values);
    CHECK(decoded && decoded->count == 4);
    CHECK(decoded->values[0].value == 1 && decoded->values[1].value == 255 && decoded->values[2].value == 65535 &&
          decoded->values[3].value == UINT32_MAX);
    for (size_t length = 1; length < values.size(); ++length) {
        if (length != 4 && length != 9 && length != 14 && length != 20) {
            CHECK(!UBX::decodeConfigurationValues(std::span(values).first(length)));
        }
    }
    values[8] = 2;
    CHECK(!UBX::decodeConfigurationValues(values));
    CHECK(!batch.append(0x20000005, 0));
    CHECK(batch.payload().empty());
    batch = {};
    CHECK(batch.append(0x20000005, 0));
    CHECK(!batch.payload().empty());
}

static void optionalCommandWriteEvidence(GPSTestClock& clock)
{
    for (const auto key : {Cfg::UART1INPROT_SPARTN.id, Cfg::ODO_USE_ODO.id}) {
        for (const auto failure : {GPSWriteStatus::Error, GPSWriteStatus::Cancelled}) {
            ProtocolReceiver receiver(clock);
            receiver.failValsetKey = key;
            receiver.valsetWriteFailure = failure;
            std::vector<GPSCommandResult> completions;
            GPSRuntimeObserver observer;
            observer.commandFinished = [&](const GPSCommandResult& result) { completions.push_back(result); };
            GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), std::move(observer), {.satelliteInfoEnabled = false});
            GPSConfig config;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
            unsigned baud = 115200;
            CHECK(!driver.configure(config, baud));
            CHECK(!completions.empty());
            const auto& result = completions.back();
            CHECK(result.evidence.command == std::to_string(Msg::CFG_VALSET.value()));
            CHECK(!result.evidence.required);
            CHECK(result.evidence.outcome == (failure == GPSWriteStatus::Cancelled
                                                  ? GPSCommandOutcome::Cancelled
                                                  : GPSCommandOutcome::TransportError));
            CHECK(result.evidence.acceptedBytes == 9 && result.evidence.writtenBytes == 7);
            CHECK(result.evidence.uncertainBytes == 2);
            // Configuration retires a pending command once; the failed attempt is not retired again.
            CHECK(driver.configurationEvidence().size() == completions.size());
        }
    }
}

/// A pre-protocol-27 base needs the station position and one constellation's observations; firmware may reject the
/// other RTCM messages.
static void legacyRTCMActivation(GPSTestClock& clock)
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
            f.receiver.legacy = true;
            f.receiver.module = "NEO-M8P";
            f.receiver.unsupportedMessages = scenario.unsupported;
            if (fixed) {
                f.base = {.mode = GPSBaseStationConfig::Fixed{
                              .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                              .accuracyMeters = 1}};
                CHECK(f.configure() == scenario.active);
                CHECK(f.driver.receiverReady() == scenario.active);
            } else {
                CHECK(f.configure());
                f.receiver.warnings.clear();
                f.receiver.queueSurveyReply(SurveyReply::Valid);
                (void) f.driver.receive(100ms);
                CHECK(failed(f.driver) == !scenario.active);
                CHECK(f.driver.error() == (scenario.active ? GPSProtocolError::None : GPSProtocolError::Protocol));
            }
            CHECK(f.receiver.warnings == (scenario.active ? QStringList{} : rejected));
            // Every RTCM output is requested, whichever the firmware rejects.
            const auto& evidence = f.driver.configurationEvidence();
            const auto requested = std::count_if(evidence.begin(), evidence.end(), [](const auto& command) {
                return command.command == std::to_string(Msg::CFG_MSG.value()) &&
                       command.outcome == GPSCommandOutcome::Rejected;
            });
            if (fixed) {
                CHECK(static_cast<size_t>(requested) == scenario.unsupported.size());
            }
            CHECK(f.receiver.messageRates.contains(Msg::RTCM3_1077.value()) ==
                  (std::ranges::find(scenario.unsupported, Msg::RTCM3_1077.value()) == scenario.unsupported.end()));
        }
    }
}

/// An F9P profile, as the goldens drive it, configured as a survey-in base.
struct ProfileBench
{
    explicit ProfileBench(GPSTestClock& clock)
        : model(UBXReceiverModel::Receiver::F9P, clock)
        , transport(std::stop_token(), &model)
        , runtime(UBX::FAMILY, transport.makeIO(makeGPSRuntimeTestIO(clock)), {}, {.satelliteInfoEnabled = false})
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
        qToLittleEndian(key, bytes.data());
        return std::ranges::any_of(transport.commands(),
                                   [&](const QByteArray& write) { return write.contains(bytes); });
    }

    /// The configuration commands from the VALSET that the first readback settles.
    std::vector<GPSConfigurationEvidence> fromSettledValset() const
    {
        const auto& evidence = runtime.configurationEvidence();
        const auto readback = std::ranges::find_if(
            evidence, [](const auto& command) { return command.command == "UBX-CFG-VALSET readback"; });
        if (readback == evidence.begin() || readback == evidence.end()) {
            return {};
        }
        return {std::prev(readback), evidence.end()};
    }

    const GPSProtocolLogCapture log;
    UBXReceiverModel model;
    ScriptedReceiver transport;
    GPSProtocolRuntime runtime;
};

/// CFG-VALSET acknowledgements do not name the batch, so an optional batch answered late is settled by readback before
/// anything else is written.
static void lateOptionalValsetReply(GPSTestClock& clock)
{
    const std::string valset = std::to_string(Msg::CFG_VALSET.value());
    const std::string readback = "UBX-CFG-VALSET readback";
    {
        // A late ACK and matching values accept CFG-SEC-JAMDET, so the older interference monitor is not configured.
        ProfileBench bench(clock);
        bench.model.delayOptionalAck = true;
        CHECK(bench.configure());
        CHECK(bench.runtime.receiverReady());
        CHECK(bench.model.optionalAckDelays == 1);
        const auto commands = bench.fromSettledValset();
        CHECK(commands.size() > 2);
        CHECK(commands[0].outcome == GPSCommandOutcome::TimedOut);
        CHECK(commands[1].command == readback && !commands[1].required);
        CHECK(commands[1].outcome == GPSCommandOutcome::ReadbackVerified);
        CHECK(commands[2].command == valset && commands[2].outcome == GPSCommandOutcome::Acknowledged);
        CHECK(!bench.wroteKey(Cfg::ITFM_ENABLE.id));
        CHECK(bench.log.warnings().empty());
    }
    {
        // A late NAK rejects the batch rather than the next one: firmware without CFG-SEC-JAMDET gets CFG-ITFM.
        ProfileBench bench(clock);
        bench.model.delayOptionalAck = true;
        bench.model.delayOptionalNak = true;
        CHECK(bench.configure());
        const auto commands = bench.fromSettledValset();
        CHECK(commands.size() > 2);
        CHECK(commands[0].outcome == GPSCommandOutcome::TimedOut);
        CHECK(commands[1].command == readback && commands[1].outcome == GPSCommandOutcome::Rejected);
        CHECK(commands[2].command == valset && commands[2].outcome == GPSCommandOutcome::Acknowledged);
        CHECK(bench.wroteKey(Cfg::ITFM_ENABLE.id));
        CHECK(bench.log.warnings().empty());
    }
    {
        // Without a late reply or a readback the batch stays ambiguous, and nothing more is written.
        ProfileBench bench(clock);
        bench.model.delayOptionalAck = true;
        bench.model.faultReadbackKey = Cfg::SEC_JAMDET_SENSITIVITY_HI.id;
        bench.model.readbackReply = UBXReceiverModel::ReadbackReply::Timeout;
        CHECK(!bench.configure());
        CHECK(!bench.runtime.receiverReady());
        const auto commands = bench.fromSettledValset();
        CHECK(commands.size() == 2);
        CHECK(commands[1].command == readback && commands[1].outcome == GPSCommandOutcome::TimedOut);
        CHECK(!bench.wroteKey(Cfg::ITFM_ENABLE.id));
        CHECK(bench.log.warnings() == QStringList{QStringLiteral("CFG-SEC-JAMDET_SENSITIVITY_HI not supported")});
    }
    {
        // A required batch fails on its timeout; no readback can accept it.
        ProfileBench bench(clock);
        bench.model.disableReply = UBXReceiverModel::DisableReply::Timeout;
        CHECK(!bench.configure());
        const auto& evidence = bench.runtime.configurationEvidence();
        CHECK(evidence.back().command == valset && evidence.back().required);
        CHECK(evidence.back().outcome == GPSCommandOutcome::TimedOut);
        CHECK(std::ranges::none_of(evidence, [&](const auto& command) { return command.command == readback; }));
    }
}

/// A NAK of an optional batch, or of its readback, proves its keys unknown and nothing more of it to arrive, also when
/// readback stands in for a baud-change ACK lost in the UART handoff. The batch is rejected rather than ambiguous: its
/// fallback follows and later batches are written.
static void rejectedOptionalBatch(GPSTestClock& clock)
{
    const std::string valset = std::to_string(Msg::CFG_VALSET.value());
    const std::string readback = "UBX-CFG-VALSET readback";

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
        clock.reset(1000000);
        ProtocolReceiver receiver(clock);
        receiver.receiverBaud = 9600;
        receiver.protocol = "27.31";
        receiver.loseBaudAck = scenario.baudAckLost;
        receiver.unsupportedKeys = {scenario.key};
        receiver.loseUnsupportedNak = scenario.batchNakLost;
        GPSProtocolRuntime driver(UBX::FAMILY, receiver.io(), {}, {.satelliteInfoEnabled = false});
        GPSConfig config{};
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
        unsigned baud = 0;
        CHECK(driver.configure(config, baud));
        CHECK(driver.receiverReady());
        CHECK(receiver.warnings.empty());
        CHECK(!receiver.currentSettings.contains(scenario.key));
        CHECK(receiver.currentSettings.contains(Cfg::ITFM_ENABLE.id) ==
              (scenario.key == Cfg::SEC_JAMDET_SENSITIVITY_HI.id));
        CHECK(receiver.currentSettings.at(Cfg::MSGOUT_UBX_NAV_PVT.port(UBX::MsgOutPort::UART1).id) == 1);
        const auto& evidence = driver.configurationEvidence();
        CHECK(std::ranges::count(evidence, GPSCommandOutcome::Rejected, &GPSConfigurationEvidence::outcome) == 1);
        const auto rejected =
            std::ranges::find(evidence, GPSCommandOutcome::Rejected, &GPSConfigurationEvidence::outcome);
        CHECK(rejected->command == readback);
        CHECK(std::prev(rejected)->command == valset);
        CHECK(std::prev(rejected)->outcome ==
              (scenario.baudAckLost ? GPSCommandOutcome::Written : GPSCommandOutcome::TimedOut));
    }
}

/// An M8 may acknowledge a CFG-MSG rate up to a second late, so an unanswered RTCM rate is polled, and the rates the
/// poll returns for that message confirm or reject it. After a poll that expires with a reply still to come, no more
/// rates are written, as a late reply would be taken for the next rate's: activation then needs the station position
/// and an observation message confirmed before it.
static void legacyRTCMRatePolls(GPSTestClock& clock)
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

    const std::string rate = std::to_string(Msg::CFG_MSG.value());
    const QStringList unanswered{
        QStringLiteral("Receiver did not answer a CFG-MSG rate or its poll; later RTCM messages are not configured")};
    for (const auto& scenario : cases) {
        Fixture f(clock);
        f.receiver.legacy = true;
        f.receiver.module = "NEO-M8P";
        f.receiver.rateAckMessage = scenario.message;
        f.receiver.rateAck = scenario.ack;
        f.receiver.silentRatePoll = scenario.silentPoll;
        f.receiver.ratePollAckDelay = scenario.pollAckDelay;
        f.receiver.unsupportedMessages = scenario.unsupported;
        f.base = {.mode = GPSBaseStationConfig::Fixed{
                      .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                      .accuracyMeters = 1}};
        CHECK(f.configure() == scenario.active);
        CHECK(f.receiver.ratePolls == 1);
        CHECK(f.receiver.warnings == (scenario.stops ? unanswered : QStringList{}));
        const auto& evidence = f.driver.configurationEvidence();
        const auto poll = std::ranges::find(evidence, "UBX-CFG-MSG " + std::to_string(scenario.message) + " readback",
                                            &GPSConfigurationEvidence::command);
        CHECK(poll != evidence.end() && poll->outcome == scenario.poll);
        CHECK(std::prev(poll)->command == rate && std::prev(poll)->outcome == GPSCommandOutcome::TimedOut);
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
        CHECK(later == expected);
        CHECK(f.receiver.messageRates.at(order.back()) == (scenario.stops ? 0 : 1));
    }
}

/// A slow M8 answers each command up to a second late, so RTCM activation after survey-in does not fit one streaming
/// service. Activation waits for the base rate's ACK, sends a rate only with time left for its poll, and resumes in the
/// next service: the session and the finished survey survive, and every RTCM message is configured.
static void slowLegacyRTCMActivation(GPSTestClock& clock)
{
    for (const auto delay : {700ms, 950ms}) {
        Fixture f(clock);
        f.receiver.legacy = true;
        f.receiver.module = "NEO-M8P";
        CHECK(f.configure());
        const auto modes = f.receiver.modes;
        const auto& requests = decoderOf(f.driver).state().requests;
        f.receiver.replyDelay = delay;
        f.receiver.queueSurveyReply(SurveyReply::Valid);
        const uint64_t started = clock.nowUs();
        (void) f.driver.receive(100ms);
        // The first service stops within its budget and leaves the rest to the next service.
        CHECK(clock.nowUs() - started <
              static_cast<uint64_t>(std::chrono::microseconds(100ms + GPSCommandChannel::SERVICE_TIMEOUT).count()));
        CHECK(requests.rtcmActivation);
        CHECK(f.receiver.messageRates.at(Msg::RTCM3_1127.value()) == 0);
        for (int service = 0; service < 4 && requests.rtcmActivation; ++service) {
            (void) f.driver.receive(100ms);
        }
        CHECK(!requests.rtcmActivation);
        CHECK(!failed(f.driver));
        CHECK(f.receiver.modes == modes);
        for (const auto& output : UBX::Plan::legacyRTCMOutput(false)) {
            CHECK(f.receiver.messageRates.at(output.rate.message.value()) == output.rate.rate);
        }
        CHECK(f.receiver.warnings.empty());
    }
}

const auto& testCases()
{
    static const struct
    {
        const char* name;
        void (*run)(GPSTestClock&);
    } cases[] = {
        {"isolated-frame-control", isolatedFrameAndControl},
        {"checked-wire-codecs", checkedWireCodecs},
        {"optional-command-write-evidence", optionalCommandWriteEvidence},
        {"legacy-rtcm-activation", legacyRTCMActivation},
        {"late-optional-valset-reply", lateOptionalValsetReply},
        {"rejected-optional-batch", rejectedOptionalBatch},
        {"legacy-rtcm-rate-polls", legacyRTCMRatePolls},
        {"slow-legacy-rtcm-activation", slowLegacyRTCMActivation},
        {"native-configuration-validation", invalidConfiguration},
        {"integrity-original-receipts", integrityReceipts},
        {"control-deadline", controlDeadline},
        {"identification-write-budget", identificationWriteBudget},
        {"explicit-no-fix", explicitNoFix},
        {"out-of-range-coordinates", outOfRangeCoordinates},
        {"read-only-baud-discovery", baudDiscovery},
        {"discovery-failures-and-late-acks", discoveryFailures},
        {"navigation-fix-flags-and-ordering", navigationFixFlags},
        {"transactional-frames", transactionalFrames},
        {"reentrant-payload", reentrantPayload},
        {"receive-read-diagnostics", receiveFailureLogging},
        {"comms-diagnostic-values", commsDiagnostics},
        {"comms-malformed-replies", invalidCommsDiagnostics},
        {"comms-expired-reply", expiredCommsDiagnostics},
        {"buffer-warning-rate-limit",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.queueBufferWarning(false);
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 0);
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 1);
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 1);
             clock.advanceBy(5000000);
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 2);
         }},
        {"buffer-poll-failure-rate-limit",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.failCommsWrite = true;
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 1);
             clock.advanceBy(5000000);
             f.receiver.failCommsWrite = false;
             f.receiver.queueBufferWarning();
             f.driver.receive(100ms);
             CHECK(f.receiver.commsPolls == 2);
         }},
        {"already-stopped",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.success(1);
         }},
        {"poll-read-failure",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.readFailure(GPSProtocolError::Transport);
         }},
        {"poll-read-cancelled",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.readFailure(GPSProtocolError::Cancelled);
         }},
        {"silent-then-stopped",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Silent, SurveyReply::Stopped};
             f.success(2);
         }},
        {"delayed-stop",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Active, SurveyReply::Active, SurveyReply::Stopped};
             f.success(3);
             CHECK(f.receiver.startedAt - f.receiver.disabledAt >= 200000);
         }},
        {"completed-survey-is-not-stopped",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Valid, SurveyReply::Stopped};
             f.success(2);
         }},
        {"bad-checksum-is-not-confirmation",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::BadChecksum, SurveyReply::Stopped};
             f.success(2);
         }},
        {"bad-length-is-not-confirmation",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::BadLength, SurveyReply::Stopped};
             f.success(2);
         }},
        {"active-timeout",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Active};
             f.timeout();
         }},
        {"valid-timeout",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Valid};
             f.timeout();
         }},
        {"silent-timeout",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.surveyReplies = {SurveyReply::Silent};
             f.timeout();
         }},
        {"reconfigure-does-not-reuse-stop-confirmation",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.success(1);
             f.receiver.resetState();
             f.receiver.surveyReplies = {SurveyReply::Silent};
             f.timeout();
         }},
        {"disable-nak",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.rejectDisable = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.surveyPolls == 0 && f.receiver.starts == 0);
         }},
        {"start-nak",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.rejectStart = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0, 1}));
             CHECK(f.receiver.surveyPolls == 1 && f.receiver.starts == 1);
         }},
        {"poll-write-failure",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.receiver.failPollWrite = true;
             CHECK(!f.configure());
             CHECK(!f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({0}));
             CHECK(f.receiver.starts == 0);
         }},
        {"configured-status-callback",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.success(1);
             f.receiver.queueSurveyReply(SurveyReply::Active);
             f.driver.receive(100ms);
             CHECK(f.receiver.statusCallbacks == 1);
         }},
        {"configured-survey-activates-rtcm",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.success(1);
             f.receiver.queueSurveyReply(SurveyReply::Valid);
             f.driver.receive(100ms);
             CHECK(f.receiver.statusCallbacks == 1);
             CHECK(f.receiver.rtcmEnables == 1);
             CHECK(!failed(f.driver));
         }},
        {"fixed-base-does-not-poll",
         [](GPSTestClock& clock) {
             Fixture f(clock);
             f.base = {.mode = GPSBaseStationConfig::Fixed{
                           .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                           .accuracyMeters = 1.0f}};
             CHECK(f.configure());
             CHECK(f.driver.receiverReady());
             CHECK(f.receiver.modes == std::vector<uint32_t>({2}));
             CHECK(f.receiver.surveyPolls == 0 && f.receiver.starts == 0);
             CHECK(f.receiver.rtcmEnables == 1);
         }},
    };

    return cases;
}
}  // namespace

class GPSProtocolUbxTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _cases_data();
    void _cases();
};

void GPSProtocolUbxTest::_cases_data()
{
    QTest::addColumn<int>("index");
    const auto& cases = testCases();
    for (size_t index = 0; index < std::size(cases); ++index) {
        QTest::newRow(cases[index].name) << static_cast<int>(index);
    }
}

void GPSProtocolUbxTest::_cases()
{
    QFETCH(int, index);
    GPSTestClock clock;
    try {
        testCases()[index].run(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolUbxTest, TestLabel::Unit)

#include "gps-ubx-test.moc"
