#include "GPSTransport.h"

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSTransportLog, "GPS.Transport.GPSTransport")

GPSTransport::GPSTransport(const std::atomic_bool& requestStop) : _requestStop(requestStop)
{
    qCDebug(GPSTransportLog) << this;
}

GPSTransport::~GPSTransport()
{
    qCDebug(GPSTransportLog) << this;
}

GPSWriteResult GPSTransport::writeBounded(const uint8_t*, int, QDeadlineTimer)
{
    return {isCancelled() ? GPSWriteStatus::Cancelled : GPSWriteStatus::Unsupported};
}

GPSWriteResult GPSTransport::writeConfiguration(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSWriteStatus::InvalidData};
    }
    if (deadline.hasExpired()) {
        return {GPSWriteStatus::TimedOut};
    }
    const QDeadlineTimer cap(configurationWriteTimeout(), Qt::PreciseTimer);
    return writeBounded(buffer, length, std::min(deadline, cap));
}

std::chrono::milliseconds GPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(500);
}
