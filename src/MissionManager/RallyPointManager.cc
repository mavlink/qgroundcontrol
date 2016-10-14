/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RallyPointManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(RallyPointManagerLog, "RallyPointManagerLog")

RallyPointManager::RallyPointManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{

}

RallyPointManager::~RallyPointManager()
{

}

void RallyPointManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(RallyPointManagerLog) << "Sending error" << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

void RallyPointManager::loadFromVehicle(void)
{
    // No support in generic vehicle
    loadComplete(QList<QGeoCoordinate>());
}

void RallyPointManager::sendToVehicle(const QList<QGeoCoordinate>& rgPoints)
{
    Q_UNUSED(rgPoints);
    // No support in generic vehicle
}
