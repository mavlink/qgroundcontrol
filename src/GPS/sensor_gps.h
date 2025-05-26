/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
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

	static constexpr uint8_t FIX_TYPE_NONE = 1;
	static constexpr uint8_t FIX_TYPE_2D = 2;
	static constexpr uint8_t FIX_TYPE_3D = 3;
	static constexpr uint8_t FIX_TYPE_RTCM_CODE_DIFFERENTIAL = 4;
	static constexpr uint8_t FIX_TYPE_RTK_FLOAT = 5;
	static constexpr uint8_t FIX_TYPE_RTK_FIXED = 6;
	static constexpr uint8_t FIX_TYPE_EXTRAPOLATED = 8;
	uint8_t fix_type;

	float eph;
	float epv;

	float hdop;
	float vdop;

	int32_t noise_per_ms;
	uint16_t automatic_gain_control;

	static constexpr uint8_t JAMMING_STATE_UNKNOWN = 0;
	static constexpr uint8_t JAMMING_STATE_OK = 1;
	static constexpr uint8_t JAMMING_STATE_WARNING = 2;
	static constexpr uint8_t JAMMING_STATE_CRITICAL = 3;
	uint8_t jamming_state;
	int32_t jamming_indicator;

	static constexpr uint8_t SPOOFING_STATE_UNKNOWN = 0;
	static constexpr uint8_t SPOOFING_STATE_NONE = 1;
	static constexpr uint8_t SPOOFING_STATE_INDICATED = 2;
	static constexpr uint8_t SPOOFING_STATE_MULTIPLE = 3;
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

	static constexpr uint8_t RTCM_MSG_USED_UNKNOWN = 0;
	static constexpr uint8_t RTCM_MSG_USED_NOT_USED = 1;
	static constexpr uint8_t RTCM_MSG_USED_USED = 2;
	uint8_t rtcm_msg_used;
};
Q_DECLARE_METATYPE(sensor_gps_s);
