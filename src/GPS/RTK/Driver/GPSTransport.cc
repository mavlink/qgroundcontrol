#include "GPSTransport.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSTransportLog, "GPS.RTK.Driver.GPSTransport")

GPSTransport::GPSTransport(const std::atomic_bool& requestStop)
    : _requestStop(requestStop)
{
    qCDebug(GPSTransportLog) << this;
}

GPSTransport::~GPSTransport()
{
    qCDebug(GPSTransportLog) << this;
}
