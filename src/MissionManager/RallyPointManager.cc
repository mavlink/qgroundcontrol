/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RallyPointManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(RallyPointManagerLog, "RallyPointManagerLog")

RallyPointManager::RallyPointManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _planManager              (vehicle, MAV_MISSION_TYPE_RALLY)
{
    connect(&_planManager, &PlanManager::inProgressChanged,         this, &RallyPointManager::inProgressChanged);
    connect(&_planManager, &PlanManager::error,                     this, &RallyPointManager::error);
    connect(&_planManager, &PlanManager::removeAllComplete,         this, &RallyPointManager::removeAllComplete);
    connect(&_planManager, &PlanManager::sendComplete,              this, &RallyPointManager::_sendComplete);
    connect(&_planManager, &PlanManager::newMissionItemsAvailable,  this, &RallyPointManager::_planManagerLoadComplete);
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
     _planManager.loadFromVehicle();
}

void RallyPointManager::sendToVehicle(const QList<QGeoCoordinate>& rgPoints)
{
    _rgSendPoints.clear();
    for (const QGeoCoordinate& rallyPoint: rgPoints) {
        _rgSendPoints.append(rallyPoint);
    }

    QList<MissionItem*> rallyItems;
    for (int i=0; i<rgPoints.count(); i++) {

        MissionItem* item = new MissionItem(0,
                                            MAV_CMD_NAV_RALLY_POINT,
                                            MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                            0, 0, 0, 0,                 // param 1-4 unused
                                            rgPoints[i].latitude(),
                                            rgPoints[i].longitude(),
                                            rgPoints[i].altitude(),
                                            false,                      // autocontinue
                                            false,                      // isCurrentItem
                                            this);                      // parent
        rallyItems.append(item);
    }

    // Plan manager takes control of MissionItems, so no need to delete
    _planManager.writeMissionItems(rallyItems);
}

void RallyPointManager::removeAll(void)
{
    _rgPoints.clear();
    _planManager.removeAll();
}

bool RallyPointManager::supported(void) const
{
    return (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY) && (_vehicle->maxProtoVersion() >= 200);
}

void RallyPointManager::_planManagerLoadComplete(bool removeAllRequested)
{
    _rgPoints.clear();

    Q_UNUSED(removeAllRequested);

    const QList<MissionItem*>& rallyItems = _planManager.missionItems();

    for (int i=0; i<rallyItems.count(); i++) {
        MissionItem* item = rallyItems[i];

        MAV_CMD command = item->command();

        if (command == MAV_CMD_NAV_RALLY_POINT) {
            _rgPoints.append(QGeoCoordinate(item->param5(), item->param6(), item->param7()));


        } else {
            qCDebug(RallyPointManagerLog) << "RallyPointManager load: Unsupported command %1" << item->command();
            break;
        }
    }


    emit loadComplete();
}

void RallyPointManager::_sendComplete(bool error)
{
    if (error) {
        _rgPoints.clear();
    } else {
        _rgPoints = _rgSendPoints;
    }
    _rgSendPoints.clear();
    emit sendComplete(error);
}
