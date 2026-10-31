#include "VideoStreamStatsMeter.h"

#include <chrono>
#include <utility>

#include "VideoStreamStats.h"

namespace {

/// How much a cumulative counter grew from @p before to @p now; one below its previous value restarted from zero.
quint64 statsCounterGrowth(quint64 now, quint64 before)
{
    return (now >= before) ? (now - before) : now;
}

qint64 steadyClockMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

VideoStreamStatsMeter::VideoStreamStatsMeter(Clock clock)
    : _clock(clock ? std::move(clock) : Clock(steadyClockMs))
{}

void VideoStreamStatsMeter::reset()
{
    _lastSampleMs.reset();
    _last = {};
    _framesDisplayed = 0;
    _bytesReceived = 0;
    _framesDropped = 0;
    _packetsLost = 0;
    _window.clear();
}

bool VideoStreamStatsMeter::update(const Counters& counters, VideoStreamStats& stats)
{
    const qint64 nowMs = _clock();
    if (_lastSampleMs && (nowMs <= *_lastSampleMs)) {
        return false;
    }

    // Totals count from reset(), the baseline sample included.
    _framesDisplayed += statsCounterGrowth(counters.framesDisplayed, _last.framesDisplayed);
    _bytesReceived += statsCounterGrowth(counters.bytesReceived, _last.bytesReceived);
    const quint64 framesDropped = statsCounterGrowth(counters.framesDropped, _last.framesDropped);
    _framesDropped += framesDropped;
    quint64 packetsReceived = 0;
    quint64 packetsLost = 0;
    if (counters.rtp) {
        const RtpCounters last = _last.rtp.value_or(RtpCounters{});
        packetsReceived = statsCounterGrowth(counters.rtp->packetsReceived, last.packetsReceived);
        packetsLost = statsCounterGrowth(counters.rtp->packetsLost, last.packetsLost);
        _packetsLost += packetsLost;
    }

    const std::optional<qint64> lastSampleMs = std::exchange(_lastSampleMs, nowMs);
    _last = counters;
    _window.push_back({.ms = nowMs, .framesDisplayed = _framesDisplayed, .bytesReceived = _bytesReceived});
    if (_window.size() > static_cast<std::size_t>(kRateWindowIntervals) + 1) {
        _window.pop_front();
    }
    if (!lastSampleMs) {
        return false;
    }

    const double seconds = static_cast<double>(nowMs - *lastSampleMs) / 1000.0;
    const RatePoint& oldest = _window.front();
    const double windowSeconds = static_cast<double>(nowMs - oldest.ms) / 1000.0;
    stats.valid = true;
    stats.fps = static_cast<double>(_framesDisplayed - oldest.framesDisplayed) / windowSeconds;
    stats.bitrateKbps = static_cast<double>(_bytesReceived - oldest.bytesReceived) * 8.0 / 1000.0 / windowSeconds;
    stats.framesDropped = static_cast<qint64>(_framesDropped);
    stats.droppedFps = static_cast<double>(framesDropped) / seconds;
    if (counters.rtp) {
        const quint64 packetsDue = packetsReceived + packetsLost;
        stats.packetsLost = static_cast<qint64>(_packetsLost);
        stats.lossPercent =
            (packetsDue > 0) ? (100.0 * static_cast<double>(packetsLost) / static_cast<double>(packetsDue)) : 0.0;
    } else {
        stats.packetsLost = -1;
        stats.lossPercent = -1.0;
    }
    return true;
}
