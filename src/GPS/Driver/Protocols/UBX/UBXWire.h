/****************************************************************************
 *
 *   Copyright (c) 2012-2023 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#pragma once
#include <algorithm>
#include <array>
#include <span>

#include "GPSWire.h"
#include "UBXMessages.h"

namespace UBX {
// All callers validate frame length/version before decoding; absent optional tails stay zero.
template <typename T>
T payload(std::span<const uint8_t> bytes, size_t offset = 0);
template <typename T>
std::array<uint8_t, sizeof(T)> encode(const T& value);

template <>
inline ubx_payload_rx_nav_posllh_t payload<ubx_payload_rx_nav_posllh_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_posllh_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.lon = GPSWire::read<int32_t>(bytes, 4).value_or(0);
    value.lat = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.height = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.hMSL = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.hAcc = GPSWire::read<uint32_t>(bytes, 20).value_or(0);
    value.vAcc = GPSWire::read<uint32_t>(bytes, 24).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_dop_t payload<ubx_payload_rx_nav_dop_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_dop_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.gDOP = GPSWire::read<uint16_t>(bytes, 4).value_or(0);
    value.pDOP = GPSWire::read<uint16_t>(bytes, 6).value_or(0);
    value.tDOP = GPSWire::read<uint16_t>(bytes, 8).value_or(0);
    value.vDOP = GPSWire::read<uint16_t>(bytes, 10).value_or(0);
    value.hDOP = GPSWire::read<uint16_t>(bytes, 12).value_or(0);
    value.nDOP = GPSWire::read<uint16_t>(bytes, 14).value_or(0);
    value.eDOP = GPSWire::read<uint16_t>(bytes, 16).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sol_t payload<ubx_payload_rx_nav_sol_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sol_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.fTOW = GPSWire::read<int32_t>(bytes, 4).value_or(0);
    value.week = GPSWire::read<int16_t>(bytes, 8).value_or(0);
    value.gpsFix = GPSWire::read<uint8_t>(bytes, 10).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 11).value_or(0);
    value.ecefX = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.ecefY = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.ecefZ = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.pAcc = GPSWire::read<uint32_t>(bytes, 24).value_or(0);
    value.ecefVX = GPSWire::read<int32_t>(bytes, 28).value_or(0);
    value.ecefVY = GPSWire::read<int32_t>(bytes, 32).value_or(0);
    value.ecefVZ = GPSWire::read<int32_t>(bytes, 36).value_or(0);
    value.sAcc = GPSWire::read<uint32_t>(bytes, 40).value_or(0);
    value.pDOP = GPSWire::read<uint16_t>(bytes, 44).value_or(0);
    value.reserved1 = GPSWire::read<uint8_t>(bytes, 46).value_or(0);
    value.numSV = GPSWire::read<uint8_t>(bytes, 47).value_or(0);
    value.reserved2 = GPSWire::read<uint32_t>(bytes, 48).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_pvt_t payload<ubx_payload_rx_nav_pvt_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_pvt_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.year = GPSWire::read<uint16_t>(bytes, 4).value_or(0);
    value.month = GPSWire::read<uint8_t>(bytes, 6).value_or(0);
    value.day = GPSWire::read<uint8_t>(bytes, 7).value_or(0);
    value.hour = GPSWire::read<uint8_t>(bytes, 8).value_or(0);
    value.min = GPSWire::read<uint8_t>(bytes, 9).value_or(0);
    value.sec = GPSWire::read<uint8_t>(bytes, 10).value_or(0);
    value.valid = GPSWire::read<uint8_t>(bytes, 11).value_or(0);
    value.tAcc = GPSWire::read<uint32_t>(bytes, 12).value_or(0);
    value.nano = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.fixType = GPSWire::read<uint8_t>(bytes, 20).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 21).value_or(0);
    value.reserved1 = GPSWire::read<uint8_t>(bytes, 22).value_or(0);
    value.numSV = GPSWire::read<uint8_t>(bytes, 23).value_or(0);
    value.lon = GPSWire::read<int32_t>(bytes, 24).value_or(0);
    value.lat = GPSWire::read<int32_t>(bytes, 28).value_or(0);
    value.height = GPSWire::read<int32_t>(bytes, 32).value_or(0);
    value.hMSL = GPSWire::read<int32_t>(bytes, 36).value_or(0);
    value.hAcc = GPSWire::read<uint32_t>(bytes, 40).value_or(0);
    value.vAcc = GPSWire::read<uint32_t>(bytes, 44).value_or(0);
    value.velN = GPSWire::read<int32_t>(bytes, 48).value_or(0);
    value.velE = GPSWire::read<int32_t>(bytes, 52).value_or(0);
    value.velD = GPSWire::read<int32_t>(bytes, 56).value_or(0);
    value.gSpeed = GPSWire::read<int32_t>(bytes, 60).value_or(0);
    value.headMot = GPSWire::read<int32_t>(bytes, 64).value_or(0);
    value.sAcc = GPSWire::read<uint32_t>(bytes, 68).value_or(0);
    value.headAcc = GPSWire::read<uint32_t>(bytes, 72).value_or(0);
    value.pDOP = GPSWire::read<uint16_t>(bytes, 76).value_or(0);
    value.reserved2 = GPSWire::read<uint16_t>(bytes, 78).value_or(0);
    value.reserved3 = GPSWire::read<uint32_t>(bytes, 80).value_or(0);
    value.headVeh = GPSWire::read<int32_t>(bytes, 84).value_or(0);
    value.reserved4 = GPSWire::read<uint32_t>(bytes, 88).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_timeutc_t payload<ubx_payload_rx_nav_timeutc_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_timeutc_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.tAcc = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.nano = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.year = GPSWire::read<uint16_t>(bytes, 12).value_or(0);
    value.month = GPSWire::read<uint8_t>(bytes, 14).value_or(0);
    value.day = GPSWire::read<uint8_t>(bytes, 15).value_or(0);
    value.hour = GPSWire::read<uint8_t>(bytes, 16).value_or(0);
    value.min = GPSWire::read<uint8_t>(bytes, 17).value_or(0);
    value.sec = GPSWire::read<uint8_t>(bytes, 18).value_or(0);
    value.valid = GPSWire::read<uint8_t>(bytes, 19).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svinfo_part1_t payload<ubx_payload_rx_nav_svinfo_part1_t>(std::span<const uint8_t> input,
                                                                                    size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svinfo_part1_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.numCh = GPSWire::read<uint8_t>(bytes, 4).value_or(0);
    value.globalFlags = GPSWire::read<uint8_t>(bytes, 5).value_or(0);
    value.reserved2 = GPSWire::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svinfo_part2_t payload<ubx_payload_rx_nav_svinfo_part2_t>(std::span<const uint8_t> input,
                                                                                    size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svinfo_part2_t value{};
    value.chn = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.svid = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 2).value_or(0);
    value.quality = GPSWire::read<uint8_t>(bytes, 3).value_or(0);
    value.cno = GPSWire::read<uint8_t>(bytes, 4).value_or(0);
    value.elev = GPSWire::read<int8_t>(bytes, 5).value_or(0);
    value.azim = GPSWire::read<int16_t>(bytes, 6).value_or(0);
    value.prRes = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sat_part1_t payload<ubx_payload_rx_nav_sat_part1_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sat_part1_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.version = GPSWire::read<uint8_t>(bytes, 4).value_or(0);
    value.numSvs = GPSWire::read<uint8_t>(bytes, 5).value_or(0);
    value.reserved = GPSWire::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sat_part2_t payload<ubx_payload_rx_nav_sat_part2_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sat_part2_t value{};
    value.gnssId = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.svId = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.cno = GPSWire::read<uint8_t>(bytes, 2).value_or(0);
    value.elev = GPSWire::read<int8_t>(bytes, 3).value_or(0);
    value.azim = GPSWire::read<int16_t>(bytes, 4).value_or(0);
    value.prRes = GPSWire::read<int16_t>(bytes, 6).value_or(0);
    value.flags = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_status_t payload<ubx_payload_rx_nav_status_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_status_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.gpsFix = GPSWire::read<uint8_t>(bytes, 4).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 5).value_or(0);
    value.fixStat = GPSWire::read<uint8_t>(bytes, 6).value_or(0);
    value.flags2 = GPSWire::read<uint8_t>(bytes, 7).value_or(0);
    value.ttff = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    value.msss = GPSWire::read<uint32_t>(bytes, 12).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svin_t payload<ubx_payload_rx_nav_svin_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svin_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved1[i] = GPSWire::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.iTOW = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.dur = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    value.meanX = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.meanY = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.meanZ = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.meanXHP = GPSWire::read<int8_t>(bytes, 24).value_or(0);
    value.meanYHP = GPSWire::read<int8_t>(bytes, 25).value_or(0);
    value.meanZHP = GPSWire::read<int8_t>(bytes, 26).value_or(0);
    value.reserved2 = GPSWire::read<int8_t>(bytes, 27).value_or(0);
    value.meanAcc = GPSWire::read<uint32_t>(bytes, 28).value_or(0);
    value.obs = GPSWire::read<uint32_t>(bytes, 32).value_or(0);
    value.valid = GPSWire::read<uint8_t>(bytes, 36).value_or(0);
    value.active = GPSWire::read<uint8_t>(bytes, 37).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved3[i] = GPSWire::read<uint8_t>(bytes, 38 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_velned_t payload<ubx_payload_rx_nav_velned_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_velned_t value{};
    value.iTOW = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.velN = GPSWire::read<int32_t>(bytes, 4).value_or(0);
    value.velE = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.velD = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.speed = GPSWire::read<uint32_t>(bytes, 16).value_or(0);
    value.gSpeed = GPSWire::read<uint32_t>(bytes, 20).value_or(0);
    value.heading = GPSWire::read<int32_t>(bytes, 24).value_or(0);
    value.sAcc = GPSWire::read<uint32_t>(bytes, 28).value_or(0);
    value.cAcc = GPSWire::read<uint32_t>(bytes, 32).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_hw_ubx6_t payload<ubx_payload_rx_mon_hw_ubx6_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_hw_ubx6_t value{};
    value.pinSel = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.pinBank = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.pinDir = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    value.pinVal = GPSWire::read<uint32_t>(bytes, 12).value_or(0);
    value.noisePerMS = GPSWire::read<uint16_t>(bytes, 16).value_or(0);
    value.agcCnt = GPSWire::read<uint16_t>(bytes, 18).value_or(0);
    value.aStatus = GPSWire::read<uint8_t>(bytes, 20).value_or(0);
    value.aPower = GPSWire::read<uint8_t>(bytes, 21).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 22).value_or(0);
    value.reserved1 = GPSWire::read<uint8_t>(bytes, 23).value_or(0);
    value.usedMask = GPSWire::read<uint32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 25; ++i)
        value.VP[i] = GPSWire::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.jamInd = GPSWire::read<uint8_t>(bytes, 53).value_or(0);
    value.reserved3 = GPSWire::read<uint16_t>(bytes, 54).value_or(0);
    value.pinIrq = GPSWire::read<uint32_t>(bytes, 56).value_or(0);
    value.pullH = GPSWire::read<uint32_t>(bytes, 60).value_or(0);
    value.pullL = GPSWire::read<uint32_t>(bytes, 64).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_hw_ubx7_t payload<ubx_payload_rx_mon_hw_ubx7_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_hw_ubx7_t value{};
    value.pinSel = GPSWire::read<uint32_t>(bytes, 0).value_or(0);
    value.pinBank = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.pinDir = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    value.pinVal = GPSWire::read<uint32_t>(bytes, 12).value_or(0);
    value.noisePerMS = GPSWire::read<uint16_t>(bytes, 16).value_or(0);
    value.agcCnt = GPSWire::read<uint16_t>(bytes, 18).value_or(0);
    value.aStatus = GPSWire::read<uint8_t>(bytes, 20).value_or(0);
    value.aPower = GPSWire::read<uint8_t>(bytes, 21).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 22).value_or(0);
    value.reserved1 = GPSWire::read<uint8_t>(bytes, 23).value_or(0);
    value.usedMask = GPSWire::read<uint32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 17; ++i)
        value.VP[i] = GPSWire::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.jamInd = GPSWire::read<uint8_t>(bytes, 45).value_or(0);
    value.reserved3 = GPSWire::read<uint16_t>(bytes, 46).value_or(0);
    value.pinIrq = GPSWire::read<uint32_t>(bytes, 48).value_or(0);
    value.pullH = GPSWire::read<uint32_t>(bytes, 52).value_or(0);
    value.pullL = GPSWire::read<uint32_t>(bytes, 56).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_hw_deprecated_t payload<ubx_payload_rx_mon_hw_deprecated_t>(std::span<const uint8_t> input,
                                                                                      size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_hw_deprecated_t value{};
    for (size_t i = 0; i < 56; ++i)
        value.reserved0[i] = GPSWire::read<uint8_t>(bytes, 0 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_sec_sig_t payload<ubx_payload_rx_sec_sig_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_sec_sig_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.reserved0 = GPSWire::read<uint8_t>(bytes, 2).value_or(0);
    value.jamNumCentFreqs = GPSWire::read<uint8_t>(bytes, 3).value_or(0);
    value.jamFlags = GPSWire::read<uint8_t>(bytes, 4).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_ver_part1_t payload<ubx_payload_rx_mon_ver_part1_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_ver_part1_t value{};
    for (size_t i = 0; i < 30; ++i)
        value.swVersion[i] = GPSWire::read<uint8_t>(bytes, 0 + i * 1).value_or(0);
    for (size_t i = 0; i < 10; ++i)
        value.hwVersion[i] = GPSWire::read<uint8_t>(bytes, 30 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_ver_part2_t payload<ubx_payload_rx_mon_ver_part2_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_ver_part2_t value{};
    for (size_t i = 0; i < 30; ++i)
        value.extension[i] = GPSWire::read<uint8_t>(bytes, 0 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_rxm_rtcm_t payload<ubx_payload_rx_rxm_rtcm_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_rxm_rtcm_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.subType = GPSWire::read<uint16_t>(bytes, 2).value_or(0);
    value.refStationID = GPSWire::read<uint16_t>(bytes, 4).value_or(0);
    value.msgType = GPSWire::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_rxm_cor_t payload<ubx_payload_rx_rxm_cor_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_rxm_cor_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.ebno = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved0[i] = GPSWire::read<uint8_t>(bytes, 2 + i * 1).value_or(0);
    value.statusInfo = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.msgType = GPSWire::read<uint16_t>(bytes, 8).value_or(0);
    value.msgSubType = GPSWire::read<uint16_t>(bytes, 10).value_or(0);
    return value;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_prt_t)> encode(const ubx_payload_tx_cfg_prt_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_prt_t) == 20);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_prt_t)> bytes{};
    GPSWire::write(bytes, 0, value.portID);
    GPSWire::write(bytes, 1, value.reserved0);
    GPSWire::write(bytes, 2, value.txReady);
    GPSWire::write(bytes, 4, value.mode);
    GPSWire::write(bytes, 8, value.baudRate);
    GPSWire::write(bytes, 12, value.inProtoMask);
    GPSWire::write(bytes, 14, value.outProtoMask);
    GPSWire::write(bytes, 16, value.flags);
    GPSWire::write(bytes, 18, value.reserved5);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_rate_t)> encode(const ubx_payload_tx_cfg_rate_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_rate_t) == 6);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_rate_t)> bytes{};
    GPSWire::write(bytes, 0, value.measRate);
    GPSWire::write(bytes, 2, value.navRate);
    GPSWire::write(bytes, 4, value.timeRef);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_cfg_t)> encode(const ubx_payload_tx_cfg_cfg_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_cfg_t) == 13);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_cfg_t)> bytes{};
    GPSWire::write(bytes, 0, value.clearMask);
    GPSWire::write(bytes, 4, value.saveMask);
    GPSWire::write(bytes, 8, value.loadMask);
    GPSWire::write(bytes, 12, value.deviceMask);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_nav5_t)> encode(const ubx_payload_tx_cfg_nav5_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_nav5_t) == 36);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_nav5_t)> bytes{};
    GPSWire::write(bytes, 0, value.mask);
    GPSWire::write(bytes, 2, value.dynModel);
    GPSWire::write(bytes, 3, value.fixMode);
    GPSWire::write(bytes, 4, value.fixedAlt);
    GPSWire::write(bytes, 8, value.fixedAltVar);
    GPSWire::write(bytes, 12, value.minElev);
    GPSWire::write(bytes, 13, value.drLimit);
    GPSWire::write(bytes, 14, value.pDop);
    GPSWire::write(bytes, 16, value.tDop);
    GPSWire::write(bytes, 18, value.pAcc);
    GPSWire::write(bytes, 20, value.tAcc);
    GPSWire::write(bytes, 22, value.staticHoldThresh);
    GPSWire::write(bytes, 23, value.dgpsTimeOut);
    GPSWire::write(bytes, 24, value.cnoThreshNumSVs);
    GPSWire::write(bytes, 25, value.cnoThresh);
    GPSWire::write(bytes, 26, value.reserved);
    GPSWire::write(bytes, 28, value.staticHoldMaxDist);
    GPSWire::write(bytes, 30, value.utcStandard);
    GPSWire::write(bytes, 31, value.reserved3);
    GPSWire::write(bytes, 32, value.reserved4);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_rst_t)> encode(const ubx_payload_tx_cfg_rst_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_rst_t) == 4);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_rst_t)> bytes{};
    GPSWire::write(bytes, 0, value.navBbrMask);
    GPSWire::write(bytes, 2, value.resetMode);
    GPSWire::write(bytes, 3, value.reserved1);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_sbas_t)> encode(const ubx_payload_tx_cfg_sbas_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_sbas_t) == 8);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_sbas_t)> bytes{};
    GPSWire::write(bytes, 0, value.mode);
    GPSWire::write(bytes, 1, value.usage);
    GPSWire::write(bytes, 2, value.maxSBAS);
    GPSWire::write(bytes, 3, value.scanmode2);
    GPSWire::write(bytes, 4, value.scanmode1);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_tmode3_t)> encode(const ubx_payload_tx_cfg_tmode3_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_tmode3_t) == 40);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_tmode3_t)> bytes{};
    GPSWire::write(bytes, 0, value.version);
    GPSWire::write(bytes, 1, value.reserved1);
    GPSWire::write(bytes, 2, value.flags);
    GPSWire::write(bytes, 4, value.ecefXOrLat);
    GPSWire::write(bytes, 8, value.ecefYOrLon);
    GPSWire::write(bytes, 12, value.ecefZOrAlt);
    GPSWire::write(bytes, 16, value.ecefXOrLatHP);
    GPSWire::write(bytes, 17, value.ecefYOrLonHP);
    GPSWire::write(bytes, 18, value.ecefZOrAltHP);
    GPSWire::write(bytes, 19, value.reserved2);
    GPSWire::write(bytes, 20, value.fixedPosAcc);
    GPSWire::write(bytes, 24, value.svinMinDur);
    GPSWire::write(bytes, 28, value.svinAccLimit);
    for (size_t i = 0; i < 8; ++i) {
        GPSWire::write(bytes, 32 + i * 1, value.reserved3[i]);
    }
    return bytes;
}

template <>
inline ubx_payload_rx_nav_relposned_t payload<ubx_payload_rx_nav_relposned_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_relposned_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.reserved0 = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.refStationId = GPSWire::read<uint16_t>(bytes, 2).value_or(0);
    value.iTOW = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.relPosN = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.relPosE = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.relPosD = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.relPosLength = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.relPosHeading = GPSWire::read<int32_t>(bytes, 24).value_or(0);
    value.reserved1 = GPSWire::read<uint32_t>(bytes, 28).value_or(0);
    value.relPosHPN = GPSWire::read<int8_t>(bytes, 32).value_or(0);
    value.relPosHPE = GPSWire::read<int8_t>(bytes, 33).value_or(0);
    value.relPosHPD = GPSWire::read<int8_t>(bytes, 34).value_or(0);
    value.relPosHPLength = GPSWire::read<int8_t>(bytes, 35).value_or(0);
    value.accN = GPSWire::read<uint32_t>(bytes, 36).value_or(0);
    value.accE = GPSWire::read<uint32_t>(bytes, 40).value_or(0);
    value.accD = GPSWire::read<uint32_t>(bytes, 44).value_or(0);
    value.accLength = GPSWire::read<uint32_t>(bytes, 48).value_or(0);
    value.accHeading = GPSWire::read<uint32_t>(bytes, 52).value_or(0);
    value.reserved2 = GPSWire::read<uint32_t>(bytes, 56).value_or(0);
    value.flags = GPSWire::read<uint32_t>(bytes, 60).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_daheading_t payload<ubx_payload_rx_nav_daheading_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_daheading_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved0[i] = GPSWire::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.iTOW = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.relPosN = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.relPosE = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.relPosD = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.relPosLength = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.relPosHeading = GPSWire::read<int32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved1[i] = GPSWire::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.accN = GPSWire::read<uint32_t>(bytes, 32).value_or(0);
    value.accE = GPSWire::read<uint32_t>(bytes, 36).value_or(0);
    value.accD = GPSWire::read<uint32_t>(bytes, 40).value_or(0);
    value.accLength = GPSWire::read<uint32_t>(bytes, 44).value_or(0);
    value.accHeading = GPSWire::read<uint32_t>(bytes, 48).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved2[i] = GPSWire::read<uint8_t>(bytes, 52 + i * 1).value_or(0);
    value.flags = GPSWire::read<uint32_t>(bytes, 56).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_hpposllh_t payload<ubx_payload_rx_nav_hpposllh_t>(std::span<const uint8_t> input,
                                                                            size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_hpposllh_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved1[i] = GPSWire::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.flags = GPSWire::read<int8_t>(bytes, 3).value_or(0);
    value.iTOW = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.lon = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.lat = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.height = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.hMSL = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.lonHp = GPSWire::read<int8_t>(bytes, 24).value_or(0);
    value.latHp = GPSWire::read<int8_t>(bytes, 25).value_or(0);
    value.heightHp = GPSWire::read<int8_t>(bytes, 26).value_or(0);
    value.hMSLHp = GPSWire::read<int8_t>(bytes, 27).value_or(0);
    value.hAcc = GPSWire::read<uint32_t>(bytes, 28).value_or(0);
    value.vAcc = GPSWire::read<uint32_t>(bytes, 32).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_comms_port_t payload<ubx_payload_rx_mon_comms_port_t>(std::span<const uint8_t> input,
                                                                                size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_comms_port_t value{};
    value.portId = GPSWire::read<uint16_t>(bytes, 0).value_or(0);
    value.txPending = GPSWire::read<uint16_t>(bytes, 2).value_or(0);
    value.txBytes = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    value.txUsage = GPSWire::read<uint8_t>(bytes, 8).value_or(0);
    value.txPeakUsage = GPSWire::read<uint8_t>(bytes, 9).value_or(0);
    value.rxPending = GPSWire::read<uint16_t>(bytes, 10).value_or(0);
    value.rxBytes = GPSWire::read<uint32_t>(bytes, 12).value_or(0);
    value.rxUsage = GPSWire::read<uint8_t>(bytes, 16).value_or(0);
    value.rxPeakUsage = GPSWire::read<uint8_t>(bytes, 17).value_or(0);
    value.overrunErrs = GPSWire::read<uint16_t>(bytes, 18).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.msgs[i] = GPSWire::read<uint16_t>(bytes, 20 + i * 2).value_or(0);
    for (size_t i = 0; i < 8; ++i)
        value.reserved[i] = GPSWire::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.skipped = GPSWire::read<uint32_t>(bytes, 36).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_comms_t payload<ubx_payload_rx_mon_comms_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_comms_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.nPorts = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.txErrors = GPSWire::read<uint8_t>(bytes, 2).value_or(0);
    value.reserved = GPSWire::read<uint8_t>(bytes, 3).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.protIds[i] = GPSWire::read<uint8_t>(bytes, 4 + i * 1).value_or(0);
    for (size_t i = 0; i < 8; ++i)
        value.ports[i] = payload<ubx_payload_rx_mon_comms_port_t>(bytes, 8 + i * 40);
    return value;
}

template <>
inline ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t
payload<ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t value{};
    value.blockId = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    value.antStatus = GPSWire::read<uint8_t>(bytes, 2).value_or(0);
    value.antPower = GPSWire::read<uint8_t>(bytes, 3).value_or(0);
    value.postStatus = GPSWire::read<uint32_t>(bytes, 4).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved2[i] = GPSWire::read<uint8_t>(bytes, 8 + i * 1).value_or(0);
    value.noisePerMS = GPSWire::read<uint16_t>(bytes, 12).value_or(0);
    value.agcCnt = GPSWire::read<uint16_t>(bytes, 14).value_or(0);
    value.jamInd = GPSWire::read<uint8_t>(bytes, 16).value_or(0);
    value.ofsI = GPSWire::read<int8_t>(bytes, 17).value_or(0);
    value.magI = GPSWire::read<uint8_t>(bytes, 18).value_or(0);
    value.ofsQ = GPSWire::read<int8_t>(bytes, 19).value_or(0);
    value.magQ = GPSWire::read<uint8_t>(bytes, 20).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved3[i] = GPSWire::read<uint8_t>(bytes, 21 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_rf_t payload<ubx_payload_rx_mon_rf_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_rf_t value{};
    value.version = GPSWire::read<uint8_t>(bytes, 0).value_or(0);
    value.nBlocks = GPSWire::read<uint8_t>(bytes, 1).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved1[i] = GPSWire::read<uint8_t>(bytes, 2 + i * 1).value_or(0);
    for (size_t i = 0; i < 1; ++i)
        value.block[i] = payload<ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t>(bytes, 4 + i * 24);
    return value;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t)> encode(
    const ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t) == 8);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t)> bytes{};
    GPSWire::write(bytes, 0, value.gnssId);
    GPSWire::write(bytes, 1, value.resTrkCh);
    GPSWire::write(bytes, 2, value.maxTrkCh);
    GPSWire::write(bytes, 3, value.reserved1);
    GPSWire::write(bytes, 4, value.flags);
    return bytes;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_gnss_t)> encode(const ubx_payload_tx_cfg_gnss_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_gnss_t) == 60);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_gnss_t)> bytes{};
    GPSWire::write(bytes, 0, value.msgVer);
    GPSWire::write(bytes, 1, value.numTrkChHw);
    GPSWire::write(bytes, 2, value.numTrkChUse);
    GPSWire::write(bytes, 3, value.numConfigBlocks);
    for (size_t i = 0; i < 7; ++i) {
        const auto block = encode(value.block[i]);
        std::copy(block.begin(), block.end(), bytes.begin() + 4 + i * 8);
    }
    return bytes;
}

template <>
inline ubx_payload_rx_ack_ack_t payload<ubx_payload_rx_ack_ack_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_ack_ack_t value{};
    value.msg = GPSWire::read<uint16_t>(bytes, 0).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_ack_nak_t payload<ubx_payload_rx_ack_nak_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_ack_nak_t value{};
    value.msg = GPSWire::read<uint16_t>(bytes, 0).value_or(0);
    return value;
}

template <>
inline std::array<uint8_t, sizeof(ubx_payload_tx_cfg_msg_t)> encode(const ubx_payload_tx_cfg_msg_t& value)
{
    static_assert(sizeof(ubx_payload_tx_cfg_msg_t) == 3);
    std::array<uint8_t, sizeof(ubx_payload_tx_cfg_msg_t)> bytes{};
    GPSWire::write(bytes, 0, value.msg);
    GPSWire::write(bytes, 2, value.rate);
    return bytes;
}

template <typename T, size_t N>
std::array<uint8_t, sizeof(T) * N> encode(const T (&values)[N])
{
    std::array<uint8_t, sizeof(T) * N> bytes{};
    for (size_t i = 0; i < N; ++i) {
        const auto block = encode(values[i]);
        std::copy(block.begin(), block.end(), bytes.begin() + i * sizeof(T));
    }
    return bytes;
}
}  // namespace UBX
