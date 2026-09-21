#pragma once
#include <array>
#include <stdexcept>
#include <vector>

#include "Femto/FemtoMessages.h"
#include "LittleEndian.h"
#include "RTCMFramer.h"
#include "SBF/SBFMessages.h"

// Fixtures encode documented offsets independently of decoded object alignment.
inline std::vector<uint8_t> rtcmPacket(std::span<const uint8_t> payload)
{
    std::vector<uint8_t> result{0xd3, static_cast<uint8_t>(payload.size() >> 8), static_cast<uint8_t>(payload.size())};
    result.insert(result.end(), payload.begin(), payload.end());
    const auto crc = RTCMFramer::crc24q(result);
    result.insert(result.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    return result;
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

inline std::vector<uint8_t> bytes(const sbf_payload_vel_cov_geodetic_t& v)
{
    std::vector<uint8_t> b(42);
    (void) LittleEndian::write<float>(b, 2, v.cov_vn_vn);
    return b;
}

inline std::vector<uint8_t> bytes(const sbf_payload_dop_t& v)
{
    std::vector<uint8_t> b(18);
    (void) LittleEndian::write<uint16_t>(b, 6, v.hDOP);
    return b;
}

inline std::vector<uint8_t> bytes(const sbf_payload_att_euler& v)
{
    std::vector<uint8_t> b(30);
    (void) LittleEndian::write<uint16_t>(b, 2, v.mode);
    (void) LittleEndian::write<float>(b, 6, v.heading);
    return b;
}

inline std::vector<uint8_t> bytes(const sbf_payload_att_cov_euler& v)
{
    std::vector<uint8_t> b(26);
    (void) LittleEndian::write<float>(b, 2, v.cov_headhead);
    return b;
}

inline std::vector<uint8_t> bytes(const femto_uav_gps_t& v)
{
    std::vector<uint8_t> b(88);
    (void) LittleEndian::write<int32_t>(b, 8, v.lat);
    (void) LittleEndian::write<int32_t>(b, 12, v.lon);
    b[84] = v.fix_type;
    b[86] = v.satellites_used;
    return b;
}
