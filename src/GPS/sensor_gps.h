/****************************************************************************
 *
 *   Copyright (C) 2013-2022 PX4 Development Team. All rights reserved.
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

/* https://github.com/PX4/Firmware/blob/master/msg/SensorGps.msg */

#pragma once

#include <stdint.h>
#include <QtCore/QMetaType>

struct sensor_gps_s
{
	uint64_t timestamp;
	uint64_t timestamp_sample;

	uint32_t device_id;

	double latitude_deg;
	double longitude_deg;
	double altitude_msl_m;
	double altitude_ellipsoid_m;

	float s_variance_m_s;
	float c_variance_rad;
	uint8_t fix_type;

	float eph;
	float epv;

	float hdop;
	float vdop;

	int32_t noise_per_ms;
	uint16_t automatic_gain_control;

	static constexpr const uint8_t JAMMING_STATE_UNKNOWN = 0;
	static constexpr const uint8_t JAMMING_STATE_OK = 1;
	static constexpr const uint8_t JAMMING_STATE_WARNING = 2;
	static constexpr const uint8_t JAMMING_STATE_CRITICAL = 3;
	uint8_t jamming_state;
	int32_t jamming_indicator;

	static constexpr const uint8_t SPOOFING_STATE_UNKNOWN = 0;
	static constexpr const uint8_t SPOOFING_STATE_NONE = 1;
	static constexpr const uint8_t SPOOFING_STATE_INDICATED = 2;
	static constexpr const uint8_t SPOOFING_STATE_MULTIPLE = 3;
	uint8_t spoofing_state;

	float vel_m_s;
	float vel_n_m_s;
	float vel_e_m_s;
	float vel_d_m_s;
	float cog_rad;
	bool vel_ned_valid;

	int32_t timestamp_time_relative;
	uint64_t time_utc_usec;

	uint8_t satellites_used;

	float heading;
	float heading_offset;
	float heading_accuracy;

	float rtcm_injection_rate;
	uint8_t selected_rtcm_instance;

	bool rtcm_crc_failed;
	static constexpr const uint8_t RTCM_MSG_USED_UNKNOWN = 0;
	static constexpr const uint8_t RTCM_MSG_USED_NOT_USED = 1;
	static constexpr const uint8_t RTCM_MSG_USED_USED = 2;
	uint8_t rtcm_msg_used;
};
Q_DECLARE_METATYPE(sensor_gps_s);
