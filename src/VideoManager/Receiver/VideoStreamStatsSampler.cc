#include "VideoStreamStatsSampler.h"

#include <utility>

#include "RecordingPlan.h"
#include "StreamSupervisor.h"

VideoStreamStatsSampler::VideoStreamStatsSampler(const StreamSupervisor& supervisor, VideoStreamStatsMeter::Clock clock)
    : _supervisor(supervisor)
    , _meter(std::move(clock))
{}

bool VideoStreamStatsSampler::reset()
{
    _meter.reset();
    return _publish(VideoStreamStats());
}

bool VideoStreamStatsSampler::sample(const VideoStreamStatsMeter::Counters& counters, VideoStreamStats stats,
                                     const RecordingPlan* recording)
{
    stats.uptimeMs = _supervisor.uptime().count();
    stats.reconnects = _supervisor.reconnects();
    const qint64 recordingMs = recording ? recording->recordingMs() : -1;
    stats.recordingMs = recordingMs;
    stats.recordingBytes = (recordingMs >= 0) ? recording->bytesWritten() : -1;
    if (!_meter.update(counters, stats)) {
        return false;
    }
    return _publish(stats);
}

bool VideoStreamStatsSampler::_publish(const VideoStreamStats& stats)
{
    if (stats == _stats) {
        return false;
    }
    _stats = stats;
    return true;
}
