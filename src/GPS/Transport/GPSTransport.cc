#include "GPSTransport.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>

#include <algorithm>
#include <limits>

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

GPSTransport::WriteResult GPSTransport::writeBounded(const uint8_t*, int, QDeadlineTimer)
{
    return {isCancelled() ? WriteStatus::Cancelled : WriteStatus::Unsupported};
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

GPSTransport::WriteResult GPSTransport::write(const uint8_t* buffer, int length)
{
    return writeBounded(buffer, length, QDeadlineTimer(configurationWriteTimeout()));
}

std::chrono::milliseconds GPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(500);
}

bool GPSTransport::waitForSocket(QAbstractSocket* socket, const std::function<bool()>& ready,
                                 QDeadlineTimer deadline) const
{
    if (!socket || isCancelled() || fatalError()) {
        return false;
    }
    if (ready()) {
        return true;
    }
    if (deadline.hasExpired()) {
        return false;
    }

    // Short waitForConnected calls abort DNS/connection progress on timeout.
    QEventLoop loop;
    QTimer cancellation;
    QTimer timeout;
    const auto check = [&]() {
        if (isCancelled() || fatalError() || deadline.hasExpired() || ready()) {
            loop.quit();
        }
    };
    QObject::connect(socket, &QAbstractSocket::stateChanged, &loop, check);
    QObject::connect(socket, &QAbstractSocket::errorOccurred, &loop, check);
    QObject::connect(socket, &QAbstractSocket::readyRead, &loop, check);
    QObject::connect(socket, &QAbstractSocket::bytesWritten, &loop, check);
    QObject::connect(&cancellation, &QTimer::timeout, &loop, check);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.setSingleShot(true);
    timeout.setTimerType(Qt::PreciseTimer);
    cancellation.start(kCancellationPollMs);
    if (!deadline.isForever()) {
        timeout.start(
            static_cast<int>((std::min) (deadline.remainingTime(), qint64((std::numeric_limits<int>::max)()))));
    }
    loop.exec();
    return !isCancelled() && !fatalError() && ready();
}
