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

#ifndef MissionEditor_H
#define MissionEditor_H

#include "QGCQmlWidgetHolder.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

class MissionEditor : public QGCQmlWidgetHolder
{
    Q_OBJECT
    
public:
    MissionEditor(QWidget* parent = NULL);
    ~MissionEditor();

    Q_PROPERTY(QmlObjectListModel*  missionItems                READ missionItems               NOTIFY missionItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines               READ waypointLines              NOTIFY waypointLinesChanged)
    Q_PROPERTY(bool                 canEdit                     READ canEdit                    NOTIFY canEditChanged)
    Q_PROPERTY(bool                 liveHomePositionAvailable   READ liveHomePositionAvailable  NOTIFY liveHomePositionAvailableChanged)
    Q_PROPERTY(QGeoCoordinate       liveHomePosition            READ liveHomePosition           NOTIFY liveHomePositionChanged)
    
    Q_INVOKABLE int addMissionItem(QGeoCoordinate coordinate);
    Q_INVOKABLE void getMissionItems(void);
    Q_INVOKABLE void setMissionItems(void);
    Q_INVOKABLE void loadMissionFromFile(void);
    Q_INVOKABLE void saveMissionToFile(void);
    Q_INVOKABLE void removeMissionItem(int index);
    Q_INVOKABLE void moveUp(int index);
    Q_INVOKABLE void moveDown(int index);

    // Property accessors
    
    QmlObjectListModel* missionItems(void) { return _missionItems; }
    QmlObjectListModel* waypointLines(void) { return &_waypointLines; }
    bool canEdit(void) { return _canEdit; }
    bool liveHomePositionAvailable(void) { return _liveHomePositionAvailable; }
    QGeoCoordinate liveHomePosition(void) { return _liveHomePosition; }

signals:
    void missionItemsChanged(void);
    void canEditChanged(bool canEdit);
    void waypointLinesChanged(void);
    void liveHomePositionAvailableChanged(bool homePositionAvailable);
    void liveHomePositionChanged(const QGeoCoordinate& homePosition);
    
private slots:
    void _newMissionItemsAvailable();
    void _itemCoordinateChanged(const QGeoCoordinate& coordinate);
    void _itemCommandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command);
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _activeVehicleHomePositionAvailableChanged(bool homePositionAvailable);
    void _activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition);

private:
    void _recalcSequence(void);
    void _recalcWaypointLines(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllMissionItems(void);
    void _deinitAllMissionItems(void);
    void _initMissionItem(MissionItem* item);
    void _deinitMissionItem(MissionItem* item);

private:
    QmlObjectListModel* _missionItems;
    QmlObjectListModel  _waypointLines;
    bool                _canEdit;           ///< true: UI can edit these items, false: can't edit, can only send to vehicle or save
    Vehicle*            _activeVehicle;
    bool                _liveHomePositionAvailable;
    QGeoCoordinate      _liveHomePosition;

    static const char* _settingsGroup;
};

#endif
