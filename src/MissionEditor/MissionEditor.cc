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

#include "MissionEditor.h"
#include "ScreenToolsController.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>

const char* MissionEditor::_settingsGroup = "MissionEditor";

MissionEditor::MissionEditor(QWidget *parent)
    : QGCQmlWidgetHolder(parent)
    , _missionItems(NULL)
{
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
    
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    if (activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();
        connect(missionManager, &MissionManager::newMissionItemsAvailable, this, &MissionEditor::_newMissionItemsAvailable);
        _newMissionItemsAvailable();
    } else {
        _missionItems = new QmlObjectListModel(this);
    }
    
    setContextPropertyObject("controller", this);

    setSource(QUrl::fromUserInput("qrc:/qml/MissionEditor.qml"));
}

MissionEditor::~MissionEditor()
{
}

void MissionEditor::_newMissionItemsAvailable(void)
{
    if (_missionItems) {
        _missionItems->deleteLater();
    }
    
    _missionItems = MultiVehicleManager::instance()->activeVehicle()->missionManager()->copyMissionItems();
    _reSequence();
    
    emit missionItemsChanged();
}

void MissionEditor::getMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (activeVehicle) {
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionEditor::setMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (activeVehicle) {
        activeVehicle->missionManager()->writeMissionItems(*_missionItems);
    }
}

int MissionEditor::addMissionItem(QGeoCoordinate coordinate)
{
    MissionItem * newItem = new MissionItem(this, _missionItems->count(), coordinate);
    if (_missionItems->count() == 0) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    qDebug() << "MissionItem" << newItem->coordinate();
    _missionItems->append(newItem);
    
    return _missionItems->count() - 1;
}

void MissionEditor::_reSequence(void)
{
    for (int i=0; i<_missionItems->count(); i++) {
        qobject_cast<MissionItem*>(_missionItems->get(i))->setSequenceNumber(i);
    }
}

void MissionEditor::removeMissionItem(int index)
{
    _missionItems->removeAt(index);
    _reSequence();
}

void MissionEditor::moveUp(int index)
{
    if (_missionItems->count() < 2 || index <= 0 || index >= _missionItems->count()) {
        return;
    }
    
    MissionItem item1 = *qobject_cast<MissionItem*>(_missionItems->get(index - 1));
    MissionItem item2 = *qobject_cast<MissionItem*>(_missionItems->get(index));
    
    _missionItems->removeAt(index - 1);
    _missionItems->removeAt(index - 1);
    
    _missionItems->insert(index - 1, new MissionItem(item2, _missionItems));
    _missionItems->insert(index, new MissionItem(item1, _missionItems));
    
    _reSequence();
}

void MissionEditor::moveDown(int index)
{
    if (_missionItems->count() < 2 || index >= _missionItems->count() - 1) {
        return;
    }
    
    MissionItem item1 = *qobject_cast<MissionItem*>(_missionItems->get(index));
    MissionItem item2 = *qobject_cast<MissionItem*>(_missionItems->get(index + 1));
    
    _missionItems->removeAt(index);
    _missionItems->removeAt(index);
    
    _missionItems->insert(index, new MissionItem(item2, _missionItems));
    _missionItems->insert(index + 1, new MissionItem(item1, _missionItems));
    
    _reSequence();
}
