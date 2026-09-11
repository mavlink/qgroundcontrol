/****************************************************************************
 *
 *   Copyright (c) 2012-2018 PX4 Development Team. All rights reserved.
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

#include "FemtoPrivate.h"
#include "GPSWire.h"
#include "NMEASentence.h"

namespace {
inline femto_uav_gps_t decodePosition(std::span<const uint8_t> bytes)
{
    femto_uav_gps_t value{};
    value.time_utc_usec = GPSWire::read<uint64_t>(bytes, 0).value_or(0);
    value.lat = GPSWire::read<int32_t>(bytes, 8).value_or(0);
    value.lon = GPSWire::read<int32_t>(bytes, 12).value_or(0);
    value.alt = GPSWire::read<int32_t>(bytes, 16).value_or(0);
    value.alt_ellipsoid = GPSWire::read<int32_t>(bytes, 20).value_or(0);
    value.s_variance_m_s = GPSWire::read<float>(bytes, 24).value_or(0);
    value.c_variance_rad = GPSWire::read<float>(bytes, 28).value_or(0);
    value.eph = GPSWire::read<float>(bytes, 32).value_or(0);
    value.epv = GPSWire::read<float>(bytes, 36).value_or(0);
    value.hdop = GPSWire::read<float>(bytes, 40).value_or(0);
    value.vdop = GPSWire::read<float>(bytes, 44).value_or(0);
    value.noise_per_ms = GPSWire::read<int32_t>(bytes, 48).value_or(0);
    value.jamming_indicator = GPSWire::read<int32_t>(bytes, 52).value_or(0);
    value.vel_m_s = GPSWire::read<float>(bytes, 56).value_or(0);
    value.vel_n_m_s = GPSWire::read<float>(bytes, 60).value_or(0);
    value.vel_e_m_s = GPSWire::read<float>(bytes, 64).value_or(0);
    value.vel_d_m_s = GPSWire::read<float>(bytes, 68).value_or(0);
    value.cog_rad = GPSWire::read<float>(bytes, 72).value_or(0);
    value.timestamp_time_relative = GPSWire::read<int32_t>(bytes, 76).value_or(0);
    value.heading = GPSWire::read<float>(bytes, 80).value_or(0);
    value.fix_type = GPSWire::read<uint8_t>(bytes, 84).value_or(0);
    value.vel_ned_valid = GPSWire::read<uint8_t>(bytes, 85).value_or(0);
    value.satellites_used = GPSWire::read<uint8_t>(bytes, 86).value_or(0);
    value.heading_type = GPSWire::read<uint8_t>(bytes, 87).value_or(0);
    return value;
}
}  // namespace

int GPSDriverFemto::handleMessage(int len)
{
    int ret = 0;
    if (_output_mode != OutputMode::RTCM &&
        (len < _femto_msg.header.femto_header.headerlength ||
         _femto_msg.header.femto_header.messagelength > len - _femto_msg.header.femto_header.headerlength)) {
        return 0;
    }
    uint16_t messageid = _femto_msg.header.femto_header.messageid;

    if (messageid == FEMTO_MSG_ID_UAVGPS) { /**< uavgpsB*/
        if (_femto_msg.header.femto_header.messagelength < sizeof(femto_uav_gps_t)) {
            return 0;
        }
        _femto_uav_gps = decodePosition({_femto_msg.data, _femto_msg.header.femto_header.messagelength});

        _gps_position->time_utc_usec = _femto_uav_gps.time_utc_usec;
        _gps_position->latitude_deg = _femto_uav_gps.lat / 1e7;
        _gps_position->longitude_deg = _femto_uav_gps.lon / 1e7;
        _gps_position->altitude_msl_m = _femto_uav_gps.alt / 1e3;
        _gps_position->altitude_ellipsoid_m = _femto_uav_gps.alt_ellipsoid / 1e3;
        _gps_position->speedAccuracyMetersPerSecond = _femto_uav_gps.s_variance_m_s;
        _gps_position->courseAccuracyRadians = _femto_uav_gps.c_variance_rad;
        _gps_position->eph = _femto_uav_gps.eph;
        _gps_position->accuracy_timestamp = nowUs();
        _gps_position->epv = _femto_uav_gps.epv;
        _gps_position->hdop = _femto_uav_gps.hdop;
        _gps_position->dop_timestamp = nowUs();
        _gps_position->vdop = _femto_uav_gps.vdop;
        _gps_position->noise_per_ms = _femto_uav_gps.noise_per_ms;
        _gps_position->jamming_indicator = _femto_uav_gps.jamming_indicator;
        _gps_position->rf_timestamp = nowUs();
        _gps_position->vel_m_s = _femto_uav_gps.vel_m_s;
        _gps_position->vel_n_m_s = _femto_uav_gps.vel_n_m_s;
        _gps_position->vel_e_m_s = _femto_uav_gps.vel_e_m_s;
        _gps_position->vel_d_m_s = _femto_uav_gps.vel_d_m_s;
        _gps_position->cog_rad = _femto_uav_gps.cog_rad;
        _gps_position->timestamp_time_relative = _femto_uav_gps.timestamp_time_relative;
        _gps_position->fix_type = _femto_uav_gps.fix_type;
        _gps_position->vel_ned_valid = _femto_uav_gps.vel_ned_valid;
        _gps_position->satellites_used = _femto_uav_gps.satellites_used;

        if (_femto_uav_gps.heading_type == 6) {
            float heading = _femto_uav_gps.heading;
            heading *= M_PI_F / 180.0f;  // deg to rad, now in range [0, 2pi]
            heading -= _heading_offset;  // range: [-pi, 3pi]

            if (heading > M_PI_F) {
                heading -= 2.f * M_PI_F;  // final range is [-pi, pi]
            }

            _gps_position->heading = heading;
            _gps_position->heading_timestamp = nowUs();

        } else {
            _gps_position->heading = NAN;
            _gps_position->heading_timestamp = nowUs();
        }

        _gps_position->timestamp = nowUs();

        ret = 1;

    } else if (_satellite_info && messageid == FEMTO_MSG_ID_UAVSTATUS) { /**< set satellite info */
        if (_femto_msg.header.femto_header.messagelength < offsetof(femto_uav_status_t, sat_status)) {
            return 0;
        }
        const std::span<const uint8_t> status(_femto_msg.data, _femto_msg.header.femto_header.messagelength);
        const auto count = GPSWire::read<uint32_t>(status, 36).value_or(0);
        if (count > 64 || status.size() < 40 + count * 8) {
            return 0;
        }

        _satellite_info->timestamp = nowUs();
        _satellite_info->count = MIN(count, GPSSatelliteReport::SAT_INFO_MAX_SATELLITES);

        for (size_t i = 0; i < _satellite_info->count; i++) {
            _satellite_info->entries[i].id = GPSWire::read<uint8_t>(status, 40 + i * 8 + 0).value_or(0);
            _satellite_info->entries[i].used.reset();
            _satellite_info->entries[i].elevation = GPSWire::read<uint8_t>(status, 40 + i * 8 + 3).value_or(0);
            _satellite_info->entries[i].azimuth = GPSWire::read<uint16_t>(status, 40 + i * 8 + 4).value_or(0);
            _satellite_info->entries[i].signal = GPSWire::read<uint8_t>(status, 40 + i * 8 + 2).value_or(0);
            _satellite_info->entries[i].prn = GPSWire::read<uint8_t>(status, 40 + i * 8 + 0).value_or(0);
        }

        ret = 2;

    } else if (OutputMode::RTCM == _output_mode && messageid == FEMTO_MSG_ID_GPGGA &&
               (memcmp(_femto_msg.data + 3, "GGA,", 3) == 0)) { /**< GPGGA only used in base station, for survey-in */
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_femto_msg.data), static_cast<size_t>(len)});
        const auto fix = parsed ? NMEA::gga(*parsed) : std::nullopt;
        if (!fix)
            return 0;
        if (!_correction_output_activated && fix->quality == 7) {
            _survey_in_start = 0;
            sendSurveyInStatusUpdate(false, true, fix->latitude, fix->longitude, fix->altitude);
            _rtcmActivationPending = true;
        }
        if (_satellite_info && fix->satellitesUsed) {
            publishSatelliteUsage(*fix->satellitesUsed);
        }
    }

    // handle survey-in status update
    if (_survey_in_start != 0) {
        const gps_abstime now = nowUs();
        uint32_t survey_in_duration = (now - _survey_in_start) / 1000000;

        if (survey_in_duration != _survey_duration) {
            _survey_duration = survey_in_duration;
            sendSurveyInStatusUpdate(true, false);
        }
    }

    return ret;
}

int GPSDriverFemto::parseChar(uint8_t temp)
{
    int iRet = 0;

    if (_rtcm_parsing) {
        if (_rtcm_parsing->addByte(temp) && _rtcm_parsing->valid()) {
            gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            decodeInit();
            _rtcm_parsing->reset();
            return iRet;
        }
    }

    if (_output_mode == OutputMode::GPS) {
        switch (_decode_state) {
            case FemtoDecodeState::pream_ble1:
                if (temp == FEMTO_PREAMBLE1) {
                    _decode_state = FemtoDecodeState::pream_ble2;
                    _femto_msg.read = 0;
                }

                break;

            case FemtoDecodeState::pream_ble2:
                if (temp == FEMTO_PREAMBLE2) {
                    _decode_state = FemtoDecodeState::pream_ble3;

                } else {
                    _decode_state = FemtoDecodeState::pream_ble1;
                }

                break;

            case FemtoDecodeState::pream_ble3:
                if (temp == FEMTO_PREAMBLE3) {
                    _decode_state = FemtoDecodeState::head_length;

                } else {
                    _decode_state = FemtoDecodeState::pream_ble1;
                }

                break;

            case FemtoDecodeState::head_length:
                if (temp != sizeof(_femto_msg.header.femto_header)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }
                _femto_msg.header.data[0] = FEMTO_PREAMBLE1;
                _femto_msg.header.data[1] = FEMTO_PREAMBLE2;
                _femto_msg.header.data[2] = FEMTO_PREAMBLE3;
                _femto_msg.header.data[3] = temp;
                _femto_msg.header.femto_header.headerlength = temp;
                _decode_state = FemtoDecodeState::head_data;
                _femto_msg.read = 4;
                break;

            case FemtoDecodeState::head_data:
                if (_femto_msg.read >= sizeof(_femto_msg.header.data)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }

                _femto_msg.header.data[_femto_msg.read] = temp;
                _femto_msg.read++;

                if (_femto_msg.read >= _femto_msg.header.femto_header.headerlength) {
                    _femto_msg.header.femto_header.messageid =
                        GPSWire::read<uint16_t>(_femto_msg.header.data, 4).value_or(0);
                    _femto_msg.header.femto_header.messagelength =
                        GPSWire::read<uint16_t>(_femto_msg.header.data, 8).value_or(0);
                    if (_femto_msg.header.femto_header.messagelength > sizeof(_femto_msg.data)) {
                        _decode_state = FemtoDecodeState::pream_ble1;
                    } else {
                        _decode_state = _femto_msg.header.femto_header.messagelength == 0 ? FemtoDecodeState::crc1
                                                                                          : FemtoDecodeState::data;
                    }
                }

                break;

            case FemtoDecodeState::data:
                if (static_cast<size_t>(_femto_msg.read - _femto_msg.header.femto_header.headerlength) >=
                    sizeof(_femto_msg.data)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }

                _femto_msg.data[_femto_msg.read - _femto_msg.header.femto_header.headerlength] = temp;
                _femto_msg.read++;

                if (_femto_msg.read >=
                    (_femto_msg.header.femto_header.messagelength + _femto_msg.header.femto_header.headerlength)) {
                    _decode_state = FemtoDecodeState::crc1;
                }

                break;

            case FemtoDecodeState::crc1:
                _femto_msg.crc = (uint32_t(temp) << 0);
                _decode_state = FemtoDecodeState::crc2;
                break;

            case FemtoDecodeState::crc2:
                _femto_msg.crc += (uint32_t(temp) << 8);
                _decode_state = FemtoDecodeState::crc3;
                break;

            case FemtoDecodeState::crc3:
                _femto_msg.crc += (uint32_t(temp) << 16);
                _decode_state = FemtoDecodeState::crc4;
                break;

            case FemtoDecodeState::crc4: {
                _femto_msg.crc += (uint32_t(temp) << 24);
                _decode_state = FemtoDecodeState::pream_ble1;

                uint32_t crc = QGC::crc32Update({_femto_msg.header.data, _femto_msg.header.femto_header.headerlength});
                crc = QGC::crc32Update({_femto_msg.data, _femto_msg.header.femto_header.messagelength}, crc);

                if (_femto_msg.crc == crc) {
                    iRet = _femto_msg.read;

                } else {
                }
            } break;

            default:
                break;
        }

    } else { /**< RTCM mode */

        switch (_decode_state) {
            case FemtoDecodeState::pream_ble1:
                if (temp == '$') {
                    _decode_state = FemtoDecodeState::pream_nmea_got_sync1;
                    _femto_msg.read = 0;
                    _femto_msg.data[_femto_msg.read++] = temp;
                }

                break;

            case FemtoDecodeState::pream_nmea_got_sync1:
                if (temp == '$') {
                    _decode_state = FemtoDecodeState::pream_nmea_got_sync1;
                    _femto_msg.read = 0;

                } else if (temp == '*') {
                    _decode_state = FemtoDecodeState::pream_nmea_got_asteriks;
                }

                if (_femto_msg.read >= (sizeof(_femto_msg.data) - 5)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    _femto_msg.read = 0;

                } else {
                    _femto_msg.data[_femto_msg.read++] = temp;
                }

                break;

            case FemtoDecodeState::pream_nmea_got_asteriks:
                _femto_msg.data[_femto_msg.read++] = temp;
                _decode_state = FemtoDecodeState::pream_nmea_got_first_cs_byte;
                break;

            case FemtoDecodeState::pream_nmea_got_first_cs_byte: {
                _femto_msg.data[_femto_msg.read++] = temp;
                uint8_t checksum = 0;
                uint8_t* buffer = _femto_msg.data + 1;
                uint8_t* bufend = _femto_msg.data + _femto_msg.read - 3;

                for (; buffer < bufend; buffer++) {
                    checksum ^= *buffer;
                }

                if ((NMEAFields::hexDigit(checksum >> 4) == *(_femto_msg.data + _femto_msg.read - 2)) &&
                    (NMEAFields::hexDigit(checksum & 0x0F) == *(_femto_msg.data + _femto_msg.read - 1))) {
                    iRet = _femto_msg.read;
                    _femto_msg.header.femto_header.messageid = FEMTO_MSG_ID_GPGGA;

                    if (_rtcm_parsing) {
                        _rtcm_parsing->reset();
                    }
                }

                decodeInit();
                break;
            }

            default:
                break;
        }
    }

    return iRet;
}

void GPSDriverFemto::decodeInit()
{
    _decode_state = FemtoDecodeState::pream_ble1;
}

void GPSDriverFemto::sendSurveyInStatusUpdate(bool active, bool valid, double latitude, double longitude,
                                              float altitude)
{
    GPSSurveyReport status;
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = altitude;
    status.duration = !_baseConfig.useFixedBase ? _survey_duration : 0;
    status.mean_accuracy = 0;  // unknown
    status.flags = (int) valid | ((int) active << 1);
    surveyInStatus(status);
}

int GPSDriverFemto::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    return length > 0 ? handleMessage(length) : 0;
}
