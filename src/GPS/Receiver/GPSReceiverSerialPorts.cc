#include "GPSReceiverSerialPorts.h"

#include <utility>

#ifndef QGC_NO_SERIAL_LINK
#include "GPSReceiverSystemSerialPorts.h"
#endif

GPSReceiverSerialPorts::GPSReceiverSerialPorts(QObject* parent)
    : QObject(parent)
{}

GPSReceiverSerialPorts* GPSReceiverSerialPorts::create([[maybe_unused]] SerialPortManager* manager, QObject* parent)
{
#ifdef QGC_NO_SERIAL_LINK
    return new GPSReceiverSerialPorts(parent);
#else
    return new GPSReceiverSystemSerialPorts(manager, parent);
#endif
}

void GPSReceiverSerialPorts::setEntries(QList<GPSSerialPortEntry> entries)
{
    if (entries != _entries) {
        _entries = std::move(entries);
        emit entriesChanged();
    }
}
