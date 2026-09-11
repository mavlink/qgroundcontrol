#include "GPSSerialDiscovery.h"

#include "QGCLoggingCategory.h"
QGC_LOGGING_CATEGORY(GPSSerialDiscoveryLog, "GPS.Receiver.GPSSerialDiscovery")

GPSSerialDiscovery::GPSSerialDiscovery(QObject* parent)
    : QObject(parent)
{
    qCDebug(GPSSerialDiscoveryLog) << this;
}

GPSSerialDiscovery::~GPSSerialDiscovery()
{
    qCDebug(GPSSerialDiscoveryLog) << this;
}
