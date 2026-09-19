#pragma once

#include <functional>

#include <QtCore/QtGlobal>

#include "MonotonicClock.h"

/// Lightweight, non-QObject helper that tracks a windowed data rate.
///
/// Call recordBytes() whenever data passes through and refresh() from a periodic
/// UI timer to expire the rate during silence. Completed windows retain their
/// rate until the next refresh or recordBytes call crosses a window boundary.
///
/// Thread safety: none — use from a single thread.
class DataRateTracker
{
public:
    /// Monotonic microseconds, injectable for deterministic window tests.
    using Clock = std::function<quint64()>;
    explicit DataRateTracker(Clock clock = MonotonicClock::nowUs);

    /// Record incoming/outgoing bytes. Call whenever data passes through.
    void recordBytes(qsizetype bytes);

    /// Advance the rate window without changing cumulative bytes.
    void refresh();

    /// Total bytes recorded since construction or last reset.
    quint64 totalBytes() const { return _totalBytes; }

    /// Rate of the last completed window, in bytes/sec.
    double bytesPerSec() const { return _currentRate; }

    /// Current data rate in KB/s.
    double kBps() const { return _currentRate / 1024.0; }

    /// Whether the rate was recalculated on the last recordBytes()/refresh() call.
    /// Useful for the caller to know when to emit signals.
    bool rateUpdated() const { return _rateUpdated; }

    /// Reset all counters and restart the rate window.
    void reset();

private:
    Clock _clock;
    quint64 _windowStartedUs = 0;
    quint64 _totalBytes = 0;
    qsizetype _windowBytes = 0;
    double _currentRate = 0.0;
    bool _rateUpdated = false;

    static constexpr quint64 kWindowUs = 1000000;
};
