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

class MissionEditor : public QGCQmlWidgetHolder
{
    Q_OBJECT
    
public:
    MissionEditor(QWidget* parent = NULL);
    ~MissionEditor();

    Q_PROPERTY(QmlObjectListModel*  missionItems    READ missionItems       NOTIFY missionItemsChanged)
    Q_PROPERTY(QmlObjectListModel*  waypointLines   READ waypointLines      NOTIFY waypointLinesChanged)
    Q_PROPERTY(bool                 canEdit         READ canEdit            NOTIFY canEditChanged)
    
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
    
signals:
    void missionItemsChanged(void);
    void canEditChanged(bool canEdit);
    void waypointLinesChanged(void);
    
private slots:
    void _newMissionItemsAvailable();
    void _missionListDirtyChanged(bool dirty);
    
private:
    void _reSequence(void);
    void _rebuildWaypointLines(void);
   
private:
    QmlObjectListModel* _missionItems;
    QmlObjectListModel  _waypointLines;
    bool                _canEdit;           ///< true: UI can edit these items, false: can't edit, can only send to vehicle or save
    
    static const char* _settingsGroup;
};

#endif
