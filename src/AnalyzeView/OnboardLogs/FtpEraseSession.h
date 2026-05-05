#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QQueue>

#include "OnboardLogEntry.h"

/// Queue and lifecycle state for one MAVLink FTP erase batch.
class FtpEraseSession final
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

    bool active() const { return _active; }

    bool canceling() const { return _canceling; }

    quint64 generation() const { return _generation; }

    QPointer<OnboardLogEntry> currentEntry() const { return _currentEntry; }

    qsizetype pendingCount() const { return _queue.size(); }

private:
    quint64 _generation = 0;
    QQueue<QPointer<OnboardLogEntry>> _queue;
    QPointer<OnboardLogEntry> _currentEntry;
    uint _failureCount = 0;
    bool _active = false;
    bool _canceling = false;
};
