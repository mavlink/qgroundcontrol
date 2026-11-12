#include "GPSDriverTest.h"

#include <chrono>
#include <optional>
#include <string>

#include <QtCore/QByteArray>

#include "GPSCancellation.h"
#include "GPSDecodedData.h"
#include "GPSDriver.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/GPSBaseStationConfigs.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

const GPSCancelToken neverStop{};

/// What the receiver outputs next, replacing output the driver has not read.
void send(ScriptedReceiver& link, const QByteArray& bytes)
{
    link.clearReplies();
    link.queueReply(bytes);
}

class ConfigurationProbeTransport
{
public:
    ConfigurationProbeTransport()
        : transport(neverStop)
    {
        transport.setClock(&clock);
        transport.setConfigurationWriteTimeoutHandler([this] { return cap; });
        transport.setWriteHandler(
            [this](const QByteArray& bytes,
                   const ScriptedReceiver::WriteContext& context) -> std::optional<GPSWriteResult> {
                ++boundedCalls;
                budgetMs = context.transportDeadline.remainingTime();
                if (delayReturn) {
                    // A write that returns only after its deadline has passed.
                    clock.advanceBy(static_cast<uint64_t>(budgetMs + 1) * 1000);
                    const int length = bytes.size();
                    return GPSWriteResult{GPSWriteStatus::Completed, length, length};
                }
                return GPSWriteResult{GPSWriteStatus::Unsupported};
            });
    }

    operator GPSTransport&() { return transport; }

    GPSTestClock clock{GPSTestClock::START_US};
    ScriptedReceiver transport;
    std::chrono::milliseconds cap{500};
    bool delayReturn = false;
    int boundedCalls = 0;
    qint64 budgetMs = 0;
};

}  // namespace

void GPSDriverTest::_ashtechSatelliteSnapshots()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<AshtechReceiverModel> link(clock);
    std::vector<GPSSatelliteReport> snapshots;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const auto& snapshot) { snapshots.push_back(snapshot); };
    GPSDriver driver(GPSType::Trimble, link, {.base = fixedBaseAt(47, 8, 500)}, sinks, clock.source());
    QVERIFY(driver.configure());
    unsigned epoch = 120000;
    const auto feed = [&](QByteArrayView body) {
        snapshots.clear();
        send(link, nmeaBytes(body) + nmeaBytes("GNRMC," + QByteArray::number(++epoch) + ".00,V,,,,,,,090926,,,N"));
        return driver.receiveOutcome(20ms);
    };
    QCOMPARE(feed("GPGSV,1,1,01,01,10,20,30").liveness, GPSReceiveLiveness::Data);
    // The empty SBAS scope must not clear GPS; its unchanged counts are not republished.
    QCOMPARE(snapshots.size(), size_t(1));
    QCOMPARE(snapshots[0].inView, std::optional<int>{1});
    QCOMPARE(feed("GLGSV,1,1,01,65,20,30,40").updates, GPSReceiveUpdates(GPSReceiveUpdate::Satellites));
    QCOMPARE(snapshots.back().inView, std::optional<int>{2});
    QCOMPARE(feed("GPGSV,1,1,02,01,10,20,30,33,15,25,35").liveness, GPSReceiveLiveness::Data);
    QCOMPARE(snapshots.back().inView, std::optional<int>{3});
    // Each satellite counts for its constellation, satellite 33 for SBAS; without GSA none says which are used.
    using Constellation = GPSSatelliteReport::Constellation;
    auto constellations = snapshots.back().constellationsInView();
    QCOMPARE(constellations.size(), size_t(3));
    QCOMPARE(constellations[0], (Constellation{GPSConstellation::GPS, 1, std::nullopt}));
    QCOMPARE(constellations[1], (Constellation{GPSConstellation::GLONASS, 1, std::nullopt}));
    QCOMPARE(constellations[2], (Constellation{GPSConstellation::SBAS, 1, std::nullopt}));
    QCOMPARE(feed("GLGSV,1,1,00").liveness, GPSReceiveLiveness::Data);
    QCOMPARE(snapshots.back().inView, std::optional<int>{2});
    constellations = snapshots.back().constellationsInView();
    QCOMPARE(constellations.size(), size_t(2));
    QCOMPARE(constellations[1].constellation, GPSConstellation::SBAS);
    // An empty GPS view empties its SBAS scope with it, in one report.
    QCOMPARE(feed("GPGSV,1,1,00").liveness, GPSReceiveLiveness::Data);
    QCOMPARE(snapshots.size(), size_t(1));
    QCOMPARE(snapshots[0].inView, std::optional<int>{0});
    QVERIFY(snapshots[0].constellationsInView().empty());
}

void GPSDriverTest::_nativeIntegrityProvenance()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<UBXReceiverModel> transport(clock, UBXReceiverModel::Receiver::F9P);
    auto& receiver = transport.model;
    std::vector<GPSPositionReport> positions;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const auto& report) { positions.push_back(report); };
    GPSDriver driver(GPSType::UBlox, transport, {.base = fixedBaseAt(47, 8, 500, 1)}, sinks, clock.source());
    QVERIFY(driver.configure());
    receiver.coalesceReplies = true;
    const auto navigation = [&](uint32_t tow) {
        auto pvt = ubxFix3D();
        (void) LittleEndian::write<int32_t>(pvt, 32, 500000);
        (void) LittleEndian::write<int32_t>(pvt, 36, 500000);
        receiver.queueBytes(ubxNavigationEpoch(pvt, tow));
        for (int attempt = 0; attempt < 12; ++attempt) {
            QVERIFY(!driver.receiveOutcome(20ms).terminal());
            if (!positions.empty()) {
                break;
            }
        }
    };
    QByteArray rf(28, '\0');
    rf[1] = 1;
    rf[5] = 3;
    receiver.queueFrame(0x0a, 0x38, rf);
    navigation(1000);
    QVERIFY(!positions.empty());
    const auto first = positions.back();
    QCOMPARE(first.integrity.jamming.state, GPSIntegrityReport::JammingState::Critical);
    QVERIFY(first.integrity.jamming.timestampUs > 0);

    // The next epoch, a second later, adds a spoofing indication.
    clock.advanceBy(1000000);
    QByteArray status(16, '\0');
    status[7] = 2 << 3;
    receiver.queueFrame(0x01, 0x03, status);
    navigation(2000);
    QCOMPARE(positions.size(), size_t{2});
    const auto second = positions.back();
    QCOMPARE(second.integrity.jamming.timestampUs, first.integrity.jamming.timestampUs);
    QVERIFY(second.integrity.spoofing.timestampUs > first.integrity.jamming.timestampUs);
    QCOMPARE(second.integrity.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    const auto fresh = second.integrity.freshAt(first.integrity.jamming.timestampUs + 5'000'000);
    QCOMPARE(fresh.jamming.state, GPSIntegrityReport::JammingState::Unknown);
    QCOMPARE(fresh.spoofing.state, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(first.integrity.jamming.state, GPSIntegrityReport::JammingState::Critical);
}

void GPSDriverTest::_femtoSatelliteUsage()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<FemtoReceiverModel> link(clock);
    std::vector<GPSSatelliteReport> reports;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const auto& report) { reports.push_back(report); };
    GPSDriver driver(GPSType::Femto, link, {.base = fixedBaseAt(47, 8, 500)}, sinks, clock.source());
    QVERIFY(driver.configure());
    for (const auto& count : {QByteArray("12"), QByteArray("00"), QByteArray()}) {
        send(link, nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1," + count + ",0.9,545.4,M,46.9,M,,"));
        const auto result = driver.receiveOutcome(20ms);
        QCOMPARE(result.liveness, GPSReceiveLiveness::Data);
        QVERIFY(result.updates.testFlag(GPSReceiveUpdate::Satellites));
    }
    QCOMPARE(reports.size(), size_t(3));
    QVERIFY(!reports[0].inView);
    QCOMPARE(reports[0].used, std::optional<int>{12});
    QCOMPARE(reports[1].used, std::optional<int>{0});
    QVERIFY(!reports[2].used);
    QCOMPARE(reports[0].timestampUs, uint64_t{0});
}

void GPSDriverTest::_receiveOutcomes()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<FemtoReceiverModel> link(clock);
    GPSDriver driver(GPSType::Femto, link, {.base = fixedBaseAt(47, 8, 500)}, {}, clock.source());
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::InvalidArgument);
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    std::optional<GPSReadResult> readFailure;
    link.setReadHandler([&readFailure](uint8_t*, int, std::chrono::milliseconds) { return readFailure; });
    link.clearReplies();
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Idle);
    send(link, nmeaBytes("GPTXT,01,01,02,diagnostic"));
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Activity);
    const QString detail = QStringLiteral("Receiver disconnected: Gerät");
    readFailure = GPSReadResult{GPSReadStatus::Error, 0, detail};
    expectLogMessage("GPS.Protocols.Femto", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed \\(status %1\\): %2")
                                            .arg(static_cast<int>(GPSReadStatus::Error))
                                            .arg(QRegularExpression::escape(detail))));
    const auto failed = driver.receiveOutcome(0ms);
    verifyExpectedLogMessage();
    QCOMPARE(failed.error, GPSProtocolError::Transport);
    QCOMPARE(failed.detail, detail);
    QVERIFY(!link.fatalError());
    const auto latched = driver.receiveOutcome(0ms);
    QCOMPARE(latched.error, GPSProtocolError::Transport);
    QCOMPARE(latched.detail, detail);
    readFailure.reset();
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    readFailure = GPSReadResult{GPSReadStatus::Cancelled, 0, QStringLiteral("Receiver stopped")};
    const auto cancelled = driver.receiveOutcome(0ms);
    QCOMPARE(cancelled.error, GPSProtocolError::Cancelled);
    QCOMPARE(cancelled.detail, readFailure->detail);
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationError(), readFailure->detail);
    readFailure.reset();
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    QVERIFY(driver.receiveOutcome(0ms).detail.isEmpty());
    // A link that failed between reads reports why, through a zero-length read.
    const QString linkDetail = QStringLiteral("Serial GPS input buffer exhausted");
    link.setReadHandler([&](uint8_t*, int length, std::chrono::milliseconds) {
        return length == 0 ? std::optional(GPSReadResult{GPSReadStatus::Overflow, 0, linkDetail}) : std::nullopt;
    });
    link.setFatalError(true);
    const auto lost = driver.receiveOutcome(0ms);
    QCOMPARE(lost.error, GPSProtocolError::Transport);
    QCOMPARE(lost.detail, linkDetail);
}

void GPSDriverTest::_rtcmActivationRejected()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<UBXReceiverModel> transport(clock, UBXReceiverModel::Receiver::F9P);
    auto& receiver = transport.model;
    GPSDriver driver(GPSType::UBlox, transport, {.base = surveyInBase(2, 1s)}, {}, clock.source());
    QVERIFY(driver.configure());
    receiver.rejectRtcmActivation = true;
    QByteArray survey(40, '\0');
    (void) LittleEndian::write<uint32_t>(mutableBytesOf(survey), 8, 5);
    survey[36] = 1;
    receiver.queueFrame(0x01, 0x3b, survey);
    GPSReceiveResult result;
    for (int attempt = 0; attempt < 4; ++attempt) {
        result = driver.receiveOutcome(20ms);
        if (result.terminal()) {
            break;
        }
    }
    QCOMPARE(result.error, GPSProtocolError::Protocol);
    QCOMPARE(result.detail, QStringLiteral("u-blox receiver did not start RTCM output after the survey-in"));
    QVERIFY(result.terminal());
    QVERIFY(!receiver.readError());
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::Protocol);
    transport.cancel();
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::Protocol);
}

void GPSDriverTest::_configurationDeadline_data()
{
    QTest::addColumn<int>("capMs");
    QTest::addColumn<bool>("delayReturn");
    QTest::newRow("desktop-default") << 500 << false;
    QTest::newRow("short-transport-cap") << 20 << false;
    QTest::newRow("tcp-cap-command-deadline") << 5000 << false;
    QTest::newRow("expired-write") << 500 << true;
}

void GPSDriverTest::_configurationDeadline()
{
    QFETCH(int, capMs);
    QFETCH(bool, delayReturn);
    ConfigurationProbeTransport transport;
    transport.cap = std::chrono::milliseconds(capMs);
    transport.delayReturn = delayReturn;
    GPSDriver driver(GPSType::UBlox, transport, {.base = fixedBaseAt(47, 8, 500, 1)}, {}, transport.clock.source());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.budgetMs > 0);
    QVERIFY(transport.budgetMs <= std::min(capMs, 250));
    if (delayReturn) {
        // A write that returns after its deadline still sends its whole frame; only the reply wait times out.
        QVERIFY(transport.boundedCalls >= 1);
        QVERIFY(!driver.configurationEvidence().empty());
        const auto& evidence = driver.configurationEvidence().front();
        QVERIFY(evidence.acceptedBytes > 0);
        QCOMPARE(evidence.writtenBytes, evidence.acceptedBytes);
    }
}

void GPSDriverTest::_configurationWriteEvidence_data()
{
    // GPSCommandChannelTest covers every write result; these show the driver reports the failed write's evidence.
    QTest::addColumn<GPSWriteResult>("result");
    QTest::newRow("error-after-write") << GPSWriteResult{GPSWriteStatus::Error, 6, 6};
    QTest::newRow("cancelled") << GPSWriteResult{GPSWriteStatus::Cancelled, 0, 0};
}

void GPSDriverTest::_configurationWriteEvidence()
{
    QFETCH(GPSWriteResult, result);
    result.detail = QStringLiteral("Configuration write failed: Gerät");
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    link.setWriteHandler(
        [result](const QByteArray&, const ScriptedReceiver::WriteContext&) { return std::optional(result); });
    GPSDriver driver(GPSType::UBlox, link, {.base = fixedBaseAt(47, 8, 500, 1)}, {}, clock.source());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationError(), result.detail);
    QVERIFY(!driver.configurationEvidence().empty());
    for (const auto& command : driver.configurationEvidence()) {
        QCOMPARE(command.outcome, result.status == GPSWriteStatus::Cancelled ? GPSCommandOutcome::Cancelled
                                                                             : GPSCommandOutcome::TransportError);
        QCOMPARE(command.acceptedBytes, result.acceptedBytes);
        QCOMPARE(command.writtenBytes, result.writtenBytes);
    }
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::InvalidArgument);
}

void GPSDriverTest::_ubloxRoleTransition_data()
{
    QTest::addColumn<UBXReceiverModel::Receiver>("model");
    QTest::addColumn<bool>("fixed");
    QTest::newRow("M8P-fixed") << UBXReceiverModel::Receiver::M8PBase << true;
    QTest::newRow("M8P-survey") << UBXReceiverModel::Receiver::M8PBase << false;
    QTest::newRow("F9P-fixed") << UBXReceiverModel::Receiver::F9P << true;
    QTest::newRow("F9P-survey") << UBXReceiverModel::Receiver::F9P << false;
}

void GPSDriverTest::_ubloxRoleTransition()
{
    QFETCH(UBXReceiverModel::Receiver, model);
    QFETCH(bool, fixed);
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<UBXReceiverModel> transport(clock, model);
    auto& receiver = transport.model;
    {
        GPSReceiverConfig config;
        config.base = fixed ? fixedBaseAt(47.0, 8.0, 500.0f, 0.0f) : surveyInBase(2.0, 180s);
        GPSDriver base(GPSType::UBlox, transport, config, {}, clock.source());
        QVERIFY(base.configure());
        QCOMPARE(receiver.timeMode, fixed ? 2u : 1u);
        if (fixed) {
            QCOMPARE(receiver.fixedAccuracy, 0u);
        }
    }
    // Destroying the driver leaves the receiver in its base role.
    QCOMPARE(receiver.timeMode, fixed ? 2u : 1u);
    QCOMPARE(receiver.navigationModel, 2u);
    QCOMPARE(receiver.resetCommands, 0);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_invalidConfiguration_data()
{
    // GPSReceiverConfigTest covers validation; these show the driver refuses an invalid base before any I/O.
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::addColumn<QString>("message");
    QTest::newRow("zero-survey-accuracy")
        << GPSBaseStationConfig{.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 0, .duration = 180s}}
        << QStringLiteral("Enter a valid survey-in accuracy and duration");
    QTest::newRow("missing-fixed-position") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{}}
                                            << QStringLiteral("Enter a valid fixed base position and accuracy");
}

void GPSDriverTest::_invalidConfiguration()
{
    QFETCH(GPSBaseStationConfig, config);
    QFETCH(QString, message);
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    GPSDriver driver(GPSType::UBlox, link, GPSReceiverConfig{.base = config}, GPSDriverSinks{}, clock.source());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(message)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(link.writes(), 0);
    QCOMPARE(link.baudChanges(), 0);
    QCOMPARE(link.reads(), 0);
    QCOMPARE(driver.receiveOutcome(10ms).error, GPSProtocolError::InvalidArgument);
}

void GPSDriverTest::_nativeConfigurationRejectedBeforeIo_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<unsigned>("fixedBaud");
    QTest::addColumn<QString>("message");
    QTest::newRow("unknown-receiver") << 255 << GPSReceiverConfig{} << 0u
                                      << QStringLiteral("Unsupported GPS receiver type");
    QTest::newRow("passive-invalid-baud") << int(GPSType::Passive) << GPSReceiverConfig{.baudRate = 1} << 0u
                                          << QStringLiteral("Select a valid serial baud rate");
    QTest::newRow("fixed-baud-mismatch")
        << int(GPSType::UBlox)
        << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2, .duration = 180s}},
                             .baudRate = 38400}
        << 115200u << QStringLiteral("Selected baud rate differs from the transport's fixed baud rate");
}

void GPSDriverTest::_nativeConfigurationRejectedBeforeIo()
{
    QFETCH(int, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(unsigned, fixedBaud);
    QFETCH(QString, message);
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    link.setFixedBaudRate(fixedBaud);
    GPSDriver driver(static_cast<GPSType>(type), link, config, {}, clock.source());
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::InvalidArgument);
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(message)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.receiveOutcome(0ms).error, GPSProtocolError::InvalidArgument);
    QCOMPARE(link.opens(), 0);
    QCOMPARE(link.writes(), 0);
    QCOMPARE(link.baudChanges(), 0);
    QCOMPARE(link.reads(), 0);
}

namespace {

/// A receiver that shows its family at once: u-blox with a queued NAV-PVT frame, Septentrio streaming SBF blocks.
struct AutomaticLink
{
    explicit AutomaticLink(GPSType type)
        : ublox(clock, UBXReceiverModel::Receiver::F9P)
        , septentrio(clock)
        , transport(type == GPSType::UBlox ? static_cast<ReceiverBench&>(ublox) : septentrio)
    {
        transport.setFixedBaudRate(115200);
        if (type == GPSType::UBlox) {
            ublox.model.queueFrame(0x01, 0x07, QByteArray(92, '\0'));
        } else {
            septentrio.model.streaming = true;
        }
    }

    GPSTestClock clock{GPSTestClock::START_US};
    ModelReceiver<UBXReceiverModel> ublox;
    ModelReceiver<SBFReceiverModel> septentrio;
    ReceiverBench& transport;
};

}  // namespace

void GPSDriverTest::_automaticDetection_data()
{
    QTest::addColumn<GPSType>("detected");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<QString>("error");
    QTest::addColumn<QString>("warning");

    const GPSBaseStationConfig survey = surveyInBase(2, 180s);
    const GPSBaseStationConfig averaging = averagingBase(60s);
    QTest::newRow("ublox") << GPSType::UBlox << GPSReceiverConfig{.base = survey} << QString() << QString();
    // Consent is a permission: a family without persistent configuration ignores it.
    QTest::newRow("ublox-ignores-consent")
        << GPSType::UBlox << GPSReceiverConfig{.base = survey, .allowPersistentChanges = true} << QString()
        << QString();
    GPSBaseStationConfig compact = survey;
    compact.compactObservations = true;
    QTest::newRow("septentrio-compact-falls-back")
        << GPSType::Septentrio << GPSReceiverConfig{.base = compact} << QString()
        << QStringLiteral("has no compact \\(MSM4\\) RTCM option");
    // A family that sends its own set of constellations sends it whatever the request leaves out.
    GPSBaseStationConfig gpsOnly = survey;
    gpsOnly.correctionConstellations = {};
    QTest::newRow("septentrio-constellations-fall-back")
        << GPSType::Septentrio << GPSReceiverConfig{.base = gpsOnly} << QString()
        << QStringLiteral("has no choice of RTCM constellations");
    QTest::newRow("ublox-keeps-constellations")
        << GPSType::UBlox << GPSReceiverConfig{.base = gpsOnly} << QString() << QString();
    QTest::newRow("septentrio-averaging-unsupported")
        << GPSType::Septentrio << GPSReceiverConfig{.base = averaging}
        << QStringLiteral("Detected Septentrio receiver does not support receiver-managed averaging")
        << QStringLiteral("does not support receiver-managed averaging");
}

void GPSDriverTest::_automaticDetection()
{
    QFETCH(GPSType, detected);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(QString, error);
    QFETCH(QString, warning);
    AutomaticLink link(detected);
    QList<GPSType> reported;
    GPSDriverSinks sinks;
    sinks.onReceiverDetected = [&](GPSType type) {
        reported.append(type);
        link.septentrio.model.streaming = false;
    };
    GPSDriver driver(GPSType::Automatic, link.transport, config, sinks, link.clock.source());
    if (!warning.isEmpty()) {
        expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(warning));
    }
    QCOMPARE(driver.configure(), error.isEmpty());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(driver.configurationError(), error);
    QCOMPARE(reported, QList<GPSType>{detected});
    if (!error.isEmpty()) {
        // An unsupported request fails before any receiver command.
        QVERIFY(driver.configurationEvidence().empty());
        QVERIFY(link.transport.commands().isEmpty());
    }
}

void GPSDriverTest::_automaticDetectionCancelled_data()
{
    QTest::addColumn<GPSDetectionHint>("hint");
    QTest::newRow("scan") << GPSDetectionHint{};
    QTest::newRow("hint") << GPSDetectionHint{.family = GPSType::UBlox, .baud = 115200, .foundBaud = 38400};
}

void GPSDriverTest::_automaticDetectionCancelled()
{
    QFETCH(GPSDetectionHint, hint);
    AutomaticLink link(GPSType::UBlox);
    link.transport.cancel();
    int reported = 0;
    GPSDriverSinks sinks;
    sinks.onReceiverDetected = [&](GPSType) { ++reported; };
    GPSDriver driver(GPSType::Automatic, link.transport, {.base = surveyInBase(2, 180s)}, sinks, link.clock.source());
    driver.setDetectionHint(hint);
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Receiver detection failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationError(), QStringLiteral("Receiver detection cancelled"));
    QCOMPARE(reported, 0);
    QVERIFY(link.transport.commands().isEmpty());
}

void GPSDriverTest::_detectionSkipsRefusedRate()
{
    // A serial adapter that cannot run the first candidate rate stays usable at the next one.
    AutomaticLink link(GPSType::UBlox);
    link.transport.setFixedBaudRate(0);
    link.transport.setReceiverBaudRate(38400);
    link.transport.setBaudRateEnforced(true);
    QList<unsigned> rates;
    link.transport.setBaudRateHandler([&rates](unsigned rate) -> std::optional<bool> {
        rates.append(rate);
        return rate == 115200 ? std::optional<bool>(false) : std::nullopt;
    });
    QList<GPSType> reported;
    GPSDriverSinks sinks;
    sinks.onReceiverDetected = [&](GPSType type) { reported.append(type); };
    GPSDriver driver(GPSType::Automatic, link.transport, {.base = surveyInBase(2, 180s)}, sinks, link.clock.source());
    // Configuration then needs the refused rate and names it.
    const QString error = QStringLiteral("Detected u-blox receiver at 38400 baud: The link cannot run at 115200 baud");
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(error)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(reported, QList<GPSType>{GPSType::UBlox});
    QCOMPARE(rates.mid(0, 2), (QList<unsigned>{115200, 38400}));
    QCOMPARE(driver.configurationError(), error);
}

namespace {

/// A u-blox F9P behind receiver detection, as the automatic golden scenarios set one up: it answers only while the
/// host runs the link at its rate, which its configuration moves to 115200.
struct DetectableUBlox
{
    explicit DetectableUBlox(unsigned baud)
        : link(clock, UBXReceiverModel::Receiver::F9P)
    {
        (void) link.detect(Dialect::UBX);
        link.model.receiverBaud = baud;
        link.atRate = [this] { return link.model.hostBaud == link.model.receiverBaud; };
    }

    /// Whether the host sent an identity query other than the u-blox one: those are all text.
    bool otherFamiliesAsked() const { return std::ranges::any_of(link.detection->writes, textCommand); }

    GPSTestClock clock{GPSTestClock::START_US};
    ModelReceiver<UBXReceiverModel> link;
};

/// What a driver's observer saw of its detection.
struct DetectionTrace
{
    GPSDriverObserver observer(GPSTestClock& clock)
    {
        GPSDriverObserver result;
        result.detecting = [this, &clock](GPSType family, unsigned baud, bool remembered) {
            // "any" is the scan as it listens at a rate, before it asks each family there.
            const QString name = family == GPSType::Automatic ? QStringLiteral("any") : gpsReceiverName(family);
            const QString step = QStringLiteral("%1@%2").arg(name).arg(baud);
            if (remembered || family == GPSType::Automatic) {
                looked.append(step);
            } else {
                asked.append(step);
            }
            if (family == GPSType::Automatic && !scanStartedAtUs) {
                scanStartedAtUs = clock.nowUs();
            }
        };
        result.detected = [this, &clock](const GPSReceiverDetection& detection, unsigned) {
            found = detection.found()
                        ? QStringLiteral("%1@%2").arg(gpsReceiverName(detection.family->type)).arg(detection.baud)
                        : QString();
            evidence = detection.evidence;
            detectedAtUs = clock.nowUs();
        };
        return result;
    }

    /// The family and rate of each step, as "u-blox@115200": the family of the hint, and the rates of the scan.
    QStringList looked;
    /// The families the scan asked, each with its rate.
    QStringList asked;
    QString found;
    QString evidence;
    uint64_t scanStartedAtUs = 0;
    uint64_t detectedAtUs = 0;
};

uint64_t microseconds(std::chrono::milliseconds duration)
{
    return static_cast<uint64_t>(std::chrono::microseconds(duration).count());
}

}  // namespace

void GPSDriverTest::_detectionHint_data()
{
    QTest::addColumn<unsigned>("receiverBaud");
    QTest::addColumn<GPSDetectionHint>("hint");
    QTest::addColumn<QStringList>("looked");
    QTest::addColumn<GPSDetectionHint>("next");
    const GPSDetectionHint ublox{.family = GPSType::UBlox, .baud = 115200, .foundBaud = 38400};
    const QStringList scanTo38400{QStringLiteral("any@115200"), QStringLiteral("any@38400")};
    QTest::newRow("no-hint") << 38400U << GPSDetectionHint{} << scanTo38400 << ublox;
    // A receiver that kept running answers where the last session left it, which says nothing new about where it
    // answers after a restart.
    QTest::newRow("kept-running") << 115200U << ublox << QStringList{QStringLiteral("u-blox@115200")} << ublox;
    QTest::newRow("restarted") << 38400U << ublox
                               << QStringList{QStringLiteral("u-blox@115200"), QStringLiteral("u-blox@38400")} << ublox;
    // A receiver that answers at neither rate is found by the scan, as without a hint.
    QTest::newRow("at-another-rate") << 9600U << ublox
                                     << QStringList{QStringLiteral("u-blox@115200"), QStringLiteral("u-blox@38400"),
                                                    QStringLiteral("any@115200"),    QStringLiteral("any@38400"),
                                                    QStringLiteral("any@460800"),    QStringLiteral("any@9600")}
                                     << GPSDetectionHint{.family = GPSType::UBlox, .baud = 115200, .foundBaud = 9600};
    QTest::newRow("another-family") << 38400U
                                    << GPSDetectionHint{.family = GPSType::Septentrio,
                                                        .baud = 115200,
                                                        .foundBaud = 115200}
                                    << (QStringList{QStringLiteral("Septentrio@115200")} + scanTo38400) << ublox;
    // Detection probes no passive receiver, so there is none to ask.
    QTest::newRow("not-a-family") << 38400U
                                  << GPSDetectionHint{.family = GPSType::Passive, .baud = 38400, .foundBaud = 38400}
                                  << scanTo38400 << ublox;
}

void GPSDriverTest::_detectionHint()
{
    QFETCH(unsigned, receiverBaud);
    QFETCH(GPSDetectionHint, hint);
    QFETCH(QStringList, looked);
    QFETCH(GPSDetectionHint, next);
    DetectableUBlox receiver(receiverBaud);
    DetectionTrace trace;
    QList<GPSType> reported;
    GPSDriverSinks sinks;
    sinks.onReceiverDetected = [&](GPSType type) { reported.append(type); };
    GPSDriver driver(GPSType::Automatic, receiver.link, {.base = surveyInBase(2, 180s)}, sinks, receiver.clock.source(),
                     trace.observer(receiver.clock));
    driver.setDetectionHint(hint);
    QVERIFY(driver.nextDetectionHint() == GPSDetectionHint{});
    QVERIFY2(driver.configure(), qPrintable(driver.configurationError()));
    QCOMPARE(trace.looked, looked);
    QCOMPARE(trace.found, QStringLiteral("u-blox@%1").arg(receiverBaud));
    QCOMPARE(trace.evidence, QStringLiteral("identity query"));
    QCOMPARE(reported, QList<GPSType>{GPSType::UBlox});
    // The hint for the next session: the family, the rate its configuration left the link at, and where it was found.
    QVERIFY2(driver.nextDetectionHint() == next, qPrintable(QStringLiteral("baud %1, found at %2")
                                                                .arg(driver.nextDetectionHint().baud)
                                                                .arg(driver.nextDetectionHint().foundBaud)));
    // A hint that holds asks no other family and waits for no output. At its first rate the receiver is found before
    // the scan would have finished listening there, and at its second one query's wait later.
    const bool scanned = trace.scanStartedAtUs != 0;
    QCOMPARE(receiver.otherFamiliesAsked(), scanned);
    // A scan tells each family it asks, the first of the family table at its first rate, up to the one that answers.
    if (scanned) {
        QVERIFY(!trace.asked.isEmpty());
        QCOMPARE(trace.asked.constFirst(), QStringLiteral("u-blox@115200"));
        QVERIFY(trace.asked.contains(QStringLiteral("Quectel@115200")));
        QCOMPARE(trace.asked.constLast(), QStringLiteral("u-blox@%1").arg(receiverBaud));
    } else {
        QCOMPARE(trace.asked, QStringList());
    }
    if (!scanned) {
        const uint64_t elapsedUs = trace.detectedAtUs - GPSTestClock::START_US;
        QVERIFY(GPSDriver::HINT_PROBE_TIMEOUT <= GPSReceiverDetector::LISTEN_WINDOW);
        QVERIFY2(elapsedUs < looked.size() * microseconds(GPSDriver::HINT_PROBE_TIMEOUT),
                 qPrintable(QStringLiteral("%1 ms").arg(elapsedUs / 1000)));
    }
}

void GPSDriverTest::_staleDetectionHintCost_data()
{
    QTest::addColumn<GPSType>("family");
    for (const GPSReceiverDescriptor& family : gpsReceiverDescriptors()) {
        if (family.capabilities.rtkBase && family.type != GPSType::UBlox) {
            QTest::newRow(family.name.data()) << family.type;
        }
    }
}

void GPSDriverTest::_staleDetectionHintCost()
{
    QFETCH(GPSType, family);
    // Another receiver is on the link than the one the last session found there.
    DetectableUBlox receiver(38400);
    DetectionTrace trace;
    GPSDriver driver(GPSType::Automatic, receiver.link, {.base = surveyInBase(2, 180s)}, {}, receiver.clock.source(),
                     trace.observer(receiver.clock));
    driver.setDetectionHint({.family = family, .baud = 115200, .foundBaud = 230400});
    QVERIFY2(driver.configure(), qPrintable(driver.configurationError()));
    QCOMPARE(trace.found, QStringLiteral("u-blox@38400"));
    QCOMPARE(trace.looked.size(), 4);
    // Each of the hint's two rates costs one identity query that is not answered, before the scan starts. That is
    // bounded for every family, also for one that waits longer for its answer when it is scanned for.
    const auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds(trace.scanStartedAtUs - GPSTestClock::START_US));
    QVERIFY2(cost <= 2 * GPSDriver::HINT_PROBE_TIMEOUT, qPrintable(QStringLiteral("%1 ms").arg(cost.count())));
}

void GPSDriverTest::_detectionHintOnFixedRateLink()
{
    // A network bridge runs at one rate, whatever rates the hint names. The link shows its family in its output, which
    // the scan would listen for: the hinted family is asked instead.
    AutomaticLink link(GPSType::UBlox);
    DetectionTrace trace;
    GPSDriver driver(GPSType::Automatic, link.transport, {.base = surveyInBase(2, 180s)}, {}, link.clock.source(),
                     trace.observer(link.clock));
    driver.setDetectionHint({.family = GPSType::UBlox, .baud = 9600, .foundBaud = 38400});
    QVERIFY2(driver.configure(), qPrintable(driver.configurationError()));
    QCOMPARE(trace.looked, QStringList{QStringLiteral("u-blox@115200")});
    QCOMPARE(trace.found, QStringLiteral("u-blox@115200"));
    QCOMPARE(trace.evidence, QStringLiteral("identity query"));
    QVERIFY(driver.nextDetectionHint() ==
            (GPSDetectionHint{.family = GPSType::UBlox, .baud = 115200, .foundBaud = 115200}));
}

void GPSDriverTest::_mismatchHint_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<GPSType>("receiver");
    QTest::addColumn<QString>("error");

    QTest::newRow("trimble-on-ublox") << GPSType::Trimble << GPSType::UBlox
                                      << QStringLiteral(
                                             "No Trimble receiver answered at 115200 baud. UBX frames were received; "
                                             "this looks like a u-blox receiver");
    QTest::newRow("femto-on-septentrio")
        << GPSType::Femto << GPSType::Septentrio
        << QStringLiteral(
               "No Femtomes receiver answered at 115200 baud. Septentrio command replies were received; this looks "
               "like a Septentrio receiver");
    QTest::newRow("ublox-on-ublox") << GPSType::UBlox << GPSType::UBlox << QString();
}

void GPSDriverTest::_mismatchHint()
{
    QFETCH(GPSType, type);
    QFETCH(GPSType, receiver);
    QFETCH(QString, error);
    AutomaticLink link(receiver);
    link.septentrio.model.streaming = false;
    const GPSReceiverConfig config{.base = fixedBaseAt(47.123, 8.456, 500)};
    GPSDriver driver(type, link.transport, config, {}, link.clock.source());
    if (!error.isEmpty()) {
        expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    }
    QCOMPARE(driver.configure(), error.isEmpty());
    if (!error.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(driver.configurationError(), error);
    QVERIFY(!driver.configurationError().contains(QStringLiteral("47.123")));
    QVERIFY(!driver.configurationNeedsConsent());
}

void GPSDriverTest::_failedConfigurationKeepsIdentity()
{
    GPSTestClock clock(GPSTestClock::START_US);
    // A receiver that identifies itself and cannot be a base.
    ModelReceiver<UBXReceiverModel> transport(clock, UBXReceiverModel::Receiver::M8N);
    GPSDriver driver(GPSType::UBlox, transport, {.base = surveyInBase(2, 180s)}, {}, clock.source());
    QVERIFY(driver.receiverIdentity().isEmpty());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.receiverIdentity(), QStringLiteral("NEO-M8N SPG 3.01"));
    // The identity is the attempt's: a receiver that no longer answers has none.
    transport.faults.rules.push_back(silence({}));
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(driver.receiverIdentity().isEmpty());
}

void GPSDriverTest::_satelliteExpiry_data()
{
    QTest::addColumn<bool>("positionTraffic");
    QTest::newRow("idle") << false;
    QTest::newRow("position-only-traffic") << true;
}

void GPSDriverTest::_satelliteExpiry()
{
    QFETCH(bool, positionTraffic);
    const QByteArray position = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    const QByteArray nextPosition = "$GPGGA,123520,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*4D\r\n";
    const QByteArray satellites = "$GPGSV,1,1,01,01,10,20,30*79\r\n";
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    GPSSatelliteReport latest;
    int unavailableReports = 0;
    bool viewCoverageSeen = false;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const GPSSatelliteReport& report) {
        latest = report;
        if (report.inView) {
            viewCoverageSeen = true;
        } else if (viewCoverageSeen) {
            ++unavailableReports;
        }
    };
    GPSDriver driver(GPSType::Passive, link, {.baudRate = 115200}, std::move(sinks), clock.source());
    QVERIFY(driver.configure());
    const uint64_t receivedAtUs = clock.nowUs();
    send(link, satellites + position);
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Data);
    QCOMPARE(latest.inView, std::optional<int>{1});
    QVERIFY(latest.timestampUs != 0);
    QCOMPARE(latest.constellationsInView().size(), size_t(1));
    QCOMPARE(unavailableReports, 0);

    // The view lasts the shared freshness window, whether or not positions keep arriving.
    const uint64_t expiresAtUs =
        receivedAtUs +
        static_cast<uint64_t>(std::chrono::microseconds(GPSDecodedData::SatelliteCounts::FRESHNESS_TIMEOUT).count());
    const GPSReceiveLiveness traffic = positionTraffic ? GPSReceiveLiveness::Data : GPSReceiveLiveness::Idle;
    clock.advanceTo(expiresAtUs - 1);
    send(link, positionTraffic ? position : QByteArray{});
    QCOMPARE(driver.receiveOutcome(0ms).liveness, traffic);
    QCOMPARE(unavailableReports, 0);
    clock.advanceTo(expiresAtUs);
    send(link, positionTraffic ? position : QByteArray{});
    const auto result = driver.receiveOutcome(0ms);
    QCOMPARE(unavailableReports, 1);
    QCOMPARE(latest.timestampUs, uint64_t{0});
    QVERIFY(!latest.inView);
    QVERIFY(latest.constellationsInView().empty());
    // The count the positions carry lapses with them.
    QCOMPARE(latest.used, positionTraffic ? std::optional<int>{8} : std::nullopt);
    QCOMPARE(result.liveness, traffic);
    if (!positionTraffic) {
        QCOMPARE(result.updates, GPSReceiveUpdates{});
    }
    std::optional<GPSReadResult> readFailure;
    link.setReadHandler([&readFailure](uint8_t*, int, std::chrono::milliseconds) { return readFailure; });
    link.clearReplies();
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Idle);
    QCOMPARE(unavailableReports, 1);

    send(link, satellites + nextPosition);
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Data);
    QCOMPARE(latest.inView, std::optional<int>{1});
    QVERIFY(latest.timestampUs != 0);
    QCOMPARE(link.writes(), 0);
}

void GPSDriverTest::_satelliteUsageSources()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    std::vector<std::optional<int>> used;
    GPSSatelliteReport latest;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const GPSSatelliteReport& report) {
        used.push_back(report.used);
        latest = report;
    };
    GPSDriver driver(GPSType::Passive, link, {.baudRate = 115200}, std::move(sinks), clock.source());
    QVERIFY(driver.configure());
    // Each epoch's GGA counts twelve satellites used, while its GSA lists three.
    for (int epoch = 0; epoch < 4; ++epoch) {
        const QByteArray time = QByteArray::number(123519 + epoch);
        for (const QByteArray& sentence :
             {"GPGGA," + time + ",4807.038,N,01131.000,E,1,12,0.9,545.4,M,46.9,M,,",
              QByteArray("GPGSA,A,3,01,02,03,,,,,,,,,,1.0,0.8,0.6"),
              QByteArray("GPGSV,1,1,04,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60")}) {
            send(link, nmeaBytes(sentence));
            QCOMPARE(driver.receiveOutcome(20ms).error, GPSProtocolError::None);
        }
        clock.advanceBy(1000000);
    }
    // The GGA count stands in until the first satellite epoch closes; the published count then stays on the GSA sum.
    QCOMPARE(used, (std::vector<std::optional<int>>{12, 3, 3, 3}));
    // The constellation's own counts are the GSV and GSA ones, never the GGA count.
    QCOMPARE(latest.constellationsInView().size(), size_t(1));
    QCOMPARE(latest.constellationsInView().front(), (GPSSatelliteReport::Constellation{GPSConstellation::GPS, 4, 3}));
}

void GPSDriverTest::_inputProtocolSwitch()
{
    GPSTestClock clock(GPSTestClock::START_US);
    ModelReceiver<ScriptedReceiver::Model> link(clock);
    std::vector<GPSSatelliteReport> satellites;
    std::vector<GPSType> protocols;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const GPSSatelliteReport& report) { satellites.push_back(report); };
    sinks.onReceiverDetected = [&](GPSType protocol) { protocols.push_back(protocol); };
    GPSDriver driver(GPSType::Passive, link, {.baudRate = 115200}, std::move(sinks), clock.source());
    QVERIFY(driver.configure());
    send(link, nmeaBytes("GPGSV,1,1,01,01,10,20,30") +
                   nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Data);
    QCOMPARE(protocols, std::vector<GPSType>{GPSType::Passive});
    // Identifying the first protocol withdraws nothing: only the NMEA counts were published.
    QCOMPARE(satellites.size(), size_t(2));
    QCOMPARE(satellites.back().inView, std::optional<int>{1});
    QCOMPARE(satellites.back().used, std::optional<int>{8});

    // NMEA falls silent and UBX positions, which carry no satellite counts, take over.
    clock.advanceBy(3000000);
    const auto epoch = ubxNavigationEpoch(ubxFix3D(), 1000);
    send(link, QByteArray(reinterpret_cast<const char*>(epoch.data()), static_cast<qsizetype>(epoch.size())));
    QCOMPARE(driver.receiveOutcome(0ms).liveness, GPSReceiveLiveness::Data);
    QCOMPARE(protocols.back(), GPSType::UBlox);
    QCOMPARE(satellites.size(), size_t(3));
    QCOMPARE(satellites.back(), GPSSatelliteReport{});
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSDriverTest, TestLabel::Unit)
