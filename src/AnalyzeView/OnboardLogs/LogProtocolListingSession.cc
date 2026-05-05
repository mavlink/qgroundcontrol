#include "LogProtocolListingSession.h"

#include <algorithm>

quint64 LogProtocolListingSession::begin(bool oneBasedIds)
{
    ++_generation;
    _receivedEntries.clear();
    _firstLogId = oneBasedIds ? 1 : 0;
    _retryCount = 0;
    _receivedEntryCount = 0;
    _announcedLogCount = 0;
    _announcedLastLogNumber = 0;
    _active = true;
    _partial = false;
    _receivedResponse = false;
    _initialized = false;
    _oneBasedIds = oneBasedIds;
    return _generation;
}

bool LogProtocolListingSession::cancel()
{
    const bool wasActive = _active;
    ++_generation;
    _active = false;
    return wasActive;
}

void LogProtocolListingSession::finish()
{
    _active = false;
}

void LogProtocolListingSession::clear()
{
    (void) cancel();
    _receivedEntries.clear();
    _firstLogId = 0;
    _retryCount = 0;
    _receivedEntryCount = 0;
    _announcedLogCount = 0;
    _announcedLastLogNumber = 0;
    _partial = false;
    _receivedResponse = false;
    _initialized = false;
    _oneBasedIds = false;
}

LogProtocolListingSession::Response LogProtocolListingSession::observeResponse(uint16_t logCount,
                                                                               uint16_t lastLogNumber, uint16_t entryId)
{
    _receivedResponse = true;

    Response response;
    response.entryCount = _receivedEntries.size();
    if (_initialized) {
        if (logCount != _announcedLogCount) {
            response.inconsistentCount = true;
            _partial = true;
        } else if ((logCount != 0) && (lastLogNumber != _announcedLastLogNumber)) {
            response.inconsistentRange = true;
            _partial = true;
        }
        return response;
    }
    if (logCount == 0) {
        response.emptyList = true;
        return response;
    }

    _initialized = true;
    _announcedLogCount = logCount;
    _announcedLastLogNumber = lastLogNumber;
    const uint32_t rangeEnd = static_cast<uint32_t>(lastLogNumber) + 1;
    if (rangeEnd >= logCount) {
        const uint32_t reportedFirstLogId = rangeEnd - logCount;
        // Some implementations report a one-past-the-end value. Trust the
        // advertised range only when it contains the entry that carried it.
        if ((entryId >= reportedFirstLogId) && (entryId <= lastLogNumber)) {
            _firstLogId = static_cast<uint16_t>(reportedFirstLogId);
        }
    }
    response.entryCount = (std::min) (static_cast<int>(logCount), kMaxLogEntries);
    response.firstResponse = true;
    response.truncated = logCount > kMaxLogEntries;
    _partial = response.truncated;
    _receivedEntries.resize(response.entryCount);
    _receivedEntries.fill(false);
    return response;
}

LogProtocolListingSession::EntryAcceptance LogProtocolListingSession::acceptEntry(uint16_t id, uint32_t size)
{
    if (_receivedEntries.isEmpty()) {
        return {.result = EntryResult::NoLogs};
    }
    const int modelIndex = static_cast<int>(id) - static_cast<int>(_firstLogId);
    if ((modelIndex < 0) || (modelIndex >= _receivedEntries.size())) {
        return {.result = EntryResult::OutOfRange, .modelIndex = modelIndex};
    }

    const bool duplicate = _receivedEntries.testBit(modelIndex);
    if (!duplicate) {
        _receivedEntries.setBit(modelIndex);
        ++_receivedEntryCount;
    }

    EntryResult result = EntryResult::Accepted;
    if (duplicate) {
        result = EntryResult::Duplicate;
    } else if (_oneBasedIds && (size == 0)) {
        result = EntryResult::IgnoredEmptyArduPilotLog;
    }

    return {
        .result = result,
        .modelIndex = modelIndex,
        .complete = _receivedEntryCount >= _receivedEntries.size(),
    };
}

std::optional<LogProtocolListingSession::MissingRange> LogProtocolListingSession::firstMissingRange() const
{
    int start = -1;
    int end = -1;
    for (int index = 0; index < _receivedEntries.size(); ++index) {
        if (!_receivedEntries.testBit(index)) {
            if (start < 0) {
                start = index;
            }
            end = index;
        } else if (start >= 0) {
            break;
        }
    }

    if (start < 0) {
        return std::nullopt;
    }

    return MissingRange{
        .start = static_cast<uint32_t>(start) + _firstLogId,
        .end = static_cast<uint32_t>(end) + _firstLogId,
    };
}

bool LogProtocolListingSession::tryConsumeRetry()
{
    if (_retryCount >= kMaxListingRetries) {
        return false;
    }

    ++_retryCount;
    return true;
}
