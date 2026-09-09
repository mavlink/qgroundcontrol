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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctime>

#include "ashtech.h"
#include "rtcm.h"

#define MIN(X,Y)	((X) < (Y) ? (X) : (Y))
#define ASH_UNUSED(x) (void)x;

//#define ASH_DEBUG(...)		{GPS_WARN(__VA_ARGS__);}
#define ASH_DEBUG(...)		{/*GPS_WARN(__VA_ARGS__);*/}

GPSDriverAshtech::GPSDriverAshtech(GPSCallbackPtr callback, void *callback_user,
				   sensor_gps_s *gps_position, satellite_info_s *satellite_info,
				   float heading_offset) :
	GPSBaseStationSupport(callback, callback_user),
	_heading_offset(heading_offset),
	_gps_position(gps_position),
	_satellite_info(satellite_info)
{
	decodeInit();
}

GPSDriverAshtech::~GPSDriverAshtech()
{
	delete _rtcm_parsing;
}

/*
 * All NMEA descriptions are taken from
 * http://www.trimble.com/OEM_ReceiverHelp/V4.44/en/NMEA-0183messages_MessageOverview.html
 */

int GPSDriverAshtech::handleMessage(int len)
{
	char *endp;

	if (len < 7) {
		return 0;
	}

	int uiCalcComma = 0;

	for (int i = 0 ; i < len; i++) {
		if (_rx_buffer[i] == ',') { uiCalcComma++; }
	}

	char *bufptr = (char *)(_rx_buffer + 6);
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
		Fields 5 and 6 together yield the total offset. For example, if field 5 is -5 and field 6 is +15, local time is 5 hours and 15 minutes earlier than GMT.
		*/
		double ashtech_time = 0.0;
		int day = 0, month = 0, year = 0, local_time_off_hour = 0, local_time_off_min = 0;
		ASH_UNUSED(local_time_off_min);
		ASH_UNUSED(local_time_off_hour);

		nmeaNextField(bufptr, ashtech_time);

		nmeaNextField(bufptr, day);

		nmeaNextField(bufptr, month);

		nmeaNextField(bufptr, year);

		nmeaNextField(bufptr, local_time_off_hour);

		nmeaNextField(bufptr, local_time_off_min);

		int ashtech_hour = static_cast<int>(ashtech_time / 10000);
		int ashtech_minute = static_cast<int>((ashtech_time - ashtech_hour * 10000) / 100);
		double ashtech_sec = static_cast<double>(ashtech_time - ashtech_hour * 10000 - ashtech_minute * 100);
		uint64_t usecs = static_cast<uint64_t>((ashtech_sec - static_cast<uint64_t>(ashtech_sec))) * 1000000;

		tm timeinfo{};
		timeinfo.tm_year = year - 1900;
		timeinfo.tm_mon = month - 1;
		timeinfo.tm_mday = day;
		timeinfo.tm_hour = ashtech_hour;
		timeinfo.tm_min = ashtech_minute;
		timeinfo.tm_sec = int(ashtech_sec);
		_gps_position->time_utc_usec = timeFromUtc(timeinfo, usecs * 1000);

		_last_timestamp_time = gps_absolute_time();
	}

	else if ((memcmp(_rx_buffer + 3, "GGA,", 3) == 0) && (uiCalcComma == 14) && !_got_pashr_pos_message) {
		/*
		  Time, position, and fix related data
		  An example of the GBS message string is:

		  $GPGGA,172814.0,3723.46587704,N,12202.26957864,W,2,6,1.2,18.893,M,-25.669,M,2.0,0031*4F

		  Note - The data string exceeds the ASHTECH standard length.
		  GGA message fields
		  Field   Meaning
		  0   Message ID $GPGGA
		  1   UTC of position fix
		  2   Latitude
		  3   Direction of latitude:
		  N: North
		  S: South
		  4   Longitude
		  5   Direction of longitude:
		  E: East
		  W: West
		  6   GPS Quality indicator:
		  0: Fix not valid
		  1: GPS fix
		  2: Differential GPS fix, OmniSTAR VBS
		  4: Real-Time Kinematic, fixed integers
		  5: Real-Time Kinematic, float integers, OmniSTAR XP/HP or Location RTK
		  7   Number of SVs in use, range from 00 through to 24+
		  8   HDOP
		  9   Orthometric height (MSL reference)
		  10  M: unit of measure for orthometric height is meters
		  11  Geoid separation
		  12  M: geoid separation measured in meters
		  13  Age of differential GPS data record, Type 1 or Type 9. Null field when DGPS is not used.
		  14  Reference station ID, range 0000-4095. A null field when any reference station ID is selected and no corrections are received1.
		  15
		  The checksum data, always begins with *
		  Note - If a user-defined geoid model, or an inclined
		*/
		double ashtech_time = 0.0, lat = 0.0, lon = 0.0, alt = 0.0;
		int num_of_sv = 0, fix_quality = 0;
		double hdop = 99.9;
		char ns = '?', ew = '?';

		ASH_UNUSED(ashtech_time);
		ASH_UNUSED(num_of_sv);
		ASH_UNUSED(hdop);

		nmeaNextField(bufptr, ashtech_time);

		nmeaNextField(bufptr, lat);

		nmeaNextField(bufptr, ns);

		nmeaNextField(bufptr, lon);

		nmeaNextField(bufptr, ew);

		nmeaNextField(bufptr, fix_quality);

		nmeaNextField(bufptr, num_of_sv);

		nmeaNextField(bufptr, hdop);

		nmeaNextField(bufptr, alt);

		if (ns == 'S') {
			lat = -lat;
		}

		if (ew == 'W') {
			lon = -lon;
		}

		/* convert from degrees, minutes and seconds to degrees * 1e7 */
		_gps_position->latitude_deg = nmeaToDegrees(lat);
		_gps_position->longitude_deg = nmeaToDegrees(lon);
		_gps_position->altitude_msl_m = alt;
		_rate_count_lat_lon++;

		if (fix_quality <= 0) {
			_gps_position->fix_type = 0;

		} else {
			/*
			 * in this NMEA message float integers (value 5) mode has higher value than fixed integers (value 4), whereas it provides lower quality,
			 * and since value 3 is not being used, I "moved" value 5 to 3 to add it to _gps_position->fix_type
			 */
			if (fix_quality == 5) { fix_quality = 3; }

			/*
			 * fix quality 1 means just a normal 3D fix, so I'm subtracting 1 here. This way we'll have 3 for auto, 4 for DGPS, 5 for floats, 6 for fixed.
			 */
			_gps_position->fix_type = 3 + fix_quality - 1;
		}

		_gps_position->timestamp = gps_absolute_time();

		_gps_position->vel_m_s = 0;                                  /**< GPS ground speed (m/s) */
		_gps_position->vel_n_m_s = 0;                                /**< GPS ground speed in m/s */
		_gps_position->vel_e_m_s = 0;                                /**< GPS ground speed in m/s */
		_gps_position->vel_d_m_s = 0;                                /**< GPS ground speed in m/s */
		_gps_position->cog_rad =
			0;                                  /**< Course over ground (NOT heading, but direction of movement) in rad, -PI..PI */
		_gps_position->vel_ned_valid = true;                         /**< Flag to indicate if NED speed is valid */
		_gps_position->c_variance_rad = 0.1f;
		ret = 1;

	} else if (memcmp(_rx_buffer, "$GPHDT,", 7) == 0 && uiCalcComma == 2) {
		/*
		Heading message
		Example $GPHDT,121.2,T*35

		f1 Last computed heading value, in degrees (0-359.99)
		T "T" for "True"
		 */

		float heading = 0.f;

		if (nmeaNextField(bufptr, heading)) {

			ASH_DEBUG("heading update: %.3f", (double)heading);

			heading *= M_PI_F / 180.0f; // deg to rad, now in range [0, 2pi]
			heading -= _heading_offset; // range: [-pi, 3pi]

			if (heading > M_PI_F) {
				heading -= 2.f * M_PI_F; // final range is [-pi, pi]
			}

			_gps_position->heading = heading;
		}

	} else if ((memcmp(_rx_buffer, "$PASHR,POS,", 11) == 0) && (uiCalcComma == 18)) {
		_got_pashr_pos_message = true;
		/*
		Example $PASHR,POS,2,10,125410.00,5525.8138702,N,03833.9587380,E,131.555,1.0,0.0,0.007,-0.001,2.0,1.0,1.7,1.0,*34

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
		bufptr = (char *)(_rx_buffer + 10);

		/*
		 * Ashtech would return empty space as coordinate (lat, lon or alt) if it doesn't have a fix yet
		 */
		int coordinatesFound = 0;
		double ashtech_time = 0.0, lat = 0.0, lon = 0.0, alt = 0.0;
		int num_of_sv = 0, fix_quality = 0;
		double track_true = 0.0, ground_speed = 0.0, age_of_corr = 0.0;
		double hdop = 99.9, vdop = 99.9,  pdop = 99.9, tdop = 99.9, vertic_vel = 0.0;
		char ns = '?', ew = '?';

		ASH_UNUSED(ashtech_time);
		ASH_UNUSED(num_of_sv);
		ASH_UNUSED(age_of_corr);
		ASH_UNUSED(pdop);
		ASH_UNUSED(tdop);

		nmeaNextField(bufptr, fix_quality);

		nmeaNextField(bufptr, num_of_sv);

		nmeaNextField(bufptr, ashtech_time);

		if (bufptr && *(++bufptr) != ',') {
			/*
			 * if a coordinate is skipped (i.e. no fix), it either won't get into this block (two commas in a row)
			 * or strtod won't find anything and endp will point exactly where bufptr is. The same is for lon and alt.
			 */
			lat = strtod(bufptr, &endp);

			if (bufptr != endp) {coordinatesFound++;}

			bufptr = endp;
		}

		nmeaNextField(bufptr, ns);

		if (bufptr && *(++bufptr) != ',') {
			lon = strtod(bufptr, &endp);

			if (bufptr != endp) {coordinatesFound++;}

			bufptr = endp;
		}

		nmeaNextField(bufptr, ew);

		if (bufptr && *(++bufptr) != ',') {
			alt = strtod(bufptr, &endp);

			if (bufptr != endp) {coordinatesFound++;}

			bufptr = endp;
		}

		nmeaNextField(bufptr, age_of_corr);

		nmeaNextField(bufptr, track_true);

		nmeaNextField(bufptr, ground_speed);

		nmeaNextField(bufptr, vertic_vel);

		nmeaNextField(bufptr, pdop);

		nmeaNextField(bufptr, hdop);

		nmeaNextField(bufptr, vdop);

		nmeaNextField(bufptr, tdop);

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
		_gps_position->vdop = static_cast<float>(vdop);
		_rate_count_lat_lon++;

		if (coordinatesFound < 3) {
			_gps_position->fix_type = 0;

		} else {
			if (fix_quality == 9 || fix_quality == 10) { // SBAS differential or BeiDou differential
				_gps_position->fix_type = 4; // use RTCM differential

			} else if (fix_quality == 12 || fix_quality == 22) { // RTK float or RTK float dithered
				_gps_position->fix_type = 5;

			} else if (fix_quality == 13 || fix_quality == 23) { // RTK fixed or RTK fixed dithered
				_gps_position->fix_type = 6;

			} else {
				_gps_position->fix_type = 3 + fix_quality;
			}

			// we got a valid position, activate correction output if needed
			if (_configure_done && _output_mode == OutputMode::RTCM &&
			    _board == AshtechBoard::trimble_mb_two && !_correction_output_activated) {
				activateCorrectionOutput();
			}
		}

		_gps_position->timestamp = gps_absolute_time();

		float track_rad = static_cast<float>(track_true) * M_PI_F / 180.0f;

		float velocity_ms = static_cast<float>(ground_speed) / 1.9438445f;			/** knots to m/s */
		float velocity_north = static_cast<float>(velocity_ms) * cosf(track_rad);
		float velocity_east  = static_cast<float>(velocity_ms) * sinf(track_rad);

		_gps_position->vel_m_s = velocity_ms;				/** GPS ground speed (m/s) */
		_gps_position->vel_n_m_s = velocity_north;			/** GPS ground speed in m/s */
		_gps_position->vel_e_m_s = velocity_east;			/** GPS ground speed in m/s */
		_gps_position->vel_d_m_s = static_cast<float>(-vertic_vel);				/** GPS ground speed in m/s */
		_gps_position->cog_rad =
			track_rad;				/** Course over ground (NOT heading, but direction of movement) in rad, -PI..PI */
		_gps_position->vel_ned_valid = true;				/** Flag to indicate if NED speed is valid */
		_gps_position->c_variance_rad = 0.1f;
		_rate_count_vel++;
		ret = 1;

	} else if ((memcmp(_rx_buffer + 3, "GST,", 3) == 0) && (uiCalcComma == 8)) {
		/*
		  Position error statistics
		  An example of the GST message string is:

		  $GPGST,172814.0,0.006,0.023,0.020,273.6,0.023,0.020,0.031*6A

		  The Talker ID ($--) will vary depending on the satellite system used for the position solution:

		  $GP - GPS only
		  $GL - GLONASS only
		  $GN - Combined
		  GST message fields
		  Field   Meaning
		  0   Message ID $GPGST
		  1   UTC of position fix
		  2   RMS value of the pseudorange residuals; includes carrier phase residuals during periods of RTK (float) and RTK (fixed) processing
		  3   Error ellipse semi-major axis 1 sigma error, in meters
		  4   Error ellipse semi-minor axis 1 sigma error, in meters
		  5   Error ellipse orientation, degrees from true north
		  6   Latitude 1 sigma error, in meters
		  7   Longitude 1 sigma error, in meters
		  8   Height 1 sigma error, in meters
		  9   The checksum data, always begins with *
		*/
		double ashtech_time = 0.0, lat_err = 0.0, lon_err = 0.0, alt_err = 0.0;
		double min_err = 0.0, maj_err = 0.0, deg_from_north = 0.0, rms_err = 0.0;

		ASH_UNUSED(ashtech_time);
		ASH_UNUSED(min_err);
		ASH_UNUSED(maj_err);
		ASH_UNUSED(deg_from_north);
		ASH_UNUSED(rms_err);

		nmeaNextField(bufptr, ashtech_time);

		nmeaNextField(bufptr, rms_err);

		nmeaNextField(bufptr, maj_err);

		nmeaNextField(bufptr, min_err);

		nmeaNextField(bufptr, deg_from_north);

		nmeaNextField(bufptr, lat_err);

		nmeaNextField(bufptr, lon_err);

		nmeaNextField(bufptr, alt_err);

		_gps_position->eph = sqrtf(static_cast<float>(lat_err) * static_cast<float>(lat_err)
					   + static_cast<float>(lon_err) * static_cast<float>(lon_err));
		_gps_position->epv = static_cast<float>(alt_err);

		_gps_position->s_variance_m_s = 0;

	} else if ((memcmp(_rx_buffer + 3, "GSV,", 4) == 0) && (uiCalcComma >= 3)) {
		/*
		  The GSV message string identifies the number of SVs in view, the PRN numbers, elevations, azimuths, and SNR values. An example of the GSV message string is:

		  $GPGSV,4,1,13,02,02,213,,03,-3,000,,11,00,121,,14,13,172,05*67

		  GSV message fields
		  Field   Meaning
		  0   Message ID $GPGSV
		  1   Total number of messages of this type in this cycle
		  2   Message number
		  3   Total number of SVs visible
		  4   SV PRN number
		  5   Elevation, in degrees, 90 maximum
		  6   Azimuth, degrees from True North, 000 through 359
		  7   SNR, 00 through 99 dB (null when not tracking)
		  8-11    Information about second SV, same format as fields 4 through 7
		  12-15   Information about third SV, same format as fields 4 through 7
		  16-19   Information about fourth SV, same format as fields 4 through 7
		  20  The checksum data, always begins with *
		*/
		/*
		 * currently process only gps, because do not know what
		 * Global satellite ID I should use for non GPS sats
		 */
		bool bGPS = false;

		if (memcmp(_rx_buffer, "$GP", 3) != 0) {
			return 0;

		} else {
			bGPS = true;
		}

		int all_msg_num = 0, this_msg_num = 0, tot_sv_visible = 0;
		struct gsv_sat {
			int svid;
			int elevation;
			int azimuth;
			int snr;
			int prn;
		} sat[4];

		memset(sat, 0, sizeof(sat));

		nmeaNextField(bufptr, all_msg_num);

		nmeaNextField(bufptr, this_msg_num);

		nmeaNextField(bufptr, tot_sv_visible);

		if ((this_msg_num < 1) || (this_msg_num > all_msg_num) || (tot_sv_visible < 0)) {
			return 0;
		}

		const uint64_t page_start = static_cast<uint64_t>(this_msg_num - 1) * 4;

		if (page_start > static_cast<uint64_t>(tot_sv_visible)) {
			return 0;
		}

		if (this_msg_num == 0 && bGPS && _satellite_info) {
			memset(_satellite_info->svid,      0, sizeof(_satellite_info->svid));
			memset(_satellite_info->used,      0, sizeof(_satellite_info->used));
			memset(_satellite_info->elevation, 0, sizeof(_satellite_info->elevation));
			memset(_satellite_info->azimuth,   0, sizeof(_satellite_info->azimuth));
			memset(_satellite_info->snr,       0, sizeof(_satellite_info->snr));
			memset(_satellite_info->prn,       0, sizeof(_satellite_info->prn));
		}

		const int remaining_satellites = tot_sv_visible - static_cast<int>(page_start);
		const int satellites_in_message = (uiCalcComma - 3) / 4;
		const int satellites_in_page = remaining_satellites < satellites_in_message ?
					       remaining_satellites : satellites_in_message;
		const int satellites_to_parse = satellites_in_page < 4 ? satellites_in_page : 4;

		if (this_msg_num == all_msg_num) {
			_gps_position->satellites_used = tot_sv_visible;

			if (_satellite_info) {
				_satellite_info->count = MIN(tot_sv_visible, satellite_info_s::SAT_INFO_MAX_SATELLITES);
				_satellite_info->timestamp = gps_absolute_time();
				ret = 2;
			}
		}

		if (_satellite_info) {
			for (int y = 0; y < satellites_to_parse; y++) {
				nmeaNextField(bufptr, sat[y].svid);

				nmeaNextField(bufptr, sat[y].elevation);

				nmeaNextField(bufptr, sat[y].azimuth);

				nmeaNextField(bufptr, sat[y].snr);

				const uint64_t satellite_index = page_start + y;

				if (satellite_index < satellite_info_s::SAT_INFO_MAX_SATELLITES) {
					_satellite_info->svid[satellite_index]      = sat[y].svid;
					_satellite_info->used[satellite_index]      = (sat[y].snr > 0);
					_satellite_info->elevation[satellite_index] = sat[y].elevation;
					_satellite_info->azimuth[satellite_index]   = sat[y].azimuth;
					_satellite_info->snr[satellite_index]       = sat[y].snr;
					_satellite_info->prn[satellite_index]       = sat[y].prn;
				}
			}
		}

	} else if (memcmp(_rx_buffer, "$PASHR,NAK", 10) == 0) {
		ASH_DEBUG("Nack received");

		if (_command_state == NMEACommandState::waiting) {
			_command_state = NMEACommandState::nack;
		}

	} else if (memcmp(_rx_buffer, "$PASHR,ACK", 10) == 0) {
		ASH_DEBUG("Ack received");

		if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::Acked) {
			_command_state = NMEACommandState::received;
		}

	} else if (memcmp(_rx_buffer, "$PASHR,PRT,", 11) == 0 && uiCalcComma == 3) {
		if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::PRT) {
			_command_state = NMEACommandState::received;
			_port = _rx_buffer[11];
			ASH_DEBUG("Connected port: %c", _port);
		}

	} else if (memcmp(_rx_buffer, "$PASHR,RID,", 11) == 0) {
		if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RID) {
			_command_state = NMEACommandState::received;

			if (memcmp(_rx_buffer + 11, "MB2", 3) == 0) {
				_board = AshtechBoard::trimble_mb_two;

			} else {
				_board = AshtechBoard::other;
			}

			ASH_DEBUG("Connected board: %i", (int)_board);
		}

	} else if (memcmp(_rx_buffer, "$PASHR,RECEIPT,", 15) == 0) {
		// this is the response to $PASHS,POS,AVG,100
		// example: $PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,114502.56,28.12.2011
		if (_command_state == NMEACommandState::waiting && _waiting_for_command == NMEACommand::RECEIPT) {
			_command_state = NMEACommandState::received;
		}

		// when finished we get one of the follwing messages:
		// - successful: $PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100.20*09
		// - unsuccessful: $PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR
		char *finished_find = strstr((char *)_rx_buffer, "FINISHED,");

		if (finished_find) {
			const bool error = strstr((const char *)_rx_buffer, "ERR");
			_survey_in_start = 0;

			if (error) {
				sendSurveyInStatusUpdate(false, false);

			} else {
				// extract the position
				double lat = 0., lon = 0.;
				float alt = 0.f;
				char ns = '?', ew = '?';
				bufptr = finished_find + 9; // skip over FINISHED,
				// skip the next 2 arguments
				bufptr = strstr(bufptr, ",");

				if (bufptr) { bufptr = strstr(bufptr + 1, ","); }

				nmeaNextField(bufptr, lat);

				nmeaNextField(bufptr, ns);

				nmeaNextField(bufptr, lon);

				nmeaNextField(bufptr, ew);

				nmeaNextField(bufptr, alt);

				if (ns == 'S') { lat = -lat; }

				if (ew == 'W') { lon = -lon; }

				lat = nmeaToDegrees(lat);
				lon = nmeaToDegrees(lon);

				sendSurveyInStatusUpdate(false, true, lat, lon, alt);

				activateRTCMOutput(true);
			}
		}

	}

	if (ret == 1) {
		_gps_position->timestamp_time_relative = (int32_t)(_last_timestamp_time - _gps_position->timestamp);
	}


	// handle survey-in status update
	if (_survey_in_start != 0) {
		const gps_abstime now = gps_absolute_time();
		uint32_t survey_in_duration = (now - _survey_in_start) / 1000000;

		if (survey_in_duration != _base_settings.settings.survey_in.min_dur) {
			_base_settings.settings.survey_in.min_dur = survey_in_duration;
			sendSurveyInStatusUpdate(true, false);
		}
	}

	return ret;
}

void GPSDriverAshtech::activateRTCMOutput(bool reduce_update_rate)
{
	char buffer[40];
	const char *rtcm_options[] = {
		"$PASHS,NME,POS,%c,ON,0.2\r\n",  // reduce position updates to 5 Hz

		"$PASHS,RT3,1074,%c,ON,1\r\n", // GPS observations
		"$PASHS,RT3,1084,%c,ON,1\r\n", // GLONASS observations
		"$PASHS,RT3,1094,%c,ON,1\r\n", // Galileo observations

		"$PASHS,RT3,1114,%c,ON,1\r\n", // QZSS observations
		"$PASHS,RT3,1124,%c,ON,1\r\n", // BDS observations
		"$PASHS,RT3,1006,%c,ON,1\r\n", // Static position
		"$PASHS,RT3,1033,%c,ON,31\r\n", // Antenna and receiver name
		"$PASHS,RT3,1013,%c,ON,1\r\n", // System parameters
		"$PASHS,RT3,1029,%c,ON,1\r\n", // ASCII message
		"$PASHS,RT3,1230,%c,ON\r\n", // GLONASS code phase bias

		// TODO: are these required (these are the ones from u-blox)?
		"$PASHS,RT3,1005,%c,ON,1\r\n",
		"$PASHS,RT3,1077,%c,ON,1\r\n",
		"$PASHS,RT3,1087,%c,ON,1\r\n",
	};

	unsigned first_index = reduce_update_rate ? 0 : 1;

	for (unsigned int conf_i = first_index; conf_i < sizeof(rtcm_options) / sizeof(rtcm_options[0]); conf_i++) {
		int str_len = snprintf(buffer, sizeof(buffer), rtcm_options[conf_i], _port);

		if (writeAckedCommand(buffer, str_len, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", buffer);
		}
	}
}

void GPSDriverAshtech::receiveWait(unsigned timeout_min)
{
	gps_abstime time_started = gps_absolute_time();

	while (gps_absolute_time() < time_started + timeout_min * 1000) {
		receive(timeout_min);
	}
}

int GPSDriverAshtech::receive(unsigned timeout)
{
	{

		uint8_t buf[GPS_READ_BUFFER_SIZE];

		/* timeout additional to poll */
		uint64_t time_started = gps_absolute_time();

		int j = 0;
		int bytes_count = 0;

		while (true) {

			/* pass received bytes to the packet decoder */
			while (j < bytes_count) {
				int l = 0;

				if ((l = parseChar(buf[j])) > 0) {
					/* return to configure during configuration or to the gps driver during normal work
					 * if a packet has arrived */
					int ret = handleMessage(l);

					if (ret > 0) {
						return ret;
					}
				}

				j++;
			}

			/* everything is read */
			j = bytes_count = 0;

			/* then poll or read for new data */
			int ret = read(buf, sizeof(buf), timeout * 2);

			if (ret < 0) {
				/* something went wrong when polling */
				return -1;

			} else if (ret == 0) {
				/* Timeout while polling or just nothing read if reading, let's
				 * stay here, and use timeout below. */

			} else if (ret > 0) {
				/* if we have new data from GPS, go handle it */
				bytes_count = ret;
			}

			/* in case we get crap from GPS or time out */
			if (time_started + timeout * 1000 * 2 < gps_absolute_time()) {
				return -1;
			}
		}
	}

}
#define HEXDIGIT_CHAR(d) ((char)((d) + (((d) < 0xA) ? '0' : 'A'-0xA)))

int GPSDriverAshtech::parseChar(uint8_t b)
{
	int iRet = 0;

	if (_rtcm_parsing) {
		if (_rtcm_parsing->addByte(b)) {
			ASH_DEBUG("got RTCM message with length %i", (int)_rtcm_parsing->messageLength());
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
			ASH_DEBUG("buffer overflow");
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
			uint8_t *buffer = _rx_buffer + 1;
			uint8_t *bufend = _rx_buffer + _rx_buffer_bytes - 3;

			for (; buffer < bufend; buffer++) { checksum ^= *buffer; }

			if ((HEXDIGIT_CHAR(checksum >> 4) == *(_rx_buffer + _rx_buffer_bytes - 2)) &&
			    (HEXDIGIT_CHAR(checksum & 0x0F) == *(_rx_buffer + _rx_buffer_bytes - 1))) {
				iRet = _rx_buffer_bytes;

				if (_rtcm_parsing) {
					_rtcm_parsing->reset();
				}
			}

			decodeInit();
		}
		break;
	}

	return iRet;
}

void GPSDriverAshtech::decodeInit()
{
	_rx_buffer_bytes = 0;
	_decode_state = NMEADecodeState::uninit;
}

int GPSDriverAshtech::writeAckedCommand(const void *buf, int buf_length, unsigned timeout)
{
	if (write(buf, buf_length) != buf_length) {
		return -1;
	}

	return waitForReply(NMEACommand::Acked, timeout);
}

int GPSDriverAshtech::waitForReply(NMEACommand command, const unsigned timeout)
{
	gps_abstime time_started = gps_absolute_time();

	ASH_DEBUG("waiting for reply for command %i", (int)command);

	_command_state = NMEACommandState::waiting;
	_waiting_for_command = command;

	while (_command_state == NMEACommandState::waiting && gps_absolute_time() < time_started + timeout * 1000) {
		receive(timeout);
	}

	return _command_state == NMEACommandState::received ? 0 : -1;
}

int GPSDriverAshtech::configure(unsigned &baudrate, const GPSConfig &config)
{
	_output_mode = config.output_mode;
	_correction_output_activated = false;
	_configure_done = false;

	/* Try different baudrates (115200 is the default for Trimble) and request the baudrate that we want.
	 *
	 * These are Ashtech proprietary commands, we can use them for auto-detection:
	 * $PASHS for setting
	 * $PASHQ for querying
	 * $PASHR for a response
	 */
	const unsigned baudrates_to_try[] = {9600, 38400, 19200, 57600, 115200};
	bool success = false;

	unsigned test_baudrate;

	for (unsigned int baud_i = 0; !success && baud_i < sizeof(baudrates_to_try) / sizeof(baudrates_to_try[0]); baud_i++) {
		test_baudrate = baudrates_to_try[baud_i];

		if (baudrate > 0 && baudrate != test_baudrate) {
			continue; // skip to next baudrate
		}

		setBaudrate(test_baudrate);

		ASH_DEBUG("baudrate set to %i", test_baudrate);

		const char port_config[] = "$PASHQ,PRT\r\n";  // ask for the current port configuration

		for (int run = 0; run < 2; ++run) { // try several times
			write(port_config, sizeof(port_config) - 1);

			if (waitForReply(NMEACommand::PRT, ASH_RESPONSE_TIMEOUT) == 0) {
				ASH_DEBUG("got port for baudrate %i", test_baudrate);
				success = true;
				break;
			}
		}
	}

	if (!success) {
		return -1;
	}

	// We successfully got a response and know to which port we are connected. Now set the desired baudrate
	// if it's different from the current one.
	const unsigned desired_baudrate = 115200; // changing this requires also changing the SPD command

	baudrate = test_baudrate;

	if (baudrate != desired_baudrate) {
		baudrate = desired_baudrate;
		const char baud_config[] = "$PASHS,SPD,%c,9\r\n"; // configure baudrate to 115200
		char baud_config_str[sizeof(baud_config)];
		int len = snprintf(baud_config_str, sizeof(baud_config_str), baud_config, _port);
		write(baud_config_str, len);
		decodeInit();
		receiveWait(200);
		decodeInit();
		setBaudrate(baudrate);

		success = false;

		for (int run = 0; run < 10; ++run) {
			// We ask for the port config again. If we get a reply, we know that the changed settings work.
			const char port_config[] = "$PASHQ,PRT\r\n";
			write(port_config, sizeof(port_config) - 1);

			if (waitForReply(NMEACommand::PRT, ASH_RESPONSE_TIMEOUT) == 0) {
				success = true;
				break;
			}
		}

		if (!success) {
			return -1;
		}

		ASH_DEBUG("Successfully configured the baudrate");
	}


// Additional commands that might be useful:
//		Reading firmware version:
//			$PASHQ,VER
//		Reading installed firmware options:
//			$PASHQ,OPTION
//		The output for the Trimble MB-two is:
//			$PASHR,OPTION,0,SERIAL NUMBER,5730C00370*3E
//			$PASHR,OPTION,@1,GEOFENCING_WW,034017C7114ED*36
//			$PASHR,OPTION,N,GPS,0340173F8924D*66
//			$PASHR,OPTION,G,GLONASS,0340178A9E138*69
//			$PASHR,OPTION,B,BEIDOU,03401434EC35A*4D
//			$PASHR,OPTION,X,L1TRACKING,0340119C547B8*40
//			$PASHR,OPTION,Y,L2TRACKING,034012CD03607*42
//			$PASHR,OPTION,W,20HZ,034016B5A5225*2A
//			$PASHR,OPTION,J,RTKROVER,034010C800693*41
//			$PASHR,OPTION,K,RTKBASE,03401065AB099*7E
//			$PASHR,OPTION,D,DUO,0340138851415*70
//			$PASHR,OPTION,S,L3TRACKING,034011C7AB73D*48
//		Reset the full configuration (however it will lead to a reboot and requires about 15s waiting time)
//			$PASHS,RST

	// get the board identification
	const char board_identification[] = "$PASHQ,RID\r\n";

	if (write(board_identification, sizeof(board_identification) - 1) == sizeof(board_identification) - 1) {
		if (waitForReply(NMEACommand::RID, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", board_identification);
			return -1;
		}
	}

	// Now configure the messages we want

	const char update_rate[] = "$PASHS,POP,20\r\n"; // set internal update rate to 20 Hz

	if (writeAckedCommand(update_rate, sizeof(update_rate) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
		ASH_DEBUG("command %s failed", update_rate);
		// for some reason we don't get a response here
	}

	// Enable dual antenna mode (2: both antennas are L1/L2 GNSS capable, flex mode, avoids the need to determine
	// the baseline length through a prior calibration stage)
	// Needs to be set before other commands
	const bool use_dual_mode = _output_mode != OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two;

	if (use_dual_mode) {
		ASH_DEBUG("Enabling DUO mode");
		const char duo_mode[] = "$PASHS,SNS,DUO,2\r\n";

		if (writeAckedCommand(duo_mode, sizeof(duo_mode) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", duo_mode);
		}

	} else {
		const char solo_mode[] = "$PASHS,SNS,SOL\r\n";

		if (writeAckedCommand(solo_mode, sizeof(solo_mode) - 1, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", solo_mode);
		}
	}

	char buffer[40];
	const char *config_options[] = {
		"$PASHS,NME,ALL,%c,OFF\r\n",    // disable all NMEA and NMEA-Like Messages
		"$PASHS,ATM,ALL,%c,OFF\r\n",    // disable all ATM (ATOM) Messages
		"$PASHS,OUT,%c,ON\r\n",         // enable periodic output
		"$PASHS,NME,ZDA,%c,ON,3\r\n",   // enable ZDA (date & time) output every 3s
		"$PASHS,NME,GST,%c,ON,3\r\n",   // position accuracy messages
		"$PASHS,NME,POS,%c,ON,0.05\r\n",// position & velocity (we can go up to 20Hz if FW option [W] is given and to 50Hz if [8] is given)
		"$PASHS,NME,GSV,%c,ON,1\r\n"    // satellite status
	};

	for (unsigned int conf_i = 0; conf_i < sizeof(config_options) / sizeof(config_options[0]); conf_i++) {
		int len = snprintf(buffer, sizeof(buffer), config_options[conf_i], _port);

		if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", buffer);
			// some commands are not acked (e.g. GSV), so don't make this fatal
		}
	}

	if (use_dual_mode) {
		// enable heading output
		const char heading_output[] = "$PASHS,NME,HDT,%c,ON,0.05\r\n";
		int len = snprintf(buffer, sizeof(buffer), heading_output, _port);

		if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", buffer);
		}
	}

	if (_output_mode == OutputMode::GPSAndRTCM || _output_mode == OutputMode::RTCM) {
		if (!_rtcm_parsing) {
			_rtcm_parsing = new RTCMParsing();
		}

		_rtcm_parsing->reset();
	}

	if (_output_mode == OutputMode::RTCM && _board == AshtechBoard::trimble_mb_two) {
		SurveyInStatus status{};
		status.latitude = status.longitude = (double)NAN;
		status.altitude = NAN;
		status.duration = 0;
		status.mean_accuracy = 0;
		const bool valid = false;
		const bool active = true;
		status.flags = (int)valid | ((int)active << 1);
		surveyInStatus(status);
	}

	if (_output_mode == OutputMode::GPSAndRTCM && _board == AshtechBoard::trimble_mb_two) {
		activateRTCMOutput(false);
	}

	_configure_done = true;
	return 0;
}

void GPSDriverAshtech::activateCorrectionOutput()
{
	if (_correction_output_activated || _output_mode != OutputMode::RTCM) {
		return;
	}

	_correction_output_activated = true;
	char buffer[100];

	if (_base_settings.type == BaseSettingsType::survey_in) {
		ASH_DEBUG("enabling survey-in");

		// setup the base reference: average the position over N seconds
		const char avg_pos[] = "$PASHS,POS,AVG,%i\r\n";
		// alternatively use the current position as reference: "$PASHS,POS,CUR\r\n"
		int len = snprintf(buffer, sizeof(buffer), avg_pos, (int)_base_settings.settings.survey_in.min_dur);

		write(buffer, len);

		if (waitForReply(NMEACommand::RECEIPT, ASH_RESPONSE_TIMEOUT) != 0) {
			ASH_DEBUG("command %s failed", buffer);
		}


		const char *config_options[] = {
			"$PASHS,ANP,OWN,TRM55971.00\r\n",    // set antenna name (arbitrary)
			"$PASHS,STI,0001\r\n"         // enter a base ID
		};


		for (unsigned int conf_i = 0; conf_i < sizeof(config_options) / sizeof(config_options[0]); conf_i++) {
			if (writeAckedCommand(config_options[conf_i], strlen(config_options[conf_i]), ASH_RESPONSE_TIMEOUT) != 0) {
				ASH_DEBUG("command %s failed", config_options[conf_i]);
			}
		}

		_base_settings.settings.survey_in.min_dur = 0; // use it as counter how long survey-in has been active
		_survey_in_start = gps_absolute_time();
		sendSurveyInStatusUpdate(true, false);

	} else {
		ASH_DEBUG("setting base station position");

		const FixedPositionSettings &settings = _base_settings.settings.fixed_position;
		char ns, ew;
		double latitude = settings.latitude;

		if (latitude < 0.) {
			latitude = -latitude;
			ns = 'S';

		} else {
			ns = 'N';
		}

		// convert to ddmm.mmmmmm format
		latitude = ((int)latitude) * 100. + (latitude - ((int)latitude)) * 60.;

		double longitude = settings.longitude;

		if (longitude < 0.) {
			longitude = -longitude;
			ew = 'W';

		} else {
			ew = 'E';
		}

		// convert to ddmm.mmmmmm format
		longitude = ((int)longitude) * 100. + (longitude - ((int)longitude)) * 60.;

		int len = snprintf(buffer, sizeof(buffer), "$PASHS,POS,%.8f,%c,%.8f,%c,%.5f,PC1",
				   latitude, ns, longitude, ew, (double)settings.altitude);

		if (len >= 0 && len < (int)sizeof(buffer)) {
			if (writeAckedCommand(buffer, len, ASH_RESPONSE_TIMEOUT) != 0) {
				ASH_DEBUG("command %s failed", buffer);
			}

		} else {
			ASH_DEBUG("snprintf failed (buffer too short)");
		}

		activateRTCMOutput(true);
		sendSurveyInStatusUpdate(false, true, settings.latitude, settings.longitude, settings.altitude);
	}
}

void
GPSDriverAshtech::sendSurveyInStatusUpdate(bool active, bool valid, double latitude, double longitude, float altitude)
{
	SurveyInStatus status{};
	status.latitude = latitude;
	status.longitude = longitude;
	status.altitude = altitude;
	status.duration = _base_settings.settings.survey_in.min_dur;
	status.mean_accuracy = 0; // unknown
	status.flags = (int)valid | ((int)active << 1);
	surveyInStatus(status);
}
