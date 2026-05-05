#pragma once

#include <QtCore/QBitArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <chrono>
#include <cstdint>
#include <optional>

class OnboardLogEntry;

/// State and invariants for one MAVLink LOG_DATA download.
class LogProtocolDownloadSession final
{
    Q_DISABLE_COPY_MOVE(LogProtocolDownloadSession)

#ifdef QGC_UNITTEST_BUILD
    friend class OnboardLogDownloadTest;
#endif

public:
    enum class FileOpenResult : uint8_t
    {
        Success,
        EntryUnavailable,
        InvalidLogId,
        DestinationExists,
        OpenFailed,
    };

    enum class PacketResult : uint8_t
    {
        Accepted,
        Duplicate,
        EntryUnavailable,
        InvalidPayload,
        MisalignedOffset,
        OutsideFile,
        SizeLimitExceeded,
        UnexpectedSize,
        WrongChunk,
        BinOutOfRange,
        SeekFailed,
        WriteFailed,
    };

    struct PacketAcceptance
    {
        PacketResult result = PacketResult::Accepted;
        uint32_t expectedCount = 0;
        bool followedByReceivedBin = false;
    };

    struct MissingRange
    {
        uint32_t offset = 0;
        uint32_t count = 0;
    };

    struct ProgressUpdate
    {
        uint32_t bytesWritten = 0;
        qreal bytesPerSecond = 0.;
    };

    struct CompletionState
    {
        bool chunkComplete = false;
        bool logComplete = false;
    };

    explicit LogProtocolDownloadSession(OnboardLogEntry* logEntry);
    ~LogProtocolDownloadSession();

    uint16_t id() const { return _id; }

    bool hasValidLogId() const { return _hasValidLogId; }

    OnboardLogEntry* entry() { return _entry; }

    const OnboardLogEntry* entry() const { return _entry; }

    bool matchesEntry(const OnboardLogEntry* entry) const { return _entry == entry; }

    FileOpenResult openFile(const QString& filePath);
    bool commit();
    void cancelWriting();

    QString fileErrorString() const { return _errorString.isEmpty() ? _file.errorString() : _errorString; }

    PacketAcceptance acceptPacket(uint32_t offset, uint8_t count, const uint8_t* data);

    void advanceChunk();
    bool chunkComplete() const;
    bool logComplete() const;
    CompletionState completionState() const;
    uint32_t currentChunkOffset() const;
    uint32_t currentChunkRequestBytes() const;
    std::optional<MissingRange> firstMissingRange() const;

    std::optional<ProgressUpdate> takeProgressUpdate(std::chrono::milliseconds interval, uint32_t byteThreshold);

    uint32_t bytesWritten() const { return _bytesWritten; }

    uint32_t actualSize() const { return _endOffset.value_or(_bytesWritten); }

    uint32_t receivedBinCount() const { return _receivedBinCount; }

    static constexpr uint32_t kTableBins = 2048;  // 2048 packets = ~180 KiB chunks
    static const uint32_t kChunkSize;

private:
    // LOG_ENTRY.size is approximate. Permit substantial drift while preventing
    // a faulty peer from growing a download without bound.
    static constexpr uint32_t kMaximumAdvertisedSizeOverrun = 64U * 1024U * 1024U;

    uint32_t _requiredChunkBins() const;
    void _resetChunk();

    const uint16_t _id = 0;
    const bool _hasValidLogId = false;
    const uint32_t _maximumDownloadSize = 0;
    QPointer<OnboardLogEntry> _entry;
    QBitArray _chunkTable;
    std::optional<uint32_t> _endOffset = std::nullopt;
    uint32_t _currentChunk = 0;
    uint32_t _receivedBinCount = 0;
    QTemporaryFile _file;
    QString _finalFilePath;
    QString _errorString;
    uint32_t _bytesWritten = 0;
    uint32_t _lastStatusBytes = 0;
    size_t _rateBytes = 0;
    qreal _averageRate = 0.;
    QElapsedTimer _elapsed;
};
