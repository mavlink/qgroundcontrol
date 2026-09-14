#include "SerialPortManagerTest.h"

#include <QtTest/QSignalSpy>

#include "GPSProvider.h"
#include "GPSTransport.h"
#include "SerialLink.h"
#include "SerialPortManager.h"

void SerialPortManagerTest::_exclusiveReservations()
{
    SerialPortManager ports;
    QVERIFY(!ports.reservePort(QString()));
    auto first = ports.reservePort(QStringLiteral(" /test/gps "));
    QVERIFY(first);
    QCOMPARE(first->systemLocation, QStringLiteral("/test/gps"));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/gps")));
    auto copy = first;
    first.reset();
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/gps")));
    auto other = ports.reservePort(QStringLiteral("/test/mavlink"));
    QVERIFY(other);
    copy.reset();
    QVERIFY(ports.canReservePort(QStringLiteral("/test/gps")));
    other.reset();
    QVERIFY(!ports.anyPortReserved());
}

void SerialPortManagerTest::_singlePortInventory()
{
    int scans = 0;
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeUnknown, QString()}};
    SerialPortManager ports(nullptr, [&]() {
        ++scans;
        return inventory;
    });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    QCOMPARE(ports.availablePorts().size(), 1);
    QCOMPARE(scans, 1);
    reservation.reset();
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(scans, 2);
    QVERIFY(ports.canReservePort(QStringLiteral("/test/mavlink")));
}

UT_REGISTER_TEST(SerialPortManagerTest, TestLabel::Unit)

void SerialPortManagerTest::_inventoryNotificationsAndBaudRates()
{
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeUnknown, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    QSignalSpy changed(&ports, &SerialPortManager::serialPortsChanged);
    (void) ports.availablePorts();
    QCOMPARE(ports.serialPorts(), QStringList{QStringLiteral("/test/gps")});
    QCOMPARE(changed.count(), 1);
    (void) ports.availablePorts();
    QCOMPARE(changed.count(), 1);
    inventory.clear();
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    QVERIFY(ports.serialPorts().isEmpty());
    QCOMPARE(changed.count(), 2);

    const QStringList rates = SerialPortManager::supportedBaudRates();
    QCOMPARE(rates, SerialConfiguration::supportedBaudRates());
    QVERIFY(rates.contains(QStringLiteral("9600")));
    QVERIFY(rates.contains(QStringLiteral("115200")));
    QVERIFY(rates.contains(QStringLiteral("921600")));
    for (qsizetype index = 1; index < rates.size(); ++index) {
        QVERIFY(rates[index - 1].toInt() < rates[index].toInt());
    }
}

void SerialPortManagerTest::_finishedReceiverReleasesReservation_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("open-failed") << false;
    QTest::newRow("cancelled-before-start") << true;
}

void SerialPortManagerTest::_finishedReceiverReleasesReservation()
{
    QFETCH(bool, cancelled);
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeRTKGPS, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    GPSProvider provider(
        [reservation = std::move(reservation)](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; },
        GPSType::u_blox, GPSReceiverConfig{});
    if (cancelled) {
        provider.stop();
    }
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.reservePort(QStringLiteral("/test/mavlink")));
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
}

void SerialPortManagerTest::_routingExclusionsDoNotOccupyPorts()
{
    SerialPortManager ports;
    ports.setSinglePortOnly(true);
    const QString port = QStringLiteral("/test/nmea");
    auto exclusion = ports.excludeFromAutoConnect(port);
    QVERIFY(exclusion);
    QVERIFY(!ports.canAutoConnectPort(port));
    QVERIFY(ports.canReservePort(port));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.canAutoConnectPort(QStringLiteral("/test/mavlink")));
    auto secondOwner = ports.excludeFromAutoConnect(port);
    exclusion.reset();
    QVERIFY(!ports.canAutoConnectPort(port));
    auto active = ports.reservePort(port);
    QVERIFY(active);
    active.reset();
    QVERIFY(!ports.canAutoConnectPort(port));
    secondOwner.reset();
    QVERIFY(ports.canAutoConnectPort(port));
}
