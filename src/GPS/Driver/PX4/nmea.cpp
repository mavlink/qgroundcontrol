/****************************************************************************
 *
 *   Copyright (c) 2020 - 2024 PX4 Development Team. All rights reserved.
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

/**
 * @file nmea.cpp
 *
 * NMEA protocol implementation.
 *
 * @author WeiPeng Guo <guoweipeng1990@sina.com>
 * @author Stone White <stone@thone.io>
 * @author Jose Jimenez-Berni <berni@ias.csic.es>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctime>

#include "nmea.h"
#include "rtcm.h"

#ifndef M_PI_F
# define M_PI_F 3.14159265358979323846f
#endif

#define MAX(X,Y)    ((X) > (Y) ? (X) : (Y))
#define NMEA_UNUSED(x) (void)x;

/**** Warning macros, disable to save memory */
#define NMEA_WARN(...)         {GPS_WARN(__VA_ARGS__);}
#define NMEA_DEBUG(...)        {/*GPS_WARN(__VA_ARGS__);*/}

GPSDriverNMEA::GPSDriverNMEA(GPSCallbackPtr callback, void *callback_user,
			     sensor_gps_s *gps_position,
			     satellite_info_s *satellite_info,
			     float heading_offset) :
	GPSHelper(callback, callback_user),
	_gps_position(gps_position),
	_satellite_info(satellite_info),
	_heading_offset(heading_offset)
{
	decodeInit();
}

GPSDriverNMEA::~GPSDriverNMEA()
{
	delete _rtcm_parsing;
}

/*
 * All NMEA descriptions are taken from
 * http://www.trimble.com/OEM_ReceiverHelp/V4.44/en/NMEA-0183messages_MessageOverview.html
 */

void GPSDriverNMEA::setTimeUtc(int year, int month, int day, double utc_hhmmss)
{
	int utc_hour = static_cast<int>(utc_hhmmss / 10000);
	int utc_minute = static_cast<int>((utc_hhmmss - utc_hour * 10000) / 100);
	double utc_sec = static_cast<double>(utc_hhmmss - utc_hour * 10000 - utc_minute * 100);
	uint64_t usecs = static_cast<uint64_t>((utc_sec - static_cast<uint64_t>(utc_sec)) * 1000000);

	tm timeinfo{};
	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = month - 1;
	timeinfo.tm_mday = day;
	timeinfo.tm_hour = utc_hour;
	timeinfo.tm_min = utc_minute;
	timeinfo.tm_sec = int(utc_sec);

	_gps_position->time_utc_usec = timeFromUtc(timeinfo, usecs * 1000, !_clock_set);

	if (_gps_position->time_utc_usec != 0) {
		_clock_set = true;
	}
}

int GPSDriverNMEA::handleMessage(int len)
{
	if (len < 7) {
		return 0;
	}

	int fieldCount = 0;

	for (int i = 0 ; i < len; i++) {
		if (_rx_buffer[i] == ',') { fieldCount++; }
	}

	char *bufptr = (char *)(_rx_buffer + 6);
	int ret = 0;

	if ((memcmp(_rx_buffer + 3, "ZDA,", 4) == 0) && (fieldCount == 6)) {
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
		double utc_time = 0.0;
		int day = 0, month = 0, year = 0, local_time_off_hour = 0, local_time_off_min = 0;
		NMEA_UNUSED(local_time_off_hour);
		NMEA_UNUSED(local_time_off_min);

		nmeaNextField(bufptr, utc_time);

		nmeaNextField(bufptr, day);

		nmeaNextField(bufptr, month);

		nmeaNextField(bufptr, year);

		nmeaNextField(bufptr, local_time_off_hour);

		nmeaNextField(bufptr, local_time_off_min);

		setTimeUtc(year, month, day, utc_time);
		_last_timestamp_time = gps_absolute_time();
		_TIME_received = true;

	} else if ((memcmp(_rx_buffer + 3, "GLL,", 4) == 0) && (fieldCount >= 7)) {
		/*
		  Latitude and Londitude data
		  Example:
		  $xxGLL,Lat,N/S,long,E/W,timestamp,A/V data validity,A/V mode indicator
		  $GNGLL,3150.712345,N,11711.912345,E,062735.00,A,A*76
		*/
		double utc_time = 0.0, lat = 0.0, lon = 0.0;
		char ns = '?', ew = '?', dvalid = '?', modeind = '?';

		nmeaNextField(bufptr, lat);

		nmeaNextField(bufptr, ns);

		nmeaNextField(bufptr, lon);

		nmeaNextField(bufptr, ew);

		nmeaNextField(bufptr, utc_time);

		nmeaNextField(bufptr, dvalid);

		nmeaNextField(bufptr, modeind);

		if (ns == 'S') {
			lat = -lat;
		}

		if (ew == 'W') {
			lon = -lon;
		}


		/* only update the values if they are valid */
		if (dvalid == 'A' && modeind == 'A') {
			_gps_position->longitude_deg = nmeaToDegrees(lon);
			_gps_position->latitude_deg = nmeaToDegrees(lat);

			if (!_POS_received && (_last_POS_timeUTC < utc_time)) {
				_last_POS_timeUTC = utc_time;
				_POS_received = true;
				_rate_count_lat_lon++;
			}

		}

	} else if ((memcmp(_rx_buffer + 3, "GGA,", 4) == 0) && (fieldCount >= 14)) {
		/*
		  Time, position, and fix related data
		  An example of the GGA message string is:
		  $xxGGA,time,lat,NS,long,EW,quality,numSV,HDOP,alt,M,sep,M,diffAge,diffStation*cs
		  $GPGGA,172814.0,3723.46587704,N,12202.26957864,W,2,6,1.2,18.893,M,-25.669,M,2.0,0031*4F
		  $GNGGA,092721.00,2926.688113,N,11127.771644,E,2,08,1.11,106.3,M,-20,M,1.0,3721*53

		  Note - The data string exceeds the nmea standard length.
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
		*/
		double utc_time = 0.0, lat = 0.0, lon = 0.0;
		float alt = 0.f, geoid_h = 0.f;
		float hdop = 99.9f, dgps_age = NAN;
		int  num_of_sv = 0, fix_quality = 0;
		char ns = '?', ew = '?';

		NMEA_UNUSED(dgps_age);

		nmeaNextField(bufptr, utc_time);

		nmeaNextField(bufptr, lat);

		nmeaNextField(bufptr, ns);

		nmeaNextField(bufptr, lon);

		nmeaNextField(bufptr, ew);

		nmeaNextField(bufptr, fix_quality);

		nmeaNextField(bufptr, num_of_sv);

		nmeaNextField(bufptr, hdop);

		nmeaNextField(bufptr, alt);

		while (*(++bufptr) != ',') {} //skip M

		nmeaNextField(bufptr, geoid_h);

		while (*(++bufptr) != ',') {} //skip M

		nmeaNextField(bufptr, dgps_age);

		if (ns == 'S') {
			lat = -lat;
		}

		if (ew == 'W') {
			lon = -lon;
		}

		/* convert from degrees, minutes and seconds to degrees */
		_gps_position->longitude_deg = nmeaToDegrees(lon);
		_gps_position->latitude_deg = nmeaToDegrees(lat);
		_gps_position->hdop = hdop;
		_gps_position->altitude_msl_m = (double)alt;
		_gps_position->altitude_ellipsoid_m = (double)(alt + geoid_h);
		_sat_num_gga = static_cast<int>(num_of_sv);


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

		if (!_POS_received && (_last_POS_timeUTC < utc_time)) {
			_last_POS_timeUTC = utc_time;
			_gps_position->timestamp = gps_absolute_time();
			_POS_received = true;
			_rate_count_lat_lon++;
		}

		_ALT_received = true;
		_SVNUM_received = true;
		_FIX_received = true;

		_gps_position->c_variance_rad = 0.1f;

	} else if (memcmp(_rx_buffer + 3, "HDT,", 4) == 0 && fieldCount == 2) {
		/*
		Heading message
		Example $GPHDT,121.2,T*35

		f1 Last computed heading value, in degrees (0-359.99)
		T "T" for "True"
		 */

		float heading_deg = 0.f;

		if (nmeaNextField(bufptr, heading_deg)) {
			handleHeading(heading_deg, NAN);
		}

		_HEAD_received = true;

	} else if ((memcmp(_rx_buffer + 3, "GNS,", 4) == 0) && (fieldCount >= 12)) {

		/*
		Message GNS
		Type Output Message
		Time and position, together with GNSS fixing related data (number of satellites in use, and
		the resulting HDOP, age of differential data if in use, etc.).
		Message Structure:
		$xxGNS,time,lat,NS,long,EW,posMode,numSV,HDOP,alt,altRef,diffAge,diffStation,navStatus*cs<CR><LF>
		Example:
		$GPGNS,091547.00,5114.50897,N,00012.28663,W,AA,10,0.83,111.1,45.6,,,V*71
		$GNGNS,092721.00,2926.68811,N,11127.77164,E,DNNN,08,1.11,106.3,-20,1.0,3721,V*0D
		$GNGNS,182243.00,4908.088781,N,12233.7501,W,AAAAN,24,0.6,191.8178,-33.6291,,,S*50

		FieldNo.  Name    Unit     Format                  Example Description
		0        xxGNS    -       string            $GPGNS GNS Message ID (xx = current Talker ID)
		1        time     -       hhmmss.ss         091547.00 UTC time, see note on UTC representation
		2        lat      -       ddmm.mmmmm        5114.50897 Latitude (degrees & minutes), see format description
		3        NS       -       character         N North/South indicator
		4        long     -       dddmm.mmmmm       00012.28663 Longitude (degrees & minutes), see format description
		5        EW       -       character         E East/West indicator
		6      posMode    -       character         AA Positioning mode, see position fix flags description. First character for GPS, second character forGLONASS
		7       numSV     -       numeric         10 Number of satellites used (range: 0-99)
		8         HDOP    -       numeric         0.83 Horizontal Dilution of Precision
		9         alt     m       numeric         111.1 Altitude above mean sea level
		10        sep    m        numeric         45.6 Geoid separation: difference between ellipsoid and mean sea level UBX-18010854 - R05 Advance Information Page 18 of 262 u-blox ZED-F9P Interface Description - Manual GNS continued
		11    diffAge    s        numeric         - Age of differential corrections (blank when DGPS is not used)
		12 diffStation   -        numeric         - ID of station providing differential corrections (blank when DGPS is not used)
		13 navStatus    -         character         V Navigational status indicator (V = Equipment is not providing navigational status information) NMEA v4.10 and above only
		14 cs - hexadecimal *71   Checksum
		15 <CR><LF> - character - Carriage return and line feed
		*/
		double utc_time = 0.0;
		double lat = 0.0, lon = 0.0;
		char pos_Mode[5] = {'N', 'N', 'N', 'N', 'N'};
		int num_of_sv = 0;
		float alt = 0.f, geoid_h = 0.f;
		float hdop = 0.f;
		char ns = '?', ew = '?';
		int i = 0;
		NMEA_UNUSED(pos_Mode);

		nmeaNextField(bufptr, utc_time);

		nmeaNextField(bufptr, lat);

		nmeaNextField(bufptr, ns);

		nmeaNextField(bufptr, lon);

		nmeaNextField(bufptr, ew);

		/* as more GPS systems are added this string can grow, so only parse out the first X, but keep going until we hit the end of field */
		do {
			if (i < 5) { pos_Mode[i++] =  *(bufptr); }

		} while (*(++bufptr) != ',');

		nmeaNextField(bufptr, num_of_sv);

		nmeaNextField(bufptr, hdop);

		nmeaNextField(bufptr, alt);

		nmeaNextField(bufptr, geoid_h);

		if (ns == 'S') {
			lat = -lat;
		}

		if (ew == 'W') {
			lon = -lon;
		}

		/* convert from degrees, minutes and seconds to degrees */
		_gps_position->latitude_deg = nmeaToDegrees(lat);
		_gps_position->longitude_deg = nmeaToDegrees(lon);
		_gps_position->hdop = hdop;
		_gps_position->altitude_msl_m = (double)alt;
		_gps_position->altitude_ellipsoid_m = (double)(alt + geoid_h);
		_sat_num_gns = static_cast<int>(num_of_sv);

		if (!_POS_received && (_last_POS_timeUTC < utc_time)) {
			_last_POS_timeUTC = utc_time;
			_gps_position->timestamp = gps_absolute_time();
			_POS_received = true;
			_rate_count_lat_lon++;
		}

		_ALT_received = true;
		_SVNUM_received = true;


	} else if ((memcmp(_rx_buffer + 3, "RMC,", 4) == 0) && (fieldCount >= 11)) {

		/*
		Position, velocity, and time
		The RMC string is:

		$xxRMC,time,status,lat,NS,long,EW,spd,cog,date,mv,mvEW,posMode,navStatus*cs<CR><LF>
		The Talker ID ($--) will vary depending on the satellite system used for the position solution:
		$GNRMC,092721.00,A,2926.688113,N,11127.771644,E,0.780,,200520,,,D,V*1D

		GPRMC message fields
		Field	Meaning
		0	Message ID $GPRMC
		1	UTC of position fix
		2	Status A=active or V=void
		3	Latitude
		4	Longitude
		5	Speed over the ground in knots
		6	Track angle in degrees (True)
		7	Date
		8	Magnetic variation in degrees
		9	The checksum data, always begins with *
		*/
		double utc_time = 0.0;
		char Status = 'V';
		double lat = 0.0, lon = 0.0;
		float ground_speed_K = 0.f;
		float track_true = 0.f;
		int nmea_date = 0;
		float Mag_var = 0.f;
		char ns = '?', ew = '?';
		NMEA_UNUSED(Mag_var);

		nmeaNextField(bufptr, utc_time);

		nmeaNextField(bufptr, Status);

		nmeaNextField(bufptr, lat);

		nmeaNextField(bufptr, ns);

		nmeaNextField(bufptr, lon);

		nmeaNextField(bufptr, ew);

		nmeaNextField(bufptr, ground_speed_K);

		nmeaNextField(bufptr, track_true);

		nmeaNextField(bufptr, nmea_date);

		nmeaNextField(bufptr, Mag_var);

		if (ns == 'S') {
			lat = -lat;
		}

		if (ew == 'W') {
			lon = -lon;
		}

		if (Status == 'V') {
			_gps_position->fix_type = 0;
		}

		float track_rad = track_true * M_PI_F / 180.0f; // rad in range [0, 2pi]

		if (track_rad > M_PI_F) {
			track_rad -= 2.f * M_PI_F; // rad in range [-pi, pi]
		}

		float velocity_ms = ground_speed_K / 1.9438445f;
		float velocity_north = velocity_ms * cosf(track_rad);
		float velocity_east  = velocity_ms * sinf(track_rad);

		_gps_position->cog_rad = track_rad;
		_gps_position->c_variance_rad = 0.1f;

		if (!_unicore_parser.agricaValid()) {
			// We ignore RMC position for Unicore, because we have GGA configured at the rate we want.

			/* convert from degrees, minutes and seconds to degrees */
			_gps_position->latitude_deg = nmeaToDegrees(lat);
			_gps_position->longitude_deg = nmeaToDegrees(lon);

			if (!_POS_received && (_last_POS_timeUTC < utc_time)) {
				_gps_position->timestamp = gps_absolute_time();
				_last_POS_timeUTC = utc_time;
				_POS_received = true;
				_rate_count_lat_lon++;
			}

			_gps_position->vel_m_s = velocity_ms;
			_gps_position->vel_n_m_s = velocity_north;
			_gps_position->vel_e_m_s = velocity_east;
			_gps_position->vel_ned_valid = true; /**< Flag to indicate if NED speed is valid */
			_gps_position->s_variance_m_s = 0;

			if (!_VEL_received && (_last_VEL_timeUTC < utc_time)) {
				_last_VEL_timeUTC = utc_time;
				_VEL_received = true;
				_rate_count_vel++;
			}
		}

		int nmea_day = static_cast<int>(nmea_date / 10000);
		int nmea_mth = static_cast<int>((nmea_date - nmea_day * 10000) / 100);
		int nmea_year = static_cast<int>(nmea_date - nmea_day * 10000 - nmea_mth * 100);
		setTimeUtc(nmea_year + 2000, nmea_mth, nmea_day, utc_time);

		_last_timestamp_time = gps_absolute_time();
		_TIME_received = true;

	}	else if ((memcmp(_rx_buffer + 3, "GST,", 4) == 0) && (fieldCount == 8)) {

		/*
		Position error statistics
		An example of the GST message string is:

		$GPGST,172814.0,0.006,0.023,0.020,273.6,0.023,0.020,0.031*6A
		$GNGST,091200.54,45,,,,1.2,0.77,2.2*70
		$GNGST,092720.50,43,,,,2.6,2.6,5.9*49

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
		double utc_time = 0.0;
		float lat_err = 0.f, lon_err = 0.f, alt_err = 0.f;
		float min_err = 0.f, maj_err = 0.f, deg_from_north = 0.f, rms_err = 0.f;

		NMEA_UNUSED(utc_time);
		NMEA_UNUSED(min_err);
		NMEA_UNUSED(maj_err);
		NMEA_UNUSED(deg_from_north);
		NMEA_UNUSED(rms_err);

		nmeaNextField(bufptr, utc_time);

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

		_EPH_received = true;

	} else if ((memcmp(_rx_buffer + 3, "GSA,", 4) == 0) && (fieldCount >= 17)) {

		/*
		GPS DOP and active satellites
		An example of the GSA message string is:
		$GPGSA,<1>,<2>,<3>,<3>,,,,,<3>,<3>,<3>,<4>,<5>,<6>*<7><CR><LF>
		$GNGSA,A,3,82,67,74,68,73,80,83,,,,,,0.99,0.53,0.84,2*09
		$GNGSA,A,3,12,19,06,17,02,09,28,05,,,,,2.38,1.10,2.11,1*05
		$GNGSA,A,3,27,04,16,08,09,26,31,11,,,,,1.96,1.05,1.65,1*08

		GSA message fields
		Field	Meaning
		0	Message ID $GPGSA
		1	Mode 1, M = manual, A = automatic
		2	Mode 2, Fix type, 1 = not available, 2 = 2D, 3 = 3D
		3	PRN number, 01 through 32 for GPS, 33 through 64 for SBAS, 64+ for GLONASS
		4 	PDOP: 0.5 through 99.9
		5	HDOP: 0.5 through 99.9
		6	VDOP: 0.5 through 99.9
		7	The checksum data, always begins with *
		*/
		char M_pos = ' ';
		int fix_mode = 0;
		int sat_id[12] {0};
		float pdop = 99.9f, hdop = 99.9f, vdop = 99.9f;

		NMEA_UNUSED(M_pos);
		NMEA_UNUSED(sat_id);
		NMEA_UNUSED(pdop);

		nmeaNextField(bufptr, M_pos);

		nmeaNextField(bufptr, fix_mode);

		for (int y = 0; y < 12; y++) {
			nmeaNextField(bufptr, sat_id[y]);
		}

		nmeaNextField(bufptr, pdop);

		nmeaNextField(bufptr, hdop);

		nmeaNextField(bufptr, vdop);

		if (fix_mode <= 1) {
			_gps_position->fix_type = 0;

		} else {
			_gps_position->hdop = static_cast<float>(hdop);
			_gps_position->vdop = static_cast<float>(vdop);
			_DOP_received = true;

		}


	} else if ((memcmp(_rx_buffer + 3, "GSV,", 4) == 0) && (fieldCount >= 3)) {
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

		int all_page_num = 0, this_page_num = 0, tot_sv_visible = 0;
		struct gsv_sat {
			int svid;
			int elevation;
			int azimuth;
			int snr;
		} sat[4] {};

		nmeaNextField(bufptr, all_page_num);

		nmeaNextField(bufptr, this_page_num);

		nmeaNextField(bufptr, tot_sv_visible);

		if ((this_page_num < 1) || (this_page_num > all_page_num) || (tot_sv_visible < 0)) {
			return 0;
		}

		const uint64_t page_start = static_cast<uint64_t>(this_page_num - 1) * 4;

		if (page_start > static_cast<uint64_t>(tot_sv_visible)) {
			return 0;
		}

		if (memcmp(_rx_buffer, "$GP", 3) == 0) {
			_sat_num_gpgsv = tot_sv_visible;

		} else if (memcmp(_rx_buffer, "$GL", 3) == 0) {
			_sat_num_glgsv = tot_sv_visible;

		} else if (memcmp(_rx_buffer, "$GA", 3) == 0) {
			_sat_num_gagsv = tot_sv_visible;

		} else if (memcmp(_rx_buffer, "$GB", 3) == 0) {
			_sat_num_gbgsv = tot_sv_visible;

		} else if (memcmp(_rx_buffer, "$BD", 3) == 0) {
			_sat_num_bdgsv = tot_sv_visible;

		}

		if (this_page_num == 0 && _satellite_info) {
			memset(_satellite_info->svid,     0, sizeof(_satellite_info->svid));
			memset(_satellite_info->used,     0, sizeof(_satellite_info->used));
			memset(_satellite_info->snr,      0, sizeof(_satellite_info->snr));
			memset(_satellite_info->elevation, 0, sizeof(_satellite_info->elevation));
			memset(_satellite_info->azimuth,  0, sizeof(_satellite_info->azimuth));
		}

		const int remaining_satellites = tot_sv_visible - static_cast<int>(page_start);
		const int satellites_in_message = (fieldCount - 3) / 4;
		const int satellites_in_page = remaining_satellites < satellites_in_message ?
					       remaining_satellites : satellites_in_message;
		const int satellites_to_parse = satellites_in_page < 4 ? satellites_in_page : 4;

		if (this_page_num == all_page_num) {
			_SVNUM_received = true;
			_SVINFO_received = true;

			if (_satellite_info) {
				_satellite_info->count = tot_sv_visible < satellite_info_s::SAT_INFO_MAX_SATELLITES ?
							 tot_sv_visible : satellite_info_s::SAT_INFO_MAX_SATELLITES;
				_satellite_info->timestamp = gps_absolute_time();
				ret |= 2;
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
					_satellite_info->snr[satellite_index]       = sat[y].snr;
					_satellite_info->elevation[satellite_index] = sat[y].elevation;
					_satellite_info->azimuth[satellite_index]   = sat[y].azimuth;
				}
			}
		}


	} else if ((memcmp(_rx_buffer + 3, "VTG,", 4) == 0) && (fieldCount >= 8)) {

		/*$GNVTG,,T,,M,0.683,N,1.265,K*30
		  $GNVTG,,T,,M,0.780,N,1.445,K*33

		Field	Meaning
		0	Message ID $GPVTG
		1	Track made good (degrees true)
		2	T: track made good is relative to true north
		3	Track made good (degrees magnetic)
		4	M: track made good is relative to magnetic north
		5	Speed, in knots
		6	N: speed is measured in knots
		7	Speed over ground in kilometers/hour (kph)
		8	K: speed over ground is measured in kph
		9	The checksum data, always begins with *
		*/

		float track_true = 0.f;
		char T;
		float track_mag = 0.f;
		char M;
		float ground_speed = 0.f;
		char N;
		float ground_speed_K = 0.f;
		char K;
		NMEA_UNUSED(T);
		NMEA_UNUSED(track_mag);
		NMEA_UNUSED(M);
		NMEA_UNUSED(N);
		NMEA_UNUSED(ground_speed_K);
		NMEA_UNUSED(K);

		nmeaNextField(bufptr, track_true);

		nmeaNextField(bufptr, T);

		nmeaNextField(bufptr, track_mag);

		nmeaNextField(bufptr, M);

		nmeaNextField(bufptr, ground_speed);

		nmeaNextField(bufptr, N);

		nmeaNextField(bufptr, ground_speed_K);

		nmeaNextField(bufptr, K);

		float track_rad = track_true * M_PI_F / 180.0f; // rad in range [0, 2pi]

		if (track_rad > M_PI_F) {
			track_rad -= 2.f * M_PI_F; // rad in range [-pi, pi]
		}

		if (!_unicore_parser.agricaValid()) {
			float velocity_ms = ground_speed / 1.9438445f;
			float velocity_north = velocity_ms * cosf(track_rad);
			float velocity_east  = velocity_ms * sinf(track_rad);
			_gps_position->vel_m_s = velocity_ms;
			_gps_position->vel_n_m_s = velocity_north;
			_gps_position->vel_e_m_s = velocity_east;
			_gps_position->vel_ned_valid = true; /** Flag to indicate if NED speed is valid */
			_gps_position->c_variance_rad = 0.1f;
			_gps_position->s_variance_m_s = 0;

			if (!_VEL_received) {
				_VEL_received = true;
				_rate_count_vel++;
			}

			_gps_position->cog_rad = track_rad;
		}

	} else {
		NMEA_DEBUG("Unable to parse %c%c%c%c message", _rx_buffer[3], _rx_buffer[4], _rx_buffer[5], _rx_buffer[6]);
	}

	if (_sat_num_gga > 0) {
		_gps_position->satellites_used = _sat_num_gga;

	} else if (_SVNUM_received && _SVINFO_received && _FIX_received) {

		_sat_num_gsv = _sat_num_gpgsv + _sat_num_glgsv + _sat_num_gagsv
			       + _sat_num_gbgsv + _sat_num_bdgsv;
		_gps_position->satellites_used = MAX(_sat_num_gns, _sat_num_gsv);
	}

	if (_VEL_received && _POS_received) {
		_gps_position->timestamp_time_relative = (int32_t)(_last_timestamp_time - _gps_position->timestamp);
		ret |= 1;
		_VEL_received = false;
		_POS_received = false;
	}

	return ret;
}

int GPSDriverNMEA::receive(unsigned timeout)
{
	uint8_t buf[GPS_READ_BUFFER_SIZE];

	/* timeout additional to poll */
	gps_abstime time_started = gps_absolute_time();

	int handled = 0;

	while (true) {
		int ret = read(buf, sizeof(buf), timeout);

		if (ret < 0) {
			/* something went wrong when polling or reading */
			if (ret != ReadCancelled) {
				NMEA_WARN("poll_or_read err");
			}
			return -1;

		} else if (ret != 0) {

			/* pass received bytes to the packet decoder */
			for (int i = 0; i < ret; i++) {
				int l = parseChar(buf[i]);

				if (l > 0) {
					handled |= handleMessage(l);
				}

				UnicoreParser::Result result = _unicore_parser.parseChar(buf[i]);

				if (result == UnicoreParser::Result::GotHeading) {

					// Don't mark this as handled, just publish it with position later.

					_unicore_heading_received_last = gps_absolute_time();

					// Unicore seems to publish heading and standard deviation of 0
					// to signal that it has not initialized the heading yet.
					if (_unicore_parser.heading().heading_stddev_deg > 0.0f) {
						// Unicore publishes the heading between True North and
						// the baseline vector from master antenna to slave
						// antenna.
						// Assuming that the master is in front and the slave
						// in the back, this means that we need to flip the
						// heading 180 degrees.

						handleHeading(
							_unicore_parser.heading().heading_deg + 180.0f,
							_unicore_parser.heading().heading_stddev_deg);
					}

					NMEA_DEBUG("Got heading: %.1f deg, stddev: %.1f deg, baseline: %.2f m\n",
						   (double)_unicore_parser.heading().heading_deg,
						   (double)_unicore_parser.heading().heading_stddev_deg,
						   (double)_unicore_parser.heading().baseline_m);

					// We don't specifically publish this but it's just added with the next position
					// update.

				} else if (result == UnicoreParser::Result::GotAgrica) {

					// Don't mark this as handled, just publish it with position later.

					// Receiving this message tells us that we are talking to a UM982. If
					// UNIHEADINGA is not configured by default, we request it now.

					if (gps_absolute_time() - _unicore_heading_received_last > 1000000) {
						request_unicore_messages();
					}

					_gps_position->vel_m_s = _unicore_parser.agrica().velocity_m_s;
					_gps_position->vel_n_m_s = _unicore_parser.agrica().velocity_north_m_s;
					_gps_position->vel_e_m_s = _unicore_parser.agrica().velocity_east_m_s;
					_gps_position->vel_d_m_s = -_unicore_parser.agrica().velocity_up_m_s;
					// s_variance_m_s contains a 1-sigma speed accuracy in m/s.
					_gps_position->s_variance_m_s = sqrtf(
						(_unicore_parser.agrica().stddev_velocity_north_m_s * _unicore_parser.agrica().stddev_velocity_north_m_s +
						 _unicore_parser.agrica().stddev_velocity_east_m_s * _unicore_parser.agrica().stddev_velocity_east_m_s +
						 _unicore_parser.agrica().stddev_velocity_up_m_s * _unicore_parser.agrica().stddev_velocity_up_m_s));

					_gps_position->cog_rad = atan2f(
									 _unicore_parser.agrica().velocity_north_m_s,
									 _unicore_parser.agrica().velocity_east_m_s);

					_gps_position->vel_ned_valid = true;
					_VEL_received = true;
					_rate_count_vel++;

					// We don't specifically publish this but it's just added with the next position
					// update.
				}
			}

			if (handled > 0) {
				return handled;
			}
		}

		/* abort after timeout if no useful packets received */
		if (time_started + timeout * 1000 < gps_absolute_time()) {
			return -1;
		}
	}
}

void GPSDriverNMEA::handleHeading(float heading_deg, float heading_stddev_deg)
{
	float heading_rad = heading_deg * M_PI_F / 180.0f; // rad in range [0, 2pi]
	heading_rad -= _heading_offset; // rad in range [-pi, 3pi]

	if (heading_rad > M_PI_F) {
		heading_rad -= 2.f * M_PI_F; // rad in range [-pi, pi]
	}

	// We are not publishing heading_offset because it wasn't done in the past,
	// and the UBX driver doesn't do it either. I'm assuming it would cause the
	// offset to be applied twice.

	_gps_position->heading = heading_rad;

	const float heading_stddev_rad = heading_stddev_deg * M_PI_F / 180.0f;
	_gps_position->heading_accuracy = heading_stddev_rad;
}

void GPSDriverNMEA::request_unicore_messages()
{
	// Configure position messages on serial port. Don't save it though.
	{
		// position
		uint8_t buf[] = "GPGGA COM1 0.2\r\n";
		write(buf, sizeof(buf) - 1);
	}

	{
		// velocity
		uint8_t buf[] = "UNIAGRICA COM1 0.2\r\n";
		write(buf, sizeof(buf) - 1);
	}

	{
		// heading
		uint8_t buf[] = "UNIHEADINGA COM1 0.2\r\n";
		write(buf, sizeof(buf) - 1);
	}

	{
		// eph, epv
		uint8_t buf[] = "GPGST COM1 1.0\r\n";
		write(buf, sizeof(buf) - 1);
	}

	{
		// vdop
		uint8_t buf[] = "GPGSA COM1 1.0\r\n";
		write(buf, sizeof(buf) - 1);
	}

	{
		// time
		uint8_t buf[] = "GPRMC COM1 1.0\r\n";
		write(buf, sizeof(buf) - 1);
	}
}

#define HEXDIGIT_CHAR(d) ((char)((d) + (((d) < 0xA) ? '0' : 'A'-0xA)))

int GPSDriverNMEA::parseChar(uint8_t b)
{
	int iRet = 0;

	if (_rtcm_parsing) {
		if (_rtcm_parsing->addByte(b)) {
			NMEA_DEBUG("got RTCM message with length %i", (int)_rtcm_parsing->messageLength());
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

void GPSDriverNMEA::decodeInit()
{
	_rx_buffer_bytes = 0;
	_decode_state = NMEADecodeState::uninit;
}

int GPSDriverNMEA::configure(unsigned &baudrate, const GPSConfig &config)
{
	_output_mode = config.output_mode;

	if (_output_mode != OutputMode::GPS) {
		NMEA_WARN("RTCM output have to be configured manually");

		if (!_rtcm_parsing) {
			_rtcm_parsing = new RTCMParsing();
		}

		_rtcm_parsing->reset();
	}

	// If a baudrate is defined, we test this first
	if (baudrate > 0) {
		setBaudrate(baudrate);
		decodeInit();
		int ret = receive(400);
		gps_usleep(2000);

		// If a valid POS message is received we have GPS
		if (_POS_received || ret > 0) {
			return 0;
		}
	}

	// If we haven't found the GPS with the defined baudrate, we try other rates
	const unsigned baudrates_to_try[] = {9600, 19200, 38400, 57600, 115200, 230400};
	unsigned test_baudrate;

	for (unsigned int baud_i = 0; !_POS_received
	     && baud_i < sizeof(baudrates_to_try) / sizeof(baudrates_to_try[0]); baud_i++) {

		test_baudrate = baudrates_to_try[baud_i];
		setBaudrate(test_baudrate);

		NMEA_DEBUG("baudrate set to %i", test_baudrate);

		decodeInit();
		int ret = receive(400);
		gps_usleep(2000);

		// If a valid POS message is received we have GPS
		if (_POS_received || ret > 0) {
			return 0;
		}
	}

	// If nothing is found we leave the specified or default
	if (baudrate > 0) {
		return setBaudrate(baudrate);
	}

	return setBaudrate(NMEA_DEFAULT_BAUDRATE);
}
