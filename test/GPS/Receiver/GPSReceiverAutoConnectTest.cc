#include "GPSReceiverAutoConnectTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "GPSConnectionConfig.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverSession.h"
#include "GPSReplayScheduler.h"
#include "GPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

namespace {
#ifndef QGC_NO_SERIAL_LINK
GPSConnectionConfig serialConfig(const QString& device = {})
{
    GPSConnectionConfig config;
    config.device = device;
    config.receiver.role = GPSReceiverConfig::Role::Position;
    return config;
}

GPSProvider::TransportFactory failedOpenFactory()
{
    return [](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>(); };
}

GPSProvider::TransportFactory waitingFactory()
{
    return [](const std::atomic_bool& stop) -> std::unique_ptr<GPSTransport> {
        while (!stop) {
            QThread::msleep(1);
        }
        return {};
    };
}

QList<SerialPortManager::Port> receiverInventory()
{
    return {{QStringLiteral("/test/rtk"), QStringLiteral("rtk"), QGCSerialPortInfo::BoardTypeRTKGPS,
             QStringLiteral("u-blox")}};
}
#endif
}  // namespace

void GPSReceiverAutoConnectTest::_receiverErrorDetailReachesStatus()
{
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy changes(&controller, &GPSReceiverAutoConnect::stateChanged);
    QSignalSpy failed(&receiver, &GPSReceiverSession::connectionError);
    receiver.start(GPSConnectionConfig{.receiverType = GPSType::u_blox, .receiver = {}}.profile(), {});
    QTRY_VERIFY_WITH_TIMEOUT(!failed.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(!controller.errorDetail().isEmpty());
    QCOMPARE(controller.property("errorDetail").toString(), receiver.errorDetail());
    QVERIFY(!changes.isEmpty());
}

#ifndef QGC_NO_SERIAL_LINK
void GPSReceiverAutoConnectTest::_nmeaDiscoveryExclusionDoesNotRevokeReceiver()
{
    SerialPortManager ports(nullptr, receiverInventory);
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    controller.setProfile(serialConfig().profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return waitingFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy connects(&controller, &GPSReceiverAutoConnect::connectRequested);
    QSignalSpy disconnects(&controller, &GPSReceiverAutoConnect::disconnectRequested);
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 1);
    QVERIFY(receiver.hasReceiver());
    const auto exclusion = ports.excludeFromAutoConnect(QStringLiteral("/test/rtk"));
    controller.update();
    QVERIFY(receiver.hasReceiver());
    QVERIFY(disconnects.isEmpty());
    controller.disconnectSelected();
    QCOMPARE(disconnects.size(), 1);
    QVERIFY(receiver.stopping());
}

void GPSReceiverAutoConnectTest::_serialRetriesKeepConfiguration()
{
    SerialPortManager ports(nullptr, receiverInventory);
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto& state = controller._connection;
    auto config = serialConfig(QStringLiteral("/test/rtk"));
    config.receiver.role = GPSReceiverConfig::Role::RTKBase;
    config.receiver.base.surveyInAccMeters = 1.0;
    config.receiver.base.surveyInDurationSecs = 120;
    controller.setProfile(config.profile());
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return failedOpenFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy attempts(&controller, &GPSReceiverAutoConnect::connectRequested);
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(attempts.size(), 1);
    const auto first = qvariant_cast<GPSReceiverConfig>(attempts.at(0).at(2));
    QCOMPARE(first.base.surveyInAccMeters, 1.0);
    QCOMPARE(first.base.surveyInDurationSecs, 120);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());

    config.receiver.base.surveyInAccMeters = 3.0;
    config.receiver.base.surveyInDurationSecs = 240;
    controller.setProfile(config.profile());
    state._retryDeadlineMs = 0;
    controller.update();
    QCOMPARE(attempts.size(), 2);
    const auto retry = qvariant_cast<GPSReceiverConfig>(attempts.at(1).at(2));
    QCOMPARE(retry.base.surveyInAccMeters, first.base.surveyInAccMeters);
    QCOMPARE(retry.base.surveyInDurationSecs, first.base.surveyInDurationSecs);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());

    const auto failedGeneration = receiver.attempt().generation;
    QCOMPARE(receiver.attempt().phase, GPSReceiverAttempt::Phase::Failed);
    controller.disconnectSelected();
    QVERIFY(controller.connectSelected());
    QCOMPARE(receiver.attempt().generation, failedGeneration);
    QCOMPARE(state.state(), GPSConnectionState::Disconnected);
    QVERIFY(state._retryDeadlineMs < 0);
    controller.update();
    QCOMPARE(attempts.size(), 3);
    const auto replacement = qvariant_cast<GPSReceiverConfig>(attempts.at(2).at(2));
    QCOMPARE(replacement.base.surveyInAccMeters, 3.0);
    QCOMPARE(replacement.base.surveyInDurationSecs, 240);
    config.receiver.role = GPSReceiverConfig::Role::Position;
    controller.setProfile(config.profile(), true);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(attempts.size(), 4);
    QCOMPARE(qvariant_cast<GPSReceiverConfig>(attempts.at(3).at(2)).role, GPSReceiverConfig::Role::Position);
}

void GPSReceiverAutoConnectTest::_manualSerialSelectionAndPause()
{
    const QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/other"), QStringLiteral("other"), QGCSerialPortInfo::BoardTypeRTKGPS,
         QStringLiteral("u-blox")},
        {QStringLiteral("/test/chosen"), QStringLiteral("chosen"), QGCSerialPortInfo::BoardTypeUnknown, {}}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto config = serialConfig(QStringLiteral("/test/chosen"));
    config.receiverType = GPSType::femto;
    config.receiverName = QStringLiteral("Femtomes");
    controller.setProfile(config.profile());
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return waitingFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy connects(&controller, &GPSReceiverAutoConnect::connectRequested);
    controller.update();
    QVERIFY(!controller.active());
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(connects.size(), 1);
    QCOMPARE(connects.first().first().toString(), QStringLiteral("/test/chosen"));
    QCOMPARE(connects.first().at(1).toString(), QStringLiteral("Femtomes"));
    controller.disconnectNetwork();
    QVERIFY(controller.active());
    controller.disconnectSelected();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    QVERIFY(!controller.active());
    controller.update();
    QCOMPARE(connects.size(), 1);
    controller.setAutoConnect(true);
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 2);
    controller.disconnectSelected();
    QVERIFY(controller.autoConnectPaused());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    controller.update();
    QCOMPARE(connects.size(), 2);
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(connects.size(), 3);
    config.transport = GPSConnectionConfig::Tcp;
    controller.setProfile(config.profile(), true);
    controller.setAutoConnect(false);
    controller.update();
    QVERIFY(!controller.active());
    QCOMPARE(connects.size(), 3);
}

void GPSReceiverAutoConnectTest::_discoveryUnplugAndDisable()
{
    auto inventory = receiverInventory();
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    controller.setProfile(serialConfig().profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return waitingFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy connects(&controller, &GPSReceiverAutoConnect::connectRequested);
    QSignalSpy disconnects(&controller, &GPSReceiverAutoConnect::disconnectRequested);
    controller.update();
    QVERIFY(connects.isEmpty());
    controller.update();
    QCOMPARE(connects.size(), 1);
    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 if (disconnects.isEmpty()) {
                                     controller.update();
                                 }
                                 return disconnects.size() == 1;
                             })(),
                             TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    inventory = receiverInventory();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 controller.update();
                                 return connects.size() == 2;
                             })(),
                             TestTimeout::mediumMs());
    controller.setAutoConnect(false);
    QCOMPARE(disconnects.size(), 2);
    controller.update();
    QCOMPARE(disconnects.size(), 2);
}

void GPSReceiverAutoConnectTest::_unplugNotificationPreservesChangedIntent_data()
{
    QTest::addColumn<QString>("action");
    QTest::newRow("disconnect") << QStringLiteral("disconnect");
    QTest::newRow("disable") << QStringLiteral("disable");
    QTest::newRow("replace-config") << QStringLiteral("replace-config");
}

void GPSReceiverAutoConnectTest::_unplugNotificationPreservesChangedIntent()
{
    QFETCH(QString, action);
    auto inventory = receiverInventory();
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto& state = controller._connection;
    const auto original = serialConfig(QStringLiteral("/test/rtk"));
    auto replacement = original;
    replacement.device = QStringLiteral("/test/replacement");
    controller.setProfile(original.profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return waitingFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy connects(&controller, &GPSReceiverAutoConnect::connectRequested);
    QSignalSpy disconnects(&controller, &GPSReceiverAutoConnect::disconnectRequested);
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 1);
    QVERIFY(receiver.hasReceiver());
    QVERIFY(controller._sessionConfig.has_value());
    QCOMPARE(controller._sessionConfig->endpoint.device, original.device);

    connect(&controller, &GPSReceiverAutoConnect::disconnectRequested, &controller, [&]() {
        if (action == QStringLiteral("disconnect")) {
            controller.disconnectSelected();
        } else if (action == QStringLiteral("disable")) {
            controller.setAutoConnect(false);
        } else {
            controller.setProfile(replacement.profile(), true);
        }
    });
    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 if (disconnects.isEmpty()) {
                                     controller.update();
                                 }
                                 return disconnects.size() == 1;
                             })(),
                             TestTimeout::mediumMs());
    QVERIFY(!controller.active());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!controller._sessionConfig.has_value());
    QVERIFY(!state.shouldConnect(false));
    QCOMPARE(controller.autoConnectPaused(), action == QStringLiteral("disconnect"));
    QCOMPARE(connects.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());

    inventory = receiverInventory();
    QTRY_VERIFY_WITH_TIMEOUT(!ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 1);
    QCOMPARE(disconnects.size(), 1);
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!state.shouldConnect(false));
    if (action == QStringLiteral("replace-config")) {
        QVERIFY(controller._sessionConfig.has_value());
        QCOMPARE(controller._sessionConfig->endpoint.device, replacement.device);
    } else {
        QVERIFY(!controller.active());
        QVERIFY(!controller._sessionConfig.has_value());
    }
}

void GPSReceiverAutoConnectTest::_excludedPorts_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason : {"bootloader", "composite", "busy", "nmea", "single-port", "other-board"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void GPSReceiverAutoConnectTest::_excludedPorts()
{
    QFETCH(QString, reason);
    auto port = receiverInventory().first();
    port.bootloader = reason == QStringLiteral("bootloader");
    port.autoConnectAllowed = reason != QStringLiteral("composite");
    if (reason == QStringLiteral("other-board")) {
        port.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    }
    SerialPortManager ports(nullptr, [&]() { return QList<SerialPortManager::Port>{port}; });
    (void) ports.availablePorts();
    SerialPortManager::ReservationPtr reservation;
    const auto exclusion = reason == QStringLiteral("nmea") ? ports.excludeFromAutoConnect(port.systemLocation)
                                                            : SerialPortManager::ReservationPtr();
    if (reason == QStringLiteral("busy")) {
        reservation = ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        ports.setSinglePortOnly(true);
        reservation = ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    controller.setProfile(serialConfig().profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return waitingFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy connects(&controller, &GPSReceiverAutoConnect::connectRequested);
    controller.update();
    controller.update();
    QVERIFY(connects.isEmpty());
}

void GPSReceiverAutoConnectTest::_failedAttemptsBackOffAndRespectReservations()
{
    SerialPortManager ports(nullptr, receiverInventory);
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto& state = controller._connection;
    controller.setProfile(serialConfig().profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return failedOpenFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy attempts(&controller, &GPSReceiverAutoConnect::connectRequested);
    controller.update();
    controller.update();
    QCOMPARE(attempts.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
    controller.update();
    QCOMPARE(attempts.size(), 1);
    QCOMPARE(state._retryDelayMs, 1000);
    auto claim = ports.reservePort(QStringLiteral("/test/rtk"));
    QVERIFY(claim);
    state._retryDeadlineMs = 0;
    controller.update();
    QCOMPARE(attempts.size(), 1);
    claim.reset();
    for (const int delay : {2000, 4000, 8000, 16000, 30000, 30000}) {
        state._retryDeadlineMs = 0;
        const auto previousAttempts = attempts.size();
        controller.update();
        QCOMPARE(attempts.size(), previousAttempts + 1);
        QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
        QCOMPARE(state._retryDelayMs, delay);
    }
    const auto previousAttempts = attempts.size();
    controller.setAutoConnect(false);
    controller.update();
    QCOMPARE(state._retryDelayMs, 1000);
    QVERIFY(state._retryDeadlineMs < 0);
    QCOMPARE(attempts.size(), previousAttempts);
}

void GPSReceiverAutoConnectTest::_failedOpenRetriesWithoutUnplug()
{
    SerialPortManager ports(nullptr, receiverInventory);
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto& state = controller._connection;
    controller.setProfile(serialConfig().profile());
    controller.setAutoConnect(true);
    controller.setSerialDiscovery(&ports);
    controller.setSerialTransportFactory([](const QString&) { return failedOpenFactory(); });
    controller._connectDelayMs = 0;
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QSignalSpy attempts(&controller, &GPSReceiverAutoConnect::connectRequested);
    controller.update();
    controller.update();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
    QCOMPARE(attempts.size(), 1);
    state._retryDeadlineMs = 0;
    controller.update();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
    QCOMPARE(attempts.size(), 2);
    const auto exclusion = ports.excludeFromAutoConnect(QStringLiteral("/test/rtk"));
    state._retryDeadlineMs = 0;
    controller.update();
    QCOMPARE(attempts.size(), 2);
}
#endif

void GPSReceiverAutoConnectTest::_networkRetriesAndStops()
{
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    auto& state = controller._connection;
    QSignalSpy active(&controller, &GPSReceiverAutoConnect::networkActiveChanged);
    QSignalSpy disconnects(&controller, &GPSReceiverAutoConnect::disconnectRequested);
    QVERIFY(!controller.connectNetwork(GPSType::u_blox, {}));
    std::atomic_int attempts = 0;
    const GPSProvider::TransportFactory factory = [&](const std::atomic_bool&) {
        ++attempts;
        return std::unique_ptr<GPSTransport>();
    };
    const auto cleanup = qScopeGuard([&]() { receiver.shutdown(); });
    QVERIFY(controller.connectNetwork(GPSType::u_blox, factory));
    QVERIFY(controller.networkActive());
    QVERIFY(!controller.connectNetwork(GPSType::u_blox, factory));
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
    QCOMPARE(attempts.load(), 1);
    controller.update();
    QCOMPARE(attempts.load(), 1);
    QCOMPARE(state._retryDelayMs, 1000);
    for (const int delay : {2000, 4000, 8000, 16000, 30000, 30000}) {
        state._retryDeadlineMs = 0;
        const int previousAttempts = attempts.load();
        controller.update();
        QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
        QCOMPARE(attempts.load(), previousAttempts + 1);
        QCOMPARE(state._retryDelayMs, delay);
    }
    controller.disconnectNetwork();
    QVERIFY(!controller.networkActive());
    QCOMPARE(active.size(), 2);
    QCOMPARE(disconnects.size(), 1);
    QCOMPARE(state._retryDelayMs, 1000);
    QVERIFY(state._retryDeadlineMs < 0);
    const int previousAttempts = attempts.load();
    controller.update();
    controller.stop();
    QCOMPARE(attempts.load(), previousAttempts);
    QCOMPARE(disconnects.size(), 1);
}

void GPSReceiverAutoConnectTest::_disconnectDoesNotBlockAndReconnectWaits()
{
    struct Gate
    {
        QSemaphore entered;
        QSemaphore release;
        std::atomic_int attempts = 0;
        std::atomic_bool cancelled = false;
    };

    auto gate = std::make_shared<Gate>();
    GPSReceiverSession receiver;
    GPSReceiverAutoConnect controller(&receiver);
    const auto cleanup = qScopeGuard([&]() {
        receiver.stop();
        gate->release.release();
        receiver.shutdown();
    });
    const GPSProvider::TransportFactory factory = [gate](const std::atomic_bool& stop) {
        if (++gate->attempts == 1) {
            gate->entered.release();
            gate->release.acquire();
            gate->cancelled = stop.load();
        }
        return std::unique_ptr<GPSTransport>();
    };
    QVERIFY(controller.connectNetwork(GPSType::u_blox, factory));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QElapsedTimer elapsed;
    elapsed.start();
    controller.disconnectNetwork();
    QVERIFY(elapsed.elapsed() < 1000);
    QVERIFY(receiver.stopping());
    QVERIFY(!controller.active());
    QCOMPARE(controller.connectionState(), GPSConnectionState::Stopping);
    QVERIFY(controller.connectNetwork(GPSType::u_blox, factory));
    controller.update();
    QVERIFY(controller.active());
    QCOMPARE(gate->attempts.load(), 1);
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    QVERIFY(gate->cancelled);
    controller.update();
    QTRY_COMPARE_WITH_TIMEOUT(gate->attempts.load(), 2, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver() && !receiver.stopping(), TestTimeout::mediumMs());
    QCOMPARE(controller.connectionState(), GPSConnectionState::Retrying);
}

UT_REGISTER_TEST(GPSReceiverAutoConnectTest, TestLabel::Unit)

void GPSReceiverAutoConnectTest::_invalidManualAttemptRecoversAutomatically()
{
    GPSReceiverSession session;
    GPSReceiverAutoConnect controller(&session);
    const auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        release->release();
        session.shutdown();
    });
    GPSConnectionConfig config;
    config.transport = GPSConnectionConfig::Tcp;
    config.receiver.role = GPSReceiverConfig::Role::Position;
    controller.setProfile(config.profile());
    QVERIFY(!controller.connectSelected());
    QVERIFY(!controller.property("validationError").toString().isEmpty());

    config.host = QStringLiteral("localhost");
    config.port = 2101;
    const auto profile = config.profile();
    controller.setProfile(profile, true);
    QVERIFY(controller.property("validationError").toString().isEmpty());
    controller.setAutoConnect(true);
    controller._sessionConfig = profile;
    controller._transportFactory = [release](const std::atomic_bool&) {
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    };
    controller.update();
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    emit worker->receiverReady();
    QTRY_COMPARE_WITH_TIMEOUT(controller.connectionState(), GPSConnectionState::Ready, TestTimeout::mediumMs());
    QVERIFY(controller.active());
    QVERIFY(controller.errorDetail().isEmpty());
    QVERIFY(controller.validationError().isEmpty());
}

void GPSReceiverAutoConnectTest::_restartDuringStopKeepsNewIntent()
{
    GPSReceiverSession session;
    GPSReceiverAutoConnect controller(&session);
    const auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        controller.stop();
        release->release();
        session.shutdown();
    });
    const GPSProvider::TransportFactory blocked = [release](const std::atomic_bool&) {
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    };
    QVERIFY(controller.connectNetwork(GPSType::u_blox, blocked));
    bool restarted = false;
    const auto restart = connect(&controller, &GPSReceiverAutoConnect::stateChanged, &controller, [&]() {
        if (!controller.active() && !restarted) {
            restarted = true;
            controller.stop();
            QVERIFY(controller.connectNetwork(GPSType::u_blox, blocked));
        }
    });
    controller.stop();
    disconnect(restart);
    QVERIFY(restarted);
    QVERIFY(controller.active());
    QVERIFY(controller.networkActive());
    QVERIFY(controller._sessionConfig.has_value());
    QVERIFY(controller._transportFactory);
}

void GPSReceiverAutoConnectTest::_receiverCanDisappearDuringStopping()
{
    auto session = std::make_unique<GPSReceiverSession>();
    GPSReceiverAutoConnect controller(session.get());
    const auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        release->release();
        if (session) {
            session->shutdown();
        }
    });
    const GPSProvider::TransportFactory blocked = [release](const std::atomic_bool&) {
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    };
    QVERIFY(controller.connectNetwork(GPSType::u_blox, blocked));
    bool deleted = false;
    connect(&controller, &GPSReceiverAutoConnect::stateChanged, &controller, [&]() {
        if (!deleted && controller.connectionState() == GPSConnectionState::Stopping) {
            deleted = true;
            release->release();
            session->shutdown();
            session.reset();
        }
    });
    controller.stop();
    QVERIFY(deleted);
    QVERIFY(!session);
    QVERIFY(!controller.active());
    QVERIFY(!controller.networkActive());
}

void GPSReceiverAutoConnectTest::_retryRunsWithoutPolling()
{
    GPSReplayScheduler scheduler;
    GPSReceiverSession session;
    GPSReceiverAutoConnect controller(&session, nullptr, nullptr, &scheduler);
    GPSConnectionConfig config;
    config.transport = GPSConnectionConfig::Tcp;
    config.host = QStringLiteral("localhost");
    config.port = 2101;
    config.receiver.role = GPSReceiverConfig::Role::Position;
    controller.setProfile(config.profile());
    std::atomic_int attempts = 0;
    const auto cleanup = qScopeGuard([&]() {
        controller.stop();
        session.shutdown();
    });
    QVERIFY(controller.connectReceiver(config.profile(), [&attempts](const std::atomic_bool&) {
        ++attempts;
        return std::unique_ptr<GPSTransport>();
    }));
    QTRY_COMPARE_WITH_TIMEOUT(controller.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    QCOMPARE(attempts.load(), 1);
    controller.setSuspended(true);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    QCOMPARE(attempts.load(), 1);
    controller.setSuspended(false);
    QVERIFY(scheduler.advanceBy(std::chrono::microseconds(0)));
    QTRY_COMPARE_WITH_TIMEOUT(attempts.load(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(controller.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    controller.disconnectSelected();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(60)));
    QCOMPARE(attempts.load(), 2);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void GPSReceiverAutoConnectTest::_suspensionDuringAdmissionDefersStart()
{
    GPSReplayScheduler scheduler;
    GPSReceiverSession session;
    GPSReceiverAutoConnect controller(&session, nullptr, nullptr, &scheduler);
    std::atomic_int attempts = 0;
    const auto cleanup = qScopeGuard([&]() {
        controller.stop();
        session.shutdown();
    });
    bool suspended = false;
    const auto notification = connect(&controller, &GPSReceiverAutoConnect::stateChanged, &controller, [&]() {
        if (!suspended && controller.connectionState() == GPSConnectionState::Connecting) {
            suspended = true;
            controller.setSuspended(true);
        }
    });
    controller.connectNetwork(GPSType::u_blox, [&attempts](const std::atomic_bool&) {
        ++attempts;
        return std::unique_ptr<GPSTransport>();
    });
    QVERIFY(suspended);
    QVERIFY(!session.hasReceiver());
    QCOMPARE(attempts.load(), 0);
    disconnect(notification);
    controller.setSuspended(false);
    QVERIFY(scheduler.advanceBy(std::chrono::microseconds(0)));
    QTRY_COMPARE_WITH_TIMEOUT(attempts.load(), 1, TestTimeout::mediumMs());
}
