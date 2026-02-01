#include "LogEntry.h"
#include "QGCApplication.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QtMath>

Q_STATIC_LOGGING_CATEGORY(LogEntryLog, "AnalyzeView.LogEntry")

LogDownloadData::LogDownloadData(LogEntry * const entry)
    : ID(entry->id())
    , entry(entry)
{
    qCDebug(LogEntryLog) << this;
}

LogDownloadData::~LogDownloadData()
{
    qCDebug(LogEntryLog) << this;
}

void LogDownloadData::advanceChunk()
{
    ++current_chunk;
    chunk_table = QBitArray(chunkBins(), false);
}

uint32_t LogDownloadData::chunkBins() const
{
    const qreal num = static_cast<qreal>((entry->size() - (current_chunk * kChunkSize))) / static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    return qMin(static_cast<uint32_t>(qCeil(num)), kTableBins);
}

uint32_t LogDownloadData::numChunks() const
{
    const qreal num = static_cast<qreal>(entry->size()) / static_cast<qreal>(kChunkSize);
    return qCeil(num);
}

bool LogDownloadData::chunkEquals(const bool val) const
{
    return (chunk_table == QBitArray(chunk_table.size(), val));
}

/*===========================================================================*/

LogEntry::LogEntry(uint logId, const QDateTime &dateTime, uint logSize, bool received, QObject *parent)
    : QObject(parent)
    , _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
{
    qCDebug(LogEntryLog) << this;
}

LogEntry::~LogEntry()
{
    qCDebug(LogEntryLog) << this;
}

QString LogEntry::sizeStr() const
{
    return qgcApp()->bigSizeToString(_logSize);
}
