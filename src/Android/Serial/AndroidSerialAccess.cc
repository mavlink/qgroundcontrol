#include "AndroidSerialAccess.h"

#include <atomic>

#include "AndroidSerialJni.h"
#include "AndroidSerialPortInfo.h"
#include "QGCLoggingCategory.h"

// The category of the port list, by the name that it has always had.
QGC_LOGGING_CATEGORY(AndroidSerialPortInfoLog, "Android.AndroidSerialPortInfo")

namespace {
std::atomic<bool> s_usePosixSerial{false};
}  // namespace

void AndroidSerialAccess::setUsePosixSerial(bool use)
{
    s_usePosixSerial.store(use);
    qCDebug(AndroidSerialPortInfoLog) << "Serial ports:" << (use ? "USB and POSIX" : "USB");
}

bool AndroidSerialAccess::usePosixSerial()
{
    return s_usePosixSerial.load();
}

bool AndroidSerialAccess::hasPosixSerialPorts()
{
    return !QSerialPortInfo::posixPorts(QStringLiteral("/dev"), QStringLiteral("/sys/class/tty")).isEmpty();
}

bool AndroidSerialAccess::hasUsbPermission(const QString& portName)
{
    const QString location = QSerialPortInfo::portNameToSystemLocation(portName);
    return QSerialPortInfo::isUsbLocation(location) && AndroidSerial::hasPermission(location);
}

bool AndroidSerialAccess::requestUsbPermission(const QString& portName)
{
    const QString location = QSerialPortInfo::portNameToSystemLocation(portName);
    return QSerialPortInfo::isUsbLocation(location) && !AndroidSerial::hasPermission(location) &&
           AndroidSerial::requestPermission(location);
}

bool AndroidSerialAccess::isUsbPermissionDenied(const QString& portName)
{
    const QString location = QSerialPortInfo::portNameToSystemLocation(portName);
    return QSerialPortInfo::isUsbLocation(location) && AndroidSerial::isPermissionDenied(location);
}
