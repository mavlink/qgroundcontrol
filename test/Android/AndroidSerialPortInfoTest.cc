#include "AndroidSerialPortInfoTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "AndroidSerialFake.h"
#include "AndroidSerialPorts.h"
#include "qserialport.h"
#include "qserialportinfo.h"
#include "qserialportinfo_p.h"

namespace {
AndroidSerialFake& java()
{
    return AndroidSerialFake::instance();
}
}  // namespace

void AndroidSerialPortInfoTest::init()
{
    UnitTest::init();
    AndroidSerialFake::reset();
}

void AndroidSerialPortInfoTest::_posixPortsAreDescribedBySysfs()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QDir dir(root.path());
    const auto writeFile = [&dir](const QString& path, const QByteArray& content = QByteArray()) {
        QFile file(dir.filePath(path));
        return dir.mkpath(QFileInfo(file).path()) && file.open(QIODevice::WriteOnly) &&
               (file.write(content) == content.size());
    };

    // A USB adapter that the kernel drives, a UART of the device's own, a node that sysfs does not know, and one
    // that is no serial port.
    for (const char* node : {"dev/ttyUSB0", "dev/ttyS1", "dev/ttyACM3", "dev/tty0"}) {
        QVERIFY(writeFile(QString::fromLatin1(node)));
    }
    const QString usbDevice = QStringLiteral("sys/devices/usb1/1-1/");
    QVERIFY(writeFile(usbDevice + QStringLiteral("idVendor"), "0403\n"));
    QVERIFY(writeFile(usbDevice + QStringLiteral("idProduct"), "6001\n"));
    QVERIFY(writeFile(usbDevice + QStringLiteral("manufacturer"), "FTDI\n"));
    QVERIFY(writeFile(usbDevice + QStringLiteral("product"), "FT232R USB UART\n"));
    QVERIFY(writeFile(usbDevice + QStringLiteral("serial"), "A1B2C3\n"));
    QVERIFY(dir.mkpath(usbDevice + QStringLiteral("1-1:1.0/ttyUSB0/tty/ttyUSB0")));
    QVERIFY(dir.mkpath(QStringLiteral("sys/devices/platform/serial/tty/ttyS1")));
    QVERIFY(dir.mkpath(QStringLiteral("sys/class/tty")));
    // The kernel lists every tty as a link into its device.
    QVERIFY(QFile::link(dir.filePath(usbDevice + QStringLiteral("1-1:1.0/ttyUSB0/tty/ttyUSB0")),
                        dir.filePath(QStringLiteral("sys/class/tty/ttyUSB0"))));
    QVERIFY(QFile::link(dir.filePath(QStringLiteral("sys/devices/platform/serial/tty/ttyS1")),
                        dir.filePath(QStringLiteral("sys/class/tty/ttyS1"))));

    const QList<QSerialPortInfo> ports = QSerialPortInfoPrivate::posixPorts(
        dir.filePath(QStringLiteral("dev")), dir.filePath(QStringLiteral("sys/class/tty")));
    QCOMPARE(ports.size(), 3);
    QCOMPARE(ports[0].portName(), QStringLiteral("ttyACM3"));
    QCOMPARE(ports[1].portName(), QStringLiteral("ttyS1"));
    QCOMPARE(ports[2].portName(), QStringLiteral("ttyUSB0"));

    const QSerialPortInfo& adapter = ports[2];
    QCOMPARE(adapter.systemLocation(), dir.filePath(QStringLiteral("dev/ttyUSB0")));
    QVERIFY(adapter.hasVendorIdentifier());
    QCOMPARE(adapter.vendorIdentifier(), quint16(0x0403));
    QVERIFY(adapter.hasProductIdentifier());
    QCOMPARE(adapter.productIdentifier(), quint16(0x6001));
    QCOMPARE(adapter.manufacturer(), QStringLiteral("FTDI"));
    QCOMPARE(adapter.description(), QStringLiteral("FT232R USB UART"));
    QCOMPARE(adapter.serialNumber(), QStringLiteral("A1B2C3"));

    for (const QSerialPortInfo& undescribed : {ports[0], ports[1]}) {
        QVERIFY(!undescribed.hasVendorIdentifier());
        QVERIFY(!undescribed.hasProductIdentifier());
        QVERIFY(undescribed.description().isEmpty());
        QVERIFY(undescribed.serialNumber().isEmpty());
    }
}

void AndroidSerialPortInfoTest::_posixSettingAddsToTheUsbPorts()
{
    QSerialPortInfoPrivate usbPort;
    usbPort.portName = AndroidSerialFake::usbPortName;
    java().ports = {QSerialPortInfo(usbPort)};

    QList<QSerialPortInfo> listed = QSerialPortInfo::availablePorts();
    QCOMPARE(listed.size(), 1);
    QCOMPARE(listed.first().portName(), usbPort.portName);

    // A device that lets QGC open its own serial ports still takes USB adapters.
    AndroidSerialPorts::setUsePosixSerial(true);
    const auto restoreSetting = qScopeGuard([] { AndroidSerialPorts::setUsePosixSerial(false); });
    listed = QSerialPortInfo::availablePorts();
    const QList<QSerialPortInfo> posixPorts =
        QSerialPortInfoPrivate::posixPorts(QStringLiteral("/dev"), QStringLiteral("/sys/class/tty"));
    QCOMPARE(listed.size(), 1 + posixPorts.size());
    QCOMPARE(listed.first().portName(), usbPort.portName);
}

void AndroidSerialPortInfoTest::_usbPermissionIsRequestedForAUsbPortThatNeedsIt()
{
    const QString stableName = QStringLiteral("usb:26ac:0011:0");

    // Java is asked by the name that opens the port: a port name stands for a device node, a stable name for
    // itself.
    QVERIFY(!AndroidSerialPorts::hasUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(AndroidSerialPorts::requestUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(AndroidSerialPorts::requestUsbPermission(stableName));
    QCOMPARE(java().permissionRequests, QStringList({AndroidSerialFake::usbPortName, stableName}));

    // The user is not asked for what QGC may open already.
    java().permittedPorts = {AndroidSerialFake::usbPortName};
    QVERIFY(AndroidSerialPorts::hasUsbPermission(QStringLiteral("bus/usb/001/002")));
    QVERIFY(!AndroidSerialPorts::requestUsbPermission(QStringLiteral("bus/usb/001/002")));
    QCOMPARE(java().permissionRequests.size(), 2);

    // Java finds no attached device to ask for.
    java().requestPermissionResult = false;
    QVERIFY(!AndroidSerialPorts::requestUsbPermission(stableName));
    QCOMPARE(java().permissionRequests.size(), 3);

    // The system decides who may open a serial device node; Java knows no such port.
    java().permittedPorts = {QStringLiteral("/dev/ttyS1")};
    java().requestPermissionResult = true;
    QVERIFY(!AndroidSerialPorts::hasUsbPermission(QStringLiteral("/dev/ttyS1")));
    QVERIFY(!AndroidSerialPorts::requestUsbPermission(QStringLiteral("/dev/ttyS1")));
    QVERIFY(!AndroidSerialPorts::requestUsbPermission(QStringLiteral("ttyACM0")));
    QCOMPARE(java().permissionRequests.size(), 3);
}

void AndroidSerialPortInfoTest::_portInfoIsFoundByTheStableNameOfAPort()
{
    QSerialPortInfoPrivate namedPort;
    namedPort.device = AndroidSerialFake::usbPortName;
    namedPort.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(namedPort.device);
    namedPort.stableName = QStringLiteral("usb:2dae:1011:0");
    namedPort.vendorIdentifier = 0x2dae;
    namedPort.hasVendorIdentifier = true;
    // A device that QGC may not open yet has no stable name.
    QSerialPortInfoPrivate unnamedPort;
    unnamedPort.device = QStringLiteral("/dev/bus/usb/001/003");
    unnamedPort.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(unnamedPort.device);
    java().ports = {QSerialPortInfo(unnamedPort), QSerialPortInfo(namedPort)};

    // A link that is saved by the stable name of its port opens the port by it, and then asks by the open port
    // what its device is.
    QSerialPort port(namedPort.stableName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    const QSerialPortInfo info(port);
    QCOMPARE(info.systemLocation(), namedPort.device);
    QCOMPARE(info.vendorIdentifier(), namedPort.vendorIdentifier);

    QVERIFY(QSerialPortInfo(QString()).isNull());
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidSerialPortInfoTest, TestLabel::Unit)
