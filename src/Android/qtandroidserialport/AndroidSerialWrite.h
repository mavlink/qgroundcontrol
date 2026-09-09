#pragma once

#include <QtCore/QDeadlineTimer>

#include <functional>

namespace AndroidSerialWrite {
enum class Status
{
    Completed,
    TimedOut,
    Cancelled,
    Error,
    InvalidData
};

struct Result
{
    Status status = Status::Error;
    qint64 writtenBytes = 0;
    qint64 uncertainBytes = 0;
};

using Cancelled = std::function<bool()>;
using Step = std::function<Result(const char*, int, int)>;

/// Each step is synchronous and bounded by its timeout. Uncertain bytes are never retried.
Result run(const char* data, int length, qint64 baud, QDeadlineTimer deadline, const Cancelled& cancelled,
           const Step& writeStep);

#ifdef Q_OS_UNIX
/// The descriptor must already be nonblocking; ownership remains with the caller.
Result writePosix(int descriptor, const char* data, int length, qint64 baud, QDeadlineTimer deadline,
                  const Cancelled& cancelled);
#endif
}  // namespace AndroidSerialWrite
