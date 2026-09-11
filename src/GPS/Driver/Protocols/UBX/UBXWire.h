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

#include "LittleEndian.h"
#include "UBXMessages.h"

namespace UBX {
// All callers validate frame length/version before decoding; absent optional tails stay zero.
template <typename T>
T payload(std::span<const uint8_t> bytes, size_t offset = 0);
template <typename T>
std::array<uint8_t, WIRE_SIZE<T>> encode(const T& value);

template <>
inline ubx_payload_rx_nav_posllh_t payload<ubx_payload_rx_nav_posllh_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_posllh_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.lon = LittleEndian::read<int32_t>(bytes, 4).value_or(0);
    value.lat = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.height = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.hMSL = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.hAcc = LittleEndian::read<uint32_t>(bytes, 20).value_or(0);
    value.vAcc = LittleEndian::read<uint32_t>(bytes, 24).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_dop_t payload<ubx_payload_rx_nav_dop_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_dop_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.gDOP = LittleEndian::read<uint16_t>(bytes, 4).value_or(0);
    value.pDOP = LittleEndian::read<uint16_t>(bytes, 6).value_or(0);
    value.tDOP = LittleEndian::read<uint16_t>(bytes, 8).value_or(0);
    value.vDOP = LittleEndian::read<uint16_t>(bytes, 10).value_or(0);
    value.hDOP = LittleEndian::read<uint16_t>(bytes, 12).value_or(0);
    value.nDOP = LittleEndian::read<uint16_t>(bytes, 14).value_or(0);
    value.eDOP = LittleEndian::read<uint16_t>(bytes, 16).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sol_t payload<ubx_payload_rx_nav_sol_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sol_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.fTOW = LittleEndian::read<int32_t>(bytes, 4).value_or(0);
    value.week = LittleEndian::read<int16_t>(bytes, 8).value_or(0);
    value.gpsFix = LittleEndian::read<uint8_t>(bytes, 10).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 11).value_or(0);
    value.ecefX = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.ecefY = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.ecefZ = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.pAcc = LittleEndian::read<uint32_t>(bytes, 24).value_or(0);
    value.ecefVX = LittleEndian::read<int32_t>(bytes, 28).value_or(0);
    value.ecefVY = LittleEndian::read<int32_t>(bytes, 32).value_or(0);
    value.ecefVZ = LittleEndian::read<int32_t>(bytes, 36).value_or(0);
    value.sAcc = LittleEndian::read<uint32_t>(bytes, 40).value_or(0);
    value.pDOP = LittleEndian::read<uint16_t>(bytes, 44).value_or(0);
    value.reserved1 = LittleEndian::read<uint8_t>(bytes, 46).value_or(0);
    value.numSV = LittleEndian::read<uint8_t>(bytes, 47).value_or(0);
    value.reserved2 = LittleEndian::read<uint32_t>(bytes, 48).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_pvt_t payload<ubx_payload_rx_nav_pvt_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_pvt_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.year = LittleEndian::read<uint16_t>(bytes, 4).value_or(0);
    value.month = LittleEndian::read<uint8_t>(bytes, 6).value_or(0);
    value.day = LittleEndian::read<uint8_t>(bytes, 7).value_or(0);
    value.hour = LittleEndian::read<uint8_t>(bytes, 8).value_or(0);
    value.min = LittleEndian::read<uint8_t>(bytes, 9).value_or(0);
    value.sec = LittleEndian::read<uint8_t>(bytes, 10).value_or(0);
    value.valid = LittleEndian::read<uint8_t>(bytes, 11).value_or(0);
    value.tAcc = LittleEndian::read<uint32_t>(bytes, 12).value_or(0);
    value.nano = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.fixType = LittleEndian::read<uint8_t>(bytes, 20).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 21).value_or(0);
    value.reserved1 = LittleEndian::read<uint8_t>(bytes, 22).value_or(0);
    value.numSV = LittleEndian::read<uint8_t>(bytes, 23).value_or(0);
    value.lon = LittleEndian::read<int32_t>(bytes, 24).value_or(0);
    value.lat = LittleEndian::read<int32_t>(bytes, 28).value_or(0);
    value.height = LittleEndian::read<int32_t>(bytes, 32).value_or(0);
    value.hMSL = LittleEndian::read<int32_t>(bytes, 36).value_or(0);
    value.hAcc = LittleEndian::read<uint32_t>(bytes, 40).value_or(0);
    value.vAcc = LittleEndian::read<uint32_t>(bytes, 44).value_or(0);
    value.velN = LittleEndian::read<int32_t>(bytes, 48).value_or(0);
    value.velE = LittleEndian::read<int32_t>(bytes, 52).value_or(0);
    value.velD = LittleEndian::read<int32_t>(bytes, 56).value_or(0);
    value.gSpeed = LittleEndian::read<int32_t>(bytes, 60).value_or(0);
    value.headMot = LittleEndian::read<int32_t>(bytes, 64).value_or(0);
    value.sAcc = LittleEndian::read<uint32_t>(bytes, 68).value_or(0);
    value.headAcc = LittleEndian::read<uint32_t>(bytes, 72).value_or(0);
    value.pDOP = LittleEndian::read<uint16_t>(bytes, 76).value_or(0);
    value.reserved2 = LittleEndian::read<uint16_t>(bytes, 78).value_or(0);
    value.reserved3 = LittleEndian::read<uint32_t>(bytes, 80).value_or(0);
    value.headVeh = LittleEndian::read<int32_t>(bytes, 84).value_or(0);
    value.reserved4 = LittleEndian::read<uint32_t>(bytes, 88).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_timeutc_t payload<ubx_payload_rx_nav_timeutc_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_timeutc_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.tAcc = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.nano = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.year = LittleEndian::read<uint16_t>(bytes, 12).value_or(0);
    value.month = LittleEndian::read<uint8_t>(bytes, 14).value_or(0);
    value.day = LittleEndian::read<uint8_t>(bytes, 15).value_or(0);
    value.hour = LittleEndian::read<uint8_t>(bytes, 16).value_or(0);
    value.min = LittleEndian::read<uint8_t>(bytes, 17).value_or(0);
    value.sec = LittleEndian::read<uint8_t>(bytes, 18).value_or(0);
    value.valid = LittleEndian::read<uint8_t>(bytes, 19).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svinfo_part1_t payload<ubx_payload_rx_nav_svinfo_part1_t>(std::span<const uint8_t> input,
                                                                                    size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svinfo_part1_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.numCh = LittleEndian::read<uint8_t>(bytes, 4).value_or(0);
    value.globalFlags = LittleEndian::read<uint8_t>(bytes, 5).value_or(0);
    value.reserved2 = LittleEndian::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svinfo_part2_t payload<ubx_payload_rx_nav_svinfo_part2_t>(std::span<const uint8_t> input,
                                                                                    size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svinfo_part2_t value{};
    value.chn = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.svid = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 2).value_or(0);
    value.quality = LittleEndian::read<uint8_t>(bytes, 3).value_or(0);
    value.cno = LittleEndian::read<uint8_t>(bytes, 4).value_or(0);
    value.elev = LittleEndian::read<int8_t>(bytes, 5).value_or(0);
    value.azim = LittleEndian::read<int16_t>(bytes, 6).value_or(0);
    value.prRes = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sat_part1_t payload<ubx_payload_rx_nav_sat_part1_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sat_part1_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.version = LittleEndian::read<uint8_t>(bytes, 4).value_or(0);
    value.numSvs = LittleEndian::read<uint8_t>(bytes, 5).value_or(0);
    value.reserved = LittleEndian::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_sat_part2_t payload<ubx_payload_rx_nav_sat_part2_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_sat_part2_t value{};
    value.gnssId = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.svId = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.cno = LittleEndian::read<uint8_t>(bytes, 2).value_or(0);
    value.elev = LittleEndian::read<int8_t>(bytes, 3).value_or(0);
    value.azim = LittleEndian::read<int16_t>(bytes, 4).value_or(0);
    value.prRes = LittleEndian::read<int16_t>(bytes, 6).value_or(0);
    value.flags = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_status_t payload<ubx_payload_rx_nav_status_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_status_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.gpsFix = LittleEndian::read<uint8_t>(bytes, 4).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 5).value_or(0);
    value.fixStat = LittleEndian::read<uint8_t>(bytes, 6).value_or(0);
    value.flags2 = LittleEndian::read<uint8_t>(bytes, 7).value_or(0);
    value.ttff = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    value.msss = LittleEndian::read<uint32_t>(bytes, 12).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_svin_t payload<ubx_payload_rx_nav_svin_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_svin_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved1[i] = LittleEndian::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.dur = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    value.meanX = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.meanY = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.meanZ = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.meanXHP = LittleEndian::read<int8_t>(bytes, 24).value_or(0);
    value.meanYHP = LittleEndian::read<int8_t>(bytes, 25).value_or(0);
    value.meanZHP = LittleEndian::read<int8_t>(bytes, 26).value_or(0);
    value.reserved2 = LittleEndian::read<int8_t>(bytes, 27).value_or(0);
    value.meanAcc = LittleEndian::read<uint32_t>(bytes, 28).value_or(0);
    value.obs = LittleEndian::read<uint32_t>(bytes, 32).value_or(0);
    value.valid = LittleEndian::read<uint8_t>(bytes, 36).value_or(0);
    value.active = LittleEndian::read<uint8_t>(bytes, 37).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved3[i] = LittleEndian::read<uint8_t>(bytes, 38 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_velned_t payload<ubx_payload_rx_nav_velned_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_velned_t value{};
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.velN = LittleEndian::read<int32_t>(bytes, 4).value_or(0);
    value.velE = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.velD = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.speed = LittleEndian::read<uint32_t>(bytes, 16).value_or(0);
    value.gSpeed = LittleEndian::read<uint32_t>(bytes, 20).value_or(0);
    value.heading = LittleEndian::read<int32_t>(bytes, 24).value_or(0);
    value.sAcc = LittleEndian::read<uint32_t>(bytes, 28).value_or(0);
    value.cAcc = LittleEndian::read<uint32_t>(bytes, 32).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_hw_ubx6_t payload<ubx_payload_rx_mon_hw_ubx6_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_hw_ubx6_t value{};
    value.pinSel = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.pinBank = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.pinDir = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    value.pinVal = LittleEndian::read<uint32_t>(bytes, 12).value_or(0);
    value.noisePerMS = LittleEndian::read<uint16_t>(bytes, 16).value_or(0);
    value.agcCnt = LittleEndian::read<uint16_t>(bytes, 18).value_or(0);
    value.aStatus = LittleEndian::read<uint8_t>(bytes, 20).value_or(0);
    value.aPower = LittleEndian::read<uint8_t>(bytes, 21).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 22).value_or(0);
    value.reserved1 = LittleEndian::read<uint8_t>(bytes, 23).value_or(0);
    value.usedMask = LittleEndian::read<uint32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 25; ++i)
        value.VP[i] = LittleEndian::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.jamInd = LittleEndian::read<uint8_t>(bytes, 53).value_or(0);
    value.reserved3 = LittleEndian::read<uint16_t>(bytes, 54).value_or(0);
    value.pinIrq = LittleEndian::read<uint32_t>(bytes, 56).value_or(0);
    value.pullH = LittleEndian::read<uint32_t>(bytes, 60).value_or(0);
    value.pullL = LittleEndian::read<uint32_t>(bytes, 64).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_hw_ubx7_t payload<ubx_payload_rx_mon_hw_ubx7_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_hw_ubx7_t value{};
    value.pinSel = LittleEndian::read<uint32_t>(bytes, 0).value_or(0);
    value.pinBank = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.pinDir = LittleEndian::read<uint32_t>(bytes, 8).value_or(0);
    value.pinVal = LittleEndian::read<uint32_t>(bytes, 12).value_or(0);
    value.noisePerMS = LittleEndian::read<uint16_t>(bytes, 16).value_or(0);
    value.agcCnt = LittleEndian::read<uint16_t>(bytes, 18).value_or(0);
    value.aStatus = LittleEndian::read<uint8_t>(bytes, 20).value_or(0);
    value.aPower = LittleEndian::read<uint8_t>(bytes, 21).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 22).value_or(0);
    value.reserved1 = LittleEndian::read<uint8_t>(bytes, 23).value_or(0);
    value.usedMask = LittleEndian::read<uint32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 17; ++i)
        value.VP[i] = LittleEndian::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.jamInd = LittleEndian::read<uint8_t>(bytes, 45).value_or(0);
    value.reserved3 = LittleEndian::read<uint16_t>(bytes, 46).value_or(0);
    value.pinIrq = LittleEndian::read<uint32_t>(bytes, 48).value_or(0);
    value.pullH = LittleEndian::read<uint32_t>(bytes, 52).value_or(0);
    value.pullL = LittleEndian::read<uint32_t>(bytes, 56).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_sec_sig_t payload<ubx_payload_rx_sec_sig_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_sec_sig_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.reserved0 = LittleEndian::read<uint8_t>(bytes, 2).value_or(0);
    value.jamNumCentFreqs = LittleEndian::read<uint8_t>(bytes, 3).value_or(0);
    value.jamFlags = LittleEndian::read<uint8_t>(bytes, 4).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_ver_part1_t payload<ubx_payload_rx_mon_ver_part1_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_ver_part1_t value{};
    for (size_t i = 0; i < 30; ++i)
        value.swVersion[i] = LittleEndian::read<uint8_t>(bytes, 0 + i * 1).value_or(0);
    for (size_t i = 0; i < 10; ++i)
        value.hwVersion[i] = LittleEndian::read<uint8_t>(bytes, 30 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_ver_part2_t payload<ubx_payload_rx_mon_ver_part2_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_ver_part2_t value{};
    for (size_t i = 0; i < 30; ++i)
        value.extension[i] = LittleEndian::read<uint8_t>(bytes, 0 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_rxm_rtcm_t payload<ubx_payload_rx_rxm_rtcm_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_rxm_rtcm_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.subType = LittleEndian::read<uint16_t>(bytes, 2).value_or(0);
    value.refStationID = LittleEndian::read<uint16_t>(bytes, 4).value_or(0);
    value.msgType = LittleEndian::read<uint16_t>(bytes, 6).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_rxm_cor_t payload<ubx_payload_rx_rxm_cor_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_rxm_cor_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.ebno = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved0[i] = LittleEndian::read<uint8_t>(bytes, 2 + i * 1).value_or(0);
    value.statusInfo = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.msgType = LittleEndian::read<uint16_t>(bytes, 8).value_or(0);
    value.msgSubType = LittleEndian::read<uint16_t>(bytes, 10).value_or(0);
    return value;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_prt_t>> encode(const ubx_payload_tx_cfg_prt_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_prt_t>> bytes{};
    LittleEndian::write(bytes, 0, value.portID);
    LittleEndian::write(bytes, 1, value.reserved0);
    LittleEndian::write(bytes, 2, value.txReady);
    LittleEndian::write(bytes, 4, value.mode);
    LittleEndian::write(bytes, 8, value.baudRate);
    LittleEndian::write(bytes, 12, value.inProtoMask);
    LittleEndian::write(bytes, 14, value.outProtoMask);
    LittleEndian::write(bytes, 16, value.flags);
    LittleEndian::write(bytes, 18, value.reserved5);
    return bytes;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_rate_t>> encode(const ubx_payload_tx_cfg_rate_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_rate_t>> bytes{};
    LittleEndian::write(bytes, 0, value.measRate);
    LittleEndian::write(bytes, 2, value.navRate);
    LittleEndian::write(bytes, 4, value.timeRef);
    return bytes;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_nav5_t>> encode(const ubx_payload_tx_cfg_nav5_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_nav5_t>> bytes{};
    LittleEndian::write(bytes, 0, value.mask);
    LittleEndian::write(bytes, 2, value.dynModel);
    LittleEndian::write(bytes, 3, value.fixMode);
    LittleEndian::write(bytes, 4, value.fixedAlt);
    LittleEndian::write(bytes, 8, value.fixedAltVar);
    LittleEndian::write(bytes, 12, value.minElev);
    LittleEndian::write(bytes, 13, value.drLimit);
    LittleEndian::write(bytes, 14, value.pDop);
    LittleEndian::write(bytes, 16, value.tDop);
    LittleEndian::write(bytes, 18, value.pAcc);
    LittleEndian::write(bytes, 20, value.tAcc);
    LittleEndian::write(bytes, 22, value.staticHoldThresh);
    LittleEndian::write(bytes, 23, value.dgpsTimeOut);
    LittleEndian::write(bytes, 24, value.cnoThreshNumSVs);
    LittleEndian::write(bytes, 25, value.cnoThresh);
    LittleEndian::write(bytes, 26, value.reserved);
    LittleEndian::write(bytes, 28, value.staticHoldMaxDist);
    LittleEndian::write(bytes, 30, value.utcStandard);
    LittleEndian::write(bytes, 31, value.reserved3);
    LittleEndian::write(bytes, 32, value.reserved4);
    return bytes;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_tmode3_t>> encode(const ubx_payload_tx_cfg_tmode3_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_tmode3_t>> bytes{};
    LittleEndian::write(bytes, 0, value.version);
    LittleEndian::write(bytes, 1, value.reserved1);
    LittleEndian::write(bytes, 2, value.flags);
    LittleEndian::write(bytes, 4, value.ecefXOrLat);
    LittleEndian::write(bytes, 8, value.ecefYOrLon);
    LittleEndian::write(bytes, 12, value.ecefZOrAlt);
    LittleEndian::write(bytes, 16, value.ecefXOrLatHP);
    LittleEndian::write(bytes, 17, value.ecefYOrLonHP);
    LittleEndian::write(bytes, 18, value.ecefZOrAltHP);
    LittleEndian::write(bytes, 19, value.reserved2);
    LittleEndian::write(bytes, 20, value.fixedPosAcc);
    LittleEndian::write(bytes, 24, value.svinMinDur);
    LittleEndian::write(bytes, 28, value.svinAccLimit);
    for (size_t i = 0; i < 8; ++i) {
        LittleEndian::write(bytes, 32 + i * 1, value.reserved3[i]);
    }
    return bytes;
}

template <>
inline ubx_payload_rx_nav_relposned_t payload<ubx_payload_rx_nav_relposned_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_relposned_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.reserved0 = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.refStationId = LittleEndian::read<uint16_t>(bytes, 2).value_or(0);
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.relPosN = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.relPosE = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.relPosD = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.relPosLength = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.relPosHeading = LittleEndian::read<int32_t>(bytes, 24).value_or(0);
    value.reserved1 = LittleEndian::read<uint32_t>(bytes, 28).value_or(0);
    value.relPosHPN = LittleEndian::read<int8_t>(bytes, 32).value_or(0);
    value.relPosHPE = LittleEndian::read<int8_t>(bytes, 33).value_or(0);
    value.relPosHPD = LittleEndian::read<int8_t>(bytes, 34).value_or(0);
    value.relPosHPLength = LittleEndian::read<int8_t>(bytes, 35).value_or(0);
    value.accN = LittleEndian::read<uint32_t>(bytes, 36).value_or(0);
    value.accE = LittleEndian::read<uint32_t>(bytes, 40).value_or(0);
    value.accD = LittleEndian::read<uint32_t>(bytes, 44).value_or(0);
    value.accLength = LittleEndian::read<uint32_t>(bytes, 48).value_or(0);
    value.accHeading = LittleEndian::read<uint32_t>(bytes, 52).value_or(0);
    value.reserved2 = LittleEndian::read<uint32_t>(bytes, 56).value_or(0);
    value.flags = LittleEndian::read<uint32_t>(bytes, 60).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_daheading_t payload<ubx_payload_rx_nav_daheading_t>(std::span<const uint8_t> input,
                                                                              size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_daheading_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved0[i] = LittleEndian::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.relPosN = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.relPosE = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.relPosD = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.relPosLength = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.relPosHeading = LittleEndian::read<int32_t>(bytes, 24).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved1[i] = LittleEndian::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.accN = LittleEndian::read<uint32_t>(bytes, 32).value_or(0);
    value.accE = LittleEndian::read<uint32_t>(bytes, 36).value_or(0);
    value.accD = LittleEndian::read<uint32_t>(bytes, 40).value_or(0);
    value.accLength = LittleEndian::read<uint32_t>(bytes, 44).value_or(0);
    value.accHeading = LittleEndian::read<uint32_t>(bytes, 48).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved2[i] = LittleEndian::read<uint8_t>(bytes, 52 + i * 1).value_or(0);
    value.flags = LittleEndian::read<uint32_t>(bytes, 56).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_nav_hpposllh_t payload<ubx_payload_rx_nav_hpposllh_t>(std::span<const uint8_t> input,
                                                                            size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_nav_hpposllh_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved1[i] = LittleEndian::read<uint8_t>(bytes, 1 + i * 1).value_or(0);
    value.flags = LittleEndian::read<int8_t>(bytes, 3).value_or(0);
    value.iTOW = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.lon = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.lat = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.height = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.hMSL = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.lonHp = LittleEndian::read<int8_t>(bytes, 24).value_or(0);
    value.latHp = LittleEndian::read<int8_t>(bytes, 25).value_or(0);
    value.heightHp = LittleEndian::read<int8_t>(bytes, 26).value_or(0);
    value.hMSLHp = LittleEndian::read<int8_t>(bytes, 27).value_or(0);
    value.hAcc = LittleEndian::read<uint32_t>(bytes, 28).value_or(0);
    value.vAcc = LittleEndian::read<uint32_t>(bytes, 32).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_comms_port_t payload<ubx_payload_rx_mon_comms_port_t>(std::span<const uint8_t> input,
                                                                                size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_comms_port_t value{};
    value.portId = LittleEndian::read<uint16_t>(bytes, 0).value_or(0);
    value.txPending = LittleEndian::read<uint16_t>(bytes, 2).value_or(0);
    value.txBytes = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    value.txUsage = LittleEndian::read<uint8_t>(bytes, 8).value_or(0);
    value.txPeakUsage = LittleEndian::read<uint8_t>(bytes, 9).value_or(0);
    value.rxPending = LittleEndian::read<uint16_t>(bytes, 10).value_or(0);
    value.rxBytes = LittleEndian::read<uint32_t>(bytes, 12).value_or(0);
    value.rxUsage = LittleEndian::read<uint8_t>(bytes, 16).value_or(0);
    value.rxPeakUsage = LittleEndian::read<uint8_t>(bytes, 17).value_or(0);
    value.overrunErrs = LittleEndian::read<uint16_t>(bytes, 18).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.msgs[i] = LittleEndian::read<uint16_t>(bytes, 20 + i * 2).value_or(0);
    for (size_t i = 0; i < 8; ++i)
        value.reserved[i] = LittleEndian::read<uint8_t>(bytes, 28 + i * 1).value_or(0);
    value.skipped = LittleEndian::read<uint32_t>(bytes, 36).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_comms_t payload<ubx_payload_rx_mon_comms_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_comms_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.nPorts = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.txErrors = LittleEndian::read<uint8_t>(bytes, 2).value_or(0);
    value.reserved = LittleEndian::read<uint8_t>(bytes, 3).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.protIds[i] = LittleEndian::read<uint8_t>(bytes, 4 + i * 1).value_or(0);
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
    value.blockId = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.flags = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    value.antStatus = LittleEndian::read<uint8_t>(bytes, 2).value_or(0);
    value.antPower = LittleEndian::read<uint8_t>(bytes, 3).value_or(0);
    value.postStatus = LittleEndian::read<uint32_t>(bytes, 4).value_or(0);
    for (size_t i = 0; i < 4; ++i)
        value.reserved2[i] = LittleEndian::read<uint8_t>(bytes, 8 + i * 1).value_or(0);
    value.noisePerMS = LittleEndian::read<uint16_t>(bytes, 12).value_or(0);
    value.agcCnt = LittleEndian::read<uint16_t>(bytes, 14).value_or(0);
    value.jamInd = LittleEndian::read<uint8_t>(bytes, 16).value_or(0);
    value.ofsI = LittleEndian::read<int8_t>(bytes, 17).value_or(0);
    value.magI = LittleEndian::read<uint8_t>(bytes, 18).value_or(0);
    value.ofsQ = LittleEndian::read<int8_t>(bytes, 19).value_or(0);
    value.magQ = LittleEndian::read<uint8_t>(bytes, 20).value_or(0);
    for (size_t i = 0; i < 3; ++i)
        value.reserved3[i] = LittleEndian::read<uint8_t>(bytes, 21 + i * 1).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_mon_rf_t payload<ubx_payload_rx_mon_rf_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_mon_rf_t value{};
    value.version = LittleEndian::read<uint8_t>(bytes, 0).value_or(0);
    value.nBlocks = LittleEndian::read<uint8_t>(bytes, 1).value_or(0);
    for (size_t i = 0; i < 2; ++i)
        value.reserved1[i] = LittleEndian::read<uint8_t>(bytes, 2 + i * 1).value_or(0);
    for (size_t i = 0; i < 1; ++i)
        value.block[i] = payload<ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t>(bytes, 4 + i * 24);
    return value;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t>> encode(
    const ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_gnss_t::ubx_payload_tx_cgf_gnss_block_t>> bytes{};
    LittleEndian::write(bytes, 0, value.gnssId);
    LittleEndian::write(bytes, 1, value.resTrkCh);
    LittleEndian::write(bytes, 2, value.maxTrkCh);
    LittleEndian::write(bytes, 3, value.reserved1);
    LittleEndian::write(bytes, 4, value.flags);
    return bytes;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_gnss_t>> encode(const ubx_payload_tx_cfg_gnss_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_gnss_t>> bytes{};
    LittleEndian::write(bytes, 0, value.msgVer);
    LittleEndian::write(bytes, 1, value.numTrkChHw);
    LittleEndian::write(bytes, 2, value.numTrkChUse);
    LittleEndian::write(bytes, 3, value.numConfigBlocks);
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
    value.msg = LittleEndian::read<uint16_t>(bytes, 0).value_or(0);
    return value;
}

template <>
inline ubx_payload_rx_ack_nak_t payload<ubx_payload_rx_ack_nak_t>(std::span<const uint8_t> input, size_t offset)
{
    const auto bytes = offset <= input.size() ? input.subspan(offset) : std::span<const uint8_t>{};
    ubx_payload_rx_ack_nak_t value{};
    value.msg = LittleEndian::read<uint16_t>(bytes, 0).value_or(0);
    return value;
}

template <>
inline std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_msg_t>> encode(const ubx_payload_tx_cfg_msg_t& value)
{
    std::array<uint8_t, UBX::WIRE_SIZE<ubx_payload_tx_cfg_msg_t>> bytes{};
    LittleEndian::write(bytes, 0, value.msg);
    LittleEndian::write(bytes, 2, value.rate);
    return bytes;
}

template <typename T, size_t N>
std::array<uint8_t, WIRE_SIZE<T> * N> encode(const T (&values)[N])
{
    std::array<uint8_t, WIRE_SIZE<T> * N> bytes{};
    for (size_t i = 0; i < N; ++i) {
        const auto block = encode(values[i]);
        std::copy(block.begin(), block.end(), bytes.begin() + i * WIRE_SIZE<T>);
    }
    return bytes;
}
}  // namespace UBX
