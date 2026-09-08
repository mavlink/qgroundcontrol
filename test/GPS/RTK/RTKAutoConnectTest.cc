#include "RTKAutoConnectTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "RTKAutoConnect.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

#ifndef QGC_NO_SERIAL_LINK
void RTKAutoConnectTest::_serialRetriesKeepConfiguration()
{
    expectLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* rtk = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), false);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Serial);
    saved.setFactValue(rtk->serialDevice(), QStringLiteral("/test/rtk"));
    saved.setFactValue(rtk->useFixedBasePosition(), 0);
    saved.setFactValue(rtk->surveyInAccuracyLimit(), 1.0);
    saved.setFactValue(rtk->surveyInMinObservationDuration(), 120);
    SerialPortManager ports(nullptr, []() {
        return QList<SerialPortManager::Port>{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                               QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    });
    GPSRtk receiver;
    RTKAutoConnect controller(&receiver, settings, rtk);
    controller.setSerialDiscovery(&ports);
    controller._connectDelayMs = 0;
    QSignalSpy attempts(&controller, &RTKAutoConnect::connectRequested);
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(attempts.size(), 1);
    const auto first = qvariant_cast<GPSReceiverConfig>(attempts.at(0).at(2));
    QCOMPARE(first.surveyInAccMeters, 1.0);
    QCOMPARE(first.surveyInDurationSecs, 120);

    rtk->surveyInAccuracyLimit()->setRawValue(3.0);
    rtk->surveyInMinObservationDuration()->setRawValue(240);
    controller.update();
    controller._connection._retryDeadline.setRemainingTime(0);
    controller.update();
    QCOMPARE(attempts.size(), 2);
    const auto retry = qvariant_cast<GPSReceiverConfig>(attempts.at(1).at(2));
    QCOMPARE(retry.surveyInAccMeters, 1.0);
    QCOMPARE(retry.surveyInDurationSecs, 120);

    controller.disconnectSelected();
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(attempts.size(), 3);
    const auto replacement = qvariant_cast<GPSReceiverConfig>(attempts.at(2).at(2));
    QCOMPARE(replacement.surveyInAccMeters, 3.0);
    QCOMPARE(replacement.surveyInDurationSecs, 240);
    verifyExpectedLogMessage();
}

void RTKAutoConnectTest::_manualSerialSelectionAndPause()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* rtkSettings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), false);
    saved.setFactValue(settings->autoConnectNetworkRTKGPS(), false);
    saved.setFactValue(rtkSettings->connectionType(), RTKSettings::Serial);
    saved.setFactValue(rtkSettings->serialDevice(), QStringLiteral("/test/chosen"));
    saved.setFactValue(rtkSettings->networkReceiverType(), 3);
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/other"), QStringLiteral("other"), QGCSerialPortInfo::BoardTypeRTKGPS,
         QStringLiteral("u-blox")},
        {QStringLiteral("/test/chosen"), QStringLiteral("chosen"), QGCSerialPortInfo::BoardTypeUnknown, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSRtk receiver;
    RTKAutoConnect controller(&receiver, settings, rtkSettings);
    controller.setSerialDiscovery(&ports);
    controller._connectDelayMs = 0;
    QSignalSpy connects(&controller, &RTKAutoConnect::connectRequested);
    QSignalSpy disconnects(&controller, &RTKAutoConnect::disconnectRequested);
    controller.update();
    QVERIFY(!controller.active());
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(connects.size(), 1);
    QCOMPARE(connects.first().first().toString(), QStringLiteral("/test/chosen"));
    QCOMPARE(connects.first().at(1).toString(), QStringLiteral("Femtomes"));
    controller.disconnectNetwork();
    QVERIFY(controller.active());
    auto ownReservation = ports.reservePort(QStringLiteral("/test/chosen"));
    QVERIFY(ownReservation);
    controller.update();
    QCOMPARE(disconnects.size(), 0);
    ownReservation.reset();
    auto nmeaExclusion = ports.excludeFromAutoConnect(QStringLiteral("/test/chosen"));
    controller.update();
    QCOMPARE(disconnects.size(), 1);
    QCOMPARE(connects.size(), 1);
    QVERIFY(controller.active());
    nmeaExclusion.reset();
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 2);
    controller.disconnectSelected();
    QVERIFY(!controller.active());
    controller.update();
    QCOMPARE(connects.size(), 2);
    settings->autoConnectRTKGPS()->setRawValue(true);
    controller.update();
    controller.update();
    QCOMPARE(connects.size(), 3);
    controller.disconnectSelected();
    QVERIFY(controller.autoConnectPaused());
    controller.update();
    QCOMPARE(connects.size(), 3);
    QVERIFY(controller.connectSelected());
    controller.update();
    QCOMPARE(connects.size(), 4);
    rtkSettings->connectionType()->setRawValue(RTKSettings::Tcp);
    controller.update();
    QVERIFY(!controller.active());
    QCOMPARE(connects.size(), 4);
}

void RTKAutoConnectTest::_discoveryUnplugAndDisable()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    QList<SerialPortManager::Port> inventory{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                              QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    QSignalSpy disconnects(&discovery, &RTKAutoConnect::disconnectRequested);
    discovery.update();
    QCOMPARE(connects.count(), 0);
    discovery.update();
    QCOMPARE(connects.count(), 1);
    QCOMPARE(connects.first().first().toString(), QStringLiteral("/test/rtk"));
    discovery.update();
    QCOMPARE(connects.count(), 1);
    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 discovery.update();
                                 return disconnects.count() == 1;
                             })(),
                             TestTimeout::mediumMs());
    inventory.append({QStringLiteral("/test/rtk"), QStringLiteral("rtk"), QGCSerialPortInfo::BoardTypeRTKGPS,
                      QStringLiteral("u-blox")});
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 discovery.update();
                                 return connects.count() == 2;
                             })(),
                             TestTimeout::mediumMs());
    settings->autoConnectRTKGPS()->setRawValue(false);
    discovery.update();
    QCOMPARE(disconnects.count(), 2);
    discovery.update();
    QCOMPARE(disconnects.count(), 2);
}

void RTKAutoConnectTest::_excludedPorts_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason : {"bootloader", "composite", "busy", "nmea", "single-port", "other-board"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void RTKAutoConnectTest::_excludedPorts()
{
    QFETCH(QString, reason);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), reason == QStringLiteral("nmea")
                                                   ? AutoConnectSettings::NmeaSourceSerial
                                                   : AutoConnectSettings::NmeaSourceDisabled);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/rtk"));
    SerialPortManager::Port port{QStringLiteral("/test/rtk"), QStringLiteral("rtk"), QGCSerialPortInfo::BoardTypeRTKGPS,
                                 QStringLiteral("u-blox")};
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
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.count(), 0);
}


void RTKAutoConnectTest::_failedAttemptsBackOffAndRespectReservations()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    QList<SerialPortManager::Port> inventory{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                              QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.size(), 1);
    discovery.update();
    QCOMPARE(connects.size(), 1);
    QCOMPARE(discovery._connection._retryDelayMs, 1000);
    QVERIFY(!discovery._connection._retryDeadline.isForever());
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 discovery.update();
                                 return connects.size() == 2;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(discovery._connection._retryDelayMs, 2000);
    discovery.update();
    QCOMPARE(connects.size(), 2);

    auto claim = ports.reservePort(QStringLiteral("/test/rtk"));
    QVERIFY(claim);
    discovery._connection._retryDeadline.setRemainingTime(0);
    discovery.update();
    QCOMPARE(connects.size(), 2);
    claim.reset();
    discovery.update();
    QCOMPARE(connects.size(), 3);
    for (const int delay : {8000, 16000, 30000, 30000}) {
        discovery.update();
        QCOMPARE(discovery.connectionState(), GPSConnectionState::Retrying);
        discovery._connection._retryDeadline.setRemainingTime(0);
        const auto attempts = connects.size();
        discovery.update();
        QCOMPARE(connects.size(), attempts + 1);
        QCOMPARE(discovery._connection._retryDelayMs, delay);
    }
    const auto attemptsBeforeDisable = connects.size();
    settings->autoConnectRTKGPS()->setRawValue(false);
    discovery.update();
    QCOMPARE(discovery._connection._retryDelayMs, 1000);
    QVERIFY(discovery._connection._retryDeadline.isForever());
    QVERIFY(discovery._autoConnectedPort.isEmpty());
    QCOMPARE(connects.size(), attemptsBeforeDisable);
}

void RTKAutoConnectTest::_failedOpenRetriesWithoutUnplug()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    QList<SerialPortManager::Port> inventory{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                              QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy attempts(&discovery, &RTKAutoConnect::connectRequested);
    connect(&discovery, &RTKAutoConnect::connectRequested, &receiver,
            [&]() { receiver.connectReceiver(GPSType::u_blox, {}, {}); });
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    discovery.update();
    discovery.update();
    QCOMPARE(attempts.size(), 1);
    QVERIFY(receiver.hasReceiver());
    discovery.update();
    QCOMPARE(attempts.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    discovery.update();
    QCOMPARE(attempts.size(), 1);
    discovery._connection._retryDeadline.setRemainingTime(0);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    discovery.update();
    QCOMPARE(attempts.size(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(discovery._connection._retryDelayMs, 2000);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/rtk"));
    const auto nmeaExclusion = ports.excludeFromAutoConnect(QStringLiteral("/test/rtk"));
    discovery._connection._retryDeadline.setRemainingTime(0);
    discovery.update();
    QCOMPARE(attempts.size(), 2);
    QVERIFY(discovery._autoConnectedPort.isEmpty());
}
#endif

void RTKAutoConnectTest::_networkRetriesAndStops()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    GPSRtk receiver;
    RTKAutoConnect controller(&receiver, nullptr, nullptr);
    QSignalSpy active(&controller, &RTKAutoConnect::networkActiveChanged);
    QSignalSpy disconnects(&controller, &RTKAutoConnect::disconnectRequested);
    QVERIFY(!controller.connectNetwork(GPSType::u_blox, {}));
    QVERIFY(!controller.networkActive());
    std::atomic_int attempts = 0;
    const GPSProvider::TransportFactory factory = [&](const std::atomic_bool&) {
        ++attempts;
        return std::unique_ptr<GPSTransport>();
    };
    const auto expectFailure = [this]() {
        expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    };
    expectFailure();
    QVERIFY(controller.connectNetwork(GPSType::u_blox, factory));
    QVERIFY(controller.networkActive());
    QVERIFY(!controller.connectNetwork(GPSType::u_blox, factory));
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(attempts.load(), 1);
    controller.update();
    QCOMPARE(attempts.load(), 1);
    QCOMPARE(controller._connection._retryDelayMs, 1000);
    QVERIFY(!controller._connection._retryDeadline.isForever());

    for (const int delay : {2000, 4000, 8000, 16000, 30000, 30000}) {
        controller._connection._retryDeadline.setRemainingTime(0);
        const int previousAttempts = attempts.load();
        expectFailure();
        controller.update();
        QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QCOMPARE(attempts.load(), previousAttempts + 1);
        QCOMPARE(controller._connection._retryDelayMs, delay);
    }
    controller.disconnectNetwork();
    QVERIFY(!controller.networkActive());
    QCOMPARE(active.size(), 2);
    QCOMPARE(disconnects.size(), 1);
    QCOMPARE(controller._connection._retryDelayMs, 1000);
    QVERIFY(controller._connection._retryDeadline.isForever());
    const int attemptsBeforeStop = attempts.load();
    controller.update();
    controller.stop();
    QCOMPARE(attempts.load(), attemptsBeforeStop);
    QCOMPARE(disconnects.size(), 1);
}

UT_REGISTER_TEST(RTKAutoConnectTest, TestLabel::Unit)

void RTKAutoConnectTest::_disconnectDoesNotBlockAndReconnectWaits()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());

    struct Gate
    {
        QSemaphore entered;
        QSemaphore release;
        std::atomic_int attempts = 0;
        std::atomic_bool cancelled = false;
    };

    auto gate = std::make_shared<Gate>();
    const auto unblock = qScopeGuard([&]() { gate->release.release(); });
    GPSRtk receiver;
    RTKAutoConnect controller(&receiver, nullptr, nullptr);
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
    QCOMPARE(controller.connectionState(), GPSConnectionState::Stopping);
    QCOMPARE(gate->attempts.load(), 1);
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.stopping(), TestTimeout::mediumMs());
    QVERIFY(gate->cancelled);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    controller.update();
    QTRY_COMPARE_WITH_TIMEOUT(gate->attempts.load(), 2, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(controller.connectionState(), GPSConnectionState::Retrying);
}
