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

#include "UBXMessageCodec.h"
#include "UBXPrivate.h"

int GPSDriverUBX::parseChar(uint8_t byte)
{
    if (_rtcm_parsing && (_frameDecoder.idle() || _rtcm_parsing->hasPartialFrame())) {
        if (_rtcm_parsing->addByte(byte)) {
            if (_rtcm_parsing->valid())
                gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            _rtcm_parsing->reset();
            return 0;
        }
        if (_rtcm_parsing->hasPartialFrame())
            return 0;
    }
    const auto frame = _frameDecoder.consume(byte);
    if (!frame)
        return 0;
    _rx_msg = frame->message;
    _rx_payload_length = frame->length;
    _framePayload = frame->payload;
    const int updates = decodeValidatedPayload();
    if (_rtcm_parsing)
        _rtcm_parsing->reset();
    return updates;
}

int  // -1 = abort, 0 = continue
GPSDriverUBX::payloadRxInit()
{
    int ret = 0;

    _rx_state = UBX_RXMSG_HANDLE;  // handle by default

    switch (_rx_msg) {
        case UBX_MSG_CFG_VALGET:
            if (!_controller.readbackPending()) {
                _rx_state = UBX_RXMSG_IGNORE;
            } else if (_rx_payload_length < 4 || _rx_payload_length > UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;
            }
            break;
        case UBX_MSG_MON_COMMS:
            break;

        case UBX_MSG_NAV_PVT:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else if (!_use_nav_pvt) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if not using NAV-PVT
            }

            break;

        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_NOTICE:
        case UBX_MSG_INF_WARNING:
            if (_rx_payload_length >= UBX::MAX_CONTROL_PAYLOAD_SIZE) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;
            }

            break;

        case UBX_MSG_NAV_POSLLH:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else if (_use_nav_pvt) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_NAV_SOL:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else if (_use_nav_pvt) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_NAV_STATUS:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_NAV_DOP:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_NAV_RELPOSNED:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_NAV_DAHEADING:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_NAV_HPPOSLLH:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_NAV_TIMEUTC:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else if (_use_nav_pvt) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:
            if (_satellite_info == nullptr) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if sat info not requested

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else {
                *_satellite_info = {};  // initialize sat info
            }

            break;

        case UBX_MSG_NAV_SVIN:
            break;

        case UBX_MSG_NAV_VELNED:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation

            } else if (_use_nav_pvt) {
                _rx_state = UBX_RXMSG_DISABLE;  // disable if using NAV-PVT instead
            }

            break;

        case UBX_MSG_MON_VER:
            break;  // unconditionally handle this message

        case UBX_MSG_MON_HW:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_MON_RF:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_SEC_SIG:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;
            }

            break;

        case UBX_MSG_RXM_RTCM:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_RXM_COR:
            if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;
            }

            break;

        case UBX_MSG_ACK_ACK:
            if (!_controller.awaitingAcknowledgement()) {
                _rx_state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        case UBX_MSG_ACK_NAK:
            if (!_controller.awaitingAcknowledgement()) {
                _rx_state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        default:
            _rx_state = UBX_RXMSG_DISABLE;  // disable all other messages
            break;
    }

    switch (_rx_state) {
        case UBX_RXMSG_HANDLE:  // handle message
        case UBX_RXMSG_IGNORE:  // ignore message but don't report error
            ret = 0;
            break;

        case UBX_RXMSG_DISABLE:  // disable unexpected messages

                                 // TODO: UBX-MON-HW2
            // [uavcan:52:gps] ubx msg 0x0a0b len 28 unexpected

            _pendingDisableMessage = _rx_msg;

            ret = -1;  // return error, abort handling this message
            break;

        case UBX_RXMSG_ERROR_LENGTH:  // error: invalid length

            ret = -1;                 // return error, abort handling this message
            break;

        default:       // invalid message state
            log(GPSProtocolLogLevel::Warning, "ubx internal err1");
            ret = -1;  // return error, abort handling this message
            break;
    }

    return ret;
}

void GPSDriverUBX::decodeNavSat(std::span<const uint8_t> payload)
{
    auto decoded_header = UBX::MessageCodec<ubx_payload_rx_nav_sat_part1_t>::block(payload);
    if (!decoded_header)
        return;
    const auto header = *decoded_header;
    _satellite_info->count = std::min<size_t>(header.numSvs, _satellite_info->entries.size());
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS, GPSConstellation::NavIC};
    for (size_t index = 0; index < _satellite_info->count; ++index) {
        auto decoded_wire = UBX::MessageCodec<ubx_payload_rx_nav_sat_part2_t>::block(
            payload, UBX::WIRE_SIZE<std::remove_cv_t<decltype(header)>> +
                         index * UBX::WIRE_SIZE<ubx_payload_rx_nav_sat_part2_t>);
        if (!decoded_wire)
            return;
        const auto wire = *decoded_wire;
        auto& satellite = _satellite_info->entries[index];
        satellite.constellation = wire.gnssId < std::size(systems) ? systems[wire.gnssId] : GPSConstellation::Unknown;
        satellite.id = satellite.prn = wire.svId;
        satellite.used = (wire.flags & 8) != 0;
        satellite.elevation = wire.elev;
        satellite.azimuth = wire.azim;
        satellite.signal = wire.cno;
    }
}

void GPSDriverUBX::decodeNavSvinfo(std::span<const uint8_t> payload)
{
    auto decoded_header = UBX::MessageCodec<ubx_payload_rx_nav_svinfo_part1_t>::block(payload);
    if (!decoded_header)
        return;
    const auto header = *decoded_header;
    _satellite_info->count = std::min<size_t>(header.numCh, _satellite_info->entries.size());
    for (size_t index = 0; index < _satellite_info->count; ++index) {
        auto decoded_wire = UBX::MessageCodec<ubx_payload_rx_nav_svinfo_part2_t>::block(
            payload, UBX::WIRE_SIZE<std::remove_cv_t<decltype(header)>> +
                         index * UBX::WIRE_SIZE<ubx_payload_rx_nav_svinfo_part2_t>);
        if (!decoded_wire)
            return;
        const auto wire = *decoded_wire;
        auto& satellite = _satellite_info->entries[index];
        satellite.id = satellite.prn = wire.svid;
        satellite.used = (wire.flags & 1) != 0;
        satellite.elevation = wire.elev;
        satellite.azimuth = wire.azim;
        satellite.signal = wire.cno;
    }
}

void GPSDriverUBX::decodeMonVer(std::span<const uint8_t> payload)
{
    _model_name[0] = '\0';
    _firmware_version[0] = '\0';
    auto decoded_payload_rx_mon_ver_part1 = UBX::MessageCodec<ubx_payload_rx_mon_ver_part1_t>::block(payload);
    if (!decoded_payload_rx_mon_ver_part1)
        return;
    auto payload_rx_mon_ver_part1 = *decoded_payload_rx_mon_ver_part1;
    // The protocol specifies these as nul-terminated strings, but the terminator comes
    // from the device, so enforce it before anything walks the field.
    payload_rx_mon_ver_part1.swVersion[sizeof(payload_rx_mon_ver_part1.swVersion) - 1] = 0;
    payload_rx_mon_ver_part1.hwVersion[sizeof(payload_rx_mon_ver_part1.hwVersion) - 1] = 0;
    memcpy(_firmware_version, payload_rx_mon_ver_part1.swVersion, sizeof(_firmware_version));

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
            _board = known_board.board;
            known = true;
            break;
        }
    }

    if (!known) {
        log(GPSProtocolLogLevel::Warning, "unknown board hw: %s", payload_rx_mon_ver_part1.hwVersion);
    }
    for (size_t offset = UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part1_t>; offset < payload.size();
         offset += UBX::WIRE_SIZE<ubx_payload_rx_mon_ver_part2_t>) {
        auto decoded_payload_rx_mon_ver_part2 =
            UBX::MessageCodec<ubx_payload_rx_mon_ver_part2_t>::block(payload, offset);
        if (!decoded_payload_rx_mon_ver_part2)
            return;
        auto payload_rx_mon_ver_part2 = *decoded_payload_rx_mon_ver_part2;
        // Part 2 complete: decode Part 2 buffer
        // Same as above: the protocol specifies a nul-terminated string, the device provides
        // the terminator, so enforce it before strstr() walks the field.
        payload_rx_mon_ver_part2.extension[sizeof(payload_rx_mon_ver_part2.extension) - 1] = 0;

        // "FWVER=" Firmware of product category and version
        const char* fwver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "FWVER=");

        if (fwver_str != nullptr) {
            strncpy(_firmware_version, fwver_str + strlen("FWVER="), sizeof(_firmware_version) - 1);
            _firmware_version[sizeof(_firmware_version) - 1] = '\0';
            log(GPSProtocolLogLevel::Debug, "u-blox firmware version: %s", fwver_str + strlen("FWVER="));

            // Check if its a ZED-F9P-15B
            if ((_board == Board::u_blox9) && strstr(fwver_str, "HPGL1L5")) {
                _board = Board::u_blox9_F9P_L1L5;
            }
        }

        // "PROTVER=" Supported protocol version.
        const char* protver_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "PROTVER=");

        if (protver_str != nullptr) {
            log(GPSProtocolLogLevel::Debug, "u-blox protocol version: %s", protver_str + strlen("PROTVER="));
        }

        // "MOD=" Module identification. Set in production.
        const char* mod_str = strstr((const char*) payload_rx_mon_ver_part2.extension, "MOD=");

        if (mod_str != nullptr) {
            strncpy(_model_name, mod_str + strlen("MOD="), sizeof(_model_name) - 1);
            _model_name[sizeof(_model_name) - 1] = '\0';
            _is_m8p = strstr(mod_str, "M8P") != nullptr;
            // in case of u-blox9 family, check if it's an F9P
            if (_board == Board::u_blox9) {
                if (strstr(mod_str, "F9P")) {
                    _board = Board::u_blox9_F9P_L1L2;
                }

            } else if (_board == Board::u_blox10) {
                if (strstr(mod_str, "DAN-F10N")) {
                    _board = Board::u_blox10_L1L5;
                }
            }

            log(GPSProtocolLogLevel::Debug, "u-blox module: %s", mod_str + strlen("MOD="));
        }
    }
}

int  // 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::payloadRxDone(GPSPositionReport& position)
{
    int ret = 0;

    // return if no message handled
    if (_rx_state != UBX_RXMSG_HANDLE) {
        return ret;
    }

    // handle message
    switch (_rx_msg) {
        case UBX_MSG_NAV_PVT: {
            const auto decoded_payload_rx_nav_pvt =
                UBX::MessageCodec<ubx_payload_rx_nav_pvt_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_pvt)
                break;
            const auto& payload_rx_nav_pvt = *decoded_payload_rx_nav_pvt;

            // Check if position fix flag is good
            if ((payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK) == 1) {
                position.fix_type = payload_rx_nav_pvt.fixType;

                if (payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN) {
                    position.fix_type = 4;  // DGPS
                }

                uint8_t carr_soln = payload_rx_nav_pvt.flags >> 6;

                if (carr_soln == 1) {
                    position.fix_type = 5;  // Float RTK

                } else if (carr_soln == 2) {
                    position.fix_type = 6;  // Fixed RTK
                }

                position.vel_ned_valid = true;

            } else {
                position.fix_type = 0;
                position.vel_ned_valid = false;
            }

            position.satellites_used = payload_rx_nav_pvt.numSV;

            if (_assembleEpochs ? !_epochHasHighPrecision : position.fix_type < 6) {
                // When RTK is active and solid (fix=6), these values will be filled by HPPOSLLH:
                position.latitude_deg = payload_rx_nav_pvt.lat * UBX::DEGREES_PER_COORDINATE;
                position.longitude_deg = payload_rx_nav_pvt.lon * UBX::DEGREES_PER_COORDINATE;
                position.altitude_msl_m = payload_rx_nav_pvt.hMSL * 1e-3;
                position.altitude_ellipsoid_m = payload_rx_nav_pvt.height * 1e-3;

                position.eph = static_cast<float>(payload_rx_nav_pvt.hAcc) * 1e-3f;
                position.accuracy_timestamp = nowUs();
                position.epv = static_cast<float>(payload_rx_nav_pvt.vAcc) * 1e-3f;

                _got_posllh = true;
            }

            position.speedAccuracyMetersPerSecond = static_cast<float>(payload_rx_nav_pvt.sAcc) * 1e-3f;

            position.vel_m_s = static_cast<float>(payload_rx_nav_pvt.gSpeed) * 1e-3f;

            position.vel_n_m_s = static_cast<float>(payload_rx_nav_pvt.velN) * 1e-3f;
            position.vel_e_m_s = static_cast<float>(payload_rx_nav_pvt.velE) * 1e-3f;
            position.vel_d_m_s = static_cast<float>(payload_rx_nav_pvt.velD) * 1e-3f;

            position.cog_rad = static_cast<float>(payload_rx_nav_pvt.headMot) * GPS_DEG_TO_RAD * 1e-5f;
            position.courseAccuracyRadians = static_cast<float>(payload_rx_nav_pvt.headAcc) * GPS_DEG_TO_RAD * 1e-5f;

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
                position.time_utc_usec = timeFromUtc(timeinfo, payload_rx_nav_pvt.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                position.time_utc_usec = 0;
            }

            position.timestamp = nowUs();
            _last_timestamp_time = position.timestamp;

            _got_velned = true;

            ret = 1;
            break;
        }
        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_NOTICE: {
            uint8_t* p_buf = _framePayload.data();
            p_buf[_rx_payload_length] = 0;

        } break;

        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_WARNING: {
            uint8_t* p_buf = _framePayload.data();
            p_buf[_rx_payload_length] = 0;
            log(GPSProtocolLogLevel::Warning, "ubx msg: %s", p_buf);

            if (strncmp(reinterpret_cast<const char*>(p_buf), "txbuf", 5) == 0) {
                _comms_request_pending = true;
            }
        } break;

        case UBX_MSG_MON_COMMS:
            logCommsDiagnostics();
            break;
        case UBX_MSG_CFG_VALGET:
            if (const auto values = UBX::decodeConfigurationValues({_framePayload.data(), _rx_payload_length}))
                _controller.accept(*values);
            break;

        case UBX_MSG_NAV_POSLLH: {
            const auto decoded_payload_rx_nav_posllh =
                UBX::MessageCodec<ubx_payload_rx_nav_posllh_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_posllh)
                break;
            const auto& payload_rx_nav_posllh = *decoded_payload_rx_nav_posllh;

            position.latitude_deg = payload_rx_nav_posllh.lat * UBX::DEGREES_PER_COORDINATE;
            position.longitude_deg = payload_rx_nav_posllh.lon * UBX::DEGREES_PER_COORDINATE;
            position.altitude_msl_m = payload_rx_nav_posllh.hMSL * 1e-3;
            position.altitude_ellipsoid_m = payload_rx_nav_posllh.height * 1e-3;
            position.eph = static_cast<float>(payload_rx_nav_posllh.hAcc) * 1e-3f;  // from mm to m
            position.accuracy_timestamp = nowUs();
            position.epv = static_cast<float>(payload_rx_nav_posllh.vAcc) * 1e-3f;  // from mm to m

            position.timestamp = nowUs();

            _got_posllh = true;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_HPPOSLLH: {
            const auto decoded_payload_rx_nav_hpposllh =
                UBX::MessageCodec<ubx_payload_rx_nav_hpposllh_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_hpposllh)
                break;
            const auto& payload_rx_nav_hpposllh = *decoded_payload_rx_nav_hpposllh;

            if (payload_rx_nav_hpposllh.flags == 0 && (_assembleEpochs || position.fix_type == 6)) {
                position.latitude_deg =
                    payload_rx_nav_hpposllh.lat * UBX::DEGREES_PER_COORDINATE +
                    payload_rx_nav_hpposllh.latHp * 1e-9;  // regular precision lat/lon (1e7), plus high precision (1e9)
                position.longitude_deg =
                    payload_rx_nav_hpposllh.lon * UBX::DEGREES_PER_COORDINATE + payload_rx_nav_hpposllh.lonHp * 1e-9;
                position.altitude_msl_m =
                    payload_rx_nav_hpposllh.hMSL * 1e-3 +
                    payload_rx_nav_hpposllh.hMSLHp *
                        1e-4;  // regular precision altitude, mm, plus high precision components of altitude, 0.1 mm
                position.altitude_ellipsoid_m =
                    payload_rx_nav_hpposllh.height * 1e-3 + payload_rx_nav_hpposllh.heightHp * 1e-4;
                position.eph = static_cast<float>(payload_rx_nav_hpposllh.hAcc) *
                               1e-4f;  // Accuracy estimates, convert from 0.1 mm to m
                position.epv = static_cast<float>(payload_rx_nav_hpposllh.vAcc) * 1e-4f;
                position.accuracy_timestamp = nowUs();

                position.timestamp = nowUs();

                _got_posllh = true;

                ret = 1;
            }

            break;
        }
        case UBX_MSG_NAV_SOL: {
            const auto decoded_payload_rx_nav_sol =
                UBX::MessageCodec<ubx_payload_rx_nav_sol_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_sol)
                break;
            const auto& payload_rx_nav_sol = *decoded_payload_rx_nav_sol;

            position.fix_type = payload_rx_nav_sol.gpsFix;
            position.speedAccuracyMetersPerSecond =
                static_cast<float>(payload_rx_nav_sol.sAcc) * 1e-2f;  // from cm to m
            position.satellites_used = payload_rx_nav_sol.numSV;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_STATUS: {
            const auto decoded_payload_rx_nav_status =
                UBX::MessageCodec<ubx_payload_rx_nav_status_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_status)
                break;
            const auto& payload_rx_nav_status = *decoded_payload_rx_nav_status;

            _integrity.spoofing_state = (payload_rx_nav_status.flags2 & UBX_RX_NAV_STATUS_SPOOFDETSTATE_MASK) >>
                                        UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;
            _integrity.spoofing_state_timestamp = nowUs();

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_DOP: {
            const auto decoded_payload_rx_nav_dop =
                UBX::MessageCodec<ubx_payload_rx_nav_dop_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_dop)
                break;
            const auto& payload_rx_nav_dop = *decoded_payload_rx_nav_dop;

            position.hdop = payload_rx_nav_dop.hDOP * UBX::DOP_PER_UNIT;
            position.dop_timestamp = nowUs();
            position.vdop = payload_rx_nav_dop.vDOP * UBX::DOP_PER_UNIT;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_TIMEUTC: {
            const auto decoded_payload_rx_nav_timeutc =
                UBX::MessageCodec<ubx_payload_rx_nav_timeutc_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_timeutc)
                break;
            const auto& payload_rx_nav_timeutc = *decoded_payload_rx_nav_timeutc;

            if (payload_rx_nav_timeutc.valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) {
                tm timeinfo{};
                timeinfo.tm_year = payload_rx_nav_timeutc.year - 1900;
                timeinfo.tm_mon = payload_rx_nav_timeutc.month - 1;
                timeinfo.tm_mday = payload_rx_nav_timeutc.day;
                timeinfo.tm_hour = payload_rx_nav_timeutc.hour;
                timeinfo.tm_min = payload_rx_nav_timeutc.min;
                timeinfo.tm_sec = payload_rx_nav_timeutc.sec;
                position.time_utc_usec = timeFromUtc(timeinfo, payload_rx_nav_timeutc.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                position.time_utc_usec = 0;
            }

            _last_timestamp_time = nowUs();

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:

            // Satellite entries were decoded from the validated payload.
            _satellite_info->timestamp = nowUs();

            ret = 2;
            break;

        case UBX_MSG_NAV_SVIN: {
            const auto decoded_payload_rx_nav_svin =
                UBX::MessageCodec<ubx_payload_rx_nav_svin_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_svin)
                break;
            const auto& payload_rx_nav_svin = *decoded_payload_rx_nav_svin;

            {
                const ubx_payload_rx_nav_svin_t& svin = payload_rx_nav_svin;
                _survey_in_stopped = svin.active == 0 && svin.valid == 0;

                if (!_decodeNavigation) {
                    ret = 1;
                    break;
                }

                GPSSurveyReport status{};
                status.accuracyKnown = true;
                status.altitudeDatum = GPSSurveyReport::AltitudeDatum::Ellipsoid;
                double ecef_x = (static_cast<double>(svin.meanX) + static_cast<double>(svin.meanXHP) * 0.01) * 0.01;
                double ecef_y = (static_cast<double>(svin.meanY) + static_cast<double>(svin.meanYHP) * 0.01) * 0.01;
                double ecef_z = (static_cast<double>(svin.meanZ) + static_cast<double>(svin.meanZHP) * 0.01) * 0.01;
                ECEF2lla(ecef_x, ecef_y, ecef_z, status.latitude, status.longitude, status.altitude);
                status.duration = svin.dur;
                status.mean_accuracy = svin.meanAcc / 10;
                status.flags = (svin.valid & 1) | ((svin.active & 1) << 1);
                surveyInStatus(status);

                if (svin.valid == 1 && svin.active == 0) {
                    _rtcmActivationPending = true;
                }
            }

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_VELNED: {
            const auto decoded_payload_rx_nav_velned =
                UBX::MessageCodec<ubx_payload_rx_nav_velned_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_velned)
                break;
            const auto& payload_rx_nav_velned = *decoded_payload_rx_nav_velned;

            position.vel_m_s = static_cast<float>(payload_rx_nav_velned.gSpeed) * 1e-2f;
            position.vel_n_m_s = static_cast<float>(payload_rx_nav_velned.velN) * 1e-2f;  // NED NORTH velocity
            position.vel_e_m_s = static_cast<float>(payload_rx_nav_velned.velE) * 1e-2f;  // NED EAST velocity
            position.vel_d_m_s = static_cast<float>(payload_rx_nav_velned.velD) * 1e-2f;  // NED DOWN velocity
            position.cog_rad = static_cast<float>(payload_rx_nav_velned.heading) * GPS_DEG_TO_RAD * 1e-5f;
            position.courseAccuracyRadians = static_cast<float>(payload_rx_nav_velned.cAcc) * GPS_DEG_TO_RAD * 1e-5f;
            position.vel_ned_valid = true;

            _got_velned = true;

            ret = 1;
            break;
        }
        case UBX_MSG_NAV_RELPOSNED: {
            const auto decoded_payload_rx_nav_relposned =
                UBX::MessageCodec<ubx_payload_rx_nav_relposned_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_relposned)
                break;
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

                position.heading = heading_rad;
                position.heading_timestamp = nowUs();
                position.heading_accuracy = heading_acc_rad;

                GPSRelativeReport gps_rel{};

                gps_rel.timestamp_sample = nowUs();  // TODO: adjust with delay estimate

                gps_rel.time_utc_usec =
                    uint64_t(payload_rx_nav_relposned.iTOW) * 1000;  // TODO: convert iTOW ms GPS time of week
                gps_rel.reference_station_id = payload_rx_nav_relposned.refStationId;

                gps_rel.position[0] =
                    (payload_rx_nav_relposned.relPosN + payload_rx_nav_relposned.relPosHPN * 1e-2f) * 1e-2f;
                gps_rel.position[1] =
                    (payload_rx_nav_relposned.relPosE + payload_rx_nav_relposned.relPosHPE * 1e-2f) * 1e-2f;
                gps_rel.position[2] =
                    (payload_rx_nav_relposned.relPosD + payload_rx_nav_relposned.relPosHPD * 1e-2f) * 1e-2f;

                gps_rel.position_length = rel_length_m;

                gps_rel.heading = heading_rad;
                gps_rel.heading_accuracy = heading_acc_rad;

                gps_rel.position_accuracy[0] = payload_rx_nav_relposned.accN * 1e-4f;  // 0.1mm -> m
                gps_rel.position_accuracy[1] = payload_rx_nav_relposned.accE * 1e-4f;  // 0.1mm -> m
                gps_rel.position_accuracy[2] = payload_rx_nav_relposned.accD * 1e-4f;  // 0.1mm -> m

                gps_rel.accuracy_length = payload_rx_nav_relposned.accLength * 1e-4f;  // 0.1mm -> m;

                gps_rel.gnss_fix_ok = flags & (1 << 0);
                gps_rel.differential_solution = flags & (1 << 1);
                gps_rel.relative_position_valid = flags & (1 << 2);
                gps_rel.carrier_solution_floating = flags & (1 << 3);
                gps_rel.carrier_solution_fixed = flags & (1 << 4);
                gps_rel.moving_base_mode = flags & (1 << 5);
                gps_rel.reference_position_miss = flags & (1 << 6);
                gps_rel.reference_observations_miss = flags & (1 << 7);
                gps_rel.heading_valid = heading_qualified;
                gps_rel.relative_position_normalized = flags & (1 << 9);

                gotRelativePositionMessage(gps_rel);

                ret = 1;
            }

            break;
        }
        case UBX_MSG_NAV_DAHEADING: {
            const auto decoded_payload_rx_nav_daheading =
                UBX::MessageCodec<ubx_payload_rx_nav_daheading_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_nav_daheading)
                break;
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

                position.heading = heading_rad;
                position.heading_timestamp = nowUs();
                position.heading_accuracy = heading_acc_rad;

                GPSRelativeReport gps_rel{};

                gps_rel.timestamp_sample = nowUs();

                // time_utc_usec is left at 0 (documented as unavailable): NAV-DAHEADING only carries
                // iTOW, which cannot be converted to UTC without the week number and leap seconds.

                gps_rel.position[0] = payload_rx_nav_daheading.relPosN * 1e-3f;  // mm -> m
                gps_rel.position[1] = payload_rx_nav_daheading.relPosE * 1e-3f;
                gps_rel.position[2] = payload_rx_nav_daheading.relPosD * 1e-3f;

                gps_rel.position_length = rel_length_m;

                gps_rel.heading = heading_rad;
                gps_rel.heading_accuracy = heading_acc_rad;

                gps_rel.position_accuracy[0] = payload_rx_nav_daheading.accN * 1e-3f;
                gps_rel.position_accuracy[1] = payload_rx_nav_daheading.accE * 1e-3f;
                gps_rel.position_accuracy[2] = payload_rx_nav_daheading.accD * 1e-3f;

                gps_rel.accuracy_length = payload_rx_nav_daheading.accLength * 1e-3f;

                // NAV-DAHEADING has no reference station, moving base or normalized position flags
                gps_rel.gnss_fix_ok = flags & (1 << 0);
                gps_rel.differential_solution = flags & (1 << 1);
                gps_rel.relative_position_valid = rel_pos_valid;
                gps_rel.carrier_solution_floating = flags & (1 << 3);
                gps_rel.carrier_solution_fixed = carrier_solution_fixed;
                gps_rel.heading_valid = heading_qualified;

                gotRelativePositionMessage(gps_rel);

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
            switch (_rx_payload_length) {
                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx6_t>: /* u-blox 6 msg format */ {
                    const auto decoded = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx6_t>::decode(
                        {_framePayload.data(), _rx_payload_length});
                    if (!decoded)
                        break;
                    const auto& payload_rx_mon_hw_ubx6 = *decoded;
                    _integrity.noise_per_ms = payload_rx_mon_hw_ubx6.noisePerMS;
                    _integrity.automatic_gain_control = payload_rx_mon_hw_ubx6.agcCnt;
                    _integrity.jamming_indicator = payload_rx_mon_hw_ubx6.jamInd;
                    _integrity.rf_timestamp = nowUs();

                    ret = 1;
                    break;
                }

                case UBX::WIRE_SIZE<ubx_payload_rx_mon_hw_ubx7_t>: /* u-blox 7+ msg format */ {
                    const auto decoded = UBX::MessageCodec<ubx_payload_rx_mon_hw_ubx7_t>::decode(
                        {_framePayload.data(), _rx_payload_length});
                    if (!decoded)
                        break;
                    const auto& payload_rx_mon_hw_ubx7 = *decoded;
                    _integrity.noise_per_ms = payload_rx_mon_hw_ubx7.noisePerMS;
                    _integrity.automatic_gain_control = payload_rx_mon_hw_ubx7.agcCnt;
                    _integrity.jamming_indicator = payload_rx_mon_hw_ubx7.jamInd;
                    _integrity.rf_timestamp = nowUs();

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
            const auto decoded_payload_rx_mon_rf =
                UBX::MessageCodec<ubx_payload_rx_mon_rf_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_mon_rf)
                break;
            const auto& payload_rx_mon_rf = *decoded_payload_rx_mon_rf;

            // TODO: only block 0 is read. F9P reports 2 blocks, X20 3, each with its own noisePerMS,
            // agcCnt and cwSuppression (jamInd). cwSuppression is the CW notch in effect per front end,
            // i.e. the per-frequency mitigation state GNSS_BANDS wants; the block covering a SEC-SIG
            // center frequency comes from rfBlockGnssBand (HPG 2.10) or blockId on older firmware.
            _integrity.noise_per_ms = payload_rx_mon_rf.block[0].noisePerMS;
            _integrity.automatic_gain_control = payload_rx_mon_rf.block[0].agcCnt;
            _integrity.jamming_indicator = payload_rx_mon_rf.block[0].jamInd;
            _integrity.rf_timestamp = nowUs();

            if (!_got_sec_sig) {
                _integrity.jamming_state = payload_rx_mon_rf.block[0].flags & 0x03;
                _integrity.jamming_state_timestamp = nowUs();
            }

            ret = 1;
            break;
        }
        case UBX_MSG_SEC_SIG: {
            const auto decoded_payload_rx_sec_sig =
                UBX::MessageCodec<ubx_payload_rx_sec_sig_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_sec_sig)
                break;
            const auto& payload_rx_sec_sig = *decoded_payload_rx_sec_sig;

            {
                const uint8_t version = payload_rx_sec_sig.version;
                uint8_t flag_byte;

                if (version == 1) {
                    if (_rx_payload_length < 5) {
                        ret = 0;
                        break;
                    }

                    flag_byte = payload_rx_sec_sig.jamFlags;

                } else {
                    flag_byte = payload_rx_sec_sig.flags;
                }

                uint8_t jamming_state = 0;

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
                        jamming_state = 3;

                    } else {
                        jamming_state = jam_state;
                    }
                }

                _integrity.jamming_state = jamming_state;
                _integrity.jamming_state_timestamp = nowUs();
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
            const auto decoded_payload_rx_rxm_rtcm =
                UBX::MessageCodec<ubx_payload_rx_rxm_rtcm_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_rxm_rtcm)
                break;
            const auto& payload_rx_rxm_rtcm = *decoded_payload_rx_rxm_rtcm;

            _integrity.corrections_timestamp = nowUs();
            _integrity.corrections_protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_RTCM3;
            _integrity.corrections_crc_failed = (payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_CRCFAILED_MASK) != 0;
            _integrity.corrections_msg_used =
                (payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_MSGUSED_MASK) >> UBX_RX_RXM_RTCM_MSGUSED_SHIFT;

            ret = 1;
            break;
        }
        case UBX_MSG_RXM_COR: {
            const auto decoded_payload_rx_rxm_cor =
                UBX::MessageCodec<ubx_payload_rx_rxm_cor_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_rxm_cor)
                break;
            const auto& payload_rx_rxm_cor = *decoded_payload_rx_rxm_cor;

            {
                const uint32_t status = payload_rx_rxm_cor.statusInfo;
                uint8_t protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_UNKNOWN;

                switch (status & UBX_RX_RXM_COR_PROTOCOL_MASK) {
                    case 1:
                        protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_RTCM3;
                        break;

                    case 2:
                        protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_SPARTN;
                        break;

                    case 5:
                        protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_HAS;
                        break;

                    case 29:
                        protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_PMP;
                        break;

                    case 30:
                        protocol = GPSIntegrityReport::CORRECTIONS_PROTOCOL_QZSS_L6;
                        break;
                }

                _integrity.corrections_timestamp = nowUs();
                _integrity.corrections_protocol = protocol;
                _integrity.corrections_crc_failed =
                    ((status & UBX_RX_RXM_COR_ERRSTATUS_MASK) >> UBX_RX_RXM_COR_ERRSTATUS_SHIFT) == 2;
                _integrity.corrections_msg_used =
                    (status & UBX_RX_RXM_COR_MSGUSED_MASK) >> UBX_RX_RXM_COR_MSGUSED_SHIFT;
            }

            ret = 1;
            break;
        }
        case UBX_MSG_ACK_ACK: {
            const auto decoded_payload_rx_ack_ack =
                UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_ack_ack)
                break;
            const auto& payload_rx_ack_ack = *decoded_payload_rx_ack_ack;

            _controller.accept(UBX::Acknowledgement{payload_rx_ack_ack.msg, true});

            ret = 1;
            break;
        }
        case UBX_MSG_ACK_NAK: {
            const auto decoded_payload_rx_ack_ack =
                UBX::MessageCodec<ubx_payload_rx_ack_ack_t>::decode({_framePayload.data(), _rx_payload_length});
            if (!decoded_payload_rx_ack_ack)
                break;
            const auto& payload_rx_ack_ack = *decoded_payload_rx_ack_ack;

            _controller.accept(UBX::Acknowledgement{payload_rx_ack_ack.msg, false});

            ret = 1;
            break;
        }
        default:
            break;
    }

    if (ret > 0) {
        switch (_rx_msg) {
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

    if (ret > 0) {
        position.timestamp_time_relative = (int32_t) (_last_timestamp_time - position.timestamp);
    }

    return ret;
}

void GPSDriverUBX::logCommsDiagnostics()
{
    if (_comms_poll_deadline == 0 || nowUs() > _comms_poll_deadline) {
        return;
    }

    const auto decoded_status =
        UBX::MessageCodec<ubx_payload_rx_mon_comms_t>::decode({_framePayload.data(), _rx_payload_length});
    if (!decoded_status)
        return;
    const auto& status = *decoded_status;

    if (status.version != 0 || status.nPorts > UBX_MON_COMMS_MAX_PORTS ||
        _rx_payload_length != 8 + status.nPorts * UBX::WIRE_SIZE<ubx_payload_rx_mon_comms_port_t>) {
        return;
    }

    _comms_poll_deadline = 0;
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

void GPSDriverUBX::decodeInit()
{
    _frameDecoder.reset();
}

float GPSDriverUBX::relPosHeadingToYaw(int32_t heading) const
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

void GPSDriverUBX::calcChecksum(const uint8_t* buffer, const uint16_t length, ubx_checksum_t* checksum)
{
    for (uint16_t i = 0; i < length; i++) {
        checksum->ck_a = checksum->ck_a + buffer[i];
        checksum->ck_b = checksum->ck_b + checksum->ck_a;
    }
}

int GPSDriverUBX::decodeValidatedPayload()
{
    if (_rx_payload_length > _framePayload.size()) {
        return 0;
    }
    if (!UBX::validPayload(_rx_msg, {_framePayload.data(), _rx_payload_length}))
        return 0;
    if (_rx_msg == UBX::NAV_EOE && _rx_payload_length == 4 && _assembleEpochs) {
        uint32_t tow = 0;
        for (unsigned index = 0; index < 4; ++index)
            tow |= uint32_t(_framePayload[index]) << (index * 8);
        if (tow < UBXNavigationEpoch::WEEK_MS)
            _navigationEpochs.end(tow, [this](const auto& report) { publishEpoch(report); });
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    if (payloadRxInit() != 0 || _rx_state != UBX_RXMSG_HANDLE) {
        return 0;
    }
    UBXNavigationEpoch::Epoch* epoch = nullptr;
    const auto publish = [this](const auto& report) { publishEpoch(report); };
    const auto* schema = UBX::messageSchema(_rx_msg);
    const bool timed = schema && schema->towOffset >= 0;
    if (_assembleEpochs && timed) {
        const size_t offset = schema->towOffset;
        const auto tow = LittleEndian::read<uint32_t>(_framePayload, offset).value_or(0);
        epoch = _navigationEpochs.find(tow, nowUs(), publish);
        if (!epoch)
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        _epochHasHighPrecision = epoch->highPrecision;
    }
    const std::span<const uint8_t> payload{_framePayload.data(), _rx_payload_length};
    switch (_rx_msg) {
        case UBX_MSG_NAV_SAT:
            *_satellite_info = {};
            decodeNavSat(payload);
            break;
        case UBX_MSG_NAV_SVINFO:
            *_satellite_info = {};
            decodeNavSvinfo(payload);
            break;
        case UBX_MSG_MON_VER:
            decodeMonVer(payload);
            break;
        default:
            break;
    }
    const int updates = payloadRxDone(epoch ? epoch->position : *_gps_position);
    if (epoch) {
        if (_rx_msg == UBX_MSG_NAV_PVT) {
            epoch->positionValid = epoch->velocityValid = true;
        } else if (_rx_msg == UBX_MSG_NAV_POSLLH) {
            epoch->positionValid = true;
        } else if (_rx_msg == UBX_MSG_NAV_VELNED) {
            epoch->velocityValid = true;
        } else if (_rx_msg == UBX_MSG_NAV_HPPOSLLH && (updates & 1)) {
            epoch->highPrecision = true;
        }
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    }
    // ACKs and ancillary metadata are useful protocol activity, not new position epochs.
    if ((updates & 1) && _rx_msg != UBX_MSG_NAV_PVT && _rx_msg != UBX_MSG_NAV_POSLLH &&
        _rx_msg != UBX_MSG_NAV_HPPOSLLH && _rx_msg != UBX_MSG_NAV_VELNED)
        return (updates & ~1) | GPSDecodedBatch::PROTOCOL_ACTIVITY;
    return updates;
}

int GPSDriverUBX::decodeByte(uint8_t byte)
{
    return parseChar(byte);
}
