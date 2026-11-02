#pragma once

#include "VideoStreamStats.h"
#include "VideoStreamStatsMeter.h"

class RecordingPlan;
class StreamSupervisor;

/// \brief The VideoStreamStats a receiver publishes, sampled about once a second while it streams.
///
/// The backend fills the fields only it can measure and counts what VideoStreamStatsMeter turns into rates; the sampler
/// adds the fields every backend reports alike: uptime and reconnects from the receiver's StreamSupervisor, and the
/// recording's time and size from its RecordingPlan. It keeps the stats it last produced, so a receiver publishes
/// changes only.
class VideoStreamStatsSampler
{
public:
    explicit VideoStreamStatsSampler(const StreamSupervisor& supervisor, VideoStreamStatsMeter::Clock clock = {});

    /// The stats the last sample() or reset() produced.
    const VideoStreamStats& stats() const { return _stats; }

    /// A session ended: the rates start over and the stats go invalid. True when that changed them.
    bool reset();

    /// One sample. @p stats has the backend's fields, starting from stats(); @p counters its counters since the session
    /// started. @p recording: what the receiver records, if anything. True when the stats changed. The first sample
    /// after reset() only sets the meter's baseline.
    bool sample(const VideoStreamStatsMeter::Counters& counters, VideoStreamStats stats,
                const RecordingPlan* recording);

private:
    bool _publish(const VideoStreamStats& stats);

    const StreamSupervisor& _supervisor;
    VideoStreamStatsMeter _meter;
    VideoStreamStats _stats;
};
