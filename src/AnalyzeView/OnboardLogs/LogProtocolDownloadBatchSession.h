#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <memory>

#include "LogProtocolDownloadSession.h"
#include "OnboardLogEntry.h"
#include "OnboardLogEntryQueue.h"
#include "OnboardLogSession.h"

/// Queue and retry state for one batch of MAVLink LOG_DATA downloads.
class LogProtocolDownloadBatchSession final : public OnboardLogSessionBase
{
    Q_DISABLE_COPY_MOVE(LogProtocolDownloadBatchSession)

public:
    struct Cancellation
    {
        QPointer<OnboardLogEntry> currentEntry;
        bool wasActive = false;
    };

    LogProtocolDownloadBatchSession() = default;

    [[nodiscard]] quint64 begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path,
                                const QString& fileExtension = QStringLiteral("bin"));
    Cancellation cancel();
    void finish();
    void clear();

    LogProtocolDownloadSession* startNext();
    void completeCurrent();

    void resetRetries() { _retryCount = 0; }

    bool tryConsumeRetry(int maximumRetries);

    bool isCurrent(quint64 generation, const OnboardLogEntry* entry = nullptr) const;

    LogProtocolDownloadSession* current() { return _current.get(); }

    const LogProtocolDownloadSession* current() const { return _current.get(); }

    qsizetype pendingCount() const { return _entries.size(); }

    int retryCount() const { return _retryCount; }

    const QString& path() const { return _path; }

    const QString& fileExtension() const { return _fileExtension; }

private:
    OnboardLogEntryQueue _entries;
    std::unique_ptr<LogProtocolDownloadSession> _current;
    QString _path;
    QString _fileExtension;
    int _retryCount = 0;
};
