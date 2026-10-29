#include "RallyPointManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RallyPointManagerLog, "PlanManager.RallyPointManager")

RallyPointManager::RallyPointManager(Vehicle* vehicle)
    : PlanManager(vehicle, MAV_MISSION_TYPE_RALLY)
{
    connect(this, &PlanManager::inProgressChanged,          this, &RallyPointManager::inProgressChanged);
    connect(this, &PlanManager::error,                      this, &RallyPointManager::error);
    connect(this, &PlanManager::removeAllComplete,          this, &RallyPointManager::removeAllComplete);
    connect(this, &PlanManager::sendComplete,               this, &RallyPointManager::_sendComplete);
    connect(this, &PlanManager::newMissionItemsAvailable,   this, &RallyPointManager::_planManagerLoadComplete);
}


RallyPointManager::~RallyPointManager()
{

}

void RallyPointManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(RallyPointManagerLog) << "Sending error" << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

void RallyPointManager::sendToVehicle(const QList<RallyPointData>& rgPoints)
{
    _rgSendPoints = rgPoints;

    QList<MissionItem*> rallyItems;
    for (int i=0; i<rgPoints.count(); i++) {
        MissionItem* item = new MissionItem(
            0, MAV_CMD_NAV_RALLY_POINT, rgPoints[i].frame, 0, 0, 0, 0,  // param 1-4 unused
            rgPoints[i].coordinate.latitude(), rgPoints[i].coordinate.longitude(), rgPoints[i].coordinate.altitude(),
            false,                                                      // autocontinue
            false,                                                      // isCurrentItem
            this);                                                      // parent
        rallyItems.append(item);
    }

    // Plan manager takes control of MissionItems, so no need to delete
    writeMissionItems(rallyItems);
}

void RallyPointManager::removeAll(void)
{
    _rgPoints.clear();
    PlanManager::removeAll();
}

bool RallyPointManager::supported(void) const
{
    return _vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
}

void RallyPointManager::_planManagerLoadComplete(bool removeAllRequested)
{
    _rgPoints.clear();

    Q_UNUSED(removeAllRequested);

    const QList<MissionItem*>& rallyItems = missionItems();

    for (int i=0; i<rallyItems.count(); i++) {
        MissionItem* item = rallyItems[i];

        MAV_CMD command = item->command();

        if (command == MAV_CMD_NAV_RALLY_POINT) {
            _rgPoints.append({QGeoCoordinate(item->param5(), item->param6(), item->param7()), item->frame()});
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
