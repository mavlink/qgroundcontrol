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

void GeoFenceManager::_clearGeoFence(void)
{
    _geoFence.fenceType = GeoFenceNone;
    _geoFence.circleRadius = 0.0;
    _geoFence.polygon.clear();
    _geoFence.breachReturnPoint = QGeoCoordinate();
    emit newGeoFenceAvailable();
}

void GeoFenceManager::requestGeoFence(void)
{
    qCWarning(GeoFenceManagerLog) << "Internal error: GeoFenceManager::requestGeoFence called";
}

/// Set and send the specified geo fence to the vehicle
void GeoFenceManager::setGeoFence(const GeoFence_t& geoFence)
{
    Q_UNUSED(geoFence);
    qCWarning(GeoFenceManagerLog) << "Internal error: GeoFenceManager::setGeoFence called";
}
