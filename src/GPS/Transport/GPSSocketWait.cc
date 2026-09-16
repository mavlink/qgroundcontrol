#include <algorithm>
#include <limits>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>

#include "GPSSocketWait_p.h"
#include "GPSTransport.h"

bool gpsWaitForSocket(const GPSTransport& transport, QAbstractSocket* socket, const std::function<bool()>& ready,
                      QDeadlineTimer deadline, int cancellationPollMs)
{
    if (!socket || transport.isCancelled() || transport.fatalError()) {
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
        if (transport.isCancelled() || transport.fatalError() || deadline.hasExpired() || ready()) {
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
    cancellation.start(cancellationPollMs);
    if (!deadline.isForever()) {
        timeout.start(
            static_cast<int>((std::min) (deadline.remainingTime(), qint64((std::numeric_limits<int>::max)()))));
    }
    loop.exec();
    return !transport.isCancelled() && !transport.fatalError() && ready();
}
