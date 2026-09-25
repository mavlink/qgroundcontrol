#include <algorithm>
#include <limits>
#include <stop_token>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>

#include "GPSSocketWait_p.h"
#include "GPSTransport.h"

namespace GPSSocketWait {
bool waitFor(const GPSTransport& transport, QAbstractSocket* socket, const std::function<bool()>& ready,
             QDeadlineTimer deadline)
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
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.setSingleShot(true);
    timeout.setTimerType(Qt::PreciseTimer);
    if (!deadline.isForever()) {
        timeout.start(
            static_cast<int>((std::min) (deadline.remainingTime(), qint64((std::numeric_limits<int>::max)()))));
    }
    // request_stop() runs this on the stopping thread, so it only posts. Declared after the loop so it is destroyed
    // first: the destructor waits for a running invocation, and the loop's destructor discards an unhandled post.
    const std::stop_callback wake(transport.stopToken(), [&loop]() {
        QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
    });
    loop.exec();
    return !transport.isCancelled() && !transport.fatalError() && ready();
}
}  // namespace GPSSocketWait
