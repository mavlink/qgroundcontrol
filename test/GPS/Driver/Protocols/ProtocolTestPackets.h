#pragma once
#include <vector>

#include "Femto/FemtoMessages.h"
#include "LittleEndian.h"
#include "SBF/SBFMessages.h"

// Fixtures encode documented offsets independently of decoded object alignment.
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
