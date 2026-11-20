// The registry of the ports that Java reports to. It is apart from the JNI calls in AndroidSerialJni.cc so that it
// also builds for the host, where the tests of the serial port run on it in place of a fake.

#include <QtCore/QHash>
#include <QtCore/QReadWriteLock>

#include "AndroidSerialJni.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialLog, "Android.AndroidSerial");

namespace AndroidSerial {

// Java gets a handle that is never used twice instead of a C++ pointer, so a report that arrives after its port
// has closed finds nothing. A report looks the listener up and calls it under the read lock; unregisterPort()
// takes the write lock, and so returns only when no report is still using the listener. The pattern follows Qt
// Bluetooth's LowEnergyNotificationHub.
static QReadWriteLock s_listenersLock;
static QHash<PortHandle, PortListener*> s_listeners;
static PortHandle s_lastHandle = kNoPort;

PortHandle registerPort(PortListener* listener)
{
    if (!listener) {
        qCWarning(AndroidSerialLog) << "registerPort called with null listener";
        return kNoPort;
    }

    QWriteLocker locker(&s_listenersLock);

    const PortHandle handle = ++s_lastHandle;
    s_listeners.insert(handle, listener);
    return handle;
}

void unregisterPort(PortHandle handle)
{
    QWriteLocker locker(&s_listenersLock);
    s_listeners.remove(handle);
}

bool deliverNewData(PortHandle handle, const char* bytes, int length)
{
    QReadLocker locker(&s_listenersLock);
    PortListener* const listener = s_listeners.value(handle, nullptr);
    if (!listener) {
        qCDebug(AndroidSerialLog) << "nativeDeviceNewData: stale token, port already closed";
        return true;
    }

    return listener->newDataArrived(bytes, length);
}

void deliverBytesWritten(PortHandle handle, int count)
{
    QReadLocker locker(&s_listenersLock);
    PortListener* const listener = s_listeners.value(handle, nullptr);
    if (!listener) {
        qCDebug(AndroidSerialLog) << "nativeDeviceBytesWritten: stale token, port already closed";
        return;
    }

    listener->bytesWritten(count);
}

void deliverResourceError(PortHandle handle, ResourceErrorReason reason, const QString& detail)
{
    QReadLocker locker(&s_listenersLock);
    PortListener* const listener = s_listeners.value(handle, nullptr);
    if (!listener) {
        qCDebug(AndroidSerialLog) << "nativeDeviceResourceError: stale token, port already closed";
        return;
    }

    listener->resourceErrorArrived(reason, detail);
}

}  // namespace AndroidSerial
