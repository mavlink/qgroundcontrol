#include "GPSReceiverTest.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCancellation.h"
#include "GPSCorrectionManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverWorker.h"
#include "GPSSettingsBindings.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NMEASentence.h"
#include "PositionManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "Support/GPSTestHelpers.h"
#include "Transport/Support/UnusedUdpPort.h"

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
    const int ublox = gpsReceiverManufacturerForType(GPSType::ublox);
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
    settings.baseReceiverManufacturers()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    settings.connectionType()->setRawValue(RTKSettings::Tcp);
    settings.tcpHost()->setRawValue(QStringLiteral("rtk.example"));
    settings.tcpPort()->setRawValue(2101);
    settings.useFixedBasePosition()->setRawValue(static_cast<int>(BaseModeDefinition::Mode::BaseReceiverAveraging));

    // Consent and receiver-managed averaging are checked against the detected family, on the worker.
    receiver.setPersistentChangesAllowed(true);
    QVERIFY(receiver.connectReceiver());
    auto* worker = harness.workers.current();
    QVERIFY(worker);
    QCOMPARE(worker->type(), GPSType::automatic);
    QVERIFY(worker->capturedConfig().allowPersistentChanges);
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(worker->capturedConfig().base.mode));
    QCOMPARE(receiver._activeManufacturer(), GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY(receiver.activePresentation().automatic);
    QVERIFY(receiver.detectedReceiver().isEmpty());

    QSignalSpy changed(&receiver, &GPSReceiver::receiverChanged);
    worker->detected(GPSType::unicore);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Unicore"));
    QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::unicore));
    QVERIFY(receiver.activePresentation().receiverAveraging);
    worker->ready(QStringLiteral("UM982 R4.10Build15434"));
    QVERIFY(receiver.facts()->telemetryAvailable());
    QCOMPARE(receiver.detectedReceiver(), QStringLiteral("Unicore"));
    // Automatic stays selected whatever it detects.
    QCOMPARE(settings.baseReceiverManufacturers()->rawValue().toInt(), GPS_AUTOMATIC_MANUFACTURER);

    receiver.disconnectReceiver();
    QVERIFY(receiver.detectedReceiver().isEmpty());
    // A specific family ignores detections and does not replace the saved manufacturer either.
    const int ublox = gpsReceiverManufacturerForType(GPSType::ublox);
    settings.baseReceiverManufacturers()->setRawValue(ublox);
    settings.useFixedBasePosition()->setRawValue(static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    QVERIFY(receiver.connectReceiver());
    harness.workers.current()->detected(GPSType::septentrio);
    QVERIFY(receiver.detectedReceiver().isEmpty());
    QCOMPARE(receiver._activeManufacturer(), ublox);
    QCOMPARE(settings.baseReceiverManufacturers()->rawValue().toInt(), ublox);
    receiver.disconnectReceiver();
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
        QCOMPARE(receiver._activeManufacturer(), gpsReceiverManufacturerForType(GPSType::ublox));
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
    // Fixed-mode receivers report only a time fix.
    worker->position(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.selectedSource(), PositionManager::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(47.5, 8.25));
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

    GPSSurveyReport survey;
    survey.active = false;
    survey.valid = true;
    survey.position = {.latitudeDegrees = 11, .longitudeDegrees = 21, .altitudeMeters = 400};
    survey.meanAccuracyMeters = 1.5;
    worker->survey(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(11, 21));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 1.5);

    survey.meanAccuracyMeters.reset();
    worker->survey(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 2.0);

    survey.valid = false;
    survey.active = true;
    worker->survey(survey);
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
    GPSSurveyReport survey{};
    survey.valid = true;
    survey.active = true;
    survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    survey.duration = std::chrono::seconds(4294967295LL);
    survey.meanAccuracyMeters = 1.5;
    emit first->surveyInStatusUpdated(survey);
    GPSSatelliteReport satellites;
    satellites.timestampUs = 1;
    satellites.inView = 2;
    satellites.used = 7;
    emit first->satelliteInfoUpdated(satellites);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable(), TestTimeout::shortMs());
    QVERIFY(facts.valid()->rawValue().toBool());
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
    emit first->surveyInStatusUpdated(survey);
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
    QVERIFY(!facts.valid()->rawValue().toBool());
    QVERIFY(!facts.active()->rawValue().toBool());
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
    emit second->surveyInStatusUpdated(survey);
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
        stream += NMEAUtils::repairChecksum(body);
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
    QTest::newRow("ubx") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::ublox) << true
                         << QStringLiteral("u-blox (UBX)")
                         << QStringLiteral(
                                "Receiving u-blox UBX data without position messages. Enable NAV-PVT or "
                                "NMEA output.");
    QTest::newRow("sbf") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::septentrio) << true
                         << QStringLiteral("Septentrio (SBF)")
                         << QStringLiteral(
                                "Receiving Septentrio SBF data without position messages. Enable "
                                "PVTGeodetic or NMEA output.");
    QTest::newRow("nmea") << GPSInputProblem::NoPositions << static_cast<int>(GPSType::passive) << true
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

#ifndef QGC_NO_SERIAL_LINK

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
    ScriptedGPSReceiver harness(nullptr, {.serialPorts = &serial.manager});
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
    // The picker shows the device's name and path, and stores the path.
    QCOMPARE(entries(&GPSSerialPortEntry::label), (QStringList{
                                                      QStringLiteral("u-blox GNSS receiver – /dev/ttyACM0"),
                                                      QStringLiteral("CubeOrange+ – /dev/ttyACM1 (in use)"),
                                                      QStringLiteral("STLINK-V3 – /dev/ttyACM2"),
                                                      QStringLiteral("SiK Radio – /dev/ttyUSB0"),
                                                      QStringLiteral("/dev/ttyS0"),
                                                  }));
    QCOMPARE(entries(&GPSSerialPortEntry::value).first(), QStringLiteral("/dev/ttyACM0"));

    // The mark follows the vehicle link at the next scan; the receiver's own claim never marks its port, even while
    // a disconnected session's worker still holds it.
    vehicleLink.reset();
    receiver.disconnectReceiver();
    serial.manager.rescan();
    QCOMPARE(entries(&GPSSerialPortEntry::label).at(1), QStringLiteral("CubeOrange+ – /dev/ttyACM1"));
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/dev/ttyACM0")));
    QCOMPARE(entries(&GPSSerialPortEntry::label).first(), QStringLiteral("u-blox GNSS receiver – /dev/ttyACM0"));
    harness.workers.finishRetired();
    QVERIFY(!serial.manager.isPortReserved(QStringLiteral("/dev/ttyACM0")));
}

void GPSReceiverTest::_serialReservationSurvivesDelayedStop()
{
    TestSerialPorts serial;
    GPSReceiver receiver;
    BlockedTransportGate gate;
    receiver.setConfiguration(receiverConfiguration(kPassiveManufacturer));
    receiver._serialTransportFactory = [factory = gate.factory()](const QString&, GPSCancelToken cancelToken) {
        return factory(std::move(cancelToken));
    };
    QVERIFY(receiver.connectSerial(QStringLiteral("/test/selected"), GPSType::passive, 115200, false,
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
    transport->setBaudrateHandler([state](unsigned baud) -> std::optional<bool> {
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
    GPSReceiver receiver(nullptr, nullptr, {.serialPorts = &serial.manager});
    auto configuration = serialConfiguration(kPassiveManufacturer, QStringLiteral("/test/passive"), baud);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    receiver._serialTransportFactory = [state](const QString&, GPSCancelToken cancelToken) {
        return makePassiveTestTransport(std::move(cancelToken), state);
    };
    const auto releaseWorker = qScopeGuard([&] { state->releaseRead.release(); });
    // The user's Connect claims the port through the application's serial ports.
    QVERIFY(receiver.connectReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.facts()->telemetryAvailable() && state->reading.available() > 0,
                             TestTimeout::mediumMs());
    QCOMPARE(state->baud.load(), baud);
    QCOMPARE(state->baudChanges.load(), 1U);
    QCOMPARE(state->writes.load(), 0U);
    QVERIFY(!receiver.facts()->active()->rawValue().toBool());
    QVERIFY(!receiver.facts()->valid()->rawValue().toBool());
    QVERIFY(serial.manager.isPortReserved(QStringLiteral("/test/passive")));
    state->releaseRead.release();
    receiver.disconnectReceiver();
    QVERIFY(!receiver.hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(serial.manager.canReservePort(QStringLiteral("/test/passive")), TestTimeout::mediumMs());
}
#endif
