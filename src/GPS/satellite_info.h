/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/* https://github.com/PX4/Firmware/blob/master/msg/SatelliteInfo.msg */

#pragma once

#include <stdint.h>

#include <QtCore/QMetaType>

struct satellite_info_s
{
	uint64_t timestamp;
	static constexpr uint8_t SAT_INFO_MAX_SATELLITES = 20;

	uint8_t count;
	uint8_t svid[20];
	uint8_t used[20];
	uint8_t elevation[20];
	uint8_t azimuth[20];
	uint8_t snr[20];
	uint8_t prn[20];
};
Q_DECLARE_METATYPE(satellite_info_s);
