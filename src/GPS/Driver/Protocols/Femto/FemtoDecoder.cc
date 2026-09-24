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

#include <cmath>
#include <cstddef>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CRC32.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSFixQuality.h"
#include "LittleEndian.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"

namespace {
constexpr uint8_t FEMTO_PREAMBLE1 = 0xaa;
constexpr uint8_t FEMTO_PREAMBLE2 = 0x44;
constexpr uint8_t FEMTO_PREAMBLE3 = 0x12;
inline femto_uav_gps_t decodePosition(std::span<const uint8_t> bytes)
{
    femto_uav_gps_t value{};
    value.time_utc_usec = LittleEndian::read<uint64_t>(bytes, 0).value_or(0);
    value.lat = LittleEndian::read<int32_t>(bytes, 8).value_or(0);
    value.lon = LittleEndian::read<int32_t>(bytes, 12).value_or(0);
    value.alt = LittleEndian::read<int32_t>(bytes, 16).value_or(0);
    value.alt_ellipsoid = LittleEndian::read<int32_t>(bytes, 20).value_or(0);
    value.eph = LittleEndian::read<float>(bytes, 32).value_or(0);
    value.epv = LittleEndian::read<float>(bytes, 36).value_or(0);
    value.hdop = LittleEndian::read<float>(bytes, 40).value_or(0);
    value.vdop = LittleEndian::read<float>(bytes, 44).value_or(0);
    value.noise_per_ms = LittleEndian::read<int32_t>(bytes, 48).value_or(0);
    value.jamming_indicator = LittleEndian::read<int32_t>(bytes, 52).value_or(0);
    value.vel_m_s = LittleEndian::read<float>(bytes, 56).value_or(0);
    value.cog_rad = LittleEndian::read<float>(bytes, 72).value_or(0);
    value.heading = LittleEndian::read<float>(bytes, 80).value_or(0);
    value.fix_type = LittleEndian::read<uint8_t>(bytes, 84).value_or(0);
    value.velocityValid = LittleEndian::read<uint8_t>(bytes, 85).value_or(0);
    value.satellites_used = LittleEndian::read<uint8_t>(bytes, 86).value_or(0);
    value.heading_type = LittleEndian::read<uint8_t>(bytes, 87).value_or(0);
    return value;
}
}  // namespace

int GPSNativeFemto::handleMessage(int len)
{
    int ret = 0;
    if (_output_mode != OutputMode::RTCM &&
        (len < _femto_msg.header[3] || _femto_msg.payloadLength > len - _femto_msg.header[3])) {
        return 0;
    }
    uint16_t messageid = _femto_msg.messageId;

    if (messageid == FEMTO_MSG_ID_UAVGPS) { /**< uavgpsB*/
        if (_femto_msg.payloadLength < Femto::GPS_PAYLOAD_SIZE) {
            return 0;
        }
        const auto wire = decodePosition({_femto_msg.data, _femto_msg.payloadLength});

        _position.navigation.utcTimeUs = wire.time_utc_usec;
        _position.navigation.latitudeDegrees = wire.lat / 1e7;
        _position.navigation.longitudeDegrees = wire.lon / 1e7;
        _position.navigation.altitudeMslMeters = wire.alt / 1e3;
        _position.navigation.altitudeEllipsoidMeters = wire.alt_ellipsoid / 1e3;
        _position.navigation.horizontalAccuracyMeters = wire.eph;
        _position.navigation.verticalAccuracyMeters = wire.epv;
        _position.navigation.horizontalDop = wire.hdop;
        _position.navigation.verticalDop = wire.vdop;
        _integrity.rf.noisePerMillisecond = wire.noise_per_ms;
        _integrity.rf.timestampUs = nowUs();
        _integrity.rf.jammingIndicator = wire.jamming_indicator;
        _integrity.rf.timestampUs = nowUs();
        publishIntegrity();
        _position.navigation.speedMetersPerSecond = wire.vel_m_s;
        _position.navigation.courseRadians = wire.cog_rad;
        _position.navigation.fixType = gpsFixQualityFromValue(wire.fix_type);
        _position.velocityValid = wire.velocityValid;
        _position.navigation.satellitesUsed = wire.satellites_used;

        if (wire.heading_type == 6) {
            float heading = wire.heading;
            heading *= GPS_PI / 180.0f;  // deg to rad, now in range [0, 2pi]

            if (heading > GPS_PI) {
                heading -= 2.f * GPS_PI;  // final range is [-pi, pi]
            }

            _position.navigation.headingRadians = heading;

        } else {
            _position.navigation.headingRadians = NAN;
        }

        _position.navigation.timestampUs = nowUs();

        ret = 1;

    } else if (_satellites && messageid == FEMTO_MSG_ID_UAVSTATUS) { /**< set satellite info */
        if (_femto_msg.payloadLength < Femto::STATUS_HEADER_SIZE) {
            return 0;
        }
        const std::span<const uint8_t> status(_femto_msg.data, _femto_msg.payloadLength);
        const auto count = LittleEndian::read<uint32_t>(status, 36).value_or(0);
        if (count > 64 || status.size() < 40 + count * 8) {
            return 0;
        }

        _satellites->timestamp = nowUs();
        _satellites->count = std::min<uint32_t>(count, GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES);

        for (size_t i = 0; i < _satellites->count; i++) {
            _satellites->entries[i].id = LittleEndian::read<uint8_t>(status, 40 + i * 8 + 0).value_or(0);
            _satellites->entries[i].used.reset();
            _satellites->entries[i].elevation = LittleEndian::read<uint8_t>(status, 40 + i * 8 + 3).value_or(0);
            _satellites->entries[i].azimuth = LittleEndian::read<uint16_t>(status, 40 + i * 8 + 4).value_or(0);
            _satellites->entries[i].signal = LittleEndian::read<uint8_t>(status, 40 + i * 8 + 2).value_or(0);
            _satellites->entries[i].prn = LittleEndian::read<uint8_t>(status, 40 + i * 8 + 0).value_or(0);
        }

        ret = 2;

    } else if (OutputMode::RTCM == _output_mode && messageid == FEMTO_MSG_ID_GPGGA &&
               (memcmp(_femto_msg.data + 3, "GGA,", 3) == 0)) { /**< GPGGA only used in base station, for survey-in */
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_femto_msg.data), static_cast<size_t>(len)});
        const auto fix = parsed ? NMEA::gga(*parsed) : std::nullopt;
        if (!fix) {
            return 0;
        }
        if (!_correction_output_activated && fix->quality == 7) {
            _survey_in_start = 0;
            sendSurveyInStatusUpdate(false, true, fix->latitude, fix->longitude, fix->altitude + fix->geoidSeparation);
            _rtcmActivationPending = true;
        }
        if (_satellites) {
            publishSatelliteUsage(fix->satellitesUsed);
        }
    }

    // handle survey-in status update
    if (_survey_in_start != 0) {
        const uint64_t now = nowUs();
        uint32_t survey_in_duration = (now - _survey_in_start) / 1000000;

        if (survey_in_duration != _survey_duration) {
            _survey_duration = survey_in_duration;
            sendSurveyInStatusUpdate(true, false);
        }
    }

    return ret;
}

int GPSNativeFemto::parseChar(uint8_t temp)
{
    int iRet = 0;

    if (_rtcm_parsing && _rtcm_parsing->ownsByte(temp)) {
        _nmeaFramer.reset();
        _rtcm_parsing->addByte(temp);
        drainRTCM(*_rtcm_parsing);
        return 0;
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
                if (temp != Femto::HEADER_SIZE) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }
                _femto_msg.header[0] = FEMTO_PREAMBLE1;
                _femto_msg.header[1] = FEMTO_PREAMBLE2;
                _femto_msg.header[2] = FEMTO_PREAMBLE3;
                _femto_msg.header[3] = temp;
                _decode_state = FemtoDecodeState::head_data;
                _femto_msg.read = 4;
                break;

            case FemtoDecodeState::head_data:
                if (_femto_msg.read >= sizeof(_femto_msg.header)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }

                _femto_msg.header[_femto_msg.read] = temp;
                _femto_msg.read++;

                if (_femto_msg.read >= _femto_msg.header[3]) {
                    _femto_msg.messageId = LittleEndian::read<uint16_t>(_femto_msg.header, 4).value_or(0);
                    _femto_msg.payloadLength = LittleEndian::read<uint16_t>(_femto_msg.header, 8).value_or(0);
                    if (_femto_msg.payloadLength > sizeof(_femto_msg.data)) {
                        _decode_state = FemtoDecodeState::pream_ble1;
                    } else {
                        _decode_state = _femto_msg.payloadLength == 0 ? FemtoDecodeState::crc1 : FemtoDecodeState::data;
                    }
                }

                break;

            case FemtoDecodeState::data:
                if (static_cast<size_t>(_femto_msg.read - _femto_msg.header[3]) >= sizeof(_femto_msg.data)) {
                    _decode_state = FemtoDecodeState::pream_ble1;
                    break;
                }

                _femto_msg.data[_femto_msg.read - _femto_msg.header[3]] = temp;
                _femto_msg.read++;

                if (_femto_msg.read >= (_femto_msg.payloadLength + _femto_msg.header[3])) {
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

                uint32_t crc = QGC::crc32Update({_femto_msg.header, _femto_msg.header[3]});
                crc = QGC::crc32Update({_femto_msg.data, _femto_msg.payloadLength}, crc);

                if (_femto_msg.crc == crc) {
                    iRet = _femto_msg.read;

                } else {
                }
            } break;

            default:
                break;
        }

    } else { /**< RTCM mode */

        iRet = static_cast<int>(_nmeaFramer.addByte(temp));
        if (iRet > 0) {
            _femto_msg.messageId = FEMTO_MSG_ID_GPGGA;
            if (_rtcm_parsing) {
                _rtcm_parsing->reset();
            }
        }
    }

    return iRet;
}

void GPSNativeFemto::decodeInit()
{
    _decode_state = FemtoDecodeState::pream_ble1;
    _nmeaFramer.reset();
}

void GPSNativeFemto::sendSurveyInStatusUpdate(bool active, bool valid, double latitude, double longitude,
                                              float altitude)
{
    GPSNativeSurveyReport status;
    status.survey.position.latitudeDegrees = latitude;
    status.survey.position.longitudeDegrees = longitude;
    status.survey.position.altitudeMeters = altitude;
    status.survey.duration = std::chrono::seconds(
        !std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ? _survey_duration : 0);
    status.survey.valid = valid;
    status.survey.active = active;
    surveyInStatus(status);
}

int GPSNativeFemto::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    const int result = length > 0 ? handleMessage(length) : 0;
    if (result & GPSDecodedBatch::POSITION_UPDATE) {
        publishPosition(_position);
    }
    if ((result & GPSDecodedBatch::SATELLITES_UPDATE) && _satellites) {
        publishSatellites(*_satellites);
    }
    return result;
}

void GPSNativeFemto::flushDecoded()
{
    if (_rtcm_parsing) {
        drainRTCM(*_rtcm_parsing);
    }
}

GPSNativeFemto::GPSNativeFemto(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
{
    decodeInit();
}

int GPSNativeFemto::receive(unsigned timeout)
{
    const int result = receiveDecoded(timeout);
    serviceControls();
    return ioError() ? ioError() : result;
}

void GPSNativeFemto::servicePendingCommands()
{
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        activateRTCMOutput();
    }
}
