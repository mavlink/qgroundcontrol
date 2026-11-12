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
    auto first = ports.reservePort(QStringLiteral(" /test/gps "), QStringLiteral("the receiver"));
    QVERIFY(first);
    QCOMPARE(first->systemLocation, QStringLiteral("/test/gps"));
    QCOMPARE(ports.reservationOwner(QStringLiteral("/test/gps ")), QStringLiteral("the receiver"));
    QVERIFY(!ports.reservePort(QStringLiteral("/test/gps")));
    auto copy = first;
    first.reset();
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/gps")));
    auto other = ports.reservePort(QStringLiteral("/test/mavlink"));
    QVERIFY(other);
    QVERIFY(ports.reservationOwner(QStringLiteral("/test/mavlink")).isEmpty());
    copy.reset();
    QVERIFY(ports.canReservePort(QStringLiteral("/test/gps")));
    QVERIFY(ports.reservationOwner(QStringLiteral("/test/gps")).isEmpty());
    other.reset();
    QVERIFY(!ports.isPortReserved(QStringLiteral("/test/mavlink")));
}

void SerialPortManagerTest::_stableNamesFindPortsAgain()
{
    SerialPortManager::Port radio{QStringLiteral("/test/usb/001/004"), QStringLiteral("001/004"),
                                  QGCSerialPortInfo::BoardTypeUnknown, QString()};
    radio.stableName = QStringLiteral("usb:0403:6015:R1");
    radio.displayName = QStringLiteral("Radio (001/004)");
    // Adapters without a serial number share a stable name.
    SerialPortManager::Port twin{QStringLiteral("/test/usb/001/005"), QStringLiteral("001/005"),
                                 QGCSerialPortInfo::BoardTypeUnknown, QString()};
    twin.stableName = QStringLiteral("usb:1a86:7523:");
    SerialPortManager::Port otherTwin = twin;
    otherTwin.systemLocation = QStringLiteral("/test/usb/001/006");
    otherTwin.portName = QStringLiteral("001/006");
    QList<SerialPortManager::Port> inventory{radio, twin, otherTwin};
    SerialPortManager ports(nullptr, [&] { return inventory; });
    QSignalSpy enumerated(&ports, &SerialPortManager::portsEnumerated);
    (void) ports.availablePorts();
    const QStringList identities = enumerated.first().first().toStringList();
    QVERIFY(identities.contains(radio.stableName));
    QVERIFY(identities.contains(twin.stableName));
    QCOMPARE(ports.displayName(radio.stableName), radio.displayName);

    QCOMPARE(ports.persistentName(radio.systemLocation), radio.stableName);
    QCOMPARE(ports.persistentName(radio.portName), radio.stableName);
    QCOMPARE(ports.persistentName(twin.systemLocation), twin.systemLocation);
    QCOMPARE(ports.persistentName(twin.stableName), twin.stableName);
    QCOMPARE(ports.persistentName(QStringLiteral("/test/absent")), QStringLiteral("/test/absent"));

    auto claim = ports.reservePort(radio.stableName, QStringLiteral("the radio link"));
    QVERIFY(claim);
    QVERIFY(ports.isSamePort(radio.stableName, radio.systemLocation));
    QVERIFY(!ports.isSamePort(radio.stableName, twin.systemLocation));
    QVERIFY(ports.isPortReserved(radio.systemLocation));
    QVERIFY(!ports.reservePort(radio.systemLocation));
    QVERIFY(!ports.canAutoConnectPort(radio.systemLocation));
    QCOMPARE(ports.reservationOwner(radio.systemLocation), QStringLiteral("the radio link"));
    QVERIFY(ports.canReservePort(twin.systemLocation));

    // A stable name that two listed ports share stands for neither, so that it never finds the wrong one.
    QVERIFY(!ports.isSamePort(twin.stableName, twin.systemLocation));
    QVERIFY(!ports.isSamePort(twin.stableName, otherTwin.systemLocation));
    QVERIFY(ports.displayName(twin.stableName).isEmpty());
    auto twinClaim = ports.reservePort(twin.systemLocation);
    QVERIFY(twinClaim);
    QVERIFY(!ports.isPortReserved(twin.stableName));
    twinClaim.reset();

    // Replugged, the radio has another system location, and the claim by its stable name is on that.
    const QString replugged = QStringLiteral("/test/usb/001/009");
    inventory[0].systemLocation = replugged;
    inventory[0].portName = QStringLiteral("001/009");
    ports.rescan();
    QVERIFY(ports.isPortReserved(replugged));
    QVERIFY(!ports.isPortReserved(radio.systemLocation));
    claim.reset();
    QVERIFY(ports.canReservePort(replugged));
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
    const QString port = QStringLiteral("/test/nmea");
    auto exclusion = ports.excludeFromAutoConnect(port);
    QVERIFY(exclusion);
    QVERIFY(ports.isAutoConnectExcluded(QStringLiteral(" /test/nmea ")));
    QVERIFY(!ports.canAutoConnectPort(port));
    QVERIFY(ports.canReservePort(port));
    QVERIFY(!ports.isPortReserved(port));
    QVERIFY(ports.canAutoConnectPort(QStringLiteral("/test/mavlink")));
    auto secondOwner = ports.excludeFromAutoConnect(port);
    exclusion.reset();
    QVERIFY(!ports.canAutoConnectPort(port));
    auto active = ports.reservePort(port);
    QVERIFY(active);
    QVERIFY(ports.isAutoConnectExcluded(port));
    QVERIFY(!ports.isAutoConnectExcluded(QStringLiteral("/test/mavlink")));
    QVERIFY(ports.canAutoConnectPort(QStringLiteral("/test/mavlink")));
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
