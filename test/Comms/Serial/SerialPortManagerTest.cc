#include "SerialPortManagerTest.h"

#include <optional>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
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

    // A port is found by any of its names.
    const auto locationOf = [&ports](const QString& name) {
        const std::optional<SerialPortManager::Port> port = ports.port(name);
        return port ? port->systemLocation : QString();
    };
    for (const QString& name : {radio.stableName, radio.portName, QStringLiteral(" /test/usb/001/004 ")}) {
        QCOMPARE(locationOf(name), radio.systemLocation);
    }
    QVERIFY(!ports.port(QStringLiteral("/test/absent")));

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
    QVERIFY(!ports.port(twin.stableName));
    QCOMPARE(locationOf(twin.systemLocation), twin.systemLocation);
    auto twinClaim = ports.reservePort(twin.systemLocation);
    QVERIFY(twinClaim);
    QVERIFY(!ports.isPortReserved(twin.stableName));
    twinClaim.reset();

    // Replugged, the radio has another system location, and the claim by its stable name is on that.
    const QString replugged = QStringLiteral("/test/usb/001/009");
    inventory[0].systemLocation = replugged;
    inventory[0].portName = QStringLiteral("001/009");
    ports.rescan();
    QCOMPARE(locationOf(radio.stableName), replugged);
    QVERIFY(ports.isPortReserved(replugged));
    QVERIFY(!ports.isPortReserved(radio.systemLocation));
    claim.reset();
    QVERIFY(ports.canReservePort(replugged));
}

void SerialPortManagerTest::_claimByAStableNameHoldsOnceTheNameIsShared()
{
    SerialPortManager::Port receiver{QStringLiteral("/test/usb/001/004"), QStringLiteral("001/004"),
                                     QGCSerialPortInfo::BoardTypeUnknown, QString()};
    receiver.stableName = QStringLiteral("usb:1546:01a9:");
    QList<SerialPortManager::Port> inventory{receiver};
    SerialPortManager ports(nullptr, [&] { return inventory; });
    (void) ports.availablePorts();
    auto claim = ports.reservePort(receiver.stableName, QStringLiteral("the link"));
    QVERIFY(claim);

    // The claim follows the device to where it is replugged.
    const QString replugged = QStringLiteral("/test/usb/001/007");
    inventory[0].systemLocation = replugged;
    inventory[0].portName = QStringLiteral("001/007");
    ports.rescan();
    QVERIFY(ports.isPortReserved(replugged));

    // A second device of the kind is attached. Their name stands for neither now, and the port that is open
    // stays claimed, so that nothing else tries to open it.
    SerialPortManager::Port twin = inventory[0];
    twin.systemLocation = QStringLiteral("/test/usb/001/008");
    twin.portName = QStringLiteral("001/008");
    inventory.append(twin);
    ports.rescan();
    QVERIFY(!ports.port(receiver.stableName));
    QVERIFY(ports.isPortReserved(replugged));
    QCOMPARE(ports.reservationOwner(replugged), QStringLiteral("the link"));
    QVERIFY(!ports.canAutoConnectPort(replugged));
    QVERIFY(!ports.reservePort(replugged));
    QVERIFY(ports.canReservePort(twin.systemLocation));
    QVERIFY(!ports.isPortReserved(receiver.systemLocation));

    claim.reset();
    QVERIFY(ports.canReservePort(replugged));
}

void SerialPortManagerTest::_linksToPortsFindThem()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // A device file, and a link to it as udev makes in /dev/serial/by-id
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile device(dir.filePath(QStringLiteral("ttyACM4")));
    QVERIFY(device.open(QIODevice::WriteOnly));
    device.close();
    const QString location = QFileInfo(device.fileName()).canonicalFilePath();
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("by-id")));
    const QString link = dir.filePath(QStringLiteral("by-id/usb-ArduPilot_Pixhawk6X_0123-if00"));
    QVERIFY(QFile::link(location, link));

    SerialPortManager::Port autopilot{location, QStringLiteral("ttyACM4"), QGCSerialPortInfo::BoardTypePixhawk,
                                      QStringLiteral("Pixhawk6X")};
    SerialPortManager::Port other{QStringLiteral("/test/ttyACM5"), QStringLiteral("ttyACM5"),
                                  QGCSerialPortInfo::BoardTypeUnknown, QString()};
    QList<SerialPortManager::Port> inventory{autopilot, other};
    SerialPortManager ports(nullptr, [&] { return inventory; });
    (void) ports.availablePorts();

    QCOMPARE(SerialPortManager::linkTarget(link), location);
    QCOMPARE(SerialPortManager::linkTarget(QStringLiteral(" %1 ").arg(link)), location);
    QVERIFY(SerialPortManager::linkTarget(location).isEmpty());
    QVERIFY(SerialPortManager::linkTarget(QStringLiteral("ttyACM4")).isEmpty());
    QVERIFY(SerialPortManager::linkTarget(dir.filePath(QStringLiteral("by-id/absent"))).isEmpty());

    const std::optional<SerialPortManager::Port> found = ports.port(link);
    QVERIFY(found);
    QCOMPARE(found->systemLocation, location);
    QVERIFY(ports.isSamePort(link, location));
    QVERIFY(!ports.isSamePort(link, other.systemLocation));
    QCOMPARE(ports.displayName(link), autopilot.portName);
    // A saved link keeps the path it was given
    QCOMPARE(ports.persistentName(link), link);

    // A claim by either name holds the port against the other
    auto claim = ports.reservePort(link, QStringLiteral("the link"));
    QVERIFY(claim);
    QVERIFY(!ports.canReservePort(location));
    QCOMPARE(ports.reservationOwner(location), QStringLiteral("the link"));
    claim.reset();
    claim = ports.reservePort(location);
    QVERIFY(claim);
    QVERIFY(!ports.canReservePort(link));
    claim.reset();

    // Unplugged: the link leads nowhere, and so to no port
    QVERIFY(QFile::remove(location));
    QVERIFY(SerialPortManager::linkTarget(link).isEmpty());
    QVERIFY(!ports.port(link));
#else
    QSKIP("Ports are device files only on desktop Unix");
#endif
}

void SerialPortManagerTest::_portLinksLeadToListedPorts()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // Device files, links to them as udev makes in /dev/serial/by-id, and other entries there
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto makeFile = [&dir](const QString& name) {
        QFile file(dir.filePath(name));
        return file.open(QIODevice::WriteOnly) ? QFileInfo(file.fileName()).canonicalFilePath() : QString();
    };
    const QString location = makeFile(QStringLiteral("ttyACM4"));
    const QString unlisted = makeFile(QStringLiteral("ttyUSB0"));
    QVERIFY(!location.isEmpty());
    QVERIFY(!unlisted.isEmpty());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("by-id")));
    const QString byId = dir.filePath(QStringLiteral("by-id"));
    const QString link = dir.filePath(QStringLiteral("by-id/usb-ArduPilot_Pixhawk6X_0123-if00"));
    QVERIFY(QFile::link(location, link));
    QVERIFY(QFile::link(unlisted, dir.filePath(QStringLiteral("by-id/usb-FTDI_FT232R_0456-if00-port0"))));
    QVERIFY(QFile::link(dir.filePath(QStringLiteral("absent")), dir.filePath(QStringLiteral("by-id/dangling"))));
    QVERIFY(!makeFile(QStringLiteral("by-id/not-a-link")).isEmpty());

    QList<SerialPortManager::Port> inventory{
        {location, QStringLiteral("ttyACM4"), QGCSerialPortInfo::BoardTypePixhawk, QStringLiteral("Pixhawk6X")}};
    SerialPortManager ports(nullptr, [&] { return inventory; });
    (void) ports.availablePorts();

    QCOMPARE(ports.portLinks(byId), QStringList{link});
    QVERIFY(ports.portLinks(dir.filePath(QStringLiteral("no-such-directory"))).isEmpty());

    // Unplugged: the link leads nowhere
    QVERIFY(QFile::remove(location));
    QVERIFY(ports.portLinks(byId).isEmpty());
#else
    QSKIP("Ports are device files only on desktop Unix");
#endif
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
