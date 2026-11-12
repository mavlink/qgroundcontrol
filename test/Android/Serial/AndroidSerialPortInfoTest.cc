#include "AndroidSerialPortInfoTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "AndroidSerialAccess.h"
#include "AndroidSerialFake.h"
#include "AndroidSerialPort.h"
#include "AndroidSerialPortInfo.h"

namespace {

bool writeTreeFile(const QDir& dir, const QString& path, const QByteArray& content = QByteArray())
{
    QFile file(dir.filePath(path));
    return dir.mkpath(QFileInfo(file).path()) && file.open(QIODevice::WriteOnly) &&
           (file.write(content) == content.size());
}

/// Adds a tty to a sysfs tree. The kernel has a tty as a directory under its device, to which the tty links back
/// and which has a driver, and lists every tty as a link in /sys/class/tty.
bool addSysfsTty(const QDir& dir, const QString& device, const QString& name)
{
    const QString tty = device + QStringLiteral("/tty/") + name;
    return dir.mkpath(tty) && dir.mkpath(device + QStringLiteral("/driver")) &&
           dir.mkpath(QStringLiteral("sys/class/tty")) &&
           QFile::link(dir.filePath(device), dir.filePath(tty + QStringLiteral("/device"))) &&
           QFile::link(dir.filePath(tty), dir.filePath(QStringLiteral("sys/class/tty/") + name));
}

}  // namespace

void AndroidSerialPortInfoTest::init()
{
    UnitTest::init();
    AndroidSerialFake::reset();
}

void AndroidSerialPortInfoTest::_posixPortsAreListedAndDescribedBySysfs()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QDir dir(root.path());

    for (const char* node : {"dev/ttyUSB0", "dev/ttyS1", "dev/ttyS2", "dev/ttyACM3", "dev/ttyMT1", "dev/tty0"}) {
        QVERIFY(writeTreeFile(dir, QString::fromLatin1(node)));
    }

    // A USB adapter that the kernel drives.
    const QString usbDevice = QStringLiteral("sys/devices/usb1/1-1/");
    QVERIFY(writeTreeFile(dir, usbDevice + QStringLiteral("idVendor"), "0403\n"));
    QVERIFY(writeTreeFile(dir, usbDevice + QStringLiteral("idProduct"), "6001\n"));
    QVERIFY(writeTreeFile(dir, usbDevice + QStringLiteral("manufacturer"), "FTDI\n"));
    QVERIFY(writeTreeFile(dir, usbDevice + QStringLiteral("product"), "FT232R USB UART\n"));
    QVERIFY(writeTreeFile(dir, usbDevice + QStringLiteral("serial"), "A1B2C3\n"));
    QVERIFY(addSysfsTty(dir, usbDevice + QStringLiteral("1-1:1.0/ttyUSB0"), QStringLiteral("ttyUSB0")));
    // A UART of the device's own, and a spare port of its driver that has no hardware.
    const QString uarts = QStringLiteral("sys/devices/platform/serial8250");
    QVERIFY(addSysfsTty(dir, uarts, QStringLiteral("ttyS1")));
    QVERIFY(writeTreeFile(dir, uarts + QStringLiteral("/tty/ttyS1/type"), "4\n"));
    QVERIFY(addSysfsTty(dir, uarts, QStringLiteral("ttyS2")));
    QVERIFY(writeTreeFile(dir, uarts + QStringLiteral("/tty/ttyS2/type"), "0\n"));
    // Two UARTs that only sysfs tells to be serial ports. The second has no node to open.
    QVERIFY(addSysfsTty(dir, QStringLiteral("sys/devices/platform/mtk-uart"), QStringLiteral("ttyMT1")));
    QVERIFY(addSysfsTty(dir, QStringLiteral("sys/devices/platform/mtk-uart"), QStringLiteral("ttyMT2")));
    // A terminal of the kernel's, which has no driver. The node ttyACM3 is one that sysfs does not know.
    QVERIFY(dir.mkpath(QStringLiteral("sys/devices/virtual/tty/tty0")));
    QVERIFY(QFile::link(dir.filePath(QStringLiteral("sys/devices/virtual/tty/tty0")),
                        dir.filePath(QStringLiteral("sys/class/tty/tty0"))));

    const QList<QSerialPortInfo> ports =
        QSerialPortInfo::posixPorts(dir.filePath(QStringLiteral("dev")), dir.filePath(QStringLiteral("sys/class/tty")));
    QStringList names;
    for (const QSerialPortInfo& port : ports) {
        names.append(port.portName());
    }
    QCOMPARE(names, QStringList({QStringLiteral("ttyACM3"), QStringLiteral("ttyMT1"), QStringLiteral("ttyS1"),
                                 QStringLiteral("ttyUSB0")}));

    const QSerialPortInfo& adapter = ports[3];
    QCOMPARE(adapter.systemLocation(), dir.filePath(QStringLiteral("dev/ttyUSB0")));
    QVERIFY(adapter.hasVendorIdentifier());
    QCOMPARE(adapter.vendorIdentifier(), quint16(0x0403));
    QVERIFY(adapter.hasProductIdentifier());
    QCOMPARE(adapter.productIdentifier(), quint16(0x6001));
    QCOMPARE(adapter.manufacturer(), QStringLiteral("FTDI"));
    QCOMPARE(adapter.description(), QStringLiteral("FT232R USB UART"));
    QCOMPARE(adapter.serialNumber(), QStringLiteral("A1B2C3"));

    for (const QSerialPortInfo& undescribed : {ports[0], ports[1], ports[2]}) {
        QVERIFY(!undescribed.hasVendorIdentifier());
        QVERIFY(!undescribed.hasProductIdentifier());
        QVERIFY(undescribed.description().isEmpty());
        QVERIFY(undescribed.serialNumber().isEmpty());
    }
}

void AndroidSerialPortInfoTest::_posixSettingAddsToTheUsbPorts()
{
    // A device with one serial port of its own. The machine that runs the test may have none that it may open.
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QDir dir(root.path());
    QVERIFY(dir.mkpath(QStringLiteral("dev")));
    QVERIFY(QFile(dir.filePath(QStringLiteral("dev/ttyS1"))).open(QIODevice::WriteOnly));
    const auto listedNames = [&dir] {
        QStringList names;
        const QList<QSerialPortInfo> listed = QSerialPortInfo::availablePorts(
            dir.filePath(QStringLiteral("dev")), dir.filePath(QStringLiteral("sys/class/tty")));
        for (const QSerialPortInfo& port : listed) {
            names.append(port.portName());
        }
        return names;
    };

    QSerialPortInfo::Data usbPort;
    usbPort.portName = AndroidSerialFake::usbPortName;
    java().ports = {QSerialPortInfo(usbPort)};
    QCOMPARE(listedNames(), QStringList({usbPort.portName}));

    // A device that lets QGC open its own serial ports still takes USB adapters.
    AndroidSerialAccess::setUsePosixSerial(true);
    const auto restoreSetting = qScopeGuard([] { AndroidSerialAccess::setUsePosixSerial(false); });
    QCOMPARE(listedNames(), QStringList({usbPort.portName, QStringLiteral("ttyS1")}));
}

void AndroidSerialPortInfoTest::_nodeOfAUsbDeviceThatJavaListsIsLeftOut()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QDir dir(root.path());
    // A serial node of a USB device that the kernel drives, on bus 1.
    const auto addUsbNode = [&dir](int deviceNumber, const QString& interface, const QString& name) {
        const QString usbDevice = QStringLiteral("sys/devices/usb1/1-%1/").arg(deviceNumber);
        return writeTreeFile(dir, usbDevice + QStringLiteral("idVendor"), "2dae\n") &&
               writeTreeFile(dir, usbDevice + QStringLiteral("busnum"), "1\n") &&
               writeTreeFile(dir, usbDevice + QStringLiteral("devnum"), QByteArray::number(deviceNumber) + '\n') &&
               addSysfsTty(dir, usbDevice + interface, name) && writeTreeFile(dir, QStringLiteral("dev/") + name);
    };
    QVERIFY(addUsbNode(2, QStringLiteral("1-2:1.0"), QStringLiteral("ttyACM0")));
    QVERIFY(addUsbNode(9, QStringLiteral("1-9:1.0"), QStringLiteral("ttyACM1")));
    QVERIFY(addUsbNode(9, QStringLiteral("1-9:1.2"), QStringLiteral("ttyACM2")));
    QVERIFY(addUsbNode(12, QStringLiteral("1-12:1.0/ttyUSB0"), QStringLiteral("ttyUSB0")));

    // Java has a driver for the first device and for the second, which has two ports, and none for the third.
    const auto usbPort = [](const QString& location) {
        QSerialPortInfo::Data port;
        port.systemLocation = location;
        port.portName = QSerialPortInfo::portNameFromSystemLocation(location);
        return QSerialPortInfo(port);
    };
    const QStringList usbLocations = {QStringLiteral("/dev/bus/usb/001/002"), QStringLiteral("/dev/bus/usb/001/009#p0"),
                                      QStringLiteral("/dev/bus/usb/001/009#p1")};
    java().ports = {usbPort(usbLocations[0]), usbPort(usbLocations[1]), usbPort(usbLocations[2])};
    AndroidSerialAccess::setUsePosixSerial(true);
    const auto restoreSetting = qScopeGuard([] { AndroidSerialAccess::setUsePosixSerial(false); });

    QStringList listed;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts(dir.filePath(QStringLiteral("dev")),
                                                                         dir.filePath(QStringLiteral("sys/class/tty")));
    for (const QSerialPortInfo& port : ports) {
        listed.append(port.systemLocation());
    }
    // A device that is listed both ways would be two ports, and Java takes the device from the kernel's driver as
    // it opens it, which ends a connection on the node.
    QCOMPARE(listed, usbLocations + QStringList({dir.filePath(QStringLiteral("dev/ttyUSB0"))}));
}

void AndroidSerialPortInfoTest::_usbPermissionIsRequestedForAUsbPortThatNeedsIt()
{
    const QString stableName = QStringLiteral("usb:26ac:0011:0");

    // Java is asked by the name that opens the port: a port name stands for a device node, a stable name for
    // itself.
    QVERIFY(!AndroidSerialAccess::hasUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(AndroidSerialAccess::requestUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(AndroidSerialAccess::requestUsbPermission(stableName));
    QCOMPARE(java().permissionRequests, QStringList({AndroidSerialFake::usbPortName, stableName}));

    // The user is not asked for what QGC may open already.
    java().permittedPorts = {AndroidSerialFake::usbPortName};
    QVERIFY(AndroidSerialAccess::hasUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(!AndroidSerialAccess::requestUsbPermission(QStringLiteral("bus/usb/001/002")));
    QCOMPARE(java().permissionRequests.size(), 2);

    // Java finds no attached device to ask for.
    java().requestPermissionResult = false;
    QVERIFY(!AndroidSerialAccess::requestUsbPermission(stableName));
    QCOMPARE(java().permissionRequests.size(), 3);

    // A refusal is asked about by the same names.
    QVERIFY(!AndroidSerialAccess::isUsbPermissionDenied(QStringLiteral("bus/usb/001/002")));
    java().deniedPorts = {AndroidSerialFake::usbPortName, stableName, QStringLiteral("/dev/ttyS1")};
    QVERIFY(AndroidSerialAccess::isUsbPermissionDenied(QStringLiteral("bus/usb/001/002")));
    QVERIFY(AndroidSerialAccess::isUsbPermissionDenied(stableName));

    // The system decides who may open a serial device node; Java knows no such port.
    java().permittedPorts = {QStringLiteral("/dev/ttyS1")};
    java().requestPermissionResult = true;
    QVERIFY(!AndroidSerialAccess::hasUsbPermission(QStringLiteral("/dev/ttyS1")));
    QVERIFY(!AndroidSerialAccess::isUsbPermissionDenied(QStringLiteral("/dev/ttyS1")));
    QVERIFY(!AndroidSerialAccess::requestUsbPermission(QStringLiteral("/dev/ttyS1")));
    QVERIFY(!AndroidSerialAccess::requestUsbPermission(QStringLiteral("ttyACM0")));
    QCOMPARE(java().permissionRequests.size(), 3);
}

void AndroidSerialPortInfoTest::_portInfoIsFoundByTheStableNameOfAPort()
{
    QSerialPortInfo::Data namedPort;
    namedPort.systemLocation = AndroidSerialFake::usbPortName;
    namedPort.portName = QSerialPortInfo::portNameFromSystemLocation(namedPort.systemLocation);
    namedPort.stableName = QStringLiteral("usb:2dae:1011:0");
    namedPort.vendorIdentifier = 0x2dae;
    namedPort.hasVendorIdentifier = true;
    // A device that QGC may not open yet has no stable name.
    QSerialPortInfo::Data unnamedPort;
    unnamedPort.systemLocation = QStringLiteral("/dev/bus/usb/001/003");
    unnamedPort.portName = QSerialPortInfo::portNameFromSystemLocation(unnamedPort.systemLocation);
    java().ports = {QSerialPortInfo(unnamedPort), QSerialPortInfo(namedPort)};

    // A link that is saved by the stable name of its port opens the port by it, and then asks by the open port
    // what its device is.
    QSerialPort port(namedPort.stableName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    const QSerialPortInfo info(port);
    QCOMPARE(info.systemLocation(), namedPort.systemLocation);
    QCOMPARE(info.vendorIdentifier(), namedPort.vendorIdentifier);

    QVERIFY(QSerialPortInfo(QString()).isNull());
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidSerialPortInfoTest, TestLabel::Unit)
