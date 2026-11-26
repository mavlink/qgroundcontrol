#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>

#include <QtCore/QtTypes>

/// The time between a stream's keyframes, from their timestamps, for VideoStreamStats::keyframeIntervalMs.
///
/// Averaged over the last kWindow keyframes: on a stream without a jitter buffer the timestamps are arrival times,
/// which a busy sender or network bunches up, and an average across several keyframes evens that out.
///
/// Threading: one thread notes the keyframes, the one the stream arrives on; any thread reads the interval.
class KeyframeIntervalMeter
{
public:
    static constexpr std::size_t kWindow = 5;

    /// A keyframe of timestamp @p time arrived. One at the last keyframe's time is the same keyframe, as the parameter
    /// sets that arrive in a buffer of their own; one before it starts over, as after a new segment or a sender that
    /// restarted.
    void note(std::chrono::nanoseconds time);

    /// The average time between the keyframes noted, in ms; -1 before the second.
    double intervalMs() const;

private:
    /// The last keyframes' timestamps, oldest first. Only the noting thread uses them.
    std::array<std::chrono::nanoseconds, kWindow> _times{};
    std::size_t _count = 0;
    /// -1 before the second keyframe.
    std::atomic<qint64> _intervalNs = -1;
};
