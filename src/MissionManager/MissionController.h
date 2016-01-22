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
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(MissionControllerLog)

class MissionController : public QObject
{
    Q_OBJECT

public:
    MissionController(QObject* parent = NULL);
    ~MissionController();

    Q_PROPERTY(QmlObjectListModel*  missionItems                READ missionItems                   NOTIFY missionItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines               READ waypointLines                  NOTIFY waypointLinesChanged)
    Q_PROPERTY(bool                 liveHomePositionAvailable   READ liveHomePositionAvailable      NOTIFY liveHomePositionAvailableChanged)
    Q_PROPERTY(QGeoCoordinate       liveHomePosition            READ liveHomePosition               NOTIFY liveHomePositionChanged)
    Q_PROPERTY(bool                 autoSync                    READ autoSync   WRITE setAutoSync   NOTIFY autoSyncChanged)

    Q_INVOKABLE void start(bool editMode);
    Q_INVOKABLE void getMissionItems(void);
    Q_INVOKABLE void sendMissionItems(void);
    Q_INVOKABLE void loadMissionFromFile(void);
    Q_INVOKABLE void saveMissionToFile(void);
    Q_INVOKABLE void removeMissionItem(int index);
    Q_INVOKABLE void removeAllMissionItems(void);

    /// @param i: index to insert at
    Q_INVOKABLE int insertMissionItem(QGeoCoordinate coordinate, int i);

    // Property accessors

    QmlObjectListModel* missionItems(void);
    QmlObjectListModel* waypointLines(void) { return &_waypointLines; }
    bool liveHomePositionAvailable(void) { return _liveHomePositionAvailable; }
    QGeoCoordinate liveHomePosition(void) { return _liveHomePosition; }
    bool autoSync(void) { return _autoSync; }
    void setAutoSync(bool autoSync);

signals:
    void missionItemsChanged(void);
    void waypointLinesChanged(void);
    void liveHomePositionAvailableChanged(bool homePositionAvailable);
    void liveHomePositionChanged(const QGeoCoordinate& homePosition);
    void autoSyncChanged(bool autoSync);
    void newItemsFromVehicle(void);

private slots:
    void _newMissionItemsAvailableFromVehicle();
    void _itemCoordinateChanged(const QGeoCoordinate& coordinate);
    void _itemFrameChanged(int frame);
    void _itemCommandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command);
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _activeVehicleHomePositionAvailableChanged(bool homePositionAvailable);
    void _activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition);
    void _dirtyChanged(bool dirty);
    void _inProgressChanged(bool inProgress);

private:
    void _recalcSequence(void);
    void _recalcWaypointLines(void);
    void _recalcChildItems(void);
    void _recalcAll(void);
    void _initAllMissionItems(void);
    void _deinitAllMissionItems(void);
    void _initMissionItem(MissionItem* item);
    void _deinitMissionItem(MissionItem* item);
    void _autoSyncSend(void);
    void _setupMissionItems(bool loadFromVehicle, bool forceLoad);
    void _setupActiveVehicle(Vehicle* activeVehicle, bool forceLoadFromVehicle);
    void _calcPrevWaypointValues(bool homePositionValid, double homeAlt, MissionItem* currentItem, MissionItem* prevItem, double* azimuth, double* distance, double* altDifference);
    bool _findLastAltitude(double* lastAltitude);
    bool _findLastAcceptanceRadius(double* lastAcceptanceRadius);

private:
    bool                _editMode;
    QmlObjectListModel* _missionItems;
    QmlObjectListModel  _waypointLines;
    Vehicle*            _activeVehicle;
    bool                _liveHomePositionAvailable;
    QGeoCoordinate      _liveHomePosition;
    bool                _autoSync;
    bool                _firstItemsFromVehicle;
    bool                _missionItemsRequested;
    bool                _queuedSend;

    static const char* _settingsGroup;
};

#endif
