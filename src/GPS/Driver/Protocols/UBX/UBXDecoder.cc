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

#include <cmath>
#include <string.h>
#include <string_view>

#include "NMEASentence.h"
#include "RTCMFramer.h"
#include "UBX/GPSDriverUBX.h"
#include "UBXMessageCodec.h"

namespace {
GPSPositionReport::FixType navigationFix(uint8_t wireFix, uint8_t flags, bool hasCarrierFlags)
{
    using Fix = GPSPositionReport::FixType;
    if (!(flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK)) {
        return Fix::NoFix;
    }
    switch (wireFix) {
        case 0:
        case 5:
            return Fix::NoFix;
        case 1:
            return Fix::Extrapolated;
        case 2:
            // Correction flags cannot establish a three-dimensional navigation solution.
            return Fix::Fix2D;
        case 3:
        case 4:
            break;
        default:
            return Fix::Unknown;
    }
    if (hasCarrierFlags) {
        const uint8_t carrier = flags >> 6;
        if (carrier == 1) {
            return Fix::RTKFloat;
        }
        if (carrier == 2) {
            return Fix::RTKFixed;
        }
    }
    return flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN ? Fix::Differential : Fix::Fix3D;
}

bool velocityValid(GPSPositionReport::FixType fix)
{
    return fix != GPSPositionReport::FixType::Unknown && fix != GPSPositionReport::FixType::NoFix;
}

/// Fixed-size satellite blocks follow the header; a truncated block ends the report at the preceding entry.
template <typename Header, typename Block, typename Count, typename Assign>
void decodeSatelliteBlocks(std::span<const uint8_t> payload, GPSNativeSatelliteReport& report, Count count,
                           Assign assign)
{
    const auto header = UBX::MessageCodec<Header>::block(payload);
    if (!header) {
        return;
    }
    const auto satelliteCount = std::min<size_t>(count(*header), GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES);
    if (satelliteCount == 0) {
        (void) report.ensureConstellation(GPSConstellation::Unknown);
    }
    for (size_t index = 0; index < satelliteCount; ++index) {
        const auto block =
            UBX::MessageCodec<Block>::block(payload, UBX::WIRE_SIZE<Header> + index * UBX::WIRE_SIZE<Block>);
        if (!block) {
            return;
        }
        assign(report, *block);
    }
}
}  // namespace

int GPSNativeUBX::parseChar(uint8_t byte)
{
    if (_rtcm_parsing && _frameDecoder.idle() && _rtcm_parsing->ownsByte(byte)) {
        _rtcm_parsing->addByte(byte);
        drainRTCM(*_rtcm_parsing);
        return 0;
    }
    // Keep the completed frame local: synchronous logging can reenter or reset the parser.
    const auto frame = _frameDecoder.consume(byte);
    if (!frame) {
        return 0;
    }
    if (_rtcm_parsing) {
        _rtcm_parsing->reset();
    }
    return decodeValidatedPayload(frame->message, std::span(frame->payload).first(frame->length));
}

bool GPSNativeUBX::payloadRxInit(uint16_t message, std::span<const uint8_t> payload)
{
    auto state = UBX_RXMSG_HANDLE;

    switch (message) {
        case UBX_MSG_CFG_TMODE3:
            if (!_timeModeReadback.pending) {
                state = UBX_RXMSG_IGNORE;
            }
            break;
        case UBX_MSG_CFG_VALGET:
            if (!_controller.readbackPending()) {
                state = UBX_RXMSG_IGNORE;
            } else if (payload.size() < 4 || payload.size() > UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                state = UBX_RXMSG_ERROR_LENGTH;
            }
            break;
        case UBX_MSG_MON_COMMS:
            break;

        case UBX_MSG_NAV_PVT:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation

            } else if (!_decodeContext.useNavPvt) {
                state = UBX_RXMSG_DISABLE;  // disable if not using NAV-PVT
            }

            break;

        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_NOTICE:
        case UBX_MSG_INF_WARNING:
            if (payload.size() >= UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                state = UBX_RXMSG_ERROR_LENGTH;
            }

            break;

        case UBX_MSG_NAV_POSLLH:
        case UBX_MSG_NAV_SOL:
        case UBX_MSG_NAV_TIMEUTC:
        case UBX_MSG_NAV_VELNED:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation

            } else if (_decodeContext.useNavPvt) {
                state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_NAV_STATUS:
        case UBX_MSG_NAV_DOP:
        case UBX_MSG_NAV_RELPOSNED:
        case UBX_MSG_NAV_DAHEADING:
        case UBX_MSG_NAV_HPPOSLLH:
        case UBX_MSG_MON_HW:
        case UBX_MSG_MON_RF:
        case UBX_MSG_SEC_SIG:
        case UBX_MSG_RXM_RTCM:
        case UBX_MSG_RXM_COR:
            if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation
            }

            break;

        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:
            if (_satellites == nullptr) {
                state = UBX_RXMSG_DISABLE;  // disable if sat info not requested

            } else if (!_decodeContext.navigation) {
                state = UBX_RXMSG_IGNORE;  // ignore if not _decodeContext.navigation
            }

            break;

        case UBX_MSG_NAV_SVIN:
            break;

        case UBX_MSG_MON_VER:
            break;  // unconditionally handle this message

        case UBX_MSG_ACK_ACK:
            if (!_controller.awaitingAcknowledgement()) {
                state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        case UBX_MSG_ACK_NAK:
            if (!_controller.awaitingAcknowledgement() && !_controller.readbackPending() &&
                !_timeModeReadback.pending) {
                state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        default:
            state = UBX_RXMSG_DISABLE;  // disable all other messages
            break;
    }

    if (state == UBX_RXMSG_DISABLE) {
        _pendingDisableMessage = message;
    }
    return state == UBX_RXMSG_HANDLE;
}

void GPSNativeUBX::decodeNavSat(std::span<const uint8_t> payload)
{
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS, GPSConstellation::NavIC};
    decodeSatelliteBlocks<ubx_payload_rx_nav_sat_part1_t, ubx_payload_rx_nav_sat_part2_t>(
        payload, *_satellites, [](const auto& header) { return header.numSvs; },
        [&systems](auto& report, const auto& wire) {
            const auto constellation =
                wire.gnssId < std::size(systems) ? systems[wire.gnssId] : GPSConstellation::Unknown;
            auto* system = report.ensureConstellation(constellation);
            if (!system) {
                return;
            }
            ++system->inView;
            system->inUse = system->inUse.value_or(0) + ((wire.flags & 8) != 0 ? 1 : 0);
        });
}

void GPSNativeUBX::decodeNavSvinfo(std::span<const uint8_t> payload)
{
    decodeSatelliteBlocks<ubx_payload_rx_nav_svinfo_part1_t, ubx_payload_rx_nav_svinfo_part2_t>(
        payload, *_satellites, [](const auto& header) { return header.numCh; },
        [](auto& report, const auto& wire) {
            auto* system = report.ensureConstellation(GPSConstellation::Unknown);
            if (!system) {
                return;
            }
            ++system->inView;
            system->inUse = system->inUse.value_or(0) + ((wire.flags & 1) != 0 ? 1 : 0);
        });
}

void GPSNativeUBX::decodeMonVer(std::span<const uint8_t> payload)
{
    _identity = {};
    auto decoded_payload_rx_mon_ver_part1 = UBX::MessageCodec<ubx_payload_rx_mon_ver_part1_t>::block(payload);
    if (!decoded_payload_rx_mon_ver_part1) {
        return;
    }
    auto payload_rx_mon_ver_part1 = *decoded_payload_rx_mon_ver_part1;
    // The protocol specifies these as nul-terminated strings, but the terminator comes
    // from the device, so enforce it before anything walks the field.
    payload_rx_mon_ver_part1.swVersion[sizeof(payload_rx_mon_ver_part1.swVersion) - 1] = 0;
    payload_rx_mon_ver_part1.hwVersion[sizeof(payload_rx_mon_ver_part1.hwVersion) - 1] = 0;
    memcpy(_identity.firmwareVersion, payload_rx_mon_ver_part1.swVersion, sizeof(_identity.firmwareVersion));

    // Device detection (See
    // https://forum.u-blox.com/index.php/9432/need-help-decoding-ubx-mon-ver-hardware-string)
    static constexpr struct
    {
        char hw_version[9];
        Board board;
    } known_boards[] = {
        {"00040005", Board::u_blox5},    {"00040007", Board::u_blox6}, {"00070000", Board::u_blox7},
        {"00080000", Board::u_blox8},    {"00190000", Board::u_blox9}, {"000A0000", Board::u_blox10},
        {"000B0000", Board::u_blox_X20},
    };

    bool known = false;

    for (const auto& known_board : known_boards) {
        if (strncmp((const char*) payload_rx_mon_ver_part1.hwVersion, known_board.hw_version,
                    sizeof(payload_rx_mon_ver_part1.hwVersion)) == 0) {
            _identity.board = known_board.board;
            known = true;
            break;
        }
    }

    if (!known) {
        log(GPSProtocolLogLevel::Warning, "unknown board hw: %s", payload_rx_mon_ver_part1.hwVersion);
    }
    _identity.protocol27 =
        _identity.board == Board::u_blox9 || _identity.board == Board::u_blox10 || _identity.board == Board::u_blox_X20;
    _identity.timeModeUnsupported = _identity.board == Board::u_blox5 || _identity.board == Board::u_blox6 ||
                                    _identity.board == Board::u_blox7 || _identity.board == Board::u_blox10;
    for (size_t offset = UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part1_t>; offset < payload.size();
         offset += UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part2_t>) {
        auto decoded_payload_rx_mon_ver_part2 =
            UBX::MessageCodec<ubx_payload_rx_mon_ver_part2_t>::block(payload, offset);
        if (!decoded_payload_rx_mon_ver_part2) {
            return;
        }
        auto payload_rx_mon_ver_part2 = *decoded_payload_rx_mon_ver_part2;
        // Part 2 complete: decode Part 2 buffer
        // Same as above: the protocol specifies a nul-terminated string, the device provides
        // the terminator, so enforce it before strstr() walks the field.
        payload_rx_mon_ver_part2.extension[sizeof(payload_rx_mon_ver_part2.extension) - 1] = 0;

        // "FWVER=" Firmware of product category and version
        const char* fwver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "FWVER=");

        if (fwver_str != nullptr) {
            if (strncmp(fwver_str, "FWVER=SPG ", 10) == 0 || strncmp(fwver_str, "FWVER=HPS ", 10) == 0) {
                _identity.timeModeUnsupported = true;
            }
            strncpy(_identity.firmwareVersion, fwver_str + strlen("FWVER="), sizeof(_identity.firmwareVersion) - 1);
            _identity.firmwareVersion[sizeof(_identity.firmwareVersion) - 1] = '\0';
            log(GPSProtocolLogLevel::Debug, "u-blox firmware version: %s", fwver_str + strlen("FWVER="));

            // Check if its a ZED-F9P-15B
            if ((_identity.board == Board::u_blox9) && strstr(fwver_str, "HPGL1L5")) {
                _identity.board = Board::u_blox9_F9P_L1L5;
            }
        }

        // "PROTVER=" Supported protocol version.
        const char* protver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "PROTVER=");

        if (protver_str != nullptr) {
            log(GPSProtocolLogLevel::Debug, "u-blox protocol version: %s", protver_str + strlen("PROTVER="));
            const std::string_view version(protver_str + strlen("PROTVER="));
            const auto major = NMEA::number<unsigned>(version.substr(0, version.find('.')));
            if (!major || *major == 0) {
                _identity.board = Board::unknown;
                return;
            }
            _identity.protocol27 = *major >= 27;
            if (*major < 20) {
                _identity.timeModeUnsupported = true;
            }
        }

        // "MOD=" Module identification. Set in production.
        const char* mod_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "MOD=");

        if (mod_str != nullptr) {
            if (strncmp(mod_str, "MOD=NEO-M8P-0", 13) == 0 || strcmp(mod_str, "MOD=NEO-M8N") == 0 ||
                strcmp(mod_str, "MOD=NEO-M9N") == 0) {
                _identity.timeModeUnsupported = true;
            }
            strncpy(_identity.modelName, mod_str + strlen("MOD="), sizeof(_identity.modelName) - 1);
            _identity.modelName[sizeof(_identity.modelName) - 1] = '\0';
            _identity.isM8p = strstr(mod_str, "M8P") != nullptr;
            // in case of u-blox9 family, check if it's an F9P
            if (_identity.board == Board::u_blox9) {
                if (strstr(mod_str, "F9P")) {
                    _identity.board = Board::u_blox9_F9P_L1L2;
                }

            } else if (_identity.board == Board::u_blox10) {
                if (strstr(mod_str, "DAN-F10N")) {
                    _identity.board = Board::u_blox10_L1L5;
                }
            }

            log(GPSProtocolLogLevel::Debug, "u-blox module: %s", mod_str + strlen("MOD="));
        }
    }
}

int  // 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSNativeUBX::payloadRxDone(uint16_t message, std::span<const uint8_t> payload, GPSNativePositionReport& position)
{
    int ret = 0;

    // handle message
    switch (message) {
        case UBX_MSG_NAV_PVT: {
            const auto decoded_payload_rx_nav_pvt = UBX::MessageCodec<ubx_payload_rx_nav_pvt_t>::decode(payload);
            if (!decoded_payload_rx_nav_pvt) {
                break;
            }
            const auto& payload_rx_nav_pvt = *decoded_payload_rx_nav_pvt;
            position.navigation.fixType = navigationFix(payload_rx_nav_pvt.fixType, payload_rx_nav_pvt.flags, true);
            position.velocityValid = velocityValid(position.navigation.fixType);

            position.navigation.satellitesUsed = payload_rx_nav_pvt.numSV;

            if (_decodeContext.assembleEpochs ? !_epochHasHighPrecision
                                              : position.navigation.fixType != GPSPositionReport::FixType::RTKFixed) {
                // When RTK is active and solid (fix=6), these values will be filled by HPPOSLLH:
                position.navigation.latitudeDegrees = payload_rx_nav_pvt.lat * UBX::DEGREES_PER_COORDINATE;
                position.navigation.longitudeDegrees = payload_rx_nav_pvt.lon * UBX::DEGREES_PER_COORDINATE;
                position.navigation.altitudeMslMeters = payload_rx_nav_pvt.hMSL * 1e-3;
                position.navigation.altitudeEllipsoidMeters = payload_rx_nav_pvt.height * 1e-3;

                position.navigation.horizontalAccuracyMeters = static_cast<float>(payload_rx_nav_pvt.hAcc) * 1e-3f;
                position.navigation.verticalAccuracyMeters = static_cast<float>(payload_rx_nav_pvt.vAcc) * 1e-3f;

                _got_posllh = true;
            }

            position.navigation.speedMetersPerSecond = static_cast<float>(payload_rx_nav_pvt.gSpeed) * 1e-3f;

            position.navigation.courseRadians = static_cast<float>(payload_rx_nav_pvt.headMot) * GPS_DEG_TO_RAD * 1e-5f;

            // Check if time and date fix flags are good
            if ((payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDDATE) &&
                (payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDTIME) &&
                (payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_FULLYRESOLVED)) {
                tm timeinfo{};
                timeinfo.tm_year = payload_rx_nav_pvt.year - 1900;
                timeinfo.tm_mon = payload_rx_nav_pvt.month - 1;
                timeinfo.tm_mday = payload_rx_nav_pvt.day;
                timeinfo.tm_hour = payload_rx_nav_pvt.hour;
                timeinfo.tm_min = payload_rx_nav_pvt.min;
                timeinfo.tm_sec = payload_rx_nav_pvt.sec;
                position.navigation.utcTimeUs = timeFromUtc(timeinfo, payload_rx_nav_pvt.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                position.navigation.utcTimeUs = 0;
            }

            position.navigation.timestampUs = nowUs();

            _got_velned = true;

            ret = 1;
            break;
        }
        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_NOTICE:
            break;

        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_WARNING: {
            const std::string_view text(reinterpret_cast<const char*>(payload.data()), payload.size());
            log(GPSProtocolLogLevel::Warning, "ubx msg: %.*s", int(text.size()), text.data());

            if (text.starts_with("txbuf")) {
                _comms.pending = true;
            }
        } break;

        case UBX_MSG_MON_COMMS:
            logCommsDiagnostics(payload);
            break;
        case UBX_MSG_CFG_TMODE3:
            if (_timeModeReadback.pending && payload[0] == 0 && payload[1] == 0) {
                _timeModeReadback.response = payload[2];
            }
            break;
        case UBX_MSG_CFG_VALGET:
            if (const auto values = UBX::decodeConfigurationValues(payload)) {
                _controller.accept(*values);
            }
            break;

        case UBX_MSG_NAV_POSLLH: {
            const auto decoded_payload_rx_nav_posllh = UBX::MessageCodec<ubx_payload_rx_nav_posllh_t>::decode(payload);
            if (!decoded_payload_rx_nav_posllh) {
                break;
            }
            const auto& payload_rx_nav_posllh = *decoded_payload_rx_nav_posllh;

            position.navigation.latitudeDegrees = payload_rx_nav_posllh.lat * UBX::DEGREES_PER_COORDINATE;
            position.navigation.longitudeDegrees = payload_rx_nav_posllh.lon * UBX::DEGREES_PER_COORDINATE;
            position.navigation.altitudeMslMeters = payload_rx_nav_posllh.hMSL * 1e-3;
            position.navigation.altitudeEllipsoidMeters = payload_rx_nav_posllh.height * 1e-3;
            position.navigation.horizontalAccuracyMeters =
                static_cast<float>(payload_rx_nav_posllh.hAcc) * 1e-3f;  // from mm to m
            position.navigation.verticalAccuracyMeters =
                static_cast<float>(payload_rx_nav_posllh.vAcc) * 1e-3f;  // from mm to m

            position.navigation.timestampUs = nowUs();

            _got_posllh = true;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_HPPOSLLH: {
            const auto decoded_payload_rx_nav_hpposllh =
                UBX::MessageCodec<ubx_payload_rx_nav_hpposllh_t>::decode(payload);
            if (!decoded_payload_rx_nav_hpposllh) {
                break;
            }
            const auto& payload_rx_nav_hpposllh = *decoded_payload_rx_nav_hpposllh;

            if (payload_rx_nav_hpposllh.flags == 0 &&
                (_decodeContext.assembleEpochs ||
                 position.navigation.fixType == GPSPositionReport::FixType::RTKFixed)) {
                position.navigation.latitudeDegrees =
                    payload_rx_nav_hpposllh.lat * UBX::DEGREES_PER_COORDINATE +
                    payload_rx_nav_hpposllh.latHp * 1e-9;  // regular precision lat/lon (1e7), plus high precision (1e9)
                position.navigation.longitudeDegrees =
                    payload_rx_nav_hpposllh.lon * UBX::DEGREES_PER_COORDINATE + payload_rx_nav_hpposllh.lonHp * 1e-9;
                position.navigation.altitudeMslMeters =
                    payload_rx_nav_hpposllh.hMSL * 1e-3 +
                    payload_rx_nav_hpposllh.hMSLHp *
                        1e-4;  // regular precision altitude, mm, plus high precision components of altitude, 0.1 mm
                position.navigation.altitudeEllipsoidMeters =
                    payload_rx_nav_hpposllh.height * 1e-3 + payload_rx_nav_hpposllh.heightHp * 1e-4;
                position.navigation.horizontalAccuracyMeters = static_cast<float>(payload_rx_nav_hpposllh.hAcc) *
                                                               1e-4f;  // Accuracy estimates, convert from 0.1 mm to m
                position.navigation.verticalAccuracyMeters = static_cast<float>(payload_rx_nav_hpposllh.vAcc) * 1e-4f;

                position.navigation.timestampUs = nowUs();

                _got_posllh = true;

                ret = 1;
            }

            break;
        }
        case UBX_MSG_NAV_SOL: {
            const auto decoded_payload_rx_nav_sol = UBX::MessageCodec<ubx_payload_rx_nav_sol_t>::decode(payload);
            if (!decoded_payload_rx_nav_sol) {
                break;
            }
            const auto& payload_rx_nav_sol = *decoded_payload_rx_nav_sol;

            position.navigation.fixType = navigationFix(payload_rx_nav_sol.gpsFix, payload_rx_nav_sol.flags, false);
            position.velocityValid = velocityValid(position.navigation.fixType);
            position.navigation.satellitesUsed = payload_rx_nav_sol.numSV;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_STATUS: {
            const auto decoded_payload_rx_nav_status = UBX::MessageCodec<ubx_payload_rx_nav_status_t>::decode(payload);
            if (!decoded_payload_rx_nav_status) {
                break;
            }
            const auto& payload_rx_nav_status = *decoded_payload_rx_nav_status;

            _integrity.spoofing.state = GPSIntegrityReport::spoofingStateFromValue(
                (payload_rx_nav_status.flags2 & UBX_RX_NAV_STATUS_SPOOFDETSTATE_MASK) >>
                UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT);
            _integrity.spoofing.timestampUs = nowUs();

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_DOP: {
            const auto decoded_payload_rx_nav_dop = UBX::MessageCodec<ubx_payload_rx_nav_dop_t>::decode(payload);
            if (!decoded_payload_rx_nav_dop) {
                break;
            }
            const auto& payload_rx_nav_dop = *decoded_payload_rx_nav_dop;
            position.navigation.horizontalDop = payload_rx_nav_dop.hDOP * UBX::DOP_PER_UNIT;
            position.navigation.horizontalDop = payload_rx_nav_dop.hDOP * UBX::DOP_PER_UNIT;
            position.navigation.verticalDop = payload_rx_nav_dop.vDOP * UBX::DOP_PER_UNIT;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_TIMEUTC: {
            const auto decoded_payload_rx_nav_timeutc =
                UBX::MessageCodec<ubx_payload_rx_nav_timeutc_t>::decode(payload);
            if (!decoded_payload_rx_nav_timeutc) {
                break;
            }
            const auto& payload_rx_nav_timeutc = *decoded_payload_rx_nav_timeutc;

            if (payload_rx_nav_timeutc.valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) {
                tm timeinfo{};
                timeinfo.tm_year = payload_rx_nav_timeutc.year - 1900;
                timeinfo.tm_mon = payload_rx_nav_timeutc.month - 1;
                timeinfo.tm_mday = payload_rx_nav_timeutc.day;
                timeinfo.tm_hour = payload_rx_nav_timeutc.hour;
                timeinfo.tm_min = payload_rx_nav_timeutc.min;
                timeinfo.tm_sec = payload_rx_nav_timeutc.sec;
                position.navigation.utcTimeUs = timeFromUtc(timeinfo, payload_rx_nav_timeutc.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                position.navigation.utcTimeUs = 0;
            }

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:

            // Satellite entries were decoded from the validated payload.
            for (uint8_t index = 0; index < _satellites->count; ++index) {
                auto& system = _satellites->constellations[index];
                system.inViewTimestampUs = nowUs();
                if (system.inUse) {
                    system.inUseTimestampUs = system.inViewTimestampUs;
                }
            }

            ret = 2;
            break;

        case UBX_MSG_NAV_SVIN: {
            const auto decoded_payload_rx_nav_svin = UBX::MessageCodec<ubx_payload_rx_nav_svin_t>::decode(payload);
            if (!decoded_payload_rx_nav_svin) {
                break;
            }
            const auto& payload_rx_nav_svin = *decoded_payload_rx_nav_svin;

            {
                const ubx_payload_rx_nav_svin_t& svin = payload_rx_nav_svin;
                _survey_in_stopped = svin.active == 0 && svin.valid == 0;

                if (!_decodeContext.navigation) {
                    ret = 1;
                    break;
                }

                GPSNativeSurveyReport status{};
                const auto surveyPosition = fromEcef({
                    .x = (static_cast<double>(svin.meanX) + static_cast<double>(svin.meanXHP) * 0.01) * 0.01,
                    .y = (static_cast<double>(svin.meanY) + static_cast<double>(svin.meanYHP) * 0.01) * 0.01,
                    .z = (static_cast<double>(svin.meanZ) + static_cast<double>(svin.meanZHP) * 0.01) * 0.01,
                });
                status.survey.position = surveyPosition;
                status.survey.duration = std::chrono::seconds(svin.dur);
                status.survey.meanAccuracyMeters = static_cast<double>(svin.meanAcc / 10) / 1000.0;
                status.survey.valid = (svin.valid & 1) != 0;
                status.survey.active = (svin.active & 1) != 0;
                surveyInStatus(status);

                if (svin.valid == 1 && svin.active == 0) {
                    _rtcmActivationPending = true;
                }
            }

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_VELNED: {
            const auto decoded_payload_rx_nav_velned = UBX::MessageCodec<ubx_payload_rx_nav_velned_t>::decode(payload);
            if (!decoded_payload_rx_nav_velned) {
                break;
            }
            const auto& payload_rx_nav_velned = *decoded_payload_rx_nav_velned;

            position.navigation.speedMetersPerSecond = static_cast<float>(payload_rx_nav_velned.gSpeed) * 1e-2f;
            position.navigation.courseRadians =
                static_cast<float>(payload_rx_nav_velned.heading) * GPS_DEG_TO_RAD * 1e-5f;
            position.velocityValid = velocityValid(position.navigation.fixType);

            _got_velned = true;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_RELPOSNED: {
            const auto decoded_payload_rx_nav_relposned =
                UBX::MessageCodec<ubx_payload_rx_nav_relposned_t>::decode(payload);
            if (!decoded_payload_rx_nav_relposned) {
                break;
            }
            const auto& payload_rx_nav_relposned = *decoded_payload_rx_nav_relposned;

            {
                const float rel_length_cm =
                    payload_rx_nav_relposned.relPosLength + payload_rx_nav_relposned.relPosHPLength * 1e-2f;
                const float rel_length_m = rel_length_cm * 1e-2f;  // cm -> m
                const uint32_t flags = payload_rx_nav_relposned.flags;
                const bool heading_valid_flag = flags & (1 << 8);
                const bool rel_pos_valid = flags & (1 << 2);
                const bool carrier_solution_fixed = flags & (1 << 4);

                const bool heading_qualified = heading_valid_flag && rel_pos_valid &&
                                               (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

                float heading_rad = NAN;
                float heading_acc_rad = NAN;

                if (heading_qualified) {
                    heading_rad = relPosHeadingToYaw(payload_rx_nav_relposned.relPosHeading);
                    heading_acc_rad = payload_rx_nav_relposned.accHeading * GPS_DEG_TO_RAD * 1e-5f;
                }

                position.navigation.headingRadians = heading_rad;
                position.navigation.headingAccuracyRadians = heading_acc_rad;

                ret = 1;
            }

            break;
        }
        case UBX_MSG_NAV_DAHEADING: {
            const auto decoded_payload_rx_nav_daheading =
                UBX::MessageCodec<ubx_payload_rx_nav_daheading_t>::decode(payload);
            if (!decoded_payload_rx_nav_daheading) {
                break;
            }
            const auto& payload_rx_nav_daheading = *decoded_payload_rx_nav_daheading;

            {
                const float rel_length_m = payload_rx_nav_daheading.relPosLength * 1e-3f;  // mm -> m
                const uint32_t flags = payload_rx_nav_daheading.flags;
                const bool heading_valid_flag = flags & (1 << 6);                          // bit 8 in NAV-RELPOSNED
                const bool rel_pos_valid = flags & (1 << 2);
                const bool carrier_solution_fixed = flags & (1 << 4);

                const bool heading_qualified = heading_valid_flag && rel_pos_valid &&
                                               (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

                float heading_rad = NAN;
                float heading_acc_rad = NAN;

                if (heading_qualified) {
                    heading_rad = relPosHeadingToYaw(payload_rx_nav_daheading.relPosHeading);
                    heading_acc_rad = payload_rx_nav_daheading.accHeading * GPS_DEG_TO_RAD * 1e-5f;
                }

                position.navigation.headingRadians = heading_rad;
                position.navigation.headingAccuracyRadians = heading_acc_rad;

                ret = 1;
            }

            break;
        }
        case UBX_MSG_MON_VER:

            // This is polled only on startup, and the startup code waits for an ack
            _controller.accept(UBX::Acknowledgement{UBX_MSG_MON_VER, true});

            ret = 1;
            break;

        case UBX_MSG_MON_HW: {
            switch (payload.size()) {
                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx6_t>: /* u-blox 6 msg format */ {
                    const auto decoded = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx6_t>::decode(payload);
                    if (!decoded) {
                        break;
                    }
                    const auto& payload_rx_mon_hw_ubx6 = *decoded;
                    _integrity.rf.noisePerMillisecond = payload_rx_mon_hw_ubx6.noisePerMS;
                    _integrity.rf.automaticGainControl = payload_rx_mon_hw_ubx6.agcCnt;
                    _integrity.rf.jammingIndicator = payload_rx_mon_hw_ubx6.jamInd;
                    _integrity.rf.timestampUs = nowUs();

                    ret = 1;
                    break;
                }

                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx7_t>: /* u-blox 7+ msg format */ {
                    const auto decoded = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx7_t>::decode(payload);
                    if (!decoded) {
                        break;
                    }
                    const auto& payload_rx_mon_hw_ubx7 = *decoded;
                    _integrity.rf.noisePerMillisecond = payload_rx_mon_hw_ubx7.noisePerMS;
                    _integrity.rf.automaticGainControl = payload_rx_mon_hw_ubx7.agcCnt;
                    _integrity.rf.jammingIndicator = payload_rx_mon_hw_ubx7.jamInd;
                    _integrity.rf.timestampUs = nowUs();

                    ret = 1;
                    break;
                }

                case UBX::MON_HW_DEPRECATED_SIZE: /* u-blox 27+ deprecated, ignore */
                    ret = 0;
                    break;

                default:      // unexpected payload size:
                    ret = 0;  // don't handle message
                    break;
            }

            break;
        }
        case UBX_MSG_MON_RF: {
            const auto decoded_payload_rx_mon_rf = UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode(payload);
            if (!decoded_payload_rx_mon_rf) {
                break;
            }
            const auto& payload_rx_mon_rf = *decoded_payload_rx_mon_rf;

            // TODO: only block 0 is read. F9P reports 2 blocks, X20 3, each with its own noisePerMS,
            // agcCnt and cwSuppression (jamInd). cwSuppression is the CW notch in effect per front end,
            // i.e. the per-frequency mitigation state GNSS_BANDS wants; the block covering a SEC-SIG
            // center frequency comes from rfBlockGnssBand (HPG 2.10) or blockId on older firmware.
            _integrity.rf.noisePerMillisecond = payload_rx_mon_rf.block[0].noisePerMS;
            _integrity.rf.automaticGainControl = payload_rx_mon_rf.block[0].agcCnt;
            _integrity.rf.jammingIndicator = payload_rx_mon_rf.block[0].jamInd;
            _integrity.rf.timestampUs = nowUs();

            if (!_got_sec_sig) {
                _integrity.jamming.state =
                    GPSIntegrityReport::jammingStateFromValue(payload_rx_mon_rf.block[0].flags & 0x03);
                _integrity.jamming.timestampUs = nowUs();
            }

            ret = 1;
            break;
        }
        case UBX_MSG_SEC_SIG: {
            const auto decoded_payload_rx_sec_sig = UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode(payload);
            if (!decoded_payload_rx_sec_sig) {
                break;
            }
            const auto& payload_rx_sec_sig = *decoded_payload_rx_sec_sig;

            {
                const uint8_t version = payload_rx_sec_sig.version;
                uint8_t flag_byte;

                if (version == 1) {
                    if (payload.size() < 5) {
                        ret = 0;
                        break;
                    }

                    flag_byte = payload_rx_sec_sig.jamFlags;

                } else {
                    flag_byte = payload_rx_sec_sig.flags;
                }

                auto jammingState = GPSIntegrityReport::JammingState::Unknown;

                // TODO: bits 6..4 of the same byte are spfState (v1: spfFlags at offset 8, bits 3..1).
                // spoofing_state still comes from NAV-STATUS spoofDetState, whose F9 value 3 means
                // "multiple indications"; SEC-SIG distinguishes indicated/suspected from affirmed/
                // detected, which is the DETECTED vs AFFECTED split the MAVLink GNSS_INTEGRITY rework
                // maps to.
                if (flag_byte & 0x01) {
                    const uint8_t jam_state = (flag_byte >> 1) & 0x03;

                    // SEC-SIG jamState: 0 unknown, 1 none, 2 warning (jamming indicated).
                    // Pre-v2 MON-RF also had 3 = critical. sensor_gps 2 is "mitigated";
                    // commander only alerts on 3 (detected).
                    if (jam_state >= 2) {
                        jammingState = GPSIntegrityReport::JammingState::Critical;

                    } else {
                        jammingState = GPSIntegrityReport::jammingStateFromValue(jam_state);
                    }
                }

                _integrity.jamming.state = jammingState;
                _integrity.jamming.timestampUs = nowUs();
                _got_sec_sig = true;

                // TODO: v2/v3 carry jamNumCentFreqs X4 groups after the header (bits 23..0 centFreq in
                // kHz, bit 24 jammed), one per in-use band. Not parsed: sensor_gps has nowhere to put
                // per-band state until the GNSS_BANDS message from mavlink/rfcs#30 lands, at which
                // point both the RX struct and payloadRxInit() length check need the repeated group.
            }

            ret = 1;
            break;
        }
        case UBX_MSG_RXM_RTCM: {
            const auto decoded_payload_rx_rxm_rtcm = UBX::MessageCodec<ubx_payload_rx_rxm_rtcm_t>::decode(payload);
            if (!decoded_payload_rx_rxm_rtcm) {
                break;
            }
            const auto& payload_rx_rxm_rtcm = *decoded_payload_rx_rxm_rtcm;

            _integrity.corrections.timestampUs = nowUs();
            _integrity.corrections.protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
            _integrity.corrections.crcFailed = (payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_CRCFAILED_MASK) != 0;
            _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue(
                (payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_MSGUSED_MASK) >> UBX_RX_RXM_RTCM_MSGUSED_SHIFT);

            ret = 1;
            break;
        }
        case UBX_MSG_RXM_COR: {
            const auto decoded_payload_rx_rxm_cor = UBX::MessageCodec<ubx_payload_rx_rxm_cor_t>::decode(payload);
            if (!decoded_payload_rx_rxm_cor) {
                break;
            }
            const auto& payload_rx_rxm_cor = *decoded_payload_rx_rxm_cor;

            {
                const uint32_t status = payload_rx_rxm_cor.statusInfo;
                auto protocol = GPSIntegrityReport::CorrectionProtocol::Unknown;

                switch (status & UBX_RX_RXM_COR_PROTOCOL_MASK) {
                    case 1:
                        protocol = GPSIntegrityReport::CorrectionProtocol::RTCM3;
                        break;

                    case 2:
                        protocol = GPSIntegrityReport::CorrectionProtocol::SPARTN;
                        break;

                    case 5:
                        protocol = GPSIntegrityReport::CorrectionProtocol::HAS;
                        break;

                    case 29:
                        protocol = GPSIntegrityReport::CorrectionProtocol::PMP;
                        break;

                    case 30:
                        protocol = GPSIntegrityReport::CorrectionProtocol::QZSSL6;
                        break;
                }

                _integrity.corrections.timestampUs = nowUs();
                _integrity.corrections.protocol = protocol;
                _integrity.corrections.crcFailed =
                    ((status & UBX_RX_RXM_COR_ERRSTATUS_MASK) >> UBX_RX_RXM_COR_ERRSTATUS_SHIFT) == 2;
                _integrity.corrections.use = GPSIntegrityReport::correctionUseFromValue(
                    (status & UBX_RX_RXM_COR_MSGUSED_MASK) >> UBX_RX_RXM_COR_MSGUSED_SHIFT);
            }

            ret = 1;
            break;
        }
        case UBX_MSG_ACK_ACK: {
            const auto decoded_payload_rx_ack_ack = UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode(payload);
            if (!decoded_payload_rx_ack_ack) {
                break;
            }
            const auto& payload_rx_ack_ack = *decoded_payload_rx_ack_ack;

            _controller.accept(UBX::Acknowledgement{payload_rx_ack_ack.msg, true});

            ret = 1;
            break;
        }
        case UBX_MSG_ACK_NAK: {
            const auto decoded_payload_rx_ack_ack = UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode(payload);
            if (!decoded_payload_rx_ack_ack) {
                break;
            }
            const auto& payload_rx_ack_ack = *decoded_payload_rx_ack_ack;

            _controller.accept(UBX::Acknowledgement{payload_rx_ack_ack.msg, false});

            ret = 1;
            break;
        }
        default:
            break;
    }

    if (ret > 0) {
        switch (message) {
            case UBX_MSG_NAV_STATUS:
            case UBX_MSG_MON_HW:
            case UBX_MSG_MON_RF:
            case UBX_MSG_SEC_SIG:
            case UBX_MSG_RXM_RTCM:
            case UBX_MSG_RXM_COR:
                publishIntegrity();
                break;
            default:
                break;
        }
    }

    return ret;
}

void GPSNativeUBX::logCommsDiagnostics(std::span<const uint8_t> payload)
{
    if (_comms.deadlineUs == 0 || nowUs() > _comms.deadlineUs) {
        return;
    }

    const auto decoded_status = UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode(payload);
    if (!decoded_status) {
        return;
    }
    const auto& status = *decoded_status;

    if (status.version != 0 || status.nPorts > UBX_MON_COMMS_MAX_PORTS ||
        payload.size() != 8 + status.nPorts * UBX::WIRE_SIZE<ubx_payload_rx_mon_comms_port_t>) {
        return;
    }

    _comms.deadlineUs = 0;
    log(GPSProtocolLogLevel::Warning, "MON-COMMS after txbuf: txErrors=0x%02x ports=%u (snapshot after warning)",
        (unsigned) status.txErrors, (unsigned) status.nPorts);

    for (unsigned i = 0; i < status.nPorts; ++i) {
        const auto& port = status.ports[i];
        [[maybe_unused]] const char* name = "unknown";

        switch (port.portId) {
            case 0x0000:
                name = "I2C";
                break;
            case 0x0100:
                name = "UART1";
                break;
            case 0x0201:
                name = "UART2";
                break;
            case 0x0300:
                name = "USB";
                break;
            case 0x0400:
                name = "SPI";
                break;
            default:
                break;
        }

        log(GPSProtocolLogLevel::Warning,
            "MON-COMMS %s port=0x%04x txPending=%u txUsage=%u%% txPeakUsage=%u%% "
            "rxPending=%u rxUsage=%u%% overrunErrs=%u skipped=%lu",
            name, (unsigned) port.portId, (unsigned) port.txPending, (unsigned) port.txUsage,
            (unsigned) port.txPeakUsage, (unsigned) port.rxPending, (unsigned) port.rxUsage,
            (unsigned) port.overrunErrs, (unsigned long) port.skipped);
    }
}

void GPSNativeUBX::decodeInit()
{
    _frameDecoder.reset();
}

float GPSNativeUBX::relPosHeadingToYaw(int32_t heading) const
{
    float heading_rad = heading * GPS_DEG_TO_RAD * 1e-5f;

    // Normalize to [-pi, pi]
    if (heading_rad > GPS_PI) {
        heading_rad -= 2.f * GPS_PI;

    } else if (heading_rad < -GPS_PI) {
        heading_rad += 2.f * GPS_PI;
    }

    return heading_rad;
}

void GPSNativeUBX::calcChecksum(const uint8_t* buffer, const uint16_t length, ubx_checksum_t* checksum)
{
    for (uint16_t i = 0; i < length; i++) {
        checksum->ck_a = checksum->ck_a + buffer[i];
        checksum->ck_b = checksum->ck_b + checksum->ck_a;
    }
}

int GPSNativeUBX::decodeValidatedPayload(uint16_t message, std::span<const uint8_t> payload)
{
    const auto* schema = UBX::messageSchema(message);
    if (!UBX::validPayload(message, payload, schema)) {
        return 0;
    }
    if (message == UBX::NAV_EOE && payload.size() == 4 && _decodeContext.assembleEpochs) {
        uint32_t tow = 0;
        for (unsigned index = 0; index < 4; ++index) {
            tow |= uint32_t(payload[index]) << (index * 8);
        }
        if (tow < UBXNavigationEpoch::WEEK_MS) {
            _navigationEpochs.end(tow, [this](const auto& report) { publishEpoch(report); });
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (!payloadRxInit(message, payload)) {
        return 0;
    }
    UBXNavigationEpoch::Epoch* epoch = nullptr;
    const auto publish = [this](const auto& report) { publishEpoch(report); };
    const bool timed = schema && schema->towOffset >= 0;
    if (_decodeContext.assembleEpochs && timed) {
        const size_t offset = schema->towOffset;
        const auto tow = LittleEndian::read<uint32_t>(payload, offset).value_or(0);
        epoch = _navigationEpochs.find(tow, nowUs(), publish);
        if (!epoch) {
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        _epochHasHighPrecision = epoch->highPrecision;
    }
    switch (message) {
        case UBX_MSG_NAV_SAT:
            *_satellites = {};
            decodeNavSat(payload);
            break;
        case UBX_MSG_NAV_SVINFO:
            *_satellites = {};
            decodeNavSvinfo(payload);
            break;
        case UBX_MSG_MON_VER:
            decodeMonVer(payload);
            break;
        default:
            break;
    }
    const int updates = payloadRxDone(message, payload, epoch ? epoch->position : _position);
    if (epoch) {
        if (message == UBX_MSG_NAV_PVT) {
            epoch->positionValid = epoch->velocityValid = true;
        } else if (message == UBX_MSG_NAV_POSLLH) {
            epoch->positionValid = true;
        } else if (message == UBX_MSG_NAV_VELNED) {
            epoch->velocityValid = true;
        } else if (message == UBX_MSG_NAV_HPPOSLLH && (updates & GPSDecodedBatch::POSITION_UPDATE)) {
            epoch->highPrecision = true;
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    // ACKs and ancillary metadata are useful protocol activity, not new position epochs.
    if ((updates & GPSDecodedBatch::POSITION_UPDATE) && message != UBX_MSG_NAV_PVT && message != UBX_MSG_NAV_POSLLH &&
        message != UBX_MSG_NAV_HPPOSLLH && message != UBX_MSG_NAV_VELNED) {
        return (updates & ~GPSDecodedBatch::POSITION_UPDATE) | GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (updates & GPSDecodedBatch::POSITION_UPDATE) {
        publishPosition(_position);
    }
    if ((updates & GPSDecodedBatch::SATELLITES_UPDATE) && _satellites) {
        publishSatellites(*_satellites);
    }
    return updates;
}

int GPSNativeUBX::decodeByte(uint8_t byte)
{
    return parseChar(byte);
}
