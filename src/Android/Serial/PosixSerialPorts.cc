// The serial device nodes of the Android device, which QSerialPortInfo lists if the app says so.

#include <algorithm>
#include <optional>
#include <unistd.h>
#include <utility>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include "AndroidSerialPortInfo.h"

namespace {

QString sysfsAttribute(const QDir& dir, const QString& name)
{
    QFile file(dir.filePath(name));
    return file.open(QIODevice::ReadOnly) ? QString::fromLatin1(file.readAll()).simplified() : QString();
}

/// Whether sysfs has a driver for the device of a tty. The kernel's consoles and terminals have none.
bool hasDriver(const QString& sysTtyPath)
{
    return QFileInfo::exists(sysTtyPath + QStringLiteral("/device/driver"));
}

/// Whether sysfs gives a UART the type of a port without hardware, which fails every read and write. The PC
/// serial driver keeps spare ports of this kind. Desktop QSerialPort opens such a port to ask for its type,
/// which moves the modem lines of a real one and fails on one that is held exclusively.
bool isUartWithoutHardware(const QString& sysTtyPath)
{
    return sysfsAttribute(QDir(sysTtyPath), QStringLiteral("type")) == QLatin1String("0");
}

/// The directory that sysfs has for the USB device of a tty. A UART of the device's own is under no USB device.
std::optional<QDir> usbDeviceOf(const QString& sysTtyPath)
{
    const QString ttyPath = QFileInfo(sysTtyPath).canonicalFilePath();
    if (ttyPath.isEmpty()) {
        return std::nullopt;
    }

    QDir dir(ttyPath);
    while (!dir.exists(QStringLiteral("idVendor"))) {
        if (!dir.cdUp() || dir.isRoot()) {
            return std::nullopt;
        }
    }

    return dir;
}

/// Whether Java lists a port of this USB device. It names a port by the device's node in /dev/bus/usb, followed
/// by a suffix if the device has several ports.
bool isListedUsbDevice(const QDir& usbDevice, const QList<QSerialPortInfo>& usbPorts)
{
    bool hasBus = false;
    bool hasDevice = false;
    const int bus = sysfsAttribute(usbDevice, QStringLiteral("busnum")).toInt(&hasBus);
    const int device = sysfsAttribute(usbDevice, QStringLiteral("devnum")).toInt(&hasDevice);
    if (!hasBus || !hasDevice) {
        return false;
    }

    const QString deviceName =
        QStringLiteral("/dev/bus/usb/%1/%2").arg(bus, 3, 10, QLatin1Char('0')).arg(device, 3, 10, QLatin1Char('0'));
    return std::any_of(usbPorts.cbegin(), usbPorts.cend(), [&deviceName](const QSerialPortInfo& port) {
        const QString location = port.systemLocation();
        return (location == deviceName) || location.startsWith(deviceName + QLatin1Char('#'));
    });
}

void describeUsbDevice(const QDir& usbDevice, QSerialPortInfo::Data& info)
{
    info.vendorIdentifier =
        sysfsAttribute(usbDevice, QStringLiteral("idVendor")).toUShort(&info.hasVendorIdentifier, 16);
    info.productIdentifier =
        sysfsAttribute(usbDevice, QStringLiteral("idProduct")).toUShort(&info.hasProductIdentifier, 16);
    info.description = sysfsAttribute(usbDevice, QStringLiteral("product"));
    info.manufacturer = sysfsAttribute(usbDevice, QStringLiteral("manufacturer"));
    info.serialNumber = sysfsAttribute(usbDevice, QStringLiteral("serial"));
}

}  // namespace

QList<QSerialPortInfo> QSerialPortInfo::posixPorts(const QString& devDir, const QString& sysTtyDir,
                                                   const QList<QSerialPortInfo>& usbPorts)
{
    // The UARTs of common SoCs, and the USB serial adapters that the kernel drives itself. Their names find them
    // where the app may not read sysfs.
    static const QStringList kNodePatterns = {
        QStringLiteral("ttyS*"),   QStringLiteral("ttyHS*"),  QStringLiteral("ttyMSM*"),
        QStringLiteral("ttyHSL*"), QStringLiteral("ttymxc*"), QStringLiteral("ttyAMA*"),
        QStringLiteral("ttyTHS*"), QStringLiteral("ttyUSB*"), QStringLiteral("ttyACM*"),
    };

    QStringList deviceNames = QDir(devDir).entryList(kNodePatterns, QDir::System | QDir::Files);
    // As for desktop QSerialPort, any other tty that has a driver is a serial port too: a UART by another name.
    const QStringList ttyNames = QDir(sysTtyDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& ttyName : ttyNames) {
        if (!deviceNames.contains(ttyName) && hasDriver(sysTtyDir + QLatin1Char('/') + ttyName)) {
            deviceNames.append(ttyName);
        }
    }
    deviceNames.sort();

    QList<QSerialPortInfo> ports;

    for (const QString& deviceName : std::as_const(deviceNames)) {
        const QString systemLocation = devDir + QLatin1Char('/') + deviceName;
        const QString sysTtyPath = sysTtyDir + QLatin1Char('/') + deviceName;
        if ((::access(systemLocation.toLocal8Bit().constData(), R_OK | W_OK) != 0) ||
            isUartWithoutHardware(sysTtyPath)) {
            continue;
        }
        // One device, one port. Java takes a USB device from the kernel's driver as it opens it, which would end
        // a connection that is open on the device's node.
        const std::optional<QDir> usbDevice = usbDeviceOf(sysTtyPath);
        if (usbDevice && isListedUsbDevice(*usbDevice, usbPorts)) {
            continue;
        }

        Data info;
        info.portName = deviceName;
        info.systemLocation = systemLocation;
        if (usbDevice) {
            describeUsbDevice(*usbDevice, info);
        }
        ports.append(QSerialPortInfo(info));
    }

    return ports;
}
