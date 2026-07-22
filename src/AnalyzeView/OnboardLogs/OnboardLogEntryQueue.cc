#include "OnboardLogEntryQueue.h"

#include "OnboardLogEntry.h"

void OnboardLogEntryQueue::reset(const QList<QPointer<OnboardLogEntry>>& entries)
{
    _entries.clear();
    for (const QPointer<OnboardLogEntry>& entry : entries) {
        if (entry) {
            _entries.enqueue(entry);
        }
    }
}

QPointer<OnboardLogEntry> OnboardLogEntryQueue::takeNext()
{
    while (!_entries.isEmpty()) {
        const QPointer<OnboardLogEntry> entry = _entries.dequeue();
        if (entry) {
            return entry;
        }
    }
    return nullptr;
}

QList<QPointer<OnboardLogEntry>> OnboardLogEntryQueue::takeAll()
{
    QList<QPointer<OnboardLogEntry>> entries;
    entries.reserve(_entries.size());
    while (const QPointer<OnboardLogEntry> entry = takeNext()) {
        entries.append(entry);
    }
    return entries;
}
