#pragma once

#include <algorithm>

#include <QtCore/QIODevice>

#include "GPSTransport.h"

namespace GPSStreamWrite {
template <typename Wait, typename Confirmed, typename Detail, typename Retire>
GPSWriteResult writeBounded(const GPSTransport& transport, QIODevice* device, const uint8_t* buffer, int length,
                            QDeadlineTimer deadline, qint64 capacity, Wait wait, Confirmed confirmed, Detail detail,
                            Retire retire)
{
    if (transport.isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSWriteStatus::InvalidData};
    }
    if (transport.fatalError() || !device || device->bytesToWrite() != 0) {
        return {GPSWriteStatus::Error, 0, 0, detail()};
    }
    if (length == 0) {
        return {GPSWriteStatus::Completed};
    }
    int accepted = 0;
    GPSWriteStatus status = GPSWriteStatus::Completed;
    while (accepted < length || device->bytesToWrite() > 0) {
        if (transport.isCancelled() || transport.fatalError() || deadline.hasExpired()) {
            break;
        }
        if (accepted < length && device->bytesToWrite() < capacity) {
            const qint64 count =
                device->write(reinterpret_cast<const char*>(buffer) + accepted,
                              (std::min) (qint64(length - accepted), capacity - device->bytesToWrite()));
            if (count < 0) {
                status = GPSWriteStatus::Error;
                break;
            }
            accepted += static_cast<int>(count);
        }
        if (device->bytesToWrite() > 0) {
            wait(deadline);
        }
    }
    if (transport.isCancelled()) {
        status = GPSWriteStatus::Cancelled;
    } else if (transport.fatalError()) {
        status = GPSWriteStatus::Error;
    } else if (deadline.hasExpired()) {
        status = GPSWriteStatus::TimedOut;
    }
    const int written = static_cast<int>(std::clamp(confirmed(accepted), qint64(0), qint64(accepted)));
    const GPSWriteResult result{status, accepted, written, status == GPSWriteStatus::Error ? detail() : QString()};
    if (status != GPSWriteStatus::Completed && accepted > 0) {
        retire();
    }
    return result;
}
}  // namespace GPSStreamWrite
