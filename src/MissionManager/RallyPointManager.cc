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
    emit loadComplete(QList<QGeoCoordinate>());
}

void RallyPointManager::sendToVehicle(const QList<QGeoCoordinate>& rgPoints)
{
    // No support in generic vehicle
    Q_UNUSED(rgPoints);
    emit sendComplete(false /* error */);
}

void RallyPointManager::removeAll(void)
{
    // No support in generic vehicle
    emit removeAllComplete(false /* error */);
}

bool RallyPointManager::supported(void) const
{
    return (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY) && (_vehicle->maxProtoVersion() >= 200);
}
