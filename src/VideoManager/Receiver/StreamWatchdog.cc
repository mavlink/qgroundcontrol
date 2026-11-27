#include "StreamWatchdog.h"

#include <algorithm>

#include "MonotonicClock.h"

namespace {

constexpr qint64 kMinTimeoutMs = 1000;

}  // namespace

qint64 StreamWatchdog::now()
{
    return MonotonicClock::nowMs();
}

void StreamWatchdog::start(std::chrono::seconds timeout, std::optional<std::chrono::milliseconds> firstFrameGrace,
                           SinkLimits sink)
{
    // A zero timeout would otherwise trip on every check.
    _timeoutMs = std::max<qint64>(std::chrono::milliseconds(timeout).count(), kMinTimeoutMs);
    _firstFrameGraceMs.reset();
    if (firstFrameGrace) {
        _firstFrameGraceMs = std::max<qint64>(firstFrameGrace->count(), 0);
    }
    _sinkTimeoutMs = _timeoutMs * std::max(sink.timeouts, 1);
    _firstSinkFrameGraceMs = std::max<qint64>(sink.firstFrameGrace.count(), 0);
    _sourceFrames.store(0, std::memory_order_relaxed);
    _lastSourceFrameMs.store(0, std::memory_order_relaxed);
    _lastSinkFrameMs.store(0, std::memory_order_relaxed);
    _sinkFrameSeen.store(false, std::memory_order_relaxed);
}

void StreamWatchdog::start(std::chrono::seconds timeout, std::optional<std::chrono::milliseconds> firstFrameGrace)
{
    start(timeout, firstFrameGrace, SinkLimits{});
}

quint64 StreamWatchdog::noteSourceFrame(qint64 nowMs)
{
    // The time before the count, which publishes it: check() reads them the other way round.
    _lastSourceFrameMs.store(nowMs, std::memory_order_relaxed);
    return _sourceFrames.fetch_add(1, std::memory_order_release) + 1;
}

bool StreamWatchdog::noteSinkFrame(qint64 nowMs)
{
    _lastSinkFrameMs.store(nowMs, std::memory_order_relaxed);
    return !_sinkFrameSeen.exchange(true, std::memory_order_relaxed);
}

void StreamWatchdog::excuseSinkSilence(qint64 nowMs)
{
    _lastSinkFrameMs.store(nowMs, std::memory_order_relaxed);
}

void StreamWatchdog::startSinkClock(qint64 nowMs)
{
    _lastSinkFrameMs.store(nowMs, std::memory_order_relaxed);
    _sinkFrameSeen.store(false, std::memory_order_relaxed);
}

StreamWatchdog::Result StreamWatchdog::check(qint64 nowMs, bool displaying)
{
    // The count before the time: a counted frame's time is then the one read, or a later one. Read the other way
    // round, a first frame noted in between would get the short timeout measured from before it, a false stall.
    const bool firstFrameSeen = _sourceFrames.load(std::memory_order_acquire) > 0;
    qint64 lastSourceFrameMs = 0;
    // The first check starts the clock, unless a frame's time got there first.
    if (_lastSourceFrameMs.compare_exchange_strong(lastSourceFrameMs, nowMs, std::memory_order_relaxed)) {
        lastSourceFrameMs = nowMs;
    }

    if (firstFrameSeen || _firstFrameGraceMs) {
        const qint64 sourceTimeoutMs = firstFrameSeen ? _timeoutMs : (_timeoutMs + *_firstFrameGraceMs);
        const qint64 sourceSilentMs = nowMs - lastSourceFrameMs;
        if (sourceSilentMs > sourceTimeoutMs) {
            return {.stall = Stall::Source, .silentMs = sourceSilentMs};
        }
    }

    if (displaying) {
        // The flag before the time, as for the source: a first frame noted in between gets the grace, not a false
        // stall.
        const qint64 sinkTimeoutMs =
            _sinkTimeoutMs + (_sinkFrameSeen.load(std::memory_order_relaxed) ? 0 : _firstSinkFrameGraceMs);
        const qint64 sinkSilentMs = nowMs - _lastSinkFrameMs.load(std::memory_order_relaxed);
        if (sinkSilentMs > sinkTimeoutMs) {
            return {.stall = Stall::Decoder, .silentMs = sinkSilentMs};
        }
    }

    return {};
}
