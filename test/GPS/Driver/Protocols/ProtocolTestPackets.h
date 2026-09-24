#pragma once
#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../../../../src/GPS/Driver/Protocols/SBF/SBFMessages.h"
#include "../../../../src/GPS/RTCM/RTCMFramer.h"
#include "../../../../src/Utilities/Parsing/Wire/LittleEndian.h"

// Fixtures encode documented offsets independently of decoded object alignment.
inline char hexDigit(unsigned value)
{
    value &= 0xf;
    return static_cast<char>(value < 10 ? '0' + value : 'A' + value - 10);
}

inline std::string nmeaSentence(std::string_view body, std::string_view lineEnding = "\r\n")
{
    uint8_t checksum = 0;
    for (const auto byte : body) {
        checksum ^= static_cast<uint8_t>(byte);
    }
    std::string result;
    result.reserve(body.size() + 4 + lineEnding.size());
    result.push_back('$');
    result.append(body);
    result.push_back('*');
    result.push_back(hexDigit(checksum >> 4));
    result.push_back(hexDigit(checksum));
    result.append(lineEnding);
    return result;
}

inline std::vector<uint8_t> nmeaPacket(std::string_view body, std::string_view lineEnding = "\r\n")
{
    const auto text = nmeaSentence(body, lineEnding);
    return {text.begin(), text.end()};
}

inline void ubxChecksum(std::vector<uint8_t>& frame)
{
    if (frame.size() < 8) {
        throw std::runtime_error("UBX frame too short");
    }
    uint8_t a = 0;
    uint8_t b = 0;
    for (size_t index = 2; index + 2 < frame.size(); ++index) {
        a += frame[index];
        b += a;
    }
    frame[frame.size() - 2] = a;
    frame.back() = b;
}

inline std::vector<uint8_t> ubxFrame(uint16_t message, std::span<const uint8_t> payload)
{
    if (payload.size() > UINT16_MAX) {
        throw std::runtime_error("UBX payload too large");
    }
    std::vector<uint8_t> result{
        0xb5, 0x62, uint8_t(message), uint8_t(message >> 8), uint8_t(payload.size()), uint8_t(payload.size() >> 8)};
    result.insert(result.end(), payload.begin(), payload.end());
    result.insert(result.end(), {0, 0});
    ubxChecksum(result);
    return result;
}

inline std::vector<uint8_t> ubxFrame(uint16_t message, std::initializer_list<uint8_t> payload)
{
    return ubxFrame(message, std::span<const uint8_t>(payload.begin(), payload.size()));
}

inline std::vector<uint8_t> rtcmPacket(std::span<const uint8_t> payload)
{
    std::vector<uint8_t> result{0xd3, static_cast<uint8_t>(payload.size() >> 8), static_cast<uint8_t>(payload.size())};
    result.insert(result.end(), payload.begin(), payload.end());
    const auto crc = RTCMFramer::crc24q(result);
    result.insert(result.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    return result;
}

inline uint16_t sbfCrc16(const uint8_t* data, uint32_t length)
{
    uint16_t crc = 0;
    while (length--) {
        uint8_t x = (crc >> 8) ^ *data++;
        x ^= x >> 4;
        crc = static_cast<uint16_t>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
    }
    return crc;
}

inline std::vector<uint8_t> sbfBlock(uint16_t id, std::span<const uint8_t> payload, uint32_t tow = 0,
                                     uint16_t week = 2435)
{
    if (payload.size() > UINT16_MAX - 14) {
        throw std::runtime_error("SBF payload too large");
    }
    std::vector<uint8_t> packet(14 + payload.size());
    (void) LittleEndian::write<uint16_t>(packet, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(packet, 4, id);
    (void) LittleEndian::write<uint16_t>(packet, 6, uint16_t(packet.size()));
    (void) LittleEndian::write<uint32_t>(packet, 8, tow);
    (void) LittleEndian::write<uint16_t>(packet, 12, week);
    std::copy(payload.begin(), payload.end(), packet.begin() + 14);
    (void) LittleEndian::write<uint16_t>(packet, 2, sbfCrc16(packet.data() + 4, packet.size() - 4));
    return packet;
}

template <typename Driver>
void verifyRTCMRecovery(Driver& driver, std::vector<std::vector<uint8_t>>& frames)
{
    const auto frame = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    auto noisy = frame;
    noisy.insert(noisy.begin(), 0xd3);
    frames.clear();
    driver.consume(noisy);
    if (frames != std::vector<std::vector<uint8_t>>{frame}) {
        throw std::runtime_error("RTCM stray-preamble recovery failed");
    }
    std::vector<uint8_t> payload;
    for (int i = 0; i < 20; ++i) {
        payload.insert(payload.end(), frame.begin(), frame.end());
    }
    auto corrupt = rtcmPacket(payload);
    corrupt.back() ^= 1;
    frames.clear();
    driver.consume(corrupt);
    for (int i = 0; i < 20; ++i) {
        driver.consume({});
    }
    if (frames != std::vector(20, frame)) {
        throw std::runtime_error("RTCM bounded recovery drain failed");
    }
}

inline std::vector<uint8_t> bytes(const sbf_payload_pvt_geodetic_t& v)
{
    std::vector<uint8_t> b(80);
    b[0] = v.mode_type | (v.mode_2d << 7);
    (void) LittleEndian::write<double>(b, 2, v.latitude);
    (void) LittleEndian::write<double>(b, 10, v.longitude);
    (void) LittleEndian::write<double>(b, 18, v.height);
    (void) LittleEndian::write<float>(b, 42, v.cog);
    b[60] = v.nr_sv;
    (void) LittleEndian::write<uint16_t>(b, 76, v.h_accuracy);
    return b;
}
