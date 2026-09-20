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

#include "AshtechPrivate.h"
#include "NMEA/GPSNMEAReport.h"
#include "NMEA/GPSNMEASatelliteReport.h"

namespace {
bool validReceiptDate(std::string_view date)
{
    if (date.size() != 10 || date[2] != '.' || date[5] != '.') {
        return false;
    }
    const auto day = NMEA::number<unsigned>(date.substr(0, 2));
    const auto month = NMEA::number<unsigned>(date.substr(3, 2));
    const auto year = NMEA::number<int>(date.substr(6, 4));
    return day && month && year && *year >= 1980 && *year <= 9999 &&
           std::chrono::year_month_day{std::chrono::year{*year}, std::chrono::month{*month}, std::chrono::day{*day}}
               .ok();
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
        double ashtech_time = 0.0;
        int day = 0, month = 0, year = 0, local_time_off_hour = 0, local_time_off_min = 0;
        (void) local_time_off_min;
        (void) local_time_off_hour;

        bufptr.read(ashtech_time);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(day);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(month);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(year);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(local_time_off_hour);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(local_time_off_min);

        if (!bufptr.valid()) {
            return 0;
        }

        if (ashtech_time < 0 || ashtech_time >= 240000 || year < 1980 || year > 9999 || month < 1 || month > 12 ||
            day < 1 || day > 31) {
            return 0;
        }
        int ashtech_hour = static_cast<int>(ashtech_time / 10000);
        int ashtech_minute = static_cast<int>((ashtech_time - ashtech_hour * 10000) / 100);
        double ashtech_sec = static_cast<double>(ashtech_time - ashtech_hour * 10000 - ashtech_minute * 100);
        if (ashtech_minute > 59 || ashtech_sec >= 60.0) {
            return 0;
        }
        uint64_t usecs = static_cast<uint64_t>((ashtech_sec - static_cast<uint64_t>(ashtech_sec)) * 1000000);

        tm timeinfo{};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = ashtech_hour;
        timeinfo.tm_min = ashtech_minute;
        timeinfo.tm_sec = int(ashtech_sec);
        _gps_position->time_utc_usec = timeFromUtc(timeinfo, usecs * 1000);

        _last_timestamp_time = nowUs();
    }

    else if ((memcmp(_rx_buffer + 3, "GGA,", 3) == 0) && (uiCalcComma == 14) && !_got_pashr_pos_message) {
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_rx_buffer), static_cast<size_t>(len)});
        const auto fix = parsed ? NMEA::gga(*parsed) : std::nullopt;
        if (!fix) {
            return 0;
        }
        applyNMEAGGA(*_gps_position, *fix, nowUs());
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
            heading -= _heading_offset;  // range: [-pi, 3pi]

            if (heading > GPS_PI) {
                heading -= 2.f * GPS_PI;  // final range is [-pi, pi]
            }

            _gps_position->heading = heading;
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
        double ashtech_time = 0.0, lat = 0.0, lon = 0.0, alt = 0.0;
        int num_of_sv = 0, fix_quality = 0;
        double track_true = 0.0, ground_speed = 0.0, age_of_corr = 0.0;
        double hdop = 99.9, vdop = 99.9, pdop = 99.9, tdop = 99.9, vertic_vel = 0.0;
        char ns = '?', ew = '?';

        (void) ashtech_time;
        (void) num_of_sv;
        (void) age_of_corr;
        (void) pdop;
        (void) tdop;

        bufptr.read(fix_quality);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(num_of_sv);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(ashtech_time);

        if (!bufptr.valid()) {
            return 0;
        }

        if (bufptr.read(lat)) {
            ++coordinatesFound;
        }
        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(ns);

        if (!bufptr.valid()) {
            return 0;
        }

        if (bufptr.read(lon)) {
            ++coordinatesFound;
        }
        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(ew);

        if (!bufptr.valid()) {
            return 0;
        }

        if (bufptr.read(alt)) {
            ++coordinatesFound;
        }
        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(age_of_corr);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(track_true);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(ground_speed);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(vertic_vel);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(pdop);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(hdop);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(vdop);

        if (!bufptr.valid()) {
            return 0;
        }

        bufptr.read(tdop);

        if (!bufptr.valid()) {
            return 0;
        }

        if (fix_quality < 0 || fix_quality > 23 || num_of_sv < 0 || num_of_sv > 255 || lat < 0 || lat > 9000 ||
            lon < 0 || lon > 18000 || (ns != 'N' && ns != 'S') || (ew != 'E' && ew != 'W')) {
            return 0;
        }
        if (ns == 'S') {
            lat = -lat;
        }

        if (ew == 'W') {
            lon = -lon;
        }

        _gps_position->latitude_deg = NMEA::degreesFromDegreesMinutes(lat);
        _gps_position->longitude_deg = NMEA::degreesFromDegreesMinutes(lon);
        _gps_position->altitude_ellipsoid_m = alt;
        _gps_position->altitude_msl_m = NAN;
        _gps_position->hdop = static_cast<float>(hdop);
        _gps_position->dop_timestamp = nowUs();
        _gps_position->vdop = static_cast<float>(vdop);

        if (coordinatesFound < 3) {
            _gps_position->fix_type = GPSPositionReport::FixType::NoFix;

        } else {
            if (fix_quality == 9 || fix_quality == 10) {  // SBAS differential or BeiDou differential
                _gps_position->fix_type = GPSPositionReport::FixType::Differential;

            } else if (fix_quality == 12 || fix_quality == 22) {  // RTK float or RTK float dithered
                _gps_position->fix_type = GPSPositionReport::FixType::RTKFloat;

            } else if (fix_quality == 13 || fix_quality == 23) {  // RTK fixed or RTK fixed dithered
                _gps_position->fix_type = GPSPositionReport::FixType::RTKFixed;

            } else {
                _gps_position->fix_type = GPSPositionReport::fixTypeFromValue(3 + fix_quality);
            }

            _got_pashr_pos_message = true;
            // we got a valid position, activate correction output if needed
            if (_configure_done && _output_mode == OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two &&
                !_correction_output_activated) {
                _correctionSetupPending = true;
            }
        }

        _gps_position->timestamp = nowUs();

        float track_rad = static_cast<float>(track_true) * GPS_PI / 180.0f;

        float velocity_ms = static_cast<float>(ground_speed) / 1.9438445f; /** knots to m/s */
        float velocity_north = static_cast<float>(velocity_ms) * cosf(track_rad);
        float velocity_east = static_cast<float>(velocity_ms) * sinf(track_rad);

        _gps_position->vel_m_s = velocity_ms;                       /** GPS ground speed (m/s) */
        _gps_position->vel_n_m_s = velocity_north;                  /** GPS ground speed in m/s */
        _gps_position->vel_e_m_s = velocity_east;                   /** GPS ground speed in m/s */
        _gps_position->vel_d_m_s = static_cast<float>(-vertic_vel); /** GPS ground speed in m/s */
        _gps_position->cog_rad =
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
        _gps_position->eph = error->horizontalAccuracy;
        _gps_position->accuracy_timestamp = nowUs();
        _gps_position->epv = error->verticalAccuracy;
        _gps_position->speedAccuracyMetersPerSecond = NAN;

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

        if (started) {
            const auto interval = NMEA::number<uint32_t>(fields[6]);
            if (receipt->count != 9 || fields[5] != "INTERVAL" || !interval || *interval == 0 ||
                !NMEA::utcMilliseconds(fields[7]) || !validReceiptDate(fields[8])) {
                return 0;
            }
        } else {
            const auto interval = NMEA::number<uint32_t>(fields[4]);
            if (!interval || *interval == 0 || fields[5] != "FINISHED" || !NMEA::utcMilliseconds(fields[6]) ||
                !validReceiptDate(fields[7]) || receipt->count != (failed ? 9 : 16)) {
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

        if (_output_mode != OutputMode::RTCM || !_configure_done || _baseConfig.useFixedBase ||
            _board != AshtechBoard::trimble_mb_two) {
            return 0;
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
        _gps_position->timestamp_time_relative = (int32_t) (_last_timestamp_time - _gps_position->timestamp);
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

    if (_rtcm_parsing) {
        if (_rtcm_parsing->addByte(b) && _rtcm_parsing->valid()) {
            gotRTCMMessage(_rtcm_parsing->frame().data(), _rtcm_parsing->frame().size());
            decodeInit();
            _rtcm_parsing->reset();
            return iRet;
        }
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
    if (_baseConfig.useFixedBase) {
        status.altitudeDatum = GPSNativeSurveyReport::AltitudeDatum::Ellipsoid;
    }
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = altitude;
    status.duration = !_baseConfig.useFixedBase ? _survey_duration : 0;
    status.mean_accuracy = 0;  // unknown
    status.flags = (int) valid | ((int) active << 1);
    surveyInStatus(status);
}

int GPSNativeAshtech::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    const int result = length > 0 ? handleMessage(length) : 0;
    _drainSatellites();
    return result;
}

void GPSNativeAshtech::_queueSatellites(NMEA::SatelliteEpoch epoch)
{
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
    _queueSatellites(_satelliteAssembler.flushDue(nowUs()));
    _drainSatellites();
}
