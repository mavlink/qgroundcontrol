/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBVehicle.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtNumeric>

QGC_LOGGING_CATEGORY(ADSBVehicleLog, "qgc.adsb.adsbvehicle")

ADSBVehicle::ADSBVehicle(const ADSB::VehicleInfo_t &vehicleInfo, QObject *parent)
    : QObject(parent)
{
    // qCDebug(ADSBVehicleLog) << Q_FUNC_INFO << this;

    _info.icaoAddress = vehicleInfo.icaoAddress;
    update(vehicleInfo);
}

ADSBVehicle::~ADSBVehicle()
{
    // qCDebug(ADSBVehicleLog) << Q_FUNC_INFO << this;
}

void ADSBVehicle::update(const ADSB::VehicleInfo_t &vehicleInfo)
{
    if (vehicleInfo.icaoAddress != icaoAddress()) {
        qCWarning(ADSBVehicleLog) << "ICAO address mismatch expected:" << icaoAddress() << "actual:" << vehicleInfo.icaoAddress;
        return;
    }

    qCDebug(ADSBVehicleLog) << "Updating" << QStringLiteral("%1 Flags: %2").arg(vehicleInfo.icaoAddress, 0, 16).arg(vehicleInfo.availableFlags, 0, 2);

    if (vehicleInfo.availableFlags & ADSB::LocationAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.location.latitude(), coordinate().latitude()) || !QGC::fuzzyCompare(vehicleInfo.location.longitude(), coordinate().longitude())) {
            _info.location.setLatitude(vehicleInfo.location.latitude());
            _info.location.setLongitude(vehicleInfo.location.longitude());
            emit coordinateChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::AltitudeAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.location.altitude(), altitude())) {
            _info.location.setAltitude(vehicleInfo.location.altitude());
            emit altitudeChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::HeadingAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.heading, heading())) {
            _info.heading = vehicleInfo.heading;
            emit headingChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::VelocityAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.velocity, velocity())) {
            _info.velocity = vehicleInfo.velocity;
            emit velocityChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::CallsignAvailable) {
        if (vehicleInfo.callsign != callsign()) {
            _info.callsign = vehicleInfo.callsign;
            emit callsignChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::SquawkAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.velocity, squawk())) {
            _info.squawk = vehicleInfo.squawk;
            emit squawkChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::VerticalVelAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.verticalVel, verticalVel())) {
            _info.verticalVel = vehicleInfo.verticalVel;
            emit verticalVelChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::AlertAvailable) {
        if (vehicleInfo.alert != alert()) {
            _info.alert = vehicleInfo.alert;
            emit alertChanged();
        }
    }

    _lastUpdateTimer.start();
}
