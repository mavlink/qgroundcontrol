// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <atomic>
#include <unistd.h>

#include <QGCLoggingCategory.h>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include <AndroidSerial.h>
#include <AndroidSerialPorts.h>

QGC_LOGGING_CATEGORY(QSerialPortInfo_AndroidLog, "Android.AndroidSerialPortInfo")

static std::atomic<bool> s_usePosixSerial{false};

static QString sysfsAttribute(const QDir& dir, const QString& name)
{
    QFile file(dir.filePath(name));
    return file.open(QIODevice::ReadOnly) ? QString::fromLatin1(file.readAll()).simplified() : QString();
}

/// Describes a port by the USB device that sysfs has its tty under. A UART of the device's own is under none,
/// and stays undescribed.
static void describeUsbDevice(const QString& sysTtyPath, QSerialPortInfoPrivate& info)
{
    const QString ttyPath = QFileInfo(sysTtyPath).canonicalFilePath();
    if (ttyPath.isEmpty()) {
        return;
    }

    QDir dir(ttyPath);
    while (!dir.exists(QStringLiteral("idVendor"))) {
        if (!dir.cdUp() || dir.isRoot()) {
            return;
        }
    }

    info.vendorIdentifier = sysfsAttribute(dir, QStringLiteral("idVendor")).toUShort(&info.hasVendorIdentifier, 16);
    info.productIdentifier = sysfsAttribute(dir, QStringLiteral("idProduct")).toUShort(&info.hasProductIdentifier, 16);
    info.description = sysfsAttribute(dir, QStringLiteral("product"));
    info.manufacturer = sysfsAttribute(dir, QStringLiteral("manufacturer"));
    info.serialNumber = sysfsAttribute(dir, QStringLiteral("serial"));
}

QList<QSerialPortInfo> QSerialPortInfoPrivate::posixPorts(const QString& devDir, const QString& sysTtyDir)
{
    // The UARTs of common SoCs, and the USB serial adapters that the kernel drives itself.
    static const QStringList kNodePatterns = {
        QStringLiteral("ttyS*"),   QStringLiteral("ttyHS*"),  QStringLiteral("ttyMSM*"),
        QStringLiteral("ttyHSL*"), QStringLiteral("ttymxc*"), QStringLiteral("ttyAMA*"),
        QStringLiteral("ttyTHS*"), QStringLiteral("ttyUSB*"), QStringLiteral("ttyACM*"),
    };

    QList<QSerialPortInfo> serialPortInfoList;

    const QStringList deviceNames = QDir(devDir).entryList(kNodePatterns, QDir::System | QDir::Files, QDir::Name);
    for (const QString& deviceName : deviceNames) {
        const QString systemLocation = devDir + QLatin1Char('/') + deviceName;
        if (::access(systemLocation.toLocal8Bit().constData(), R_OK | W_OK) != 0) {
            continue;
        }

        QSerialPortInfoPrivate info;
        info.portName = deviceName;
        info.device = systemLocation;
        describeUsbDevice(sysTtyDir + QLatin1Char('/') + deviceName, info);
        serialPortInfoList.append(info);
    }

    return serialPortInfoList;
}

static QList<QSerialPortInfo> availablePosixPorts()
{
    return QSerialPortInfoPrivate::posixPorts(QStringLiteral("/dev"), QStringLiteral("/sys/class/tty"));
}

void AndroidSerialPorts::setUsePosixSerial(bool use)
{
    s_usePosixSerial.store(use);
    qCDebug(QSerialPortInfo_AndroidLog) << "Serial ports:" << (use ? "USB and POSIX" : "USB");
}

bool AndroidSerialPorts::usePosixSerial()
{
    return s_usePosixSerial.load();
}

bool AndroidSerialPorts::hasPosixSerialPorts()
{
    return !availablePosixPorts().isEmpty();
}

bool AndroidSerialPorts::hasUsbPermission(const QString& portName)
{
    const QString location = QSerialPortInfoPrivate::portNameToSystemLocation(portName);
    return QSerialPortInfoPrivate::isUsbLocation(location) && AndroidSerial::hasPermission(location);
}

bool AndroidSerialPorts::requestUsbPermission(const QString& portName)
{
    const QString location = QSerialPortInfoPrivate::portNameToSystemLocation(portName);
    return QSerialPortInfoPrivate::isUsbLocation(location) && !AndroidSerial::hasPermission(location) &&
           AndroidSerial::requestPermission(location);
}

QT_BEGIN_NAMESPACE

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    QList<QSerialPortInfo> ports = AndroidSerial::availablePorts();
    if (AndroidSerialPorts::usePosixSerial()) {
        ports += availablePosixPorts();
    }

    return ports;
}

// A name that still opens a USB port after its device is replugged starts with this, as QGCUsbSerialPortName.java
// builds it.
static constexpr QLatin1String kStableUsbNamePrefix("usb:");

QString QSerialPortInfoPrivate::portNameToSystemLocation(const QString& source)
{
    // A stable name of a USB port is not the name of a device node, and Java opens the port by it as it is.
    return (source.startsWith(QLatin1Char('/')) || source.startsWith(QLatin1String("./")) ||
            source.startsWith(QLatin1String("../")) || source.startsWith(kStableUsbNamePrefix))
               ? source
               : (QLatin1String("/dev/") + source);
}

bool QSerialPortInfoPrivate::isUsbLocation(const QString& location)
{
    return location.startsWith(QLatin1String("/dev/bus/usb/")) || location.startsWith(kStableUsbNamePrefix);
}

QString QSerialPortInfoPrivate::portNameFromSystemLocation(const QString& source)
{
    return source.startsWith(QLatin1String("/dev/")) ? source.mid(5) : source;
}

QT_END_NAMESPACE
