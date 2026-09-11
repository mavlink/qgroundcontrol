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

#include "AshtechPrivate.h"
#include "GPSNMEAReport.h"

int GPSDriverAshtech::handleMessage(int len)
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

    NMEAFields::Cursor bufptr({reinterpret_cast<const char*>(_rx_buffer + 7), static_cast<size_t>(len - 7)});
    int ret = 0;

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

        if (!bufptr.valid())
            return 0;

        bufptr.read(day);

        if (!bufptr.valid())
            return 0;

        bufptr.read(month);

        if (!bufptr.valid())
            return 0;

        bufptr.read(year);

        if (!bufptr.valid())
            return 0;

        bufptr.read(local_time_off_hour);

        if (!bufptr.valid())
            return 0;

        bufptr.read(local_time_off_min);

        if (!bufptr.valid())
            return 0;

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
        if (!fix)
            return 0;
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

    } else if ((memcmp(_rx_buffer, "$PASHR,POS,", 11) == 0) && (uiCalcComma == 18)) {
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
        bufptr = NMEAFields::Cursor({reinterpret_cast<const char*>(_rx_buffer + 11), static_cast<size_t>(len - 11)});

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

        if (!bufptr.valid())
            return 0;

        bufptr.read(num_of_sv);

        if (!bufptr.valid())
            return 0;

        bufptr.read(ashtech_time);

        if (!bufptr.valid())
            return 0;

        if (bufptr.read(lat))
            ++coordinatesFound;
        if (!bufptr.valid())
            return 0;

        bufptr.read(ns);

        if (!bufptr.valid())
            return 0;

        if (bufptr.read(lon))
            ++coordinatesFound;
        if (!bufptr.valid())
            return 0;

        bufptr.read(ew);

        if (!bufptr.valid())
            return 0;

        if (bufptr.read(alt))
            ++coordinatesFound;
        if (!bufptr.valid())
            return 0;

        bufptr.read(age_of_corr);

        if (!bufptr.valid())
            return 0;

        bufptr.read(track_true);

        if (!bufptr.valid())
            return 0;

        bufptr.read(ground_speed);

        if (!bufptr.valid())
            return 0;

        bufptr.read(vertic_vel);

        if (!bufptr.valid())
            return 0;

        bufptr.read(pdop);

        if (!bufptr.valid())
            return 0;

        bufptr.read(hdop);

        if (!bufptr.valid())
            return 0;

        bufptr.read(vdop);

        if (!bufptr.valid())
            return 0;

        bufptr.read(tdop);

        if (!bufptr.valid())
            return 0;

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

        _gps_position->latitude_deg = nmeaToDegrees(lat);
        _gps_position->longitude_deg = nmeaToDegrees(lon);
        _gps_position->altitude_msl_m = alt;
        _gps_position->hdop = static_cast<float>(hdop);
        _gps_position->dop_timestamp = nowUs();
        _gps_position->vdop = static_cast<float>(vdop);

        if (coordinatesFound < 3) {
            _gps_position->fix_type = 0;

        } else {
            if (fix_quality == 9 || fix_quality == 10) {          // SBAS differential or BeiDou differential
                _gps_position->fix_type = 4;                      // use RTCM differential

            } else if (fix_quality == 12 || fix_quality == 22) {  // RTK float or RTK float dithered
                _gps_position->fix_type = 5;

            } else if (fix_quality == 13 || fix_quality == 23) {  // RTK fixed or RTK fixed dithered
                _gps_position->fix_type = 6;

            } else {
                _gps_position->fix_type = 3 + fix_quality;
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
        if (!error)
            return 0;
        _gps_position->eph = error->horizontalAccuracy;
        _gps_position->accuracy_timestamp = nowUs();
        _gps_position->epv = error->verticalAccuracy;
        _gps_position->speedAccuracyMetersPerSecond = NAN;

    } else if ((memcmp(_rx_buffer + 3, "GSV,", 4) == 0) && (uiCalcComma >= 3)) {
        const auto parsed = NMEA::sentence({reinterpret_cast<const char*>(_rx_buffer), static_cast<size_t>(len)});
        if (!parsed || !_satellite_info)
            return 0;
        const auto page = NMEA::gsv(*parsed);
        if (!page)
            return 0;
        auto update = _satelliteAssembler.ingest(*parsed, nowUs());
        if (page->message == page->messages) {
            auto completed = _satelliteAssembler.flush();
            update.completed.insert(update.completed.end(), completed.begin(), completed.end());
        }
        for (const auto& system : update.completed) {
            *_satellite_info = {};
            _satellite_info->timestamp = system.inViewTimestampUs;
            _satellite_info->constellation = system.constellation;
            _satellite_info->count = std::min(system.satellites.size(), _satellite_info->entries.size());
            std::copy_n(system.satellites.begin(), _satellite_info->count, _satellite_info->entries.begin());
            publishSatellites(*_satellite_info);
        }

    } else if (memcmp(_rx_buffer, "$PASHR,NAK", 10) == 0) {
        if (_command_state == NMEACommandState::waiting) {
            _command_state = NMEACommandState::nack;
        }

    } else if (memcmp(_rx_buffer, "$PASHR,ACK", 10) == 0) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::Acked) {
            _command_state = NMEACommandState::received;
        }

    } else if (memcmp(_rx_buffer, "$PASHR,PRT,", 11) == 0 && uiCalcComma == 3) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::PRT) {
            _command_state = NMEACommandState::received;
            _port = _rx_buffer[11];
        }

    } else if (memcmp(_rx_buffer, "$PASHR,RID,", 11) == 0) {
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RID) {
            _command_state = NMEACommandState::received;

            if (memcmp(_rx_buffer + 11, "MB2", 3) == 0) {
                _board = AshtechBoard::trimble_mb_two;

            } else {
                _board = AshtechBoard::other;
            }
        }

    } else if (memcmp(_rx_buffer, "$PASHR,RECEIPT,", 15) == 0) {
        // this is the response to $PASHS,POS,AVG,100
        // example: $PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,114502.56,28.12.2011
        if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RECEIPT) {
            _command_state = NMEACommandState::received;
        }

        // when finished we get one of the follwing messages:
        // - successful:
        // $PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100.20*09
        // - unsuccessful: $PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR
        char* finished_find = strstr((char*) _rx_buffer, "FINISHED,");

        if (finished_find) {
            const bool error = strstr((const char*) _rx_buffer, "ERR");
            _survey_in_start = 0;

            if (error) {
                sendSurveyInStatusUpdate(false, false);

            } else {
                // extract the position
                double lat = 0., lon = 0.;
                float alt = 0.f;
                char ns = '?', ew = '?';
                const char* position_fields = strchr(finished_find + 9, ',');
                if (!position_fields)
                    return 0;
                position_fields = strchr(position_fields + 1, ',');
                if (!position_fields)
                    return 0;
                bufptr = NMEAFields::Cursor(std::string_view(
                    position_fields + 1, reinterpret_cast<const char*>(_rx_buffer) + len - position_fields - 1));

                bufptr.read(lat);

                if (!bufptr.valid())
                    return 0;

                bufptr.read(ns);

                if (!bufptr.valid())
                    return 0;

                bufptr.read(lon);

                if (!bufptr.valid())
                    return 0;

                bufptr.read(ew);

                if (!bufptr.valid())
                    return 0;

                bufptr.read(alt);

                if (!bufptr.valid())
                    return 0;

                if (ns == 'S') {
                    lat = -lat;
                }

                if (ew == 'W') {
                    lon = -lon;
                }

                lat = nmeaToDegrees(lat);
                lon = nmeaToDegrees(lon);

                sendSurveyInStatusUpdate(false, true, lat, lon, alt);

                _rtcmActivationPending = true;
            }
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

int GPSDriverAshtech::parseChar(uint8_t b)
{
    int iRet = 0;

    if (_rtcm_parsing) {
        if (_rtcm_parsing->addByte(b) && _rtcm_parsing->valid()) {
            gotRTCMMessage(_rtcm_parsing->message(), _rtcm_parsing->messageLength());
            decodeInit();
            _rtcm_parsing->reset();
            return iRet;
        }
    }

    switch (_decode_state) {
        /* First, look for sync1 */
        case NMEADecodeState::uninit:
            if (b == '$') {
                _decode_state = NMEADecodeState::got_sync1;
                _rx_buffer_bytes = 0;
                _rx_buffer[_rx_buffer_bytes++] = b;
            }

            break;

        case NMEADecodeState::got_sync1:
            if (b == '$') {
                _decode_state = NMEADecodeState::got_sync1;
                _rx_buffer_bytes = 0;

            } else if (b == '*') {
                _decode_state = NMEADecodeState::got_asteriks;
            }

            if (_rx_buffer_bytes >= (sizeof(_rx_buffer) - 5)) {
                _decode_state = NMEADecodeState::uninit;
                _rx_buffer_bytes = 0;

            } else {
                _rx_buffer[_rx_buffer_bytes++] = b;
            }

            break;

        case NMEADecodeState::got_asteriks:
            _rx_buffer[_rx_buffer_bytes++] = b;
            _decode_state = NMEADecodeState::got_first_cs_byte;
            break;

        case NMEADecodeState::got_first_cs_byte: {
            _rx_buffer[_rx_buffer_bytes++] = b;
            uint8_t checksum = 0;
            uint8_t* buffer = _rx_buffer + 1;
            uint8_t* bufend = _rx_buffer + _rx_buffer_bytes - 3;

            for (; buffer < bufend; buffer++) {
                checksum ^= *buffer;
            }

            if ((NMEAFields::hexDigit(checksum >> 4) == *(_rx_buffer + _rx_buffer_bytes - 2)) &&
                (NMEAFields::hexDigit(checksum & 0x0F) == *(_rx_buffer + _rx_buffer_bytes - 1))) {
                iRet = _rx_buffer_bytes;

                if (_rtcm_parsing) {
                    _rtcm_parsing->reset();
                }
            }

            decodeInit();
        } break;
    }

    return iRet;
}

void GPSDriverAshtech::decodeInit()
{
    _rx_buffer_bytes = 0;
    _decode_state = NMEADecodeState::uninit;
}

void GPSDriverAshtech::sendSurveyInStatusUpdate(bool active, bool valid, double latitude, double longitude,
                                                float altitude)
{
    GPSSurveyReport status{};
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = altitude;
    status.duration = !_baseConfig.useFixedBase ? _survey_duration : 0;
    status.mean_accuracy = 0;  // unknown
    status.flags = (int) valid | ((int) active << 1);
    surveyInStatus(status);
}

int GPSDriverAshtech::decodeByte(uint8_t byte)
{
    const int length = parseChar(byte);
    return length > 0 ? handleMessage(length) : 0;
}
