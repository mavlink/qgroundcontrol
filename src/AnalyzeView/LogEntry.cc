/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogEntry.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtMath>

QGC_LOGGING_CATEGORY(LogEntryLog, "test.analyzeview.logentry")

LogDownloadData::LogDownloadData(QGCLogEntry * const entry)
    : ID(entry->id())
    , entry(entry)
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
}

LogDownloadData::~LogDownloadData()
{
    // qCDebug(LogEntryLog) << Q_FUNC_INFO << this;
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
