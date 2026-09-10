/****************************************************************************
 *
 *   Copyright (c) 2018-2024 PX4 Development Team. All rights reserved.
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

#include "SBFPrivate.h"

int GPSDriverSBF::parseChar(const uint8_t b)
{
    int ret = 0;

    if (_rtcm_parsing) {
        if (_rtcm_parsing->addByte(b) && _rtcm_parsing->valid()) {
            gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            decodeInit();
            _rtcm_parsing->reset();
            return 0b0100;  // ret
        }
    }

    switch (_decode_state) {
        // Expecting Sync1
        case SBF_DECODE_SYNC1:
            if (b == SBF_SYNC1) {  // Sync1 found --> expecting Sync2
                SBF_TRACE_PARSER("A");
                payloadRxAdd(b);   // add a payload byte
                _decode_state = SBF_DECODE_SYNC2;
            }

            break;

        // Expecting Sync2
        case SBF_DECODE_SYNC2:
            if (b == SBF_SYNC2) {  // Sync2 found --> expecting CRC
                SBF_TRACE_PARSER("B");
                payloadRxAdd(b);   // add a payload byte
                _decode_state = SBF_DECODE_PAYLOAD;

            } else {  // Sync1 not followed by Sync2: reset parser
                decodeInit();
            }

            break;

        // Expecting payload
        case SBF_DECODE_PAYLOAD:
            SBF_TRACE_PARSER(".");

            ret = payloadRxAdd(b);  // add a payload byte

            if (ret < 0) {
                // payload not handled, discard message
                ret = 0;
                decodeInit();

            } else if (ret > 0) {
                ret = payloadRxDone();  // finish payload processing

                if (_rtcm_parsing) {
                    _rtcm_parsing->reset();
                }

                decodeInit();

            } else {
                // expecting more payload, stay in state SBF_DECODE_PAYLOAD
                ret = 0;
            }

            break;

        default:
            break;
    }

    return ret;
}

int GPSDriverSBF::payloadRxAdd(const uint8_t b)
{
    int ret = 0;
    uint8_t* p_buf = reinterpret_cast<uint8_t*>(&_buf);

    p_buf[_rx_payload_index++] = b;

    if ((_rx_payload_index > 7 && _rx_payload_index >= _buf.length) || _rx_payload_index >= sizeof(_buf)) {
        ret = 1;  // payload received completely
    }

    return ret;
}

uint16_t crc16(const uint8_t* data_p, uint32_t length)
{
    uint8_t x;
    uint16_t crc = 0;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = static_cast<uint16_t>((crc << 8) ^ (x << 12) ^ (x << 5) ^ x);
    }

    return crc;
}

int GPSDriverSBF::payloadRxDone()
{
    int ret = 0;
#ifndef NO_MKTIME
    struct tm timeinfo;
    time_t epoch;
#endif

    if (_buf.length <= 4 || _buf.length > _rx_payload_index ||
        _buf.crc16 != crc16(reinterpret_cast<uint8_t*>(&_buf) + 4, _buf.length - 4)) {
        SBF_TRACE_RXMSG("Rx Unknow");
        return 0;
    }

    size_t requiredLength = 0;
    switch (_buf.msg_id) {
        case SBF_ID_PVTGeodetic:
            requiredLength = offsetof(sbf_buf_t, payload_pvt_geodetic) + sizeof(_buf.payload_pvt_geodetic);
            break;
        case SBF_ID_VelCovGeodetic:
            requiredLength = offsetof(sbf_buf_t, payload_vel_col_geodetic) + sizeof(_buf.payload_vel_col_geodetic);
            break;
        case SBF_ID_DOP:
            requiredLength = offsetof(sbf_buf_t, payload_dop) + sizeof(_buf.payload_dop);
            break;
        case SBF_ID_AttEuler:
            requiredLength = offsetof(sbf_buf_t, payload_att_euler) + sizeof(_buf.payload_att_euler);
            break;
        case SBF_ID_AttCovEuler:
            requiredLength = offsetof(sbf_buf_t, payload_att_cov_euler) + sizeof(_buf.payload_att_cov_euler);
            break;
        default:
            return 0;
    }
    if (_buf.length < requiredLength) {
        return 0;
    }

    // handle message
    switch (_buf.msg_id) {
        case SBF_ID_PVTGeodetic: {
            SBF_TRACE_RXMSG("Rx PVTGeodetic");
            _msg_status |= 1;

            if (_buf.payload_pvt_geodetic.mode_type < 1) {
                _gps_position->fix_type = 1;

            } else {
                switch (_buf.payload_pvt_geodetic.mode_type) {
                    case 6:
                        _gps_position->fix_type = 4;
                        break;

                    case 5:
                    case 8:
                        _gps_position->fix_type = 5;
                        break;

                    case 4:
                    case 7:
                        _gps_position->fix_type = 6;
                        break;

                    default:
                        _gps_position->fix_type = 3;
                        break;
                }
            }

            // Check fix and error code
            _gps_position->vel_ned_valid = _gps_position->fix_type > 1 && _buf.payload_pvt_geodetic.error == 0;

            // Check boundaries and invalidate GPS velocities
            // We're not just checking for the do-not-use value (-2*10^10) but for any value beyond the specified max
            // values
            if (fabsf(_buf.payload_pvt_geodetic.vn) > 600.0f || fabsf(_buf.payload_pvt_geodetic.ve) > 600.0f ||
                fabsf(_buf.payload_pvt_geodetic.vu) > 600.0f) {
                _gps_position->vel_ned_valid = false;
            }

            // Check boundaries and invalidate position
            // We're not just checking for the do-not-use value (-2*10^10) but for any value beyond the specified max
            // values
            if (fabs(_buf.payload_pvt_geodetic.latitude) > (double) (M_PI_F / 2.0f) ||
                fabs(_buf.payload_pvt_geodetic.longitude) > (double) M_PI_F ||
                fabs(_buf.payload_pvt_geodetic.height) > DNU ||
                fabsf(_buf.payload_pvt_geodetic.undulation) > (float) DNU) {
                _gps_position->fix_type = 0;
            }

            if (_buf.payload_pvt_geodetic.nr_sv < 255) {  // 255 = do not use value
                _gps_position->satellites_used = _buf.payload_pvt_geodetic.nr_sv;

                if (_satellite_info) {
                    publishSatelliteUsage(_gps_position->satellites_used);
                }

            } else {
                _gps_position->satellites_used = 0;
            }

            _gps_position->latitude_deg = _buf.payload_pvt_geodetic.latitude * M_RAD_TO_DEG;
            _gps_position->longitude_deg = _buf.payload_pvt_geodetic.longitude * M_RAD_TO_DEG;
            _gps_position->altitude_ellipsoid_m = _buf.payload_pvt_geodetic.height;
            _gps_position->altitude_msl_m =
                _buf.payload_pvt_geodetic.height - static_cast<double>(_buf.payload_pvt_geodetic.undulation);

            /* H and V accuracy are reported in 2DRMS, but based off the uBlox reporting we expect RMS.
             * Devide by 100 from cm to m and in addition divide by 2 to get RMS. */
            _gps_position->eph = static_cast<float>(_buf.payload_pvt_geodetic.h_accuracy) / 200.0f;
            _gps_position->accuracy_timestamp = nowUs();
            _gps_position->epv = static_cast<float>(_buf.payload_pvt_geodetic.v_accuracy) / 200.0f;

            _gps_position->vel_n_m_s = static_cast<float>(_buf.payload_pvt_geodetic.vn);
            _gps_position->vel_e_m_s = static_cast<float>(_buf.payload_pvt_geodetic.ve);
            _gps_position->vel_d_m_s = -1.0f * static_cast<float>(_buf.payload_pvt_geodetic.vu);
            _gps_position->vel_m_s = sqrtf(_gps_position->vel_n_m_s * _gps_position->vel_n_m_s +
                                           _gps_position->vel_e_m_s * _gps_position->vel_e_m_s);

            const float course = _buf.payload_pvt_geodetic.cog;
            _gps_position->cog_rad =
                std::isfinite(course) && course >= 0.0f && course <= 360.0f ? course * M_DEG_TO_RAD_F : NAN;
            _gps_position->c_variance_rad = 1.0f * M_DEG_TO_RAD_F;

            _gps_position->time_utc_usec = 0;
#ifndef NO_MKTIME
            /* convert to unix timestamp */
            memset(&timeinfo, 0, sizeof(timeinfo));

            timeinfo.tm_year = 1980 - 1900;
            timeinfo.tm_mon = 0;
            timeinfo.tm_mday = 6 + _buf.WNc * 7;
            timeinfo.tm_hour = 0;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = _buf.TOW / 1000;

            epoch = gpsTimeToEpoch(timeinfo);

            if (epoch > GPS_EPOCH_SECS) {
                _gps_position->time_utc_usec = static_cast<uint64_t>(epoch) * 1000000ULL;
                _gps_position->time_utc_usec += (_buf.TOW % 1000) * 1000;
            }

#endif
            _gps_position->timestamp = nowUs();
            _last_timestamp_time = _gps_position->timestamp;
            ret |= (_msg_status == 7) ? 1 : 0;

            // In RTCM mode, PVTGeodetic is used to get base station survey-in
            if (_output_mode == OutputMode::RTCM) {
                GPSSurveyReport status{};
                status.accuracyKnown = true;
                status.altitudeDatum = GPSSurveyReport::AltitudeDatum::Ellipsoid;
                status.latitude = _gps_position->latitude_deg;
                status.longitude = _gps_position->longitude_deg;
                status.altitude = _gps_position->altitude_ellipsoid_m;
                status.duration = _survey_active ? (float) (nowUs() - _survey_activation_date) / 1000000.0f : 0;
                status.mean_accuracy =
                    (_buf.payload_pvt_geodetic.h_accuracy + _buf.payload_pvt_geodetic.v_accuracy) / 20;  // Value in mm
                status.flags = (_buf.payload_pvt_geodetic.mode_type > 0 ? 1 : 0) | (_survey_active & 1) << 1;
                surveyInStatus(status);
                ret |= 4;  // RTCM infos have been updated
            }

            //
            break;
        }

        case SBF_ID_VelCovGeodetic:
            SBF_TRACE_RXMSG("Rx VelCovGeodetic");
            _msg_status |= 2;
            _gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_ve_ve;

            if (_gps_position->s_variance_m_s < _buf.payload_vel_col_geodetic.cov_vn_vn) {
                _gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_vn_vn;
            }

            if (_gps_position->s_variance_m_s < _buf.payload_vel_col_geodetic.cov_vu_vu) {
                _gps_position->s_variance_m_s = _buf.payload_vel_col_geodetic.cov_vu_vu;
            }

            _gps_position->s_variance_m_s =
                _gps_position->s_variance_m_s >= 0 ? std::sqrt(_gps_position->s_variance_m_s) : NAN;
            //
            break;

        case SBF_ID_DOP:
            SBF_TRACE_RXMSG("Rx DOP");
            _msg_status |= 4;
            _gps_position->hdop = _buf.payload_dop.hDOP * 0.01f;
            _gps_position->dop_timestamp = nowUs();
            _gps_position->vdop = _buf.payload_dop.vDOP * 0.01f;
            //
            break;

        case SBF_ID_AttEuler:
            SBF_TRACE_RXMSG("Rx AttEuler");

            if (!_buf.payload_att_euler.error_not_requested) {
                int error_aux1 = _buf.payload_att_euler.error_aux1;
                int error_aux2 = _buf.payload_att_euler.error_aux2;

                //
                if (error_aux1 == 0 && error_aux2 == 0) {
                    float heading = _buf.payload_att_euler.heading;
                    heading *= M_PI_F / 180.0f;  // deg to rad, now in range [0, 2pi]

                    if (heading > M_PI_F) {
                        heading -= 2.f * M_PI_F;  // final range is [-pi, pi]
                    }

                    _gps_position->heading = heading;
                    _gps_position->heading_timestamp = nowUs();
                    //
                    //

                } else if (error_aux1 != 0) {
                    //
                } else if (error_aux2 != 0) {
                    //
                }
            } else {
                //
            }

            break;

        case SBF_ID_AttCovEuler:
            SBF_TRACE_RXMSG("Rx AttCovEuler");

            if (!_buf.payload_att_cov_euler.error_not_requested) {
                int error_aux1 = _buf.payload_att_cov_euler.error_aux1;
                int error_aux2 = _buf.payload_att_cov_euler.error_aux2;

                if (error_aux1 == 0 && error_aux2 == 0) {
                    float heading_acc = _buf.payload_att_cov_euler.cov_headhead;
                    heading_acc *= M_PI_F / 180.0f;  // deg to rad, now in range [0, 2pi]
                    _gps_position->heading_accuracy = heading_acc;
                    //
                    //

                } else if (error_aux1 != 0) {
                    //
                } else if (error_aux2 != 0) {
                    //
                }
            } else {
                //
            }

            break;

        default:
            SBF_TRACE_RXMSG("Rx other.");
            break;
    }

    if (ret > 0) {
        _gps_position->timestamp_time_relative = static_cast<int32_t>(_last_timestamp_time - _gps_position->timestamp);
    }

    if (ret == 1) {
        _msg_status &= ~1;
    }

    return ret;
}

void GPSDriverSBF::decodeInit()
{
    _decode_state = SBF_DECODE_SYNC1;
    _rx_payload_index = 0;
}

int GPSDriverSBF::decodeByte(uint8_t byte)
{
    return parseChar(byte);
}
