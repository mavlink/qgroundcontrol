#pragma once

#include <array>
#include <optional>
#include <span>

#include "GPSReceiverSettingId.h"
#include "LittleEndian.h"
#include "UBX/Generated/UBXConfigKeys.h"

/// CFG-VALSET and CFG-VALGET key/value encoding.
namespace UBX {

/// Value width in bytes of @a key; 0 for reserved sizes and for 8-byte values, which QGC never reads or writes.
[[nodiscard]] constexpr unsigned configurationValueBytes(uint32_t key)
{
    const size_t width = cfgKeyValueBytes(key);
    return width <= sizeof(uint32_t) ? static_cast<unsigned>(width) : 0;
}

struct ConfigurationValue
{
    uint32_t key;
    uint32_t value;

    bool operator==(const ConfigurationValue&) const = default;
};

/// Reads only key/value entries; VALSET and VALGET validate their different headers separately.
class ConfigurationValueCursor
{
public:
    explicit ConfigurationValueCursor(std::span<const uint8_t> entries)
        : _remaining(entries)
    {}

    bool empty() const { return _remaining.empty(); }

    bool valid() const { return _valid; }

    std::optional<ConfigurationValue> next()
    {
        if (!_valid || empty()) {
            return std::nullopt;
        }
        const auto key = LittleEndian::read<uint32_t>(_remaining, 0);
        const unsigned width = key ? configurationValueBytes(*key) : 0;
        if (!width || _remaining.size() < sizeof(uint32_t) + width) {
            _valid = false;
            return std::nullopt;
        }
        uint32_t value = 0;
        for (unsigned byte = 0; byte < width; ++byte) {
            value |= uint32_t(_remaining[sizeof(uint32_t) + byte]) << (8 * byte);
        }
        if ((*key >> 28) == 1 && value > 1) {
            _valid = false;
            return std::nullopt;
        }
        _remaining = _remaining.subspan(sizeof(uint32_t) + width);
        return ConfigurationValue{*key, value};
    }

private:
    std::span<const uint8_t> _remaining;
    bool _valid = true;
};

/// The values of one CFG-VALGET poll or response; one poll reads at most nine keys.
struct ConfigurationValues
{
    std::array<ConfigurationValue, 9> values{};
    size_t count = 0;
};

/// Decodes a CFG-VALGET response (version 1, RAM layer) of at most nine values.
[[nodiscard]] inline std::optional<ConfigurationValues> decodeConfigurationValues(std::span<const uint8_t> payload)
{
    if (payload.size() < 4 || payload[0] != 1 || payload[1] || payload[2] || payload[3]) {
        return std::nullopt;
    }
    ConfigurationValues result;
    ConfigurationValueCursor cursor(payload.subspan(4));
    while (!cursor.empty()) {
        const auto entry = cursor.next();
        if (!entry || result.count == result.values.size()) {
            return std::nullopt;
        }
        result.values[result.count++] = *entry;
    }
    return result;
}

/// A CFG-VALSET payload for the RAM layer. An append error invalidates the entire batch, including previously
/// appended entries.
template <size_t Capacity>
struct CheckedValsetBatch
{
    static_assert(Capacity >= 4);
    std::array<uint8_t, Capacity> bytes{0, 1};  // RAM layer; no persistent writes.
    size_t size = 4;
    GPSReceiverSettingSet settings{};
    bool valid = true;

    bool invalidate() { return valid = false; }

    std::span<const uint8_t> payload() const
    {
        return valid && size > 4 ? std::span<const uint8_t>(bytes).first(size) : std::span<const uint8_t>{};
    }

    /// The appended key/value entries, without the header.
    std::span<const uint8_t> entries() const { return std::span<const uint8_t>(bytes).subspan(4, size - 4); }

    bool append(uint32_t key, uint32_t value)
    {
        const unsigned width = configurationValueBytes(key);
        if (!valid || !width || (width < 4 && value >= (1u << (width * 8))) || ((key >> 28) == 1 && value > 1) ||
            sizeof(key) + width > bytes.size() - size) {
            return invalidate();
        }
        (void) LittleEndian::write(bytes, size, key);
        for (unsigned byte = 0; byte < width; ++byte) {
            bytes[size + sizeof(key) + byte] = static_cast<uint8_t>(value >> (8 * byte));
        }
        size += sizeof(key) + width;
        return true;
    }
};
}  // namespace UBX
