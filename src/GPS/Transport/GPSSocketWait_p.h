#pragma once

#include <functional>

#include <QtCore/QDeadlineTimer>

class GPSTransport;
class QAbstractSocket;

namespace GPSSocketWait {
/// Dispatches the owner thread's events until ready() holds, the link fails, the deadline expires, or a stop is
/// requested. A stop request from any thread wakes the wait at once.
bool waitFor(const GPSTransport& transport, QAbstractSocket* socket, const std::function<bool()>& ready,
             QDeadlineTimer deadline);
}  // namespace GPSSocketWait
