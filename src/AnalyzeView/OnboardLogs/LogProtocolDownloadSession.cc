#include "LogProtocolDownloadSession.h"

#include <QtCore/QFileInfo>
#include <algorithm>
#include <limits>

#include "MAVLinkLib.h"
#include "OnboardLogEntry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogProtocolDownloadSessionLog, "AnalyzeView.OnboardLogs.LogProtocolDownloadSession")

const uint32_t LogProtocolDownloadSession::kChunkSize =
    LogProtocolDownloadSession::kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;

LogProtocolDownloadSession::LogProtocolDownloadSession(OnboardLogEntry* const logEntry)
    : _id((logEntry && (logEntry->id() <= (std::numeric_limits<uint16_t>::max)()))
              ? static_cast<uint16_t>(logEntry->id())
              : 0),
      _hasValidLogId(logEntry && (logEntry->id() <= (std::numeric_limits<uint16_t>::max)())),
      _maximumDownloadSize(logEntry
                               ? static_cast<uint32_t>((
                                     std::min) (static_cast<uint64_t>(logEntry->size()) + kMaximumAdvertisedSizeOverrun,
                                                static_cast<uint64_t>((std::numeric_limits<uint32_t>::max)())))
                               : 0),
      _entry(logEntry)
{
    _resetChunk();
    _elapsed.start();
    qCDebug(LogProtocolDownloadSessionLog) << Q_FUNC_INFO << "id" << _id;
}

LogProtocolDownloadSession::~LogProtocolDownloadSession()
{
    cancelWriting();
    qCDebug(LogProtocolDownloadSessionLog) << Q_FUNC_INFO << "id" << _id;
}

LogProtocolDownloadSession::FileOpenResult LogProtocolDownloadSession::openFile(const QString& filePath)
{
    _errorString.clear();
    if (!_entry) {
        return FileOpenResult::EntryUnavailable;
    }
    if (!_hasValidLogId) {
        return FileOpenResult::InvalidLogId;
    }
    if (QFileInfo::exists(filePath)) {
        _errorString = QStringLiteral("Destination already exists");
        return FileOpenResult::DestinationExists;
    }

    _finalFilePath = filePath;
    _file.setFileTemplate(filePath + QStringLiteral(".part.XXXXXX"));
    if (!_file.open()) {
        _finalFilePath.clear();
        return FileOpenResult::OpenFailed;
    }

    return FileOpenResult::Success;
}

bool LogProtocolDownloadSession::commit()
{
    if (!_endOffset || !_file.resize(*_endOffset) || !_file.flush()) {
        return false;
    }
    _file.close();
    if (!_file.rename(_finalFilePath)) {
        return false;
    }
    _file.setAutoRemove(false);
    _finalFilePath.clear();
    return true;
}

void LogProtocolDownloadSession::cancelWriting()
{
    if (_finalFilePath.isEmpty()) {
        return;
    }
    if (_file.isOpen()) {
        _file.close();
    }
    if (_file.exists()) {
        (void) _file.remove();
    }
    _finalFilePath.clear();
    _errorString = QStringLiteral("Writing canceled by application");
}

LogProtocolDownloadSession::PacketAcceptance LogProtocolDownloadSession::acceptPacket(uint32_t offset, uint8_t count,
                                                                                      const uint8_t* data)
{
    if (!_entry) {
        return {.result = PacketResult::EntryUnavailable};
    }
    if ((offset % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        return {.result = PacketResult::MisalignedOffset};
    }

    constexpr uint32_t packetSize = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    if (count > packetSize) {
        return {.result = PacketResult::UnexpectedSize, .expectedCount = packetSize};
    }
    if ((count > 0) && !data) {
        return {.result = PacketResult::InvalidPayload};
    }
    if (!_errorString.isEmpty()) {
        return {.result = PacketResult::WriteFailed};
    }

    const uint32_t chunk = offset / kChunkSize;
    if (chunk != _currentChunk) {
        return {.result = PacketResult::WrongChunk};
    }

    const uint32_t bin = (offset - currentChunkOffset()) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    if (bin >= static_cast<uint32_t>(_chunkTable.size())) {
        return {.result = PacketResult::BinOutOfRange};
    }

    const uint64_t packetEnd64 = static_cast<uint64_t>(offset) + count;
    if (packetEnd64 > (std::numeric_limits<uint32_t>::max)()) {
        return {.result = PacketResult::OutsideFile};
    }
    if (packetEnd64 > _maximumDownloadSize) {
        return {.result = PacketResult::SizeLimitExceeded};
    }
    const uint32_t packetEnd = static_cast<uint32_t>(packetEnd64);
    const bool endOfLog = count < packetSize;
    if (_endOffset) {
        if (endOfLog && (packetEnd != *_endOffset)) {
            return {.result = PacketResult::OutsideFile};
        }
        if ((offset > *_endOffset) || (packetEnd > *_endOffset)) {
            return {.result = PacketResult::OutsideFile};
        }
    }

    if (count == 0) {
        if (_endOffset) {
            return {.result = PacketResult::Duplicate};
        }
        _endOffset = offset;
        return {.result = PacketResult::Accepted, .expectedCount = packetSize};
    }
    if (_chunkTable.testBit(static_cast<qsizetype>(bin))) {
        return {.result = PacketResult::Duplicate};
    }

    if (!_file.isOpen()) {
        return {.result = PacketResult::WriteFailed};
    }
    if ((_file.pos() != offset) && !_file.seek(offset)) {
        return {.result = PacketResult::SeekFailed};
    }
    if (_file.write(reinterpret_cast<const char*>(data), count) != count) {
        return {.result = PacketResult::WriteFailed};
    }

    _chunkTable.setBit(static_cast<qsizetype>(bin));
    ++_receivedBinCount;
    _bytesWritten += count;
    _rateBytes += count;
    if (endOfLog) {
        _endOffset = packetEnd;
    }

    const bool followedByReceivedBin =
        ((bin + 1) < static_cast<uint32_t>(_chunkTable.size())) && _chunkTable.at(static_cast<qsizetype>(bin + 1));
    return {
        .result = PacketResult::Accepted, .expectedCount = packetSize, .followedByReceivedBin = followedByReceivedBin};
}

void LogProtocolDownloadSession::advanceChunk()
{
    ++_currentChunk;
    _resetChunk();
}

bool LogProtocolDownloadSession::chunkComplete() const
{
    const uint64_t chunkEndOffset = static_cast<uint64_t>(currentChunkOffset()) + kChunkSize;
    if (!_endOffset || (*_endOffset > chunkEndOffset)) {
        return _receivedBinCount == kTableBins;
    }

    const uint32_t requiredBins = _requiredChunkBins();
    if (_receivedBinCount < requiredBins) {
        return false;
    }
    for (uint32_t bin = 0; bin < requiredBins; ++bin) {
        if (!_chunkTable.testBit(static_cast<qsizetype>(bin))) {
            return false;
        }
    }
    return true;
}

bool LogProtocolDownloadSession::logComplete() const
{
    return completionState().logComplete;
}

LogProtocolDownloadSession::CompletionState LogProtocolDownloadSession::completionState() const
{
    const bool currentChunkComplete = chunkComplete();
    const uint64_t chunkEndOffset = static_cast<uint64_t>(currentChunkOffset()) + kChunkSize;
    const bool currentLogComplete = _endOffset && (*_endOffset >= currentChunkOffset()) &&
                                    (static_cast<uint64_t>(*_endOffset) <= chunkEndOffset) && currentChunkComplete;
    return {.chunkComplete = currentChunkComplete, .logComplete = currentLogComplete};
}

uint32_t LogProtocolDownloadSession::currentChunkOffset() const
{
    return _currentChunk * kChunkSize;
}

uint32_t LogProtocolDownloadSession::currentChunkRequestBytes() const
{
    return (std::min) (kChunkSize, (std::numeric_limits<uint32_t>::max)() - currentChunkOffset());
}

std::optional<LogProtocolDownloadSession::MissingRange> LogProtocolDownloadSession::firstMissingRange() const
{
    const qsizetype size = static_cast<qsizetype>(_requiredChunkBins());
    if (size <= 0) {
        return std::nullopt;
    }

    qsizetype start = 0;
    while ((start < size) && _chunkTable.testBit(start)) {
        ++start;
    }

    qsizetype end = start;
    while ((end < size) && !_chunkTable.testBit(end)) {
        ++end;
    }

    const uint32_t offset = currentChunkOffset() + (static_cast<uint32_t>(start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t requestedBytes = static_cast<uint32_t>(end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    return MissingRange{
        .offset = offset,
        .count = (std::min) (requestedBytes, (std::numeric_limits<uint32_t>::max)() - offset),
    };
}

std::optional<LogProtocolDownloadSession::ProgressUpdate> LogProtocolDownloadSession::takeProgressUpdate(
    std::chrono::milliseconds interval, uint32_t byteThreshold)
{
    const std::chrono::milliseconds elapsed{_elapsed.elapsed()};
    const bool timeThresholdMet = elapsed >= interval;
    const bool sizeThresholdMet = (_bytesWritten - _lastStatusBytes) >= byteThreshold;
    if (!timeThresholdMet && !sizeThresholdMet) {
        return std::nullopt;
    }

    if (timeThresholdMet) {
        const qreal elapsedSeconds = std::chrono::duration<qreal>(elapsed).count();
        const qreal rate = (elapsedSeconds > 0.) ? (_rateBytes / elapsedSeconds) : 0.;
        _averageRate = (_averageRate * 0.95) + (rate * 0.05);
        _rateBytes = 0;
        _elapsed.start();
    }

    _lastStatusBytes = _bytesWritten;
    return ProgressUpdate{.bytesWritten = _bytesWritten, .bytesPerSecond = _averageRate};
}

uint32_t LogProtocolDownloadSession::_requiredChunkBins() const
{
    const uint64_t chunkEndOffset = static_cast<uint64_t>(currentChunkOffset()) + kChunkSize;
    if (!_endOffset || (static_cast<uint64_t>(*_endOffset) > chunkEndOffset)) {
        return kTableBins;
    }
    if (*_endOffset <= currentChunkOffset()) {
        return 0;
    }

    const uint32_t bytesInChunk = *_endOffset - currentChunkOffset();
    return (bytesInChunk + MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN - 1) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
}

void LogProtocolDownloadSession::_resetChunk()
{
    _chunkTable = QBitArray(kTableBins, false);
    _receivedBinCount = 0;
}
