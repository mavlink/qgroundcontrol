#include "FtpDownloadSession.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal RateSmoothingTimeConstantSeconds = 2.;

}  // namespace

quint64 FtpDownloadSession::begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path)
{
    _entries.reset(entries);
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path = path;
    _inFlight = false;
    _refreshRequested = false;
    _resetProgress();
    return beginSession(!_entries.isEmpty());
}

FtpDownloadSession::Cancellation FtpDownloadSession::cancel()
{
    Cancellation cancellation;
    if (canceling()) {
        return cancellation;
    }

    invalidateSession();
    cancellation.wasActive = active();
    cancellation.currentEntry = _currentEntry;
    cancellation.cancelRemoteDownload = _inFlight;
    _entries.clear();

    if (!active()) {
        _currentEntry = nullptr;
        _currentEntrySize = 0;
        _inFlight = false;
        return cancellation;
    }

    beginCancellation();
    return cancellation;
}

bool FtpDownloadSession::finishCancellation()
{
    const bool refreshRequested = _refreshRequested;
    _entries.clear();
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path.clear();
    finishSession();
    _inFlight = false;
    _refreshRequested = false;
    _resetProgress();
    return refreshRequested;
}

void FtpDownloadSession::finish()
{
    _entries.clear();
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path.clear();
    finishSession();
    _inFlight = false;
    _resetProgress();
}

void FtpDownloadSession::clear()
{
    invalidateSession();
    _refreshRequested = false;
    finish();
}

QPointer<OnboardLogEntry> FtpDownloadSession::takeNext()
{
    if (!active() || canceling() || _currentEntry) {
        return _currentEntry;
    }

    _currentEntry = _entries.takeNext();
    if (_currentEntry) {
        _currentEntrySize = _currentEntry->size();
        _resetProgress();
        _elapsed.start();
    } else {
        _currentEntrySize = 0;
    }
    return _currentEntry;
}

void FtpDownloadSession::completeCurrent()
{
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _inFlight = false;
    _resetProgress();
}

std::optional<FtpDownloadSession::ProgressUpdate> FtpDownloadSession::updateProgress(
    qreal progress, std::chrono::milliseconds minimumInterval)
{
    if (!active() || canceling() || !_currentEntry || !_elapsed.isValid()) {
        return std::nullopt;
    }
    const std::chrono::milliseconds elapsed{_elapsed.elapsed()};
    if (elapsed < minimumInterval) {
        return std::nullopt;
    }

    const qreal boundedProgress = std::clamp(progress, 0., 1.);
    const uint64_t totalBytes = static_cast<uint64_t>(static_cast<qreal>(_currentEntrySize) * boundedProgress);
    const uint64_t bytesSinceLastUpdate = (totalBytes >= _bytesAtLastUpdate) ? (totalBytes - _bytesAtLastUpdate) : 0;
    const qreal elapsedSeconds = std::chrono::duration<qreal>(elapsed).count();
    const qreal rate = (elapsedSeconds > 0.) ? (static_cast<qreal>(bytesSinceLastUpdate) / elapsedSeconds) : 0.;
    if (_rateInitialized) {
        const qreal smoothingFactor = -std::expm1(-elapsedSeconds / RateSmoothingTimeConstantSeconds);
        _rateAverage += smoothingFactor * (rate - _rateAverage);
    } else {
        _rateAverage = rate;
        _rateInitialized = true;
    }
    _bytesAtLastUpdate = totalBytes;
    _elapsed.start();

    return ProgressUpdate{totalBytes, _rateAverage, boundedProgress};
}

bool FtpDownloadSession::isCurrent(quint64 generation, const OnboardLogEntry* entry) const
{
    return isCurrentGeneration(generation) && (!entry || (_currentEntry == entry));
}

void FtpDownloadSession::_resetProgress()
{
    _elapsed.invalidate();
    _bytesAtLastUpdate = 0;
    _rateAverage = 0.;
    _rateInitialized = false;
}
