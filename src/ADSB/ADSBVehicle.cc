/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBVehicle.h"
#include "QGCLoggingCategory.h"
#include "QGC.h"

#include <QDebug>
#include <QtMath>

ADSBVehicle::ADSBVehicle(const VehicleInfo_t& vehicleInfo, QObject* parent)
    : QObject       (parent)
    , _icaoAddress  (vehicleInfo.icaoAddress)
    , _altitude     (qQNaN())
    , _heading      (qQNaN())
    , _alert        (false)
{
    update(vehicleInfo);
}

void ADSBVehicle::update(const VehicleInfo_t& vehicleInfo)
{
    if (_icaoAddress != vehicleInfo.icaoAddress) {
        qCWarning(ADSBVehicleManagerLog) << "ICAO address mismatch expected:actual" << _icaoAddress << vehicleInfo.icaoAddress;
        return;
    }
    if (vehicleInfo.availableFlags & CallsignAvailable) {
        if (vehicleInfo.callsign != _callsign) {
            _callsign = vehicleInfo.callsign;
            emit callsignChanged();
        }
    }
    if (vehicleInfo.availableFlags & LocationAvailable) {
        if (_coordinate != vehicleInfo.location) {
            _coordinate = vehicleInfo.location;
            emit coordinateChanged();
        }
    }
    if (vehicleInfo.availableFlags & AltitudeAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.altitude, _altitude)) {
            _altitude = vehicleInfo.altitude;
            emit altitudeChanged();
        }
    }
    if (vehicleInfo.availableFlags & HeadingAvailable) {
        if (!QGC::fuzzyCompare(vehicleInfo.heading, _heading)) {
            _heading = vehicleInfo.heading;
            emit headingChanged();
        }
    }
    if (vehicleInfo.availableFlags & AlertAvailable) {
        if (vehicleInfo.alert != _alert) {
            _alert = vehicleInfo.alert;
            emit alertChanged();
        }
    }
    _lastUpdateTimer.restart();
}

bool ADSBVehicle::expired()
{
    return _lastUpdateTimer.hasExpired(expirationTimeoutMs);
}
