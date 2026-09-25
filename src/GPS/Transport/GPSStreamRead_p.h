#pragma once

#include <algorithm>
#include <chrono>

#include <QtCore/QDeadlineTimer>

#include "GPSTransport.h"

namespace GPSStreamRead {
/// Shared read contract: cancellation, then argument validation, then an unavailable link. Input that wait()
/// reports as buffered is delivered even after the link fails; otherwise failure() describes a failed link and
/// explains a failed take().
template <typename Unavailable, typename Wait, typename Take, typename Failure>
GPSReadResult readBounded(const GPSTransport& transport, uint8_t* buffer, int length, std::chrono::milliseconds timeout,
                          Unavailable unavailable, Wait wait, Take take, Failure failure)
{
    if (transport.isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSReadStatus::InvalidData};
    }
    if (unavailable()) {
        return failure();
    }
    if (length == 0) {
        return transport.fatalError() ? failure() : GPSReadResult{GPSReadStatus::Data};
    }
    if (!wait(QDeadlineTimer((std::max) (timeout, std::chrono::milliseconds::zero()), Qt::PreciseTimer))) {
        if (transport.isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        return transport.fatalError() ? failure() : GPSReadResult{GPSReadStatus::TimedOut};
    }
    const qint64 count = take(buffer, length);
    if (count < 0) {
        return {GPSReadStatus::Error, 0, failure().detail};
    }
    return {GPSReadStatus::Data, static_cast<int>(count)};
}
}  // namespace GPSStreamRead
