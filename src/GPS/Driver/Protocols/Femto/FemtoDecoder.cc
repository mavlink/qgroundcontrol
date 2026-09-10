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
        memcpy(&_femto_uav_gps, _femto_msg.data, sizeof(femto_uav_gps_t));

        _gps_position->time_utc_usec = _femto_uav_gps.time_utc_usec;
        _gps_position->latitude_deg = _femto_uav_gps.lat / 1e7;
        _gps_position->longitude_deg = _femto_uav_gps.lon / 1e7;
        _gps_position->altitude_msl_m = _femto_uav_gps.alt / 1e3;
        _gps_position->altitude_ellipsoid_m = _femto_uav_gps.alt_ellipsoid / 1e3;
        _gps_position->s_variance_m_s = _femto_uav_gps.s_variance_m_s;
        _gps_position->c_variance_rad = _femto_uav_gps.c_variance_rad;
        _gps_position->eph = _femto_uav_gps.eph;
        _gps_position->epv = _femto_uav_gps.epv;
        _gps_position->hdop = _femto_uav_gps.hdop;
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

        } else {
            _gps_position->heading = NAN;
        }

        _gps_position->timestamp = nowUs();

        ret = 1;

    } else if (_satellite_info && messageid == FEMTO_MSG_ID_UAVSTATUS) { /**< set satellite info */
        if (_femto_msg.header.femto_header.messagelength < offsetof(femto_uav_status_t, sat_status)) {
            return 0;
        }
        const femto_uav_status_t* uav_status = (const femto_uav_status_t*) _femto_msg.data;
        if (uav_status->sat_number > sizeof(uav_status->sat_status) / sizeof(uav_status->sat_status[0]) ||
            _femto_msg.header.femto_header.messagelength <
                offsetof(femto_uav_status_t, sat_status) + uav_status->sat_number * sizeof(uav_status->sat_status[0])) {
            return 0;
        }

        _satellite_info->timestamp = nowUs();
        _satellite_info->count = MIN(uav_status->sat_number, GPSSatelliteReport::SAT_INFO_MAX_SATELLITES);

        for (size_t i = 0; i < _satellite_info->count; i++) {
            _satellite_info->entries[i].id = uav_status->sat_status[i].svid;
            _satellite_info->entries[i].used.reset();
            _satellite_info->entries[i].elevation = uav_status->sat_status[i].ele;
            _satellite_info->entries[i].azimuth = uav_status->sat_status[i].azi;
            _satellite_info->entries[i].signal = uav_status->sat_status[i].cn0;
            _satellite_info->entries[i].prn = uav_status->sat_status[i].svid;
        }

        ret = 2;

    } else if (OutputMode::RTCM == _output_mode && messageid == FEMTO_MSG_ID_GPGGA &&
               (memcmp(_femto_msg.data + 3, "GGA,", 3) == 0)) { /**< GPGGA only used in base station, for survey-in */
        int uiCalcComma = 0;

        for (int i = 0; i < len; i++) {
            if (_femto_msg.data[i] == ',') {
                uiCalcComma++;
            }
        }

        if (uiCalcComma == 14) {
            NMEAFields::Cursor bufptr(
                {reinterpret_cast<const char*>(_femto_msg.data + 7), static_cast<size_t>(len - 7)});
            double ashtech_time = 0.0, lat = 0.0, lon = 0.0, alt = 0.0;
            int num_of_sv = 0, fix_quality = 0;
            double hdop = 99.9;
            char ns = '?', ew = '?';

            bufptr.read(ashtech_time);

            if (!bufptr.valid())
                return 0;

            if (!bufptr.read(lat))
                return 0;

            if (!bufptr.valid())
                return 0;

            bufptr.read(ns);

            if (!bufptr.valid())
                return 0;

            if (!bufptr.read(lon))
                return 0;

            if (!bufptr.valid())
                return 0;

            bufptr.read(ew);

            if (!bufptr.valid())
                return 0;

            bufptr.read(fix_quality);

            if (!bufptr.valid())
                return 0;

            bufptr.read(num_of_sv);

            if (!bufptr.valid())
                return 0;

            bufptr.read(hdop);

            if (!bufptr.valid())
                return 0;

            if (!bufptr.read(alt))
                alt = NAN;

            if (!bufptr.valid())
                return 0;

            if (ns == 'S') {
                lat = -lat;
            }

            if (ew == 'W') {
                lon = -lon;
            }

            FEMTO_UNUSED(ashtech_time)
            FEMTO_UNUSED(hdop)

            if (!_correction_output_activated && 7 == fix_quality) {
                _survey_in_start = 0; /**< finished survey-in */

                lat = nmeaToDegrees(lat) * 10000000;
                lon = nmeaToDegrees(lon) * 10000000;
                alt = alt * 1000;

                sendSurveyInStatusUpdate(false, true, lat, lon, (float) alt);
                _rtcmActivationPending = true;
            }

            if (_satellite_info) {
                _satellite_info->count = 0;
                _satellite_info->usedCount = num_of_sv;
                _satellite_info->timestamp = nowUs(); /**< base station satellite count */
            }

            ret = 2;
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

int GPSDriverFemto::consume(std::span<const uint8_t> bytes)
{
    int handled = 0;
    for (const uint8_t byte : bytes) {
        const int length = parseChar(byte);
        if (length > 0) {
            handled |= handleMessage(length);
        }
    }
    return handled;
}
