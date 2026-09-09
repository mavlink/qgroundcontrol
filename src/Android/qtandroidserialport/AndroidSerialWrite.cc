#include "AndroidSerialWrite.h"

#include <algorithm>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif

namespace AndroidSerialWrite {
Result run(const char* data, int length, qint64 baud, QDeadlineTimer deadline, const Cancelled& cancelled,
           const Step& writeStep)
{
    if (!data || length < 0 || !cancelled || !writeStep) {
        return {Status::InvalidData};
    }
    Result total{Status::Completed};
    while (total.writtenBytes < length) {
        if (cancelled()) {
            total.status = Status::Cancelled;
            return total;
        }
        if (deadline.hasExpired()) {
            total.status = Status::TimedOut;
            return total;
        }
        const int timeout =
            deadline.isForever() ? 50 : static_cast<int>((std::min) (deadline.remainingTime(), qint64(50)));
        if (timeout <= 0) {
            total.status = Status::TimedOut;
            return total;
        }
        // Keep a healthy slow receiver's wire time within a single cancellation interval.
        const qint64 chunkLimit = std::clamp((baud > 0 ? baud : 9600) * timeout / 10000, qint64(1), qint64(512));
        const int count = static_cast<int>((std::min) (qint64(length) - total.writtenBytes, chunkLimit));
        const auto result = writeStep(data + total.writtenBytes, count, timeout);
        if (result.writtenBytes < 0 || result.uncertainBytes < 0 || result.writtenBytes > count ||
            result.uncertainBytes > count - result.writtenBytes) {
            total.status = Status::Error;
            total.uncertainBytes = count;
            return total;
        }
        total.writtenBytes += result.writtenBytes;
        total.uncertainBytes += result.uncertainBytes;
        if (cancelled()) {
            total.status = Status::Cancelled;
            return total;
        }
        if (result.uncertainBytes || (result.status != Status::Completed && result.status != Status::TimedOut)) {
            total.status = result.status == Status::Completed ? Status::Error : result.status;
            return total;
        }
        if (deadline.hasExpired()) {
            total.status = Status::TimedOut;
            return total;
        }
        if (result.status == Status::Completed && result.writtenBytes == 0) {
            total.status = Status::Error;
            return total;
        }
    }
    if (cancelled()) {
        total.status = Status::Cancelled;
    }
    return total;
}

#ifdef Q_OS_UNIX
Result writePosix(int descriptor, const char* data, int length, qint64 baud, QDeadlineTimer deadline,
                  const Cancelled& cancelled)
{
    return run(data, length, baud, deadline, cancelled, [descriptor](const char* bytes, int count, int timeout) {
        const QDeadlineTimer stepDeadline(timeout);
        while (!stepDeadline.hasExpired()) {
            const auto written = ::write(descriptor, bytes, static_cast<size_t>(count));
            if (written > 0) {
                return Result{Status::Completed, written};
            }
            if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                return Result{Status::Error};
            }
            pollfd event{descriptor, POLLOUT, 0};
            const int polled = ::poll(&event, 1, static_cast<int>(stepDeadline.remainingTime()));
            if (polled < 0 && errno != EINTR) {
                return Result{Status::Error};
            }
            if (polled > 0 && (event.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                return Result{Status::Error};
            }
        }
        return Result{Status::TimedOut};
    });
}
#endif
}  // namespace AndroidSerialWrite
