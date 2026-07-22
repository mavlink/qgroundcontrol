#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QQueue>

class OnboardLogEntry;

/// Null-safe queue shared by onboard-log batch operations.
class OnboardLogEntryQueue final
{
    Q_DISABLE_COPY_MOVE(OnboardLogEntryQueue)

public:
    OnboardLogEntryQueue() = default;

    void reset(const QList<QPointer<OnboardLogEntry>>& entries);

    void clear() { _entries.clear(); }

    QPointer<OnboardLogEntry> takeNext();
    QList<QPointer<OnboardLogEntry>> takeAll();

    bool isEmpty() const { return _entries.isEmpty(); }

    qsizetype size() const { return _entries.size(); }

private:
    QQueue<QPointer<OnboardLogEntry>> _entries;
};
