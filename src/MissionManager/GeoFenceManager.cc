/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoFenceManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(GeoFenceManagerLog, "GeoFenceManagerLog")

GeoFenceManager::GeoFenceManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{

}

GeoFenceManager::~GeoFenceManager()
{

}

void GeoFenceManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(GeoFenceManagerLog) << "Sending error" << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

void GeoFenceManager::loadFromVehicle(void)
{
    // No geofence support in unknown vehicle
    loadComplete(QGeoCoordinate(), QList<QGeoCoordinate>());
}

void GeoFenceManager::sendToVehicle(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon)
{
    Q_UNUSED(breachReturn);
    Q_UNUSED(polygon);

    // No geofence support in unknown vehicle
}
