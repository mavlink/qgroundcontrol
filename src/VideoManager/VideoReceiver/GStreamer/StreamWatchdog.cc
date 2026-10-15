#include "StreamWatchdog.h"

#include <algorithm>

namespace {

constexpr qint64 kMinTimeoutMs = 1000;
// The decoder may lag the source by up to this many timeouts before it counts as stalled.
constexpr qint64 kDecoderTimeoutFactor = 2;

}  // namespace

qint64 StreamWatchdog::now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void StreamWatchdog::start(uint32_t timeoutSec)
{
    // start(0) would otherwise trip on every check.
    _timeoutMs = std::max<qint64>(std::chrono::milliseconds(std::chrono::seconds(timeoutSec)).count(), kMinTimeoutMs);
    _sourceFrames.store(0, std::memory_order_relaxed);
    _lastSourceFrameMs.store(0, std::memory_order_relaxed);
    _lastSinkFrameMs.store(0, std::memory_order_relaxed);
}

quint64 StreamWatchdog::noteSourceFrame(qint64 nowMs)
{
    _lastSourceFrameMs.store(nowMs, std::memory_order_relaxed);
    return _sourceFrames.fetch_add(1, std::memory_order_relaxed) + 1;
}

bool StreamWatchdog::noteSinkFrame(qint64 nowMs)
{
    return _lastSinkFrameMs.exchange(nowMs, std::memory_order_relaxed) == 0;
}

StreamWatchdog::Result StreamWatchdog::check(qint64 nowMs, bool decoding)
{
    qint64 lastSourceFrameMs = _lastSourceFrameMs.load(std::memory_order_relaxed);
    if (lastSourceFrameMs == 0) {
        lastSourceFrameMs = nowMs;
        _lastSourceFrameMs.store(nowMs, std::memory_order_relaxed);
    }

    const qint64 sourceSilentMs = nowMs - lastSourceFrameMs;
    if (sourceSilentMs > _timeoutMs) {
        return {.stall = Stall::Source, .silentMs = sourceSilentMs};
    }

    // Decoding is only reported once a frame reached the sink, so the last sink frame time is valid.
    if (decoding) {
        const qint64 sinkSilentMs = nowMs - _lastSinkFrameMs.load(std::memory_order_relaxed);
        if (sinkSilentMs > (_timeoutMs * kDecoderTimeoutFactor)) {
            return {.stall = Stall::Decoder, .silentMs = sinkSilentMs};
        }
    }

    return {};
}

std::chrono::seconds StreamWatchdog::nextReconnectDelay()
{
    if (_sourceFrames.exchange(0, std::memory_order_relaxed) > 0) {
        _reconnectBackoff.reset();
    }
    return std::chrono::duration_cast<std::chrono::seconds>(_reconnectBackoff.next());
}
