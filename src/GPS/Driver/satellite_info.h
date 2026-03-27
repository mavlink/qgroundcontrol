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

enum class SatelliteSystem : uint8_t {
	Undefined,
	GPS,
	SBAS,
	GLONASS,
	BeiDou,
	Galileo,
	QZSS,
};

inline SatelliteSystem satelliteSystemFromSvid(uint8_t svid, uint8_t prn)
{
	if (svid >= 1 && svid <= 32)    return SatelliteSystem::GPS;
	if (svid >= 33 && svid <= 64)   return SatelliteSystem::SBAS;
	if (svid >= 65 && svid <= 96)   return SatelliteSystem::GLONASS;
	if (svid >= 120 && svid <= 158) return SatelliteSystem::SBAS;
	if (svid >= 159 && svid <= 163) return SatelliteSystem::BeiDou;
	if (svid >= 193 && svid <= 202) return SatelliteSystem::QZSS;
	if (svid >= 211 && svid <= 246) return SatelliteSystem::Galileo;
	if (svid >= 247)                return SatelliteSystem::BeiDou;

	if (prn >= 1 && prn <= 32)      return SatelliteSystem::GPS;
	if (prn >= 65 && prn <= 96)     return SatelliteSystem::GLONASS;
	if (prn >= 120 && prn <= 158)   return SatelliteSystem::SBAS;
	if (prn >= 159 && prn <= 195)   return SatelliteSystem::BeiDou;
	if (prn >= 211 && prn <= 246)   return SatelliteSystem::Galileo;

	return SatelliteSystem::Undefined;
}

inline const char* satelliteSystemName(SatelliteSystem sys)
{
	switch (sys) {
	case SatelliteSystem::GPS:       return "GPS";
	case SatelliteSystem::SBAS:      return "SBAS";
	case SatelliteSystem::GLONASS:   return "GLONASS";
	case SatelliteSystem::BeiDou:    return "BeiDou";
	case SatelliteSystem::Galileo:   return "Galileo";
	case SatelliteSystem::QZSS:      return "QZSS";
	case SatelliteSystem::Undefined: return "Other";
	}
	Q_UNREACHABLE();
}
