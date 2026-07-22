#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>

#include "OnboardLogEntry.h"
#include "OnboardLogEntryQueue.h"
#include "OnboardLogSession.h"

/// Queue and lifecycle state for one MAVLink FTP erase batch.
class FtpEraseSession final : public OnboardLogSessionBase
{
    Q_DISABLE_COPY_MOVE(FtpEraseSession)

public:
    struct Cancellation
    {
        QList<QPointer<OnboardLogEntry>> pendingEntries;
        QPointer<OnboardLogEntry> currentEntry;
        bool wasActive = false;
    };

    FtpEraseSession() = default;

    [[nodiscard]] quint64 begin(const QList<QPointer<OnboardLogEntry>>& entries);
    Cancellation cancel();
    void finishCancellation();
    uint finish();
    void clear();

    QPointer<OnboardLogEntry> takeNext();
    void completeCurrent(bool failed);

    bool hasCurrent() const { return _hasCurrent; }

    QPointer<OnboardLogEntry> currentEntry() const { return _currentEntry; }

    qsizetype pendingCount() const { return _entries.size(); }

private:
    OnboardLogEntryQueue _entries;
    QPointer<OnboardLogEntry> _currentEntry;
    uint _failureCount = 0;
    bool _hasCurrent = false;
};
