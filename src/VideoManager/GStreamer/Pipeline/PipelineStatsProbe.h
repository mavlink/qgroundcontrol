#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <gst/gst.h>

#include "VideoStreamStatsMeter.h"

struct VideoStreamStats;

/// \brief What PipelineSession measures of its stream for VideoStreamStats, apart from its branches.
///
/// The tee's probe notes every buffer that reaches the tee: its size, and a keyframe's timestamp. sample() adds what
/// the pipeline tells when asked: the RTP jitter buffers' counters, the latency and the codec.
class PipelineStatsProbe
{
public:
    /// Streaming thread (the tee's sink pad, so one at a time): @p buffer reached the tee.
    void noteBuffer(GstBuffer* buffer);

    /// Fills the bytes and RTP counters of @p counters, and the RTP, keyframe interval, latency and codec fields of
    /// @p stats, for the stream @p source feeds into @p tee in @p pipeline. Any of them may be null.
    void sample(GstElement* pipeline, GstElement* source, GstElement* tee, VideoStreamStatsMeter::Counters& counters,
                VideoStreamStats& stats) const;

    /// The latency @p pipeline reports; false when it doesn't answer the latency query.
    static bool queryLatency(GstElement* pipeline, GstClockTime& min, GstClockTime& max);

private:
    void _noteKeyframe(GstClockTime time);

    /// Keyframes whose timestamps the keyframe interval averages over: on a stream without a jitter buffer they are
    /// arrival times, which a busy sender or network bunches up, and an average across several keyframes evens that
    /// out.
    static constexpr std::size_t kKeyframeWindow = 5;

    /// Compressed bytes that reached the tee.
    std::atomic<quint64> _bytesReceived = 0;
    /// Timestamps of the last keyframes at the tee, oldest first. Only the tee's streaming thread uses them.
    std::array<GstClockTime, kKeyframeWindow> _keyframeTimes{};
    std::size_t _keyframeTimeCount = 0;
    /// The average time between the keyframes in _keyframeTimes; GST_CLOCK_TIME_NONE before the second.
    std::atomic<GstClockTime> _keyframeInterval = GST_CLOCK_TIME_NONE;
};
