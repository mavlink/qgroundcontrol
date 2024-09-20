/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

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
    CallsignAvailable = 1 << 0,
    LocationAvailable = 1 << 1,
    AltitudeAvailable = 1 << 2,
    HeadingAvailable = 1 << 3,
    AlertAvailable = 1 << 4,
};
Q_FLAG_NS(AvailableInfoType)
Q_DECLARE_FLAGS(AvailableInfoTypes, AvailableInfoType)
Q_DECLARE_OPERATORS_FOR_FLAGS(AvailableInfoTypes)

struct VehicleInfo_t {
    uint32_t icaoAddress;
    QString callsign;
    QGeoCoordinate location;
    double altitude; // TODO: Use Altitude in QGeoCoordinate?
    double heading;
    bool alert;
    AvailableInfoTypes availableFlags;
};
} // namespace ADSB

Q_DECLARE_METATYPE(ADSB::VehicleInfo_t)
