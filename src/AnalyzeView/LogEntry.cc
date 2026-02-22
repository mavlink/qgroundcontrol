#include "LogEntry.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogEntryLog, "AnalyzeView.QGCLogEntry")

LogDownloadData::LogDownloadData(QGCLogEntry * const logEntry)
    : ID(logEntry->id())
    , entry(logEntry)
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
}

LogDownloadData::~LogDownloadData()
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
}

void LogDownloadData::recordGap(uint32_t gapOffset, uint32_t gapLength)
{
    if (gapLength == 0) {
        return;
    }

    gaps.append({gapOffset, gapLength});
}

void LogDownloadData::fillGap(uint32_t dataOffset, uint32_t dataLength)
{
    for (int i = 0; i < gaps.size(); ) {
        GapRange &gap = gaps[i];
        const uint32_t gapEnd = gap.offset + gap.length;
        const uint32_t dataEnd = dataOffset + dataLength;

        // No overlap
        if (dataEnd <= gap.offset || dataOffset >= gapEnd) {
            ++i;
            continue;
        }

        if (dataOffset <= gap.offset && dataEnd >= gapEnd) {
            // Data fully covers this gap — remove it
            gaps.removeAt(i);
        } else if (dataOffset <= gap.offset) {
            // Data covers the beginning of the gap — shrink from left
            const uint32_t overlap = dataEnd - gap.offset;
            gap.offset += overlap;
            gap.length -= overlap;
            ++i;
        } else if (dataEnd >= gapEnd) {
            // Data covers the end of the gap — shrink from right
            gap.length = dataOffset - gap.offset;
            ++i;
        } else {
            // Data splits the gap into two
            const uint32_t rightOffset = dataEnd;
            const uint32_t rightLength = gapEnd - dataEnd;
            gap.length = dataOffset - gap.offset;
            gaps.insert(i + 1, {rightOffset, rightLength});
            i += 2;
        }
    }
}

bool LogDownloadData::isComplete() const
{
    return (expectedOffset >= entry->size()) && gaps.isEmpty();
}

/*===========================================================================*/

QGCLogEntry::QGCLogEntry(uint logId, const QDateTime &dateTime, uint logSize, bool received, QObject *parent)
    : QObject(parent)
    , _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
}

QGCLogEntry::~QGCLogEntry()
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
}

QString QGCLogEntry::sizeStr() const
{
    return qgcApp()->bigSizeToString(_logSize);
}
