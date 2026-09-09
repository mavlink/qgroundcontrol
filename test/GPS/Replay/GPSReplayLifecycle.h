#pragma once

#include <optional>

#include "GPSRecordingFormat.h"

struct GPSReplayTermination
{
    enum class Reason
    {
        CaptureExhausted,
        Closed,
        Cancelled,
        ReadFailure,
        OpenFailure,
        WriteFailure
    };
    Reason reason = Reason::CaptureExhausted;
    quint64 atUs = 0;  ///< Offset in the recording, independent of the adapter clock origin.
    GPSReadStatus readStatus = GPSReadStatus::InvalidData;
    std::optional<GPSOpenStatus> openStatus = std::nullopt;
    std::optional<GPSWriteStatus> writeStatus = std::nullopt;
    int recordedValue = 0;
    bool operator==(const GPSReplayTermination&) const = default;
};
Q_DECLARE_METATYPE(GPSReplayTermination)

/// One terminal result per recorded connection attempt, shared by blocking and event-loop replay.
class GPSReplayLifecycle
{
public:
    bool consume(const GPSRecordingEvent& event);
    bool exhausted(quint64 atUs);

    const std::optional<GPSReplayTermination>& termination() const { return _termination; }

    quint64 terminationCount() const { return _terminationCount; }

private:
    bool _finish(GPSReplayTermination result);
    std::optional<GPSReplayTermination> _termination = std::nullopt;
    quint64 _terminationCount = 0;
};
