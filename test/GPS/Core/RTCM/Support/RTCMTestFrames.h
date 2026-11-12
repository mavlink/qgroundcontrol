#pragma once

#include <bit>
#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>

#include "Protocols/Support/ProtocolTestPackets.h"

/// RTCM frames whose fields the GPS suites choose.
namespace GPSTest {

/// What an MSM frame's header says of its observations.
struct RtcmMsmFields
{
    uint64_t satelliteMask = 0;
    uint32_t signalMask = 0x80000000;
    uint32_t epochTime = 0;
    bool moreMessages = false;
    uint16_t stationId = 0;
    /// The bits of the cell mask after the header; one for each satellite and signal when unset.
    std::optional<int> cellMaskBits = std::nullopt;
};

/// The satellite mask of the first @a count satellites.
constexpr uint64_t rtcmMsmSatellites(int count)
{
    return count > 0 ? ~uint64_t{0} << (64 - count) : 0;
}

/// An RTCM MSM frame of message @a messageId with that header and a cell mask, without the observations after them.
/// Every other field of the header has all its bits set, so that a field read one bit off is wrong.
inline QByteArray rtcmMsmFrame(uint16_t messageId, const RtcmMsmFields& fields)
{
    QByteArray payload;
    int size = 0;
    const auto put = [&payload, &size](uint64_t value, int width) {
        for (int bit = width - 1; bit >= 0; --bit, ++size) {
            if (size % 8 == 0) {
                payload.append('\0');
            }
            payload.back() = static_cast<char>(payload.back() | (((value >> bit) & 1) << (7 - size % 8)));
        }
    };
    put(messageId, 12);
    put(fields.stationId, 12);
    put(fields.epochTime, 30);
    put(fields.moreMessages ? 1 : 0, 1);
    put(0x7, 3);   // issue of data station
    put(0x7f, 7);  // reserved
    put(0x3, 2);   // clock steering
    put(0x3, 2);   // external clock
    put(0x1, 1);   // divergence-free smoothing
    put(0x7, 3);   // smoothing interval
    put(fields.satelliteMask, 64);
    put(fields.signalMask, 32);
    const int cells =
        fields.cellMaskBits.value_or(std::popcount(fields.satelliteMask) * std::popcount(fields.signalMask));
    for (int cell = 0; cell < cells; ++cell) {
        put(1, 1);
    }
    return rtcmBytes(payload);
}

/// An MSM7 frame of @a messageId, such as 1077 for GPS, with the first @a satellites satellites at @a epochTime.
inline QByteArray rtcmMsmFrame(uint16_t messageId, int satellites, uint32_t epochTime = 0)
{
    return rtcmMsmFrame(messageId, {.satelliteMask = rtcmMsmSatellites(satellites), .epochTime = epochTime});
}

}  // namespace GPSTest
