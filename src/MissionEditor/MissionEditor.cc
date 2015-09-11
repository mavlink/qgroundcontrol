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
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
#ifndef __android__
    setMinimumWidth( 31 * ScreenToolsController::defaultFontPixelSize_s());
    setMinimumHeight(33 * ScreenToolsController::defaultFontPixelSize_s());
#endif
    
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

void MissionEditor::saveSetting(const QString &name, const QString& value)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    
    settings.setValue(name, value);
}

QString MissionEditor::loadSetting(const QString &name, const QString& defaultValue)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    
    return settings.value(name, defaultValue).toString();
}

void MissionEditor::addMissionItem(QGeoCoordinate coordinate)
{
    MissionItem * newItem = new MissionItem(this, _missionItems->count(), coordinate);
    if (_missionItems->count() == 0) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    qDebug() << "MissionItem" << newItem->coordinate();
    _missionItems->append(newItem);
}
