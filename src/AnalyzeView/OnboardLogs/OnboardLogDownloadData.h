#pragma once

#include <QtCore/QBitArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QString>

class OnboardLogEntry;

struct OnboardLogDownloadData
{
    explicit OnboardLogDownloadData(OnboardLogEntry* const logEntry);
    ~OnboardLogDownloadData();

    void advanceChunk();

    /// The number of MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN bins in the current chunk
    uint32_t chunkBins() const;

    /// The number of kChunkSize chunks in the file
    uint32_t numChunks() const;

    /// True if all bins in the chunk have been set to val
    bool chunkEquals(const bool val) const;

    uint ID = 0;
    OnboardLogEntry* const entry = nullptr;

    QBitArray chunk_table;
    uint32_t current_chunk = 0;
    QFile file;
    QString filename;
    uint written = 0;
    uint last_status_written = 0;
    size_t rate_bytes = 0;
    qreal rate_avg = 0.;
    QElapsedTimer elapsed;

    static constexpr uint32_t kTableBins = 2048;  // 2048 packets = ~180 KB chunks (4x larger for better throughput)
    static const uint32_t kChunkSize;             // kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN (defined in .cc)
};
