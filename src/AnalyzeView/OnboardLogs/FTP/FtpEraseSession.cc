#include "FtpEraseSession.h"

quint64 FtpEraseSession::begin(const QList<QPointer<OnboardLogEntry>>& entries)
{
    _entries.reset(entries);
    _currentEntry = nullptr;
    _hasCurrent = false;
    _failureCount = 0;
    return beginSession(!_entries.isEmpty());
}

FtpEraseSession::Cancellation FtpEraseSession::cancel()
{
    Cancellation cancellation;
    cancellation.wasActive = active();
    if (canceling()) {
        return cancellation;
    }

    invalidateSession();

    if (!active()) {
        _entries.clear();
        _currentEntry = nullptr;
        _hasCurrent = false;
        finishSession();
        return cancellation;
    }

    beginCancellation();
    cancellation.currentEntry = _currentEntry;
    cancellation.pendingEntries = _entries.takeAll();
    return cancellation;
}

void FtpEraseSession::finishCancellation()
{
    _entries.clear();
    _currentEntry = nullptr;
    _hasCurrent = false;
    _failureCount = 0;
    finishSession();
}

uint FtpEraseSession::finish()
{
    const uint failures = _failureCount;
    _entries.clear();
    _currentEntry = nullptr;
    _hasCurrent = false;
    _failureCount = 0;
    finishSession();
    return failures;
}

void FtpEraseSession::clear()
{
    invalidateSession();
    (void) finish();
}

QPointer<OnboardLogEntry> FtpEraseSession::takeNext()
{
    if (!active() || canceling() || hasCurrent()) {
        return _currentEntry;
    }

    _currentEntry = _entries.takeNext();
    _hasCurrent = !_currentEntry.isNull();
    return _currentEntry;
}

void FtpEraseSession::completeCurrent(bool failed)
{
    if (!hasCurrent()) {
        return;
    }
    if (failed) {
        ++_failureCount;
    }
    _currentEntry = nullptr;
    _hasCurrent = false;
}
