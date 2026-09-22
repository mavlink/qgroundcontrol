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

#include <chrono>
#include <ctime>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ashtech/GPSDriverAshtech.h"
#include "NMEA/GPSNMEAReport.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"

namespace {
struct ZdaFields
{
    double time = 0.0;
    int day = 0;
    int month = 0;
    int year = 0;
    int zoneHour = 0;
    int zoneMinute = 0;
};

struct PashrPositionFields
{
    double time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    int satellites = 0;
    int quality = 0;
    double trackDegrees = 0.0;
    double groundSpeedKnots = 0.0;
    double correctionAge = 0.0;
    double horizontalDop = 99.9;
    double verticalDop = 99.9;
    double positionDop = 99.9;
    double timeDop = 99.9;
    double verticalVelocity = 0.0;
    char northSouth = '?';
    char eastWest = '?';
};

std::optional<uint64_t> receiptUtc(std::string_view date, std::string_view time)
{
    if (date.size() != 10 || date[2] != '.' || date[5] != '.') {
        return std::nullopt;
    }
    const auto day = NMEA::number<unsigned>(date.substr(0, 2));
    const auto month = NMEA::number<unsigned>(date.substr(3, 2));
    const auto year = NMEA::number<int>(date.substr(6, 4));
    const auto milliseconds = NMEA::utcMilliseconds(time);
    if (!day || !month || !year || *year < 1980 || *year > 9999 || !milliseconds) {
        return std::nullopt;
    }
    const std::chrono::year_month_day calendar{std::chrono::year{*year}, std::chrono::month{*month},
                                               std::chrono::day{*day}};
    if (!calendar.ok()) {
        return std::nullopt;
    }
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::sys_days(calendar).time_since_epoch() +
                                                                 std::chrono::milliseconds(*milliseconds))
        .count();
}
}  // namespace

int GPSNativeAshtech::handleMessage(int len)
{
    if (len < 7) {
        return 0;
    }

    int uiCalcComma = 0;

    for (int i = 0; i < len; i++) {
        if (_rx_buffer[i] == ',') {
            uiCalcComma++;
        }
    }

    const std::string_view message{reinterpret_cast<const char*>(_rx_buffer), static_cast<size_t>(len)};
    NMEAFields::Cursor bufptr(message.substr(7));
    int ret = 0;
    if (_satellite_info) {
        if (const auto sentence = NMEA::sentence(message)) {
            auto update = _satelliteAssembler.ingest(*sentence, nowUs());
            _queueSatellites(std::move(update.completed));
            if (update.accepted) {
                ret |= GPSDecodedBatch::PROTOCOL_ACTIVITY;
            }
        }
    }

    if ((memcmp(_rx_buffer + 3, "ZDA,", 3) == 0) && (uiCalcComma == 6)) {
        /*
        UTC day, month, and year, and local time zone offset
        An example of the ZDA message string is:

        $GPZDA,172809.456,12,07,1996,00,00*45

        ZDA message fields
        Field	Meaning
        0	Message ID $GPZDA
        1	UTC
        2	Day, ranging between 01 and 31
        3	Month, ranging between 01 and 12
        4	Year
        5	Local time zone offset from GMT, ranging from 00 through 13 hours
        6	Local time zone offset from GMT, ranging from 00 through 59 minutes
        7	The checksum data, always begins with *
        Fields 5 and 6 together yield the total offset. For example, if field 5 is -5 and field 6 is +15, local time is
        5 hours and 15 minutes earlier than GMT.
        */
        ZdaFields data;
        bufptr.read(data.time);
        bufptr.read(data.day);
        bufptr.read(data.month);
        bufptr.read(data.year);
        bufptr.read(data.zoneHour);
        bufptr.read(data.zoneMinute);

        if (!bufptr.valid()) {
            return 0;
        }

        if (data.time < 0 || data.time >= 240000 || data.year < 1980 || data.year > 9999 || data.month < 1 ||
            data.month > 12 || data.day < 1 || data.day > 31) {
            return 0;
        }
        int ashtech_hour = static_cast<int>(data.time / 10000);
        int ashtech_minute = static_cast<int>((data.time - ashtech_hour * 10000) / 100);
        double ashtech_sec = static_cast<double>(data.time - ashtech_hour * 10000 - ashtech_minute * 100);
        if (ashtech_minute > 59 || ashtech_sec >= 60.0) {
            return 0;
        }
        uint64_t usecs = static_cast<uint64_t>((ashtech_sec - static_cast<uint64_t>(ashtech_sec)) * 1000000);

        tm timeinfo{};
        timeinfo.tm_year = data.year - 1900;
        timeinfo.tm_mon = data.month - 1;
        timeinfo.tm_mday = data.day;
        timeinfo.tm_hour = ashtech_hour;
        timeinfo.tm_min = ashtech_minute;
        timeinfo.tm_sec = int(ashtech_sec);
        _utcReference = timeFromUtc(timeinfo, usecs * 1000);
        _gps_position->navigation.utcTimeUs = _utcReference;

        _last_timestamp_time = nowUs();
    }

    else if ((memcmp(_rx_buffer + 3, "GGA,", 3) == 0) && (uiCalcComma == 14) && !_got_pashr_pos_message) {
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_rx_buffer), static_cast<size_t>(len)});
        const auto fix = parsed ? NMEA::gga(*parsed) : std::nullopt;
        if (!fix) {
            return 0;
        }
        applyNMEAGGA(*_gps_position, *fix, nowUs());
        _applyMetadata(NMEA::utcMilliseconds(parsed->fields[NMEA::Field::UTC_TIME]));
        ret = 1;

    } else if (memcmp(_rx_buffer, "$GPHDT,", 7) == 0 && uiCalcComma == 2) {
        /*
        Heading message
        Example $GPHDT,121.2,T*35

        f1 Last computed heading value, in degrees (0-359.99)
        T "T" for "True"
         */

        float heading = 0.f;

        if (bufptr.read(heading)) {
            heading *= GPS_PI / 180.0f;  // deg to rad, now in range [0, 2pi]

            if (heading > GPS_PI) {
                heading -= 2.f * GPS_PI;  // final range is [-pi, pi]
            }

            _gps_position->navigation.headingRadians = heading;
            _gps_position->heading_timestamp = nowUs();
        }

    } else if (message.starts_with("$PASHR,POS,") && (uiCalcComma == 18)) {
        /*
        Example
        $PASHR,POS,2,10,125410.00,5525.8138702,N,03833.9587380,E,131.555,1.0,0.0,0.007,-0.001,2.0,1.0,1.7,1.0,*34

            $PASHR,POS,d1,d2,m3,m4,c5,m6,c7,f8,f9,f10,f11,f12,f13,f14,f15,f16,s17*cc
            Parameter Description Range
              d1 Position mode 0: standalone
                               1: differential
                               2: RTK float
                               3: RTK fixed
                               5: Dead reckoning
                               9: SBAS (see NPT setting)
              d2 Number of satellite used in position fix 0-99
              m3 Current UTC time of position fix (hhmmss.ss) 000000.00-235959.99
              m4 Latitude of position (ddmm.mmmmmm) 0-90 degrees 00-59.9999999 minutes
              c5 Latitude sector N, S
              m6 Longitude of position (dddmm.mmmmmm) 0-180 degrees 00-59.9999999 minutes
              c7 Longitude sector E,W
              f8 Altitude above ellipsoid +9999.000
              f9 Differential age (data link age), seconds 0.0-600.0
              f10 True track/course over ground in degrees 0.0-359.9
              f11 Speed over ground in knots 0.0-999.9
              f12 Vertical velocity in decimeters per second +999.9
              f13 PDOP 0-99.9
              f14 HDOP 0-99.9
              f15 VDOP 0-99.9
              f16 TDOP 0-99.9
              s17 Reserved no data
              *cc Checksum
            */
        bufptr = NMEAFields::Cursor(message.substr(11));

        /*
         * Ashtech would return empty space as coordinate (lat, lon or alt) if it doesn't have a fix yet
         */
        int coordinatesFound = 0;
        PashrPositionFields data;
        bufptr.read(data.quality);
        bufptr.read(data.satellites);
        bufptr.read(data.time);
        if (bufptr.read(data.latitude)) {
            ++coordinatesFound;
        }

        bufptr.read(data.northSouth);
        if (bufptr.read(data.longitude)) {
            ++coordinatesFound;
        }

        bufptr.read(data.eastWest);
        if (bufptr.read(data.altitude)) {
            ++coordinatesFound;
        }

        bufptr.read(data.correctionAge);
        bufptr.read(data.trackDegrees);
        bufptr.read(data.groundSpeedKnots);
        bufptr.read(data.verticalVelocity);
        bufptr.read(data.positionDop);
        bufptr.read(data.horizontalDop);
        bufptr.read(data.verticalDop);
        bufptr.read(data.timeDop);

        if (!bufptr.valid()) {
            return 0;
        }

        if (data.quality < 0 || data.quality > 23 || data.satellites < 0 || data.satellites > 255 ||
            data.latitude < 0 || data.latitude > 9000 || data.longitude < 0 || data.longitude > 18000 ||
            (data.northSouth != 'N' && data.northSouth != 'S') || (data.eastWest != 'E' && data.eastWest != 'W')) {
            return 0;
        }
        if (data.northSouth == 'S') {
            data.latitude = -data.latitude;
        }

        if (data.eastWest == 'W') {
            data.longitude = -data.longitude;
        }

        _gps_position->navigation.latitudeDegrees = NMEA::degreesFromDegreesMinutes(data.latitude);
        _gps_position->navigation.longitudeDegrees = NMEA::degreesFromDegreesMinutes(data.longitude);
        _gps_position->navigation.altitudeEllipsoidMeters = data.altitude;
        _gps_position->navigation.altitudeMslMeters = NAN;
        _gps_position->navigation.horizontalDop = static_cast<float>(data.horizontalDop);
        _gps_position->dop_timestamp = nowUs();
        _gps_position->navigation.verticalDop = static_cast<float>(data.verticalDop);

        if (coordinatesFound < 3) {
            _gps_position->navigation.fixType = GPSPositionReport::FixType::NoFix;

        } else {
            if (data.quality == 9 || data.quality == 10) {  // SBAS differential or BeiDou differential
                _gps_position->navigation.fixType = GPSPositionReport::FixType::Differential;

            } else if (data.quality == 12 || data.quality == 22) {  // RTK float or RTK float dithered
                _gps_position->navigation.fixType = GPSPositionReport::FixType::RTKFloat;

            } else if (data.quality == 13 || data.quality == 23) {  // RTK fixed or RTK fixed dithered
                _gps_position->navigation.fixType = GPSPositionReport::FixType::RTKFixed;

            } else {
                _gps_position->navigation.fixType = GPSPositionReport::fixTypeFromValue(3 + data.quality);
            }

            _got_pashr_pos_message = true;
            // we got a valid position, activate correction output if needed
            if (_configure_done && _output_mode == OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two &&
                !_correction_output_activated) {
                _correctionSetupPending = true;
            }
        }

        _gps_position->navigation.timestampUs = nowUs();
        const auto fields = NMEA::sentence(message);
        _applyMetadata(fields ? NMEA::utcMilliseconds(fields->fields[4]) : std::nullopt);
        _gps_position->navigation.satellitesUsed = static_cast<uint8_t>(data.satellites);

        float track_rad = static_cast<float>(data.trackDegrees) * GPS_PI / 180.0f;

        float velocity_ms = static_cast<float>(data.groundSpeedKnots) / 1.9438445f; /** knots to m/s */
        float velocity_north = static_cast<float>(velocity_ms) * cosf(track_rad);
        float velocity_east = static_cast<float>(velocity_ms) * sinf(track_rad);

        _gps_position->navigation.speedMetersPerSecond = velocity_ms; /** GPS ground speed (m/s) */
        _gps_position->vel_n_m_s = velocity_north;                  /** GPS ground speed in m/s */
        _gps_position->vel_e_m_s = velocity_east;                   /** GPS ground speed in m/s */
        _gps_position->vel_d_m_s = static_cast<float>(-data.verticalVelocity); /** GPS ground speed in m/s */
        _gps_position->navigation.courseRadians =
            track_rad; /** Course over ground (NOT heading, but direction of movement) in rad, -PI..PI */
        _gps_position->vel_ned_valid = true; /** Flag to indicate if NED speed is valid */
        _gps_position->courseAccuracyRadians = 0.1f;
        ret = 1;

    } else if ((memcmp(_rx_buffer + 3, "GST,", 3) == 0) && (uiCalcComma == 8)) {
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_rx_buffer), static_cast<size_t>(len)});
        const auto error = parsed ? NMEA::gst(*parsed) : std::nullopt;
        if (!error) {
            return 0;
        }
        _accuracy = *error;
        _accuracyReceipt = {NMEA::utcMilliseconds(parsed->fields[NMEA::Field::UTC_TIME]), nowUs()};
        _gps_position->speedAccuracyMetersPerSecond = NAN;
        if (_positionEpoch.matches(_accuracyReceipt, METADATA_MAX_AGE_US)) {
            _expireMetadata();
            _gps_position->navigation.horizontalAccuracyMeters = _accuracy.horizontalAccuracy;
            _gps_position->navigation.verticalAccuracyMeters = _accuracy.verticalAccuracy;
            _gps_position->accuracy_timestamp = _accuracyReceipt.receivedAtUs;
            ret |= 1;
        }

    } else if (message.starts_with("$PASHR,NAK*")) {
        if (_command_state == NMEACommandState::waiting) {
            _command_state = NMEACommandState::nack;
        }

    } else if (message.starts_with("$PASHR,ACK*")) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::Acked) {
            _command_state = NMEACommandState::received;
        }

    } else if (message.starts_with("$PASHR,PRT,") && uiCalcComma == 3) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::PRT) {
            _command_state = NMEACommandState::received;
            _port = _rx_buffer[11];
        }

    } else if (message.starts_with("$PASHR,RID,")) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RID) {
            _command_state = NMEACommandState::received;

            if (message.substr(11).starts_with("MB2")) {
                _board = AshtechBoard::trimble_mb_two;

            } else {
                _board = AshtechBoard::other;
            }
        }

    } else if (message.starts_with("$PASHR,RECEIPT,")) {
        const auto receipt = NMEA::sentence(message);
        if (!receipt || receipt->count < 9) {
            return 0;
        }
        const auto& fields = receipt->fields;
        if (fields[2] != "POS" || fields[3] != "AVG") {
            return 0;
        }
        const bool started = fields[4] == "STARTED";
        const bool failed = !started && fields[8] == "ERR";
        double latitude = NAN;
        double longitude = NAN;
        float altitude = NAN;
        std::optional<uint64_t> receiptTime;
        std::optional<uint32_t> interval;

        if (started) {
            interval = NMEA::number<uint32_t>(fields[6]);
            receiptTime = receiptUtc(fields[8], fields[7]);
            if (receipt->count != 9 || fields[5] != "INTERVAL" || !interval || *interval == 0 || !receiptTime) {
                return 0;
            }
        } else {
            interval = NMEA::number<uint32_t>(fields[4]);
            receiptTime = receiptUtc(fields[7], fields[6]);
            if (!interval || *interval == 0 || fields[5] != "FINISHED" || !receiptTime ||
                receipt->count != (failed ? 9 : 16)) {
                return 0;
            }
            if (!failed) {
                const auto lat = NMEA::coordinate(fields[8], fields[9], true);
                const auto lon = NMEA::coordinate(fields[10], fields[11], false);
                const auto alt = NMEA::number<float>(fields[12]);
                const auto duration = NMEA::number<double>(fields[15]);
                if (!lat || !lon || !alt || fields[13] != "OK" || fields[14].empty() || !duration || *duration < 0) {
                    return 0;
                }
                latitude = *lat;
                longitude = *lon;
                altitude = *alt;
            }
        }

        if (_output_mode != OutputMode::RTCM || !_configure_done ||
            std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ||
            _board != AshtechBoard::trimble_mb_two || !_surveyReceiptRequested) {
            return 0;
        }
        if (*interval != std::get<GPSBaseStationConfig::SurveyIn>(_baseConfig.mode).durationSecs) {
            return 0;
        }
        if (started) {
            if (_command_state != NMEACommandState::waiting || _waiting_for_command != NMEACommand::RECEIPT ||
                _surveyReceiptStartUtc) {
                return 0;
            }
            if (_utcReference && NMEA::freshAt(_last_timestamp_time, nowUs(), METADATA_MAX_AGE_US) &&
                (*receiptTime < _utcReference || *receiptTime - _utcReference > METADATA_MAX_AGE_US)) {
                return 0;
            }
            _surveyReceiptStartUtc = receiptTime;
        } else {
            if (!_surveyReceiptStartUtc || *receiptTime < *_surveyReceiptStartUtc ||
                (!failed && *receiptTime - *_surveyReceiptStartUtc < uint64_t(*interval) * 1000000)) {
                return 0;
            }
            _surveyReceiptRequested = false;
            _surveyReceiptStartUtc.reset();
        }
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RECEIPT) {
            _command_state = failed ? NMEACommandState::nack : NMEACommandState::received;
        }
        if (!started) {
            if (_survey_in_start != 0) {
                _survey_duration = (nowUs() - _survey_in_start) / 1000000;
            }
            _survey_in_start = 0;
            sendSurveyInStatusUpdate(false, !failed, latitude, longitude, altitude);
            _rtcmActivationPending = !failed;
        }
    }

    if (ret == 1) {
        _gps_position->timestamp_time_relative =
            (int32_t) (_last_timestamp_time - _gps_position->navigation.timestampUs);
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

int GPSNativeAshtech::parseChar(uint8_t b)
{
    int iRet = 0;

    if (_rtcm_parsing && _rtcm_parsing->ownsByte(b)) {
        _nmeaFramer.reset();
        _rtcm_parsing->addByte(b);
        drainRTCM(*_rtcm_parsing);
        return 0;
    }

    iRet = static_cast<int>(_nmeaFramer.addByte(b));
    if (iRet > 0 && _rtcm_parsing) {
        _rtcm_parsing->reset();
    }

    return iRet;
}

void GPSNativeAshtech::decodeInit()
{
    _nmeaFramer.reset();
}

void GPSNativeAshtech::sendSurveyInStatusUpdate(bool active, bool valid, double latitude, double longitude,
                                                float altitude)
{
    GPSNativeSurveyReport status{};
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode)) {
        status.altitudeDatum = GPSNativeSurveyReport::AltitudeDatum::Ellipsoid;
    }
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = altitude;
    status.duration = !std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseConfig.mode) ? _survey_duration : 0;
    status.mean_accuracy = 0;  // unknown
    status.flags = (int) valid | ((int) active << 1);
    surveyInStatus(status);
}

int GPSNativeAshtech::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    const int result = length > 0 ? handleMessage(length) : 0;
    _drainSatellites();
    if (result & 1) {
        publishPosition(*_gps_position);
    }
    return result;
}

void GPSNativeAshtech::_queueSatellites(NMEA::SatelliteEpoch epoch)
{
    if (!_satellite_info) {
        return;
    }
    _pendingSatellites.insert(_pendingSatellites.end(), std::make_move_iterator(epoch.begin()),
                              std::make_move_iterator(epoch.end()));
}

void GPSNativeAshtech::_drainSatellites()
{
    size_t count = 0;
    while (count < _pendingSatellites.size() && _decoded.events.size() + 2 < GPSDecodedBatch::MAX_EVENTS) {
        *_satellite_info = gpsNMEASatelliteReport(_pendingSatellites[count++]);
        publishSatellites(*_satellite_info);
    }
    _pendingSatellites.erase(_pendingSatellites.begin(), _pendingSatellites.begin() + count);
}

void GPSNativeAshtech::flushDecoded()
{
    _expireMetadata();
    if (_rtcm_parsing) {
        drainRTCM(*_rtcm_parsing);
    }
    _queueSatellites(_satelliteAssembler.flushDue(nowUs()));
    _drainSatellites();
}

void GPSNativeAshtech::_expireMetadata()
{
    const auto now = nowUs();
    if (!_gps_position->heading_timestamp ||
        !NMEA::freshAt(_gps_position->heading_timestamp, now, METADATA_MAX_AGE_US)) {
        _gps_position->navigation.headingRadians = NAN;
        _gps_position->navigation.headingAccuracyRadians = NAN;
    }
    if (!_accuracyReceipt.time || !NMEA::freshAt(_accuracyReceipt.receivedAtUs, now, METADATA_MAX_AGE_US)) {
        _gps_position->navigation.horizontalAccuracyMeters = NAN;
        _gps_position->navigation.verticalAccuracyMeters = NAN;
    }
    if (!_utcReference || !NMEA::freshAt(_last_timestamp_time, now, METADATA_MAX_AGE_US)) {
        _gps_position->navigation.utcTimeUs = 0;
    }
}

void GPSNativeAshtech::_applyMetadata(std::optional<int> time)
{
    _expireMetadata();
    const auto now = nowUs();
    _positionEpoch = {time, now};
    const bool matches = _accuracyReceipt.matches(_positionEpoch, METADATA_MAX_AGE_US);
    _gps_position->navigation.horizontalAccuracyMeters = matches ? _accuracy.horizontalAccuracy : NAN;
    _gps_position->navigation.verticalAccuracyMeters = matches ? _accuracy.verticalAccuracy : NAN;
    _gps_position->accuracy_timestamp = matches ? _accuracyReceipt.receivedAtUs : 0;
    _gps_position->navigation.utcTimeUs =
        NMEA::utcAtTimeOfDay(_utcReference, _last_timestamp_time, time, now, METADATA_MAX_AGE_US);
}

GPSNativeAshtech::GPSNativeAshtech(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSProtocol(std::move(io), satelliteInfoEnabled)
{
    decodeInit();
}

void GPSNativeAshtech::receiveWait(unsigned timeout_min)
{
    uint64_t time_started = nowUs();

    while (nowUs() < time_started + timeout_min * 1000) {
        receive(timeout_min);
        if (ioError()) {
            return;
        }
    }
}

int GPSNativeAshtech::receive(unsigned timeout)
{
    if (const auto deadline = _satelliteAssembler.deadlineUs()) {
        timeout = std::min(timeout, static_cast<unsigned>(remainingMilliseconds(*deadline)));
    }
    const int result = receiveDecoded(timeout);
    serviceControls();
    return ioError() ? ioError() : result;
}

void GPSNativeAshtech::servicePendingCommands()
{
    if (_correctionSetupPending) {
        _correctionSetupPending = false;
        activateCorrectionOutput();
    }
    if (_rtcmActivationPending) {
        _rtcmActivationPending = false;
        activateRTCMOutput();
    }
}
