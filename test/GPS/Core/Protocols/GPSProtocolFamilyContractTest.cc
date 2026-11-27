#include "GPSProtocolFamilyContractTest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

#include <QtCore/QSet>

#include "GPSDriver.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverFamilies.h"
#include "LittleEndian.h"
#include "Protocols/Scenarios/GoldenScenarios.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// Every family in the family table.
struct ProtocolFamily
{
    const char* name;
    GPSType type;
    /// Each family logs under its own child of GPS.Protocols, so a single family can be enabled.
    const char* logCategory;
    /// A failed read logs only the read failure, and a cancelled operation logs nothing.
    bool plainReadFailures = false;
};

const std::array<ProtocolFamily, 7> kFamilies{{
    {"UBX", GPSType::UBlox, "GPS.Protocols.UBX", true},
    {"SBF", GPSType::Septentrio, "GPS.Protocols.SBF", true},
    {"Unicore", GPSType::Unicore, "GPS.Protocols.Unicore"},
    {"Quectel", GPSType::Quectel, "GPS.Protocols.Quectel"},
    {"Ashtech", GPSType::Trimble, "GPS.Protocols.Ashtech", true},
    {"Femto", GPSType::Femto, "GPS.Protocols.Femto", true},
    {"Passive NMEA", GPSType::Passive, "GPS.Protocols.Passive"},
}};

/// What the family's descriptor says it supports.
const GPSReceiverCapabilities& capabilities(const ProtocolFamily& family)
{
    return gpsReceiverDescriptor(family.type)->capabilities;
}

std::unique_ptr<GPSProtocolRuntime> createHost(const ProtocolFamily& family, GPSRuntimeIO io)
{
    return std::make_unique<GPSProtocolRuntime>(*gpsReceiverFamily(family.type), std::move(io));
}

/// A link that fails one kind of operation, and records a violation for any operation after that failure.
struct FaultyLink
{
    enum class Operation
    {
        Read,
        Write,
        Baud
    };

    GPSTestClock& clock;
    Operation fault;
    GPSProtocolError error;
    bool failed = false;
    unsigned operations = 0;
    QString detail = QStringLiteral("Receiver connection lost: Gerät disconnected");

    bool fail(Operation operation)
    {
        if (failed) {
            GPSTest::ModelViolations::record("Operation after the link failed");
        }
        if (++operations >= 100) {
            GPSTest::ModelViolations::record("More than 100 link operations");
        }
        failed = operation == fault;
        return failed;
    }

    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(clock);
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocolError::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                        detail};
            }
            clock.advanceTo(deadline.untilUs + 1000);
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocolError::Cancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error, 0, 0,
                        detail};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        result.setBaudRate = [this](unsigned) {
            return !fail(Operation::Baud)                 ? GPSBaudStatus::Configured
                   : error == GPSProtocolError::Cancelled ? GPSBaudStatus::Cancelled
                                                          : GPSBaudStatus::Error;
        };
        return result;
    }
};

GPSConfig validConfig(const ProtocolFamily& family)
{
    if (capabilities(family).passive) {
        return {};
    }
    if (capabilities(family).receiverAveraging) {
        return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s}}};
    }
    if (family.type == GPSType::Quectel) {
        return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 15, .duration = 60s}}};
    }
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
}

GPSConfig surveyConfig()
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
}

void addFamilyRows()
{
    QTest::addColumn<int>("familyIndex");
    for (qsizetype i = 0; i < std::ssize(kFamilies); ++i) {
        QTest::newRow(kFamilies[static_cast<size_t>(i)].name) << static_cast<int>(i);
    }
}

/// The fixed base every family is asked for: latitude 0, longitude 90 degrees, 100 m, which is ECEF (0, 6378237, 0).
const GPSReceiverConfig kFixedBase = GoldenScenario::fixedBase(0, 90, 100, 1);

/// A Septentrio PVTGeodetic block of solution @a mode at the fixed base's position, in @a datum, WGS84 by default.
QByteArray pvtGeodetic(uint8_t mode, uint32_t tow, uint8_t datum = 0)
{
    std::vector<uint8_t> body(80);
    body[0] = mode;
    (void) LittleEndian::write<double>(body, 10, std::numbers::pi / 2);
    (void) LittleEndian::write<double>(body, 18, 100.0);
    body[59] = datum;
    return toByteArray(sbfBlock(4007, body, tow));
}

/// One stage in the life of a base that determines its own position.
struct BaseStage
{
    /// What the receiver sends to enter the stage; empty where its model gets there by itself within @a duration.
    QByteArray output;
    std::chrono::milliseconds duration{0};
    /// The base status the stage leaves.
    GPSBaseState state;
    /// Whether a correction the receiver sends next reaches the driver's sink.
    bool forwards;
};

/// A family whose receivers QGC configures as a base, on the receiver model the goldens drive.
struct BaseFamily
{
    const char* name;
    GPSType type;
    GoldenScenario::BenchFactory bench;
    /// What the receiver of a fixed base sends once configured, when that matters to its status.
    QByteArray fixedOutput = {};
    /// A base that determines its own position, and its stages in order: determining it, holding it and, where the
    /// receiver can report that, losing it.
    GPSReceiverConfig survey = {};
    std::vector<BaseStage> stages = {};
};

const std::vector<BaseFamily>& baseFamilies()
{
    using namespace GoldenScenario;
    using enum GPSBaseState;
    constexpr std::string_view POSITION =
        "PASHR,POS,2,12,114501.00,0000.00000,N,09000.00000,E,100.000,0,90,10,0,1,1,1,1,";
    static const std::vector<BaseFamily> families{
        // NAV-SVIN reports no survey-in, which says nothing about a fixed base, and the state of one that surveys.
        {"UBX",
         GPSType::UBlox,
         ubxWire(),
         navSvin(1000, 0, false, false),
         surveyIn(2, 180),
         {{navSvin(1000, 30, false, true), 0ms, Active, false},
          {navSvin(2000, 181, true, false), 0ms, Valid, true},
          {navSvin(3000, 0, false, false), 0ms, Inactive, false}}},
        // PVTGeodetic reports a fixed location, three-dimensional and without error; before that, that the receiver is
        // still determining it (0x40), and at the end a datum the base was not configured for. GPSDriver forwards this
        // family's corrections whatever it reports, until the datum ends the session.
        {"SBF",
         GPSType::Septentrio,
         sbf(),
         pvtGeodetic(3, 1000),
         surveyIn(1, 60),
         {{pvtGeodetic(0x41, 1000), 0ms, Active, true},
          {pvtGeodetic(3, 2000), 0ms, Valid, true},
          {pvtGeodetic(3, 3000, 19), 0ms, Inactive, false}}},
        // The receiver averages for five seconds, reports FIXEDPOS every second from then on, and at the end a
        // standalone solution in a later week.
        {"Unicore",
         GPSType::Unicore,
         unicore(),
         {},
         averaging(5),
         {{{}, 0ms, Active, false},
          {{}, 8s, Valid, true},
          {QByteArray::fromStdString(unicorePosition("SINGLE", {0, 6378237, 0}, 1000, 2327)), 0ms, Inactive, false}}},
        // The receiver has the fixed base saved, so the survey is written with consent. It surveys for ten seconds,
        // reporting every second, and at the end restarts.
        {"Quectel",
         GPSType::Quectel,
         quectel([](QuectelBench& b) {
             b.model.role = 2;
             b.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
         }),
         {},
         persistent(surveyIn(15, 10)),
         {{{}, 2s, Active, false}, {{}, 12s, Valid, true}, {toByteArray(QUECTEL_BOOT), 0ms, Inactive, false}}},
        // The first position starts the base setup, which sets the fixed position or starts the survey that the
        // finish receipt completes.
        {"Ashtech",
         GPSType::Trimble,
         ashtech(),
         nmea(POSITION),
         surveyIn(1, 100),
         {{nmea(POSITION), 0ms, Active, false}, {nmea(ASHTECH_SURVEY_FINISHED), 0ms, Valid, true}}},
        // A GGA of quality 7 says that position averaging finished.
        {"Femto",
         GPSType::Femto,
         femto(),
         {},
         surveyIn(1, 60),
         {{nmea("GPGGA,123519.00,0000.00000,N,09000.00000,E,1,12,0.9,100.000,M,0.000,M,,"), 0ms, Active, false},
          {nmea("GPGGA,123520.00,0000.00000,N,09000.00000,E,7,12,0.9,100.000,M,0.000,M,,"), 0ms, Valid, true}}},
    };
    return families;
}

void addBaseFamilyRows()
{
    QTest::addColumn<int>("baseIndex");
    for (qsizetype i = 0; i < std::ssize(baseFamilies()); ++i) {
        QTest::newRow(baseFamilies()[static_cast<size_t>(i)].name) << static_cast<int>(i);
    }
}

/// A GPSDriver that configures a family's receiver model as a base, and what it delivered to its sinks.
class DrivenBase
{
public:
    DrivenBase(const BaseFamily& family, const GPSReceiverConfig& config)
        : _bench(family.bench(_clock))
        , _driver(family.type, *_bench, config, _sinks(), _driverClock())
    {
        _bench->startSession();
    }

    [[nodiscard]] bool configure() { return _driver.configure(); }

    /// The receiver sends @a output, and the driver receives until it has read it and @a duration has passed.
    void receive(const QByteArray& output, std::chrono::milliseconds duration = 0ms)
    {
        if (!output.isEmpty()) {
            _bench->queueReply(output);
        }
        const uint64_t until = _clock.nowUs() + static_cast<uint64_t>(std::chrono::microseconds(duration).count());
        for (int calls = 0; calls < MAX_RECEIVE_CALLS; ++calls) {
            if (_driver.receiveOutcome(RECEIVE_SLICE).terminal() ||
                (!_bench->hasQueuedReadData() && _clock.nowUs() >= until)) {
                return;
            }
        }
    }

    std::vector<GPSBaseStatus> statuses;
    int corrections = 0;

private:
    static constexpr std::chrono::milliseconds RECEIVE_SLICE{100};
    static constexpr int MAX_RECEIVE_CALLS = 500;

    GPSDriverSinks _sinks()
    {
        GPSDriverSinks sinks;
        sinks.onRTCM = [this](const QByteArray&) { ++corrections; };
        sinks.onBaseStatus = [this](const GPSBaseStatus& status) { statuses.push_back(status); };
        return sinks;
    }

    /// The driver's waits run the events of a receiver model that has them.
    GPSClock _driverClock()
    {
        GPSClock clock = _clock.source();
        if (_bench->wait) {
            clock.wait = _bench->wait;
        }
        return clock;
    }

    GPSTestClock _clock{GPSTestClock::START_US};
    std::unique_ptr<ReceiverBench> _bench;
    GPSDriver _driver;
};
}  // namespace

void GPSProtocolFamilyContractTest::_familyTable()
{
    // The family table lists every receiver family once, each with its descriptor. Detection probes in table order,
    // and passive input, which answers no probe, comes last.
    const auto families = gpsReceiverFamilies();
    QCOMPARE(families.size(), gpsReceiverDescriptors().size());
    QCOMPARE(families.size(), kFamilies.size());
    QSet<GPSType> types;
    for (const GPSReceiverFamily* family : families) {
        QVERIFY(!types.contains(family->type));
        types.insert(family->type);
        QVERIFY(gpsReceiverDescriptor(family->type));
        QCOMPARE(gpsReceiverFamily(family->type), family);
    }
    QCOMPARE(families.back()->type, GPSType::Passive);
    QVERIFY(families.back()->baudCandidates.empty());
    for (const auto& family : kFamilies) {
        const auto* descriptor = gpsReceiverFamily(family.type);
        QVERIFY2(descriptor, family.name);
        QCOMPARE(descriptor->type, family.type);
        QVERIFY2(descriptor->create && descriptor->logCategory, family.name);
        QCOMPARE(descriptor->logCategory().categoryName(), family.logCategory);
        QVERIFY2(descriptor->create(), family.name);
    }
    QVERIFY(!gpsReceiverFamily(static_cast<GPSType>(-1)));
}

void GPSProtocolFamilyContractTest::_decodeIsPure_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_decodeIsPure()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    GPSTestClock clock;
    auto protocol = createHost(family, makeDecoderOnlyIO(clock));
    constexpr std::array<uint8_t, 8> noise{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
    for (size_t split = 0; split <= noise.size(); ++split) {
        const auto first = protocol->decode(std::span(noise).first(split));
        const auto second = protocol->decode(std::span(noise).subspan(split));
        QVERIFY(first.events.empty());
        QVERIFY(second.events.empty());
    }
    const auto empty = protocol->decode({});
    QVERIFY(empty.events.empty());
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    if (capabilities(family).surveyIn || capabilities(family).passive) {
        QSKIP("Family supports survey-in, or takes no base configuration");
    }
    GPSTestClock clock;
    auto protocol = createHost(family, makeDecoderOnlyIO(clock));
    unsigned baud = capabilities(family).passive ? 115200 : 0;
    QVERIFY2(!protocol->configure(surveyConfig(), baud), family.name);
    QVERIFY(!protocol->receiverReady());
    // A family refuses an unsupported request with a description; GPSDriver logs it.
    QVERIFY(!protocol->errorDetail().isEmpty());
}

void GPSProtocolFamilyContractTest::_logCategory_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_logCategory()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    const bool passive = capabilities(family).passive;
    const GPSProtocolLogCapture log;
    GPSTestClock clock;
    // Passive input only sets the baud rate; every other family reads a reply first.
    FaultyLink link{clock, passive ? FaultyLink::Operation::Baud : FaultyLink::Operation::Read,
                    GPSProtocolError::Transport};
    auto host = createHost(family, link.io());
    unsigned baud = passive ? 115200 : 0;
    QVERIFY(!host->configure(validConfig(family), baud));
    // Whatever the family logs uses its own category.
    QCOMPARE(log.categories().count(QString::fromLatin1(family.logCategory)), log.categories().size());
}

void GPSProtocolFamilyContractTest::_linkFailure_data()
{
    QTest::addColumn<int>("familyIndex");
    QTest::addColumn<FaultyLink::Operation>("fault");
    QTest::addColumn<GPSProtocolError>("error");
    for (size_t index = 0; index < kFamilies.size(); ++index) {
        const auto& family = kFamilies[index];
        for (const auto& [fault, operation] :
             {std::pair{FaultyLink::Operation::Read, "read"}, std::pair{FaultyLink::Operation::Write, "write"},
              std::pair{FaultyLink::Operation::Baud, "baud"}}) {
            // Passive input only sets the baud rate.
            if (capabilities(family).passive && fault != FaultyLink::Operation::Baud) {
                continue;
            }
            for (const auto& [error, outcome] : {std::pair{GPSProtocolError::Cancelled, "cancelled"},
                                                 std::pair{GPSProtocolError::Transport, "failed"}}) {
                QTest::addRow("%s-%s-%s", family.name, operation, outcome) << int(index) << fault << error;
            }
        }
    }
}

void GPSProtocolFamilyContractTest::_linkFailure()
{
    QFETCH(int, familyIndex);
    QFETCH(FaultyLink::Operation, fault);
    QFETCH(GPSProtocolError, error);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    const bool passive = capabilities(family).passive;
    GPSTestClock clock;
    FaultyLink link{clock, fault, error};
    const GPSProtocolLogCapture log;
    auto receiver = createHost(family, link.io());
    GPSConfig config{};
    if (capabilities(family).receiverAveraging) {
        config.base.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s};
    } else if (!passive) {
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    }
    unsigned baudRate = passive ? 115200 : 0;
    const bool configured = receiver->configure(config, baudRate);
    QVERIFY(link.failed);
    QVERIFY(!configured);
    QCOMPARE(receiver->error(), error);
    if (fault != FaultyLink::Operation::Baud) {
        QVERIFY(!receiver->errorDetail().isEmpty());
    }
    const auto warnings = log.warnings();
    const bool readFailure = fault == FaultyLink::Operation::Read && error != GPSProtocolError::Cancelled;
    if (readFailure && family.plainReadFailures) {
        QCOMPARE(warnings, QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                           .arg(static_cast<int>(GPSReadStatus::Error))
                                           .arg(link.detail)});
    } else if (readFailure) {
        QVERIFY(!warnings.empty());
    } else if (family.plainReadFailures) {
        QVERIFY(warnings.empty());
    }
    QCOMPARE(receiver->receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(receiver->error(), error);
    if (fault != FaultyLink::Operation::Baud) {
        QVERIFY(!receiver->errorDetail().isEmpty());
    }
    QCOMPARE(log.warnings(), warnings);
}

void GPSProtocolFamilyContractTest::_fixedBaseReportsValid_data()
{
    addBaseFamilyRows();
}

void GPSProtocolFamilyContractTest::_fixedBaseReportsValid()
{
    QFETCH(int, baseIndex);
    const auto& family = baseFamilies()[static_cast<size_t>(baseIndex)];
    DrivenBase base(family, kFixedBase);
    QVERIFY(base.configure());
    base.receive(family.fixedOutput, 2s);
    // Every family reports the fixed position it runs on, whichever of its messages says so.
    QVERIFY(!base.statuses.empty());
    const GPSBaseStatus& status = base.statuses.back();
    QCOMPARE(status.state, GPSBaseState::Valid);
    QVERIFY(matches(status.position.latitudeDegrees, 0));
    QVERIFY(matches(status.position.longitudeDegrees, 90));
    QVERIFY(!status.meanAccuracyMeters);
    // So its corrections are forwarded.
    base.receive(GoldenScenario::rtcm1005());
    QCOMPARE(base.corrections, 1);
}

void GPSProtocolFamilyContractTest::_correctionsFollowBaseStatus_data()
{
    addBaseFamilyRows();
}

void GPSProtocolFamilyContractTest::_correctionsFollowBaseStatus()
{
    QFETCH(int, baseIndex);
    const auto& family = baseFamilies()[static_cast<size_t>(baseIndex)];
    // A receiver that loses its base fails the session, with a warning.
    const GPSProtocolLogCapture log;
    DrivenBase base(family, family.survey);
    QVERIFY(base.configure());
    for (const BaseStage& stage : family.stages) {
        const int forwarded = base.corrections;
        base.receive({}, stage.duration);
        // In one read, so that the correction follows what the receiver reports even when that ends the session.
        base.receive(stage.output + GoldenScenario::rtcm1005());
        QVERIFY(!base.statuses.empty());
        QCOMPARE(base.statuses.back().state, stage.state);
        QCOMPARE(base.corrections - forwarded, stage.forwards ? 1 : 0);
    }
}

void GPSProtocolFamilyContractTest::_passiveInputForwardsCorrections()
{
    // A receiver that was not configured here reports no base, so its corrections wait for none.
    const BaseFamily family{.name = "Passive NMEA", .type = GPSType::Passive, .bench = GoldenScenario::passive()};
    DrivenBase input(family, GoldenScenario::passiveInput(115200));
    QVERIFY(input.configure());
    input.receive(GoldenScenario::rtcm1005());
    QVERIFY(input.statuses.empty());
    QCOMPARE(input.corrections, 1);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolFamilyContractTest, TestLabel::Unit)
