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

#include "UBXMessageSchema.h"
#include "UBXPrivate.h"

void GPSDriverUBX::handleConfigurationReadback()
{
    if (!_configuration_readback_pending || _rx_payload_length < 4) {
        return;
    }
    const std::span<const uint8_t> payload(_framePayload.data(), _rx_payload_length);
    if (payload[0] != 1 || payload[1] != 0 || payload[2] != 0 || payload[3] != 0) {
        return;
    }
    uint16_t seen = 0;
    uint32_t values[9]{};
    for (unsigned offset = 4; offset < _rx_payload_length;) {
        if (_rx_payload_length - offset < 4) {
            return;
        }
        const auto key = GPSWire::read<uint32_t>(payload, offset).value_or(0);
        offset += 4;
        const unsigned code = key >> 28;
        if (code < 1 || code > 4) {
            return;
        }
        const unsigned width = code <= 2 ? 1 : 1u << (code - 2);
        if (_rx_payload_length - offset < width) {
            return;
        }
        unsigned index = 0;
        while (index < _configuration_readback_count && _configuration_readback_keys[index] != key) {
            ++index;
        }
        if (index == _configuration_readback_count || (seen & (1u << index))) {
            return;
        }
        values[index] = width == 1   ? GPSWire::read<uint8_t>(payload, offset).value_or(0)
                        : width == 2 ? GPSWire::read<uint16_t>(payload, offset).value_or(0)
                                     : GPSWire::read<uint32_t>(payload, offset).value_or(0);
        offset += width;
        if (code == 1 && values[index] > 1) {
            return;
        }
        seen |= 1u << index;
    }
    if (seen != (1u << _configuration_readback_count) - 1) {
        return;
    }
    memcpy(_configuration_readback_values, values, sizeof(values));
    _configuration_readback_ready = true;
}

int  // 0 = decoding, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::parseChar(const uint8_t b)
{
    int ret = 0;

    // A framed payload owns its bytes; embedded preambles belong to that payload.
    if (_rtcm_parsing && (_decode_state == UBX_DECODE_SYNC1 || _rtcm_parsing->hasPartialFrame())) {
        const bool complete = _rtcm_parsing->addByte(b);
        if (complete) {
            if (_rtcm_parsing->valid())
                gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            _rtcm_parsing->reset();
            return ret;
        }
        if (_rtcm_parsing->hasPartialFrame())
            return ret;
    }

    switch (_decode_state) {
        /* Expecting Sync1 */
        case UBX_DECODE_SYNC1:
            if (b == UBX_SYNC1) {  // Sync1 found --> expecting Sync2

                _decode_state = UBX_DECODE_SYNC2;
            }

            break;

        /* Expecting Sync2 */
        case UBX_DECODE_SYNC2:
            if (b == UBX_SYNC2) {  // Sync2 found --> expecting Class

                _decode_state = UBX_DECODE_CLASS;

            } else {  // Sync1 not followed by Sync2: reset parser
                decodeInit();
            }

            break;

        /* Expecting Class */
        case UBX_DECODE_CLASS:

            addByteToChecksum(b);  // checksum is calculated for everything except Sync and Checksum bytes
            _rx_msg = b;
            _decode_state = UBX_DECODE_ID;
            break;

        /* Expecting ID */
        case UBX_DECODE_ID:

            addByteToChecksum(b);
            _rx_msg |= b << 8;
            _decode_state = UBX_DECODE_LENGTH1;
            break;

        /* Expecting first length byte */
        case UBX_DECODE_LENGTH1:

            addByteToChecksum(b);
            _rx_payload_length = b;
            _decode_state = UBX_DECODE_LENGTH2;
            break;

        /* Expecting second length byte */
        case UBX_DECODE_LENGTH2:

            addByteToChecksum(b);
            _rx_payload_length |= b << 8;  // calculate payload size

            _framePayloadIndex = 0;
            _decode_state = _rx_payload_length ? UBX_DECODE_PAYLOAD : UBX_DECODE_CHKSUM1;
            break;

        case UBX_DECODE_PAYLOAD:
            addByteToChecksum(b);
            if (_framePayloadIndex < _framePayload.size()) {
                _framePayload[_framePayloadIndex] = b;
            }
            if (++_framePayloadIndex == _rx_payload_length) {
                _decode_state = UBX_DECODE_CHKSUM1;
            }
            break;

        /* Expecting first checksum byte */
        case UBX_DECODE_CHKSUM1:
            if (_rx_ck_a != b) {
                decodeInit();

            } else {
                _decode_state = UBX_DECODE_CHKSUM2;
            }

            break;

        /* Expecting second checksum byte */
        case UBX_DECODE_CHKSUM2:
            if (_rx_ck_b != b) {
            } else {
                ret = decodeValidatedPayload();

                if (_rtcm_parsing) {
                    _rtcm_parsing->reset();
                }
            }

            decodeInit();
            break;

        default:
            break;
    }

    return ret;
}

int  // -1 = abort, 0 = continue
GPSDriverUBX::payloadRxInit()
{
    int ret = 0;

    _rx_state = UBX_RXMSG_HANDLE;  // handle by default

    switch (_rx_msg) {
        case UBX_MSG_CFG_VALGET:
            if (!_configuration_readback_pending) {
                _rx_state = UBX_RXMSG_IGNORE;
            } else if (_rx_payload_length < 4 || _rx_payload_length > sizeof(_buf)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;
            }
            break;
        case UBX_MSG_MON_COMMS:
            if (_rx_payload_length < 8 || _rx_payload_length > sizeof(ubx_payload_rx_mon_comms_t) ||
                (_rx_payload_length - 8) % sizeof(ubx_payload_rx_mon_comms_port_t) != 0) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;
            }

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
            if (_rx_payload_length >= sizeof(ubx_buf_t)) {
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
            if ((_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx6_t))    /* u-blox 6 msg format */
                && (_rx_payload_length != sizeof(ubx_payload_rx_mon_hw_ubx7_t)) /* u-blox 7+ msg format */
                && (_rx_payload_length != UBX::MON_HW_DEPRECATED_SIZE)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_MON_RF:
            if (_rx_payload_length < sizeof(ubx_payload_rx_mon_rf_t) ||
                (_rx_payload_length - 4) % sizeof(ubx_payload_rx_mon_rf_t::ubx_payload_rx_mon_rf_block_t) != 0) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_SEC_SIG:
            if (_rx_payload_length < 4) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;
            }

            break;

        case UBX_MSG_RXM_RTCM:
            if (_rx_payload_length != sizeof(ubx_payload_rx_rxm_rtcm_t)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;  // ignore if not _decodeNavigation
            }

            break;

        case UBX_MSG_RXM_COR:
            if (_rx_payload_length != sizeof(ubx_payload_rx_rxm_cor_t)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (!_decodeNavigation) {
                _rx_state = UBX_RXMSG_IGNORE;
            }

            break;

        case UBX_MSG_ACK_ACK:
            if (_rx_payload_length != sizeof(ubx_payload_rx_ack_ack_t)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (_ack_state != UBX_ACK_WAITING) {
                _rx_state = UBX_RXMSG_IGNORE;  // No command is awaiting acknowledgement.
            }

            break;

        case UBX_MSG_ACK_NAK:
            if (_rx_payload_length != sizeof(ubx_payload_rx_ack_nak_t)) {
                _rx_state = UBX_RXMSG_ERROR_LENGTH;

            } else if (_ack_state != UBX_ACK_WAITING) {
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
            GPS_WARN("ubx internal err1");
            ret = -1;  // return error, abort handling this message
            break;
    }

    return ret;
}

void GPSDriverUBX::decodeNavSat(std::span<const uint8_t> payload)
{
    const auto header = UBX::payload<ubx_payload_rx_nav_sat_part1_t>(payload);
    _satellite_info->count = std::min<size_t>(header.numSvs, _satellite_info->entries.size());
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS, GPSConstellation::NavIC};
    for (size_t index = 0; index < _satellite_info->count; ++index) {
        const auto wire = UBX::payload<ubx_payload_rx_nav_sat_part2_t>(
            payload, sizeof(header) + index * sizeof(ubx_payload_rx_nav_sat_part2_t));
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
    const auto header = UBX::payload<ubx_payload_rx_nav_svinfo_part1_t>(payload);
    _satellite_info->count = std::min<size_t>(header.numCh, _satellite_info->entries.size());
    for (size_t index = 0; index < _satellite_info->count; ++index) {
        const auto wire = UBX::payload<ubx_payload_rx_nav_svinfo_part2_t>(
            payload, sizeof(header) + index * sizeof(ubx_payload_rx_nav_svinfo_part2_t));
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
    _buf.payload_rx_mon_ver_part1 = UBX::payload<ubx_payload_rx_mon_ver_part1_t>(payload);
    // The protocol specifies these as nul-terminated strings, but the terminator comes
    // from the device, so enforce it before anything walks the field.
    _buf.payload_rx_mon_ver_part1.swVersion[sizeof(_buf.payload_rx_mon_ver_part1.swVersion) - 1] = 0;
    _buf.payload_rx_mon_ver_part1.hwVersion[sizeof(_buf.payload_rx_mon_ver_part1.hwVersion) - 1] = 0;
    memcpy(_firmware_version, _buf.payload_rx_mon_ver_part1.swVersion, sizeof(_firmware_version));

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
        if (strncmp((const char*) _buf.payload_rx_mon_ver_part1.hwVersion, known_board.hw_version,
                    sizeof(_buf.payload_rx_mon_ver_part1.hwVersion)) == 0) {
            _board = known_board.board;
            known = true;
            break;
        }
    }

    if (!known) {
        GPS_WARN("unknown board hw: %s", _buf.payload_rx_mon_ver_part1.hwVersion);
    }
    for (size_t offset = sizeof(ubx_payload_rx_mon_ver_part1_t); offset < payload.size();
         offset += sizeof(ubx_payload_rx_mon_ver_part2_t)) {
        _buf.payload_rx_mon_ver_part2 = UBX::payload<ubx_payload_rx_mon_ver_part2_t>(payload, offset);
        // Part 2 complete: decode Part 2 buffer
        // Same as above: the protocol specifies a nul-terminated string, the device provides
        // the terminator, so enforce it before strstr() walks the field.
        _buf.payload_rx_mon_ver_part2.extension[sizeof(_buf.payload_rx_mon_ver_part2.extension) - 1] = 0;

        // "FWVER=" Firmware of product category and version
        const char* fwver_str = strstr((const char*) _buf.payload_rx_mon_ver_part2.extension, "FWVER=");

        if (fwver_str != nullptr) {
            strncpy(_firmware_version, fwver_str + strlen("FWVER="), sizeof(_firmware_version) - 1);
            _firmware_version[sizeof(_firmware_version) - 1] = '\0';
            GPS_INFO("u-blox firmware version: %s", fwver_str + strlen("FWVER="));

            // Check if its a ZED-F9P-15B
            if ((_board == Board::u_blox9) && strstr(fwver_str, "HPGL1L5")) {
                _board = Board::u_blox9_F9P_L1L5;
            }
        }

        // "PROTVER=" Supported protocol version.
        const char* protver_str = strstr((const char*) _buf.payload_rx_mon_ver_part2.extension, "PROTVER=");

        if (protver_str != nullptr) {
            GPS_INFO("u-blox protocol version: %s", protver_str + strlen("PROTVER="));
        }

        // "MOD=" Module identification. Set in production.
        const char* mod_str = strstr((const char*) _buf.payload_rx_mon_ver_part2.extension, "MOD=");

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

            GPS_INFO("u-blox module: %s", mod_str + strlen("MOD="));
        }
    }
}

int  // 0 = no message handled, 1 = message handled, 2 = sat info message handled
GPSDriverUBX::payloadRxDone()
{
    int ret = 0;

    // return if no message handled
    if (_rx_state != UBX_RXMSG_HANDLE) {
        return ret;
    }

    // handle message
    switch (_rx_msg) {
        case UBX_MSG_NAV_PVT:

            // Check if position fix flag is good
            if ((_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_GNSSFIXOK) == 1) {
                _gps_position->fix_type = _buf.payload_rx_nav_pvt.fixType;

                if (_buf.payload_rx_nav_pvt.flags & UBX_RX_NAV_PVT_FLAGS_DIFFSOLN) {
                    _gps_position->fix_type = 4;  // DGPS
                }

                uint8_t carr_soln = _buf.payload_rx_nav_pvt.flags >> 6;

                if (carr_soln == 1) {
                    _gps_position->fix_type = 5;  // Float RTK

                } else if (carr_soln == 2) {
                    _gps_position->fix_type = 6;  // Fixed RTK
                }

                _gps_position->vel_ned_valid = true;

            } else {
                _gps_position->fix_type = 0;
                _gps_position->vel_ned_valid = false;
            }

            _gps_position->satellites_used = _buf.payload_rx_nav_pvt.numSV;

            if (_assembleEpochs ? !_epochHasHighPrecision : _gps_position->fix_type < 6) {
                // When RTK is active and solid (fix=6), these values will be filled by HPPOSLLH:
                _gps_position->latitude_deg = _buf.payload_rx_nav_pvt.lat * UBX::DEGREES_PER_COORDINATE;
                _gps_position->longitude_deg = _buf.payload_rx_nav_pvt.lon * UBX::DEGREES_PER_COORDINATE;
                _gps_position->altitude_msl_m = _buf.payload_rx_nav_pvt.hMSL * 1e-3;
                _gps_position->altitude_ellipsoid_m = _buf.payload_rx_nav_pvt.height * 1e-3;

                _gps_position->eph = static_cast<float>(_buf.payload_rx_nav_pvt.hAcc) * 1e-3f;
                _gps_position->accuracy_timestamp = nowUs();
                _gps_position->epv = static_cast<float>(_buf.payload_rx_nav_pvt.vAcc) * 1e-3f;

                _got_posllh = true;
            }

            _gps_position->speedAccuracyMetersPerSecond = static_cast<float>(_buf.payload_rx_nav_pvt.sAcc) * 1e-3f;

            _gps_position->vel_m_s = static_cast<float>(_buf.payload_rx_nav_pvt.gSpeed) * 1e-3f;

            _gps_position->vel_n_m_s = static_cast<float>(_buf.payload_rx_nav_pvt.velN) * 1e-3f;
            _gps_position->vel_e_m_s = static_cast<float>(_buf.payload_rx_nav_pvt.velE) * 1e-3f;
            _gps_position->vel_d_m_s = static_cast<float>(_buf.payload_rx_nav_pvt.velD) * 1e-3f;

            _gps_position->cog_rad = static_cast<float>(_buf.payload_rx_nav_pvt.headMot) * M_DEG_TO_RAD_F * 1e-5f;
            _gps_position->courseAccuracyRadians =
                static_cast<float>(_buf.payload_rx_nav_pvt.headAcc) * M_DEG_TO_RAD_F * 1e-5f;

            // Check if time and date fix flags are good
            if ((_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDDATE) &&
                (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_VALIDTIME) &&
                (_buf.payload_rx_nav_pvt.valid & UBX_RX_NAV_PVT_VALID_FULLYRESOLVED)) {
                tm timeinfo{};
                timeinfo.tm_year = _buf.payload_rx_nav_pvt.year - 1900;
                timeinfo.tm_mon = _buf.payload_rx_nav_pvt.month - 1;
                timeinfo.tm_mday = _buf.payload_rx_nav_pvt.day;
                timeinfo.tm_hour = _buf.payload_rx_nav_pvt.hour;
                timeinfo.tm_min = _buf.payload_rx_nav_pvt.min;
                timeinfo.tm_sec = _buf.payload_rx_nav_pvt.sec;
                _gps_position->time_utc_usec = timeFromUtc(timeinfo, _buf.payload_rx_nav_pvt.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                _gps_position->time_utc_usec = 0;
            }

            _gps_position->timestamp = nowUs();
            _last_timestamp_time = _gps_position->timestamp;

            _got_velned = true;

            ret = 1;
            break;

        case UBX_MSG_INF_DEBUG:
        case UBX_MSG_INF_NOTICE: {
            uint8_t* p_buf = _framePayload.data();
            p_buf[_rx_payload_length] = 0;

        } break;

        case UBX_MSG_INF_ERROR:
        case UBX_MSG_INF_WARNING: {
            uint8_t* p_buf = _framePayload.data();
            p_buf[_rx_payload_length] = 0;
            GPS_WARN("ubx msg: %s", p_buf);

            if (strncmp(reinterpret_cast<const char*>(p_buf), "txbuf", 5) == 0) {
                _comms_request_pending = true;
            }
        } break;

        case UBX_MSG_MON_COMMS:
            logCommsDiagnostics();
            break;
        case UBX_MSG_CFG_VALGET:
            handleConfigurationReadback();
            break;

        case UBX_MSG_NAV_POSLLH:

            _gps_position->latitude_deg = _buf.payload_rx_nav_posllh.lat * UBX::DEGREES_PER_COORDINATE;
            _gps_position->longitude_deg = _buf.payload_rx_nav_posllh.lon * UBX::DEGREES_PER_COORDINATE;
            _gps_position->altitude_msl_m = _buf.payload_rx_nav_posllh.hMSL * 1e-3;
            _gps_position->altitude_ellipsoid_m = _buf.payload_rx_nav_posllh.height * 1e-3;
            _gps_position->eph = static_cast<float>(_buf.payload_rx_nav_posllh.hAcc) * 1e-3f;  // from mm to m
            _gps_position->accuracy_timestamp = nowUs();
            _gps_position->epv = static_cast<float>(_buf.payload_rx_nav_posllh.vAcc) * 1e-3f;  // from mm to m

            _gps_position->timestamp = nowUs();

            _got_posllh = true;

            ret = 1;
            break;

        case UBX_MSG_NAV_HPPOSLLH:

            if (_buf.payload_rx_nav_hpposllh.flags == 0 && (_assembleEpochs || _gps_position->fix_type == 6)) {
                _gps_position->latitude_deg = _buf.payload_rx_nav_hpposllh.lat * UBX::DEGREES_PER_COORDINATE +
                                              _buf.payload_rx_nav_hpposllh.latHp *
                                                  1e-9;  // regular precision lat/lon (1e7), plus high precision (1e9)
                _gps_position->longitude_deg = _buf.payload_rx_nav_hpposllh.lon * UBX::DEGREES_PER_COORDINATE +
                                               _buf.payload_rx_nav_hpposllh.lonHp * 1e-9;
                _gps_position->altitude_msl_m =
                    _buf.payload_rx_nav_hpposllh.hMSL * 1e-3 +
                    _buf.payload_rx_nav_hpposllh.hMSLHp *
                        1e-4;  // regular precision altitude, mm, plus high precision components of altitude, 0.1 mm
                _gps_position->altitude_ellipsoid_m =
                    _buf.payload_rx_nav_hpposllh.height * 1e-3 + _buf.payload_rx_nav_hpposllh.heightHp * 1e-4;
                _gps_position->eph = static_cast<float>(_buf.payload_rx_nav_hpposllh.hAcc) *
                                     1e-4f;  // Accuracy estimates, convert from 0.1 mm to m
                _gps_position->epv = static_cast<float>(_buf.payload_rx_nav_hpposllh.vAcc) * 1e-4f;
                _gps_position->accuracy_timestamp = nowUs();

                _gps_position->timestamp = nowUs();

                _got_posllh = true;

                ret = 1;
            }

            break;

        case UBX_MSG_NAV_SOL:

            _gps_position->fix_type = _buf.payload_rx_nav_sol.gpsFix;
            _gps_position->speedAccuracyMetersPerSecond =
                static_cast<float>(_buf.payload_rx_nav_sol.sAcc) * 1e-2f;  // from cm to m
            _gps_position->satellites_used = _buf.payload_rx_nav_sol.numSV;

            ret = 1;
            break;

        case UBX_MSG_NAV_STATUS:

            _gps_position->spoofing_state =
                (_buf.payload_rx_nav_status.flags2 & UBX_RX_NAV_STATUS_SPOOFDETSTATE_MASK) >>
                UBX_RX_NAV_STATUS_SPOOFDETSTATE_SHIFT;
            _gps_position->spoofing_state_timestamp = nowUs();

            ret = 1;
            break;

        case UBX_MSG_NAV_DOP:

            _gps_position->hdop = _buf.payload_rx_nav_dop.hDOP * UBX::DOP_PER_UNIT;
            _gps_position->dop_timestamp = nowUs();
            _gps_position->vdop = _buf.payload_rx_nav_dop.vDOP * UBX::DOP_PER_UNIT;

            ret = 1;
            break;

        case UBX_MSG_NAV_TIMEUTC:

            if (_buf.payload_rx_nav_timeutc.valid & UBX_RX_NAV_TIMEUTC_VALID_VALIDUTC) {
                tm timeinfo{};
                timeinfo.tm_year = _buf.payload_rx_nav_timeutc.year - 1900;
                timeinfo.tm_mon = _buf.payload_rx_nav_timeutc.month - 1;
                timeinfo.tm_mday = _buf.payload_rx_nav_timeutc.day;
                timeinfo.tm_hour = _buf.payload_rx_nav_timeutc.hour;
                timeinfo.tm_min = _buf.payload_rx_nav_timeutc.min;
                timeinfo.tm_sec = _buf.payload_rx_nav_timeutc.sec;
                _gps_position->time_utc_usec = timeFromUtc(timeinfo, _buf.payload_rx_nav_timeutc.nano);

            } else {
                // The struct is reused across messages, so without this a receiver
                // that lost time in a reset keeps reporting the last time it knew.
                // 0 is the defined "unavailable" value.
                _gps_position->time_utc_usec = 0;
            }

            _last_timestamp_time = nowUs();

            ret = 1;
            break;

        case UBX_MSG_NAV_SAT:
        case UBX_MSG_NAV_SVINFO:

            // Satellite entries were decoded from the validated payload.
            _satellite_info->timestamp = nowUs();

            ret = 2;
            break;

        case UBX_MSG_NAV_SVIN:

        {
            ubx_payload_rx_nav_svin_t& svin = _buf.payload_rx_nav_svin;
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

        case UBX_MSG_NAV_VELNED:

            _gps_position->vel_m_s = static_cast<float>(_buf.payload_rx_nav_velned.gSpeed) * 1e-2f;
            _gps_position->vel_n_m_s =
                static_cast<float>(_buf.payload_rx_nav_velned.velN) * 1e-2f;  // NED NORTH velocity
            _gps_position->vel_e_m_s =
                static_cast<float>(_buf.payload_rx_nav_velned.velE) * 1e-2f;  // NED EAST velocity
            _gps_position->vel_d_m_s =
                static_cast<float>(_buf.payload_rx_nav_velned.velD) * 1e-2f;  // NED DOWN velocity
            _gps_position->cog_rad = static_cast<float>(_buf.payload_rx_nav_velned.heading) * M_DEG_TO_RAD_F * 1e-5f;
            _gps_position->courseAccuracyRadians =
                static_cast<float>(_buf.payload_rx_nav_velned.cAcc) * M_DEG_TO_RAD_F * 1e-5f;
            _gps_position->vel_ned_valid = true;

            _got_velned = true;

            ret = 1;
            break;

        case UBX_MSG_NAV_RELPOSNED:

        {
            const float rel_length_cm =
                _buf.payload_rx_nav_relposned.relPosLength + _buf.payload_rx_nav_relposned.relPosHPLength * 1e-2f;
            const float rel_length_m = rel_length_cm * 1e-2f;  // cm -> m
            const uint32_t flags = _buf.payload_rx_nav_relposned.flags;
            const bool heading_valid_flag = flags & (1 << 8);
            const bool rel_pos_valid = flags & (1 << 2);
            const bool carrier_solution_fixed = flags & (1 << 4);

            const bool heading_qualified = heading_valid_flag && rel_pos_valid &&
                                           (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

            float heading_rad = NAN;
            float heading_acc_rad = NAN;

            if (heading_qualified) {
                heading_rad = relPosHeadingToYaw(_buf.payload_rx_nav_relposned.relPosHeading);
                heading_acc_rad = _buf.payload_rx_nav_relposned.accHeading * M_DEG_TO_RAD_F * 1e-5f;
            }

            _gps_position->heading = heading_rad;
            _gps_position->heading_timestamp = nowUs();
            _gps_position->heading_accuracy = heading_acc_rad;

            GPSRelativeReport gps_rel{};

            gps_rel.timestamp_sample = nowUs();  // TODO: adjust with delay estimate

            gps_rel.time_utc_usec =
                uint64_t(_buf.payload_rx_nav_relposned.iTOW) * 1000;  // TODO: convert iTOW ms GPS time of week
            gps_rel.reference_station_id = _buf.payload_rx_nav_relposned.refStationId;

            gps_rel.position[0] =
                (_buf.payload_rx_nav_relposned.relPosN + _buf.payload_rx_nav_relposned.relPosHPN * 1e-2f) * 1e-2f;
            gps_rel.position[1] =
                (_buf.payload_rx_nav_relposned.relPosE + _buf.payload_rx_nav_relposned.relPosHPE * 1e-2f) * 1e-2f;
            gps_rel.position[2] =
                (_buf.payload_rx_nav_relposned.relPosD + _buf.payload_rx_nav_relposned.relPosHPD * 1e-2f) * 1e-2f;

            gps_rel.position_length = rel_length_m;

            gps_rel.heading = heading_rad;
            gps_rel.heading_accuracy = heading_acc_rad;

            gps_rel.position_accuracy[0] = _buf.payload_rx_nav_relposned.accN * 1e-4f;  // 0.1mm -> m
            gps_rel.position_accuracy[1] = _buf.payload_rx_nav_relposned.accE * 1e-4f;  // 0.1mm -> m
            gps_rel.position_accuracy[2] = _buf.payload_rx_nav_relposned.accD * 1e-4f;  // 0.1mm -> m

            gps_rel.accuracy_length = _buf.payload_rx_nav_relposned.accLength * 1e-4f;  // 0.1mm -> m;

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

        case UBX_MSG_NAV_DAHEADING:

        {
            const float rel_length_m = _buf.payload_rx_nav_daheading.relPosLength * 1e-3f;  // mm -> m
            const uint32_t flags = _buf.payload_rx_nav_daheading.flags;
            const bool heading_valid_flag = flags & (1 << 6);                               // bit 8 in NAV-RELPOSNED
            const bool rel_pos_valid = flags & (1 << 2);
            const bool carrier_solution_fixed = flags & (1 << 4);

            const bool heading_qualified = heading_valid_flag && rel_pos_valid &&
                                           (rel_length_m < UBX_HEADING_MAX_BASELINE_M) && carrier_solution_fixed;

            float heading_rad = NAN;
            float heading_acc_rad = NAN;

            if (heading_qualified) {
                heading_rad = relPosHeadingToYaw(_buf.payload_rx_nav_daheading.relPosHeading);
                heading_acc_rad = _buf.payload_rx_nav_daheading.accHeading * M_DEG_TO_RAD_F * 1e-5f;
            }

            _gps_position->heading = heading_rad;
            _gps_position->heading_timestamp = nowUs();
            _gps_position->heading_accuracy = heading_acc_rad;

            GPSRelativeReport gps_rel{};

            gps_rel.timestamp_sample = nowUs();

            // time_utc_usec is left at 0 (documented as unavailable): NAV-DAHEADING only carries
            // iTOW, which cannot be converted to UTC without the week number and leap seconds.

            gps_rel.position[0] = _buf.payload_rx_nav_daheading.relPosN * 1e-3f;  // mm -> m
            gps_rel.position[1] = _buf.payload_rx_nav_daheading.relPosE * 1e-3f;
            gps_rel.position[2] = _buf.payload_rx_nav_daheading.relPosD * 1e-3f;

            gps_rel.position_length = rel_length_m;

            gps_rel.heading = heading_rad;
            gps_rel.heading_accuracy = heading_acc_rad;

            gps_rel.position_accuracy[0] = _buf.payload_rx_nav_daheading.accN * 1e-3f;
            gps_rel.position_accuracy[1] = _buf.payload_rx_nav_daheading.accE * 1e-3f;
            gps_rel.position_accuracy[2] = _buf.payload_rx_nav_daheading.accD * 1e-3f;

            gps_rel.accuracy_length = _buf.payload_rx_nav_daheading.accLength * 1e-3f;

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

        case UBX_MSG_MON_VER:

            // This is polled only on startup, and the startup code waits for an ack
            if (_ack_state == UBX_ACK_WAITING && _ack_waiting_msg == UBX_MSG_MON_VER) {
                _ack_state = UBX_ACK_GOT_ACK;
            }

            ret = 1;
            break;

        case UBX_MSG_MON_HW:

            switch (_rx_payload_length) {
                case sizeof(ubx_payload_rx_mon_hw_ubx6_t): /* u-blox 6 msg format */
                    _gps_position->noise_per_ms = _buf.payload_rx_mon_hw_ubx6.noisePerMS;
                    _gps_position->automatic_gain_control = _buf.payload_rx_mon_hw_ubx6.agcCnt;
                    _gps_position->jamming_indicator = _buf.payload_rx_mon_hw_ubx6.jamInd;
                    _gps_position->rf_timestamp = nowUs();

                    ret = 1;
                    break;

                case sizeof(ubx_payload_rx_mon_hw_ubx7_t): /* u-blox 7+ msg format */
                    _gps_position->noise_per_ms = _buf.payload_rx_mon_hw_ubx7.noisePerMS;
                    _gps_position->automatic_gain_control = _buf.payload_rx_mon_hw_ubx7.agcCnt;
                    _gps_position->jamming_indicator = _buf.payload_rx_mon_hw_ubx7.jamInd;
                    _gps_position->rf_timestamp = nowUs();

                    ret = 1;
                    break;

                case UBX::MON_HW_DEPRECATED_SIZE: /* u-blox 27+ deprecated, ignore */
                    ret = 0;
                    break;

                default:      // unexpected payload size:
                    ret = 0;  // don't handle message
                    break;
            }

            break;

        case UBX_MSG_MON_RF:

            // TODO: only block 0 is read. F9P reports 2 blocks, X20 3, each with its own noisePerMS,
            // agcCnt and cwSuppression (jamInd). cwSuppression is the CW notch in effect per front end,
            // i.e. the per-frequency mitigation state GNSS_BANDS wants; the block covering a SEC-SIG
            // center frequency comes from rfBlockGnssBand (HPG 2.10) or blockId on older firmware.
            _gps_position->noise_per_ms = _buf.payload_rx_mon_rf.block[0].noisePerMS;
            _gps_position->automatic_gain_control = _buf.payload_rx_mon_rf.block[0].agcCnt;
            _gps_position->jamming_indicator = _buf.payload_rx_mon_rf.block[0].jamInd;
            _gps_position->rf_timestamp = nowUs();

            if (!_got_sec_sig) {
                _gps_position->jamming_state = _buf.payload_rx_mon_rf.block[0].flags & 0x03;
                _gps_position->jamming_state_timestamp = nowUs();
            }

            ret = 1;
            break;

        case UBX_MSG_SEC_SIG:

        {
            const uint8_t version = _buf.payload_rx_sec_sig.version;
            uint8_t flag_byte;

            if (version == 1) {
                if (_rx_payload_length < 5) {
                    ret = 0;
                    break;
                }

                flag_byte = _buf.payload_rx_sec_sig.jamFlags;

            } else {
                flag_byte = _buf.payload_rx_sec_sig.flags;
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

            _gps_position->jamming_state = jamming_state;
            _gps_position->jamming_state_timestamp = nowUs();
            _got_sec_sig = true;

            // TODO: v2/v3 carry jamNumCentFreqs X4 groups after the header (bits 23..0 centFreq in
            // kHz, bit 24 jammed), one per in-use band. Not parsed: sensor_gps has nowhere to put
            // per-band state until the GNSS_BANDS message from mavlink/rfcs#30 lands, at which
            // point both the RX struct and payloadRxInit() length check need the repeated group.
        }

            ret = 1;
            break;

        case UBX_MSG_RXM_RTCM:

            _gps_position->corrections_timestamp = nowUs();
            _gps_position->corrections_protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_RTCM3;
            _gps_position->corrections_crc_failed =
                (_buf.payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_CRCFAILED_MASK) != 0;
            _gps_position->corrections_msg_used =
                (_buf.payload_rx_rxm_rtcm.flags & UBX_RX_RXM_RTCM_MSGUSED_MASK) >> UBX_RX_RXM_RTCM_MSGUSED_SHIFT;

            ret = 1;
            break;

        case UBX_MSG_RXM_COR:

        {
            const uint32_t status = _buf.payload_rx_rxm_cor.statusInfo;
            uint8_t protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_UNKNOWN;

            switch (status & UBX_RX_RXM_COR_PROTOCOL_MASK) {
                case 1:
                    protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_RTCM3;
                    break;

                case 2:
                    protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_SPARTN;
                    break;

                case 5:
                    protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_HAS;
                    break;

                case 29:
                    protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_PMP;
                    break;

                case 30:
                    protocol = GPSPositionReport::CORRECTIONS_PROTOCOL_QZSS_L6;
                    break;
            }

            _gps_position->corrections_timestamp = nowUs();
            _gps_position->corrections_protocol = protocol;
            _gps_position->corrections_crc_failed =
                ((status & UBX_RX_RXM_COR_ERRSTATUS_MASK) >> UBX_RX_RXM_COR_ERRSTATUS_SHIFT) == 2;
            _gps_position->corrections_msg_used =
                (status & UBX_RX_RXM_COR_MSGUSED_MASK) >> UBX_RX_RXM_COR_MSGUSED_SHIFT;
        }

            ret = 1;
            break;

        case UBX_MSG_ACK_ACK:

            if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
                _ack_state = UBX_ACK_GOT_ACK;
            }

            ret = 1;
            break;

        case UBX_MSG_ACK_NAK:

            if ((_ack_state == UBX_ACK_WAITING) && (_buf.payload_rx_ack_ack.msg == _ack_waiting_msg)) {
                _ack_state = UBX_ACK_GOT_NAK;
            }

            ret = 1;
            break;

        default:
            break;
    }

    if (ret > 0) {
        _gps_position->timestamp_time_relative = (int32_t) (_last_timestamp_time - _gps_position->timestamp);
    }

    return ret;
}

void GPSDriverUBX::logCommsDiagnostics()
{
    if (_comms_poll_deadline == 0 || nowUs() > _comms_poll_deadline) {
        return;
    }

    const auto& status = _buf.payload_rx_mon_comms;

    if (status.version != 0 || status.nPorts > UBX_MON_COMMS_MAX_PORTS ||
        _rx_payload_length != 8 + status.nPorts * sizeof(ubx_payload_rx_mon_comms_port_t)) {
        return;
    }

    _comms_poll_deadline = 0;
    GPS_WARN("MON-COMMS after txbuf: txErrors=0x%02x ports=%u (snapshot after warning)", (unsigned) status.txErrors,
             (unsigned) status.nPorts);

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

        GPS_WARN(
            "MON-COMMS %s port=0x%04x txPending=%u txUsage=%u%% txPeakUsage=%u%% "
            "rxPending=%u rxUsage=%u%% overrunErrs=%u skipped=%lu",
            name, (unsigned) port.portId, (unsigned) port.txPending, (unsigned) port.txUsage,
            (unsigned) port.txPeakUsage, (unsigned) port.rxPending, (unsigned) port.rxUsage,
            (unsigned) port.overrunErrs, (unsigned long) port.skipped);
    }
}

void GPSDriverUBX::decodeInit()
{
    _decode_state = UBX_DECODE_SYNC1;
    _rx_ck_a = 0;
    _rx_ck_b = 0;
    _rx_payload_length = 0;
}

float GPSDriverUBX::relPosHeadingToYaw(int32_t heading) const
{
    float heading_rad = heading * M_DEG_TO_RAD_F * 1e-5f;

    // Normalize to [-pi, pi]
    if (heading_rad > M_PI_F) {
        heading_rad -= 2.f * M_PI_F;

    } else if (heading_rad < -M_PI_F) {
        heading_rad += 2.f * M_PI_F;
    }

    return heading_rad;
}

void GPSDriverUBX::addByteToChecksum(const uint8_t b)
{
    _rx_ck_a = _rx_ck_a + b;
    _rx_ck_b = _rx_ck_b + _rx_ck_a;
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
    GPSPositionReport* output = _gps_position;
    if (_assembleEpochs && timed) {
        const size_t offset = schema->towOffset;
        const auto tow = GPSWire::read<uint32_t>(_framePayload, offset).value_or(0);
        epoch = _navigationEpochs.find(tow, nowUs(), publish);
        if (!epoch)
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        _gps_position = &epoch->position;
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
            _buf = {};
            switch (_rx_msg) {
                case UBX_MSG_NAV_POSLLH:
                    _buf.payload_rx_nav_posllh = UBX::payload<ubx_payload_rx_nav_posllh_t>(payload);
                    break;
                case UBX_MSG_NAV_DOP:
                    _buf.payload_rx_nav_dop = UBX::payload<ubx_payload_rx_nav_dop_t>(payload);
                    break;
                case UBX_MSG_NAV_SOL:
                    _buf.payload_rx_nav_sol = UBX::payload<ubx_payload_rx_nav_sol_t>(payload);
                    break;
                case UBX_MSG_NAV_PVT:
                    _buf.payload_rx_nav_pvt = UBX::payload<ubx_payload_rx_nav_pvt_t>(payload);
                    break;
                case UBX_MSG_NAV_TIMEUTC:
                    _buf.payload_rx_nav_timeutc = UBX::payload<ubx_payload_rx_nav_timeutc_t>(payload);
                    break;
                case UBX_MSG_NAV_STATUS:
                    _buf.payload_rx_nav_status = UBX::payload<ubx_payload_rx_nav_status_t>(payload);
                    break;
                case UBX_MSG_NAV_SVIN:
                    _buf.payload_rx_nav_svin = UBX::payload<ubx_payload_rx_nav_svin_t>(payload);
                    break;
                case UBX_MSG_NAV_VELNED:
                    _buf.payload_rx_nav_velned = UBX::payload<ubx_payload_rx_nav_velned_t>(payload);
                    break;
                case UBX_MSG_SEC_SIG:
                    _buf.payload_rx_sec_sig = UBX::payload<ubx_payload_rx_sec_sig_t>(payload);
                    break;
                case UBX_MSG_RXM_RTCM:
                    _buf.payload_rx_rxm_rtcm = UBX::payload<ubx_payload_rx_rxm_rtcm_t>(payload);
                    break;
                case UBX_MSG_RXM_COR:
                    _buf.payload_rx_rxm_cor = UBX::payload<ubx_payload_rx_rxm_cor_t>(payload);
                    break;
                case UBX_MSG_NAV_RELPOSNED:
                    _buf.payload_rx_nav_relposned = UBX::payload<ubx_payload_rx_nav_relposned_t>(payload);
                    break;
                case UBX_MSG_NAV_DAHEADING:
                    _buf.payload_rx_nav_daheading = UBX::payload<ubx_payload_rx_nav_daheading_t>(payload);
                    break;
                case UBX_MSG_NAV_HPPOSLLH:
                    _buf.payload_rx_nav_hpposllh = UBX::payload<ubx_payload_rx_nav_hpposllh_t>(payload);
                    break;
                case UBX_MSG_MON_COMMS:
                    _buf.payload_rx_mon_comms = UBX::payload<ubx_payload_rx_mon_comms_t>(payload);
                    break;
                case UBX_MSG_MON_RF:
                    _buf.payload_rx_mon_rf = UBX::payload<ubx_payload_rx_mon_rf_t>(payload);
                    break;
                case UBX_MSG_ACK_ACK:
                    _buf.payload_rx_ack_ack = UBX::payload<ubx_payload_rx_ack_ack_t>(payload);
                    break;
                case UBX_MSG_ACK_NAK:
                    _buf.payload_rx_ack_nak = UBX::payload<ubx_payload_rx_ack_nak_t>(payload);
                    break;
                case UBX_MSG_MON_HW:
                    if (payload.size() == sizeof(ubx_payload_rx_mon_hw_ubx6_t))
                        _buf.payload_rx_mon_hw_ubx6 = UBX::payload<ubx_payload_rx_mon_hw_ubx6_t>(payload);
                    else if (payload.size() == sizeof(ubx_payload_rx_mon_hw_ubx7_t))
                        _buf.payload_rx_mon_hw_ubx7 = UBX::payload<ubx_payload_rx_mon_hw_ubx7_t>(payload);
                    break;
                default:
                    break;
            }
            break;
    }
    const int updates = payloadRxDone();
    _gps_position = output;
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
