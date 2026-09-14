#include "SerialPortManagerTest.h"

#include <QtTest/QSignalSpy>

#include "SerialLink.h"
#include "SerialPortManager.h"

void SerialPortManagerTest::_exclusiveReservations()
{
    SerialPortManager::Port gps{QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeRTKGPS,
                                QStringLiteral("Receiver")};
    gps.physicalDeviceId = QStringLiteral("1:2:serial");
    gps.description = QStringLiteral("NMEA interface");
    auto mavlink = gps;
    mavlink.systemLocation = QStringLiteral("/test/mavlink");
    mavlink.portName = QStringLiteral("mavlink");
    mavlink.description = QStringLiteral("MAVLink interface");
    SerialPortManager ports(nullptr, [&] { return QList<SerialPortManager::Port>{gps, mavlink}; });
    const auto inventory = ports.availablePorts();
    QCOMPARE(inventory.size(), 2);
    QCOMPARE(inventory[0].physicalDeviceId, inventory[1].physicalDeviceId);
    QCOMPARE(inventory[0].description, gps.description);
    QCOMPARE(inventory[1].description, mavlink.description);
    QCOMPARE(ports.serialPorts(), QStringList({gps.systemLocation, mavlink.systemLocation}));
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
    QSignalSpy enumerated(&ports, &SerialPortManager::portsEnumerated);
    QCOMPARE(ports.availablePorts().size(), 1);
    QCOMPARE(enumerated.size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    QCOMPARE(ports.availablePorts().size(), 1);
    QCOMPARE(scans, 1);
    QCOMPARE(enumerated.size(), 1);
    reservation.reset();
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(scans, 2);
    QCOMPARE(enumerated.size(), 2);
    QVERIFY(enumerated.last().first().toStringList().isEmpty());
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

void SerialPortManagerTest::_routingExclusionsDoNotOccupyPorts()
{
    SerialPortManager ports;
    ports.setSinglePortOnly(true);
    const QString port = QStringLiteral("/test/nmea");
    auto exclusion = ports.excludeFromAutoConnect(port);
    QVERIFY(exclusion);
    QVERIFY(ports.isAutoConnectExcluded(QStringLiteral(" /test/nmea ")));
    QVERIFY(!ports.canAutoConnectPort(port));
    QVERIFY(ports.canReservePort(port));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.canAutoConnectPort(QStringLiteral("/test/mavlink")));
    auto secondOwner = ports.excludeFromAutoConnect(port);
    exclusion.reset();
    QVERIFY(!ports.canAutoConnectPort(port));
    auto active = ports.reservePort(port);
    QVERIFY(active);
    QVERIFY(ports.isAutoConnectExcluded(port));
    QVERIFY(!ports.isAutoConnectExcluded(QStringLiteral("/test/mavlink")));
    QVERIFY(!ports.canAutoConnectPort(QStringLiteral("/test/mavlink")));
    active.reset();
    QVERIFY(!ports.canAutoConnectPort(port));
    secondOwner.reset();
    QVERIFY(!ports.isAutoConnectExcluded(port));
    QVERIFY(ports.canAutoConnectPort(port));
}

void SerialPortManagerTest::_displayMetadataUsesCachedInventory()
{
    int scans = 0;
    SerialPortManager::Port port{QStringLiteral("/test/serial"), QStringLiteral("serial"),
                                 QGCSerialPortInfo::BoardTypeUnknown, QString()};
    port.displayName = QStringLiteral("USB receiver (serial)");
    SerialPortManager ports(nullptr, [&] {
        ++scans;
        return QList<SerialPortManager::Port>{port};
    });
    QSignalSpy enumerated(&ports, &SerialPortManager::portsEnumerated);
    QCOMPARE(ports.displayName(port.systemLocation), port.displayName);
    QCOMPARE(ports.displayName(port.portName), port.displayName);
    QVERIFY(ports.displayName(QStringLiteral("/test/missing")).isEmpty());
    QCOMPARE(scans, 1);
    QCOMPARE(enumerated.size(), 1);
    const auto identities = enumerated.first().first().toStringList();
    QVERIFY(identities.contains(port.systemLocation));
    QVERIFY(identities.contains(port.portName));
    port.displayName = QStringLiteral("Updated receiver (serial)");
    QTRY_COMPARE_WITH_TIMEOUT(ports.displayName(port.systemLocation), port.displayName, TestTimeout::mediumMs());
    QCOMPARE(scans, 2);
    QCOMPARE(enumerated.size(), 2);
}
