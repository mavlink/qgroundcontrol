#include "RTKAutoConnectTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "NMEASourceManager.h"
#include "NTRIPManager.h"
#include "PositionManager.h"
#include "RTKAutoConnect.h"
#include "RTKSettings.h"
#include "SerialPortManager.h"
#include "SettingsManager.h"

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
    for (const auto* reason : {"bootloader", "busy", "nmea", "single-port", "other-board"}) {
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
    if (reason == QStringLiteral("other-board")) {
        port.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    }
    SerialPortManager ports(nullptr, [&]() { return QList<SerialPortManager::Port>{port}; });
    (void) ports.availablePorts();
    SerialPortManager::ReservationPtr reservation;
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

UT_REGISTER_TEST(RTKAutoConnectTest, TestLabel::Unit)

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
    QCOMPARE(discovery._retryDelayMs, 1000);
    QVERIFY(!discovery._retryDeadline.isForever());
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 discovery.update();
                                 return connects.size() == 2;
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(discovery._retryDelayMs, 2000);
    discovery.update();
    QCOMPARE(connects.size(), 2);

    auto claim = ports.reservePort(QStringLiteral("/test/rtk"));
    QVERIFY(claim);
    discovery._retryDeadline.setRemainingTime(0);
    discovery.update();
    QCOMPARE(connects.size(), 2);
    claim.reset();
    discovery.update();
    QCOMPARE(connects.size(), 3);
    for (const int delay : {8000, 16000, 30000, 30000}) {
        discovery._retryDeadline.setRemainingTime(0);
        const auto attempts = connects.size();
        discovery.update();
        QCOMPARE(connects.size(), attempts + 1);
        QCOMPARE(discovery._retryDelayMs, delay);
    }
    const auto attemptsBeforeDisable = connects.size();
    settings->autoConnectRTKGPS()->setRawValue(false);
    discovery.update();
    QCOMPARE(discovery._retryDelayMs, 1000);
    QVERIFY(discovery._retryDeadline.isForever());
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
            [&]() { receiver.connectReceiver(GPSType::ublox, {}); });
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
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
    discovery._retryDeadline.setRemainingTime(0);
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    discovery.update();
    QCOMPARE(attempts.size(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(discovery._retryDelayMs, 2000);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/rtk"));
    discovery._retryDeadline.setRemainingTime(0);
    discovery.update();
    QCOMPARE(attempts.size(), 2);
    QVERIFY(discovery._autoConnectedPort.isEmpty());
}

void RTKAutoConnectTest::_compositeReceiverSelection_data()
{
    QTest::addColumn<QString>("scenario");
    QTest::addColumn<bool>("connectSecond");
    QTest::newRow("busy-primary-blocks-duplicate") << QStringLiteral("duplicate") << false;
    QTest::newRow("nmea-interface-remains-eligible") << QStringLiteral("nmea-label") << true;
    QTest::newRow("mavlink-sibling") << QStringLiteral("mavlink-sibling") << true;
    QTest::newRow("bootloader-sibling") << QStringLiteral("bootloader") << true;
    QTest::newRow("explicit-nmea-sibling") << QStringLiteral("nmea-source") << true;
    QTest::newRow("unknown-identities") << QStringLiteral("unknown") << true;
    QTest::newRow("distinct-devices") << QStringLiteral("distinct") << true;
}

void RTKAutoConnectTest::_compositeReceiverSelection()
{
    QFETCH(QString, scenario);
    QFETCH(bool, connectSecond);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), scenario == QStringLiteral("nmea-source")
                                                   ? AutoConnectSettings::NmeaSourceSerial
                                                   : AutoConnectSettings::NmeaSourceDisabled);
    SerialPortManager::Port first{QStringLiteral("/test/receiver-first"), QStringLiteral("first"),
                                  QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")};
    first.physicalDeviceId = QStringLiteral("1:2:serial");
    auto second = first;
    second.systemLocation = QStringLiteral("/test/receiver-second");
    second.portName = QStringLiteral("second");
    if (scenario == QStringLiteral("nmea-label")) {
        second.description = QStringLiteral("NMEA interface");
    } else if (scenario == QStringLiteral("mavlink-sibling")) {
        first.boardType = QGCSerialPortInfo::BoardTypePixhawk;
    } else if (scenario == QStringLiteral("bootloader")) {
        first.bootloader = true;
    } else if (scenario == QStringLiteral("unknown")) {
        first.physicalDeviceId.clear();
        second.physicalDeviceId.clear();
    } else if (scenario == QStringLiteral("distinct")) {
        second.physicalDeviceId = QStringLiteral("1:2:other");
    }
    saved.setFactValue(settings->autoConnectNmeaPort(), first.systemLocation);
    SerialPortManager ports(nullptr, [&] { return QList<SerialPortManager::Port>{first, second}; });
    auto claim = ports.reservePort(first.systemLocation);
    QVERIFY(claim);
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.size(), connectSecond ? 1 : 0);
    if (connectSecond) {
        QCOMPARE(connects.first().first().toString(), second.systemLocation);
        discovery._retryDeadline.setRemainingTime(0);
        discovery.update();
        QCOMPARE(connects.size(), 2);
        QCOMPARE(connects.last().first().toString(), second.systemLocation);
    }
}

void RTKAutoConnectTest::_genericUsbNeedsExplicitSelection_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("unicore") << 5;
    QTest::newRow("quectel") << 6;
    QTest::newRow("passive") << 7;
}

void RTKAutoConnectTest::_genericUsbNeedsExplicitSelection()
{
    QFETCH(int, manufacturer);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* rtkSettings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    saved.setFactValue(rtkSettings->baseReceiverManufacturers(), manufacturer);
    saved.setFactValue(rtkSettings->serialDevice(), QStringLiteral("/test/ch340"));
    const QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/ch340"), QStringLiteral("ch340"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("CH340")},
        {QStringLiteral("/test/ftdi"), QStringLiteral("ftdi"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("FTDI")},
        {QStringLiteral("/test/known"), QStringLiteral("known"), QGCSerialPortInfo::BoardTypeRTKGPS,
         QStringLiteral("u-blox")},
    };
    SerialPortManager ports(nullptr, [&] { return inventory; });
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.size(), 1);
    QCOMPARE(connects.first().first().toString(), QStringLiteral("/test/known"));
    QVERIFY(ports.canReservePort(QStringLiteral("/test/ch340")));
    QVERIFY(ports.canReservePort(QStringLiteral("/test/ftdi")));
}

void RTKAutoConnectTest::_manualConnectionRetiresAutoOwnership()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{{QStringLiteral("/test/known"), QStringLiteral("known"),
                                               QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    });
    GPSRtk receiver;
    RTKAutoConnect discovery(settings, &receiver, &ports);
    discovery._connectDelayMs = 0;
    QSignalSpy connects(&discovery, &RTKAutoConnect::connectRequested);
    QSignalSpy disconnects(&discovery, &RTKAutoConnect::disconnectRequested);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.size(), 1);
    receiver.disconnectConfiguredGPS();
    discovery.update();
    discovery.stop();
    QCOMPARE(disconnects.size(), 0);
    QCOMPARE(connects.size(), 1);
    QVERIFY(!settings->autoConnectRTKGPS()->rawValue().toBool());
    settings->autoConnectRTKGPS()->setRawValue(true);
    discovery.update();
    discovery.update();
    QCOMPARE(connects.size(), 2);
}

void RTKAutoConnectTest::_notificationSupersedesDiscovery_data()
{
    QTest::addColumn<bool>("disconnecting");
    QTest::addColumn<QString>("action");
    for (const bool disconnecting : {false, true}) {
        for (const auto* action : {"delete", "stop", "update"}) {
            const QByteArray name = QByteArray(disconnecting ? "disconnect-" : "enumeration-") + action;
            QTest::newRow(name.constData()) << disconnecting << QString::fromLatin1(action);
        }
    }
}

void RTKAutoConnectTest::_notificationSupersedesDiscovery()
{
    QFETCH(bool, disconnecting);
    QFETCH(QString, action);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/rtk"));
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                               QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    });
    GPSRtk receiver;
    auto discovery = std::make_unique<RTKAutoConnect>(settings, &receiver, &ports);
    discovery->_connectDelayMs = 0;
    QSignalSpy connects(discovery.get(), &RTKAutoConnect::connectRequested);
    if (disconnecting) {
        discovery->update();
        discovery->update();
        QCOMPARE(connects.size(), 1);
        settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    }
    bool notified = false;
    const auto supersede = [&] {
        notified = true;
        if (action == QStringLiteral("delete")) {
            discovery.reset();
        } else if (action == QStringLiteral("stop")) {
            discovery->stop();
        } else {
            settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
            discovery->update();
        }
    };
    const auto notification = disconnecting
                                  ? connect(discovery.get(), &RTKAutoConnect::disconnectRequested, this, supersede)
                                  : connect(&ports, &SerialPortManager::portsEnumerated, this, supersede);
    discovery->update();
    disconnect(notification);
    QVERIFY(notified);
    QCOMPARE(connects.size(), disconnecting ? 1 : 0);
    if (action == QStringLiteral("delete")) {
        QVERIFY(!discovery);
    } else if (action == QStringLiteral("stop")) {
        QVERIFY(discovery->_waitingPorts.isEmpty());
    } else {
        discovery->update();
        QCOMPARE(connects.size(), disconnecting ? 2 : 1);
    }
}

void RTKAutoConnectTest::_shutdownDuringConnectionTick()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    saved.setFactValue(settings->nmeaUdpPort(), spare.localPort());
    spare.close();
    auto* applicationCorrections = GPSManager::instance()->corrections();
    const auto restoreCorrections = qScopeGuard(
        [applicationCorrections] { NTRIPManager::instance()->setCorrectionManager(applicationCorrections); });
    int enumerations = 0;
    SerialPortManager ports(nullptr, [&] {
        ++enumerations;
        return QList<SerialPortManager::Port>{};
    });
    QGCPositionManager position;
    GPSManager manager;
    manager._nmeaSources = new NMEASourceManager(settings, &position, &manager);
    manager._rtkAutoConnect = new RTKAutoConnect(settings, manager.gpsRtk(), &ports, &manager);
    connect(&position, &QGCPositionManager::nmeaSourceChanged, &manager, &GPSManager::shutdown);
    manager._updateConnections();
    QVERIFY(manager._shutdown);
    QVERIFY(!position.nmeaSourceDevice());
    QCOMPARE(enumerations, 0);
    manager._updateConnections();
    QCOMPARE(enumerations, 0);
}
