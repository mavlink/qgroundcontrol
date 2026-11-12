#include "RTCMMsmHeader.h"

#include <array>
#include <cstddef>
#include <span>

#include "RTCMBits.h"
#include "RTCMFramer.h"

namespace {
/// Each constellation has ten message numbers from 1071 on, of which the first seven are MSM1 to MSM7.
constexpr uint16_t MSM_FIRST_MESSAGE = 1071;
constexpr uint16_t MSM_MESSAGES_PER_CONSTELLATION = 10;
constexpr uint8_t MSM_TYPE_COUNT = 7;
constexpr std::array MSM_CONSTELLATIONS{GPSConstellation::GPS,  GPSConstellation::GLONASS, GPSConstellation::Galileo,
                                        GPSConstellation::SBAS, GPSConstellation::QZSS,    GPSConstellation::BeiDou,
                                        GPSConstellation::NavIC};

/// Bit positions in the payload. Between the multiple message bit and the masks lie 18 bits about the data's issue,
/// the receiver clock and the smoothing.
constexpr size_t MSM_STATION_ID_OFFSET = 12;
constexpr size_t MSM_STATION_ID_BITS = 12;
constexpr size_t MSM_EPOCH_OFFSET = 24;
constexpr size_t MSM_EPOCH_BITS = 30;
constexpr size_t MSM_MORE_MESSAGES_OFFSET = 54;
constexpr size_t MSM_SATELLITE_MASK_OFFSET = 73;
constexpr size_t MSM_SATELLITE_MASK_BITS = 64;
constexpr size_t MSM_SIGNAL_MASK_OFFSET = 137;
constexpr size_t MSM_SIGNAL_MASK_BITS = 32;
constexpr size_t MSM_CELL_MASK_OFFSET = 169;
}  // namespace

std::optional<RTCMMsmHeader> RTCMMsmHeader::fromFrame(QByteArrayView frame)
{
    const uint16_t messageId = RTCMFramer::frameMessageId(frame);
    if (messageId < MSM_FIRST_MESSAGE) {
        return std::nullopt;
    }
    const size_t constellation = (messageId - MSM_FIRST_MESSAGE) / MSM_MESSAGES_PER_CONSTELLATION;
    const auto type = static_cast<uint8_t>((messageId - MSM_FIRST_MESSAGE) % MSM_MESSAGES_PER_CONSTELLATION + 1);
    if (constellation >= MSM_CONSTELLATIONS.size() || type > MSM_TYPE_COUNT || !RTCMFramer::isValidFrame(frame)) {
        return std::nullopt;
    }
    const std::span<const uint8_t> payload(
        reinterpret_cast<const uint8_t*>(frame.data()) + RTCMFramer::HEADER_SIZE,
        static_cast<size_t>(frame.size()) - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE);
    const size_t payloadBits = payload.size() * 8;
    if (payloadBits < MSM_CELL_MASK_OFFSET) {
        return std::nullopt;
    }
    RTCMMsmHeader header;
    header.constellation = MSM_CONSTELLATIONS[constellation];
    header.msmType = type;
    header.stationId = static_cast<uint16_t>(RTCMBits::field(payload, MSM_STATION_ID_OFFSET, MSM_STATION_ID_BITS));
    header.epochTime = static_cast<uint32_t>(RTCMBits::field(payload, MSM_EPOCH_OFFSET, MSM_EPOCH_BITS));
    header.moreMessages = RTCMBits::field(payload, MSM_MORE_MESSAGES_OFFSET, 1) != 0;
    header.satelliteMask = RTCMBits::field(payload, MSM_SATELLITE_MASK_OFFSET, MSM_SATELLITE_MASK_BITS);
    header.signalMask = static_cast<uint32_t>(RTCMBits::field(payload, MSM_SIGNAL_MASK_OFFSET, MSM_SIGNAL_MASK_BITS));
    const size_t cells = static_cast<size_t>(header.satelliteCount()) * std::popcount(header.signalMask);
    if (cells > MAX_CELLS || payloadBits < MSM_CELL_MASK_OFFSET + cells) {
        return std::nullopt;
    }
    return header;
}
