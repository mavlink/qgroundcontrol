#include "GPSReplayLifecycle.h"

#include <cerrno>

bool GPSReplayLifecycle::_finish(GPSReplayTermination result)
{
    if (_termination) {
        return false;
    }
    _termination = result;
    ++_terminationCount;
    return true;
}

bool GPSReplayLifecycle::consume(const GPSRecordingEvent& event)
{
    using K = GPSRecordingEvent::Kind;
    using R = GPSReplayTermination::Reason;
    GPSReplayTermination result{.atUs = event.atUs, .recordedValue = event.value()};
    switch (event.kind()) {
        case K::Open:
        case K::OpenError: {
            _termination.reset();
            const auto status =
                event.openStatus().value_or(event.kind() == K::Open ? GPSOpenStatus::Opened : GPSOpenStatus::Error);
            if (status == GPSOpenStatus::Opened) {
                return false;
            }
            result.reason = status == GPSOpenStatus::Cancelled ? R::Cancelled : R::OpenFailure;
            result.readStatus = status == GPSOpenStatus::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error;
            result.openStatus = status;
            break;
        }
        case K::Close:
        case K::Disconnect:
            result.readStatus = event.readStatus().value_or(event.value() == -ECANCELED ? GPSReadStatus::Cancelled
                                                                                        : GPSReadStatus::Closed);
            result.reason = result.readStatus == GPSReadStatus::Cancelled ? R::Cancelled : R::Closed;
            break;
        case K::Cancel:
        case K::ReadError:
            result.readStatus = event.readStatus().value_or(event.kind() == K::Cancel ? GPSReadStatus::Cancelled
                                                                                      : GPSReadStatus::Error);
            result.reason = result.readStatus == GPSReadStatus::Cancelled ? R::Cancelled : R::ReadFailure;
            break;
        case K::WriteError:
            if (!event.fatal()) {
                return false;
            }
            result.writeStatus = GPSWriteStatus::Error;
            result.reason = R::WriteFailure;
            result.readStatus = GPSReadStatus::Error;
            break;
        case K::BoundedWrite:
            if (!event.writeResult() || (!event.fatal() && event.writeResult()->status != GPSWriteStatus::Cancelled)) {
                return false;
            }
            result.writeStatus = event.writeResult()->status;
            result.reason = event.writeResult()->status == GPSWriteStatus::Cancelled ? R::Cancelled : R::WriteFailure;
            result.readStatus = result.reason == R::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error;
            break;
        default:
            return false;
    }
    return _finish(result);
}

bool GPSReplayLifecycle::exhausted(quint64 atUs)
{
    return _finish({.reason = GPSReplayTermination::Reason::CaptureExhausted, .atUs = atUs});
}
