#include "LogProtocolDownloadBatchSession.h"

quint64 LogProtocolDownloadBatchSession::begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path,
                                               const QString& fileExtension)
{
    _entries.reset(entries);
    _current.reset();
    _path = path;
    _fileExtension = fileExtension;
    _retryCount = 0;
    return beginSession(!_entries.isEmpty());
}

LogProtocolDownloadBatchSession::Cancellation LogProtocolDownloadBatchSession::cancel()
{
    Cancellation cancellation;
    cancellation.wasActive = active();
    cancellation.currentEntry = _current ? _current->entry() : nullptr;
    invalidateSession();
    finish();
    return cancellation;
}

void LogProtocolDownloadBatchSession::finish()
{
    _entries.clear();
    _current.reset();
    _path.clear();
    _fileExtension.clear();
    _retryCount = 0;
    finishSession();
}

void LogProtocolDownloadBatchSession::clear()
{
    invalidateSession();
    finish();
}

LogProtocolDownloadSession* LogProtocolDownloadBatchSession::startNext()
{
    if (!active() || _current) {
        return _current.get();
    }

    const QPointer<OnboardLogEntry> entry = _entries.takeNext();
    if (!entry) {
        return nullptr;
    }

    _retryCount = 0;
    _current = std::make_unique<LogProtocolDownloadSession>(entry);
    return _current.get();
}

void LogProtocolDownloadBatchSession::completeCurrent()
{
    _current.reset();
    _retryCount = 0;
}

bool LogProtocolDownloadBatchSession::tryConsumeRetry(int maximumRetries)
{
    if (_retryCount >= maximumRetries) {
        return false;
    }

    ++_retryCount;
    return true;
}

bool LogProtocolDownloadBatchSession::isCurrent(quint64 generation, const OnboardLogEntry* entry) const
{
    return isCurrentGeneration(generation) && (!entry || (_current && _current->matchesEntry(entry)));
}
