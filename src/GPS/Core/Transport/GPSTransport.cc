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
        return writeExpired(bytes);
    }
    const QDeadlineTimer cap(configurationWriteTimeout(), Qt::PreciseTimer);
    return writeData(bytes, (std::min) (deadline, cap));
}

GPSWriteResult GPSTransport::writeExpired(QByteArrayView)
{
    return {GPSWriteStatus::TimedOut};
}

std::chrono::milliseconds GPSTransport::configurationWriteTimeout() const
{
    return DEFAULT_WRITE_TIMEOUT;
}

bool GPSTransport::setBaudRate(unsigned baudRate)
{
    return !isCancelled() && !fatalError() && fixedBaudRate() != 0 && baudRate == fixedBaudRate();
}
