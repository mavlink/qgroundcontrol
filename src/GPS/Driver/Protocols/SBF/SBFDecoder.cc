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

#include "GPSWire.h"
#include "SBFPrivate.h"

namespace {
sbf_buf_t decodeBlock(std::span<const uint8_t> bytes)
{
    sbf_buf_t value{};
    value.sync = GPSWire::read<uint16_t>(bytes, 0).value_or(0);
    value.crc16 = GPSWire::read<uint16_t>(bytes, 2).value_or(0);
    const auto id = GPSWire::read<uint16_t>(bytes, 4).value_or(0);
    value.msg_id = id & 0x1fff;
    value.msg_revision = id >> 13;
    value.length = GPSWire::read<uint16_t>(bytes, 6).value_or(0);
    value.TOW = GPSWire::read<uint32_t>(bytes, 8).value_or(0);
    value.WNc = GPSWire::read<uint16_t>(bytes, 12).value_or(0);
    switch (value.msg_id) {
        case SBF_ID_PVTGeodetic:
            value.payload_pvt_geodetic.mode_type = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 0) & 15;
            value.payload_pvt_geodetic.mode_reserved = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 4) & 3;
            value.payload_pvt_geodetic.mode_base_fixed = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 6) & 1;
            value.payload_pvt_geodetic.mode_2d = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 7) & 1;
            value.payload_pvt_geodetic.error = GPSWire::read<uint8_t>(bytes, 15).value_or(0);
            value.payload_pvt_geodetic.latitude = GPSWire::read<double>(bytes, 16).value_or(0);
            value.payload_pvt_geodetic.longitude = GPSWire::read<double>(bytes, 24).value_or(0);
            value.payload_pvt_geodetic.height = GPSWire::read<double>(bytes, 32).value_or(0);
            value.payload_pvt_geodetic.undulation = GPSWire::read<float>(bytes, 40).value_or(0);
            value.payload_pvt_geodetic.vn = GPSWire::read<float>(bytes, 44).value_or(0);
            value.payload_pvt_geodetic.ve = GPSWire::read<float>(bytes, 48).value_or(0);
            value.payload_pvt_geodetic.vu = GPSWire::read<float>(bytes, 52).value_or(0);
            value.payload_pvt_geodetic.cog = GPSWire::read<float>(bytes, 56).value_or(0);
            value.payload_pvt_geodetic.rx_clk_bias = GPSWire::read<double>(bytes, 60).value_or(0);
            value.payload_pvt_geodetic.RxClkDrift = GPSWire::read<float>(bytes, 68).value_or(0);
            value.payload_pvt_geodetic.time_system = GPSWire::read<uint8_t>(bytes, 72).value_or(0);
            value.payload_pvt_geodetic.datum = GPSWire::read<uint8_t>(bytes, 73).value_or(0);
            value.payload_pvt_geodetic.nr_sv = GPSWire::read<uint8_t>(bytes, 74).value_or(0);
            value.payload_pvt_geodetic.wa_corr_info = GPSWire::read<uint8_t>(bytes, 75).value_or(0);
            value.payload_pvt_geodetic.reference_id = GPSWire::read<uint16_t>(bytes, 76).value_or(0);
            value.payload_pvt_geodetic.mean_corr_age = GPSWire::read<uint16_t>(bytes, 78).value_or(0);
            value.payload_pvt_geodetic.signal_info = GPSWire::read<uint32_t>(bytes, 80).value_or(0);
            value.payload_pvt_geodetic.alert_flag = GPSWire::read<uint8_t>(bytes, 84).value_or(0);
            value.payload_pvt_geodetic.nr_bases = GPSWire::read<uint8_t>(bytes, 85).value_or(0);
            value.payload_pvt_geodetic.ppp_info = GPSWire::read<uint16_t>(bytes, 86).value_or(0);
            value.payload_pvt_geodetic.latency = GPSWire::read<uint16_t>(bytes, 88).value_or(0);
            value.payload_pvt_geodetic.h_accuracy = GPSWire::read<uint16_t>(bytes, 90).value_or(0);
            value.payload_pvt_geodetic.v_accuracy = GPSWire::read<uint16_t>(bytes, 92).value_or(0);
            break;
        case SBF_ID_VelCovGeodetic:
            value.payload_vel_col_geodetic.mode_type = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 0) & 15;
            value.payload_vel_col_geodetic.mode_reserved = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 4) & 3;
            value.payload_vel_col_geodetic.mode_base_fixed = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 6) & 1;
            value.payload_vel_col_geodetic.mode_2d = (GPSWire::read<uint8_t>(bytes, 14).value_or(0) >> 7) & 1;
            value.payload_vel_col_geodetic.error = GPSWire::read<uint8_t>(bytes, 15).value_or(0);
            value.payload_vel_col_geodetic.cov_vn_vn = GPSWire::read<float>(bytes, 16).value_or(0);
            value.payload_vel_col_geodetic.cov_ve_ve = GPSWire::read<float>(bytes, 20).value_or(0);
            value.payload_vel_col_geodetic.cov_vu_vu = GPSWire::read<float>(bytes, 24).value_or(0);
            value.payload_vel_col_geodetic.cov_dt_dt = GPSWire::read<float>(bytes, 28).value_or(0);
            value.payload_vel_col_geodetic.cov_vn_ve = GPSWire::read<float>(bytes, 32).value_or(0);
            value.payload_vel_col_geodetic.cov_vn_vu = GPSWire::read<float>(bytes, 36).value_or(0);
            value.payload_vel_col_geodetic.cov_vn_dt = GPSWire::read<float>(bytes, 40).value_or(0);
            value.payload_vel_col_geodetic.cov_ve_vu = GPSWire::read<float>(bytes, 44).value_or(0);
            value.payload_vel_col_geodetic.cov_ve_dt = GPSWire::read<float>(bytes, 48).value_or(0);
            value.payload_vel_col_geodetic.cov_vu_dt = GPSWire::read<float>(bytes, 52).value_or(0);
            break;
        case SBF_ID_DOP:
            value.payload_dop.nr_sv = GPSWire::read<uint8_t>(bytes, 14).value_or(0);
            value.payload_dop.reserved = GPSWire::read<uint8_t>(bytes, 15).value_or(0);
            value.payload_dop.pDOP = GPSWire::read<uint16_t>(bytes, 16).value_or(0);
            value.payload_dop.tDOP = GPSWire::read<uint16_t>(bytes, 18).value_or(0);
            value.payload_dop.hDOP = GPSWire::read<uint16_t>(bytes, 20).value_or(0);
            value.payload_dop.vDOP = GPSWire::read<uint16_t>(bytes, 22).value_or(0);
            value.payload_dop.hpl = GPSWire::read<float>(bytes, 24).value_or(0);
            value.payload_dop.vpl = GPSWire::read<float>(bytes, 28).value_or(0);
            break;
        case SBF_ID_AttEuler:
            value.payload_att_euler.nr_sv = GPSWire::read<uint8_t>(bytes, 14).value_or(0);
            value.payload_att_euler.error_aux1 = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 0) & 3;
            value.payload_att_euler.error_aux2 = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 2) & 3;
            value.payload_att_euler.error_reserved = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 4) & 7;
            value.payload_att_euler.error_not_requested = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 7) & 1;
            value.payload_att_euler.mode = GPSWire::read<uint16_t>(bytes, 16).value_or(0);
            value.payload_att_euler.reserved = GPSWire::read<uint16_t>(bytes, 18).value_or(0);
            value.payload_att_euler.heading = GPSWire::read<float>(bytes, 20).value_or(0);
            value.payload_att_euler.pitch = GPSWire::read<float>(bytes, 24).value_or(0);
            value.payload_att_euler.roll = GPSWire::read<float>(bytes, 28).value_or(0);
            value.payload_att_euler.pitch_dot = GPSWire::read<float>(bytes, 32).value_or(0);
            value.payload_att_euler.roll_dot = GPSWire::read<float>(bytes, 36).value_or(0);
            value.payload_att_euler.heading_dot = GPSWire::read<float>(bytes, 40).value_or(0);
            break;
        case SBF_ID_AttCovEuler:
            value.payload_att_cov_euler.reserved = GPSWire::read<uint8_t>(bytes, 14).value_or(0);
            value.payload_att_cov_euler.error_aux1 = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 0) & 3;
            value.payload_att_cov_euler.error_aux2 = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 2) & 3;
            value.payload_att_cov_euler.error_reserved = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 4) & 7;
            value.payload_att_cov_euler.error_not_requested = (GPSWire::read<uint8_t>(bytes, 15).value_or(0) >> 7) & 1;
            value.payload_att_cov_euler.cov_headhead = GPSWire::read<float>(bytes, 16).value_or(0);
            value.payload_att_cov_euler.cov_pitchpitch = GPSWire::read<float>(bytes, 20).value_or(0);
            value.payload_att_cov_euler.cov_rollroll = GPSWire::read<float>(bytes, 24).value_or(0);
            value.payload_att_cov_euler.cov_headpitch = GPSWire::read<float>(bytes, 28).value_or(0);
            value.payload_att_cov_euler.cov_headroll = GPSWire::read<float>(bytes, 32).value_or(0);
            value.payload_att_cov_euler.cov_pitchroll = GPSWire::read<float>(bytes, 36).value_or(0);
            break;
        default:
            break;
    }
    return value;
}
}  // namespace

int GPSDriverSBF::parseChar(const uint8_t b)
{
    int ret = 0;

    // A native frame owns its payload, including any embedded RTCM preambles.
    if (_rtcm_parsing && (_decode_state == SBF_DECODE_SYNC1 || _rtcm_parsing->hasPartialFrame())) {
        const bool complete = _rtcm_parsing->addByte(b);
        if (complete) {
            if (_rtcm_parsing->valid())
                gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            _rtcm_parsing->reset();
            return GPSDecodedBatch::PROTOCOL_ACTIVITY;
        }
        if (_rtcm_parsing->hasPartialFrame())
            return 0;
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
    _wire[_rx_payload_index++] = b;
    const auto length = GPSWire::read<uint16_t>(_wire, 6).value_or(0);

    if ((_rx_payload_index > 7 && _rx_payload_index >= length) || _rx_payload_index >= _wire.size()) {
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

    _buf = decodeBlock(std::span<const uint8_t>(_wire).first(_rx_payload_index));
    if (_buf.length < 14 || _buf.length > _rx_payload_index || _buf.crc16 != crc16(_wire.data() + 4, _buf.length - 4)) {
        SBF_TRACE_RXMSG("Rx Unknow");
        return 0;
    }

    size_t requiredLength = 0;
    switch (_buf.msg_id) {
        case SBF_ID_PVTGeodetic:
            requiredLength = 94;
            break;
        case SBF_ID_VelCovGeodetic:
            requiredLength = 56;
            break;
        case SBF_ID_DOP:
            requiredLength = 32;
            break;
        case SBF_ID_AttEuler:
            requiredLength = 44;
            break;
        case SBF_ID_AttCovEuler:
            requiredLength = 40;
            break;
        default:
            return 0;
    }
    if (_buf.length < requiredLength || _buf.TOW >= WEEK_MS || _buf.WNc == UINT16_MAX) {
        return 0;
    }

    auto* epoch = navigationEpoch(uint64_t(_buf.WNc) * WEEK_MS + _buf.TOW);
    if (!epoch)
        return GPSDecodedBatch::PROTOCOL_ACTIVITY;
    auto* output = _gps_position;
    _gps_position = &epoch->position;
    switch (_buf.msg_id) {
        case SBF_ID_PVTGeodetic: {
            SBF_TRACE_RXMSG("Rx PVTGeodetic");
            epoch->hasPosition = true;

            if (_buf.payload_pvt_geodetic.mode_type < 1) {
                _gps_position->fix_type = 1;

            } else {
                switch (_buf.payload_pvt_geodetic.mode_type) {
                    case 2:
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

            if (_buf.payload_pvt_geodetic.error != 0)
                _gps_position->fix_type = GPSPositionReport::FIX_TYPE_NONE;
            else if (_buf.payload_pvt_geodetic.mode_2d && _gps_position->fix_type >= GPSPositionReport::FIX_TYPE_3D)
                _gps_position->fix_type = GPSPositionReport::FIX_TYPE_2D;

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
                _gps_position->satellites_used = UINT8_MAX;
                if (_satellite_info)
                    publishSatelliteUsage(std::nullopt);
            }

            _gps_position->latitude_deg = _buf.payload_pvt_geodetic.latitude * M_RAD_TO_DEG;
            _gps_position->longitude_deg = _buf.payload_pvt_geodetic.longitude * M_RAD_TO_DEG;
            _gps_position->altitude_ellipsoid_m = _buf.payload_pvt_geodetic.height;
            _gps_position->altitude_msl_m =
                _buf.payload_pvt_geodetic.height - static_cast<double>(_buf.payload_pvt_geodetic.undulation);

            /* H and V accuracy are reported in 2DRMS, but based off the uBlox reporting we expect RMS.
             * Devide by 100 from cm to m and in addition divide by 2 to get RMS. */
            _gps_position->eph = _buf.payload_pvt_geodetic.h_accuracy != UINT16_MAX
                                     ? static_cast<float>(_buf.payload_pvt_geodetic.h_accuracy) / 200.0f
                                     : NAN;
            _gps_position->accuracy_timestamp = nowUs();
            _gps_position->epv = _buf.payload_pvt_geodetic.v_accuracy != UINT16_MAX
                                     ? static_cast<float>(_buf.payload_pvt_geodetic.v_accuracy) / 200.0f
                                     : NAN;

            _gps_position->vel_n_m_s = static_cast<float>(_buf.payload_pvt_geodetic.vn);
            _gps_position->vel_e_m_s = static_cast<float>(_buf.payload_pvt_geodetic.ve);
            _gps_position->vel_d_m_s = -1.0f * static_cast<float>(_buf.payload_pvt_geodetic.vu);
            _gps_position->vel_m_s = sqrtf(_gps_position->vel_n_m_s * _gps_position->vel_n_m_s +
                                           _gps_position->vel_e_m_s * _gps_position->vel_e_m_s);

            const float course = _buf.payload_pvt_geodetic.cog;
            _gps_position->cog_rad =
                std::isfinite(course) && course >= 0.0f && course <= 360.0f ? course * M_DEG_TO_RAD_F : NAN;
            _gps_position->courseAccuracyRadians = 1.0f * M_DEG_TO_RAD_F;

            // WNc/TOW is GNSS system time, not UTC. Without receiver UTC/leap information,
            // retain the epoch key internally and let the facade use reception UTC.
            _gps_position->time_utc_usec = 0;
            _gps_position->timestamp = nowUs();

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

        case SBF_ID_VelCovGeodetic: {
            const auto& covariance = _buf.payload_vel_col_geodetic;
            const std::array variances{covariance.cov_vn_vn, covariance.cov_ve_ve, covariance.cov_vu_vu};
            const bool valid = !covariance.error && std::all_of(variances.begin(), variances.end(), [](float variance) {
                return std::isfinite(variance) && variance >= 0;
            });
            _gps_position->speedAccuracyMetersPerSecond =
                valid ? std::sqrt(*std::max_element(variances.begin(), variances.end())) : NAN;
            break;
        }
        case SBF_ID_DOP:
            _gps_position->hdop = _buf.payload_dop.hDOP != UINT16_MAX ? _buf.payload_dop.hDOP * 0.01f : NAN;
            _gps_position->vdop = _buf.payload_dop.vDOP != UINT16_MAX ? _buf.payload_dop.vDOP * 0.01f : NAN;
            _gps_position->dop_timestamp = nowUs();
            break;

        case SBF_ID_AttEuler: {
            const auto& attitude = _buf.payload_att_euler;
            _gps_position->heading = NAN;
            _gps_position->heading_timestamp = nowUs();
            if (!attitude.error_not_requested && !attitude.error_aux1 && !attitude.error_aux2 && attitude.mode >= 1 &&
                attitude.mode <= 4 && std::isfinite(attitude.heading) && std::abs(attitude.heading) <= 360) {
                _gps_position->heading = std::remainder(attitude.heading, 360.0f) * M_DEG_TO_RAD_F;
            }
            break;
        }
        case SBF_ID_AttCovEuler: {
            const auto& covariance = _buf.payload_att_cov_euler;
            const float variance = covariance.cov_headhead;
            const bool valid = !covariance.error_not_requested && !covariance.error_aux1 && !covariance.error_aux2 &&
                               std::isfinite(variance) && variance >= 0;
            _gps_position->heading_accuracy = valid ? std::sqrt(variance) * M_DEG_TO_RAD_F : NAN;
            break;
        }

        default:
            SBF_TRACE_RXMSG("Rx other.");
            break;
    }

    _gps_position = output;
    return ret | GPSDecodedBatch::PROTOCOL_ACTIVITY;
}

GPSDriverSBF::NavigationEpoch* GPSDriverSBF::navigationEpoch(uint64_t receiverTimeMs)
{
    flushDecoded();
    if (_lastPublishedEpoch && receiverTimeMs <= *_lastPublishedEpoch)
        return nullptr;
    for (auto& epoch : _epochs)
        if (epoch && epoch->receiverTimeMs == receiverTimeMs)
            return &*epoch;
    auto slot = _epochs.begin();
    while (slot != _epochs.end() && *slot)
        ++slot;
    if (slot == _epochs.end()) {
        slot = _epochs[0]->receiverTimeMs < _epochs[1]->receiverTimeMs ? _epochs.begin() : _epochs.begin() + 1;
        if (receiverTimeMs <= (*slot)->receiverTimeMs)
            return nullptr;
        finishEpoch(*slot);
    }
    *slot = NavigationEpoch{.receiverTimeMs = receiverTimeMs, .receiptUs = nowUs(), .position = {}};
    return &**slot;
}

void GPSDriverSBF::finishEpoch(std::optional<NavigationEpoch>& epoch)
{
    if (epoch->hasPosition && (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch)) {
        *_gps_position = epoch->position;
        _gps_position->timestamp = epoch->receiptUs;
        _decoded.updates |= 1;
        _decoded.events.emplace_back(*_gps_position);
    }
    if (!_lastPublishedEpoch || epoch->receiverTimeMs > *_lastPublishedEpoch)
        _lastPublishedEpoch = epoch->receiverTimeMs;
    epoch.reset();
}

void GPSDriverSBF::flushDecoded()
{
    if (_epochs[0] && _epochs[1] && _epochs[0]->receiverTimeMs > _epochs[1]->receiverTimeMs)
        std::swap(_epochs[0], _epochs[1]);
    const auto now = nowUs();
    for (auto& epoch : _epochs)
        if (epoch && now >= epoch->receiptUs && now - epoch->receiptUs >= EPOCH_MAX_AGE_US)
            finishEpoch(epoch);
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
