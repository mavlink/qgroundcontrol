#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <vector>

#include <QtCore/QByteArrayView>

#include "GPSProtocolMath.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMFramer.h"
#include "RTCMMsmHeader.h"
#include "RTCMStationPosition.h"

// The fuzzer of the decoders that read single messages of what a caster or a base station sends: the RTCM 1005/1006
// station position and the header of the MSM observation messages. The input is the payload of a frame: random bytes
// would almost never carry a valid CRC-24Q, so the harness frames them. That frame, damaged copies of it and the
// input as it is are decoded, and every result is checked against the message as the standard lays it out. The seeds
// (../corpus/station-position) are a 1005 and a 1006 payload, a station on the Earth's axis, the zero coordinates of
// an unset base, a coordinate far beyond the Earth and a 1005 payload two bytes too long; and of the MSM messages a
// GPS MSM7 and a NavIC MSM4 header, one with a cell more than the standard allows and one a bit too short.

namespace {

constexpr uint16_t STATION_MESSAGE = 1005;
constexpr uint16_t STATION_WITH_HEIGHT_MESSAGE = 1006;
constexpr size_t STATION_PAYLOAD_SIZE = 19;
constexpr size_t STATION_WITH_HEIGHT_PAYLOAD_SIZE = 21;
/// The ECEF coordinates follow the 34 bits of the message number, station ID, ITRF year and indicators, two bits
/// apart: 38-bit two's complement counts of 0.1 mm. The antenna height of message 1006 comes after them.
constexpr size_t STATION_ECEF_OFFSETS[] = {34, 74, 114};
constexpr size_t STATION_ECEF_BITS = 38;
constexpr size_t STATION_HEIGHT_OFFSET = 152;
constexpr size_t STATION_HEIGHT_BITS = 16;
constexpr double STATION_METERS_PER_UNIT = 0.0001;
/// Beyond the altitudes that the accepted distances from the Earth's centre, 6000 to 7000 km, reach.
constexpr double STATION_MIN_ALTITUDE_METERS = -400000;
constexpr double STATION_MAX_ALTITUDE_METERS = 650000;

/// The MSM messages are the first seven of each ten numbers from 1071 to 1137, one ten for each constellation.
constexpr uint16_t MSM_FIRST_MESSAGE = 1071;
constexpr uint16_t MSM_LAST_MESSAGE = 1137;
constexpr GPSConstellation MSM_CONSTELLATIONS[] = {
    GPSConstellation::GPS,  GPSConstellation::GLONASS, GPSConstellation::Galileo, GPSConstellation::SBAS,
    GPSConstellation::QZSS, GPSConstellation::BeiDou,  GPSConstellation::NavIC};
/// After the message number come the station ID, 12 bits, the epoch, 30 bits, the multiple message bit and 18 bits
/// about the data's issue, the clock and the smoothing; then the satellite mask, 64 bits, the signal mask, 32 bits,
/// and the cell mask, which has a bit for each satellite and each signal.
constexpr size_t MSM_STATION_OFFSET = 12;
constexpr size_t MSM_EPOCH_OFFSET = 24;
constexpr size_t MSM_MORE_OFFSET = 54;
constexpr size_t MSM_SATELLITES_OFFSET = 73;
constexpr size_t MSM_SIGNALS_OFFSET = 137;
constexpr size_t MSM_CELLS_OFFSET = 169;
constexpr size_t MSM_MAX_CELLS = 64;

void require(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

/// The @a width bits at bit @a offset of @a bytes, most significant bit first; at most 57 bits.
uint64_t bitsAt(std::span<const uint8_t> bytes, size_t offset, size_t width)
{
    uint64_t window = 0;
    for (size_t index = offset / 8; index < offset / 8 + sizeof(window); ++index) {
        window = (window << 8) | (index < bytes.size() ? bytes[index] : 0);
    }
    return (window >> (64 - offset % 8 - width)) & ((uint64_t{1} << width) - 1);
}

struct ExpectedStation
{
    GPSProtocolMath::Ecef ecef;
    std::optional<double> antennaHeightMeters;
};

/// What @a frame holds when the decoder has to accept it: a valid 1005 or 1006 frame of the message's size whose
/// coordinates lie near the Earth's surface.
std::optional<ExpectedStation> expectedStation(std::span<const uint8_t> frame)
{
    if (!RTCMFramer::isValidFrame(frame)) {
        return std::nullopt;
    }
    const auto payload =
        frame.subspan(RTCMFramer::HEADER_SIZE, frame.size() - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE);
    const uint64_t message = bitsAt(payload, 0, 12);
    const bool withHeight = message == STATION_WITH_HEIGHT_MESSAGE;
    if ((message != STATION_MESSAGE && !withHeight) ||
        payload.size() != (withHeight ? STATION_WITH_HEIGHT_PAYLOAD_SIZE : STATION_PAYLOAD_SIZE)) {
        return std::nullopt;
    }
    double meters[3];
    for (size_t axis = 0; axis < 3; ++axis) {
        auto units = static_cast<int64_t>(bitsAt(payload, STATION_ECEF_OFFSETS[axis], STATION_ECEF_BITS));
        if (units >> (STATION_ECEF_BITS - 1)) {
            units -= int64_t{1} << STATION_ECEF_BITS;
        }
        meters[axis] = static_cast<double>(units) * STATION_METERS_PER_UNIT;
    }
    ExpectedStation expected{{meters[0], meters[1], meters[2]}, std::nullopt};
    if (!GPSProtocolMath::nearEarthSurface(expected.ecef)) {
        return std::nullopt;
    }
    if (withHeight) {
        expected.antennaHeightMeters =
            static_cast<double>(bitsAt(payload, STATION_HEIGHT_OFFSET, STATION_HEIGHT_BITS)) * STATION_METERS_PER_UNIT;
    }
    return expected;
}

void checkStation(std::span<const uint8_t> frame)
{
    const auto station = RTCMStationPosition::fromFrame(
        QByteArrayView(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size())));
    const auto expected = expectedStation(frame);
    require(station.has_value() == expected.has_value());
    if (!station) {
        return;
    }
    const GPSEllipsoidPosition& position = station->position;
    require(std::abs(position.latitudeDegrees) <= 90 && std::abs(position.longitudeDegrees) <= 180);
    require(position.altitudeMeters > STATION_MIN_ALTITUDE_METERS &&
            position.altitudeMeters < STATION_MAX_ALTITUDE_METERS);
    // The position is where the frame says, to half of the 0.1 mm its coordinates count.
    require(GPSProtocolMath::distance(GPSProtocolMath::toEcef(position), expected->ecef) < STATION_METERS_PER_UNIT / 2);
    require(station->antennaHeightMeters.has_value() == expected->antennaHeightMeters.has_value());
    if (station->antennaHeightMeters) {
        require(std::abs(*station->antennaHeightMeters - *expected->antennaHeightMeters) < STATION_METERS_PER_UNIT / 2);
    }
}

/// What @a frame holds when the decoder has to accept it: a valid frame of an MSM message with the header and a
/// cell mask of at most 64 cells.
std::optional<RTCMMsmHeader> expectedMsmHeader(std::span<const uint8_t> frame)
{
    if (!RTCMFramer::isValidFrame(frame)) {
        return std::nullopt;
    }
    const auto payload =
        frame.subspan(RTCMFramer::HEADER_SIZE, frame.size() - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE);
    const uint64_t message = bitsAt(payload, 0, 12);
    if (message < MSM_FIRST_MESSAGE || message > MSM_LAST_MESSAGE || (message - MSM_FIRST_MESSAGE) % 10 >= 7 ||
        payload.size() * 8 < MSM_CELLS_OFFSET) {
        return std::nullopt;
    }
    RTCMMsmHeader expected;
    expected.constellation = MSM_CONSTELLATIONS[(message - MSM_FIRST_MESSAGE) / 10];
    expected.msmType = static_cast<uint8_t>((message - MSM_FIRST_MESSAGE) % 10 + 1);
    expected.stationId = static_cast<uint16_t>(bitsAt(payload, MSM_STATION_OFFSET, 12));
    expected.epochTime = static_cast<uint32_t>(bitsAt(payload, MSM_EPOCH_OFFSET, 30));
    expected.moreMessages = bitsAt(payload, MSM_MORE_OFFSET, 1) != 0;
    // In two halves: bitsAt() reads at most 57 bits.
    expected.satelliteMask =
        (bitsAt(payload, MSM_SATELLITES_OFFSET, 32) << 32) | bitsAt(payload, MSM_SATELLITES_OFFSET + 32, 32);
    expected.signalMask = static_cast<uint32_t>(bitsAt(payload, MSM_SIGNALS_OFFSET, 32));
    const size_t cells =
        static_cast<size_t>(std::popcount(expected.satelliteMask)) * std::popcount(expected.signalMask);
    if (cells > MSM_MAX_CELLS || payload.size() * 8 < MSM_CELLS_OFFSET + cells) {
        return std::nullopt;
    }
    return expected;
}

void checkMsmHeader(std::span<const uint8_t> frame)
{
    const auto header = RTCMMsmHeader::fromFrame(
        QByteArrayView(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size())));
    const auto expected = expectedMsmHeader(frame);
    require(header.has_value() == expected.has_value());
    if (!header) {
        return;
    }
    require(header->constellation == expected->constellation && header->msmType == expected->msmType);
    require(header->stationId == expected->stationId && header->epochTime == expected->epochTime);
    require(header->moreMessages == expected->moreMessages);
    require(header->satelliteMask == expected->satelliteMask && header->signalMask == expected->signalMask);
    require(header->satelliteCount() == std::popcount(expected->satelliteMask));
}

void checkFrame(std::span<const uint8_t> frame)
{
    checkStation(frame);
    checkMsmHeader(frame);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const std::span<const uint8_t> input(data, size);
    checkFrame(input);
    const auto payload = input.first(std::min<size_t>(size, RTCMFramer::MAX_PAYLOAD_SIZE));
    const std::vector<uint8_t> frame = GPSTest::rtcmPacket(payload);
    checkFrame(frame);

    // What the decoders must not take for that frame: a wrong CRC, and a missing or an extra byte.
    std::vector<uint8_t> damaged = frame;
    damaged.back() ^= 0x01;
    checkFrame(damaged);
    checkFrame(std::span(frame).first(frame.size() - 1));
    damaged = frame;
    damaged.push_back(0);
    checkFrame(damaged);
    return 0;
}
