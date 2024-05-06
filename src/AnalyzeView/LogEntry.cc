/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogEntry.h"
#include "QGCMapEngine.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtMath>

#define kTableBins 512
#define kChunkSize (kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN)

QGC_LOGGING_CATEGORY(LogEntryLog, "qgc.analyzeview.logentry")

//-----------------------------------------------------------------------------
LogDownloadData::LogDownloadData(QGCLogEntry* entry_)
    : ID(entry_->id())
    , entry(entry_)
    , written(0)
    , rate_bytes(0)
    , rate_avg(0)
{

}

void LogDownloadData::advanceChunk()
{
       current_chunk++;
       chunk_table = QBitArray(chunkBins(), false);
}

// The number of MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN bins in the current chunk
uint32_t LogDownloadData::chunkBins() const
{
    return qMin(qCeil((entry->size() - current_chunk*kChunkSize)/static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN)),
                kTableBins);
}

// The number of kChunkSize chunks in the file
uint32_t LogDownloadData::numChunks() const
{
    return qCeil(entry->size() / static_cast<qreal>(kChunkSize));
}

// True if all bins in the chunk have been set to val
bool LogDownloadData::chunkEquals(const bool val) const
{
    return chunk_table == QBitArray(chunk_table.size(), val);
}

//----------------------------------------------------------------------------------------
QGCLogEntry::QGCLogEntry(uint logId, const QDateTime& dateTime, uint logSize, bool received)
    : _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
    , _selected(false)
{
    _status = tr("Pending");
}

//----------------------------------------------------------------------------------------
QString QGCLogEntry::sizeStr() const
{
    return QGCMapEngine::bigSizeToString(_logSize);
}
