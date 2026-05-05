#include "FtpEraseSession.h"

quint64 FtpEraseSession::begin(const QList<QPointer<OnboardLogEntry>>& entries)
{
    ++_generation;
    _queue.clear();
    for (const QPointer<OnboardLogEntry>& entry : entries) {
        if (entry) {
            _queue.enqueue(entry);
        }
    }
    _currentEntry = nullptr;
    _failureCount = 0;
    _active = !_queue.isEmpty();
    _canceling = false;
    return _generation;
}

FtpEraseSession::Cancellation FtpEraseSession::cancel()
{
    Cancellation cancellation;
    cancellation.wasActive = _active;
    if (_canceling) {
        return cancellation;
    }

    ++_generation;

    if (!_active) {
        _queue.clear();
        _currentEntry = nullptr;
        _active = false;
        return cancellation;
    }

    _canceling = true;
    cancellation.currentEntry = _currentEntry;
    cancellation.pendingEntries.reserve(_queue.size());
    while (!_queue.isEmpty()) {
        cancellation.pendingEntries.append(_queue.dequeue());
    }
    return cancellation;
}

void FtpEraseSession::finishCancellation()
{
    _queue.clear();
    _currentEntry = nullptr;
    _failureCount = 0;
    _active = false;
    _canceling = false;
}

uint FtpEraseSession::finish()
{
    const uint failures = _failureCount;
    _queue.clear();
    _currentEntry = nullptr;
    _failureCount = 0;
    _active = false;
    _canceling = false;
    return failures;
}

void FtpEraseSession::clear()
{
    ++_generation;
    (void) finish();
}

QPointer<OnboardLogEntry> FtpEraseSession::takeNext()
{
    if (!_active || _canceling || _currentEntry) {
        return _currentEntry;
    }

    while (!_queue.isEmpty() && !_currentEntry) {
        _currentEntry = _queue.dequeue();
    }
    return _currentEntry;
}

void FtpEraseSession::completeCurrent(bool failed)
{
    if (!_currentEntry) {
        return;
    }
    if (failed) {
        ++_failureCount;
    }
    _currentEntry = nullptr;
}
