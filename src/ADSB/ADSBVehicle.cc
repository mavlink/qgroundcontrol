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
    _info.icaoAddress = vehicleInfo.icaoAddress;
    update(vehicleInfo);

    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

ADSBVehicle::~ADSBVehicle()
{
    // qCDebug(ADSBTCPLinkLog) << Q_FUNC_INFO << this;
}

void ADSBVehicle::update(const ADSB::VehicleInfo_t &vehicleInfo)
{
    if (vehicleInfo.icaoAddress != icaoAddress()) {
        qCWarning(ADSBVehicleLog) << "ICAO address mismatch expected:" << icaoAddress() << "actual:" << vehicleInfo.icaoAddress;
        return;
    }

    qCDebug(ADSBVehicleLog) << "Updating" << QStringLiteral("%1 Flags: %2").arg(vehicleInfo.icaoAddress, 0, 16).arg(vehicleInfo.availableFlags, 0, 2);

    if (vehicleInfo.availableFlags & ADSB::CallsignAvailable) {
        if (vehicleInfo.callsign != callsign()) {
            _info.callsign = vehicleInfo.callsign;
            emit callsignChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::LocationAvailable) {
        if (vehicleInfo.location != coordinate()) {
            _info.location = vehicleInfo.location;
            emit coordinateChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::AltitudeAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.altitude, altitude())) {
            _info.altitude = vehicleInfo.altitude;
            emit altitudeChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::HeadingAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.heading, heading())) {
            _info.heading = vehicleInfo.heading;
            emit headingChanged();
        }
    }

    if (vehicleInfo.availableFlags & ADSB::AlertAvailable) {
        if (vehicleInfo.alert != alert()) {
            _info.alert = vehicleInfo.alert;
            emit alertChanged();
        }
    }

    (void) _lastUpdateTimer.restart();
}
