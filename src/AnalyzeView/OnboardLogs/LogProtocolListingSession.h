#pragma once

#include <QtCore/QBitArray>
#include <cstdint>
#include <optional>

/// State and invariants for one MAVLink LOG_ENTRY listing attempt.
class LogProtocolListingSession final
{
    Q_DISABLE_COPY_MOVE(LogProtocolListingSession)

public:
    struct Response
    {
        int entryCount = 0;
        bool firstResponse = false;
        bool emptyList = false;
        bool inconsistentCount = false;
        bool inconsistentRange = false;
        bool truncated = false;
    };

    enum class EntryResult : uint8_t
    {
        Accepted,
        Duplicate,
        IgnoredEmptyArduPilotLog,
        OutOfRange,
        NoLogs,
    };

    struct EntryAcceptance
    {
        EntryResult result = EntryResult::OutOfRange;
        int modelIndex = -1;
        bool complete = false;
    };

    struct MissingRange
    {
        uint32_t start = 0;
        uint32_t end = 0;
    };

    LogProtocolListingSession() = default;

    [[nodiscard]] quint64 begin(bool oneBasedIds = false);
    bool cancel();
    void finish();
    void clear();

    Response observeResponse(uint16_t logCount, uint16_t lastLogNumber, uint16_t entryId);
    EntryAcceptance acceptEntry(uint16_t id, uint32_t size);
    std::optional<MissingRange> firstMissingRange() const;

    bool tryConsumeRetry();

    void markPartial() { _partial = true; }

    void resetRetries() { _retryCount = 0; }

    bool active() const { return _active; }

    bool partial() const { return _partial; }

    bool receivedResponse() const { return _receivedResponse; }

    int receivedEntryCount() const { return _receivedEntryCount; }

    int retryCount() const { return _retryCount; }

    uint16_t firstLogId() const { return _firstLogId; }

    uint16_t announcedLogCount() const { return _announcedLogCount; }

    quint64 generation() const { return _generation; }

    static constexpr int kMaxLogEntries = 10000;

private:
    static constexpr int kMaxListingRetries = 2;

    quint64 _generation = 0;
    QBitArray _receivedEntries;
    uint16_t _firstLogId = 0;
    int _retryCount = 0;
    int _receivedEntryCount = 0;
    uint16_t _announcedLogCount = 0;
    uint16_t _announcedLastLogNumber = 0;
    bool _active = false;
    bool _partial = false;
    bool _receivedResponse = false;
    bool _initialized = false;
    bool _oneBasedIds = false;
};
