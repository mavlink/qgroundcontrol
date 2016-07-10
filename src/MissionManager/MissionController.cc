/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionController.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "CoordinateVector.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "SimpleMissionItem.h"
#include "ComplexMissionItem.h"
#include "JsonHelper.h"
#include "ParameterLoader.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#endif

QGC_LOGGING_CATEGORY(MissionControllerLog, "MissionControllerLog")

const char* MissionController::jsonSimpleItemsKey = "items";

const char* MissionController::_settingsGroup =                 "MissionController";
const char* MissionController::_jsonVersionKey =                "version";
const char* MissionController::_jsonGroundStationKey =          "groundStation";
const char* MissionController::_jsonMavAutopilotKey =           "MAV_AUTOPILOT";
const char* MissionController::_jsonComplexItemsKey =           "complexItems";
const char* MissionController::_jsonPlannedHomePositionKey =    "plannedHomePosition";

MissionController::MissionController(QObject *parent)
    : QObject(parent)
    , _editMode(false)
    , _visualItems(NULL)
    , _complexItems(NULL)
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
    _visualItems = new QmlObjectListModel(this);
    _addPlannedHomePosition(_visualItems, false /* addToCenter */);
    _initAllVisualItems();
}

// Called when new mission items have completed downloading from Vehicle
void MissionController::_newMissionItemsAvailableFromVehicle(void)
{
    qCDebug(MissionControllerLog) << "_newMissionItemsAvailableFromVehicle";

    if (!_editMode || _missionItemsRequested || _visualItems->count() == 1) {
        // Fly Mode:
        //      - Always accepts new items from the vehicle so Fly view is kept up to date
        // Edit Mode:
        //      - Either a load from vehicle was manually requested or
        //      - The initial automatic load from a vehicle completed and the current editor is empty

        QmlObjectListModel* newControllerMissionItems = new QmlObjectListModel(this);
        const QList<MissionItem*>& newMissionItems = _activeVehicle->missionManager()->missionItems();

        qCDebug(MissionControllerLog) << "loading from vehicle: count"<< _visualItems->count();
        foreach(const MissionItem* missionItem, newMissionItems) {
            newControllerMissionItems->append(new SimpleMissionItem(_activeVehicle, *missionItem, this));
        }

        _deinitAllVisualItems();

        _visualItems->deleteListAndContents();
        _visualItems = newControllerMissionItems;

        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() || _visualItems->count() == 0) {
            _addPlannedHomePosition(_visualItems,true /* addToCenter */);
        }

        _missionItemsRequested = false;

        _initAllVisualItems();
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
    if (_activeVehicle) {
        // Convert to MissionItems so we can send to vehicle
        QList<MissionItem*> missionItems;

        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            if (visualItem->isSimpleItem()) {
                missionItems.append(new MissionItem(qobject_cast<SimpleMissionItem*>(visualItem)->missionItem()));
            } else {
                ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(visualItem);
                QmlObjectListModel* complexMissionItems = complexItem->getMissionItems();
                for (int j=0; j<complexMissionItems->count(); j++) {
                    missionItems.append(new MissionItem(*qobject_cast<MissionItem*>(complexMissionItems->get(j))));
                }
                delete complexMissionItems;
            }
        }

        _activeVehicle->missionManager()->writeMissionItems(missionItems);
        _visualItems->setDirty(false);

        for (int i=0; i<missionItems.count(); i++) {
            delete missionItems[i];
        }
    }
}

int MissionController::_nextSequenceNumber(void)
{
    if (_visualItems->count() == 0) {
        qWarning() << "Internal error: Empty visual item list";
        return 0;
    } else {
        VisualMissionItem* lastItem = qobject_cast<VisualMissionItem*>(_visualItems->get(_visualItems->count() - 1));

        if (lastItem->isSimpleItem()) {
            return lastItem->sequenceNumber() + 1;
        } else {
            return qobject_cast<ComplexMissionItem*>(lastItem)->lastSequenceNumber() + 1;
        }
    }
}

int MissionController::insertSimpleMissionItem(QGeoCoordinate coordinate, int i)
{
    int sequenceNumber = _nextSequenceNumber();
    SimpleMissionItem * newItem = new SimpleMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    _initVisualItem(newItem);
    if (_visualItems->count() == 1) {
        newItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF);
    }
    newItem->setDefaultsForCommand();
    if ((MAV_CMD)newItem->command() == MAV_CMD_NAV_WAYPOINT) {
        double lastValue;
        MAV_FRAME lastFrame;

        if (_findLastAcceptanceRadius(&lastValue)) {
            newItem->missionItem().setParam2(lastValue);
        }
        if (_findLastAltitude(&lastValue, &lastFrame)) {
            newItem->missionItem().setFrame(lastFrame);
            newItem->missionItem().setParam7(lastValue);
        }
    }
    _visualItems->insert(i, newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

int MissionController::insertComplexMissionItem(QGeoCoordinate coordinate, int i)
{
    int sequenceNumber = _nextSequenceNumber();
    ComplexMissionItem* newItem = new ComplexMissionItem(_activeVehicle, this);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    _initVisualItem(newItem);

    _visualItems->insert(i, newItem);
    _complexItems->append(newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

void MissionController::removeMissionItem(int index)
{
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->removeAt(index));

    _deinitVisualItem(item);
    if (!item->isSimpleItem()) {
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(_complexItems->removeOne(item));
        if (complexItem) {
            complexItem->deleteLater();
        } else {
            qWarning() << "Complex item missing";
        }
    }
    item->deleteLater();

    _recalcAll();

    // Set the new current item

    if (index >= _visualItems->count()) {
        index--;
    }
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item =  qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        item->setIsCurrentItem(i == index);
    }
    _visualItems->setDirty(true);
}

void MissionController::removeAllMissionItems(void)
{
    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteListAndContents();
        _visualItems = new QmlObjectListModel(this);
        _addPlannedHomePosition(_visualItems, false /* addToCenter */);
        _initAllVisualItems();
        _visualItems->setDirty(true);
    }
}

bool MissionController::_loadJsonMissionFile(const QByteArray& bytes, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString)
{
    QJsonParseError jsonParseError;
    QJsonDocument   jsonDoc(QJsonDocument::fromJson(bytes, &jsonParseError));

    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = jsonParseError.errorString();
        return false;
    }

    QJsonObject json = jsonDoc.object();

    // Check for required keys
    QStringList requiredKeys;
    requiredKeys << _jsonVersionKey << _jsonPlannedHomePositionKey;
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        return false;
    }

    // Validate base key types
    QStringList             keyList;
    QList<QJsonValue::Type> typeList;
    keyList << jsonSimpleItemsKey << _jsonVersionKey << _jsonGroundStationKey << _jsonMavAutopilotKey << _jsonComplexItemsKey << _jsonPlannedHomePositionKey;
    typeList << QJsonValue::Array << QJsonValue::String << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Object;
    if (!JsonHelper::validateKeyTypes(json, keyList, typeList, errorString)) {
        return false;
    }

    // Version check
    if (json[_jsonVersionKey].toString() != "1.0") {
        errorString = QStringLiteral("QGroundControl does not support this file version");
        return false;
    }

    // Read complex items
    QJsonArray complexArray(json[_jsonComplexItemsKey].toArray());
    qCDebug(MissionControllerLog) << "Json load: complex item count" << complexArray.count();
    for (int i=0; i<complexArray.count(); i++) {
        const QJsonValue& itemValue = complexArray[i];

        if (!itemValue.isObject()) {
            errorString = QStringLiteral("Mission item is not an object");
            return false;
        }

        ComplexMissionItem* item = new ComplexMissionItem(_activeVehicle, this);
        if (item->load(itemValue.toObject(), errorString)) {
            qCDebug(MissionControllerLog) << "Json load: complex item start:stop" << item->sequenceNumber() << item->lastSequenceNumber();
            complexItems->append(item);
        } else {
            return false;
        }
    }

    // Read simple items, interspersing complex items into the full list

    int nextSimpleItemIndex= 0;
    int nextComplexItemIndex= 0;
    int nextSequenceNumber = 1; // Start with 1 since home is in 0
    QJsonArray itemArray(json[jsonSimpleItemsKey].toArray());

    qCDebug(MissionControllerLog) << "Json load: simple item loop start simpleItemCount:ComplexItemCount" << itemArray.count() << complexItems->count();
    do {
        qCDebug(MissionControllerLog) << "Json load: simple item loop nextSimpleItemIndex:nextComplexItemIndex:nextSequenceNumber" << nextSimpleItemIndex << nextComplexItemIndex << nextSequenceNumber;

        // If there is a complex item that should be next in sequence add it in
        if (nextComplexItemIndex < complexItems->count()) {
            ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(complexItems->get(nextComplexItemIndex));

            if (complexItem->sequenceNumber() == nextSequenceNumber) {
                qCDebug(MissionControllerLog) << "Json load: injecting complex item expectedSequence:actualSequence:" << nextSequenceNumber << complexItem->sequenceNumber();
                visualItems->append(complexItem);
                nextSequenceNumber = complexItem->lastSequenceNumber() + 1;
                nextComplexItemIndex++;
                continue;
            }
        }

        // Add the next available simple item
        if (nextSimpleItemIndex < itemArray.count()) {
            const QJsonValue& itemValue = itemArray[nextSimpleItemIndex++];

            if (!itemValue.isObject()) {
                errorString = QStringLiteral("Mission item is not an object");
                return false;
            }

            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);
            if (item->load(itemValue.toObject(), errorString)) {
                qCDebug(MissionControllerLog) << "Json load: adding simple item expectedSequence:actualSequence" << nextSequenceNumber << item->sequenceNumber();
                visualItems->append(item);
            } else {
                return false;
            }

            nextSequenceNumber++;
        }
    } while (nextSimpleItemIndex < itemArray.count() || nextComplexItemIndex < complexItems->count());

    if (json.contains(_jsonPlannedHomePositionKey)) {
        SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);

        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), errorString)) {
            visualItems->insert(0, item);
        } else {
            return false;
        }
    } else {
        _addPlannedHomePosition(visualItems, true /* addToCenter */);
    }

    return true;
}

bool MissionController::_loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString)
{
    bool addPlannedHomePosition = false;

    QString firstLine = stream.readLine();
    const QStringList& version = firstLine.split(" ");

    bool versionOk = false;
    if (version.size() == 3 && version[0] == "QGC" && version[1] == "WPL") {
        if (version[2] == "110") {
            // ArduPilot file, planned home position is already in position 0
            versionOk = true;
        } else if (version[2] == "120") {
            // Old QGC file, no planned home position
            versionOk = true;
            addPlannedHomePosition = true;
        }
    }

    if (versionOk) {
        while (!stream.atEnd()) {
            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);

            if (item->load(stream)) {
                visualItems->append(item);
            } else {
                errorString = QStringLiteral("The mission file is corrupted.");
                return false;
            }
        }
    } else {
        errorString = QStringLiteral("The mission file is not compatible with this version of QGroundControl.");
        return false;
    }

    if (addPlannedHomePosition || visualItems->count() == 0) {
        _addPlannedHomePosition(visualItems, true /* addToCenter */);

        // Update sequence numbers in DO_JUMP commands to take into account added home position in index 0
        for (int i=1; i<visualItems->count(); i++) {
            SimpleMissionItem* item = qobject_cast<SimpleMissionItem*>(visualItems->get(i));
            if (item && item->command() == MavlinkQmlSingleton::MAV_CMD_DO_JUMP) {
                item->missionItem().setParam1((int)item->missionItem().param1() + 1);
            }
        }
    }

    return true;
}

void MissionController::loadMissionFromFile(const QString& filename)
{
    QString errorString;

    if (filename.isEmpty()) {
        return;
    }

    QmlObjectListModel* newVisualItems = new QmlObjectListModel(this);
    QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString();
    } else {
        QByteArray  bytes = file.readAll();
        QTextStream stream(&bytes);

        QString firstLine = stream.readLine();
        if (firstLine.contains(QRegExp("QGC.*WPL"))) {
            stream.seek(0);
            _loadTextMissionFile(stream, newVisualItems, errorString);
        } else {
            _loadJsonMissionFile(bytes, newVisualItems, newComplexItems, errorString);
        }
    }

    if (!errorString.isEmpty()) {
        for (int i=0; i<newVisualItems->count(); i++) {
            newVisualItems->get(i)->deleteLater();
        }
        for (int i=0; i<newComplexItems->count(); i++) {
            newComplexItems->get(i)->deleteLater();
        }
        delete newVisualItems;
        delete newComplexItems;

        qgcApp()->showMessage(errorString);
        return;
    }

    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteListAndContents();
    }
    if (_complexItems) {
        _complexItems->deleteLater();
    }

    _visualItems = newVisualItems;
    _complexItems = newComplexItems;

    if (_visualItems->count() == 0) {
        _addPlannedHomePosition(_visualItems, true /* addToCenter */);
    }

    _initAllVisualItems();
}

void MissionController::loadMissionFromFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select Mission File to load", QString(), "Mission file (*.mission);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    loadMissionFromFile(filename);
#endif
}

void MissionController::saveMissionToFile(const QString& filename)
{
    qDebug() << filename;

    if (filename.isEmpty()) {
        return;
    }

    QString missionFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        missionFilename += QString(".%1").arg(QGCApplication::missionFileExtension);
    }

    QFile file(missionFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject missionFileObject;      // top level json object
        QJsonArray  simpleItemsObject;
        QJsonArray  complexItemsObject;

        missionFileObject[_jsonVersionKey] =        "1.0";
        missionFileObject[_jsonGroundStationKey] =  "QGroundControl";

        MAV_AUTOPILOT firmwareType = MAV_AUTOPILOT_GENERIC;
        if (_activeVehicle) {
            firmwareType = _activeVehicle->firmwareType();
        } else {
            // FIXME: Hack duplicated code from QGroundControlQmlGlobal. Had to do this for now since
            // QGroundControlQmlGlobal is not available from C++ side.

            QSettings settings;
            firmwareType = (MAV_AUTOPILOT)settings.value("OfflineEditingFirmwareType", MAV_AUTOPILOT_ARDUPILOTMEGA).toInt();
        }
        missionFileObject[_jsonMavAutopilotKey] = firmwareType;

        // Save planned home position
        QJsonObject homePositionObject;
        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (homeItem) {
            homeItem->missionItem().save(homePositionObject);
        } else {
            qgcApp()->showMessage(QStringLiteral("Internal error: VisualMissionItem at index 0 not SimpleMissionItem"));
            return;
        }
        missionFileObject[_jsonPlannedHomePositionKey] = homePositionObject;

        // Save the visual items
        for (int i=1; i<_visualItems->count(); i++) {
            QJsonObject itemObject;

            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            visualItem->save(itemObject);

            if (visualItem->isSimpleItem()) {
                simpleItemsObject.append(itemObject);
            } else {
                complexItemsObject.append(itemObject);
            }
        }

        missionFileObject[jsonSimpleItemsKey] = simpleItemsObject;
        missionFileObject[_jsonComplexItemsKey] = complexItemsObject;

        QJsonDocument saveDoc(missionFileObject);
        file.write(saveDoc.toJson());
    }

    _visualItems->setDirty(false);
}

void MissionController::saveMissionToFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(NULL, "Select file to save mission to", QString(), "Mission file (*.mission);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    saveMissionToFile(filename);
#endif
}

void MissionController::_calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  prevCoord =     prevItem->exitCoordinate();
    bool            distanceOk =    false;

    // Convert to fixed altitudes

    qCDebug(MissionControllerLog) << homeAlt
                                  << currentItem->coordinateHasRelativeAltitude() << currentItem->coordinate().altitude()
                                  << prevItem->exitCoordinateHasRelativeAltitude() << prevItem->exitCoordinate().altitude();

    distanceOk = true;
    if (currentItem->coordinateHasRelativeAltitude()) {
        currentCoord.setAltitude(homeAlt + currentCoord.altitude());
    }
    if (prevItem->exitCoordinateHasRelativeAltitude()) {
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
        *distance = 0.0;
    }
}

void MissionController::_recalcWaypointLines(void)
{
    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);

    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    } else {
        connect(homeItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_recalcAltitudeRangeBearing);
    }

    bool    showHomePosition =  homeItem->showHomePosition();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines";

    CoordVectHashTable old_table = _linesTable;
    _linesTable.clear();
    _waypointLines.clear();

    bool linkBackToHome = false;
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));


        // If we still haven't found the first coordinate item and we hit a a takeoff command link back to home
        if (firstCoordinateItem &&
                item->isSimpleItem() &&
                qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
        }

        if (item->specifiesCoordinate()) {
            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;
                VisualItemPair pair(lastCoordinateItem, item);
                if (lastCoordinateItem != homeItem || (showHomePosition && linkBackToHome)) {
                    if (old_table.contains(pair)) {
                        // Do nothing, this segment already exists and is wired up
                        _linesTable[pair] = old_table.take(pair);
                    } else {
                        // Create a new segment and wire update notifiers
                        auto linevect       = new CoordinateVector(lastCoordinateItem->isSimpleItem() ? lastCoordinateItem->coordinate() : lastCoordinateItem->exitCoordinate(), item->coordinate(), this);
                        auto originNotifier = lastCoordinateItem->isSimpleItem() ? &VisualMissionItem::coordinateChanged : &VisualMissionItem::exitCoordinateChanged,
                             endNotifier    = &VisualMissionItem::coordinateChanged;
                        // Use signals/slots to update the coordinate endpoints
                        connect(lastCoordinateItem, originNotifier, linevect, &CoordinateVector::setCoordinate1);
                        connect(item,               endNotifier,    linevect, &CoordinateVector::setCoordinate2);

                        // FIXME: We should ideally have signals for 2D position change, alt change, and 3D position change
                        // Not optimal, but still pretty fast, do a full update of range/bearing/altitudes
                        connect(item, &VisualMissionItem::coordinateChanged, this, &MissionController::_recalcAltitudeRangeBearing);
                        _linesTable[pair] = linevect;
                    }
                }
                lastCoordinateItem = item;
            }
        }
    }


    {
        // Create a temporary QObjectList and replace the model data
        QObjectList objs;
        objs.reserve(_linesTable.count());
        foreach(CoordinateVector *vect, _linesTable.values()) {
            objs.append(vect);
        }

        // We don't delete here because many links may still be valid
        _waypointLines.swapObjectList(objs);
    }


    // Anything left in the old table is an obsolete line object that can go
    qDeleteAll(old_table);

    _recalcAltitudeRangeBearing();

    emit waypointLinesChanged();
}

void MissionController::_recalcAltitudeRangeBearing()
{
    if (!_visualItems->count())
        return;

    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);

    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    }

    bool    showHomePosition =  homeItem->showHomePosition();

    qCDebug(MissionControllerLog) << "_recalcAltitudeRangeBearing";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(0.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    const double homePositionAltitude = homeItem->coordinate().altitude();
    minAltSeen = maxAltSeen = homeItem->coordinate().altitude();

    bool linkBackToHome = false;
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(0.0);

        // If we still haven't found the first coordinate item and we hit a a takeoff command link back to home
        if (firstCoordinateItem &&
                item->isSimpleItem() &&
                qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            linkBackToHome = true;
        }

        if (item->specifiesCoordinate()) {
            // Keep track of the min/max altitude for all waypoints so we can show altitudes as a percentage

            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);

            if (!item->exitCoordinateSameAsEntry()) {
                absoluteAltitude = item->exitCoordinate().altitude();
                if (item->exitCoordinateHasRelativeAltitude()) {
                    absoluteAltitude += homePositionAltitude;
                }
                minAltSeen = std::min(minAltSeen, absoluteAltitude);
                maxAltSeen = std::max(maxAltSeen, absoluteAltitude);
            }

            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;
                if (lastCoordinateItem != homeItem || (showHomePosition && linkBackToHome)) {
                    double azimuth, distance, altDifference;

                    // Subsequent coordinate items link to last coordinate item. If the last coordinate item
                    // is an invalid home position we skip the line
                    _calcPrevWaypointValues(homePositionAltitude, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);
                }
                lastCoordinateItem = item;
            }
        }
    }

    // Walk the list again calculating altitude percentages
    double altRange = maxAltSeen - minAltSeen;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (item->specifiesCoordinate()) {
            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            if (altRange == 0.0) {
                item->setAltPercent(0.0);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
            }
        }
    }
}

// This will update the sequence numbers to be sequential starting from 0
void MissionController::_recalcSequence(void)
{
    // Setup ascending sequence numbers for all visual items

    int sequenceNumber = 0;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        item->setSequenceNumber(sequenceNumber++);
        if (!item->isSimpleItem()) {
            ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

            if (complexItem) {
                 sequenceNumber = complexItem->lastSequenceNumber() + 1;
            } else {
                qWarning() << "isSimpleItem == false, yet not ComplexMissionItem";
            }
        }
    }
}

// This will update the child item hierarchy
void MissionController::_recalcChildItems(void)
{
    VisualMissionItem* currentParentItem = qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    currentParentItem->childItems()->clear();

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        // Set up non-coordinate item child hierarchy
        if (item->specifiesCoordinate()) {
            item->childItems()->clear();
            currentParentItem = item;
        } else if (item->isSimpleItem()) {
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
void MissionController::_initAllVisualItems(void)
{
    SimpleMissionItem* homeItem = NULL;

    // Setup home position at index 0

    homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
    if (!homeItem) {
        qWarning() << "homeItem not SimpleMissionItem";
        return;
    }

    homeItem->setHomePositionSpecialCase(true);
    homeItem->setShowHomePosition(_editMode);
    homeItem->missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->missionItem().setFrame(MAV_FRAME_GLOBAL);
    homeItem->setIsCurrentItem(true);

    if (!_editMode && _activeVehicle && _activeVehicle->homePositionAvailable()) {
        homeItem->setCoordinate(_activeVehicle->homePosition());
        homeItem->setShowHomePosition(true);
    }

    QmlObjectListModel* newComplexItems = new QmlObjectListModel(this);
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        _initVisualItem(item);

        // Set up complex item list
        if (!item->isSimpleItem()) {
            ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

            if (complexItem) {
                newComplexItems->append(item);
            } else {
                qWarning() << "isSimpleItem == false, but not ComplexMissionItem";
            }
        }
    }

    if (_complexItems) {
        _complexItems->deleteLater();
    }
    _complexItems = newComplexItems;

    _recalcAll();

    emit visualItemsChanged();
    emit complexVisualItemsChanged();

    _visualItems->setDirty(false);

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_dirtyChanged);
}

void MissionController::_deinitAllVisualItems(void)
{
    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_dirtyChanged);
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    _visualItems->setDirty(false);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLines);

    if (visualItem->isSimpleItem()) {
        // We need to track commandChanged on simple item since recalc has special handling for takeoff command
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        if (simpleItem) {
            connect(&simpleItem->missionItem()._commandFact, &Fact::valueChanged, this, &MissionController::_itemCommandChanged);
        } else {
            qWarning() << "isSimpleItem == true, yet not SimpleMissionItem";
        }
    } else {
        // We need to track changes of lastSequenceNumber so we can recalc sequence numbers for subsequence items
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(visualItem);
        connect(complexItem, &ComplexMissionItem::lastSequenceNumberChanged, this, &MissionController::_recalcSequence);
    }
}

void MissionController::_deinitVisualItem(VisualMissionItem* visualItem)
{
    // Disconnect all signals
    disconnect(visualItem, 0, 0, 0);
}

void MissionController::_itemCommandChanged(void)
{
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

    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    removeAllMissionItems();

    _activeVehicle = activeVehicle;

    if (_activeVehicle) {
        MissionManager* missionManager = activeVehicle->missionManager();

        connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailableFromVehicle);
        connect(missionManager, &MissionManager::inProgressChanged,         this, &MissionController::_inProgressChanged);
        connect(missionManager, &MissionManager::currentItemChanged,        this, &MissionController::_currentMissionItemChanged);
        connect(_activeVehicle, &Vehicle::homePositionAvailableChanged,     this, &MissionController::_activeVehicleHomePositionAvailableChanged);
        connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);

        if (_activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
            // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
            // We don't request mission items for new vehicles since that will happen autamatically.
            getMissionItems();
        }

        _activeVehicleHomePositionChanged(_activeVehicle->homePosition());
        _activeVehicleHomePositionAvailableChanged(_activeVehicle->homePositionAvailable());
    }

    // Whenever vehicle changes we need to update syncInProgress
    emit syncInProgressChanged(syncInProgress());
}

void MissionController::_activeVehicleHomePositionAvailableChanged(bool homePositionAvailable)
{
    if (!_editMode && _visualItems) {
        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));

        if (homeItem) {
            homeItem->setShowHomePosition(homePositionAvailable);
            _recalcWaypointLines();
        } else {
            qWarning() << "Unabled to cast home item to SimpleMissionItem";
        }
    }
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    if (!_editMode && _visualItems) {
        qobject_cast<VisualMissionItem*>(_visualItems->get(0))->setCoordinate(homePosition);
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
    _queuedSend = false;
    if (_visualItems) {
        sendMissionItems();
        _visualItems->setDirty(false);
    }
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
    if (!inProgress && _queuedSend) {
        _autoSyncSend();
    }
}

bool MissionController::_findLastAltitude(double* lastAltitude, MAV_FRAME* frame)
{
    bool found = false;    
    double foundAltitude;
    MAV_FRAME foundFrame;

    // Don't use home position
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {

            if (visualItem->isSimpleItem()) {
                SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
                if ((MAV_CMD)simpleItem->command() != MAV_CMD_NAV_TAKEOFF) {
                    foundAltitude = simpleItem->exitCoordinate().altitude();
                    foundFrame = simpleItem->missionItem().frame();
                    found = true;
                }
            }
        }
    }

    if (found) {
        *lastAltitude = foundAltitude;
        *frame = foundFrame;
    }

    return found;
}

bool MissionController::_findLastAcceptanceRadius(double* lastAcceptanceRadius)
{
    bool found = false;
    double foundAcceptanceRadius;

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->isSimpleItem()) {
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);

            if (simpleItem) {
                if ((MAV_CMD)simpleItem->command() == MAV_CMD_NAV_WAYPOINT) {
                    foundAcceptanceRadius = simpleItem->missionItem().param2();
                    found = true;
                }
            } else {
                qWarning() << "isSimpleItem == true, yet not SimpleMissionItem";
            }
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
void MissionController::_addPlannedHomePosition(QmlObjectListModel* visualItems, bool addToCenter)
{
    SimpleMissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    visualItems->insert(0, homeItem);

    if (visualItems->count() > 1  && addToCenter) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(1));

        double north = _normalizeLat(item->coordinate().latitude());
        double south = north;
        double east = _normalizeLon(item->coordinate().longitude());
        double west = east;

        for (int i=2; i<visualItems->count(); i++) {
            item = qobject_cast<VisualMissionItem*>(visualItems->get(i));

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

        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            item->setIsCurrentItem(item->sequenceNumber() == sequenceNumber);
        }
    }
}

bool MissionController::syncInProgress(void)
{
    if (_activeVehicle) {
        return _activeVehicle->missionManager()->inProgress();
    } else {
        return false;
    }
}
