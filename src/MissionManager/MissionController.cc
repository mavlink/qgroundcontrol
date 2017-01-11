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
#include "SurveyMissionItem.h"
#include "JsonHelper.h"
#include "ParameterManager.h"
#include "QGroundControlQmlGlobal.h"

#ifndef __mobile__
#include "MainWindow.h"
#include "QGCFileDialog.h"
#endif

QGC_LOGGING_CATEGORY(MissionControllerLog, "MissionControllerLog")


const char* MissionController::_settingsGroup =                 "MissionController";
const char* MissionController::_jsonFileTypeValue =             "Mission";
const char* MissionController::_jsonItemsKey =                  "items";
const char* MissionController::_jsonPlannedHomePositionKey =    "plannedHomePosition";
const char* MissionController::_jsonFirmwareTypeKey =           "firmwareType";
const char* MissionController::_jsonVehicleTypeKey =            "vehicleType";
const char* MissionController::_jsonCruiseSpeedKey =            "cruiseSpeed";
const char* MissionController::_jsonHoverSpeedKey =             "hoverSpeed";
const char* MissionController::_jsonParamsKey =                 "params";

// Deprecated V1 format keys
const char* MissionController::_jsonComplexItemsKey =           "complexItems";
const char* MissionController::_jsonMavAutopilotKey =           "MAV_AUTOPILOT";

const int   MissionController::_missionFileVersion =            2;

MissionController::MissionController(QObject *parent)
    : PlanElementController(parent)
    , _visualItems(NULL)
    , _complexItems(NULL)
    , _firstItemsFromVehicle(false)
    , _missionItemsRequested(false)
    , _queuedSend(false)
    , _missionDistance(0.0)
    , _missionTime(0.0)
    , _missionHoverDistance(0.0)
    , _missionHoverTime(0.0)
    , _missionCruiseDistance(0.0)
    , _missionCruiseTime(0.0)
    , _missionMaxTelemetry(0.0)
{

}

MissionController::~MissionController()
{

}

void MissionController::start(bool editMode)
{
    qCDebug(MissionControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);
    _init();
}

void MissionController::startStaticActiveVehicle(Vehicle *vehicle)
{
    qCDebug(MissionControllerLog) << "startStaticActiveVehicle";

    PlanElementController::startStaticActiveVehicle(vehicle);
    _init();
}

void MissionController::_init(void)
{
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

void MissionController::loadFromVehicle(void)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (activeVehicle) {
        _missionItemsRequested = true;
        activeVehicle->missionManager()->requestMissionItems();
    }
}

void MissionController::sendToVehicle(void)
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
    SurveyMissionItem* newItem = new SurveyMissionItem(_activeVehicle, this);
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
        if (!complexItem) {
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

void MissionController::removeAll(void)
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

    // V1 file format has no file type key and version key is string. Convert to new format.
    if (!json.contains(JsonHelper::jsonFileTypeKey)) {
        json[JsonHelper::jsonFileTypeKey] = _jsonFileTypeValue;
    }

    int fileVersion;
    if (!JsonHelper::validateQGCJsonFile(json,
                                         _jsonFileTypeValue,    // expected file type
                                         1,                     // minimum supported version
                                         2,                     // maximum supported version
                                         fileVersion,
                                         errorString)) {
        return false;
    }

    if (fileVersion == 1) {
        return _loadJsonMissionFileV1(json, visualItems, complexItems, errorString);
    } else {
        return _loadJsonMissionFileV2(json, visualItems, complexItems, errorString);
    }
}

bool MissionController::_loadJsonMissionFileV1(const QJsonObject& json, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString)
{
    // Validate root object keys
    QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { _jsonPlannedHomePositionKey,      QJsonValue::Object, true },
        { _jsonItemsKey,                    QJsonValue::Array,  true },
        { _jsonMavAutopilotKey,             QJsonValue::Double, true },
        { _jsonComplexItemsKey,             QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(json, rootKeyInfoList, errorString)) {
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

        SurveyMissionItem* item = new SurveyMissionItem(_activeVehicle, this);
        const QJsonObject itemObject = itemValue.toObject();
        if (item->load(itemObject, itemObject["id"].toInt(), errorString)) {
            complexItems->append(item);
        } else {
            return false;
        }
    }

    // Read simple items, interspersing complex items into the full list

    int nextSimpleItemIndex= 0;
    int nextComplexItemIndex= 0;
    int nextSequenceNumber = 1; // Start with 1 since home is in 0
    QJsonArray itemArray(json[_jsonItemsKey].toArray());

    qCDebug(MissionControllerLog) << "Json load: simple item loop start simpleItemCount:ComplexItemCount" << itemArray.count() << complexItems->count();
    do {
        qCDebug(MissionControllerLog) << "Json load: simple item loop nextSimpleItemIndex:nextComplexItemIndex:nextSequenceNumber" << nextSimpleItemIndex << nextComplexItemIndex << nextSequenceNumber;

        // If there is a complex item that should be next in sequence add it in
        if (nextComplexItemIndex < complexItems->count()) {
            SurveyMissionItem* complexItem = qobject_cast<SurveyMissionItem*>(complexItems->get(nextComplexItemIndex));

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

            const QJsonObject itemObject = itemValue.toObject();
            SimpleMissionItem* item = new SimpleMissionItem(_activeVehicle, this);
            if (item->load(itemObject, itemObject["id"].toInt(), errorString)) {
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

        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), 0, errorString)) {
            visualItems->insert(0, item);
        } else {
            return false;
        }
    } else {
        _addPlannedHomePosition(visualItems, true /* addToCenter */);
    }

    return true;
}

bool MissionController::_loadJsonMissionFileV2(const QJsonObject& json, QmlObjectListModel* visualItems, QmlObjectListModel* complexItems, QString& errorString)
{
    // Validate root object keys
    QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { _jsonPlannedHomePositionKey,      QJsonValue::Array,  true },
        { _jsonItemsKey,                    QJsonValue::Array,  true },
        { _jsonFirmwareTypeKey,             QJsonValue::Double, true },
        { _jsonVehicleTypeKey,              QJsonValue::Double, false },
        { _jsonCruiseSpeedKey,              QJsonValue::Double, false },
        { _jsonHoverSpeedKey,               QJsonValue::Double, false },
    };
    if (!JsonHelper::validateKeys(json, rootKeyInfoList, errorString)) {
        return false;
    }

    qCDebug(MissionControllerLog) << "MissionController::_loadJsonMissionFileV2 itemCount:" << json[_jsonItemsKey].toArray().count();

    // Mission Settings
    QGeoCoordinate homeCoordinate;
    if (!JsonHelper::loadGeoCoordinate(json[_jsonPlannedHomePositionKey], true /* altitudeRequired */, homeCoordinate, errorString)) {
        return false;
    }
    if (json.contains(_jsonVehicleTypeKey) && _activeVehicle->isOfflineEditingVehicle()) {
        QGroundControlQmlGlobal::offlineEditingVehicleType()->setRawValue(json[_jsonVehicleTypeKey].toDouble());
    }
    if (json.contains(_jsonCruiseSpeedKey)) {
        QGroundControlQmlGlobal::offlineEditingCruiseSpeed()->setRawValue(json[_jsonCruiseSpeedKey].toDouble());
    }
    if (json.contains(_jsonHoverSpeedKey)) {
        QGroundControlQmlGlobal::offlineEditingHoverSpeed()->setRawValue(json[_jsonHoverSpeedKey].toDouble());
    }

    SimpleMissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    homeItem->setCoordinate(homeCoordinate);
    visualItems->insert(0, homeItem);
    qCDebug(MissionControllerLog) << "plannedHomePosition" << homeCoordinate;

    // Read mission items

    int nextSequenceNumber = 1; // Start with 1 since home is in 0
    const QJsonArray rgMissionItems(json[_jsonItemsKey].toArray());
    for (int i=0; i<rgMissionItems.count(); i++) {
        // Convert to QJsonObject
        const QJsonValue& itemValue = rgMissionItems[i];
        if (!itemValue.isObject()) {
            errorString = tr("Mission item %1 is not an object").arg(i);
            return false;
        }
        const QJsonObject itemObject = itemValue.toObject();

        // Load item based on type

        QList<JsonHelper::KeyValidateInfo> itemKeyInfoList = {
            { VisualMissionItem::jsonTypeKey,  QJsonValue::String, true },
        };
        if (!JsonHelper::validateKeys(itemObject, itemKeyInfoList, errorString)) {
            return false;
        }
        QString itemType = itemObject[VisualMissionItem::jsonTypeKey].toString();

        if (itemType == VisualMissionItem::jsonTypeSimpleItemValue) {
            qCDebug(MissionControllerLog) << "Loading MISSION_ITEM: nextSequenceNumber" << nextSequenceNumber;
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_activeVehicle, this);
            if (simpleItem->load(itemObject, nextSequenceNumber++, errorString)) {
                visualItems->append(simpleItem);
            } else {
                return false;
            }
        } else if (itemType == VisualMissionItem::jsonTypeComplexItemValue) {
            QList<JsonHelper::KeyValidateInfo> complexItemKeyInfoList = {
                { ComplexMissionItem::jsonComplexItemTypeKey,  QJsonValue::String, true },
            };
            if (!JsonHelper::validateKeys(itemObject, complexItemKeyInfoList, errorString)) {
                return false;
            }
            QString complexItemType = itemObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();

            if (complexItemType == SurveyMissionItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Survey: nextSequenceNumber" << nextSequenceNumber;
                SurveyMissionItem* surveyItem = new SurveyMissionItem(_activeVehicle, this);
                if (!surveyItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = surveyItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Survey load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(surveyItem);
                complexItems->append(surveyItem);
            } else {
                errorString = tr("Unsupported complex item type: %1").arg(complexItemType);
            }
        } else {
            errorString = tr("Unknown item type: %1").arg(itemType);
            return false;
        }
    }

    // Fix up the DO_JUMP commands jump sequence number by finding the item with the matching doJumpId
    for (int i=0; i<visualItems->count(); i++) {
        if (visualItems->value<VisualMissionItem*>(i)->isSimpleItem()) {
            SimpleMissionItem* doJumpItem = visualItems->value<SimpleMissionItem*>(i);
            if ((MAV_CMD)doJumpItem->command() == MAV_CMD_DO_JUMP) {
                bool found = false;
                int findDoJumpId = doJumpItem->missionItem().param1();
                for (int j=0; j<visualItems->count(); j++) {
                    if (visualItems->value<VisualMissionItem*>(j)->isSimpleItem()) {
                        SimpleMissionItem* targetItem = visualItems->value<SimpleMissionItem*>(j);
                        if (targetItem->missionItem().doJumpId() == findDoJumpId) {
                            doJumpItem->missionItem().setParam1(targetItem->sequenceNumber());
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    errorString = tr("Could not find doJumpId: %1").arg(findDoJumpId);
                    return false;
                }
            }
        }
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

void MissionController::loadFromFile(const QString& filename)
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

void MissionController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getOpenFileName(MainWindow::instance(), "Select Mission File to load", QString(), "Mission file (*.mission);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    loadFromFile(filename);
#endif
}

void MissionController::saveToFile(const QString& filename)
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

        missionFileObject[JsonHelper::jsonVersionKey] =         _missionFileVersion;
        missionFileObject[JsonHelper::jsonGroundStationKey] =   JsonHelper::jsonGroundStationValue;

        // Mission settings

        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (!homeItem) {
            qgcApp()->showMessage(QStringLiteral("Internal error: VisualMissionItem at index 0 not SimpleMissionItem"));
            return;
        }
        QJsonValue coordinateValue;
        JsonHelper::saveGeoCoordinate(homeItem->coordinate(), true /* writeAltitude */, coordinateValue);
        missionFileObject[_jsonPlannedHomePositionKey] = coordinateValue;
        missionFileObject[_jsonFirmwareTypeKey] = _activeVehicle->firmwareType();
        missionFileObject[_jsonVehicleTypeKey] = _activeVehicle->vehicleType();
        missionFileObject[_jsonCruiseSpeedKey] = _activeVehicle->cruiseSpeed();
        missionFileObject[_jsonHoverSpeedKey] = _activeVehicle->hoverSpeed();

        // Save the visual items
        QJsonArray  rgMissionItems;
        for (int i=1; i<_visualItems->count(); i++) {
            QJsonObject itemObject;

            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            visualItem->save(itemObject);
            rgMissionItems.append(itemObject);
        }
        missionFileObject[_jsonItemsKey] = rgMissionItems;

        QJsonDocument saveDoc(missionFileObject);
        file.write(saveDoc.toJson());
    }

    _visualItems->setDirty(false);
}

void MissionController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(MainWindow::instance(), "Select file to save mission to", QString(), "Mission file (*.mission);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    saveToFile(filename);
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

double MissionController::_calcDistanceToHome(VisualMissionItem* currentItem, VisualMissionItem* homeItem)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  homeCoord =     homeItem->exitCoordinate();
    bool            distanceOk =    false;

    distanceOk = true;

    qCDebug(MissionControllerLog) << "distanceOk" << distanceOk;

    return distanceOk ? homeCoord.distanceTo(currentCoord) : 0.0;
}

void MissionController::_recalcWaypointLines(void)
{
    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    SimpleMissionItem*  homeItem = qobject_cast<SimpleMissionItem*>(lastCoordinateItem);

    if (!homeItem) {
        qWarning() << "Home item is not SimpleMissionItem";
    }

    bool    showHomePosition =  homeItem->showHomePosition();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines";

    CoordVectHashTable old_table = _linesTable;
    _linesTable.clear();
    _waypointLines.clear();

    bool linkBackToHome = false;
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));


        // If we still haven't found the first coordinate item and we hit a takeoff command, link back to home
        if (firstCoordinateItem &&
                item->isSimpleItem() &&
                (qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF ||
                 qobject_cast<SimpleMissionItem*>(item)->command() == MavlinkQmlSingleton::MAV_CMD_NAV_VTOL_TAKEOFF)) {
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

    bool showHomePosition =  homeItem->showHomePosition();

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

    double missionDistance = 0.0;
    double missionMaxTelemetry = 0.0;
    double missionTime = 0.0;
    double vtolHoverTime = 0.0;
    double vtolCruiseTime = 0.0;
    double vtolHoverDistance = 0.0;
    double vtolCruiseDistance = 0.0;
    double currentCruiseSpeed = _activeVehicle->cruiseSpeed();
    double currentHoverSpeed = _activeVehicle->hoverSpeed();

    bool vtolVehicle = _activeVehicle->vtol();
    bool vtolInHover = true;

    bool linkBackToHome = false;

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(item);
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(0.0);

        if (simpleItem && simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_DO_CHANGE_SPEED) {
            // Adjust cruise speed for time calculations
            double newSpeed = simpleItem->missionItem().param2();
            if (newSpeed > 0) {
                if (_activeVehicle->multiRotor()) {
                    currentHoverSpeed = newSpeed;
                } else {
                    currentCruiseSpeed = newSpeed;
                }
            }
        }

        // Link back to home if first item is takeoff and we have home position
        if (firstCoordinateItem && simpleItem && simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            if (showHomePosition) {
                linkBackToHome = true;
            }
        }

        // Update VTOL state
        if (simpleItem && vtolVehicle) {
            switch (simpleItem->command()) {
            case MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF:
                vtolInHover = false;
                break;
            case MavlinkQmlSingleton::MAV_CMD_NAV_LAND:
                vtolInHover = false;
                break;
            case MavlinkQmlSingleton::MAV_CMD_DO_VTOL_TRANSITION:
            {
                int transitionState = simpleItem->missionItem().param1();
                if (transitionState == MAV_VTOL_STATE_TRANSITION_TO_MC) {
                    vtolInHover = true;
                } else if (transitionState == MAV_VTOL_STATE_TRANSITION_TO_FW) {
                    vtolInHover = false;
                }
            }
                break;
            default:
                break;
            }
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
                if (lastCoordinateItem != homeItem || linkBackToHome) {
                    // This is a subsequent waypoint or we are forcing the first waypoint back to home
                    double azimuth, distance, altDifference;

                    _calcPrevWaypointValues(homePositionAltitude, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);

                    missionDistance += distance;
                    missionMaxTelemetry = qMax(missionMaxTelemetry, _calcDistanceToHome(item, homeItem));

                    // Calculate mission time
                    if (vtolVehicle) {
                        if (vtolInHover) {
                            double hoverTime = distance / _activeVehicle->hoverSpeed();
                            missionTime += hoverTime;
                            vtolHoverTime += hoverTime;
                            vtolHoverDistance += distance;
                        } else {
                            double cruiseTime = distance / currentCruiseSpeed;
                            missionTime += cruiseTime;
                            vtolCruiseTime += cruiseTime;
                            vtolCruiseDistance += distance;
                        }
                    } else {
                        missionTime += distance / (_activeVehicle->multiRotor() ? currentHoverSpeed : currentCruiseSpeed);
                    }
                }
                if (complexItem) {
                    // Add in distance/time inside survey as well
                    // This code assumes all surveys are done cruise not hover
                    double complexDistance = complexItem->complexDistance();
                    double cruiseSpeed = _activeVehicle->multiRotor() ? currentHoverSpeed : currentCruiseSpeed;
                    missionDistance += complexDistance;
                    missionTime += complexDistance / cruiseSpeed;
                    missionMaxTelemetry = qMax(missionMaxTelemetry, complexItem->greatestDistanceTo(homeItem->exitCoordinate()));

                    // Let the complex item know the current cruise speed
                    complexItem->setCruiseSpeed(cruiseSpeed);
                }
            }

            lastCoordinateItem = item;
        }
    }

    _setMissionMaxTelemetry(missionMaxTelemetry);
    _setMissionDistance(missionDistance);
    _setMissionTime(missionTime);
    _setMissionHoverDistance(vtolHoverDistance);
    _setMissionHoverTime(vtolHoverTime);
    _setMissionCruiseDistance(vtolCruiseDistance);
    _setMissionCruiseTime(vtolCruiseTime);

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

    emit plannedHomePositionChanged(plannedHomePosition());

    connect(homeItem, &VisualMissionItem::coordinateChanged, this, &MissionController::_homeCoordinateChanged);

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

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);

    _visualItems->setDirty(false);
}

void MissionController::_deinitAllVisualItems(void)
{
    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    disconnect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    _visualItems->setDirty(false);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::flightSpeedChanged,                         this, &MissionController::_recalcAltitudeRangeBearing);

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
        connect(complexItem, &ComplexMissionItem::complexDistanceChanged, this, &MissionController::_recalcAltitudeRangeBearing);
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

void MissionController::_activeVehicleBeingRemoved(void)
{
    qCDebug(MissionControllerLog) << "MissionController::_activeVehicleBeingRemoved";

    MissionManager* missionManager = _activeVehicle->missionManager();

    disconnect(missionManager, &MissionManager::newMissionItemsAvailable,   this, &MissionController::_newMissionItemsAvailableFromVehicle);
    disconnect(missionManager, &MissionManager::inProgressChanged,          this, &MissionController::_inProgressChanged);
    disconnect(missionManager, &MissionManager::currentItemChanged,         this, &MissionController::_currentMissionItemChanged);
    disconnect(_activeVehicle, &Vehicle::homePositionAvailableChanged,      this, &MissionController::_activeVehicleHomePositionAvailableChanged);
    disconnect(_activeVehicle, &Vehicle::homePositionChanged,               this, &MissionController::_activeVehicleHomePositionChanged);

    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    removeAll();
}

void MissionController::_activeVehicleSet(void)
{
    // We always remove all items on vehicle change. This leaves a user model hole:
    //      If the user has unsaved changes in the Plan view they will lose them
    removeAll();

    MissionManager* missionManager = _activeVehicle->missionManager();

    connect(missionManager, &MissionManager::newMissionItemsAvailable,  this, &MissionController::_newMissionItemsAvailableFromVehicle);
    connect(missionManager, &MissionManager::inProgressChanged,         this, &MissionController::_inProgressChanged);
    connect(missionManager, &MissionManager::currentItemChanged,        this, &MissionController::_currentMissionItemChanged);
    connect(_activeVehicle, &Vehicle::homePositionAvailableChanged,     this, &MissionController::_activeVehicleHomePositionAvailableChanged);
    connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);
    connect(_activeVehicle, &Vehicle::cruiseSpeedChanged,               this, &MissionController::_recalcAltitudeRangeBearing);
    connect(_activeVehicle, &Vehicle::hoverSpeedChanged,                this, &MissionController::_recalcAltitudeRangeBearing);

    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
        // We don't request mission items for new vehicles since that will happen autamatically.
        loadFromVehicle();
    }

    _activeVehicleHomePositionChanged(_activeVehicle->homePosition());
    _activeVehicleHomePositionAvailableChanged(_activeVehicle->homePositionAvailable());
}

void MissionController::_activeVehicleHomePositionAvailableChanged(bool homePositionAvailable)
{
    if (!_editMode && _visualItems) {
        SimpleMissionItem* homeItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));

        if (homeItem) {
            homeItem->setShowHomePosition(homePositionAvailable);
            emit plannedHomePositionChanged(plannedHomePosition());
            _recalcWaypointLines();
        } else {
            qWarning() << "Unabled to cast home item to SimpleMissionItem";
        }
    }
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    if (!_editMode && _visualItems) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(0));
        if (item) {
            if (item->coordinate() != homePosition) {
                item->setCoordinate(homePosition);
                qCDebug(MissionControllerLog) << "Home position update" << homePosition;
                emit plannedHomePositionChanged(plannedHomePosition());
                _recalcWaypointLines();
            }
        } else {
            qWarning() << "Unabled to cast home item to VisualMissionItem";
        }
    }
}

void MissionController::_setMissionMaxTelemetry(double missionMaxTelemetry)
{
    if (!qFuzzyCompare(_missionMaxTelemetry, missionMaxTelemetry)) {
        _missionMaxTelemetry = missionMaxTelemetry;
        emit missionMaxTelemetryChanged(_missionMaxTelemetry);
    }
}

void MissionController::_setMissionDistance(double missionDistance)
{
    if (!qFuzzyCompare(_missionDistance, missionDistance)) {
        _missionDistance = missionDistance;
        emit missionDistanceChanged(_missionDistance);
    }
}

void MissionController::_setMissionTime(double missionTime)
{
    if (!qFuzzyCompare(_missionTime, missionTime)) {
        _missionTime = missionTime;
        emit missionTimeChanged();
    }
}

void MissionController::_setMissionHoverTime(double missionHoverTime)
{
    if (!qFuzzyCompare(_missionHoverTime, missionHoverTime)) {
        _missionHoverTime = missionHoverTime;
        emit missionHoverTimeChanged();
    }
}

void MissionController::_setMissionHoverDistance(double missionHoverDistance)
{
    if (!qFuzzyCompare(_missionHoverDistance, missionHoverDistance)) {
        _missionHoverDistance = missionHoverDistance;
        emit missionHoverDistanceChanged(_missionHoverDistance);
    }
}

void MissionController::_setMissionCruiseTime(double missionCruiseTime)
{
    if (!qFuzzyCompare(_missionCruiseTime, missionCruiseTime)) {
        _missionCruiseTime = missionCruiseTime;
        emit missionCruiseTimeChanged();
    }
}

void MissionController::_setMissionCruiseDistance(double missionCruiseDistance)
{
    if (!qFuzzyCompare(_missionCruiseDistance, missionCruiseDistance)) {
        _missionCruiseDistance = missionCruiseDistance;
        emit missionCruiseDistanceChanged(_missionCruiseDistance);
    }
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
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
    bool homePositionSet = false;

    SimpleMissionItem* homeItem = new SimpleMissionItem(_activeVehicle, this);
    visualItems->insert(0, homeItem);

    if (visualItems->count() > 1  && addToCenter) {
        double north, south, east, west;
        bool firstCoordSet = false;

        for (int i=1; i<visualItems->count(); i++) {
            VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(i));

            if (item->specifiesCoordinate()) {
                if (firstCoordSet) {
                    double lat = _normalizeLat(item->coordinate().latitude());
                    double lon = _normalizeLon(item->coordinate().longitude());
                    north = fmax(north, lat);
                    south = fmin(south, lat);
                    east = fmax(east, lon);
                    west = fmin(west, lon);
                } else {
                    firstCoordSet = true;
                    north = _normalizeLat(item->coordinate().latitude());
                    south = north;
                    east = _normalizeLon(item->coordinate().longitude());
                    west = east;
                }
            }
        }

        if (firstCoordSet) {
            homePositionSet = true;
            homeItem->setCoordinate(QGeoCoordinate((south + ((north - south) / 2)) - 90.0, (west + ((east - west) / 2)) - 180.0, 0.0));
        }
    }

    if (!homePositionSet) {
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

bool MissionController::syncInProgress(void) const
{
    return _activeVehicle ? _activeVehicle->missionManager()->inProgress() : false;
}

bool MissionController::dirty(void) const
{
    return _visualItems ? _visualItems->dirty() : false;
}


void MissionController::setDirty(bool dirty)
{
    if (_visualItems) {
        _visualItems->setDirty(dirty);
    }
}

QGeoCoordinate MissionController::plannedHomePosition(void)
{
    if (_visualItems && _visualItems->count() > 0) {
        SimpleMissionItem* item = qobject_cast<SimpleMissionItem*>(_visualItems->get(0));
        if (item && item->showHomePosition()) {
            return item->coordinate();
        }
    }

    return QGeoCoordinate();
}

void MissionController::_homeCoordinateChanged(void)
{
    emit plannedHomePositionChanged(plannedHomePosition());
    _recalcAltitudeRangeBearing();
}

QString MissionController::fileExtension(void) const
{
    return QGCApplication::missionFileExtension;
}

double  MissionController::cruiseSpeed(void) const
{
    if (_activeVehicle) {
        return _activeVehicle->cruiseSpeed();
    } else {
        return 0.0f;
    }
}

double  MissionController::hoverSpeed(void) const
{
    if (_activeVehicle) {
        return _activeVehicle->hoverSpeed();
    } else {
        return 0.0f;
    }
}
