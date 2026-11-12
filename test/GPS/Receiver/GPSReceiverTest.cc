#include "GPSReceiverTest.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCancellation.h"
#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverConnectionPolicy.h"
#include "GPSReceiverFactGroup.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSReceiverSystemSerialPorts.h"
#endif
#include "GPSReceiverWorker.h"
#include "GPSSettingsBindings.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NMEASentence.h"
#include "PositionManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "QGCGeoid.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "Transport/Support/GPSRecordingReader.h"
#include "Transport/Support/UnusedUdpPort.h"
#include "UnitsSettings.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// Holds a receiver worker inside its transport factory until released. Destruction releases it, so a failed test
/// never leaves a worker blocked.
class BlockedTransportGate
{
public:
    BlockedTransportGate() = default;

    ~BlockedTransportGate() { release(); }

    BlockedTransportGate(const BlockedTransportGate&) = delete;
    BlockedTransportGate& operator=(const BlockedTransportGate&) = delete;

    /// Blocks until released, then reports whether the worker was cancelled meanwhile and opens no transport.
    GPSReceiverWorker::TransportFactory factory() const
    {
        return [state = _state](GPSCancelToken cancelToken) {
            state->entered.release();
            state->release.acquire();
            state->sawCancellation = cancelToken.isCancelled();
            return std::unique_ptr<GPSTransport>{};
        };
    }

    /// True once a worker is blocked in the factory.
    bool entered() const { return _state->entered.available() > 0; }

    /// Waits, without running the event loop, until a worker enters the factory.
    bool waitEntered(int timeoutMs) { return _state->entered.tryAcquire(1, timeoutMs); }

    void release() { _state->release.release(); }

    bool sawCancellation() const { return _state->sawCancellation; }

private:
    struct State
    {
        QSemaphore entered;
        QSemaphore release;
        std::atomic_bool sawCancellation = false;
    };

    std::shared_ptr<State> _state = std::make_shared<State>();
};
}  // namespace

void GPSReceiverTest::_logsFixTransitionsWithoutCoordinates()
{
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    QVERIFY(connectOverTcp(receiver));
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    const QString category = QStringLiteral("GPS.Receiver.GPSReceiver");
    const TestFixtures::LoggingCategoryFixture logging(category);
    const auto initialCount = GPSTest::debugMessages(category).size();
    expectLogMessage("GPS.Receiver.GPSReceiver", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Receiver fix changed:")));
    worker->position(fixReport(GPSFixQuality::Fix3D));
    verifyExpectedLogMessage();
    QCOMPARE(GPSTest::debugMessages(category).size(), initialCount + 1);
    expectLogMessage("GPS.Receiver.GPSReceiver", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Receiver fix changed: .*NoFix$")));
    worker->position(fixReport(GPSFixQuality::NoFix));
    verifyExpectedLogMessage();
    QCOMPARE(GPSTest::debugMessages(category).size(), initialCount + 2);
    receiver.disconnectReceiver();
    worker->position(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(GPSTest::debugMessages(category).size(), initialCount + 2);
    for (const QString& message : GPSTest::debugMessages(category)) {
        QVERIFY2(QRegularExpression(QStringLiteral("^Receiver fix changed: [\\w:]+$")).match(message).hasMatch(),
                 qPrintable(message));
    }
}

UT_REGISTER_TEST(GPSReceiverTest, TestLabel::Unit)

void GPSReceiverTest::_outputOverflowWarning_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<QString>("warning");
    const int ublox = gpsReceiverManufacturerForType(GPSType::UBlox);
    QTest::newRow("base") << ublox << false
                          << QStringLiteral(
                                 "The receiver produces more output than the connection carries, so corrections are "
                                 "being lost. Use a higher baud rate or turn on Compact RTCM corrections (MSM4).");
    QTest::newRow("compact-base") << ublox << true
                                  << QStringLiteral(
                                         "The receiver produces more output than the connection carries, so "
                                         "corrections are being lost. Use a higher baud rate.");
    QTest::newRow("passive") << kPassiveManufacturer << false
                             << QStringLiteral(
                                    "The receiver produces more output than the connection carries, so data is being "
                                    "lost. Use a higher baud rate or reduce the receiver's output.");
}

void GPSReceiverTest::_outputOverflowWarning()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, compact);
    QFETCH(QString, warning);
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler);
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration(manufacturer);
    configuration.base.compactObservations = compact;
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    worker->ready();
    QSignalSpy errors(&receiver, &GPSReceiver::errorMessageChanged);
    auto report = fixReport(GPSFixQuality::Fix3D);
    worker->position(report);
    QVERIFY(receiver.errorMessage().isEmpty());
    report.integrity.outputOverflowUs = 100;
    worker->position(report);
    QCOMPARE(receiver.errorMessage(), warning);
    QCOMPARE(errors.size(), 1);

    // Every position repeats the latest overflow; only a new one restarts the warning's time.
    QVERIFY(scheduler.advanceBy(GPSReceiver::OUTPUT_OVERFLOW_WARNING_DURATION - 1s));
    worker->position(report);
    QCOMPARE(receiver.errorMessage(), warning);
    QVERIFY(scheduler.advanceBy(1s));
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(errors.size(), 2);
    report.integrity.outputOverflowUs = 200;
    worker->position(report);
    QVERIFY(scheduler.advanceBy(20s));
    report.integrity.outputOverflowUs = 300;
    worker->position(report);
    QVERIFY(scheduler.advanceBy(20s));
    QCOMPARE(receiver.errorMessage(), warning);
    QCOMPARE(errors.size(), 3);

    // A connection or input problem takes precedence, and the warning returns once it clears.
    worker->input(GPSInputProblem::NotGNSS);
    QCOMPARE(receiver.errorMessage(),
             QStringLiteral("Receiving data that isn't GNSS output. Check the device and baud rate."));
    worker->input(GPSInputProblem::None);
    QCOMPARE(receiver.errorMessage(), warning);

    // A new session starts without it, and the old session's expiry does not touch it.
    receiver.disconnectReceiver();
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(receiver.connectReceiver());
    worker = harness.workers.current();
    QVERIFY(worker);
    worker->ready();
    errors.clear();
    report.integrity.outputOverflowUs = 0;
    worker->position(report);
    QVERIFY(scheduler.advanceBy(GPSReceiver::OUTPUT_OVERFLOW_WARNING_DURATION));
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(errors.isEmpty());
    report.integrity.outputOverflowUs = 300;
    worker->position(report);
    QCOMPARE(receiver.errorMessage(), warning);
}

void GPSReceiverTest::_automaticConnection()
{
    RTKSettings settings;
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    GPSSettingsBindings::bindReceiver(&settings, &receiver);
    settings.receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    settings.baseReceiverManufacturer()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.baseMode()->setRawValue(static_cast<int>(BaseModeDefinition::Mode::BaseReceiverAveraging));

    // Consent and receiver-managed averaging are checked against the detected family, on the worker.
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    QCOMPARE(worker->type(), GPSType::Automatic);
    QVERIFY(worker->capturedConfig().allowPersistentChanges);
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(worker->capturedConfig().base.mode));
    QCOMPARE(receiver._activeManufacturer(), GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY(receiver.activePresentation().automatic);
    QVERIFY(receiver.detectedReceiver().isEmpty());

    QSignalSpy changed(&receiver, &GPSReceiver::receiverChanged);
    worker->detected(GPSType::Unicore);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Unicore"));
    QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::Unicore));
    QVERIFY(receiver.activePresentation().receiverAveraging);
    worker->ready(QStringLiteral("UM982 R4.10Build15434"));
    QVERIFY(receiver.facts()->telemetryAvailable());
    QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Unicore"));
    // Automatic stays selected whatever it detects.
    QCOMPARE(settings.baseReceiverManufacturer()->rawValue().toInt(), GPS_AUTOMATIC_MANUFACTURER);

    receiver.disconnectReceiver();
    QVERIFY(receiver.detectedReceiver().isEmpty());
    // A specific family ignores detections and does not replace the saved manufacturer either.
    const int ublox = gpsReceiverManufacturerForType(GPSType::UBlox);
    settings.baseReceiverManufacturer()->setRawValue(ublox);
    settings.baseMode()->setRawValue(static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    QVERIFY(receiver.connectReceiver());
    harness.workers.current()->detected(GPSType::Septentrio);
    QVERIFY(receiver.detectedReceiver().isEmpty());
    QCOMPARE(receiver._activeManufacturer(), ublox);
    QCOMPARE(settings.baseReceiverManufacturer()->rawValue().toInt(), ublox);
    receiver.disconnectReceiver();
}

void GPSReceiverTest::_detectedReceiverIsRememberedPerLink()
{
    RTKSettings settings;
    ScriptedGPSReceiver harness(
        nullptr,
        {.recordingDirectory = [] { return QStringLiteral("/test/recordings"); },
         .detectedReceiver = [&settings](const QString& link) { return settings.detectedReceiver(link); },
         .rememberDetectedReceiver =
             [&settings](const RTKDetectedReceiver& detected) { settings.rememberDetectedReceiver(detected); }});
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(GPS_AUTOMATIC_MANUFACTURER);
    useTcp(configuration);
    configuration.autoConnect = false;
    const QString link = QStringLiteral("tcp:rtk.test:2101");
    const GPSDetectionHint found{.family = GPSType::UBlox, .baud = 115200, .foundBaud = 38400};
    const auto hint = [&harness] { return harness.workers.current()->detectionHint(); };
    const auto connects = [&](bool ready) {
        receiver.disconnectReceiver();
        receiver.setConfiguration(configuration);
        if (!receiver.connectReceiver()) {
            return false;
        }
        if (ready) {
            harness.workers.current()->detected(found.family);
            harness.workers.current()->ready(QStringLiteral("ZED-F9P HPG 1.32"), found);
        }
        return true;
    };

    // Nothing is known of the link, so the session scans. What it found is remembered once its receiver is ready.
    QVERIFY(connects(false));
    QCOMPARE(hint(), GPSDetectionHint{});
    harness.workers.current()->detected(found.family);
    QVERIFY(!settings.detectedReceiver(link));
    harness.workers.current()->ready(QStringLiteral("ZED-F9P HPG 1.32"), found);
    const RTKDetectedReceiver remembered{.link = link,
                                         .manufacturer = gpsReceiverManufacturerForType(GPSType::UBlox),
                                         .baudRate = 115200,
                                         .foundBaudRate = 38400};
    QCOMPARE(settings.detectedReceiver(link), std::optional(remembered));

    // The next automatic session on that link looks for the receiver first; one on another link scans.
    QVERIFY(connects(false));
    QCOMPARE(hint(), found);
    ++configuration.tcpPort;
    QVERIFY(connects(false));
    QCOMPARE(hint(), GPSDetectionHint{});
    --configuration.tcpPort;

    // A selected family needs no hint, and what its session reports is not a detection.
    configuration.baseReceiverManufacturer = gpsReceiverManufacturerForType(GPSType::Septentrio);
    QVERIFY(connects(true));
    QCOMPARE(hint(), GPSDetectionHint{});
    QCOMPARE(settings.detectedReceiver(link), std::optional(remembered));
    configuration.baseReceiverManufacturer = GPS_AUTOMATIC_MANUFACTURER;

    // A recorded session detects from scratch, so that its recording replays on its own.
    receiver.setRecordSessions(true);
    QVERIFY(connects(false));
    QVERIFY(!receiver.recordingFile().isEmpty());
    QCOMPARE(hint(), GPSDetectionHint{});
    receiver.setRecordSessions(false);

    // A family that is no longer supported is no hint.
    settings.rememberDetectedReceiver({.link = link, .manufacturer = 99, .baudRate = 115200, .foundBaudRate = 115200});
    QVERIFY(connects(false));
    QCOMPARE(hint(), GPSDetectionHint{});
}

void GPSReceiverTest::_statusTextNamesWhatDetectionTries_data()
{
    QTest::addColumn<bool>("serial");
    // A network link has no rate to name.
    QTest::addColumn<QString>("remembered");
    QTest::addColumn<QString>("listening");
    QTest::addColumn<QString>("asking");
    QTest::newRow("tcp") << false << GPSReceiver::tr("Identifying receiver: trying %1, as last time...").arg("u-blox")
                         << GPSReceiver::tr("Identifying receiver...")
                         << GPSReceiver::tr("Identifying receiver: trying %1...").arg("Trimble");
#ifndef QGC_NO_SERIAL_LINK
    QTest::newRow("serial")
        << true
        << GPSReceiver::tr("Identifying receiver: trying %1 at %2 baud, as last time...").arg("u-blox").arg(115200)
        << GPSReceiver::tr("Identifying receiver at %1 baud...").arg(38400)
        << GPSReceiver::tr("Identifying receiver: trying %1 at %2 baud...").arg("Trimble").arg(38400);
#endif
}

void GPSReceiverTest::_statusTextNamesWhatDetectionTries()
{
    QFETCH(bool, serial);
    QFETCH(QString, remembered);
    QFETCH(QString, listening);
    QFETCH(QString, asking);
    auto configuration = receiverConfiguration(GPS_AUTOMATIC_MANUFACTURER);
    configuration.autoConnect = false;
#ifdef QGC_NO_SERIAL_LINK
    ScriptedGPSReceiver harness;
#else
    TestSerialPorts ports({rtkPort()});
    ScriptedGPSReceiver harness(nullptr, {.serial = ports.side()});
    if (serial) {
        configuration.connectionType = RTKSettings::Serial;
        configuration.serialDevice = QStringLiteral("/test/rtk");
        configuration.serialBaudRate = 0;
    }
#endif
    if (!serial) {
        useTcp(configuration);
    }
    GPSReceiver& receiver = harness.receiver;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    ScriptedReceiverWorker* const worker = harness.workers.current();
    QSignalSpy texts(&receiver, &GPSReceiver::statusTextChanged);
    QCOMPARE(receiver.statusText(), GPSReceiver::tr("Identifying receiver..."));

    // First the receiver the link's last session found. Then the scan, one rate after the other: at each it listens
    // for any receiver and then asks one family after the other, also the one that did not answer as last time.
    worker->looksFor(GPSType::UBlox, 115200, true);
    QCOMPARE(receiver.statusText(), remembered);
    worker->looksFor(GPSType::Automatic, 38400);
    QCOMPARE(receiver.statusText(), listening);
    worker->looksFor(GPSType::Trimble, 38400);
    QCOMPARE(receiver.statusText(), asking);
    worker->detected(GPSType::UBlox);
    QCOMPARE(receiver.statusText(), GPSReceiver::tr("Connecting to receiver..."));
    QCOMPARE(texts.size(), 4);
    worker->ready();
    receiver.disconnectReceiver();

    // A selected family is not detected, so there is nothing to name.
    configuration.baseReceiverManufacturer = gpsReceiverManufacturerForType(GPSType::UBlox);
    receiver.setConfiguration(configuration);
    harness.workers.finishRetired();
    QVERIFY(receiver.connectReceiver());
    harness.workers.current()->looksFor(GPSType::UBlox, 115200, true);
    QCOMPARE(receiver.statusText(), GPSReceiver::tr("Connecting to receiver..."));
}

void GPSReceiverTest::_statusTextFollowsState_data()
{
    using State = GPSReceiver::State;
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("forward");
    // How far the connection gets: "opened", "ready", or "lost" for a ready receiver whose connection fails.
    QTest::addColumn<QString>("reached");
    QTest::addColumn<QList<GPSReceiver::State>>("states");
    QTest::addColumn<QString>("text");
    const int ublox = gpsReceiverManufacturerForType(GPSType::UBlox);
    QTest::newRow("never-connected") << ublox << true << QString() << QList<State>{} << QString();
    QTest::newRow("connecting") << ublox << true << QStringLiteral("opened") << QList{State::Connecting}
                                << GPSReceiver::tr("Connecting to receiver...");
    QTest::newRow("identifying") << GPS_AUTOMATIC_MANUFACTURER << true << QStringLiteral("opened")
                                 << QList{State::Connecting} << GPSReceiver::tr("Identifying receiver...");
    // A passive receiver's sentences do not depend on a base station's survey.
    QTest::newRow("forwarding") << kPassiveManufacturer << true << QStringLiteral("ready")
                                << QList{State::Connecting, State::Connected}
                                << GPSReceiver::tr("Existing base station connected; forwarding its RTCM");
    QTest::newRow("position-only") << kPassiveManufacturer << false << QStringLiteral("ready")
                                   << QList{State::Connecting, State::Connected}
                                   << GPSReceiver::tr("Receiver connected; position only");
    QTest::newRow("lost") << kPassiveManufacturer << true << QStringLiteral("lost")
                          << QList{State::Connecting, State::Connected, State::Reconnecting}
                          << GPSReceiver::tr("Connecting automatically...");
}

void GPSReceiverTest::_statusTextFollowsState()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, forward);
    QFETCH(QString, reached);
    QFETCH(QList<GPSReceiver::State>, states);
    QFETCH(QString, text);
    const bool lost = reached == QStringLiteral("lost");
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(manufacturer);
    configuration.forwardReceiverRtcm = forward;
    useTcp(configuration);
    // Only a receiver that connects automatically reconnects after its connection was lost.
    configuration.autoConnect = lost;
    receiver.setConfiguration(configuration);
    // The settings and status panels follow the state, so each one has to be notified, as it is reached.
    QList<GPSReceiver::State> notified;
    QObject observer;
    connect(&receiver, &GPSReceiver::receiverChanged, &observer, [&] {
        if (notified.isEmpty() || notified.constLast() != receiver.state()) {
            notified.append(receiver.state());
        }
    });

    if (!reached.isEmpty()) {
        QVERIFY(receiver.connectReceiver());
    }
    if (reached == QStringLiteral("ready") || lost) {
        harness.workers.current()->ready();
    }
    if (lost) {
        expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
        harness.workers.current()->fail(GPSConnectionError::DeviceError);
        verifyExpectedLogMessage();
    }
    QCOMPARE(notified, states);
    QCOMPARE(receiver.state(), states.isEmpty() ? GPSReceiver::State::Disconnected : states.constLast());
    QCOMPARE(receiver.statusText(), text);

    // Without a receiver, and none about to be connected, there is nothing to say.
    receiver.disconnectReceiver();
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(receiver.statusText().isEmpty());
}

void GPSReceiverTest::_statusTextSaysWhenNoCorrectionsAreSent_data()
{
    using Mode = BaseModeDefinition::Mode;
    using States = QList<GPSBaseState>;
    QTest::addColumn<GPSType>("family");
    // An Automatic connection detects the family instead of being told it.
    QTest::addColumn<bool>("automatic");
    QTest::addColumn<BaseModeDefinition::Mode>("mode");
    // What the receiver reports of its base position once it is ready, in order.
    QTest::addColumn<QList<GPSBaseState>>("states");
    QTest::addColumn<QString>("text");
    const QString waiting =
        GPSReceiver::tr("Waiting for a valid base position; the base sends no corrections until then");
    const QString surveying = GPSReceiver::tr("Survey-In active; the base sends no corrections until it completes");
    const QString connected = GPSReceiver::tr("Receiver connected");
    // A base's corrections are forwarded only while it reports its position valid.
    QTest::newRow("survey-not-started") << GPSType::UBlox << false << Mode::BaseSurveyIn << States{} << waiting;
    QTest::newRow("survey-running") << GPSType::UBlox << false << Mode::BaseSurveyIn << States{GPSBaseState::Active}
                                    << surveying;
    QTest::newRow("survey-complete") << GPSType::UBlox << false << Mode::BaseSurveyIn
                                     << States{GPSBaseState::Active, GPSBaseState::Valid} << connected;
    QTest::newRow("survey-lost") << GPSType::UBlox << false << Mode::BaseSurveyIn
                                 << States{GPSBaseState::Valid, GPSBaseState::Inactive} << waiting;
    QTest::newRow("fixed-unconfirmed") << GPSType::UBlox << false << Mode::BaseFixed << States{} << waiting;
    QTest::newRow("fixed") << GPSType::UBlox << false << Mode::BaseFixed << States{GPSBaseState::Valid}
                           << GPSReceiver::tr("Fixed position");
    QTest::newRow("averaging-running")
        << GPSType::Unicore << false << Mode::BaseReceiverAveraging << States{GPSBaseState::Active}
        << GPSReceiver::tr("Receiver-managed averaging active; the base sends no corrections until it completes");
    QTest::newRow("averaging-complete") << GPSType::Unicore << false << Mode::BaseReceiverAveraging
                                        << States{GPSBaseState::Active, GPSBaseState::Valid}
                                        << GPSReceiver::tr("Receiver-managed averaging — no accuracy guarantee");
    QTest::newRow("detected-family") << GPSType::UBlox << true << Mode::BaseSurveyIn << States{GPSBaseState::Active}
                                     << surveying;
    // A Septentrio base's corrections do not wait for its base position, so its status claims nothing about them.
    QTest::newRow("septentrio-survey-running") << GPSType::Septentrio << false << Mode::BaseSurveyIn
                                               << States{GPSBaseState::Active} << GPSReceiver::tr("Survey-In active");
    QTest::newRow("septentrio-survey-lost") << GPSType::Septentrio << false << Mode::BaseSurveyIn
                                            << States{GPSBaseState::Valid, GPSBaseState::Inactive} << connected;
    QTest::newRow("septentrio-fixed-unconfirmed")
        << GPSType::Septentrio << false << Mode::BaseFixed << States{} << GPSReceiver::tr("Fixed position");
    QTest::newRow("septentrio-detected") << GPSType::Septentrio << true << Mode::BaseSurveyIn
                                         << States{GPSBaseState::Active} << GPSReceiver::tr("Survey-In active");
    // A passive receiver is no base of this session: what it forwards is not held back.
    QTest::newRow("passive") << GPSType::Passive << false << Mode::BaseSurveyIn << States{GPSBaseState::Inactive}
                             << GPSReceiver::tr("Existing base station connected; forwarding its RTCM");
}

void GPSReceiverTest::_statusTextSaysWhenNoCorrectionsAreSent()
{
    using Mode = BaseModeDefinition::Mode;
    QFETCH(GPSType, family);
    QFETCH(bool, automatic);
    QFETCH(BaseModeDefinition::Mode, mode);
    QFETCH(QList<GPSBaseState>, states);
    QFETCH(QString, text);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration =
        receiverConfiguration(automatic ? GPS_AUTOMATIC_MANUFACTURER : gpsReceiverManufacturerForType(family));
    configuration.forwardReceiverRtcm = true;
    if (mode == Mode::BaseFixed) {
        configuration.base.mode = GPSBaseStationConfig::Fixed{
            .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 480.0}};
    } else if (mode == Mode::BaseReceiverAveraging) {
        configuration.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
    }
    QVERIFY(connectOverTcp(receiver, configuration));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    if (automatic) {
        worker->detected(family);
    }
    // The status panel shows the sentence it was last notified of.
    QString notified;
    QObject observer;
    connect(&receiver, &GPSReceiver::statusTextChanged, &observer, [&] { notified = receiver.statusText(); });
    worker->ready();
    for (const GPSBaseState state : states) {
        worker->baseStatus({.state = state});
    }
    QCOMPARE(receiver.statusText(), text);
    QCOMPARE(notified, text);
}

void GPSReceiverTest::_notificationsFollowCompletedConnection()
{
    BlockedTransportGate gate;
    GPSReceiver receiver;
    receiver.setWorkerFactory(workerFactory(gate.factory()));
    auto configuration = receiverConfiguration();
    useTcp(configuration);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    receiver.setConnectionError(QStringLiteral("previous failure"));
    receiver._emitChanges();
    QPointer<GPSReceiverWorker> worker;
    const auto cleanup = qScopeGuard([&] {
        if (worker) {
            worker->stop();
        }
        gate.release();
        receiver.shutdown();
    });
    QStringList notifications;
    // Destroyed before the cleanup, whose disconnect is not part of the connection being observed.
    QObject observer;
    const auto observe = [&](const QString& notification) {
        // Each notification arrives once, after the connection is installed.
        QVERIFY(receiver.hasReceiver());
        QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::UBlox));
        QVERIFY(receiver.errorMessage().isEmpty());
        notifications.append(notification);
    };
    connect(&receiver, &GPSReceiver::errorMessageChanged, &observer, [&] { observe(QStringLiteral("error")); });
    connect(&receiver, &GPSReceiver::receiverChanged, &observer, [&] { observe(QStringLiteral("receiver")); });
    QVERIFY(receiver.connectReceiver());
    worker = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(worker);
    notifications.sort();
    QCOMPARE(notifications, (QStringList{QStringLiteral("error"), QStringLiteral("receiver")}));
}

void GPSReceiverTest::_receiverPublishesGcsPosition()
{
    PositionManager positions;
    ScriptedGPSReceiver harness(nullptr, {.positions = &positions});
    auto& receiver = harness.receiver;
    QVERIFY(connectOverTcp(receiver));
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    GPSPositionReport report = fixReport(GPSFixQuality::RTKFixed);
    report.navigation.latitudeDegrees = 47.25;
    report.navigation.longitudeDegrees = 8.5;
    report.navigation.altitudeMslMeters = 450;
    report.navigation.horizontalAccuracyMeters = 0.02f;
    worker->position(report);
    // Registration waits until the receiver is configured.
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::None);

    worker->ready();
    worker->position(report);
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition().latitude(), 47.25);
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), qreal(0.02f));
    QCOMPARE(gpsFixQualityFromValue(receiver.facts()->fixType()->rawValue().toInt()), GPSFixQuality::RTKFixed);

    receiver.disconnectReceiver();
    QVERIFY(!positions.gcsPosition().isValid());
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::None);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
    worker->position(report);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
}

void GPSReceiverTest::_fixedBasePositionIsGcsPosition()
{
    PositionManager positions;
    ScriptedGPSReceiver harness(nullptr, {.positions = &positions});
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    configuration.base.mode = GPSBaseStationConfig::Fixed{
        .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 480.0f},
        .accuracyMeters = 0.0f};
    QVERIFY(connectOverTcp(receiver, configuration));
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    worker->ready();
    // The configured position is not the ground station's until the receiver reports that it uses it.
    worker->position(fixReport(GPSFixQuality::NoFix));
    QVERIFY(!positions.gcsPosition().isValid());
    worker->baseStatus({.state = GPSBaseState::Valid,
                        .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 480.0f}});
    // Fixed-mode receivers report only a time fix.
    worker->position(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::Receiver);
    // The base's ellipsoid height becomes the ground station's altitude above mean sea level.
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(47.5, 8.25, 480.0 - QGCGeo::geoidSeparation(47.5, 8.25)));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 0.01);
    const auto remoteId = receiver.acceptedPositionObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QCOMPARE(remoteId->altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(remoteId->position.coordinate().altitude(), 480.0);
    const auto gga = receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga);
    QVERIFY(gga);
    QVERIFY(gga->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(gpsFixQualityFromValue(receiver.facts()->fixType()->rawValue().toInt()), GPSFixQuality::NoFix);
    receiver.disconnectReceiver();
    QVERIFY(!positions.gcsPosition().isValid());
}

void GPSReceiverTest::_fixedBaseCheck()
{
    using Check = GPSReceiver::FixedBaseCheck;
    using Result = GPSFixedBaseCheck::Result;
    TestFixtures::SettingsFixture units;
    units.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(),
                       UnitsSettings::HorizontalDistanceUnitsMeters);
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    const GPSEllipsoidPosition position{.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 480};
    configuration.base.mode = GPSBaseStationConfig::Fixed{.position = position, .accuracyMeters = 1.0f};
    QSignalSpy changed(&receiver, &GPSReceiver::receiverChanged);
    const QString waiting =
        QStringLiteral("Waiting for the receiver's own position, to check the fixed position against it");

    // A receiver that is given the position while it is configured waits for its own before that.
    QVERIFY(connectOverTcp(receiver, configuration));
    auto* worker = harness.workers.current();
    QCOMPARE(receiver.fixedBaseCheck(), Check::None);
    QCOMPARE(receiver.statusText(), QStringLiteral("Connecting to receiver..."));
    worker->baseStatus({.fixedCheck = GPSFixedBaseCheck{}});
    QCOMPARE(receiver.fixedBaseCheck(), Check::Waiting);
    QCOMPARE(receiver.statusText(), waiting + QStringLiteral("..."));
    QVERIFY(receiver.errorMessage().isEmpty());

    // The two positions are too far apart for one place: the receiver has a problem that stays, and says so once.
    changed.clear();
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("fixed base position is far")));
    worker->baseStatus({.state = GPSBaseState::Valid,
                        .position = position,
                        .fixedCheck = GPSFixedBaseCheck{.result = Result::Implausible, .distanceMeters = 12449}});
    verifyExpectedLogMessage();
    worker->ready();
    QCOMPARE(receiver.fixedBaseCheck(), Check::Implausible);
    QCOMPARE(receiver.fixedBaseCheckDistance(), 12449.);
    QVERIFY(!changed.isEmpty());
    QVERIFY2(receiver.errorMessage().startsWith(
                 QStringLiteral("The fixed base position is 12.4 km from the position the receiver found on its own.")),
             qPrintable(receiver.errorMessage()));
    // It only warns: the base is valid and its corrections go out.
    QCOMPARE(receiver.statusText(), QStringLiteral("Fixed position"));
    QVERIFY(receiver.forwardingCorrections());
    // The statuses that follow do not repeat the result, which stays with the connection.
    worker->baseStatus({.state = GPSBaseState::Valid, .position = position});
    QCOMPARE(receiver.fixedBaseCheck(), Check::Implausible);
    receiver.disconnectReceiver();
    QCOMPARE(receiver.fixedBaseCheck(), Check::None);
    QVERIFY(std::isnan(receiver.fixedBaseCheckDistance()));
    QVERIFY(receiver.errorMessage().isEmpty());

    // A receiver that is given the position once it runs waits as a connected base that sends no corrections yet.
    QVERIFY(connectOverTcp(receiver, configuration));
    worker = harness.workers.current();
    worker->ready();
    worker->baseStatus({.state = GPSBaseState::Active, .fixedCheck = GPSFixedBaseCheck{}});
    QCOMPARE(receiver.statusText(), waiting + QStringLiteral("; the base sends no corrections until then"));
    // A position within reach, and no position to compare with, are no problem.
    worker->baseStatus({.state = GPSBaseState::Valid,
                        .position = position,
                        .fixedCheck = GPSFixedBaseCheck{.result = Result::Plausible, .distanceMeters = 3.4}});
    QCOMPARE(receiver.fixedBaseCheck(), Check::Plausible);
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(receiver.statusText(), QStringLiteral("Fixed position"));
    worker->baseStatus({.state = GPSBaseState::Valid,
                        .position = position,
                        .fixedCheck = GPSFixedBaseCheck{.result = Result::Unchecked}});
    QCOMPARE(receiver.fixedBaseCheck(), Check::Unchecked);
    QVERIFY(std::isnan(receiver.fixedBaseCheckDistance()));
    QVERIFY(receiver.errorMessage().isEmpty());
}

void GPSReceiverTest::_surveyedBasePositionIsGcsPosition()
{
    PositionManager positions;
    ScriptedGPSReceiver harness(nullptr, {.positions = &positions});
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration();
    configuration.base.mode =
        GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2.0, .duration = std::chrono::seconds{180}};
    QVERIFY(connectOverTcp(receiver, configuration));
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    const auto deliver = [&](const GPSPositionReport& report) { worker->position(report); };
    GPSPositionReport navigating = fixReport(GPSFixQuality::Fix3D);
    navigating.navigation.latitudeDegrees = 10;
    navigating.navigation.longitudeDegrees = 20;
    navigating.navigation.horizontalAccuracyMeters = 3;
    worker->ready();
    deliver(navigating);
    QCOMPARE(positions.gcsPosition().latitude(), 10.0);

    GPSBaseStatus survey;
    survey.state = GPSBaseState::Valid;
    survey.position = {.latitudeDegrees = 11, .longitudeDegrees = 21, .altitudeMeters = 400};
    survey.meanAccuracyMeters = 1.5;
    worker->baseStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(11, 21, 400.0 - QGCGeo::geoidSeparation(11, 21)));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 1.5);

    survey.meanAccuracyMeters.reset();
    worker->baseStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 2.0);

    survey.state = GPSBaseState::Active;
    worker->baseStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QVERIFY(!positions.gcsPosition().isValid());
}

void GPSReceiverTest::_retiredWorkerCannotUpdateReplacement()
{
    GPSCorrectionManager corrections;
    GPSReceiver receiver(nullptr, nullptr, {.corrections = &corrections});
    BlockedTransportGate firstGate;
    BlockedTransportGate secondGate;
    int workers = 0;
    receiver.setWorkerFactory(
        [&](GPSReceiverWorker::TransportFactory, GPSType type, const GPSReceiverConfig& config, QObject* parent) {
            return new GPSReceiverWorker((workers++ == 0 ? firstGate : secondGate).factory(), type, config, parent);
        });
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    const QString instance = QStringLiteral("tcp:rtk.test:2101");
    QVERIFY(connectOverTcp(receiver));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate.entered(), TestTimeout::mediumMs());
    QPointer<GPSReceiverWorker> first = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(first);
    GPSReceiverFactGroup& facts = *receiver.facts();
    QVERIFY(!receiver.facts()->telemetryAvailable());
    emit first->receiverReady();
    GPSBaseStatus survey{};
    survey.state = GPSBaseState::Valid;
    survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    survey.duration = std::chrono::seconds(4294967295LL);
    survey.meanAccuracyMeters = 1.5;
    emit first->baseStatusUpdated(survey);
    GPSSatelliteReport satellites;
    satellites.timestampUs = 1;
    satellites.inView = 2;
    satellites.used = 7;
    emit first->satelliteInfoUpdated(satellites);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::shortMs());
    QCOMPARE(baseState(facts), GPSBaseState::Valid);
    QCOMPARE(facts.currentBasePosition(),
             std::optional(GPSBaseStationConfig::Fixed{.position = survey.position, .accuracyMeters = 1.5f}));
    QCOMPARE(facts.currentAccuracy()->rawValue().toDouble(), 1.5);
    QCOMPARE(facts.currentDuration()->rawValue().toLongLong(), 4294967295LL);
    QCOMPARE(solution(facts), (Solution{.inView = 2, .used = 7}));
    const auto frame = GPSTest::rtcmMessage(1005);
    emit first->rtcmDataReceived(frame, GPSTest::nowMs());
    QTRY_COMPARE_WITH_TIMEOUT(routed.size(), 1, TestTimeout::shortMs());
    QCOMPARE(routed[0], frame);
    QTRY_COMPARE_WITH_TIMEOUT(corrections.selectedStream().instanceId, instance, TestTimeout::shortMs());
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSettings::LocalReceiver));
    // Retirement must reject callbacks already in the GUI queue.
    satellites.used = 12;
    emit first->rtcmDataReceived(frame, GPSTest::nowMs());
    emit first->baseStatusUpdated(survey);
    emit first->satelliteInfoUpdated(satellites);
    emit first->positionUpdated(fixReport(GPSFixQuality::Unknown));
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::ConfigFailed,
                                QStringLiteral("Retired receiver configuration failure"));
    emit first->connectionError(GPSConnectionError::DeviceError);
    // Replacing the connection retires the first worker.
    receiver.disconnectReceiver();
    QVERIFY(connectOverTcp(receiver));
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(baseState(facts), GPSBaseState::Inactive);
    QVERIFY(!facts.currentBasePosition());
    QVERIFY(qIsNaN(facts.currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(facts.currentDuration()->rawValue().toLongLong(), 0);
    QCOMPARE(solution(facts), Solution{});
    QTRY_VERIFY_WITH_TIMEOUT(secondGate.entered(), TestTimeout::mediumMs());
    deliverQueuedCalls();
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(solution(facts), Solution{});
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(routed.size(), 1);
    auto* activeWorker = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(activeWorker);
    emit activeWorker->receiverReady();
    const auto receivedAtMs = GPSTest::nowMs() - 10;
    emit activeWorker->rtcmDataReceived(frame, receivedAtMs);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::shortMs());
    QCOMPARE(routed.size(), 2);
    QCOMPARE(routed[1], frame);
    QTRY_COMPARE_WITH_TIMEOUT(corrections.selectedStream().instanceId, instance, TestTimeout::shortMs());
    firstGate.release();
    QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), TestTimeout::mediumMs());
    QVERIFY(firstGate.sawCancellation());
    QVERIFY(receiver.facts()->telemetryAvailable());

    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    const QPointer<GPSReceiverWorker> second = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(second);
    emit second->connectionError(GPSConnectionError::DeviceError);
    emit second->rtcmDataReceived(frame, GPSTest::nowMs());
    emit second->receiverReady();
    emit second->baseStatusUpdated(survey);
    emit second->satelliteInfoUpdated(satellites);
    deliverQueuedCalls();
    verifyExpectedLogMessage();
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(facts.currentDuration()->rawValue().toLongLong(), 0);
    QCOMPARE(solution(facts), Solution{});
    QTRY_COMPARE_WITH_TIMEOUT(corrections.state(), GPSCorrectionManager::State::Inactive, TestTimeout::shortMs());
    QCOMPARE(routed.size(), 2);

    secondGate.release();
    QTRY_VERIFY_WITH_TIMEOUT(second.isNull(), TestTimeout::mediumMs());
    QVERIFY(secondGate.sawCancellation());
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Inactive);
}

void GPSReceiverTest::_workerCanOutliveManager()
{
    auto receiver = std::make_unique<GPSReceiver>();
    BlockedTransportGate gate;
    receiver->setWorkerFactory(workerFactory(gate.factory()));
    QVERIFY(connectOverTcp(*receiver));
    QTRY_VERIFY_WITH_TIMEOUT(gate.entered(), TestTimeout::mediumMs());
    QPointer<GPSReceiverWorker> worker = receiver->findChild<GPSReceiverWorker*>();
    QVERIFY(worker);
    emit worker->receiverReady();
    QTRY_VERIFY_WITH_TIMEOUT(receiver->facts()->telemetryAvailable(), TestTimeout::shortMs());
    receiver.reset();
    QVERIFY(worker);
    QVERIFY(!worker->parent());
    gate.release();
    QTRY_VERIFY_WITH_TIMEOUT(worker.isNull(), TestTimeout::mediumMs());
    QVERIFY(gate.sawCancellation());
}

void GPSReceiverTest::_shutdownJoinsRetiredWorkers_data()
{
    QTest::addColumn<bool>("cooperative");
    QTest::newRow("cooperative") << true;
    QTest::newRow("stuck") << false;
}

void GPSReceiverTest::_shutdownJoinsRetiredWorkers()
{
    QFETCH(bool, cooperative);
    BlockedTransportGate gate;
    const GPSReceiverWorker::TransportFactory waitForStop = [](GPSCancelToken cancelToken) {
        QSemaphore stopped;
        const GPSCancelCallback wake(cancelToken, [&stopped] { stopped.release(); });
        stopped.acquire();
        return std::unique_ptr<GPSTransport>{};
    };
    GPSReceiver receiver;
    receiver.setWorkerFactory(workerFactory(waitForStop));
    QVERIFY(connectOverTcp(receiver));
    const QPointer<GPSReceiverWorker> retired = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(retired);
    // A disconnect leaves the worker to finish on its own.
    receiver.disconnectReceiver();
    QVERIFY(retired);
    receiver.setWorkerFactory(workerFactory(cooperative ? waitForStop : gate.factory()));
    QVERIFY(receiver.connectReceiver());
    const QPointer<GPSReceiverWorker> active = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(active && active != retired);
    if (!cooperative) {
        QVERIFY(gate.waitEntered(TestTimeout::mediumMs()));
        expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("worker did not stop within")));
    }

    // At application exit no event loop deletes finished workers, so shutdown joins them without one.
    receiver.shutdown();
    QVERIFY(retired.isNull());
    if (cooperative) {
        QVERIFY(active.isNull());
        return;
    }
    verifyExpectedLogMessage();
    QVERIFY(active);
    gate.release();
    QTRY_VERIFY_WITH_TIMEOUT(active.isNull(), TestTimeout::mediumMs());
}

void GPSReceiverTest::_receiverFramesAreValidated_data()
{
    // The receiver's stream demultiplexer drops frames that fail their CRC; RTCMConformanceTest covers that.
    QTest::addColumn<bool>("expired");
    QTest::newRow("valid") << false;
    QTest::newRow("expired-before-dequeue") << true;
}

void GPSReceiverTest::_receiverFramesAreValidated()
{
    QFETCH(bool, expired);
    const auto frame = GPSTest::rtcmMessage(1005);
    GPSCorrectionManager corrections;
    ScriptedGPSReceiver harness(nullptr, {.corrections = &corrections});
    auto& receiver = harness.receiver;
    QVERIFY(connectOverTcp(receiver));
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    const auto receivedAtMs = GPSTest::nowMs() - (expired ? GPSCorrectionSelector::FRESHNESS_TIMEOUT.count() : 0);
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    worker->rtcm(frame, receivedAtMs);
    QCOMPARE(routed.size(), expired ? 0 : 1);
    if (!routed.isEmpty()) {
        QCOMPARE(routed[0], frame);
    }
}

void GPSReceiverTest::_udpPositionOnlyReceiver()
{
    GPSCorrectionManager corrections;
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    GPSReceiver receiver(nullptr, nullptr, {.corrections = &corrections});
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.receiverRole = RTKSettings::Passive;
    configuration.forwardReceiverRtcm = false;
    configuration.connectionType = RTKSettings::Udp;
    int failures = 0;
    const quint16 port = bindUnusedUdpPort(
        [&](quint16 candidate) {
            configuration.udpPort = candidate;
            receiver.setConfiguration(configuration);
            const auto settled = [&receiver] {
                return receiver.facts()->telemetryAvailable() || !receiver.hasReceiver();
            };
            return receiver.connectReceiver() &&
                   UnitTest::waitForCondition(settled, TestTimeout::mediumMs(), QStringLiteral("receiver settled")) &&
                   receiver.facts()->telemetryAvailable();
        },
        failures);
    if (failures > 0) {
        // Another process took a probed port before the worker bound it; the next port was tried.
        ignoreLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to open GPS receiver transport")));
        ignoreLogMessage("GPS.Transport.UDPGPSTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Cannot listen for receiver data on UDP port")));
    }
    QVERIFY(port);
    QCOMPARE(receiver.activeRole(), RTKSettings::Passive);
    QVERIFY(!receiver.forwardingCorrections());
    QCOMPARE(receiver.activeEndpoint(), GPSReceiver::tr("UDP port %1").arg(port));
    // A silent position-only link waits for data instead of reconnecting.
    auto* worker = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(worker);
    QVERIFY(!worker->_endsWhenIdle);

    QByteArray stream;
    for (const QByteArray& body : {QByteArray("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A"),
                                   QByteArray("$GPGST,092750.000,1,1,1,0,1,1,2"),
                                   QByteArray("$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,")}) {
        stream += GPSTest::repairChecksum(body);
    }
    stream += GPSTest::rtcmMessage(1005, 20);
    QUdpSocket sender;
    QTRY_VERIFY_WITH_TIMEOUT(sender.writeDatagram(stream, QHostAddress::LocalHost, port) == stream.size() &&
                                 receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation),
                             TestTimeout::mediumMs());
    const auto observation = receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga);
    QVERIFY(observation);
    QCOMPARE(observation->altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    // Position-only receivers never contribute corrections.
    QVERIFY(routed.isEmpty());
    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
}

void GPSReceiverTest::_passiveForwardingRegistersCorrections_data()
{
    QTest::addColumn<bool>("forward");
    QTest::newRow("forward") << true;
    QTest::newRow("position-only") << false;
}

void GPSReceiverTest::_passiveForwardingRegistersCorrections()
{
    QFETCH(bool, forward);
    GPSCorrectionManager corrections;
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    ScriptedGPSReceiver harness(nullptr, {.corrections = &corrections});
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.forwardReceiverRtcm = forward;
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeRole(), RTKSettings::Passive);
    QCOMPARE(receiver.forwardingCorrections(), forward);
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    worker->ready();
    worker->rtcm(GPSTest::rtcmMessage(1005, 20), GPSTest::nowMs());
    deliverQueuedCalls();
    QCOMPARE(routed.isEmpty(), !forward);
}

void GPSReceiverTest::_passiveInputStatus_data()
{
    QTest::addColumn<GPSInputProblem>("problem");
    // The protocol the input carries, as a GPSType, or -1 before one is identified.
    QTest::addColumn<int>("protocol");
    QTest::addColumn<bool>("forward");
    QTest::addColumn<QString>("detected");
    QTest::addColumn<QString>("message");
    constexpr int none = -1;
    QTest::newRow("no-data") << GPSInputProblem::NoData << none << true << QString()
                             << QStringLiteral(
                                    "No data from the receiver on rtk.test:2101. Check the device and "
                                    "that no other program is using it.");
    QTest::newRow("not-gnss") << GPSInputProblem::NotGNSS << none << true << QString()
                              << QStringLiteral(
                                     "Receiving data that isn't GNSS output. Check the device and baud rate.");
    QTest::newRow("ubx") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::UBlox) << true
                         << QStringLiteral("u-blox (UBX)")
                         << QStringLiteral(
                                "Receiving u-blox UBX data without position messages. Enable NAV-PVT or "
                                "NMEA output.");
    QTest::newRow("sbf") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::Septentrio) << true
                         << QStringLiteral("Septentrio (SBF)")
                         << QStringLiteral(
                                "Receiving Septentrio SBF data without position messages. Enable "
                                "PVTGeodetic or NMEA output.");
    QTest::newRow("nmea") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::Passive) << true
                          << QStringLiteral("NMEA")
                          << QStringLiteral("Receiving NMEA data without position messages. Enable GGA output.");
    QTest::newRow("corrections-forwarded")
        << GPSInputProblem::CorrectionsOnly << none << true << QString() << QString();
    QTest::newRow("corrections-dropped") << GPSInputProblem::CorrectionsOnly << none << false << QString()
                                         << QStringLiteral(
                                                "Receiving RTCM corrections without position messages. "
                                                "Turn on Forward receiver RTCM to send them to vehicles.");
}

void GPSReceiverTest::_passiveInputStatus()
{
    QFETCH(GPSInputProblem, problem);
    QFETCH(int, protocol);
    QFETCH(bool, forward);
    QFETCH(QString, detected);
    QFETCH(QString, message);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.forwardReceiverRtcm = forward;
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    worker->ready();
    if (protocol >= 0) {
        worker->detected(static_cast<GPSType>(protocol));
    }
    // The identified protocol is shown, while the passive family's settings stay in effect.
    QCOMPARE(receiver.detectedReceiver(), detected);
    QCOMPARE(receiver._activeManufacturer(), kPassiveManufacturer);
    QSignalSpy errors(&receiver, &GPSReceiver::errorMessageChanged);
    worker->input(problem);
    QCOMPARE(receiver.errorMessage(), message);
    QCOMPARE(errors.size(), message.isEmpty() ? 0 : 1);
    QVERIFY(receiver.facts()->telemetryAvailable() && receiver.hasReceiver());
    worker->input(GPSInputProblem::None);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(receiver.facts()->telemetryAvailable());
}

void GPSReceiverTest::_tcpEndpointNamesHostAsEntered()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    useTcp(configuration);
    // The zone of a link-local IPv6 address reads like a place marker of QString::arg().
    configuration.tcpHost = QStringLiteral("fe80::1%2");
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("fe80::1%2:2101"));
}

void GPSReceiverTest::_openFailureNamesCause()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.autoConnect = false;
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    GPSReceiverFactGroup& facts = *receiver.facts();
    QSignalSpy connected(&facts, &FactGroup::telemetryAvailableChanged);
    QVERIFY(receiver.connectReceiver());
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    harness.workers.current()->fail(GPSConnectionError::OpenFailed, QStringLiteral("Connection refused"));
    verifyExpectedLogMessage();
    QCOMPARE(receiver.errorMessage(), QStringLiteral("Failed to open the receiver: Connection refused."));
    // A receiver that never opened was never connected.
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QVERIFY(connected.isEmpty());
}

void GPSReceiverTest::_recordsSessionsStartedWhileOn()
{
    TestFixtures::TempDirFixture directory;
    QTcpServer peer;
    QVERIFY(peer.listen(QHostAddress::LocalHost));
    GPSReceiver receiver(nullptr, nullptr, {.recordingDirectory = [&directory] { return directory.path(); }});
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.autoConnect = false;
    configuration.connectionType = RTKSettings::Tcp;
    configuration.tcpHost = QStringLiteral("127.0.0.1");
    configuration.tcpPort = peer.serverPort();
    receiver.setConfiguration(configuration);
    QSignalSpy fileChanges(&receiver, &GPSReceiver::recordingFileChanged);
    // Connects the receiver and returns the peer's end of its link once the session is ready, which a disconnect
    // then ends without a configuration failure.
    const auto connectSession = [&]() -> QTcpSocket* {
        const auto ready = [&receiver] { return receiver.facts()->telemetryAvailable(); };
        if (!receiver.connectReceiver() ||
            (!peer.hasPendingConnections() && !peer.waitForNewConnection(TestTimeout::mediumMs())) ||
            !UnitTest::waitForCondition(ready, TestTimeout::mediumMs(), QStringLiteral("receiver ready"))) {
            return nullptr;
        }
        return peer.nextPendingConnection();
    };

    // Off unless turned on, and turning it on leaves the session that already runs alone.
    QVERIFY(!receiver.recordSessions());
    QVERIFY(connectSession());
    QVERIFY(receiver.recordingFile().isEmpty());
    QSignalSpy recordChanges(&receiver, &GPSReceiver::recordSessionsChanged);
    receiver.setRecordSessions(true);
    QCOMPARE(recordChanges.size(), 1);
    QVERIFY(receiver.recordingFile().isEmpty());
    receiver.disconnectReceiver();
    QVERIFY(fileChanges.isEmpty());

    QTcpSocket* const link = connectSession();
    QVERIFY(link);
    const QString file = receiver.recordingFile();
    QCOMPARE(fileChanges.size(), 1);
    QCOMPARE(QFileInfo(file).absolutePath(), directory.path());
    QVERIFY2(QRegularExpression(QStringLiteral("^\\d{8}T\\d{6}\\.\\d{3}Z-passive\\.log$"))
                 .match(QFileInfo(file).fileName())
                 .hasMatch(),
             qPrintable(file));
    QByteArray stream;
    for (const QByteArray& body : {QByteArray("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A"),
                                   QByteArray("$GPGST,092750.000,1,1,1,0,1,1,2"),
                                   QByteArray("$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,")}) {
        stream += GPSTest::repairChecksum(body);
    }
    QCOMPARE(link->write(stream), stream.size());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.acceptedPositionObservation(GPSObservation::PositionUse::GroundStation),
                             TestTimeout::mediumMs());

    // The worker closes the recording as it exits, and the file is the session's only while it runs.
    receiver.shutdown();
    QVERIFY(receiver.recordingFile().isEmpty());
    QCOMPARE(fileChanges.size(), 2);
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList{QFileInfo(file).fileName()});
    QString damage;
    QByteArray received;
    std::optional<GPSTest::GPSRecordingSettings> settings;
    for (const GPSTest::GPSRecordingRecord& record : GPSTest::GPSRecordingReader::readFile(file, &damage, &settings)) {
        if (record.kind == GPSTest::GPSRecordingRecord::Kind::Received) {
            received += record.bytes;
        }
    }
    QCOMPARE(damage, QString());
    QCOMPARE(received, stream);
    // The recording says what its session was asked to do and on which kind of link.
    QVERIFY(settings);
    QCOMPARE(settings->type, GPSType::Passive);
    QCOMPARE(settings->fixedBaudRate, GPSTransport::BRIDGE_BAUD_RATE);
}

void GPSReceiverTest::_recordingFileClearsWhenRecordingFails()
{
    TestFixtures::TempDirFixture directory;
    // A regular file where the recording folder should be.
    const QString folder = directory.createFile(QStringLiteral("GPSReceiver"), "not a folder");
    QTcpServer peer;
    QVERIFY(peer.listen(QHostAddress::LocalHost));
    GPSReceiver receiver(nullptr, nullptr, {.recordingDirectory = [&folder] { return folder; }});
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    configuration.autoConnect = false;
    configuration.connectionType = RTKSettings::Tcp;
    configuration.tcpHost = QStringLiteral("127.0.0.1");
    configuration.tcpPort = peer.serverPort();
    receiver.setConfiguration(configuration);
    receiver.setRecordSessions(true);
    QSignalSpy fileChanges(&receiver, &GPSReceiver::recordingFileChanged);

    // The link opens although its recording cannot, and the receiver withdraws the file it named.
    expectLogMessage("GPS.Transport.GPSRecordingTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("file error")));
    QVERIFY(receiver.connectReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(receiver.recordingFile().isEmpty());
    QCOMPARE(fileChanges.size() % 2, 0);
    receiver.shutdown();
}

void GPSReceiverTest::_recordingsStartedTogetherGetOwnFiles()
{
    TestFixtures::TempDirFixture directory;
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler, {.recordingDirectory = [&directory] { return directory.path(); }});
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(kPassiveManufacturer);
    useTcp(configuration);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    receiver.setRecordSessions(true);

    // A recording is named by the millisecond its session starts in. These sessions follow each other far faster than
    // that, as one that an observer starts from the end of the one before does, and a recording is never made over
    // an earlier one of its name.
    QStringList files;
    for (int session = 0; session < 100; ++session) {
        QVERIFY(receiver.connectReceiver());
        files.append(receiver.recordingFile());
        receiver.disconnectReceiver();
        harness.workers.finishRetired();
    }
    // Each has a name of its own, and the names sort in the order the sessions started.
    QStringList ordered = files;
    ordered.removeDuplicates();
    ordered.sort();
    QCOMPARE(files, ordered);
}

void GPSReceiverTest::_withoutSerialLinksSerialSelectionUsesTcp()
{
    // The serial side of a build without serial links, on any build.
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler, {.serial = withoutSerialLinks});
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(!receiver.serialSupported());
    QVERIFY(receiver.serialBaudRates().isEmpty());
    QVERIFY(receiver.serialPortEntries().isEmpty());
    // No device is available, discovered or claimed.
    const QString device = QStringLiteral("/dev/ttyACM0");
    QVERIFY(!receiver._serial->available(device));
    QVERIFY(receiver._serial->settledRtkReceiver(0).isEmpty());
    QVERIFY(!receiver._serial->claim(device));

    // The default selection, a base on any USB device, has nothing to discover and no address to connect.
    auto configuration = receiverConfiguration();
    QCOMPARE(configuration.connectionType, RTKSettings::Serial);
    QVERIFY(configuration.autoConnect && configuration.serialDevice.isEmpty());
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.effectiveConnectionType(), RTKSettings::Tcp);
    QVERIFY(receiver.connectionSupported() && receiver.canConnect());
    receiver.tick();
    QVERIFY(scheduler.advanceBy(10s));
    receiver.tick();
    QCOMPARE(harness.workers.count(), 0);
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(receiver.errorMessage().isEmpty());
    QVERIFY(!receiver.connectReceiver());
    QCOMPARE(receiver.errorMessage(), GPSReceiverConnectionPolicy::tr("Enter the receiver's TCP host and port."));

    // With an address, the serial selection is that TCP receiver, at the rate of a network connection: the saved
    // serial device and rate are not used.
    configuration.serialDevice = QStringLiteral("/test/saved");
    configuration.serialBaudRate = 57600;
    configuration.tcpHost = QStringLiteral("rtk.test");
    configuration.tcpPort = 2101;
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("rtk.test:2101"));
    QCOMPARE(harness.workers.current()->capturedConfig().baudRate, GPSTransport::BRIDGE_BAUD_RATE);
    receiver.shutdown();
    harness.workers.finishRetired();
}

void GPSReceiverTest::_configuredPresentation_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("rtkBase");
    QTest::addColumn<bool>("receiverAveraging");
    QTest::addColumn<bool>("surveyIn");
    // Saved baseReceiverManufacturer values.
    QTest::newRow("automatic") << 0 << true << true << true;
    QTest::newRow("trimble") << 1 << true << false << true;
    QTest::newRow("septentrio") << 2 << true << false << true;
    QTest::newRow("femtomes") << 3 << true << false << true;
    QTest::newRow("ublox") << 4 << true << false << true;
    QTest::newRow("unicore") << 5 << true << true << false;
    QTest::newRow("quectel") << 6 << true << false << true;
    QTest::newRow("passive") << kPassiveManufacturer << false << false << false;
    QTest::newRow("invalid") << 99 << false << false << false;
}

void GPSReceiverTest::_configuredPresentation()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, rtkBase);
    QFETCH(bool, receiverAveraging);
    QFETCH(bool, surveyIn);
    GPSReceiver receiver;
    // The passive family is selected by role, so a passive receiver's manufacturer does not matter.
    const bool passive = manufacturer == kPassiveManufacturer;
    GPSReceiver::Configuration configuration;
    configuration.receiverRole = passive ? RTKSettings::Passive : RTKSettings::ConfiguredBase;
    configuration.baseReceiverManufacturer = manufacturer;
    receiver.setConfiguration(configuration);
    const auto caps = receiver.property("configuredPresentation").value<GPSReceiverPresentation>();
    QCOMPARE(caps.passive, passive);
    QCOMPARE(caps.rtkBase, rtkBase);
    QCOMPARE(caps.receiverAveraging, receiverAveraging);
    QCOMPARE(caps.surveyIn, surveyIn);
}

void GPSReceiverTest::_configurationDiagnosticRetained_data()
{
    QTest::addColumn<QString>("session");
    QTest::addColumn<GPSConnectionError>("error");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<QString>("expected");

    const struct
    {
        const char* name;
        GPSConnectionError error;
        QString detail;
    } cases[] = {{"provisioning-mismatch", GPSConnectionError::ConfigFailed,
                  QStringLiteral("Requested base settings differ from receiver readback.")},
                 {"possibly-persisted", GPSConnectionError::ConfigFailed,
                  QStringLiteral("Settings may have been saved, but reconnect failed.")},
                 {"empty-fallback", GPSConnectionError::ConfigFailed, QString()},
                 // Flash-save advice is for auto-connect; a manual connection reports a refusal like any failure.
                 {"consent-refused", GPSConnectionError::ConsentRequired,
                  QStringLiteral("LG290P role mismatch: save the requested base role externally")}};

    for (const auto& [name, error, detail] : cases) {
        const QString reported =
            detail.isEmpty()
                ? GPSReceiver::tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode.")
                : GPSReceiver::tr("Receiver configuration failed: %1").arg(detail);
        // Only a connection that reached the receiver is retried, as Connect automatically is on by default.
        const QString retried =
            detail.isEmpty()
                ? GPSReceiver::tr("Receiver configuration failed. Reconnecting automatically.")
                : GPSReceiver::tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail);
        QTest::addRow("first-attempt-%s", name) << QStringLiteral("first-attempt") << error << detail << reported;
        QTest::addRow("reconnecting-%s", name) << QStringLiteral("reconnecting") << error << detail << retried;
    }
}

void GPSReceiverTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, session);
    QFETCH(GPSConnectionError, error);
    QFETCH(QString, detail);
    QFETCH(QString, expected);
    const bool reconnecting = session == QStringLiteral("reconnecting");
    ScriptedGPSReceiver harness;
    auto& receiver = harness.receiver;
    auto configuration = receiverConfiguration(gpsReceiverManufacturerForType(GPSType::Quectel));
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    if (reconnecting) {
        worker->ready();
    }
    receiver.setConnectionError(QStringLiteral("An earlier configuration error"));
    QSignalSpy messages(&receiver, &GPSReceiver::errorMessageChanged);
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(reconnecting ? QStringLiteral("GPS receiver session ended")
                                                     : QStringLiteral("GPS receiver did not accept configuration")));
    worker->fail(error, detail);
    verifyExpectedLogMessage();
    QCOMPARE(messages.size(), 1);
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QCOMPARE(receiver.reconnecting(), reconnecting);
    QCOMPARE(receiver.errorMessage(), expected);
    // The settings show the permission for a receiver that asked for it, while they show that failure.
    QCOMPARE(receiver.persistentChangesRequired(), error == GPSConnectionError::ConsentRequired);
    receiver.setConnectionError(QString());
    QVERIFY(!receiver.persistentChangesRequired());
}

void GPSReceiverTest::_persistentConsentScope_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<bool>("keepsConsent");
    // The permission covers one receiver, connection and base selection.
    for (const auto* setting : {"serial-device", "baud-rate", "base-mode", "manufacturer", "role", "connection-type",
                                "tcp-host", "tcp-port", "udp-port"}) {
        QTest::newRow(setting) << QString::fromLatin1(setting) << false;
    }
    QTest::newRow("survey-accuracy") << QStringLiteral("survey-accuracy") << true;
    QTest::newRow("auto-connect") << QStringLiteral("auto-connect") << true;
}

void GPSReceiverTest::_persistentConsentScope()
{
    QFETCH(QString, setting);
    QFETCH(bool, keepsConsent);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(gpsReceiverManufacturerForType(GPSType::Quectel));
    receiver.setConfiguration(configuration);
    QSignalSpy changed(&receiver, &GPSReceiver::persistentChangesAllowedChanged);
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.persistentChangesAllowed());
    QCOMPARE(changed.size(), 1);

    if (setting == QStringLiteral("serial-device")) {
        configuration.serialDevice = QStringLiteral("/test/other");
    } else if (setting == QStringLiteral("baud-rate")) {
        configuration.serialBaudRate = 230400;
    } else if (setting == QStringLiteral("base-mode")) {
        configuration.base.mode = GPSBaseStationConfig::Fixed{};
    } else if (setting == QStringLiteral("manufacturer")) {
        configuration.baseReceiverManufacturer = gpsReceiverManufacturerForType(GPSType::UBlox);
    } else if (setting == QStringLiteral("role")) {
        configuration.receiverRole = RTKSettings::Passive;
    } else if (setting == QStringLiteral("connection-type")) {
        configuration.connectionType = RTKSettings::Tcp;
    } else if (setting == QStringLiteral("tcp-host")) {
        configuration.tcpHost = QStringLiteral("other.example");
    } else if (setting == QStringLiteral("tcp-port")) {
        configuration.tcpPort = 2102;
    } else if (setting == QStringLiteral("udp-port")) {
        configuration.udpPort = 14402;
    } else if (setting == QStringLiteral("survey-accuracy")) {
        std::get<GPSBaseStationConfig::SurveyIn>(configuration.base.mode).accuracyMeters = 5.;
    } else {
        configuration.autoConnect = !configuration.autoConnect;
    }
    receiver.setConfiguration(configuration);
    QCOMPARE(receiver.persistentChangesAllowed(), keepsConsent);
    QCOMPARE(changed.size(), keepsConsent ? 1 : 2);
}

void GPSReceiverTest::_persistentConsentIsSpentOnConnect_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("granted");
    QTest::addColumn<bool>("expected");
    const int quectel = gpsReceiverManufacturerForType(GPSType::Quectel);
    const int ublox = gpsReceiverManufacturerForType(GPSType::UBlox);
    QTest::newRow("quectel-granted") << quectel << true << true;
    QTest::newRow("quectel-withheld") << quectel << false << false;
    // Families that cannot save settings never receive the permission.
    QTest::newRow("ublox-granted") << ublox << true << false;
}

void GPSReceiverTest::_persistentConsentIsSpentOnConnect()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, granted);
    QFETCH(bool, expected);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    auto configuration = receiverConfiguration(manufacturer);
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    receiver.setPersistentChangesAllowed(granted);

    QVERIFY(receiver.connectReceiver());
    QVERIFY(harness.workers.current());
    QCOMPARE(harness.workers.current()->capturedConfig().allowPersistentChanges, expected);
    QVERIFY(!receiver.persistentChangesAllowed());

    // A session change or disconnect withdraws a permission granted while connected.
    receiver.setPersistentChangesAllowed(true);
    receiver.disconnectReceiver();
    QVERIFY(!receiver.persistentChangesAllowed());
}

void GPSReceiverTest::_effectiveConnection_data()
{
    QTest::addColumn<bool>("serialLinks");
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("saved");
    QTest::addColumn<int>("effective");
    QTest::addColumn<bool>("supported");
#ifdef QGC_NO_SERIAL_LINK
    const int serial = RTKSettings::Tcp;
#else
    const int serial = RTKSettings::Serial;
#endif
    const auto row = [](const char* name, bool serialLinks, RTKSettings::ReceiverRole role, int saved, int effective,
                        bool supported) {
        QTest::newRow(name) << serialLinks << int(role) << saved << effective << supported;
    };
    row("base-serial", true, RTKSettings::ConfiguredBase, RTKSettings::Serial, serial, true);
    row("base-tcp", true, RTKSettings::ConfiguredBase, RTKSettings::Tcp, RTKSettings::Tcp, true);
    row("base-udp", true, RTKSettings::ConfiguredBase, RTKSettings::Udp, RTKSettings::Udp, false);
    row("passive-udp", true, RTKSettings::Passive, RTKSettings::Udp, RTKSettings::Udp, true);
    row("unknown-type", true, RTKSettings::ConfiguredBase, 7, serial, true);
    // A receiver without serial links, as every receiver of a build without them is, connects a serial selection over
    // TCP.
    row("no-serial-links-base-serial", false, RTKSettings::ConfiguredBase, RTKSettings::Serial, RTKSettings::Tcp, true);
    row("no-serial-links-passive-udp", false, RTKSettings::Passive, RTKSettings::Udp, RTKSettings::Udp, true);
    row("no-serial-links-unknown-type", false, RTKSettings::ConfiguredBase, 7, RTKSettings::Tcp, true);
}

void GPSReceiverTest::_effectiveConnection()
{
    QFETCH(bool, serialLinks);
    QFETCH(int, role);
    QFETCH(int, saved);
    QFETCH(int, effective);
    QFETCH(bool, supported);
    ScriptedGPSReceiver harness(
        nullptr, serialLinks ? GPSReceiver::Dependencies{} : GPSReceiver::Dependencies{.serial = withoutSerialLinks});
    GPSReceiver& receiver = harness.receiver;
    QSignalSpy changed(&receiver, &GPSReceiver::configurationChanged);
    auto configuration = receiverConfiguration();
    configuration.receiverRole = static_cast<RTKSettings::ReceiverRole>(role);
    configuration.connectionType = static_cast<RTKSettings::ConnectionType>(saved);
    receiver.setConfiguration(configuration);
    QCOMPARE(int(receiver.effectiveConnectionType()), effective);
    QCOMPARE(receiver.connectionSupported(), supported);
    QCOMPARE(receiver.property("effectiveConnectionType").toInt(), effective);
    QVERIFY(changed.size() <= 1);
}

#ifndef QGC_NO_SERIAL_LINK

void GPSReceiverTest::_openSerialDevicesWith(
    GPSReceiver& receiver, std::function<std::unique_ptr<GPSTransport>(const QString&, GPSCancelToken)> open)
{
    static_cast<GPSReceiverSystemSerialPorts*>(receiver._serial)->_openDevice = std::move(open);
}

void GPSReceiverTest::_serialPortEntries()
{
    // A u-blox receiver, a flight controller connected as a vehicle, a debug probe, a radio the board list names,
    // and a port without USB identity.
    TestSerialPorts serial({
        {.systemLocation = QStringLiteral("/dev/ttyACM0"),
         .portName = QStringLiteral("ttyACM0"),
         .boardType = QGCSerialPortInfo::BoardTypeRTKGPS,
         .boardName = QStringLiteral("U-blox RTK GPS"),
         .description = QStringLiteral("u-blox GNSS receiver")},
        {.systemLocation = QStringLiteral("/dev/ttyACM1"),
         .portName = QStringLiteral("ttyACM1"),
         .boardType = QGCSerialPortInfo::BoardTypePixhawk,
         .boardName = QStringLiteral("CubeOrange"),
         .description = QStringLiteral("CubeOrange+")},
        {.systemLocation = QStringLiteral("/dev/ttyACM2"),
         .portName = QStringLiteral("ttyACM2"),
         .boardName = {},
         .description = QStringLiteral("STLINK-V3")},
        {.systemLocation = QStringLiteral("/dev/ttyUSB0"),
         .portName = QStringLiteral("ttyUSB0"),
         .boardType = QGCSerialPortInfo::BoardTypeSiKRadio,
         .boardName = QStringLiteral("SiK Radio")},
        serialPort(QStringLiteral("/dev/ttyS0")),
    });
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler, {.serial = serial.side()});
    GPSReceiver& receiver = harness.receiver;
    auto configuration = serialConfiguration(kPassiveManufacturer, QStringLiteral("/dev/ttyACM0"), 115200);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    auto vehicleLink = serial.manager.reservePort(QStringLiteral("/dev/ttyACM1"));
    QVERIFY(vehicleLink);
    QSignalSpy changes(&receiver, &GPSReceiver::serialPortEntriesChanged);
    serial.manager.rescan();
    QCOMPARE(changes.size(), 1);
    QVERIFY(receiver.connectReceiver());
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/dev/ttyACM0")));
    const auto entries = [&receiver](QString GPSSerialPortEntry::* field) {
        QStringList values;
        for (const GPSSerialPortEntry& entry : receiver.serialPortEntries()) {
            values.append(entry.*field);
        }
        return values;
    };
    // The picker shows the device's name and path, and stores the path. Other ports open while the link holds its
    // port, so only that one is marked.
    const QStringList unmarked{
        QStringLiteral("u-blox GNSS receiver – /dev/ttyACM0"),
        QStringLiteral("CubeOrange+ – /dev/ttyACM1"),
        QStringLiteral("STLINK-V3 – /dev/ttyACM2"),
        QStringLiteral("SiK Radio – /dev/ttyUSB0"),
        QStringLiteral("/dev/ttyS0"),
    };
    QStringList linkPortMarked = unmarked;
    linkPortMarked[1] = QStringLiteral("(in use) CubeOrange+ – /dev/ttyACM1");
    QCOMPARE(entries(&GPSSerialPortEntry::label), linkPortMarked);
    QCOMPARE(entries(&GPSSerialPortEntry::value).first(), QStringLiteral("/dev/ttyACM0"));

    // A link releases its port without a scan, so the mark follows it when the receiver is next polled. The
    // receiver's own claim never marks its port, even while a disconnected session's worker still holds it.
    vehicleLink.reset();
    receiver.disconnectReceiver();
    receiver.tick();
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/dev/ttyACM0")));
    QCOMPARE(entries(&GPSSerialPortEntry::label), unmarked);
    harness.workers.finishRetired();
    QVERIFY(!serial.manager.isPortReserved(QStringLiteral("/dev/ttyACM0")));

    // Where only one port can be open at a time, nothing is scanned while a link holds its port, and the port is
    // marked all the same. No other port can be opened until the link releases it, so each says so.
    serial.manager.setSinglePortOnly(true);
    QSignalSpy scans(&serial.manager, &SerialPortManager::portsEnumerated);
    vehicleLink = serial.manager.reservePort(QStringLiteral("/dev/ttyACM1"));
    QVERIFY(vehicleLink);
    changes.clear();
    receiver.tick();
    const QStringList everyPortMarked{
        QStringLiteral("(another is in use) u-blox GNSS receiver – /dev/ttyACM0"),
        QStringLiteral("(in use) CubeOrange+ – /dev/ttyACM1"),
        QStringLiteral("(another is in use) STLINK-V3 – /dev/ttyACM2"),
        QStringLiteral("(another is in use) SiK Radio – /dev/ttyUSB0"),
        QStringLiteral("(another is in use) /dev/ttyS0"),
    };
    QCOMPARE(entries(&GPSSerialPortEntry::label), everyPortMarked);
    QCOMPARE(changes.size(), 1);
    QVERIFY(scans.isEmpty());
    // Polled again, the entries are as they were, so their views are left alone.
    receiver.tick();
    QCOMPARE(changes.size(), 1);

    // Links are connected and disconnected while connection polling is suspended too, as during a flight data
    // replay. The receiver then connects nothing, not even its saved device that is free, and the marks still follow
    // the links.
    configuration.autoConnect = true;
    receiver.setConfiguration(configuration);
    receiver.startConnectionPolling([] { return true; });
    vehicleLink.reset();
    QVERIFY(scheduler.advanceBy(GPSReceiver::POLL_INTERVAL));
    QCOMPARE(entries(&GPSSerialPortEntry::label), unmarked);
    QCOMPARE(changes.size(), 2);
    QVERIFY(!receiver.hasReceiver());
    vehicleLink = serial.manager.reservePort(QStringLiteral("/dev/ttyACM1"));
    QVERIFY(vehicleLink);
    QVERIFY(scheduler.advanceBy(GPSReceiver::POLL_INTERVAL));
    QCOMPARE(entries(&GPSSerialPortEntry::label), everyPortMarked);
    QCOMPARE(changes.size(), 3);
    QVERIFY(scans.isEmpty());

    // The receiver's own session keeps the other ports closed as well. It marks them no more than its own port: the
    // marks tell what other connections hold.
    vehicleLink.reset();
    QVERIFY(receiver.connectReceiver());
    QVERIFY(!serial.manager.canReservePort(QStringLiteral("/dev/ttyACM2")));
    QVERIFY(scheduler.advanceBy(GPSReceiver::POLL_INTERVAL));
    QCOMPARE(entries(&GPSSerialPortEntry::label), unmarked);
}

void GPSReceiverTest::_serialReservationSurvivesDelayedStop()
{
    TestSerialPorts serial;
    GPSReceiver receiver;
    BlockedTransportGate gate;
    receiver.setConfiguration(receiverConfiguration(kPassiveManufacturer));
    _openSerialDevicesWith(receiver, [factory = gate.factory()](const QString&, GPSCancelToken cancelToken) {
        return factory(std::move(cancelToken));
    });
    QVERIFY(receiver.connectSerial(QStringLiteral("/test/selected"), GPSType::Passive, {.baudRate = 115200},
                                   serial.manager.reservePort(QStringLiteral("/test/selected"))));
    QTRY_VERIFY_WITH_TIMEOUT(gate.entered(), TestTimeout::mediumMs());
    QPointer<GPSReceiverWorker> worker = receiver.findChild<GPSReceiverWorker*>();
    QVERIFY(worker);
    emit worker->receiverReady();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::shortMs());
    bool heartbeat = false;
    QTimer::singleShot(0, &receiver, [&heartbeat] { heartbeat = true; });
    // The worker stays blocked until the gate opens, so a disconnect that waited for it would not return.
    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.facts()->telemetryAvailable());
    QTRY_VERIFY_WITH_TIMEOUT(heartbeat, TestTimeout::shortMs());
    QVERIFY(worker && worker->_thread && worker->_thread->isRunning());
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/test/selected")));
    gate.release();
    QTRY_VERIFY_WITH_TIMEOUT(worker.isNull(), TestTimeout::mediumMs());
    QVERIFY(serial.manager.canReservePort(QStringLiteral("/test/selected")));
}

namespace {
struct PassiveTransportState
{
    std::atomic_uint baud = 0;
    std::atomic_uint baudChanges = 0;
    std::atomic_uint writes = 0;
    QSemaphore reading;
    QSemaphore releaseRead;
};

std::unique_ptr<GPSTransport> makePassiveTestTransport(GPSCancelToken cancelToken,
                                                       const std::shared_ptr<PassiveTransportState>& state)
{
    auto transport = std::make_unique<ScriptedReceiver>(std::move(cancelToken));
    transport->setBaudRateHandler([state](unsigned baud) -> std::optional<bool> {
        state->baud = baud;
        ++state->baudChanges;
        return true;
    });
    transport->setReadHandler([state](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
        state->reading.release();
        state->releaseRead.acquire();
        return GPSReadResult{GPSReadStatus::Cancelled};
    });
    transport->setWriteHandler(
        [state](const QByteArray&, const ScriptedReceiver::WriteContext&) -> std::optional<GPSWriteResult> {
            ++state->writes;
            return GPSWriteResult{GPSWriteStatus::Error};
        });
    return transport;
}
}  // namespace

void GPSReceiverTest::_manualPassiveBaudPreserved_data()
{
    QTest::addColumn<uint>("baud");
    QTest::newRow("minimum") << 1200U;
    QTest::newRow("maximum") << 4000000U;
}

void GPSReceiverTest::_manualPassiveBaudPreserved()
{
    QFETCH(uint, baud);
    TestSerialPorts serial({serialPort(QStringLiteral("/test/passive"))});
    auto state = std::make_shared<PassiveTransportState>();
    GPSReceiver receiver(nullptr, nullptr, {.serial = serial.side()});
    auto configuration = serialConfiguration(kPassiveManufacturer, QStringLiteral("/test/passive"), baud);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    _openSerialDevicesWith(receiver, [state](const QString&, GPSCancelToken cancelToken) {
        return makePassiveTestTransport(std::move(cancelToken), state);
    });
    const auto releaseWorker = qScopeGuard([&] { state->releaseRead.release(); });
    // The user's Connect claims the port through the application's serial ports.
    QVERIFY(receiver.connectReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable() && state->reading.available() > 0,
                             TestTimeout::mediumMs());
    QCOMPARE(state->baud.load(), baud);
    QCOMPARE(state->baudChanges.load(), 1U);
    QCOMPARE(state->writes.load(), 0U);
    QCOMPARE(baseState(*receiver.facts()), GPSBaseState::Inactive);
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/test/passive")));
    state->releaseRead.release();
    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(serial.manager.canReservePort(QStringLiteral("/test/passive")), TestTimeout::mediumMs());
}
#endif
