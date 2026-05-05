#include "OnboardLogDownloadData.h"

#include <QtCore/QtMath>

#include "MAVLinkLib.h"
#include "OnboardLogEntry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(OnboardLogDownloadDataLog, "AnalyzeView.OnboardLogDownloadData")

const uint32_t OnboardLogDownloadData::kChunkSize =
    OnboardLogDownloadData::kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;

OnboardLogDownloadData::OnboardLogDownloadData(OnboardLogEntry* const logEntry) : ID(logEntry->id()), entry(logEntry)
{
    qCDebug(OnboardLogDownloadDataLog) << Q_FUNC_INFO << "id" << ID;
}

OnboardLogDownloadData::~OnboardLogDownloadData()
{
    qCDebug(OnboardLogDownloadDataLog) << Q_FUNC_INFO << "id" << ID;
}

void OnboardLogDownloadData::advanceChunk()
{
    ++current_chunk;
    chunk_table = QBitArray(chunkBins(), false);
}

uint32_t OnboardLogDownloadData::chunkBins() const
{
    const qreal num = static_cast<qreal>((entry->size() - (current_chunk * kChunkSize))) /
                      static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
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
