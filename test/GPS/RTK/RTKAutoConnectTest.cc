#include "RTKAutoConnectTest.h"

#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSRtk.h"
#include "RTKAutoConnect.h"
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
