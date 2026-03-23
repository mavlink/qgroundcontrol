#include "OnboardLogEntry.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtMath>

QGC_LOGGING_CATEGORY(OnboardLogEntryLog, "AnalyzeView.QGCOnboardLogEntry")

OnboardLogDownloadData::OnboardLogDownloadData(QGCOnboardLogEntry * const logEntry)
    : ID(logEntry->id())
    , entry(logEntry)
{
    qCDebug(OnboardLogEntryLog) << this;
}

OnboardLogDownloadData::~OnboardLogDownloadData()
{
    qCDebug(OnboardLogEntryLog) << this;
}

void OnboardLogDownloadData::advanceChunk()
{
    ++current_chunk;
    chunk_table = QBitArray(chunkBins(), false);
}

uint32_t OnboardLogDownloadData::chunkBins() const
{
    const qreal num = static_cast<qreal>((entry->size() - (current_chunk * kChunkSize))) / static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    return qMin(static_cast<uint32_t>(qCeil(num)), kTableBins);
}

uint32_t OnboardLogDownloadData::numChunks() const
{
    const qreal num = static_cast<qreal>(entry->size()) / static_cast<qreal>(kChunkSize);
    return qCeil(num);
}

bool OnboardLogDownloadData::chunkEquals(const bool val) const
{
    return (chunk_table == QBitArray(chunk_table.size(), val));
}

/*===========================================================================*/

QGCOnboardLogEntry::QGCOnboardLogEntry(uint logId, const QDateTime &dateTime, uint logSize, bool received, QObject *parent)
    : QObject(parent)
    , _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
{
    qCDebug(OnboardLogEntryLog) << this;
}

QGCOnboardLogEntry::~QGCOnboardLogEntry()
{
    qCDebug(OnboardLogEntryLog) << this;
}

QString QGCOnboardLogEntry::sizeStr() const
{
    return qgcApp()->bigSizeToString(_logSize);
}
