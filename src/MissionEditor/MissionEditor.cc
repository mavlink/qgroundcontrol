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
    : QGCQmlWidgetHolder(QString(), NULL, parent)
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
        _initAllMissionItems();
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
        _deinitAllMissionItems();
        _missionItems->deleteLater();
    }
    
    MissionManager* missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    
    _canEdit = missionManager->canEdit();
    _missionItems = missionManager->copyMissionItems();
    
    _initAllMissionItems();
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
    // FIXME: Need to pull out home position
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (activeVehicle) {
        activeVehicle->missionManager()->writeMissionItems(*_missionItems, true /* skipFirstItem */);
        _missionItems->setDirty(false);
    }
}

int MissionEditor::addMissionItem(QGeoCoordinate coordinate)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
    }
    
    MissionItem * newItem = new MissionItem(this, _missionItems->count(), coordinate, MAV_CMD_NAV_WAYPOINT);
    _initMissionItem(newItem);
    newItem->setAltitude(30);
    if (_missionItems->count() == 1) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    qDebug() << "MissionItem" << newItem->coordinate();
    _missionItems->append(newItem);
    
    _recalcAll();
    
    return _missionItems->count() - 1;
}

void MissionEditor::removeMissionItem(int index)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
        return;
    }
    
    MissionItem* item = qobject_cast<MissionItem*>(_missionItems->removeAt(index));
    
    _deinitMissionItem(item);
    
    _recalcAll();
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
    
    _recalcAll();
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
    
    _recalcAll();
}

void MissionEditor::loadMissionFromFile(void)
{
    QString errorString;
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select Mission File to load");
    
    if (filename.isEmpty()) {
        return;
    }
    
    if (_missionItems) {
        _deinitAllMissionItems();
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
    
    _initAllMissionItems();
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

void MissionEditor::_recalcWaypointLines(void)
{
    bool firstCoordinateItem = true;
    MissionItem* lastCoordinateItem = qobject_cast<MissionItem*>(_missionItems->get(0));
    
    _waypointLines.clear();
    
    for (int i=1; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));
        
        if (item->specifiesCoordinate()) {
            if (firstCoordinateItem) {
                if (item->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
                    // The first coordinate we hit is a takeoff command so link back to home position
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
    
    emit waypointLinesChanged();
}

// This will update the sequence numbers to be sequential starting from 0
void MissionEditor::_recalcSequence(void)
{
    MissionItem* currentParentItem = qobject_cast<MissionItem*>(_missionItems->get(0));
    
    currentParentItem->childItems()->clear();
    
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));
        
        // Setup ascending sequence numbers
        item->setSequenceNumber(i);
    }
}

// This will update the child item hierarchy
void MissionEditor::_recalcChildItems(void)
{
    MissionItem* currentParentItem = qobject_cast<MissionItem*>(_missionItems->get(0));
    
    currentParentItem->childItems()->clear();
    
    for (int i=1; i<_missionItems->count(); i++) {
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

void MissionEditor::_recalcAll(void)
{
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
}

/// Initializes a new set of mission items which may have come from the vehicle or have been loaded from a file
void MissionEditor::_initAllMissionItems(void)
{
    // Add the home position item to the front
    MissionItem* homeItem = new MissionItem(this);
    homeItem->setHomePositionSpecialCase(true);
    homeItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    _missionItems->insert(0, homeItem);
    
    for (int i=0; i<_missionItems->count(); i++) {
        _initMissionItem(qobject_cast<MissionItem*>(_missionItems->get(i)));
    }
    
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
    
    emit missionItemsChanged();
    emit canEditChanged(_canEdit);
    
    _missionItems->setDirty(false);
}

void MissionEditor::_deinitAllMissionItems(void)
{
    for (int i=0; i<_missionItems->count(); i++) {
        _deinitMissionItem(qobject_cast<MissionItem*>(_missionItems->get(i)));
    }
}

void MissionEditor::_initMissionItem(MissionItem* item)
{
    _missionItems->setDirty(false);
    
    connect(item, &MissionItem::commandChanged,     this, &MissionEditor::_itemCommandChanged);
    connect(item, &MissionItem::coordinateChanged,  this, &MissionEditor::_itemCoordinateChanged);
}

void MissionEditor::_deinitMissionItem(MissionItem* item)
{
    disconnect(item, &MissionItem::commandChanged,     this, &MissionEditor::_itemCommandChanged);
    disconnect(item, &MissionItem::coordinateChanged,  this, &MissionEditor::_itemCoordinateChanged);
}

void MissionEditor::_itemCoordinateChanged(const QGeoCoordinate& coordinate)
{
    Q_UNUSED(coordinate);
    _recalcWaypointLines();
}

void MissionEditor::_itemCommandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command)
{
    Q_UNUSED(command);;
    _recalcChildItems();
    _recalcWaypointLines();
}
