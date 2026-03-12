/* https://github.com/PX4/PX4-Autopilot/blob/main/msg/SatelliteInfo.msg */

#pragma once

#include <stdint.h>

#include <QtCore/QMetaType>

struct satellite_info_s
{
	uint64_t timestamp;
	static constexpr uint8_t SAT_INFO_MAX_SATELLITES = 40;

	uint8_t count;
	uint8_t svid[SAT_INFO_MAX_SATELLITES];
	uint8_t used[SAT_INFO_MAX_SATELLITES];
	uint8_t elevation[SAT_INFO_MAX_SATELLITES];
	uint16_t azimuth[SAT_INFO_MAX_SATELLITES];
	uint8_t snr[SAT_INFO_MAX_SATELLITES];
	uint8_t prn[SAT_INFO_MAX_SATELLITES];
};
Q_DECLARE_METATYPE(satellite_info_s);
