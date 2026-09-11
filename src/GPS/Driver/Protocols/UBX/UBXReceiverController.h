#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <span>

#include "GPSCommandTransaction.h"
#include "LittleEndian.h"
#include "UBXReceiverProfile.h"

namespace UBX {
struct Acknowledgement
{
    uint16_t message;
    bool accepted;
};

struct ConfigurationValues
{
    std::array<uint32_t, 9> keys{};
    std::array<uint32_t, 9> values{};
    size_t count = 0;
};

inline std::optional<ConfigurationValues> decodeConfigurationValues(std::span<const uint8_t> payload)
{
    if (payload.size() < 4 || payload[0] != 1 || payload[1] || payload[2] || payload[3])
        return std::nullopt;
    ConfigurationValues result;
    for (size_t offset = 4; offset < payload.size();) {
        const auto key = LittleEndian::read<uint32_t>(payload, offset);
        if (!key || result.count == result.keys.size())
            return std::nullopt;
        offset += 4;
        const auto width = configurationValueBytes(*key);
        if (!width || payload.size() - offset < width)
            return std::nullopt;
        uint32_t value = 0;
        for (unsigned byte = 0; byte < width; ++byte)
            value |= uint32_t(payload[offset++]) << (8 * byte);
        if ((*key >> 28) == 1 && value > 1)
            return std::nullopt;
        result.keys[result.count] = *key;
        result.values[result.count++] = value;
    }
    return result;
}

/// Owns control-response correlation; the frame decoder cannot mutate pending requests.
class ReceiverController
{
public:
    void beginAcknowledgement(uint16_t message)
    {
        _awaitingMessage = message;
        _acknowledgement = GPSCommandOutcome::Pending;
    }

    void finishAcknowledgement() { _awaitingMessage.reset(); }

    bool awaitingAcknowledgement() const { return _awaitingMessage.has_value(); }

    GPSCommandOutcome acknowledgement() const { return _acknowledgement; }

    void accept(Acknowledgement response)
    {
        if (_awaitingMessage == response.message)
            _acknowledgement = response.accepted ? GPSCommandOutcome::Acknowledged : GPSCommandOutcome::Rejected;
    }

    void beginReadback(std::span<const uint32_t> keys)
    {
        _readback = {};
        _readbackPending = !keys.empty() && keys.size() <= _readback.keys.size();
        _readbackReady = false;
        if (_readbackPending) {
            _readback.count = keys.size();
            std::copy(keys.begin(), keys.end(), _readback.keys.begin());
        }
    }

    void finishReadback() { _readbackPending = false; }

    bool readbackPending() const { return _readbackPending; }

    bool readbackReady() const { return _readbackReady; }

    const ConfigurationValues& readback() const { return _readback; }

    void accept(const ConfigurationValues& response)
    {
        if (!_readbackPending || response.count != _readback.count)
            return;
        std::array<uint32_t, 9> values{};
        uint16_t seen = 0;
        for (size_t entry = 0; entry < response.count; ++entry) {
            size_t index = 0;
            while (index < _readback.count && _readback.keys[index] != response.keys[entry])
                ++index;
            if (index == _readback.count || (seen & (1u << index)))
                return;
            values[index] = response.values[entry];
            seen |= 1u << index;
        }
        _readback.values = values;
        _readbackReady = true;
    }

private:
    std::optional<uint16_t> _awaitingMessage;
    GPSCommandOutcome _acknowledgement = GPSCommandOutcome::Pending;
    ConfigurationValues _readback;
    bool _readbackPending = false;
    bool _readbackReady = false;
};
}  // namespace UBX
