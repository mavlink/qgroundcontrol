#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QtGlobal>

/// Lightweight, non-QObject helper that tracks a rolling data rate.
///
/// Call recordBytes() whenever data passes through. The rate window is
/// kWindowMs milliseconds; once elapsed time crosses that threshold,
/// _currentRate is recalculated and rateUpdated() returns true for that call.
///
/// Thread safety: none — use from a single thread.
class DataRateTracker
{
public:
    DataRateTracker();

    /// Record incoming/outgoing bytes. Call whenever data passes through.
    void recordBytes(qsizetype bytes);

    /// Total bytes recorded since construction or last reset.
    quint64 totalBytes() const { return _totalBytes; }

    /// Current data rate in bytes/sec. Recalculated on each recordBytes() call.
    /// Returns 0 if no data has been recorded since the last window expired.
    double bytesPerSec() const { return _currentRate; }

    /// Current data rate in KB/s.
    double kBps() const { return _currentRate / 1024.0; }

    /// Whether the rate was recalculated on the last recordBytes() call.
    /// Useful for the caller to know when to emit signals.
    bool rateUpdated() const { return _rateUpdated; }

    /// Reset all counters and restart the elapsed timer.
    void reset();

private:
    QElapsedTimer _timer;
    quint64 _totalBytes = 0;
    qsizetype _windowBytes = 0;
    double _currentRate = 0.0;
    bool _rateUpdated = false;

    static constexpr qint64 kWindowMs = 1000;
};
