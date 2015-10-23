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
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "QGCFileDialog.h"
#include "CoordinateVector.h"
#include "QGCMessageBox.h"
#include "FirmwarePlugin.h"

const char* MissionController::_settingsGroup = "MissionController";

MissionController::MissionController(QObject *parent)
    : QObject(parent)
    , _editMode(false)
    , _missionItems(NULL)
    , _canEdit(true)
    , _activeVehicle(NULL)
    , _liveHomePositionAvailable(false)
    , _autoSync(false)
    , _firstMissionItemSync(false)
    , _missionItemsRequested(false)
    , _queuedSend(false)
{

}

MissionController::~MissionController()
{
}

void MissionController::start(bool editMode)
{
    _editMode = editMode;

    MultiVehicleManager* multiVehicleMgr = MultiVehicleManager::instance();

    connect(multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &MissionController::_activeVehicleChanged);

    Vehicle* activeVehicle = multiVehicleMgr->activeVehicle();
    if (activeVehicle) {
        _activeVehicleChanged(activeVehicle);
    } else {
        _missionItemsRequested = true;
        _newMissionItemsAvailable();
    }
}

void MissionController::_newMissionItemsAvailable(void)
{
    if (_editMode) {
        if (_firstMissionItemSync) {
            // This is the first time the vehicle is seeing items. We have to be careful of transitioning from offline
            // to online.

            _firstMissionItemSync = false;
            if (_missionItems && _missionItems->count() > 1) {
                QGCMessageBox::StandardButton button = QGCMessageBox::warning("Mission Editing",
                                                                              "The vehicle has sent a new set of Mission Items. "
                                                                              "Do you want to discard your current set of unsaved items and use the ones from the vehicle instead?",
                                                                              QGCMessageBox::Yes | QGCMessageBox::No,
                                                                              QGCMessageBox::No);
                if (button == QGCMessageBox::No) {
                    return;
                }
            }
        } else if (!_missionItemsRequested) {
            // We didn't specifically ask for new mission items. Disregard the new set since it is
            // the most likely the set we just sent to the vehicle.
            return;
        }
    }

    _missionItemsRequested = false;

    if (_missionItems) {
        _deinitAllMissionItems();
        _missionItems->deleteLater();
    }

    MissionManager* missionManager = NULL;
    if (_activeVehicle) {
        missionManager = _activeVehicle->missionManager();
    }

    if (!missionManager || missionManager->inProgress()) {
        _canEdit = true;
        _missionItems = new QmlObjectListModel(this);
    } else {
        _canEdit = missionManager->canEdit();
        _missionItems = missionManager->copyMissionItems();
    }

    _initAllMissionItems();
}

void MissionController::getMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();

    if (activeVehicle) {
        _missionItemsRequested = true;
        MissionManager* missionManager = activeVehicle->missionManager();
        connect(missionManager, &MissionManager::newMissionItemsAvailable, this, &MissionController::_newMissionItemsAvailable);
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionController::sendMissionItems(void)
{
    Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();

    if (activeVehicle) {
        activeVehicle->missionManager()->writeMissionItems(*_missionItems);
        _missionItems->setDirty(false);
    }
}

int MissionController::addMissionItem(QGeoCoordinate coordinate)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
    }

    // Coordinate will come through without altitude
    coordinate.setAltitude(MissionItem::defaultAltitude);

    MissionItem * newItem = new MissionItem(this, _missionItems->count(), coordinate, MAV_CMD_NAV_WAYPOINT);
    _initMissionItem(newItem);
    if (_missionItems->count() == 1) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    _missionItems->append(newItem);

    _recalcAll();

    return _missionItems->count() - 1;
}

void MissionController::removeMissionItem(int index)
{
    if (!_canEdit) {
        qWarning() << "addMissionItem called with _canEdit == false";
        return;
    }

    MissionItem* item = qobject_cast<MissionItem*>(_missionItems->removeAt(index));

    _deinitMissionItem(item);

    _recalcAll();

    // Set the new current item

    if (index >= _missionItems->count()) {
        index--;
    }
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item =  qobject_cast<MissionItem*>(_missionItems->get(i));
        item->setIsCurrentItem(i == index);
    }
}

void MissionController::loadMissionFromFile(void)
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

    // FIXME: This needs to handle APM files which have WP 0 in them

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

void MissionController::saveMissionToFile(void)
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

        for (int i=1; i<_missionItems->count(); i++) {
            qobject_cast<MissionItem*>(_missionItems->get(i))->save(out);
        }
    }

    _missionItems->setDirty(false);
}

void MissionController::_recalcWaypointLines(void)
{
    MissionItem*    lastCoordinateItem =    qobject_cast<MissionItem*>(_missionItems->get(0));
    bool            firstCoordinateItem =   true;

    _waypointLines.clear();

    for (int i=1; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        if (item->specifiesCoordinate()) {
            if (firstCoordinateItem) {
                if (item->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
                    MissionItem* homeItem = qobject_cast<MissionItem*>(_missionItems->get(0));

                    // The first coordinate we hit is a takeoff command so link back to home position if valid
                    if (homeItem->homePositionValid()) {
                        _waypointLines.append(new CoordinateVector(qobject_cast<MissionItem*>(_missionItems->get(0))->coordinate(), item->coordinate()));
                    }
                } else {
                    // First coordiante is not a takeoff command, it does not link backwards to anything
                }
                firstCoordinateItem = false;
            } else if (!lastCoordinateItem->homePosition() || lastCoordinateItem->homePositionValid()) {
                // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                // is an invalid home position we skip the line
                _waypointLines.append(new CoordinateVector(lastCoordinateItem->coordinate(), item->coordinate()));
            }
            lastCoordinateItem = item;
        }
    }

    emit waypointLinesChanged();
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

// This will update the child item hierarchy
void MissionController::_recalcChildItems(void)
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

void MissionController::_recalcAll(void)
{
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
}

/// Initializes a new set of mission items which may have come from the vehicle or have been loaded from a file
void MissionController::_initAllMissionItems(void)
{
    MissionItem* homeItem = NULL;

    if (_activeVehicle && _activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() && _missionItems->count() != 0) {
        homeItem = qobject_cast<MissionItem*>(_missionItems->get(0));
        homeItem->setHomePositionSpecialCase(true);
    } else {
        // Add the home position item to the front
        homeItem = new MissionItem(this);
        homeItem->setHomePositionSpecialCase(true);
        homeItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
        _missionItems->insert(0, homeItem);
    }
    homeItem->setHomePositionValid(false);

    for (int i=0; i<_missionItems->count(); i++) {
        _initMissionItem(qobject_cast<MissionItem*>(_missionItems->get(i)));
    }

    _recalcAll();

    emit missionItemsChanged();
    emit canEditChanged(_canEdit);

    _missionItems->setDirty(false);

    connect(_missionItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_dirtyChanged);
}

void MissionController::_deinitAllMissionItems(void)
{
    for (int i=0; i<_missionItems->count(); i++) {
        _deinitMissionItem(qobject_cast<MissionItem*>(_missionItems->get(i)));
    }

    connect(_missionItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_dirtyChanged);
}

void MissionController::_initMissionItem(MissionItem* item)
{
    _missionItems->setDirty(false);

    connect(item, &MissionItem::commandChanged,     this, &MissionController::_itemCommandChanged);
    connect(item, &MissionItem::coordinateChanged,  this, &MissionController::_itemCoordinateChanged);
}

void MissionController::_deinitMissionItem(MissionItem* item)
{
    disconnect(item, &MissionItem::commandChanged,     this, &MissionController::_itemCommandChanged);
    disconnect(item, &MissionItem::coordinateChanged,  this, &MissionController::_itemCoordinateChanged);
}

void MissionController::_itemCoordinateChanged(const QGeoCoordinate& coordinate)
{
    Q_UNUSED(coordinate);
    _recalcWaypointLines();
}

void MissionController::_itemCommandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command)
{
    Q_UNUSED(command);;
    _recalcChildItems();
    _recalcWaypointLines();
}

void MissionController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        MissionManager* missionManager = _activeVehicle->missionManager();

        disconnect(missionManager, &MissionManager::newMissionItemsAvailable,   this, &MissionController::_newMissionItemsAvailable);
        disconnect(missionManager, &MissionManager::inProgressChanged,          this, &MissionController::_inProgressChanged);
        disconnect(_activeVehicle, &Vehicle::homePositionAvailableChanged,      this, &MissionController::_activeVehicleHomePositionAvailableChanged);
        disconnect(_activeVehicle, &Vehicle::homePositionChanged,               this, &MissionController::_activeVehicleHomePositionChanged);
        _activeVehicle = NULL;
        _newMissionItemsAvailable();
        _activeVehicleHomePositionAvailableChanged(false);
    }

    _activeVehicle = activeVehicle;

    if (_activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();

        connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailable);
        connect(missionManager, &MissionManager::inProgressChanged,          this, &MissionController::_inProgressChanged);
        connect(_activeVehicle, &Vehicle::homePositionAvailableChanged,     this, &MissionController::_activeVehicleHomePositionAvailableChanged);
        connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);

        if (missionManager->inProgress()) {
            // Vehicle is still in process of requesting mission items
            _firstMissionItemSync = true;
        } else {
            // Vehicle already has mission items
            _firstMissionItemSync = false;
        }

        _missionItemsRequested = true;
        _newMissionItemsAvailable();

        _activeVehicleHomePositionChanged(_activeVehicle->homePosition());
        _activeVehicleHomePositionAvailableChanged(_activeVehicle->homePositionAvailable());
    }
}

void MissionController::_activeVehicleHomePositionAvailableChanged(bool homePositionAvailable)
{
    _liveHomePositionAvailable = homePositionAvailable;
    emit liveHomePositionAvailableChanged(_liveHomePositionAvailable);
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    _liveHomePosition = homePosition;
    emit liveHomePositionChanged(_liveHomePosition);
}

void MissionController::deleteCurrentMissionItem(void)
{
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item =  qobject_cast<MissionItem*>(_missionItems->get(i));
        if (item->isCurrentItem() && i != 0) {
            removeMissionItem(i);
            return;
        }
    }
}

void MissionController::setAutoSync(bool autoSync)
{
    // FIXME: AutoSync temporarily turned off
#if 0
    _autoSync = autoSync;
    emit autoSyncChanged(_autoSync);

    if (_autoSync) {
        _dirtyChanged(true);
    }
#else
    Q_UNUSED(autoSync)
#endif
}

void MissionController::_dirtyChanged(bool dirty)
{
    if (dirty && _autoSync) {
        Vehicle* activeVehicle = MultiVehicleManager::instance()->activeVehicle();

        if (activeVehicle && !activeVehicle->armed()) {
            if (_activeVehicle->missionManager()->inProgress()) {
                _queuedSend = true;
            } else {
                _autoSyncSend();
            }
        }
    }
}

void MissionController::_autoSyncSend(void)
{
    qDebug() << "Auto-syncing with vehicle";
    _queuedSend = false;
    if (_missionItems) {
        sendMissionItems();
        _missionItems->setDirty(false);
    }
}

void MissionController::_inProgressChanged(bool inProgress)
{
    if (!inProgress && _queuedSend) {
        _autoSyncSend();
    }
}

QmlObjectListModel* MissionController::missionItems(void)
{
    return _missionItems;
}
