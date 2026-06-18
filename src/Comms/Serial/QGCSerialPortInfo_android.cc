#include <QtCore/QMutex>

#include <QtSerialPort/QSerialPortInfo>

#include "AndroidSerialPort.h"
#include "HostSerialPort.h"
#include "NmeaSerialDevice.h"
#include "QGCSerialPort.h"
#include "QGCSerialPortInfo.h"
#include "QGCSerialPortTypes.h"
#include "SerialPlatform.h"

namespace SerialPlatform {

// makeSerialPort routing: "/dev/tty*" kernel TTYs use QSerialPort (AX12, Rock5 UARTs); USB-host "/dev/bus/usb/..." → AndroidSerialPort. Narrow "/dev/tty" prefix keeps USB paths off this branch.
QGCSerialPort* makeSerialPort(const QString& name, QObject* parent)
{
    if (const auto& factory = portFactoryOverride()) {
        return factory(name, parent);
    }
    if (name.startsWith(kDirectUartPathPrefix)) {
        return new HostSerialPort(name, parent);
    }
    return new AndroidSerialPort(name, parent);
}

QIODevice *makeNmeaSerialSource(const QString &systemLocation, qint32 baud, QObject *parent)
{
    // "/dev/tty*" kernel TTYs work as a plain GUI-thread QIODevice; USB-host paths must run off-thread.
    if (systemLocation.startsWith(kDirectUartPathPrefix)) {
        return HostSerialPort::openHostNmeaSource(systemLocation, baud, parent);
    }
    return new NmeaSerialDevice(systemLocation, baud, parent);
}

}  // namespace SerialPlatform

QList<qint32> QGCSerialPortInfo::portSpecificBaudRates(const QString &portName)
{
    // QGCSerialPortInfo carries per-port supportedBaudRates populated at enumeration; avoids a second JNI roundtrip through a dedicated AndroidSerialPort entry point.
    const QString trimmedName = portName.trimmed();
    if (trimmedName.isEmpty()) {
        return {};
    }
    for (const QGCSerialPortInfo &info : QGCSerialPortInfo::availablePorts()) {
        if ((info.systemLocation() == trimmedName) || (info.portName() == trimmedName)) {
            return info.supportedBaudRates();
        }
    }
    return {};
}

namespace {
QMutex s_nativeDevicesMutex;
bool s_nativeDevicesValid = false;
QList<QGCSerialPortInfo> s_nativeDevicesCache;
}  // namespace

// JNI USB enumeration is expensive; cache it and invalidate on devicesChanged() (attach/detach/permission-grant).
QList<QGCSerialPortInfo> QGCSerialPortInfo::_nativeDevices()
{
    QMutexLocker locker(&s_nativeDevicesMutex);

    static bool s_invalidationConnected = false;
    if (!s_invalidationConnected) {
        s_invalidationConnected = true;
        SerialPlatform::SerialDevicesNotifier *const notifier = SerialPlatform::SerialDevicesNotifier::instance();
        (void) QObject::connect(notifier, &SerialPlatform::SerialDevicesNotifier::devicesChanged, notifier, []() {
            QMutexLocker invalidationLocker(&s_nativeDevicesMutex);
            s_nativeDevicesValid = false;
        }, Qt::QueuedConnection);
    }

    if (!s_nativeDevicesValid) {
        s_nativeDevicesCache = AndroidSerialPort::availableDevices();
        s_nativeDevicesValid = true;
    }

    return s_nativeDevicesCache;
}

bool QGCSerialPortInfo::_acceptQSerialPortInfo(const QSerialPortInfo& info)
{
    // Keep only kernel TTYs; USB-host paths are already delivered by _nativeDevices().
    return info.systemLocation().startsWith(kDirectUartPathPrefix);
}

bool QGCSerialPortInfo::_platformIsSystemPort(const QGCSerialPortInfo &)
{
    // Android USB-host and direct-UART paths are real devices, never OS peripherals.
    return false;
}

QList<QGCSerialPortInfo::BoardRegExpFallback_t> QGCSerialPortInfo::_additionalDescriptionFallbacks()
{
    const QRegularExpression usbUartRegExp(QStringLiteral("USB UART$"), QRegularExpression::CaseInsensitiveOption);
    if (!usbUartRegExp.isValid()) {
        return {};
    }
    return {{usbUartRegExp, QGCSerialPortInfo::BoardTypeSiKRadio}};
}
