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
#include "CoordinateVector.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "SimpleMissionItem.h"
#include "ComplexMissionItem.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#endif

QGC_LOGGING_CATEGORY(MissionControllerLog, "MissionControllerLog")

const char* MissionController::_settingsGroup =                 "MissionController";
const char* MissionController::_jsonVersionKey =                "version";
const char* MissionController::_jsonGroundStationKey =          "groundStation";
const char* MissionController::_jsonMavAutopilotKey =           "MAV_AUTOPILOT";
const char* MissionController::_jsonItemsKey =                  "items";
const char* MissionController::_jsonPlannedHomePositionKey =    "plannedHomePosition";

MissionController::MissionController(QObject *parent)
    : QObject(parent)
    , _editMode(false)
    , _missionItems(NULL)
    , _complexMissionItems(NULL)
    , _activeVehicle(NULL)
    , _autoSync(false)
    , _firstItemsFromVehicle(false)
    , _missionItemsRequested(false)
    , _queuedSend(false)
{

}

MissionController::~MissionController()
{

}

void MissionController::start(bool editMode)
{
    qCDebug(MissionControllerLog) << "start editMode" << editMode;

    _editMode = editMode;

    MultiVehicleManager* multiVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();

    connect(multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &MissionController::_activeVehicleChanged);
    _activeVehicleChanged(multiVehicleMgr->activeVehicle());

    // We start with an empty mission
    _missionItems = new QmlObjectListModel(this);
    _addPlannedHomePosition(_missionItems, false /* addToCenter */);
    _initAllMissionItems();
}

// Called when new mission items have completed downloading from Vehicle
void MissionController::_newMissionItemsAvailableFromVehicle(void)
{
    qCDebug(MissionControllerLog) << "_newMissionItemsAvailableFromVehicle";

    if (!_editMode || _missionItemsRequested || _missionItems->count() == 1) {
        // Fly Mode:
        //      - Always accepts new items fromthe vehicle so Fly view is kept up to date
        // Edit Mode:
        //      - Either a load from vehicle was manually requested or
        //      - The initial automatic load from a vehicle completed and the current editor it empty
        _deinitAllMissionItems();
        _missionItems->deleteLater();

        _missionItems = _activeVehicle->missionManager()->copyMissionItems();
        qCDebug(MissionControllerLog) << "loading from vehicle count"<< _missionItems->count();

        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() || _missionItems->count() == 0) {
            _addPlannedHomePosition(_missionItems,true /* addToCenter */);
        }

        _missionItemsRequested = false;

        _initAllMissionItems();
        emit newItemsFromVehicle();
    }
}

void MissionController::getMissionItems(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (activeVehicle) {
        _missionItemsRequested = true;
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionController::sendMissionItems(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (activeVehicle) {
        activeVehicle->missionManager()->writeMissionItems(*_missionItems);
        _missionItems->setDirty(false);
    }
}

int MissionController::insertSimpleMissionItem(QGeoCoordinate coordinate, int i)
{
    MissionItem * newItem = new SimpleMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(_missionItems->count());
    newItem->setCoordinate(coordinate);
    newItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    _initMissionItem(newItem);
    if (_missionItems->count() == 1) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    newItem->setDefaultsForCommand();
    if ((MAV_CMD)newItem->command() == MAV_CMD_NAV_WAYPOINT) {
        double lastValue;

        if (_findLastAcceptanceRadius(&lastValue)) {
            newItem->setParam2(lastValue);
        }
        if (_findLastAltitude(&lastValue)) {
            newItem->setParam7(lastValue);
        }
    }
    _missionItems->insert(i, newItem);

    _recalcAll();

    return _missionItems->count() - 1;
}

int MissionController::insertComplexMissionItem(QGeoCoordinate coordinate, int i)
{
    ComplexMissionItem * newItem = new ComplexMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(_missionItems->count());
    newItem->setCoordinate(coordinate);
    newItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    _initMissionItem(newItem);

    _missionItems->insert(i, newItem);
    _complexMissionItems->append(newItem);

    _recalcAll();

    return _missionItems->count() - 1;
}

void MissionController::removeMissionItem(int index)
{
    MissionItem* item = qobject_cast<MissionItem*>(_missionItems->removeAt(index));

    _deinitMissionItem(item);
    item->deleteLater();

    _recalcAll();

    // Set the new current item

    if (index >= _missionItems->count()) {
        index--;
    }
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item =  qobject_cast<MissionItem*>(_missionItems->get(i));
        item->setIsCurrentItem(i == index);
    }
    _missionItems->setDirty(true);
}

void MissionController::removeAllMissionItems(void)
{
    if (_missionItems) {
        QmlObjectListModel* oldItems = _missionItems;
        _missionItems = new QmlObjectListModel(this);
        _addPlannedHomePosition(_missionItems, false /* addToCenter */);
        _initAllMissionItems();
        oldItems->deleteLater();
    }
}

bool MissionController::_loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* missionItems, QString& errorString)
{
    QJsonParseError jsonParseError;
    QJsonDocument   jsonDoc(QJsonDocument::fromJson(bytes, &jsonParseError));

    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = jsonParseError.errorString();
        return false;
    }

    QJsonObject json = jsonDoc.object();

    if (!json.contains(_jsonVersionKey)) {
        errorString = QStringLiteral("File is missing version key");
        return false;
    }
    if (json[_jsonVersionKey].toString() != QLatin1String("1.0")) {
        errorString = QStringLiteral("QGroundControl does not support this file version");
        return false;
    }

    if (json.contains(_jsonItemsKey)) {
        if (!json[_jsonItemsKey].isArray()) {
            errorString = QStringLiteral("items values must be array");
            return false;
        }

        QJsonArray itemArray(json[_jsonItemsKey].toArray());
        foreach (const QJsonValue& itemValue, itemArray) {
            if (!itemValue.isObject()) {
                errorString = QStringLiteral("Mission item is not an object");
                return false;
            }

            MissionItem* item = new SimpleMissionItem(_activeVehicle, this);
            if (item->load(itemValue.toObject(), errorString)) {
                missionItems->append(item);
            } else {
                return false;
            }
        }
    }

    if (json.contains(_jsonPlannedHomePositionKey)) {
        MissionItem* item = new SimpleMissionItem(_activeVehicle, this);

        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), errorString)) {
            missionItems->insert(0, item);
        } else {
            return false;
        }
    } else {
        _addPlannedHomePosition(missionItems, true /* addToCenter */);
    }

    return true;
}

bool MissionController::_loadTextMissionFile(QTextStream& stream, QmlObjectListModel* missionItems, QString& errorString)
{
    bool addPlannedHomePosition = false;

    QString firstLine = stream.readLine();
    const QStringList& version = firstLine.split(QChar(' '));

    bool versionOk = false;
    if (version.size() == 3 && version[0] == QLatin1String("QGC") && version[1] == QLatin1String("WPL")) {
        if (version[2] == QLatin1String("110")) {
            // ArduPilot file, planned home position is already in position 0
            versionOk = true;
        } else if (version[2] == QLatin1String("120")) {
            // Old QGC file, no planned home position
            versionOk = true;
            addPlannedHomePosition = true;
        }
    }

    if (versionOk) {
        while (!stream.atEnd()) {
            MissionItem* item = new SimpleMissionItem(_activeVehicle, this);

            if (item->load(stream)) {
                missionItems->append(item);
            } else {
                errorString = QStringLiteral("The mission file is corrupted.");
                return false;
            }
        }
    } else {
        errorString = QStringLiteral("The mission file is not compatible with this version of QGroundControl.");
        return false;
    }

    if (addPlannedHomePosition || missionItems->count() == 0) {
        _addPlannedHomePosition(missionItems, true /* addToCenter */);

        // Update sequence numbers in DO_JUMP commands to take into account added home position
        for (int i=1; i<missionItems->count(); i++) {
            MissionItem* item = qobject_cast<MissionItem*>(missionItems->get(i));
            if (item->command() == MavlinkQmlSingleton::MAV_CMD_DO_JUMP) {
                // Home is in position 0
                item->setParam1((int)item->param1() + 1);
            }
        }
    }

    return true;
}

void MissionController::_loadMissionFromFile(const QString& filename)
{
    QString errorString;

    if (filename.isEmpty()) {
        return;
    }

    QmlObjectListModel* newMissionItems = new QmlObjectListModel(this);

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString();
    } else {
        QByteArray  bytes = file.readAll();
        QTextStream stream(&bytes);

        QString firstLine = stream.readLine();
        if (firstLine.contains(QRegExp(QStringLiteral("QGC.*WPL")))) {
            stream.seek(0);
            _loadTextMissionFile(stream, newMissionItems, errorString);
        } else {
            _loadJsonMissionFile(bytes, newMissionItems, errorString);
        }
    }

    if (!errorString.isEmpty()) {
        delete newMissionItems;
        qgcApp()->showMessage(errorString);
        return;
    }

    if (_missionItems) {
        _deinitAllMissionItems();
        _missionItems->deleteLater();
    }
    _missionItems = newMissionItems;
    if (_missionItems->count() == 0) {
        _addPlannedHomePosition(_missionItems, true /* addToCenter */);
    }

    _initAllMissionItems();
}

void MissionController::loadMissionFromFile(void)
{
#ifndef __mobile__

    QString filename = QGCFileDialog::getOpenFileName(
        NULL,
        QStringLiteral("Select Mission File to load"),
        QString(),
        QStringLiteral("Mission file (*.mission);;All Files (*.*)"));

    if (filename.isEmpty()) {
        return;
    }
    _loadMissionFromFile(filename);
#endif
}

void MissionController::_saveMissionToFile(const QString& filename)
{
    qDebug() << filename;

    if (filename.isEmpty()) {
        return;
    }

    QString missionFilename = filename;
    if (!QFileInfo(filename).fileName().contains(QLatin1String("."))) {
        missionFilename += QStringLiteral(".mission");
    }

    QFile file(missionFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject missionObject;

        missionObject[_jsonVersionKey] = "1.0";
        missionObject[_jsonGroundStationKey] = "QGroundControl";

        MAV_AUTOPILOT firmwareType = MAV_AUTOPILOT_GENERIC;
        if (_activeVehicle) {
            firmwareType = _activeVehicle->firmwareType();
        } else {
            // FIXME: Hack duplicated code from QGroundControlQmlGlobal. Had to do this for now since
            // QGroundControlQmlGlobal is not available from C++ side.

            QSettings settings;
            firmwareType = (MAV_AUTOPILOT)settings.value(QStringLiteral("OfflineEditingFirmwareType"), MAV_AUTOPILOT_ARDUPILOTMEGA).toInt();
        }
        missionObject[_jsonMavAutopilotKey] = firmwareType;

        QJsonObject homePositionObject;
        qobject_cast<MissionItem*>(_missionItems->get(0))->save(homePositionObject);
        missionObject[QStringLiteral("plannedHomePosition")] = homePositionObject;

        QJsonArray itemArray;
        for (int i=1; i<_missionItems->count(); i++) {
            QJsonObject itemObject;
            qobject_cast<MissionItem*>(_missionItems->get(i))->save(itemObject);
            itemArray.append(itemObject);
        }
        missionObject[QStringLiteral("items")] = itemArray;

        QJsonDocument saveDoc(missionObject);
        file.write(saveDoc.toJson());
    }

    _missionItems->setDirty(false);
}

void MissionController::saveMissionToFile(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(NULL, QStringLiteral("Select file to save mission to"), QString(), QStringLiteral("Mission file (*.mission);;All Files (*.*)"));

    if (filename.isEmpty()) {
        return;
    }
    _saveMissionToFile(filename);
#endif
}

void MissionController::saveMobileMissionToFile(const QString& filename)
{
    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return;
    }

    _saveMissionToFile(docDirs.at(0) + QDir::separator() + filename);
}

void MissionController::loadMobileMissionFromFile(const QString& filename)
{
    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return;
    }
    _loadMissionFromFile(docDirs.at(0) + QDir::separator() + filename);
}

void MissionController::_calcPrevWaypointValues(double homeAlt, MissionItem* currentItem, MissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  prevCoord =     prevItem->coordinate();
    bool            distanceOk =    false;

    // Convert to fixed altitudes

    qCDebug(MissionControllerLog) << homeAlt
                                  << currentItem->relativeAltitude() << currentItem->coordinate().altitude()
                                  << prevItem->relativeAltitude() << prevItem->coordinate().altitude();

    distanceOk = true;
    if (currentItem->relativeAltitude()) {
        currentCoord.setAltitude(homeAlt + currentCoord.altitude());
    }
    if (prevItem->relativeAltitude()) {
        prevCoord.setAltitude(homeAlt + prevCoord.altitude());
    }

    qCDebug(MissionControllerLog) << "distanceOk" << distanceOk;

    if (distanceOk) {
        *altDifference = currentCoord.altitude() - prevCoord.altitude();
        *distance = prevCoord.distanceTo(currentCoord);
        *azimuth = prevCoord.azimuthTo(currentCoord);
    } else {
        *altDifference = 0.0;
        *azimuth = 0.0;
        *distance = -1.0;   // Signals no values
    }
}

void MissionController::_recalcWaypointLines(void)
{
    MissionItem*    lastCoordinateItem =    qobject_cast<MissionItem*>(_missionItems->get(0));
    MissionItem*    homeItem =              lastCoordinateItem;
    bool            firstCoordinateItem =   true;
    bool            showHomePosition =      homeItem->showHomePosition();
    double          homeAlt =               homeItem->coordinate().altitude();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(-1.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    double homePositionAltitude = homeItem->coordinate().altitude();
    minAltSeen = maxAltSeen = homeItem->coordinate().altitude();

    _waypointLines.clear();

    bool linkBackToHome = false;
    for (int i=1; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(-1.0);

        if (firstCoordinateItem && item->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
        }

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->relativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);

            if (!item->standaloneCoordinate()) {
                firstCoordinateItem = false;
                if (!lastCoordinateItem->homePosition() || (showHomePosition && linkBackToHome)) {
                    double azimuth, distance, altDifference;

                    // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                    // is an invalid home position we skip the line
                    _calcPrevWaypointValues(homeAlt, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);
                    _waypointLines.append(new CoordinateVector(lastCoordinateItem->coordinate(), item->coordinate()));
                }
                lastCoordinateItem = item;
            }
        }
    }

    // Walk the list again calculating altitude percentages
    double altRange = maxAltSeen - minAltSeen;
    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->relativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
            }
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

/// Initializes a new set of mission items
void MissionController::_initAllMissionItems(void)
{
    MissionItem* homeItem = NULL;

    homeItem = qobject_cast<MissionItem*>(_missionItems->get(0));
    homeItem->setHomePositionSpecialCase(true);
    homeItem->setShowHomePosition(_editMode);
    homeItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->setFrame(MAV_FRAME_GLOBAL);
    homeItem->setIsCurrentItem(true);

    if (!_editMode && _activeVehicle && _activeVehicle->homePositionAvailable()) {
        homeItem->setCoordinate(_activeVehicle->homePosition());
        homeItem->setShowHomePosition(true);
    }

    qDebug() << "home item" << homeItem->coordinate();

    QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);

    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        if (!item->simpleItem()) {
            newComplexItems->append(item);
        }
        _initMissionItem(item);
    }

    delete _complexMissionItems;
    _complexMissionItems = newComplexItems;

    _recalcAll();

    emit missionItemsChanged();
    emit complexMissionItemsChanged();

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
    connect(item, &MissionItem::frameChanged,       this, &MissionController::_itemFrameChanged);
}

void MissionController::_deinitMissionItem(MissionItem* item)
{
    disconnect(item, &MissionItem::commandChanged,     this, &MissionController::_itemCommandChanged);
    disconnect(item, &MissionItem::coordinateChanged,  this, &MissionController::_itemCoordinateChanged);
    disconnect(item, &MissionItem::frameChanged,       this, &MissionController::_itemFrameChanged);
}

void MissionController::_itemCoordinateChanged(const QGeoCoordinate& coordinate)
{
    Q_UNUSED(coordinate);
    _recalcWaypointLines();
}

void MissionController::_itemFrameChanged(int frame)
{
    Q_UNUSED(frame);
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
    qCDebug(MissionControllerLog) << "_activeVehicleChanged activeVehicle" << activeVehicle;

    if (_activeVehicle) {
        MissionManager* missionManager = _activeVehicle->missionManager();

        disconnect(missionManager, &MissionManager::newMissionItemsAvailable,   this, &MissionController::_newMissionItemsAvailableFromVehicle);
        disconnect(missionManager, &MissionManager::inProgressChanged,          this, &MissionController::_inProgressChanged);
        disconnect(missionManager, &MissionManager::currentItemChanged,         this, &MissionController::_currentMissionItemChanged);
        disconnect(_activeVehicle, &Vehicle::homePositionAvailableChanged,      this, &MissionController::_activeVehicleHomePositionAvailableChanged);
        disconnect(_activeVehicle, &Vehicle::homePositionChanged,               this, &MissionController::_activeVehicleHomePositionChanged);
        _activeVehicle = NULL;
    }

    _activeVehicle = activeVehicle;

    if (_activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();

        connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailableFromVehicle);
        connect(missionManager, &MissionManager::inProgressChanged,         this, &MissionController::_inProgressChanged);
        connect(missionManager, &MissionManager::currentItemChanged,        this, &MissionController::_currentMissionItemChanged);
        connect(_activeVehicle, &Vehicle::homePositionAvailableChanged,     this, &MissionController::_activeVehicleHomePositionAvailableChanged);
        connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);

        if (!_editMode) {
            removeAllMissionItems();
        }

        _activeVehicleHomePositionChanged(_activeVehicle->homePosition());
        _activeVehicleHomePositionAvailableChanged(_activeVehicle->homePositionAvailable());
    }
}

void MissionController::_activeVehicleHomePositionAvailableChanged(bool homePositionAvailable)
{
    if (!_editMode && _missionItems) {
        qobject_cast<MissionItem*>(_missionItems->get(0))->setShowHomePosition(homePositionAvailable);
        _recalcWaypointLines();
    }
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    if (!_editMode && _missionItems) {
        qobject_cast<MissionItem*>(_missionItems->get(0))->setCoordinate(homePosition);
        _recalcWaypointLines();
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
        Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

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
    emit syncInProgressChanged(inProgress);
    if (!inProgress && _queuedSend) {
        _autoSyncSend();
    }
}

QmlObjectListModel* MissionController::missionItems(void)
{
    return _missionItems;
}

QmlObjectListModel* MissionController::complexMissionItems(void)
{
    return _complexMissionItems;
}

bool MissionController::_findLastAltitude(double* lastAltitude)
{
    bool found = false;
    double foundAltitude;

    // Don't use home position
    for (int i=1; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        if (item->specifiesCoordinate() && !item->standaloneCoordinate()) {
            foundAltitude = item->param7();
            found = true;
        }
    }

    if (found) {
        *lastAltitude = foundAltitude;
    }

    return found;
}

bool MissionController::_findLastAcceptanceRadius(double* lastAcceptanceRadius)
{
    bool found = false;
    double foundAcceptanceRadius;

    for (int i=0; i<_missionItems->count(); i++) {
        MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));

        if ((MAV_CMD)item->command() == MAV_CMD_NAV_WAYPOINT) {
            foundAcceptanceRadius = item->param2();
            found = true;
        }
    }

    if (found) {
        *lastAcceptanceRadius = foundAcceptanceRadius;
    }

    return found;
}

double MissionController::_normalizeLat(double lat)
{
    // Normalize latitude to range: 0 to 180, S to N
    return lat + 90.0;
}

double MissionController::_normalizeLon(double lon)
{
    // Normalize longitude to range: 0 to 360, W to E
    return lon  + 180.0;
}

/// Add the home position item to the front of the list
void MissionController::_addPlannedHomePosition(QmlObjectListModel* missionItems, bool addToCenter)
{
    MissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    missionItems->insert(0, homeItem);

    if (missionItems->count() > 1  && addToCenter) {
        MissionItem* item = qobject_cast<MissionItem*>(missionItems->get(1));

        double north = _normalizeLat(item->coordinate().latitude());
        double south = north;
        double east = _normalizeLon(item->coordinate().longitude());
        double west = east;

        for (int i=2; i<missionItems->count(); i++) {
            item = qobject_cast<MissionItem*>(missionItems->get(i));

            double lat = _normalizeLat(item->coordinate().latitude());
            double lon = _normalizeLon(item->coordinate().longitude());

            north = fmax(north, lat);
            south = fmin(south, lat);
            east = fmax(east, lon);
            west = fmin(west, lon);
        }

        homeItem->setCoordinate(QGeoCoordinate((south + ((north - south) / 2)) - 90.0, (west + ((east - west) / 2)) - 180.0, 0.0));
    } else {
        homeItem->setCoordinate(qgcApp()->lastKnownHomePosition());
    }
}

void MissionController::_currentMissionItemChanged(int sequenceNumber)
{
    if (!_editMode) {
        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            sequenceNumber++;
        }

        for (int i=0; i<_missionItems->count(); i++) {
            MissionItem* item = qobject_cast<MissionItem*>(_missionItems->get(i));
            item->setIsCurrentItem(item->sequenceNumber() == sequenceNumber);
        }
    }
}

QStringList MissionController::getMobileMissionFiles(void)
{
    QStringList missionFiles;

    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return QStringList();
    }
    QDir missionDir = docDirs.at(0);

    QFileInfoList missionFileInfoList = missionDir.entryInfoList(QStringList(QStringLiteral("*.mission")),  QDir::Files, QDir::Name);

    foreach (const QFileInfo& missionFileInfo, missionFileInfoList) {
        missionFiles << missionFileInfo.baseName() + ".mission";
    }

    return missionFiles;
}

bool MissionController::syncInProgress(void)
{
    qDebug() << _activeVehicle->missionManager()->inProgress();
    return _activeVehicle->missionManager()->inProgress();
}
