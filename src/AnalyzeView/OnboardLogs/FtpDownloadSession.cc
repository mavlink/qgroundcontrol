#include "FtpDownloadSession.h"

#include <algorithm>

quint64 FtpDownloadSession::begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path)
{
    ++_generation;
    _queue.clear();
    for (const QPointer<OnboardLogEntry>& entry : entries) {
        if (entry) {
            _queue.enqueue(entry);
        }
    }
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path = path;
    _active = !_queue.isEmpty();
    _canceling = false;
    _inFlight = false;
    _refreshRequested = false;
    _resetProgress();
    return _generation;
}

FtpDownloadSession::Cancellation FtpDownloadSession::cancel()
{
    Cancellation cancellation;
    if (_canceling) {
        return cancellation;
    }

    ++_generation;
    cancellation.wasActive = _active;
    cancellation.currentEntry = _currentEntry;
    cancellation.cancelRemoteDownload = _inFlight;
    _queue.clear();

    if (!_active) {
        _currentEntry = nullptr;
        _currentEntrySize = 0;
        _inFlight = false;
        return cancellation;
    }

    _canceling = true;
    return cancellation;
}

bool FtpDownloadSession::finishCancellation()
{
    const bool refreshRequested = _refreshRequested;
    _queue.clear();
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path.clear();
    _active = false;
    _canceling = false;
    _inFlight = false;
    _refreshRequested = false;
    _resetProgress();
    return refreshRequested;
}

void FtpDownloadSession::finish()
{
    _queue.clear();
    _currentEntry = nullptr;
    _currentEntrySize = 0;
    _path.clear();
    _active = false;
    _canceling = false;
    _inFlight = false;
    _resetProgress();
}

void FtpDownloadSession::clear()
{
    ++_generation;
    _refreshRequested = false;
    finish();
}

QPointer<OnboardLogEntry> FtpDownloadSession::takeNext()
{
    if (!_active || _canceling || _currentEntry) {
        return _currentEntry;
    }

    while (!_queue.isEmpty() && !_currentEntry) {
        _currentEntry = _queue.dequeue();
    }
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
    const std::chrono::milliseconds elapsed{_elapsed.elapsed()};
    if (!_active || _canceling || !_currentEntry || !_elapsed.isValid() || (elapsed < minimumInterval)) {
        return std::nullopt;
    }

    const qreal boundedProgress = std::clamp(progress, 0., 1.);
    const uint64_t totalBytes = static_cast<uint64_t>(static_cast<qreal>(_currentEntrySize) * boundedProgress);
    const uint64_t bytesSinceLastUpdate = (totalBytes >= _bytesAtLastUpdate) ? (totalBytes - _bytesAtLastUpdate) : 0;
    const qreal elapsedSeconds = std::chrono::duration<qreal>(elapsed).count();
    const qreal rate = (elapsedSeconds > 0.) ? (bytesSinceLastUpdate / elapsedSeconds) : 0.;
    _rateAverage = (_rateAverage * 0.95) + (rate * 0.05);
    _bytesAtLastUpdate = totalBytes;
    _elapsed.start();

    return ProgressUpdate{totalBytes, _rateAverage, boundedProgress};
}

bool FtpDownloadSession::isCurrent(quint64 generation, const OnboardLogEntry* entry) const
{
    return (generation == _generation) && _active && !_canceling && (!entry || (_currentEntry == entry));
}

void FtpDownloadSession::_resetProgress()
{
    _elapsed.invalidate();
    _bytesAtLastUpdate = 0;
    _rateAverage = 0.;
}
