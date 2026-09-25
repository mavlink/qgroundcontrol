#include "GPSTransport.h"

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSTransportLog, "GPS.Transport.GPSTransport")

GPSTransport::GPSTransport(std::stop_token stopToken)
    : _stopToken(std::move(stopToken))
{
    qCDebug(GPSTransportLog) << this;
}

GPSTransport::~GPSTransport()
{
    qCDebug(GPSTransportLog) << this;
}

GPSWriteResult GPSTransport::write(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSWriteStatus::InvalidData};
    }
    if (length == 0) {
        return {GPSWriteStatus::Completed};
    }
    if (deadline.hasExpired()) {
        return {GPSWriteStatus::TimedOut};
    }
    const QDeadlineTimer cap(configurationWriteTimeout(), Qt::PreciseTimer);
    return writeData(buffer, length, std::min(deadline, cap));
}

std::chrono::milliseconds GPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(500);
}
