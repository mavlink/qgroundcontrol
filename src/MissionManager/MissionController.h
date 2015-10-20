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

#ifndef MissionController_H
#define MissionController_H

#include <QObject>

#include "QmlObjectListModel.h"
#include "Vehicle.h"

/// MissionController is a read only controller for Mission Items
class MissionController : public QObject
{
    Q_OBJECT
    
public:
    MissionController(QObject* parent = NULL);
    ~MissionController();

    Q_PROPERTY(QmlObjectListModel*  missionItems    READ missionItems   NOTIFY missionItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines   READ waypointLines  NOTIFY waypointLinesChanged)
    
    // Property accessors
    
    QmlObjectListModel* missionItems(void) { return _missionItems; }
    QmlObjectListModel* waypointLines(void) { return &_waypointLines; }

signals:
    void missionItemsChanged(void);
    void waypointLinesChanged(void);
    
private slots:
    void _newMissionItemsAvailable();
    void _activeVehicleChanged(Vehicle* activeVehicle);

private:
    void _recalcWaypointLines(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllMissionItems(void);

private:
    QmlObjectListModel* _missionItems;
    QmlObjectListModel  _waypointLines;
    Vehicle*            _activeVehicle;
};

#endif
