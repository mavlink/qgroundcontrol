#include "LogProtocolDownloadBatchSession.h"

quint64 LogProtocolDownloadBatchSession::begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path,
                                               const QString& fileExtension)
{
    ++_generation;
    _queue.clear();
    for (const QPointer<OnboardLogEntry>& entry : entries) {
        if (entry) {
            _queue.enqueue(entry);
        }
    }
    _current.reset();
    _path = path;
    _fileExtension = fileExtension;
    _retryCount = 0;
    _active = !_queue.isEmpty();
    return _generation;
}

LogProtocolDownloadBatchSession::Cancellation LogProtocolDownloadBatchSession::cancel()
{
    Cancellation cancellation;
    cancellation.wasActive = _active;
    cancellation.currentEntry = _current ? _current->entry() : nullptr;
    ++_generation;
    finish();
    return cancellation;
}

void LogProtocolDownloadBatchSession::finish()
{
    _queue.clear();
    _current.reset();
    _path.clear();
    _fileExtension.clear();
    _retryCount = 0;
    _active = false;
}

void LogProtocolDownloadBatchSession::clear()
{
    ++_generation;
    finish();
}

LogProtocolDownloadSession* LogProtocolDownloadBatchSession::startNext()
{
    if (!_active || _current) {
        return _current.get();
    }

    QPointer<OnboardLogEntry> entry;
    while (!_queue.isEmpty() && !entry) {
        entry = _queue.dequeue();
    }
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
    return (generation == _generation) && _active && (!entry || (_current && _current->matchesEntry(entry)));
}
