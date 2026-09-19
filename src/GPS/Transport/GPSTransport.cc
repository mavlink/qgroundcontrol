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

std::chrono::milliseconds GPSTransport::correctionWriteTimeout(int) const
{
    return std::chrono::milliseconds(200);
}

std::chrono::milliseconds GPSTransport::serialCorrectionWriteTimeout(int length, qint64 baud)
{
    if (baud <= 0 || length <= 0) {
        return std::chrono::milliseconds(200);
    }
    const qint64 wireBitsMs = qint64(length) * 10000;
    const qint64 wireTimeMs = wireBitsMs / baud + (wireBitsMs % baud != 0);
    // A complete 8N1 frame at 9600 baud needs over a second; still service receive before its idle deadline.
    return std::chrono::milliseconds((std::min) (wireTimeMs + 100, qint64(3000)));
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
