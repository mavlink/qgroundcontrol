/****************************************************************************
 *
 *   Copyright (c) 2012-2019 PX4 Development Team. All rights reserved.
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
 * @file emlid_reach.cpp
 *
 * @author Bastien Auneau <bastien.auneau@while-true.fr>
 */


#include "emlid_reach.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctime>
#include <cstring>
#include <errno.h>

#include <stdlib.h>

//// ERB
// 'E' Ox45 | 'R' Ox52 | ID | LENGTH (2 Bytes little endian) | payload | CHECKSUM (2B)
#define ERB_SYNC_1 0x45 // E
#define ERB_SYNC_2 0x52 // R

#define ERB_ID_VERSION         0x01
#define ERB_ID_GEODIC_POSITION 0x02
#define ERB_ID_NAV_STATUS      0x03
#define ERB_ID_DOPS            0x04
#define ERB_ID_VELOCITY_NED    0x05
#define ERB_ID_SPACE_INFO      0x06 // not used, reduces stack usage
#define ERB_ID_RTK             0x07 // RTK, TODO


#define EMLID_UNUSED(x) (void)x;

#define GPS_PI 3.141592653589793238462643383280f

#define AUTO_DETECT_MAX_READ_SENTENCE 5  // if more detect succeed


GPSDriverEmlidReach::GPSDriverEmlidReach(GPSCallbackPtr callback, void *callback_user,
		sensor_gps_s *gps_position, satellite_info_s *satellite_info) :
	GPSHelper(callback, callback_user),
	_gps_position(gps_position), _satellite_info(satellite_info)
{}


int
GPSDriverEmlidReach::configure(unsigned &baudrate, const GPSConfig &config)
{
	// TODO RTK
	if (config.output_mode != OutputMode::GPS) {
		GPS_WARN("EMLIDREACH: Unsupported Output Mode %i", (int)config.output_mode);
		return -1;
	}

	unsigned baud_allowed[] {57600, 115200, 230400};

	for (unsigned i = 0; i < sizeof(baud_allowed) / sizeof(baud_allowed[0]); i++) {
		if (baudrate > 0 && baudrate != baud_allowed[i]) {
			continue;
		}

		_sentence_cnt = 0;
		_erb_decode_state  = ERB_State::init;
		_erb_payload_len = 0;
		_erb_buff_cnt = 0;

		if (GPSHelper::setBaudrate(baud_allowed[i]) != 0) {
			continue;
		}

		if (! testConnection()) {
			continue;
		}

		baudrate = baud_allowed[i];
		return 0;
	}

	return -1;
}


bool
GPSDriverEmlidReach::testConnection()
{
	_testing_connection = true;
	receive(500);
	_testing_connection = false;
	return _sentence_cnt >= AUTO_DETECT_MAX_READ_SENTENCE;
}


int
GPSDriverEmlidReach::receive(unsigned timeout)
{
	uint8_t read_buff[GPS_READ_BUFFER_SIZE];
	unsigned read_buff_len = 0;
	uint8_t read_ind = 0;
	int return_status = 0;

	gps_abstime time_started = gps_absolute_time();

	while (true) {
		// read from serial, timeout may be truncated further in read()
		read_buff_len = read(read_buff, sizeof(read_buff), timeout);

		if (read_buff_len > 0) {
			read_ind = 0;
		}

		// process data in buffer return by read()
		while (read_ind < read_buff_len) {
			int ret = 0;
			ret = erbParseChar(read_buff[read_ind++]);

			if (ret > 0) {
				_sentence_cnt ++;

				// when testig connection, we care about syntax not semantic
				if (! _testing_connection) {
					return_status = handleErbSentence();
				}
			}
		}

		if (return_status > 0) {
			return return_status;
		}

		/* abort after timeout if no useful packets received */
		if (time_started + timeout * 1000 < gps_absolute_time()) {
			// device timed out
			return -1;
		}
	}

	return -1;
}


//// ERB

int
GPSDriverEmlidReach::erbParseChar(uint8_t b)
{
	int ret = 0;
	uint8_t *buff_ptr = (uint8_t *)&_erb_buff;

	switch (_erb_decode_state) {
	case ERB_State::init:
		if (b == ERB_SYNC_1) {
			_erb_buff_cnt = 0;
			buff_ptr[_erb_buff_cnt ++] = b;
			_erb_decode_state = ERB_State::got_sync_1;
		}

		break;

	case ERB_State::got_sync_1:
		if (b == ERB_SYNC_2) {
			buff_ptr[_erb_buff_cnt ++] = b;
			_erb_decode_state = ERB_State::got_sync_2;

		} else {
			_erb_decode_state = ERB_State::init;
		}

		break;

	case ERB_State::got_sync_2:
		if (b >= ERB_ID_VERSION && b <= ERB_ID_RTK) {
			buff_ptr[_erb_buff_cnt ++] = b;
			_erb_decode_state = ERB_State::got_id;

			if (b == ERB_ID_SPACE_INFO || b == ERB_ID_RTK) {
				//ignore those
				_erb_decode_state = ERB_State::init;
			}

		} else {
			_erb_decode_state = ERB_State::init;
		}

		break;

	case ERB_State::got_id:
		buff_ptr[_erb_buff_cnt ++] = b;
		_erb_decode_state = ERB_State::got_len_1;
		_erb_payload_len = b;
		break;

	case ERB_State::got_len_1:
		buff_ptr[_erb_buff_cnt ++] = b;
		_erb_decode_state = ERB_State::got_len_2;
		_erb_payload_len = (b << 8) | _erb_payload_len;
		break;

	case ERB_State::got_len_2:
		if (_erb_buff_cnt > ERB_SENTENCE_MAX_LEN - sizeof(erb_checksum_t)) {
			_erb_decode_state = ERB_State::init;

		} else {
			buff_ptr[_erb_buff_cnt ++] = b;
		}

		if (_erb_buff_cnt >= _erb_payload_len + static_cast<unsigned>(ERB_HEADER_LEN)) {
			_erb_decode_state = ERB_State::got_payload;
		}

		break;

	case ERB_State::got_payload:
		_erb_checksum.ck_a = b;
		_erb_decode_state = ERB_State::got_CK_A;
		break;

	case ERB_State::got_CK_A:
		_erb_checksum.ck_b = b;

		uint8_t cka = 0, ckb = 0;

		for (unsigned i = 2; i < _erb_payload_len + static_cast<unsigned>(ERB_HEADER_LEN); i++) {
			cka += buff_ptr[i];
			ckb += cka;
		}

		if (cka == _erb_checksum.ck_a && ckb == _erb_checksum.ck_b) {
			ret = 1;

		} else {
			ret = 0;
		}

		_erb_decode_state = ERB_State::init;
		break;
	}

	return ret;
}

int
GPSDriverEmlidReach::handleErbSentence()
{
	int ret = 0;

	if (_erb_buff.header.id == ERB_ID_VERSION) {
		//GPS_INFO("EMLIDREACH: ERB version: %d.%d.%d", _buff[ERB_HEADER_LEN + 4], _buff[ERB_HEADER_LEN + 5], _buff[ERB_HEADER_LEN + 6]);

	} else if (_erb_buff.header.id == ERB_ID_GEODIC_POSITION) {

		_gps_position->timestamp = gps_absolute_time();

		_last_POS_timeGPS = _erb_buff.payload.geodic_position.timeGPS;
		_gps_position->longitude_deg = _erb_buff.payload.geodic_position.longitude;
		_gps_position->latitude_deg = _erb_buff.payload.geodic_position.latitude;
		_gps_position->altitude_ellipsoid_m = _erb_buff.payload.geodic_position.altElipsoid;
		_gps_position->altitude_msl_m = _erb_buff.payload.geodic_position.altMeanSeaLevel;

		_rate_count_lat_lon++;

		_gps_position->eph = static_cast<float>(_erb_buff.payload.geodic_position.accHorizontal) * 1e-3f;
		_gps_position->epv = static_cast<float>(_erb_buff.payload.geodic_position.accVertical) * 1e-3f;

		_gps_position->vel_ned_valid = false;
		_gps_position->hdop = _hdop;
		_gps_position->vdop = _vdop;
		_gps_position->satellites_used = _satellites_used;
		_gps_position->fix_type = _fix_type;

		_POS_received = true;

	} else if (_erb_buff.header.id == ERB_ID_NAV_STATUS) {

		_fix_type = _erb_buff.payload.navigation_status.fixType;
		_fix_status = _erb_buff.payload.navigation_status.fixStatus;
		_satellites_used = _erb_buff.payload.navigation_status.numSatUsed;

		if (_fix_type == 0) {
			// no Fix
			_fix_type = 0;

		} else if (_fix_type == 1) {
			// Single
			_fix_type = (_satellites_used > 4) ? 3 : 2;

		} else if (_fix_type == 2) {
			// RTK Float
			_fix_type = 5;

		} else if (_fix_type == 3) {
			// RTK Fix
			_fix_type = 6;

		} else {
			_fix_type = 0;
		}


	} else if (_erb_buff.header.id == ERB_ID_DOPS) {

		_hdop = static_cast<float>(_erb_buff.payload.dop.dopHorizontal) / 100.0f;
		_vdop = static_cast<float>(_erb_buff.payload.dop.dopVertical) / 100.0f;

	} else if (_erb_buff.header.id == ERB_ID_VELOCITY_NED) {

		_last_VEL_timeGPS = _erb_buff.payload.ned_velocity.timeGPS;

		_gps_position->vel_n_m_s = static_cast<float>(_erb_buff.payload.ned_velocity.velN) / 100.0f;
		_gps_position->vel_e_m_s = static_cast<float>(_erb_buff.payload.ned_velocity.velE) / 100.0f;
		_gps_position->vel_d_m_s = static_cast<float>(_erb_buff.payload.ned_velocity.velD) / 100.0f;

		_gps_position->vel_m_s = static_cast<float>(_erb_buff.payload.ned_velocity.speed) / 100.0f;
		_gps_position->cog_rad = (static_cast<float>(_erb_buff.payload.ned_velocity.heading) / 1e5f) * GPS_PI / 180.0f;

		_gps_position->s_variance_m_s = static_cast<float>(_erb_buff.payload.ned_velocity.speedAccuracy) / 100.0f;

		_gps_position->vel_ned_valid = true;
		_rate_count_vel++;

		_VEL_received = true;

	} else if (_erb_buff.header.id == ERB_ID_SPACE_INFO) {

	} else if (_erb_buff.header.id == ERB_ID_RTK) {

	} else {
		//GPS_WARN("EMLIDREACH: ERB ID not known: %d", _erb_buff.header.id);
	}

	// Emlid doc: "If position and velocity are valid, it takes value 0x01, else it takes 0x00"
	if (_fix_status == 1
	    && _POS_received && _VEL_received
	    && _last_POS_timeGPS == _last_VEL_timeGPS) {
		ret = 1;
		_POS_received = false;
		_VEL_received = false;
	}

	return ret;
}


