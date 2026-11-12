#include <algorithm>
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
#include "RTCMStationPosition.h"

// The fuzzer of the RTCM 1005/1006 station position decoder, which reads what a caster or a base station sends. The
// input is the payload of a frame: random bytes would almost never carry a valid CRC-24Q, so the harness frames them.
// That frame, damaged copies of it and the input as it is are decoded, and every result is checked against the
// message as the standard lays it out. The seeds (../corpus/station-position) are a 1005 and a 1006 payload, a
// station on the Earth's axis, the zero coordinates of an unset base, a coordinate far beyond the Earth and a 1005
// payload two bytes too long.

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

void requireStation(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

/// The @a width bits at bit @a offset of @a bytes, most significant bit first; at most 57 bits.
uint64_t stationBits(std::span<const uint8_t> bytes, size_t offset, size_t width)
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
    const uint64_t message = stationBits(payload, 0, 12);
    const bool withHeight = message == STATION_WITH_HEIGHT_MESSAGE;
    if ((message != STATION_MESSAGE && !withHeight) ||
        payload.size() != (withHeight ? STATION_WITH_HEIGHT_PAYLOAD_SIZE : STATION_PAYLOAD_SIZE)) {
        return std::nullopt;
    }
    double meters[3];
    for (size_t axis = 0; axis < 3; ++axis) {
        auto units = static_cast<int64_t>(stationBits(payload, STATION_ECEF_OFFSETS[axis], STATION_ECEF_BITS));
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
            static_cast<double>(stationBits(payload, STATION_HEIGHT_OFFSET, STATION_HEIGHT_BITS)) *
            STATION_METERS_PER_UNIT;
    }
    return expected;
}

void checkStation(std::span<const uint8_t> frame)
{
    const auto station = RTCMStationPosition::fromFrame(
        QByteArrayView(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size())));
    const auto expected = expectedStation(frame);
    requireStation(station.has_value() == expected.has_value());
    if (!station) {
        return;
    }
    const GPSEllipsoidPosition& position = station->position;
    requireStation(std::abs(position.latitudeDegrees) <= 90 && std::abs(position.longitudeDegrees) <= 180);
    requireStation(position.altitudeMeters > STATION_MIN_ALTITUDE_METERS &&
                   position.altitudeMeters < STATION_MAX_ALTITUDE_METERS);
    // The position is where the frame says, to half of the 0.1 mm its coordinates count.
    requireStation(GPSProtocolMath::distance(GPSProtocolMath::toEcef(position), expected->ecef) <
                   STATION_METERS_PER_UNIT / 2);
    requireStation(station->antennaHeightMeters.has_value() == expected->antennaHeightMeters.has_value());
    if (station->antennaHeightMeters) {
        requireStation(std::abs(*station->antennaHeightMeters - *expected->antennaHeightMeters) <
                       STATION_METERS_PER_UNIT / 2);
    }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const std::span<const uint8_t> input(data, size);
    checkStation(input);
    const auto payload = input.first(std::min<size_t>(size, RTCMFramer::MAX_PAYLOAD_SIZE));
    const std::vector<uint8_t> frame = GPSTest::rtcmPacket(payload);
    checkStation(frame);

    // What the decoder must not take for that frame: a wrong CRC, and a missing or an extra byte.
    std::vector<uint8_t> damaged = frame;
    damaged.back() ^= 0x01;
    checkStation(damaged);
    checkStation(std::span(frame).first(frame.size() - 1));
    damaged = frame;
    damaged.push_back(0);
    checkStation(damaged);
    return 0;
}
