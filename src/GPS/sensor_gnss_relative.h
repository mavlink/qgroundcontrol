/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/* https://github.com/PX4/Firmware/blob/master/msg/SensorGnssRelative.msg */

#pragma once

#include <stdint.h>

#include <QtCore/QMetaType>

struct sensor_gnss_relative_s
{
	uint64_t timestamp;
	uint64_t timestamp_sample;

	uint32_t device_id;

	uint64_t time_utc_usec;

	uint16_t reference_station_id;

	float position[3];
	float position_accuracy[3];

	float heading;
	float heading_accuracy;

	float position_length;
	float accuracy_length;

	bool gnss_fix_ok;
	bool differential_solution;
	bool relative_position_valid;
	bool carrier_solution_floating;
	bool carrier_solution_fixed;
	bool moving_base_mode;
	bool reference_position_miss;
	bool reference_observations_miss;
	bool heading_valid;
	bool relative_position_normalized;
};
Q_DECLARE_METATYPE(sensor_gnss_relative_s);
