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
#include "QGCFileDialog.h"
#include "CoordinateVector.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>

const char* MissionEditor::_settingsGroup = "MissionEditor";

MissionEditor::MissionEditor(QWidget *parent)
    : QGCQmlWidgetHolder(parent)
    , _missionItems(NULL)
    , _canEdit(true)
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
        connect(_missionItems, &QmlObjectListModel::dirtyChanged, this, &MissionEditor::_missionListDirtyChanged);
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
    
    MissionManager* missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    
    _canEdit = missionManager->canEdit();
    _missionItems = missionManager->copyMissionItems();
    _reSequence();
    _missionItems->setDirty(false);
    
    connect(_missionItems, &QmlObjectListModel::dirtyChanged, this, &MissionEditor::_missionListDirtyChanged);
    _rebuildWaypointLines();
    
    emit missionItemsChanged();
    emit canEditChanged(_canEdit);
}

void MissionEditor::getMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();
        connect(missionManager, &MissionManager::newMissionItemsAvailable, this, &MissionEditor::_newMissionItemsAvailable);
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionEditor::setMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (activeVehicle) {
        activeVehicle->missionManager()->writeMissionItems(*_missionItems);
        _missionItems->setDirty(false);
    }
}

int MissionEditor::addMissionItem(QGeoCoordinate coordinate)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
    }
    
    MissionItem * newItem = new MissionItem(this, _missionItems->count(), coordinate, MAV_CMD_NAV_WAYPOINT);
    newItem->setAltitude(30);
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
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
        return;
    }
    
    _missionItems->removeAt(index);
    _reSequence();
}

void MissionEditor::moveUp(int index)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
        return;
    }
    
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
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
        return;
    }
    
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

void MissionEditor::loadMissionFromFile(void)
{
    QString errorString;
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select Mission File to load");
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (_missionItems) {
        _missionItems->deleteLater();
    }
    _missionItems = new QmlObjectListModel(this);

    _canEdit = true;
    
    QFile file(filename);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString();
    } else {
        QTextStream in(&file);
        
        const QStringList& version = in.readLine().split(" ");
        
        if (!(version.size() == 3 && version[0] == "QGC" && version[1] == "WPL" && version[2] == "120")) {
            errorString = "The mission file is not compatible with the current version of QGroundControl.";
        } else {
            while (!in.atEnd()) {
                MissionItem* item = new MissionItem();
                
                if (item->load(in)) {
                    _missionItems->append(item);
                    
                    if (!item->canEdit()) {
                        _canEdit = false;
                    }
                } else {
                    errorString = "The mission file is corrupted.";
                    break;
                }
            }
        }
        
    }
    
    if (!errorString.isEmpty()) {
        _missionItems->clear();
    }
    
    _missionItems->setDirty(false);
    emit canEditChanged(_canEdit);
    
    connect(_missionItems, &QmlObjectListModel::dirtyChanged, this, &MissionEditor::_missionListDirtyChanged);
    _rebuildWaypointLines();
}

void MissionEditor::saveMissionToFile(void)
{
    QString errorString;
    QString filename = QGCFileDialog::getSaveFileName(NULL, "Select file to save mission to");
    
    if (filename.isEmpty()) {
        return;
    }
    
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = file.errorString();
    } else {
        QTextStream out(&file);
        
        out << "QGC WPL 120\r\n";   // Version string
        
        for (int i=0; i<_missionItems->count(); i++) {
            qobject_cast<MissionItem*>(_missionItems->get(i))->save(out);
        }
    }
    
    _missionItems->setDirty(false);
}

void MissionEditor::_rebuildWaypointLines(void)
{
    _waypointLines.clear();
    for (int i=1; i<_missionItems->count(); i++) {
        MissionItem* item1 = qobject_cast<MissionItem*>(_missionItems->get(i-1));
        MissionItem* item2 = qobject_cast<MissionItem*>(_missionItems->get(i));
        
        _waypointLines.append(new CoordinateVector(item1->coordinate(), item2->coordinate()));
    }
    emit waypointLinesChanged();
}

void MissionEditor::_missionListDirtyChanged(bool dirty)
{
    Q_UNUSED(dirty);
    _rebuildWaypointLines();
}
