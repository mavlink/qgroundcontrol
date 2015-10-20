/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "MissionController.h"
#include "ScreenToolsController.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "CoordinateVector.h"

MissionController::MissionController(QObject *parent)
    : QObject(parent)
    , _missionItems(NULL)
    , _activeVehicle(NULL)
{
    MultiVehicleManager* multiVehicleMgr = MultiVehicleManager::instance();
    
    connect(multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &MissionController::_activeVehicleChanged);
    
    Vehicle* activeVehicle = multiVehicleMgr->activeVehicle();
    if (activeVehicle) {
        _activeVehicleChanged(activeVehicle);
    } else {
        _newMissionItemsAvailable();
    }
}

MissionController::~MissionController()
{
}

void MissionController::_newMissionItemsAvailable(void)
{
    if (_missionItems) {
        _missionItems->deleteLater();
    }
    
    MissionManager* missionManager = NULL;
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    if (activeVehicle) {
        missionManager = activeVehicle->missionManager();
    }

    if (!missionManager || missionManager->inProgress()) {
        _missionItems = new QmlObjectListModel(this);
    } else {
        _missionItems = missionManager->copyMissionItems();
    }

    _initAllMissionItems();
}

void MissionController::_recalcWaypointLines(void)
{
    int firstIndex = _homePositionValid  ? 0 : 1;

    _waypointLines.clear();

    if (firstIndex < _missionItems->count()) {
        bool firstCoordinateItem = true;
        MissionItem* lastCoordinateItem = qobject_cast<MissionItem*>(_missionItems->get(firstIndex));

        for (int i=firstIndex; i<_missionItems->count(); i++) {
            MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

            if (item->specifiesCoordinate()) {
                if (firstCoordinateItem) {
                    if (item->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF && _homePositionValid) {
                        // The first coordinate we hit is a takeoff command so link back to home position if we have one
                        _waypointLines.append(new CoordinateVector(qobject_cast<MissionItem*>(_missionItems->get(0))->coordinate(), item->coordinate()));
                    } else {
                        // First coordiante is not a takeoff command, it does not link backwards to anything
                    }
                    firstCoordinateItem = false;
                } else {
                    // Subsequent coordinate items link to last coordinate item
                    _waypointLines.append(new CoordinateVector(lastCoordinateItem->coordinate(), item->coordinate()));
                }
                lastCoordinateItem = item;
            }
        }
    }
    
    emit waypointLinesChanged();
}

// This will update the child item hierarchy
void MissionController::_recalcChildItems(void)
{
    int firstIndex = _homePositionValid  ? 0 : 1;

    if (_missionItems->count() > firstIndex) {
        MissionItem* currentParentItem = qobject_cast<MissionItem*>(_missionItems->get(firstIndex));

        currentParentItem->childItems()->clear();

        for (int i=firstIndex+1; i<_missionItems->count(); i++) {
            MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

            // Set up non-coordinate item child hierarchy
            if (item->specifiesCoordinate()) {
                item->childItems()->clear();
                currentParentItem = item;
            } else {
                currentParentItem->childItems()->append(item);
            }
        }
    }
}

// This will update the sequence numbers to be sequential starting from 0
void MissionController::_recalcSequence(void)
{
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        // Setup ascending sequence numbers
        item->setSequenceNumber(i);
    }
}

void MissionController::_recalcAll(void)
{
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
}

/// Initializes a new set of mission items which may have come from the vehicle or have been loaded from a file
void MissionController::_initAllMissionItems(void)
{
    // Add the home position item to the front
    MissionItem* homeItem = new MissionItem(this);
    homeItem->setHomePositionSpecialCase(true);
    homeItem->setHomePositionValid(false);
    homeItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    homeItem->setLatitude(47.3769);
    homeItem->setLongitude(8.549444);
    _missionItems->insert(0, homeItem);
    
    _recalcAll();
    
    emit missionItemsChanged();
}

void MissionController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        MissionManager* missionManager = _activeVehicle->missionManager();
        disconnect(missionManager, &MissionManager::newMissionItemsAvailable,   this, &MissionController::_newMissionItemsAvailable);
        _activeVehicle = NULL;
    }
    
    _activeVehicle = activeVehicle;
    
    if (_activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();
        connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailable);
        _newMissionItemsAvailable();
    }
}

void MissionController::setHomePositionValid(bool homePositionValid)
{
    _homePositionValid = homePositionValid;
    qobject_cast<MissionItem*>(_missionItems->get(0))->setHomePositionValid(homePositionValid);

    emit homePositionValidChanged(_homePositionValid);
}
