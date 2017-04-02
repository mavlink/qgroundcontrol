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
#include "FixedWingLandingComplexItem.h"
#include "JsonHelper.h"
#include "ParameterManager.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionSettingsItem.h"

#ifndef __mobile__
#include "MainWindow.h"
#include "QGCQFileDialog.h"
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
    , _settingsItem(NULL)
    , _firstItemsFromVehicle(false)
    , _missionItemsRequested(false)
    , _surveyMissionItemName(tr("Survey"))
    , _fwLandingMissionItemName(tr("Fixed Wing Landing"))
    , _appSettings(qgcApp()->toolbox()->settingsManager()->appSettings())
{
    _resetMissionFlightStatus();
}

MissionController::~MissionController()
{

}

void MissionController::_resetMissionFlightStatus(void)
{
    _missionFlightStatus.totalDistance =        0.0;
    _missionFlightStatus.maxTelemetryDistance = 0.0;
    _missionFlightStatus.totalTime =            0.0;
    _missionFlightStatus.hoverTime =            0.0;
    _missionFlightStatus.cruiseTime =           0.0;
    _missionFlightStatus.hoverDistance =        0.0;
    _missionFlightStatus.cruiseDistance =       0.0;
    _missionFlightStatus.cruiseSpeed =          _activeVehicle ? _activeVehicle->defaultCruiseSpeed() : std::numeric_limits<double>::quiet_NaN();
    _missionFlightStatus.hoverSpeed =           _activeVehicle ? _activeVehicle->defaultHoverSpeed() : std::numeric_limits<double>::quiet_NaN();
    _missionFlightStatus.vehicleSpeed =         _activeVehicle ? (_activeVehicle->multiRotor() || _activeVehicle->vtol() ? _missionFlightStatus.hoverSpeed : _missionFlightStatus.cruiseSpeed) : std::numeric_limits<double>::quiet_NaN();
    _missionFlightStatus.gimbalYaw =            std::numeric_limits<double>::quiet_NaN();

    // Battery information

    _missionFlightStatus.mAhBattery =           0;
    _missionFlightStatus.hoverAmps =            0;
    _missionFlightStatus.cruiseAmps =           0;
    _missionFlightStatus.ampMinutesAvailable =  0;
    _missionFlightStatus.hoverAmpsTotal =       0;
    _missionFlightStatus.cruiseAmpsTotal =      0;
    _missionFlightStatus.batteryChangePoint =   -1;
    _missionFlightStatus.batteriesRequired =    -1;

    if (_activeVehicle) {
        _activeVehicle->firmwarePlugin()->batteryConsumptionData(_activeVehicle, _missionFlightStatus.mAhBattery, _missionFlightStatus.hoverAmps, _missionFlightStatus.cruiseAmps);
        if (_missionFlightStatus.mAhBattery != 0) {
            double batteryPercentRemainingAnnounce = qgcApp()->toolbox()->settingsManager()->appSettings()->batteryPercentRemainingAnnounce()->rawValue().toDouble();
            _missionFlightStatus.ampMinutesAvailable = (double)_missionFlightStatus.mAhBattery / 1000.0 * 60.0 * ((100.0 - batteryPercentRemainingAnnounce) / 100.0);
        }
    }
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
    _addMissionSettings(_activeVehicle, _visualItems, false /* addToCenter */);
    _initAllVisualItems();
}

// Called when new mission items have completed downloading from Vehicle
void MissionController::_newMissionItemsAvailableFromVehicle(bool removeAllRequested)
{
    qCDebug(MissionControllerLog) << "_newMissionItemsAvailableFromVehicle";

    if (!_editMode || removeAllRequested || _missionItemsRequested || _visualItems->count() == 1) {
        // Fly Mode (accept if):
        //      - Always accepts new items from the vehicle so Fly view is kept up to date
        // Edit Mode (accept if):
        //      - Either a load from vehicle was manually requested or
        //      - The initial automatic load from a vehicle completed and the current editor is empty
        //      - Remove all way requested from Fly view (clear mission on flight end)

        QmlObjectListModel* newControllerMissionItems = new QmlObjectListModel(this);
        const QList<MissionItem*>& newMissionItems = _activeVehicle->missionManager()->missionItems();
        qCDebug(MissionControllerLog) << "loading from vehicle: count"<< newMissionItems.count();

        int i = 0;
        if (_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() && newMissionItems.count() != 0) {
            // First item is fake home position
            _addMissionSettings(_activeVehicle, newControllerMissionItems, false /* addToCenter */);
            MissionSettingsItem* settingsItem = newControllerMissionItems->value<MissionSettingsItem*>(0);
            if (!settingsItem) {
                qWarning() << "First item is not settings item";
                return;
            }
            settingsItem->setCoordinate(newMissionItems[0]->coordinate());
            i = 1;
        }

        for (; i<newMissionItems.count(); i++) {
            const MissionItem* missionItem = newMissionItems[i];
            newControllerMissionItems->append(new SimpleMissionItem(_activeVehicle, *missionItem, this));
        }

        _deinitAllVisualItems();
        _visualItems->deleteLater();
        _settingsItem = NULL;
        _visualItems = newControllerMissionItems;

        if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() || _visualItems->count() == 0) {
            _addMissionSettings(_activeVehicle, _visualItems, _editMode && _visualItems->count() > 0 /* addToCenter */);
        }

        _missionItemsRequested = false;

        if (_editMode) {
            MissionController::_scanForAdditionalSettings(_visualItems, _activeVehicle);
        }

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
    sendItemsToVehicle(_activeVehicle, _visualItems);
    _visualItems->setDirty(false);
}

/// Converts from visual items to MissionItems
///     @param missionItemParent QObject parent for newly allocated MissionItems
/// @return true: Mission end action was added to end of list
bool MissionController::_convertToMissionItems(QmlObjectListModel* visualMissionItems, QList<MissionItem*>& rgMissionItems, QObject* missionItemParent)
{
    bool endActionSet = false;
    int lastSeqNum = 0;

    for (int i=0; i<visualMissionItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(visualMissionItems->get(i));

        lastSeqNum = visualItem->lastSequenceNumber();
        visualItem->appendMissionItems(rgMissionItems, missionItemParent);

        qCDebug(MissionControllerLog) << "_convertToMissionItems seqNum:lastSeqNum:command"
                                      << visualItem->sequenceNumber()
                                      << lastSeqNum
                                      << visualItem->commandName();
    }

    // Mission settings has a special case for end mission action
    MissionSettingsItem* settingsItem = visualMissionItems->value<MissionSettingsItem*>(0);
    if (settingsItem) {
        endActionSet = settingsItem->addMissionEndAction(rgMissionItems, lastSeqNum + 1, missionItemParent);
    }

    return endActionSet;
}

void MissionController::sendItemsToVehicle(Vehicle* vehicle, QmlObjectListModel* visualMissionItems)
{
    if (vehicle) {
        QList<MissionItem*> rgMissionItems;

        _convertToMissionItems(visualMissionItems, rgMissionItems, vehicle);

        vehicle->missionManager()->writeMissionItems(rgMissionItems);

        for (int i=0; i<rgMissionItems.count(); i++) {
            rgMissionItems[i]->deleteLater();
        }
    }
}

int MissionController::_nextSequenceNumber(void)
{
    if (_visualItems->count() == 0) {
        qWarning() << "Internal error: Empty visual item list";
        return 0;
    } else {
        VisualMissionItem* lastItem = _visualItems->value<VisualMissionItem*>(_visualItems->count() - 1);
        return lastItem->lastSequenceNumber() + 1;
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
        double      prevAltitude;
        MAV_FRAME   prevFrame;

        if (_findPreviousAltitude(i, &prevAltitude, &prevFrame)) {
            newItem->missionItem().setFrame(prevFrame);
            newItem->missionItem().setParam7(prevAltitude);
        }
    }
    _visualItems->insert(i, newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

int MissionController::insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int i)
{
    ComplexMissionItem* newItem;

    int sequenceNumber = _nextSequenceNumber();
    if (itemName == _surveyMissionItemName) {
        newItem = new SurveyMissionItem(_activeVehicle, _visualItems);
        newItem->setCoordinate(mapCenterCoordinate);
    } else if (itemName == _fwLandingMissionItemName) {
        newItem = new FixedWingLandingComplexItem(_activeVehicle, _visualItems);
    } else {
        qWarning() << "Internal error: Unknown complex item:" << itemName;
        return sequenceNumber;
    }
    newItem->setSequenceNumber(sequenceNumber);
    _initVisualItem(newItem);

    _visualItems->insert(i, newItem);

    _recalcAll();

    return newItem->sequenceNumber();
}

void MissionController::removeMissionItem(int index)
{
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->removeAt(index));

    _deinitVisualItem(item);
    item->deleteLater();

    _recalcAll();
    _visualItems->setDirty(true);
}

void MissionController::removeAll(void)
{
    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteLater();
        _settingsItem = NULL;
        _visualItems = new QmlObjectListModel(this);
        _addMissionSettings(_activeVehicle, _visualItems, false /* addToCenter */);
        _initAllVisualItems();
        _visualItems->setDirty(true);
    }
}

bool MissionController::_loadJsonMissionFile(Vehicle* vehicle, const QByteArray& bytes, QmlObjectListModel* visualItems, QString& errorString)
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
        return _loadJsonMissionFileV1(vehicle, json, visualItems, errorString);
    } else {
        return _loadJsonMissionFileV2(vehicle, json, visualItems, errorString);
    }
}

bool MissionController::_loadJsonMissionFileV1(Vehicle* vehicle, const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString)
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
    QList<SurveyMissionItem*> surveyItems;
    QJsonArray complexArray(json[_jsonComplexItemsKey].toArray());
    qCDebug(MissionControllerLog) << "Json load: complex item count" << complexArray.count();
    for (int i=0; i<complexArray.count(); i++) {
        const QJsonValue& itemValue = complexArray[i];

        if (!itemValue.isObject()) {
            errorString = QStringLiteral("Mission item is not an object");
            return false;
        }

        SurveyMissionItem* item = new SurveyMissionItem(vehicle, visualItems);
        const QJsonObject itemObject = itemValue.toObject();
        if (item->load(itemObject, itemObject["id"].toInt(), errorString)) {
            surveyItems.append(item);
        } else {
            return false;
        }
    }

    // Read simple items, interspersing complex items into the full list

    int nextSimpleItemIndex= 0;
    int nextComplexItemIndex= 0;
    int nextSequenceNumber = 1; // Start with 1 since home is in 0
    QJsonArray itemArray(json[_jsonItemsKey].toArray());

    qCDebug(MissionControllerLog) << "Json load: simple item loop start simpleItemCount:ComplexItemCount" << itemArray.count() << surveyItems.count();
    do {
        qCDebug(MissionControllerLog) << "Json load: simple item loop nextSimpleItemIndex:nextComplexItemIndex:nextSequenceNumber" << nextSimpleItemIndex << nextComplexItemIndex << nextSequenceNumber;

        // If there is a complex item that should be next in sequence add it in
        if (nextComplexItemIndex < surveyItems.count()) {
            SurveyMissionItem* complexItem = surveyItems[nextComplexItemIndex];

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
            SimpleMissionItem* item = new SimpleMissionItem(vehicle, visualItems);
            if (item->load(itemObject, itemObject["id"].toInt(), errorString)) {
                qCDebug(MissionControllerLog) << "Json load: adding simple item expectedSequence:actualSequence" << nextSequenceNumber << item->sequenceNumber();
                nextSequenceNumber = item->lastSequenceNumber() + 1;
                visualItems->append(item);
            } else {
                return false;
            }
        }
    } while (nextSimpleItemIndex < itemArray.count() || nextComplexItemIndex < surveyItems.count());

    if (json.contains(_jsonPlannedHomePositionKey)) {
        SimpleMissionItem* item = new SimpleMissionItem(vehicle, visualItems);

        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), 0, errorString)) {
            MissionSettingsItem* settingsItem = new MissionSettingsItem(vehicle, visualItems);
            settingsItem->setCoordinate(item->coordinate());
            visualItems->insert(0, settingsItem);
            item->deleteLater();
        } else {
            return false;
        }
    } else {
        _addMissionSettings(vehicle, visualItems, true /* addToCenter */);
    }

    return true;
}

bool MissionController::_loadJsonMissionFileV2(Vehicle* vehicle, const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString)
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
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    if (!JsonHelper::loadGeoCoordinate(json[_jsonPlannedHomePositionKey], true /* altitudeRequired */, homeCoordinate, errorString)) {
        return false;
    }
    if (json.contains(_jsonVehicleTypeKey) && vehicle->isOfflineEditingVehicle()) {
        appSettings->offlineEditingVehicleType()->setRawValue(json[_jsonVehicleTypeKey].toDouble());
    }
    if (json.contains(_jsonCruiseSpeedKey)) {
        appSettings->offlineEditingCruiseSpeed()->setRawValue(json[_jsonCruiseSpeedKey].toDouble());
    }
    if (json.contains(_jsonHoverSpeedKey)) {
        appSettings->offlineEditingHoverSpeed()->setRawValue(json[_jsonHoverSpeedKey].toDouble());
    }

    MissionSettingsItem* settingsItem = new MissionSettingsItem(vehicle, visualItems);
    settingsItem->setCoordinate(homeCoordinate);
    visualItems->insert(0, settingsItem);
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
            SimpleMissionItem* simpleItem = new SimpleMissionItem(vehicle, visualItems);
            if (simpleItem->load(itemObject, nextSequenceNumber, errorString)) {
                qCDebug(MissionControllerLog) << "Loading simple item: nextSequenceNumber:command" << nextSequenceNumber << simpleItem->command();
                nextSequenceNumber = simpleItem->lastSequenceNumber() + 1;
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
                SurveyMissionItem* surveyItem = new SurveyMissionItem(vehicle, visualItems);
                if (!surveyItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = surveyItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Survey load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(surveyItem);
            } else if (complexItemType == FixedWingLandingComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Fixed Wing Landing Pattern: nextSequenceNumber" << nextSequenceNumber;
                FixedWingLandingComplexItem* landingItem = new FixedWingLandingComplexItem(vehicle, visualItems);
                if (!landingItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = landingItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "FW Landing Pattern load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(landingItem);
            } else if (complexItemType == MissionSettingsItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Mission Settings: nextSequenceNumber" << nextSequenceNumber;
                MissionSettingsItem* settingsItem = new MissionSettingsItem(vehicle, visualItems);
                if (!settingsItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = settingsItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Mission Settings load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(settingsItem);
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

bool MissionController::_loadTextMissionFile(Vehicle* vehicle, QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString)
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
            SimpleMissionItem* item = new SimpleMissionItem(vehicle, visualItems);

            if (item->load(stream)) {
                visualItems->append(item);
            } else {
                errorString = QStringLiteral("The mission file is corrupted.");
                return false;
            }
        }
    } else {
        errorString = QStringLiteral("The mission file is not compatible with this version of %1.").arg(qgcApp()->applicationName());
        return false;
    }

    if (addPlannedHomePosition || visualItems->count() == 0) {
        _addMissionSettings(vehicle, visualItems, true /* addToCenter */);

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
    QmlObjectListModel* newVisualItems = NULL;

    if (!loadItemsFromFile(_activeVehicle, filename, &newVisualItems)) {
        return;
    }

    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteLater();
        _settingsItem = NULL;
    }

    _visualItems = newVisualItems;

    if (_visualItems->count() == 0) {
        _addMissionSettings(_activeVehicle, _visualItems, true /* addToCenter */);
    }

    MissionController::_scanForAdditionalSettings(_visualItems, _activeVehicle);

    _initAllVisualItems();
}

bool MissionController::loadItemsFromFile(Vehicle* vehicle, const QString& filename, QmlObjectListModel** visualItems)
{
    *visualItems = NULL;

    QString errorString;

    if (filename.isEmpty()) {
        return false;
    }

    *visualItems = new QmlObjectListModel();

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
    } else {
        QByteArray  bytes = file.readAll();
        QTextStream stream(&bytes);

        QString firstLine = stream.readLine();
        if (firstLine.contains(QRegExp("QGC.*WPL"))) {
            stream.seek(0);
            _loadTextMissionFile(vehicle, stream, *visualItems, errorString);
        } else {
            _loadJsonMissionFile(vehicle, bytes, *visualItems, errorString);
        }
    }

    if (!errorString.isEmpty()) {
        (*visualItems)->deleteLater();

        qgcApp()->showMessage(errorString);
        return false;
    }

    return true;
}

void MissionController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString missionFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        missionFilename += QString(".%1").arg(AppSettings::missionFileExtension);
    }

    QFile file(missionFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(tr("Mission save %1 : %2").arg(filename).arg(file.errorString()));
    } else {
        QJsonObject missionFileObject;      // top level json object

        missionFileObject[JsonHelper::jsonVersionKey] =         _missionFileVersion;
        missionFileObject[JsonHelper::jsonGroundStationKey] =   JsonHelper::jsonGroundStationValue;

        // Mission settings

        MissionSettingsItem* settingsItem = _visualItems->value<MissionSettingsItem*>(0);
        if (!settingsItem) {
            qWarning() << "First item is not MissionSettingsItem";
            return;
        }
        QJsonValue coordinateValue;
        JsonHelper::saveGeoCoordinate(settingsItem->coordinate(), true /* writeAltitude */, coordinateValue);
        missionFileObject[_jsonPlannedHomePositionKey] = coordinateValue;
        missionFileObject[_jsonFirmwareTypeKey] = _activeVehicle->firmwareType();
        missionFileObject[_jsonVehicleTypeKey] = _activeVehicle->vehicleType();
        missionFileObject[_jsonCruiseSpeedKey] = _activeVehicle->defaultCruiseSpeed();
        missionFileObject[_jsonHoverSpeedKey] = _activeVehicle->defaultHoverSpeed();

        // Save the visual items

        QJsonArray rgJsonMissionItems;
        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

            visualItem->save(rgJsonMissionItems);
        }

        // Mission settings has a special case for end mission action
        if (settingsItem) {
            QList<MissionItem*> rgMissionItems;

            if (_convertToMissionItems(_visualItems, rgMissionItems, this /* missionItemParent */)) {
                QJsonObject saveObject;
                MissionItem* missionItem = rgMissionItems[rgMissionItems.count() - 1];
                missionItem->save(saveObject);
                rgJsonMissionItems.append(saveObject);
            }
            for (int i=0; i<rgMissionItems.count(); i++) {
                rgMissionItems[i]->deleteLater();
            }
        }

        missionFileObject[_jsonItemsKey] = rgJsonMissionItems;

        QJsonDocument saveDoc(missionFileObject);
        file.write(saveDoc.toJson());
    }

    // If we are connected to a real vehicle, don't clear dirty bit on saving to file since vehicle is still out of date
    if (_activeVehicle->isOfflineEditingVehicle()) {
        _visualItems->setDirty(false);
    }
}

void MissionController::_calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  prevCoord =     prevItem->exitCoordinate();
    bool            distanceOk =    false;

    // Convert to fixed altitudes

    distanceOk = true;
    if (currentItem->coordinateHasRelativeAltitude()) {
        currentCoord.setAltitude(homeAlt + currentCoord.altitude());
    }
    if (prevItem->exitCoordinateHasRelativeAltitude()) {
        prevCoord.setAltitude(homeAlt + prevCoord.altitude());
    }

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

    return distanceOk ? homeCoord.distanceTo(currentCoord) : 0.0;
}

void MissionController::_recalcWaypointLines(void)
{
    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    bool showHomePosition = _settingsItem->coordinate().isValid();

    qCDebug(MissionControllerLog) << "_recalcWaypointLines showHomePosition" << showHomePosition;

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
                if (lastCoordinateItem != _settingsItem || (showHomePosition && linkBackToHome)) {
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
                        connect(item, &VisualMissionItem::coordinateChanged, this, &MissionController::_recalcMissionFlightStatus);
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

    _recalcMissionFlightStatus();

    emit waypointLinesChanged();
}

void MissionController::_updateBatteryInfo(int waypointIndex)
{
    if (_missionFlightStatus.mAhBattery != 0) {
        _missionFlightStatus.hoverAmpsTotal = (_missionFlightStatus.hoverTime / 60.0) * _missionFlightStatus.hoverAmps;
        _missionFlightStatus.cruiseAmpsTotal = (_missionFlightStatus.cruiseTime / 60.0) * _missionFlightStatus.cruiseAmps;
        _missionFlightStatus.batteriesRequired = ceil((_missionFlightStatus.hoverAmpsTotal + _missionFlightStatus.cruiseAmpsTotal) / _missionFlightStatus.ampMinutesAvailable);
        if (_missionFlightStatus.batteriesRequired == 2 && _missionFlightStatus.batteryChangePoint == -1) {
            _missionFlightStatus.batteryChangePoint = waypointIndex - 1;
        }
    }
}

void MissionController::_addHoverTime(double hoverTime, double hoverDistance, int waypointIndex)
{
    _missionFlightStatus.totalTime += hoverTime;
    _missionFlightStatus.hoverTime += hoverTime;
    _missionFlightStatus.hoverDistance += hoverDistance;
    _missionFlightStatus.totalDistance += hoverDistance;
    _updateBatteryInfo(waypointIndex);
}

void MissionController::_addCruiseTime(double cruiseTime, double cruiseDistance, int waypointIndex)
{
    _missionFlightStatus.totalTime += cruiseTime;
    _missionFlightStatus.cruiseTime += cruiseTime;
    _missionFlightStatus.cruiseDistance += cruiseDistance;
    _missionFlightStatus.totalDistance += cruiseDistance;
    _updateBatteryInfo(waypointIndex);
}

void MissionController::_recalcMissionFlightStatus()
{
    if (!_visualItems->count()) {
        return;
    }

    bool                firstCoordinateItem =   true;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    bool showHomePosition = _settingsItem->coordinate().isValid();

    qCDebug(MissionControllerLog) << "_recalcMissionFlightStatus";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItem->setAltDifference(0.0);
    lastCoordinateItem->setAzimuth(0.0);
    lastCoordinateItem->setDistance(0.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    const double homePositionAltitude = _settingsItem->coordinate().altitude();
    minAltSeen = maxAltSeen = _settingsItem->coordinate().altitude();

    double lastVehicleYaw = 0;

    _resetMissionFlightStatus();

    bool vtolInHover = true;
    bool linkBackToHome = false;

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(item);
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(item);

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(0.0);

        // Look for speed changed
        double newSpeed = item->specifiedFlightSpeed();
        if (!qIsNaN(newSpeed)) {
            if (_activeVehicle->multiRotor()) {
                _missionFlightStatus.hoverSpeed = newSpeed;
            } else if (_activeVehicle->vtol()) {
                if (vtolInHover) {
                    _missionFlightStatus.hoverSpeed = newSpeed;
                } else {
                    _missionFlightStatus.cruiseSpeed = newSpeed;
                }
            } else {
                _missionFlightStatus.cruiseSpeed = newSpeed;
            }
            _missionFlightStatus.vehicleSpeed = newSpeed;
        }

        // Look for gimbal change
        if (_activeVehicle->vehicleYawsToNextWaypointInMission()) {
            // We current only support gimbal display in this mode
            double gimbalYaw = item->specifiedGimbalYaw();
            if (!qIsNaN(gimbalYaw)) {
                _missionFlightStatus.gimbalYaw = gimbalYaw;
            }
        }

        if (i == 0) {
            // We only process speed and gimbal from Mission Settings item
            continue;
        }

        // Link back to home if first item is takeoff and we have home position
        if (firstCoordinateItem && simpleItem && simpleItem->command() == MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF) {
            if (showHomePosition) {
                linkBackToHome = true;
            }
        }

        // Update VTOL state
        if (simpleItem && _activeVehicle->vtol()) {
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
            // Update vehicle yaw assuming direction to next waypoint
            if (item != lastCoordinateItem) {
                lastVehicleYaw = lastCoordinateItem->exitCoordinate().azimuthTo(item->coordinate());
                lastCoordinateItem->setMissionVehicleYaw(lastVehicleYaw);
            }

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
                if (lastCoordinateItem != _settingsItem || linkBackToHome) {
                    // This is a subsequent waypoint or we are forcing the first waypoint back to home
                    double azimuth, distance, altDifference;

                    _calcPrevWaypointValues(homePositionAltitude, item, lastCoordinateItem, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);

                    _missionFlightStatus.maxTelemetryDistance = qMax(_missionFlightStatus.maxTelemetryDistance, _calcDistanceToHome(item, _settingsItem));

                    // Calculate time/distance
                    double hoverTime = distance / _missionFlightStatus.hoverSpeed;
                    double cruiseTime = distance / _missionFlightStatus.cruiseSpeed;
                    if (_activeVehicle->vtol()) {
                        if (vtolInHover) {
                            _addHoverTime(hoverTime, distance, item->sequenceNumber());
                        } else {
                            _addCruiseTime(cruiseTime, distance, item->sequenceNumber());
                        }
                    } else {
                        if (_activeVehicle->multiRotor()) {
                            _addHoverTime(hoverTime, distance, item->sequenceNumber());
                        } else {
                            _addCruiseTime(cruiseTime, distance, item->sequenceNumber());
                        }
                    }
                }

                if (complexItem) {
                    // Add in distance/time inside complex items as well
                    double distance = complexItem->complexDistance();
                    _missionFlightStatus.maxTelemetryDistance = qMax(_missionFlightStatus.maxTelemetryDistance, complexItem->greatestDistanceTo(complexItem->exitCoordinate()));

                    double hoverTime = distance / _missionFlightStatus.hoverSpeed;
                    double cruiseTime = distance / _missionFlightStatus.cruiseSpeed;
                    if (_activeVehicle->vtol()) {
                        if (vtolInHover) {
                            _addHoverTime(hoverTime, distance, item->sequenceNumber());
                        } else {
                            _addCruiseTime(cruiseTime, distance, item->sequenceNumber());
                        }
                    } else {
                        if (_activeVehicle->multiRotor()) {
                            _addHoverTime(hoverTime, distance, item->sequenceNumber());
                        } else {
                            _addCruiseTime(cruiseTime, distance, item->sequenceNumber());
                        }
                    }
                }

                item->setMissionFlightStatus(_missionFlightStatus);
            }

            lastCoordinateItem = item;
        }
    }
    lastCoordinateItem->setMissionVehicleYaw(lastVehicleYaw);

    if (_missionFlightStatus.mAhBattery != 0 && _missionFlightStatus.batteryChangePoint == -1) {
        _missionFlightStatus.batteryChangePoint = 0;
    }

    emit missionMaxTelemetryChanged(_missionFlightStatus.maxTelemetryDistance);
    emit missionDistanceChanged(_missionFlightStatus.totalDistance);
    emit missionHoverDistanceChanged(_missionFlightStatus.hoverDistance);
    emit missionCruiseDistanceChanged(_missionFlightStatus.cruiseDistance);
    emit missionTimeChanged();
    emit missionHoverTimeChanged();
    emit missionCruiseTimeChanged();
    emit batteryChangePointChanged(_missionFlightStatus.batteryChangePoint);
    emit batteriesRequiredChanged(_missionFlightStatus.batteriesRequired);

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

        item->setSequenceNumber(sequenceNumber);
        sequenceNumber = item->lastSequenceNumber() + 1;
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

void MissionController::_setPlannedHomePositionFromFirstCoordinate(void)
{
    if (_settingsItem->coordinate().isValid()) {
        return;
    }

    // Set the planned home position to be a deltae from first coordinate
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = _visualItems->value<VisualMissionItem*>(i);

        if (item->specifiesCoordinate()) {
            _settingsItem->setCoordinate(item->coordinate().atDistanceAndAzimuth(30, 0));
        }
    }
}


void MissionController::_recalcAll(void)
{
    if (_editMode) {
        _setPlannedHomePositionFromFirstCoordinate();
    }
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
}

/// Initializes a new set of mission items
void MissionController::_initAllVisualItems(void)
{
    // Setup home position at index 0

    _settingsItem = qobject_cast<MissionSettingsItem*>(_visualItems->get(0));
    if (!_settingsItem) {
        qWarning() << "First item not MissionSettingsItem";
        return;
    }
    _settingsItem->setIsCurrentItem(true);

    if (!_editMode && _activeVehicle) {
        _settingsItem->setCoordinate(_activeVehicle->homePosition());
    }

    connect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &MissionController::_recalcAll);

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        _initVisualItem(item);
    }

    _recalcAll();

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_visualItemsDirtyChanged);
    connect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::_updateContainsItems);

    emit visualItemsChanged();
    emit containsItemsChanged(containsItems());

    _visualItems->setDirty(false);
}

void MissionController::_deinitAllVisualItems(void)
{
    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    disconnect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_visualItemsDirtyChanged);
    disconnect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::_updateContainsItems);
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    _visualItems->setDirty(false);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::specifiedFlightSpeedChanged,                this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::specifiedGimbalYawChanged,                  this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::lastSequenceNumberChanged,                  this, &MissionController::_recalcSequence);

    if (visualItem->isSimpleItem()) {
        // We need to track commandChanged on simple item since recalc has special handling for takeoff command
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        if (simpleItem) {
            connect(&simpleItem->missionItem()._commandFact, &Fact::valueChanged, this, &MissionController::_itemCommandChanged);
        } else {
            qWarning() << "isSimpleItem == true, yet not SimpleMissionItem";
        }
    } else {
        ComplexMissionItem* complexItem = qobject_cast<ComplexMissionItem*>(visualItem);
        if (complexItem) {
            connect(complexItem, &ComplexMissionItem::complexDistanceChanged, this, &MissionController::_recalcMissionFlightStatus);
        } else {
            qWarning() << "ComplexMissionItem not found";
        }
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
    disconnect(missionManager, &MissionManager::lastCurrentItemChanged,     this, &MissionController::resumeMissionItemChanged);
    disconnect(missionManager, &MissionManager::resumeMissionReady,         this, &MissionController::resumeMissionReady);
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
    connect(missionManager, &MissionManager::lastCurrentItemChanged,    this, &MissionController::resumeMissionItemChanged);
    connect(missionManager, &MissionManager::resumeMissionReady,        this, &MissionController::resumeMissionReady);
    connect(_activeVehicle, &Vehicle::homePositionChanged,              this, &MissionController::_activeVehicleHomePositionChanged);
    connect(_activeVehicle, &Vehicle::defaultCruiseSpeedChanged,        this, &MissionController::_recalcMissionFlightStatus);
    connect(_activeVehicle, &Vehicle::defaultHoverSpeedChanged,         this, &MissionController::_recalcMissionFlightStatus);
    connect(_activeVehicle, &Vehicle::vehicleTypeChanged,               this, &MissionController::complexMissionItemNamesChanged);

    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
        // We don't request mission items for new vehicles since that will happen autamatically.
        loadFromVehicle();
    }

    _activeVehicleHomePositionChanged(_activeVehicle->homePosition());

    emit complexMissionItemNamesChanged();
    emit resumeMissionItemChanged();
}

void MissionController::_activeVehicleHomePositionChanged(const QGeoCoordinate& homePosition)
{
    if (_visualItems) {
        MissionSettingsItem* settingsItem = qobject_cast<MissionSettingsItem*>(_visualItems->get(0));
        if (settingsItem) {
            settingsItem->setCoordinate(homePosition);
        } else {
            qWarning() << "First item is not MissionSettingsItem";
        }
    }
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
}

bool MissionController::_findPreviousAltitude(int newIndex, double* prevAltitude, MAV_FRAME* prevFrame)
{
    bool        found = false;
    double      foundAltitude;
    MAV_FRAME   foundFrame;

    if (newIndex > _visualItems->count()) {
        return false;
    }
    newIndex--;

    for (int i=newIndex; i>0; i--) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {
            if (visualItem->isSimpleItem()) {
                SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
                if ((MAV_CMD)simpleItem->command() == MAV_CMD_NAV_WAYPOINT) {
                    foundAltitude = simpleItem->exitCoordinate().altitude();
                    foundFrame = simpleItem->missionItem().frame();
                    found = true;
                    break;
                }
            }
        }
    }

    if (found) {
        *prevAltitude = foundAltitude;
        *prevFrame = foundFrame;
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

/// Add the Mission Settings complex item to the front of the items
void MissionController::_addMissionSettings(Vehicle* vehicle, QmlObjectListModel* visualItems, bool addToCenter)
{
    MissionSettingsItem* settingsItem = new MissionSettingsItem(vehicle, visualItems);

    visualItems->insert(0, settingsItem);

    if (addToCenter) {
        if (visualItems->count() > 1) {
            double north = 0.0;
            double south = 0.0;
            double east  = 0.0;
            double west  = 0.0;
            bool firstCoordSet = false;

            for (int i=1; i<visualItems->count(); i++) {
                VisualMissionItem* item = qobject_cast<VisualMissionItem*>(visualItems->get(i));
                if (item->specifiesCoordinate()) {
                    if (firstCoordSet) {
                        double lat = _normalizeLat(item->coordinate().latitude());
                        double lon = _normalizeLon(item->coordinate().longitude());
                        north = fmax(north, lat);
                        south = fmin(south, lat);
                        east  = fmax(east, lon);
                        west  = fmin(west, lon);
                    } else {
                        firstCoordSet = true;
                        north = _normalizeLat(item->coordinate().latitude());
                        south = north;
                        east  = _normalizeLon(item->coordinate().longitude());
                        west  = east;
                    }
                }
            }

            if (firstCoordSet) {
                settingsItem->setCoordinate(QGeoCoordinate((south + ((north - south) / 2)) - 90.0, (west + ((east - west) / 2)) - 180.0, 0.0));
            }
        }
    } else {
        settingsItem->setCoordinate(vehicle->homePosition());
    }
}

int MissionController::resumeMissionItem(void) const
{

    int resumeIndex = -1;

    if (!_editMode) {
        int firstTrueItemIndex = _activeVehicle->firmwarePlugin()->sendHomePositionToVehicle() ? 1 : 0;
        resumeIndex = _activeVehicle->missionManager()->lastCurrentItem();
        if (resumeIndex > firstTrueItemIndex) {
            if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
                resumeIndex++;
            }
            // Resume at the item previous to the item we were heading towards
            resumeIndex--;
        }
    }

    return resumeIndex;
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

QString MissionController::fileExtension(void) const
{
    return AppSettings::missionFileExtension;
}

void MissionController::_scanForAdditionalSettings(QmlObjectListModel* visualItems, Vehicle* vehicle)
{
    // First we look for a Fixed Wing Landing Pattern which is at the end
    FixedWingLandingComplexItem::scanForItem(visualItems, vehicle);

    int scanIndex = 0;
    while (scanIndex < visualItems->count()) {
        VisualMissionItem* visualItem = visualItems->value<VisualMissionItem*>(scanIndex);

        qCDebug(MissionControllerLog) << "MissionController::_scanForAdditionalSettings count:scanIndex" << visualItems->count() << scanIndex;

        MissionSettingsItem* settingsItem = qobject_cast<MissionSettingsItem*>(visualItem);
        if (settingsItem && settingsItem->scanForMissionSettings(visualItems, scanIndex, vehicle)) {
            continue;
        }

        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        if (simpleItem) {
            simpleItem->scanForSections(visualItems, scanIndex + 1, vehicle);
        }

        scanIndex++;
    }
}

void MissionController::_updateContainsItems(void)
{
    emit containsItemsChanged(containsItems());
}

bool MissionController::containsItems(void) const
{
    return _visualItems ? _visualItems->count() > 1 : false;
}

void MissionController::removeAllFromVehicle(void)
{
    _missionItemsRequested = true;
    _activeVehicle->missionManager()->removeAll();
}

QStringList MissionController::complexMissionItemNames(void) const
{
    QStringList complexItems;

    complexItems.append(_surveyMissionItemName);
    if (_activeVehicle->fixedWing()) {
        complexItems.append(_fwLandingMissionItemName);
    }

    return complexItems;
}

void MissionController::_visualItemsDirtyChanged(bool dirty)
{
    if (dirty) {
        if (_visualItems->count() > 1) {
            emit dirtyChanged(true);
        } else {
            // This was a change to mission settings with no other mission items added
            _visualItems->setDirty(false);
        }
    } else {
        emit dirtyChanged(false);
    }
}

void MissionController::resumeMission(int resumeIndex)
{
    if (!_activeVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
        resumeIndex--;
    }
    _activeVehicle->missionManager()->generateResumeMission(resumeIndex);
}
