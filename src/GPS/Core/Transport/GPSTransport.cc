#include "GPSTransport.h"

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSTransportLog, "GPS.Transport.GPSTransport")

GPSTransport::GPSTransport(GPSCancelToken cancelToken)
    : _cancelToken(std::move(cancelToken))
{
    qCDebug(GPSTransportLog) << this;
}

GPSTransport::~GPSTransport()
{
    qCDebug(GPSTransportLog) << this;
}

GPSWriteResult GPSTransport::write(QByteArrayView bytes, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (bytes.isEmpty()) {
        return {GPSWriteStatus::Completed};
    }
    if (deadline.hasExpired()) {
        return {GPSWriteStatus::TimedOut};
    }
    const QDeadlineTimer cap(configurationWriteTimeout(), Qt::PreciseTimer);
    return writeData(bytes, (std::min) (deadline, cap));
}

std::chrono::milliseconds GPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(500);
}

bool GPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && fixedBaudrate() != 0 && baudrate == fixedBaudrate();
}
