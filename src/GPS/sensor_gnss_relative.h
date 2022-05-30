#pragma once
#include <stdint.h>

/*
 * This file is auto-generated from https://github.com/PX4/Firmware/blob/master/msg/satellite_info.msg
 * and was manually copied here.
 */

struct sensor_gnss_relative_s {
	uint64_t timestamp;                 // time since system start (microseconds)
	uint64_t timestamp_sample;          // time since system start (microseconds)

	uint32_t device_id;                 // unique device ID for the sensor that does not change between power cycles

	uint64_t time_utc_usec;             // Timestamp (microseconds, UTC), this is the timestamp which comes from the gps module. It might be unavailable right after cold start, indicated by a value of 0

	uint16_t reference_station_id;      // Reference Station ID

	float position[3];               	// GPS NED relative position vector (m)
	float position_accuracy[3];      	// Accuracy of relative position (m)

	float heading;                   	// Heading of the relative position vector (radians)
	float heading_accuracy;          	// Accuracy of heading of the relative position vector (radians)

	float position_length;
	float accuracy_length;

	bool gnss_fix_ok;                  	// GNSS valid fix (i.e within DOP & accuracy masks)
	bool differential_solution;        	// differential corrections were applied
	bool relative_position_valid;
	bool carrier_solution_floating;    	// carrier phase range solution with floating ambiguities
	bool carrier_solution_fixed;       	// carrier phase range solution with fixed ambiguities
	bool moving_base_mode;             	// if the receiver is operating in moving base mode
	bool reference_position_miss;      	// extrapolated reference position was used to compute moving base solution this epoch
	bool reference_observations_miss;  	// extrapolated reference observations were used to compute moving base solution this epoch
	bool heading_valid;
	bool relative_position_normalized; 	// the components of the relative position vector (including the high-precision parts) are normalized
};