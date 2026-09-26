#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "CRC32.h"
#include "NMEASentence.h"

/// The header fields receiver detection tells NovAtel-style ASCII logs apart by.
struct GPSASCIILog
{
    /// The log name, such as "VERSIONA".
    std::string_view name;
    /// The second header field: a port such as "COM1" from NovAtel-compatible receivers, CPU idle time from Unicore.
    std::string_view source;
};

/// A "#NAME,<source>,...;<body>*<crc>" log whose CRC-32 (reflected, zero initial value, no final XOR) over the text
/// between '#' and '*' matches its eight hex digits; nullopt otherwise.
[[nodiscard]] inline std::optional<GPSASCIILog> gpsASCIILog(std::string_view line)
{
    constexpr size_t CRC_DIGITS = 8;
    const size_t star = line.rfind('*');
    if (!line.starts_with('#') || star == std::string_view::npos || star + 1 + CRC_DIGITS != line.size()) {
        return std::nullopt;
    }
    for (const char ch : line.substr(star + 1)) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
            return std::nullopt;
        }
    }
    const auto expected = NMEA::number<uint32_t>(line.substr(star + 1), NMEA::HEX_BASE);
    if (!expected || *expected != QGC::crc32Update({reinterpret_cast<const uint8_t*>(line.data() + 1), star - 1})) {
        return std::nullopt;
    }
    const size_t semicolon = line.find(';');
    const size_t first = line.find(',');
    if (semicolon == std::string_view::npos || first == std::string_view::npos || first > semicolon) {
        return std::nullopt;
    }
    const size_t second = line.find_first_of(",;", first + 1);
    return GPSASCIILog{.name = line.substr(1, first - 1), .source = line.substr(first + 1, second - first - 1)};
}
