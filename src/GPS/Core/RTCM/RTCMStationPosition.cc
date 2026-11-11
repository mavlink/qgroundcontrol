#include "RTCMStationPosition.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "GPSProtocolMath.h"
#include "RTCMFramer.h"

namespace {
constexpr uint16_t ARP_MESSAGE = 1005;
constexpr uint16_t ARP_WITH_HEIGHT_MESSAGE = 1006;
/// The messages are 152 and 168 bits long.
constexpr size_t ARP_PAYLOAD_SIZE = 19;
constexpr size_t ARP_WITH_HEIGHT_PAYLOAD_SIZE = 21;

/// Bit positions in the payload, after the message number, the station ID and the ITRF year and indicator bits.
constexpr size_t ECEF_X_OFFSET = 34;
constexpr size_t ECEF_Y_OFFSET = 74;
constexpr size_t ECEF_Z_OFFSET = 114;
constexpr size_t ECEF_BITS = 38;
constexpr size_t ANTENNA_HEIGHT_OFFSET = 152;
constexpr size_t ANTENNA_HEIGHT_BITS = 16;
/// Coordinates and the antenna height count 0.1 mm.
constexpr double METERS_PER_UNIT = 0.0001;

/// The @a width bits at bit @a offset of @a payload, most significant bit first.
uint64_t bitField(std::span<const uint8_t> payload, size_t offset, size_t width)
{
    uint64_t value = 0;
    for (size_t bit = offset; bit < offset + width; ++bit) {
        value = (value << 1) | ((payload[bit / 8] >> (7 - bit % 8)) & 1U);
    }
    return value;
}

double ecefMeters(std::span<const uint8_t> payload, size_t offset)
{
    const auto raw = static_cast<int64_t>(bitField(payload, offset, ECEF_BITS));
    // Two's complement in 38 bits: flipping the sign bit and subtracting its weight extends it.
    constexpr int64_t SIGN_BIT = int64_t{1} << (ECEF_BITS - 1);
    return static_cast<double>((raw ^ SIGN_BIT) - SIGN_BIT) * METERS_PER_UNIT;
}
}  // namespace

std::optional<RTCMStationPosition> RTCMStationPosition::fromFrame(QByteArrayView frame)
{
    const uint16_t messageId = RTCMFramer::frameMessageId(frame);
    if (messageId != ARP_MESSAGE && messageId != ARP_WITH_HEIGHT_MESSAGE) {
        return std::nullopt;
    }
    const bool hasAntennaHeight = messageId == ARP_WITH_HEIGHT_MESSAGE;
    const size_t payloadSize = hasAntennaHeight ? ARP_WITH_HEIGHT_PAYLOAD_SIZE : ARP_PAYLOAD_SIZE;
    if (static_cast<size_t>(frame.size()) != RTCMFramer::HEADER_SIZE + payloadSize + RTCMFramer::CRC_SIZE ||
        !RTCMFramer::isValidFrame(frame)) {
        return std::nullopt;
    }
    const std::span<const uint8_t> payload(reinterpret_cast<const uint8_t*>(frame.data()) + RTCMFramer::HEADER_SIZE,
                                           payloadSize);
    const GPSProtocolMath::Ecef ecef{ecefMeters(payload, ECEF_X_OFFSET), ecefMeters(payload, ECEF_Y_OFFSET),
                                     ecefMeters(payload, ECEF_Z_OFFSET)};
    if (!GPSProtocolMath::nearEarthSurface(ecef)) {
        return std::nullopt;
    }
    RTCMStationPosition station;
    station.position = GPSProtocolMath::fromEcef(ecef);
    if (hasAntennaHeight) {
        station.antennaHeightMeters =
            static_cast<double>(bitField(payload, ANTENNA_HEIGHT_OFFSET, ANTENNA_HEIGHT_BITS)) * METERS_PER_UNIT;
    }
    return station;
}
