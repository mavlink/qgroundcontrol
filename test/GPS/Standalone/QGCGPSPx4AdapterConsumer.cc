#include <cmath>
#include <iostream>

#include "GPSPx4Data_p.h"
#include "satellite_info.h"
#include "sensor_gps.h"

int main()
{
    sensor_gps_s raw{};
    GPSPx4Data::initialize(raw);
    const auto unreported = GPSPx4Data::position(raw);
    if (!std::isnan(unreported.latitudeDegrees) || unreported.satellitesUsed) {
        std::cerr << "Unreported PX4 position became known\n";
        return 1;
    }

    raw.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    raw.latitude_deg = 0;
    raw.rtcm_msg_used = sensor_gps_s::RTCM_MSG_USED_USED;
    const auto position = GPSPx4Data::position(raw);
    if (position.fixType != GPSPositionReport::FixType::RTKFixed || position.latitudeDegrees != 0 ||
        position.integrity.correctionCrcFailed != false) {
        std::cerr << "PX4 position conversion lost explicit values\n";
        return 2;
    }

    satellite_info_s rawSatellites{};
    rawSatellites.count = 255;
    rawSatellites.svid[0] = 9;
    const auto satellites = GPSPx4Data::satellites(rawSatellites, GPSType::ublox);
    if (satellites.count != satellite_info_s::SAT_INFO_MAX_SATELLITES || satellites.satellites[0].id != 9) {
        std::cerr << "PX4 satellite conversion lost its count bound or metadata\n";
        return 3;
    }
    return 0;
}
