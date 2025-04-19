/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QMetaType>
#include <QtPositioning/QGeoCoordinate>

namespace ADSB {
Q_NAMESPACE

/*=======================================================================*
|Messages               |TC        |Ground(still)|Ground(moving)|Airborne|
|-----------------------|----------|-------------|--------------|--------|
|Aircraft identification|1–4       |0.1 Hz       |0.2 Hz        |0.2 Hz  |
|Surface position       |5–8       |0.2 Hz       |2.0 Hz        |-.- Hz  |
|Airborne position      |9–18,20–22|-.- Hz       |-.- Hz        |2.0 Hz  |
|Airborne velocity      |19        |-.- Hz       |-.- Hz        |2.0 Hz  |
*=======================================================================*/

/// Enum for ADSB message types.
enum MessageType {
    IdentificationAndCategory = 1,
    SurfacePosition = 2,
    AirbornePosition = 3,
    AirborneVelocity = 4,
    SurveillanceAltitude = 5,
    SurveillanceId = 6,
    Unsupported = -1
};
Q_ENUM_NS(MessageType)

/// Enum for ADSB Info Types Available.
enum AvailableInfoType {
    LocationAvailable       = 1 << 0,
    AltitudeAvailable       = 1 << 1,
    HeadingAvailable        = 1 << 2,
    VelocityAvailable       = 1 << 3,
    CallsignAvailable       = 1 << 4,
    SquawkAvailable         = 1 << 5,
    VerticalVelAvailable    = 1 << 6,
    AlertAvailable          = 1 << 7
};
Q_FLAG_NS(AvailableInfoType)
Q_DECLARE_FLAGS(AvailableInfoTypes, AvailableInfoType)
Q_DECLARE_OPERATORS_FOR_FLAGS(AvailableInfoTypes)

struct VehicleInfo_t {
    AvailableInfoTypes availableFlags = AvailableInfoType();
    uint32_t icaoAddress = 0;
    QString callsign;
    QGeoCoordinate location;
    double heading = 0.0;
    uint16_t squawk = 0;
    double velocity = 0.0;
    double verticalVel = 0.0;
    uint32_t lastContact = 0;
    bool simulated = false;
    bool baro = false;
    bool alert = false;
    ADSB_ALTITUDE_TYPE altitudeeType = ADSB_ALTITUDE_TYPE_PRESSURE_QNH;
    ADSB_EMITTER_TYPE emitterType = ADSB_EMITTER_TYPE_NO_INFO;
    // TODO: Use QGeoPositionInfo, QGeoPositionInfoSource, QGeoPositionInfoSourceFactory
};
} // namespace ADSB

Q_DECLARE_METATYPE(ADSB::VehicleInfo_t)
