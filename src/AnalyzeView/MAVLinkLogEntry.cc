#include "MAVLinkLogEntry.h"
#include "QGCApplication.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QtMath>

Q_STATIC_LOGGING_CATEGORY(MAVLinkLogEntryLog, "AnalyzeView.MAVLinkLogEntry")

MAVLinkLogDownloadData::MAVLinkLogDownloadData(MAVLinkLogEntry * const entry)
    : ID(entry->id())
    , entry(entry)
{
    qCDebug(MAVLinkLogEntryLog) << this;
}

MAVLinkLogDownloadData::~MAVLinkLogDownloadData()
{
    qCDebug(MAVLinkLogEntryLog) << this;
}

void MAVLinkLogDownloadData::advanceChunk()
{
    ++current_chunk;
    chunk_table = QBitArray(chunkBins(), false);
}

uint32_t MAVLinkLogDownloadData::chunkBins() const
{
    const qreal num = static_cast<qreal>((entry->size() - (current_chunk * kChunkSize))) / static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    return qMin(static_cast<uint32_t>(qCeil(num)), kTableBins);
}

uint32_t MAVLinkLogDownloadData::numChunks() const
{
    const qreal num = static_cast<qreal>(entry->size()) / static_cast<qreal>(kChunkSize);
    return qCeil(num);
}

bool MAVLinkLogDownloadData::chunkEquals(const bool val) const
{
    return (chunk_table == QBitArray(chunk_table.size(), val));
}

/*===========================================================================*/

MAVLinkLogEntry::MAVLinkLogEntry(uint logId, const QDateTime &dateTime, uint logSize, bool received, QObject *parent)
    : QObject(parent)
    , _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
{
    qCDebug(MAVLinkLogEntryLog) << this;
}

MAVLinkLogEntry::~MAVLinkLogEntry()
{
    qCDebug(MAVLinkLogEntryLog) << this;
}

QString MAVLinkLogEntry::sizeStr() const
{
    return qgcApp()->bigSizeToString(_logSize);
}
