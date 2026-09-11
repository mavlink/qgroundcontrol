#include "GPSSocketWait.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>

#include <algorithm>
#include <limits>

bool gpsWaitForSocket(QAbstractSocket* socket, const std::function<bool()>& ready,
                      const std::function<bool()>& cancelled, const std::function<bool()>& failed,
                      QDeadlineTimer deadline)
{
    if (!socket || cancelled() || failed()) {
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
        if (cancelled() || failed() || ready()) {
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
    cancellation.start(50);
    if (!deadline.isForever()) {
        timeout.start(static_cast<int>((std::min) (deadline.remainingTime(), qint64(std::numeric_limits<int>::max()))));
    }
    loop.exec();
    return !cancelled() && !failed() && ready();
}
