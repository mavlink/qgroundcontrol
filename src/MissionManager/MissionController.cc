/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionCommandUIInfo.h"
#include "MissionController.h"
#include "MultiVehicleManager.h"
#include "MissionManager.h"
#include "CoordinateVector.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "SimpleMissionItem.h"
#include "SurveyComplexItem.h"
#include "FixedWingLandingComplexItem.h"
#include "StructureScanComplexItem.h"
#include "CorridorScanComplexItem.h"
#include "JsonHelper.h"
#include "ParameterManager.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionSettingsItem.h"
#include "QGCQGeoCoordinate.h"
#include "PlanMasterController.h"
#include "KML.h"
#include "QGCCorePlugin.h"
#include "TakeoffMissionItem.h"

#define UPDATE_TIMEOUT 5000 ///< How often we check for bounding box changes

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

const QString MissionController::patternSurveyName          (QT_TRANSLATE_NOOP("MissionController", "Survey"));
const QString MissionController::patternFWLandingName       (QT_TRANSLATE_NOOP("MissionController", "Fixed Wing Landing"));
const QString MissionController::patternStructureScanName   (QT_TRANSLATE_NOOP("MissionController", "Structure Scan"));
const QString MissionController::patternCorridorScanName    (QT_TRANSLATE_NOOP("MissionController", "Corridor Scan"));

MissionController::MissionController(PlanMasterController* masterController, QObject *parent)
    : PlanElementController     (masterController, parent)
    , _missionManager           (_managerVehicle->missionManager())
    , _missionItemCount         (0)
    , _visualItems              (nullptr)
    , _settingsItem             (nullptr)
    , _firstItemsFromVehicle    (false)
    , _itemsRequested           (false)
    , _inRecalcSequence         (false)
    , _appSettings              (qgcApp()->toolbox()->settingsManager()->appSettings())
    , _progressPct              (0)
    , _currentPlanViewSeqNum    (-1)
    , _currentPlanViewVIIndex   (-1)
    , _currentPlanViewItem      (nullptr)
    , _splitSegment             (nullptr)
{
    _resetMissionFlightStatus();
    managerVehicleChanged(_managerVehicle);
    _updateTimer.setSingleShot(true);
    connect(&_updateTimer, &QTimer::timeout, this, &MissionController::_updateTimeout);
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
    _missionFlightStatus.cruiseSpeed =          _controllerVehicle->defaultCruiseSpeed();
    _missionFlightStatus.hoverSpeed =           _controllerVehicle->defaultHoverSpeed();
    _missionFlightStatus.vehicleSpeed =         _controllerVehicle->multiRotor() || _managerVehicle->vtol() ? _missionFlightStatus.hoverSpeed : _missionFlightStatus.cruiseSpeed;
    _missionFlightStatus.vehicleYaw =           0.0;
    _missionFlightStatus.gimbalYaw =            std::numeric_limits<double>::quiet_NaN();
    _missionFlightStatus.gimbalPitch =          std::numeric_limits<double>::quiet_NaN();

    // Battery information

    _missionFlightStatus.mAhBattery =           0;
    _missionFlightStatus.hoverAmps =            0;
    _missionFlightStatus.cruiseAmps =           0;
    _missionFlightStatus.ampMinutesAvailable =  0;
    _missionFlightStatus.hoverAmpsTotal =       0;
    _missionFlightStatus.cruiseAmpsTotal =      0;
    _missionFlightStatus.batteryChangePoint =   -1;
    _missionFlightStatus.batteriesRequired =    -1;

    _controllerVehicle->firmwarePlugin()->batteryConsumptionData(_controllerVehicle, _missionFlightStatus.mAhBattery, _missionFlightStatus.hoverAmps, _missionFlightStatus.cruiseAmps);
    if (_missionFlightStatus.mAhBattery != 0) {
        double batteryPercentRemainingAnnounce = qgcApp()->toolbox()->settingsManager()->appSettings()->batteryPercentRemainingAnnounce()->rawValue().toDouble();
        _missionFlightStatus.ampMinutesAvailable = static_cast<double>(_missionFlightStatus.mAhBattery) / 1000.0 * 60.0 * ((100.0 - batteryPercentRemainingAnnounce) / 100.0);
    }

    emit missionDistanceChanged(_missionFlightStatus.totalDistance);
    emit missionTimeChanged();
    emit missionHoverDistanceChanged(_missionFlightStatus.hoverDistance);
    emit missionCruiseDistanceChanged(_missionFlightStatus.cruiseDistance);
    emit missionHoverTimeChanged();
    emit missionCruiseTimeChanged();
    emit missionMaxTelemetryChanged(_missionFlightStatus.maxTelemetryDistance);
    emit batteryChangePointChanged(_missionFlightStatus.batteryChangePoint);
    emit batteriesRequiredChanged(_missionFlightStatus.batteriesRequired);

}

void MissionController::start(bool flyView)
{
    qCDebug(MissionControllerLog) << "start flyView" << flyView;

    PlanElementController::start(flyView);
    _init();
}

void MissionController::_init(void)
{
    // We start with an empty mission
    _visualItems = new QmlObjectListModel(this);
    _addMissionSettings(_visualItems);
    _initAllVisualItems();
}

// Called when new mission items have completed downloading from Vehicle
void MissionController::_newMissionItemsAvailableFromVehicle(bool removeAllRequested)
{
    qCDebug(MissionControllerLog) << "_newMissionItemsAvailableFromVehicle flyView:count" << _flyView << _missionManager->missionItems().count();

    // Fly view always reloads on _loadComplete
    // Plan view only reloads if:
    //  - Load was specifically requested
    //  - There is no current Plan
    if (_flyView || removeAllRequested || _itemsRequested || isEmpty()) {
        // Fly Mode (accept if):
        //      - Always accepts new items from the vehicle so Fly view is kept up to date
        // Edit Mode (accept if):
        //      - Remove all was requested from Fly view (clear mission on flight end)
        //      - A load from vehicle was manually requested
        //      - The initial automatic load from a vehicle completed and the current editor is empty

        _deinitAllVisualItems();
        _visualItems->deleteLater();
        _visualItems  = nullptr;
        _settingsItem = nullptr;
        _updateContainsItems(); // This will clear containsItems which will be set again below. This will re-pop Start Mission confirmation.

        QmlObjectListModel* newControllerMissionItems = new QmlObjectListModel(this);
        const QList<MissionItem*>& newMissionItems = _missionManager->missionItems();
        qCDebug(MissionControllerLog) << "loading from vehicle: count"<< newMissionItems.count();

        _missionItemCount = newMissionItems.count();
        emit missionItemCountChanged(_missionItemCount);

        MissionSettingsItem* settingsItem = _addMissionSettings(newControllerMissionItems);

        int i=0;
        if (_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle() && newMissionItems.count() != 0 && !_flyView) {
            // First item is fake home position
            MissionItem* fakeHomeItem = newMissionItems[0];
            if (fakeHomeItem->coordinate().latitude() != 0 || fakeHomeItem->coordinate().longitude() != 0) {
                settingsItem->setInitialHomePosition(fakeHomeItem->coordinate());
            }
            i = 1;
        }

        for (; i < newMissionItems.count(); i++) {
            const MissionItem* missionItem = newMissionItems[i];
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_controllerVehicle, _flyView, *missionItem, this);
            if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(simpleItem->command()))) {
                // This needs to be a TakeoffMissionItem
                TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(*missionItem, _controllerVehicle, _flyView, settingsItem, this);
                simpleItem->deleteLater();
                simpleItem = takeoffItem;
            }
            newControllerMissionItems->append(simpleItem);
        }

        _visualItems = newControllerMissionItems;
        _settingsItem = settingsItem;

        MissionController::_scanForAdditionalSettings(_visualItems, _controllerVehicle);

        _initAllVisualItems();
        _updateContainsItems();
        emit newItemsFromVehicle();
    }
    _itemsRequested = false;
}

void MissionController::loadFromVehicle(void)
{
    if (_masterController->offline()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::loadFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::loadFromVehicle called while syncInProgress";
    } else {
        _itemsRequested = true;
        _managerVehicle->missionManager()->loadFromVehicle();
    }
}

void MissionController::_warnIfTerrainFrameUsed(void)
{
    for (int i=1; i<_visualItems->count(); i++) {
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(_visualItems->get(i));
        if (simpleItem && simpleItem->altitudeMode() == QGroundControlQmlGlobal::AltitudeModeTerrainFrame) {
            qgcApp()->showMessage(tr("Warning: You are using MAV_FRAME_GLOBAL_TERRAIN_ALT in a mission. %1 does not support sending terrain tiles to vehicle.").arg(qgcApp()->applicationName()));
            break;
        }
    }
}

void MissionController::sendToVehicle(void)
{
    if (_masterController->offline()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(MissionControllerLog) << "MissionControllerLog::sendToVehicle";
        _warnIfTerrainFrameUsed();
        if (_visualItems->count() == 1) {
            // This prevents us from sending a possibly bogus home position to the vehicle
            QmlObjectListModel emptyModel;
            sendItemsToVehicle(_managerVehicle, &emptyModel);
        } else {
            sendItemsToVehicle(_managerVehicle, _visualItems);
        }
        setDirty(false);
    }
}

/// Converts from visual items to MissionItems
///     @param missionItemParent QObject parent for newly allocated MissionItems
/// @return true: Mission end action was added to end of list
bool MissionController::_convertToMissionItems(QmlObjectListModel* visualMissionItems, QList<MissionItem*>& rgMissionItems, QObject* missionItemParent)
{
    if (visualMissionItems->count() == 0) {
        return false;
    }

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

void MissionController::convertToKMLDocument(QDomDocument& document)
{
    QObject*            deleteParent = new QObject();
    QList<MissionItem*> rgMissionItems;

    _convertToMissionItems(_visualItems, rgMissionItems, deleteParent);
    if (rgMissionItems.count() == 0) {
        return;
    }

    const double homePositionAltitude = _settingsItem->coordinate().altitude();

    QString coord;
    QStringList coords;
    // Drop home position
    bool dropPoint = true;
    for(const auto& item : rgMissionItems) {
        if(dropPoint) {
            dropPoint = false;
            continue;
        }
        const MissionCommandUIInfo* uiInfo = \
                qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_controllerVehicle, item->command());

        if (uiInfo && uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
            double amslAltitude = item->param7() + (item->frame() == MAV_FRAME_GLOBAL ? 0 : homePositionAltitude);
            coord = QString::number(item->param6(),'f',7) \
                    + "," \
                    + QString::number(item->param5(),'f',7) \
                    + "," \
                    + QString::number(amslAltitude,'f',2);
            coords.append(coord);
        }
    }

    deleteParent->deleteLater();

    Kml kml;
    kml.points(coords);
    kml.save(document);
}

void MissionController::sendItemsToVehicle(Vehicle* vehicle, QmlObjectListModel* visualMissionItems)
{
    if (vehicle) {
        QList<MissionItem*> rgMissionItems;

        _convertToMissionItems(visualMissionItems, rgMissionItems, vehicle);

        // PlanManager takes control of MissionItems so no need to delete
        vehicle->missionManager()->writeMissionItems(rgMissionItems);
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

VisualMissionItem* MissionController::_insertSimpleMissionItemWorker(QGeoCoordinate coordinate, MAV_CMD command, int visualItemIndex, bool makeCurrentItem)
{
    int sequenceNumber = _nextSequenceNumber();
    SimpleMissionItem * newItem = new SimpleMissionItem(_controllerVehicle, _flyView, this);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    newItem->setCommand(command);
    _initVisualItem(newItem);

    if (newItem->specifiesAltitude()) {
        double  prevAltitude;
        int     prevAltitudeMode;

        if (_findPreviousAltitude(visualItemIndex, &prevAltitude, &prevAltitudeMode)) {
            newItem->altitude()->setRawValue(prevAltitude);
            newItem->setAltitudeMode(static_cast<QGroundControlQmlGlobal::AltitudeMode>(prevAltitudeMode));
        }
    }
    newItem->setMissionFlightStatus(_missionFlightStatus);
    if (visualItemIndex == -1) {
        _visualItems->append(newItem);
    } else {
        _visualItems->insert(visualItemIndex, newItem);
    }

    // We send the click coordinate through here to be able to set the planned home position from the user click location if needed
    _recalcAllWithCoordinate(coordinate);

    if (makeCurrentItem) {
        setCurrentPlanViewSeqNum(newItem->sequenceNumber(), true);
    }

    return newItem;
}


VisualMissionItem* MissionController::insertSimpleMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    return _insertSimpleMissionItemWorker(coordinate, MAV_CMD_NAV_WAYPOINT, visualItemIndex, makeCurrentItem);
}

VisualMissionItem* MissionController::insertTakeoffItem(QGeoCoordinate /*coordinate*/, int visualItemIndex, bool makeCurrentItem)
{
    int sequenceNumber = _nextSequenceNumber();
    TakeoffMissionItem * newItem = new TakeoffMissionItem(_controllerVehicle->vtol() ? MAV_CMD_NAV_VTOL_TAKEOFF : MAV_CMD_NAV_TAKEOFF, _controllerVehicle, _flyView, _settingsItem, this);
    newItem->setSequenceNumber(sequenceNumber);
    _initVisualItem(newItem);

    if (newItem->specifiesAltitude()) {
        double  prevAltitude;
        int     prevAltitudeMode;

        if (_findPreviousAltitude(visualItemIndex, &prevAltitude, &prevAltitudeMode)) {
            newItem->altitude()->setRawValue(prevAltitude);
            newItem->setAltitudeMode(static_cast<QGroundControlQmlGlobal::AltitudeMode>(prevAltitudeMode));
        }
    }
    newItem->setMissionFlightStatus(_missionFlightStatus);
    if (visualItemIndex == -1) {
        _visualItems->append(newItem);
    } else {
        _visualItems->insert(visualItemIndex, newItem);
    }

    _recalcAll();

    if (makeCurrentItem) {
        setCurrentPlanViewSeqNum(newItem->sequenceNumber(), true);
    }

    return newItem;
}

VisualMissionItem* MissionController::insertLandItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    if (_managerVehicle->fixedWing()) {
        FixedWingLandingComplexItem* fwLanding = qobject_cast<FixedWingLandingComplexItem*>(insertComplexMissionItem(MissionController::patternFWLandingName, coordinate, visualItemIndex, makeCurrentItem));
        fwLanding->setLoiterDragAngleOnly(true);
        return fwLanding;
    } else {
        return _insertSimpleMissionItemWorker(coordinate, MAV_CMD_NAV_RETURN_TO_LAUNCH, visualItemIndex, makeCurrentItem);
    }
}

VisualMissionItem* MissionController::insertROIMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(_insertSimpleMissionItemWorker(coordinate, MAV_CMD_DO_SET_ROI_LOCATION, visualItemIndex, makeCurrentItem));

    if (!_controllerVehicle->firmwarePlugin()->supportedMissionCommands().contains(MAV_CMD_DO_SET_ROI_LOCATION)) {
        simpleItem->setCommand(MAV_CMD_DO_SET_ROI)  ;
        simpleItem->missionItem().setParam1(MAV_ROI_LOCATION);
    }
    _recalcROISpecialVisuals();
    return simpleItem;
}

VisualMissionItem* MissionController::insertCancelROIMissionItem(int visualItemIndex, bool makeCurrentItem)
{
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(_insertSimpleMissionItemWorker(QGeoCoordinate(), MAV_CMD_DO_SET_ROI_NONE, visualItemIndex, makeCurrentItem));

    if (!_controllerVehicle->firmwarePlugin()->supportedMissionCommands().contains(MAV_CMD_DO_SET_ROI_NONE)) {
        simpleItem->setCommand(MAV_CMD_DO_SET_ROI)  ;
        simpleItem->missionItem().setParam1(MAV_ROI_NONE);
    }
    _recalcROISpecialVisuals();
    return simpleItem;
}

VisualMissionItem* MissionController::insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int visualItemIndex, bool makeCurrentItem)
{
    ComplexMissionItem* newItem = nullptr;

    if (itemName == patternSurveyName) {
        newItem = new SurveyComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, _visualItems /* parent */);
        newItem->setCoordinate(mapCenterCoordinate);
    } else if (itemName == patternFWLandingName) {
        newItem = new FixedWingLandingComplexItem(_controllerVehicle, _flyView, _visualItems /* parent */);
    } else if (itemName == patternStructureScanName) {
        newItem = new StructureScanComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, _visualItems /* parent */);
    } else if (itemName == patternCorridorScanName) {
        newItem = new CorridorScanComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, _visualItems /* parent */);
    } else {
        qWarning() << "Internal error: Unknown complex item:" << itemName;
        return nullptr;
    }

    _insertComplexMissionItemWorker(mapCenterCoordinate, newItem, visualItemIndex, makeCurrentItem);

    return newItem;
}

VisualMissionItem* MissionController::insertComplexMissionItemFromKMLOrSHP(QString itemName, QString file, int visualItemIndex, bool makeCurrentItem)
{
    ComplexMissionItem* newItem = nullptr;

    if (itemName == patternSurveyName) {
        newItem = new SurveyComplexItem(_controllerVehicle, _flyView, file, _visualItems);
    } else if (itemName == patternStructureScanName) {
        newItem = new StructureScanComplexItem(_controllerVehicle, _flyView, file, _visualItems);
    } else if (itemName == patternCorridorScanName) {
        newItem = new CorridorScanComplexItem(_controllerVehicle, _flyView, file, _visualItems);
    } else {
        qWarning() << "Internal error: Unknown complex item:" << itemName;
        return nullptr;
    }

    _insertComplexMissionItemWorker(QGeoCoordinate(), newItem, visualItemIndex, makeCurrentItem);

    return newItem;
}

void MissionController::_insertComplexMissionItemWorker(const QGeoCoordinate& mapCenterCoordinate, ComplexMissionItem* complexItem, int visualItemIndex, bool makeCurrentItem)
{
    int sequenceNumber = _nextSequenceNumber();
    bool surveyStyleItem = qobject_cast<SurveyComplexItem*>(complexItem) ||
            qobject_cast<CorridorScanComplexItem*>(complexItem) ||
            qobject_cast<StructureScanComplexItem*>(complexItem);

    if (surveyStyleItem) {
        bool rollSupported  = false;
        bool pitchSupported = false;
        bool yawSupported   = false;

        // If the vehicle is known to have a gimbal then we automatically point the gimbal straight down if not already set

        MissionSettingsItem* settingsItem = _visualItems->value<MissionSettingsItem*>(0);
        CameraSection* cameraSection = settingsItem->cameraSection();

        // Set camera to photo mode (leave alone if user already specified)
        if (cameraSection->cameraModeSupported() && !cameraSection->specifyCameraMode()) {
            cameraSection->setSpecifyCameraMode(true);
            cameraSection->cameraMode()->setRawValue(CAMERA_MODE_IMAGE_SURVEY);
        }

        // Point gimbal straight down
        if (_controllerVehicle->firmwarePlugin()->hasGimbal(_controllerVehicle, rollSupported, pitchSupported, yawSupported) && pitchSupported) {
            // If the user already specified a gimbal angle leave it alone
            if (!cameraSection->specifyGimbal()) {
                cameraSection->setSpecifyGimbal(true);
                cameraSection->gimbalPitch()->setRawValue(-90.0);
            }
        }
    }

    complexItem->setSequenceNumber(sequenceNumber);
    complexItem->setWizardMode(true);
    _initVisualItem(complexItem);

    if (visualItemIndex == -1) {
        _visualItems->append(complexItem);
    } else {
        _visualItems->insert(visualItemIndex, complexItem);
    }

    //-- Keep track of bounding box changes in complex items
    if(!complexItem->isSimpleItem()) {
        connect(complexItem, &ComplexMissionItem::boundingCubeChanged, this, &MissionController::_complexBoundingBoxChanged);
    }
    _recalcAllWithCoordinate(mapCenterCoordinate);

    if (makeCurrentItem) {
        setCurrentPlanViewSeqNum(complexItem->sequenceNumber(), true);
    }
}

void MissionController::removeMissionItem(int viIndex)
{
    if (viIndex <= 0 || viIndex >= _visualItems->count()) {
        qWarning() << "MissionController::removeMissionItem called with bad index - count:index" << _visualItems->count() << viIndex;
        return;
    }

    bool removeSurveyStyle = _visualItems->value<SurveyComplexItem*>(viIndex) || _visualItems->value<CorridorScanComplexItem*>(viIndex);
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->removeAt(viIndex));

    _deinitVisualItem(item);
    item->deleteLater();

    if (removeSurveyStyle) {
        // Determine if the mission still has another survey style item in it
        bool foundSurvey = false;
        for (int i=1; i<_visualItems->count(); i++) {
            if (_visualItems->value<SurveyComplexItem*>(i) || _visualItems->value<CorridorScanComplexItem*>(i)) {
                foundSurvey = true;
                break;
            }
        }

        // If there is no longer a survey item in the mission remove added commands
        if (!foundSurvey) {
            bool rollSupported = false;
            bool pitchSupported = false;
            bool yawSupported = false;
            CameraSection* cameraSection = _settingsItem->cameraSection();
            if (_controllerVehicle->firmwarePlugin()->hasGimbal(_controllerVehicle, rollSupported, pitchSupported, yawSupported) && pitchSupported) {
                if (cameraSection->specifyGimbal() && cameraSection->gimbalPitch()->rawValue().toDouble() == -90.0 && cameraSection->gimbalYaw()->rawValue().toDouble() == 0.0) {
                    cameraSection->setSpecifyGimbal(false);
                }
            }
            if (cameraSection->cameraModeSupported() && cameraSection->specifyCameraMode() && cameraSection->cameraMode()->rawValue().toInt() == 0) {
                cameraSection->setSpecifyCameraMode(false);
            }
        }
    }

    _recalcAll();
    setDirty(true);
}

void MissionController::removeAll(void)
{
    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->clearAndDeleteContents();
        _visualItems->deleteLater();
        _settingsItem = nullptr;
        _visualItems = new QmlObjectListModel(this);
        _addMissionSettings(_visualItems);
        _initAllVisualItems();
        setDirty(true);
        _resetMissionFlightStatus();
    }
}

bool MissionController::_loadJsonMissionFileV1(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString)
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
    QList<SurveyComplexItem*> surveyItems;
    QJsonArray complexArray(json[_jsonComplexItemsKey].toArray());
    qCDebug(MissionControllerLog) << "Json load: complex item count" << complexArray.count();
    for (int i=0; i<complexArray.count(); i++) {
        const QJsonValue& itemValue = complexArray[i];

        if (!itemValue.isObject()) {
            errorString = QStringLiteral("Mission item is not an object");
            return false;
        }

        SurveyComplexItem* item = new SurveyComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, visualItems /* parent */);
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

    MissionSettingsItem* settingsItem = _addMissionSettings(visualItems);
    if (json.contains(_jsonPlannedHomePositionKey)) {
        SimpleMissionItem* item = new SimpleMissionItem(_controllerVehicle, _flyView, visualItems);
        if (item->load(json[_jsonPlannedHomePositionKey].toObject(), 0, errorString)) {
            settingsItem->setInitialHomePositionFromUser(item->coordinate());
            item->deleteLater();
        } else {
            return false;
        }
    }

    qCDebug(MissionControllerLog) << "Json load: simple item loop start simpleItemCount:ComplexItemCount" << itemArray.count() << surveyItems.count();
    do {
        qCDebug(MissionControllerLog) << "Json load: simple item loop nextSimpleItemIndex:nextComplexItemIndex:nextSequenceNumber" << nextSimpleItemIndex << nextComplexItemIndex << nextSequenceNumber;

        // If there is a complex item that should be next in sequence add it in
        if (nextComplexItemIndex < surveyItems.count()) {
            SurveyComplexItem* complexItem = surveyItems[nextComplexItemIndex];

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
            SimpleMissionItem* item = new SimpleMissionItem(_controllerVehicle, _flyView, visualItems);
            if (item->load(itemObject, itemObject["id"].toInt(), errorString)) {
                if (TakeoffMissionItem::isTakeoffCommand(item->mavCommand())) {
                    // This needs to be a TakeoffMissionItem
                    TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(_controllerVehicle, _flyView, settingsItem, visualItems);
                    takeoffItem->load(itemObject, itemObject["id"].toInt(), errorString);
                    item->deleteLater();
                    item = takeoffItem;
                }
                qCDebug(MissionControllerLog) << "Json load: adding simple item expectedSequence:actualSequence" << nextSequenceNumber << item->sequenceNumber();
                nextSequenceNumber = item->lastSequenceNumber() + 1;
                visualItems->append(item);
            } else {
                return false;
            }
        }
    } while (nextSimpleItemIndex < itemArray.count() || nextComplexItemIndex < surveyItems.count());

    return true;
}

bool MissionController::_loadJsonMissionFileV2(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString)
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
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();

    if (_masterController->offline()) {
        // We only update if offline since if we are online we use the online vehicle settings
        appSettings->offlineEditingFirmwareType()->setRawValue(AppSettings::offlineEditingFirmwareTypeFromFirmwareType(static_cast<MAV_AUTOPILOT>(json[_jsonFirmwareTypeKey].toInt())));
        if (json.contains(_jsonVehicleTypeKey)) {
            appSettings->offlineEditingVehicleType()->setRawValue(AppSettings::offlineEditingVehicleTypeFromVehicleType(static_cast<MAV_TYPE>(json[_jsonVehicleTypeKey].toInt())));
        }
    }
    if (json.contains(_jsonCruiseSpeedKey)) {
        appSettings->offlineEditingCruiseSpeed()->setRawValue(json[_jsonCruiseSpeedKey].toDouble());
    }
    if (json.contains(_jsonHoverSpeedKey)) {
        appSettings->offlineEditingHoverSpeed()->setRawValue(json[_jsonHoverSpeedKey].toDouble());
    }

    QGeoCoordinate homeCoordinate;
    if (!JsonHelper::loadGeoCoordinate(json[_jsonPlannedHomePositionKey], true /* altitudeRequired */, homeCoordinate, errorString)) {
        return false;
    }
    MissionSettingsItem* settingsItem = new MissionSettingsItem(_controllerVehicle, _flyView, visualItems);
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
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_controllerVehicle, _flyView, visualItems);
            if (simpleItem->load(itemObject, nextSequenceNumber, errorString)) {
                if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(simpleItem->command()))) {
                    // This needs to be a TakeoffMissionItem
                    TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(_controllerVehicle, _flyView, settingsItem, this);
                    takeoffItem->load(itemObject, nextSequenceNumber, errorString);
                    simpleItem->deleteLater();
                    simpleItem = takeoffItem;
                }
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

            if (complexItemType == SurveyComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Survey: nextSequenceNumber" << nextSequenceNumber;
                SurveyComplexItem* surveyItem = new SurveyComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, visualItems);
                if (!surveyItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = surveyItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Survey load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(surveyItem);
            } else if (complexItemType == FixedWingLandingComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Fixed Wing Landing Pattern: nextSequenceNumber" << nextSequenceNumber;
                FixedWingLandingComplexItem* landingItem = new FixedWingLandingComplexItem(_controllerVehicle, _flyView, visualItems);
                if (!landingItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = landingItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "FW Landing Pattern load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(landingItem);
            } else if (complexItemType == StructureScanComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Structure Scan: nextSequenceNumber" << nextSequenceNumber;
                StructureScanComplexItem* structureItem = new StructureScanComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, visualItems);
                if (!structureItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = structureItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Structure Scan load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(structureItem);
            } else if (complexItemType == CorridorScanComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Corridor Scan: nextSequenceNumber" << nextSequenceNumber;
                CorridorScanComplexItem* corridorItem = new CorridorScanComplexItem(_controllerVehicle, _flyView, QString() /* kmlFile */, visualItems);
                if (!corridorItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = corridorItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Corridor Scan load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(corridorItem);
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
            if (doJumpItem->command() == MAV_CMD_DO_JUMP) {
                bool found = false;
                int findDoJumpId = static_cast<int>(doJumpItem->missionItem().param1());
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

bool MissionController::_loadItemsFromJson(const QJsonObject& json, QmlObjectListModel* visualItems, QString& errorString)
{
    // V1 file format has no file type key and version key is string. Convert to new format.
    if (!json.contains(JsonHelper::jsonFileTypeKey)) {
        json[JsonHelper::jsonFileTypeKey] = _jsonFileTypeValue;
    }

    int fileVersion;
    JsonHelper::validateQGCJsonFile(json,
                                    _jsonFileTypeValue,    // expected file type
                                    1,                     // minimum supported version
                                    2,                     // maximum supported version
                                    fileVersion,
                                    errorString);

    if (fileVersion == 1) {
        return _loadJsonMissionFileV1(json, visualItems, errorString);
    } else {
        return _loadJsonMissionFileV2(json, visualItems, errorString);
    }
}

bool MissionController::_loadTextMissionFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString)
{
    bool firstItem = true;
    bool plannedHomePositionInFile = false;

    QString firstLine = stream.readLine();
    const QStringList& version = firstLine.split(" ");

    bool versionOk = false;
    if (version.size() == 3 && version[0] == "QGC" && version[1] == "WPL") {
        if (version[2] == "110") {
            // ArduPilot file, planned home position is already in position 0
            versionOk = true;
            plannedHomePositionInFile = true;
        } else if (version[2] == "120") {
            // Old QGC file, no planned home position
            versionOk = true;
            plannedHomePositionInFile = false;
        }
    }

    if (versionOk) {
        MissionSettingsItem* settingsItem = _addMissionSettings(visualItems);

        while (!stream.atEnd()) {
            SimpleMissionItem* item = new SimpleMissionItem(_controllerVehicle, _flyView, visualItems);
            if (item->load(stream)) {
                if (firstItem && plannedHomePositionInFile) {
                    settingsItem->setInitialHomePositionFromUser(item->coordinate());
                } else {
                    if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(item->command()))) {
                        // This needs to be a TakeoffMissionItem
                        TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(_controllerVehicle, _flyView, settingsItem, visualItems);
                        takeoffItem->load(stream);
                        item->deleteLater();
                        item = takeoffItem;
                    }
                    visualItems->append(item);
                }
                firstItem = false;
            } else {
                errorString = tr("The mission file is corrupted.");
                return false;
            }
        }
    } else {
        errorString = tr("The mission file is not compatible with this version of %1.").arg(qgcApp()->applicationName());
        return false;
    }

    if (!plannedHomePositionInFile) {
        // Update sequence numbers in DO_JUMP commands to take into account added home position in index 0
        for (int i=1; i<visualItems->count(); i++) {
            SimpleMissionItem* item = qobject_cast<SimpleMissionItem*>(visualItems->get(i));
            if (item && item->command() == MAV_CMD_DO_JUMP) {
                item->missionItem().setParam1(static_cast<int>(item->missionItem().param1()) + 1);
            }
        }
    }

    return true;
}

void MissionController::_initLoadedVisualItems(QmlObjectListModel* loadedVisualItems)
{
    if (_visualItems) {
        _deinitAllVisualItems();
        _visualItems->deleteLater();
        _settingsItem = nullptr;
    }

    _visualItems = loadedVisualItems;

    if (_visualItems->count() == 0) {
        _addMissionSettings(_visualItems);
    } else {
        _settingsItem = _visualItems->value<MissionSettingsItem*>(0);
    }

    MissionController::_scanForAdditionalSettings(_visualItems, _controllerVehicle);

    _initAllVisualItems();
}

bool MissionController::load(const QJsonObject& json, QString& errorString)
{
    QString errorStr;
    QString errorMessage = tr("Mission: %1");
    QmlObjectListModel* loadedVisualItems = new QmlObjectListModel(this);

    if (!_loadJsonMissionFileV2(json, loadedVisualItems, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }
    _initLoadedVisualItems(loadedVisualItems);

    return true;
}

bool MissionController::loadJsonFile(QFile& file, QString& errorString)
{
    QString         errorStr;
    QString         errorMessage = tr("Mission: %1");
    QJsonDocument   jsonDoc;
    QByteArray      bytes = file.readAll();

    if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }

    QJsonObject json = jsonDoc.object();
    QmlObjectListModel* loadedVisualItems = new QmlObjectListModel(this);
    if (!_loadItemsFromJson(json, loadedVisualItems, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }

    _initLoadedVisualItems(loadedVisualItems);

    return true;
}

bool MissionController::loadTextFile(QFile& file, QString& errorString)
{
    QString     errorStr;
    QString     errorMessage = tr("Mission: %1");
    QByteArray  bytes = file.readAll();
    QTextStream stream(bytes);

    QmlObjectListModel* loadedVisualItems = new QmlObjectListModel(this);
    if (!_loadTextMissionFile(stream, loadedVisualItems, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }

    _initLoadedVisualItems(loadedVisualItems);

    return true;
}

int MissionController::readyForSaveState(void) const
{
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        if (visualItem->readyForSaveState() != VisualMissionItem::ReadyForSave) {
            return visualItem->readyForSaveState();
        }
    }

    return VisualMissionItem::ReadyForSave;
}

void MissionController::save(QJsonObject& json)
{
    json[JsonHelper::jsonVersionKey] = _missionFileVersion;

    // Mission settings

    MissionSettingsItem* settingsItem = _visualItems->value<MissionSettingsItem*>(0);
    if (!settingsItem) {
        qWarning() << "First item is not MissionSettingsItem";
        return;
    }
    QJsonValue coordinateValue;
    JsonHelper::saveGeoCoordinate(settingsItem->coordinate(), true /* writeAltitude */, coordinateValue);
    json[_jsonPlannedHomePositionKey]   = coordinateValue;
    json[_jsonFirmwareTypeKey]          = _controllerVehicle->firmwareType();
    json[_jsonVehicleTypeKey]           = _controllerVehicle->vehicleType();
    json[_jsonCruiseSpeedKey]           = _controllerVehicle->defaultCruiseSpeed();
    json[_jsonHoverSpeedKey]            = _controllerVehicle->defaultHoverSpeed();

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

    json[_jsonItemsKey] = rgJsonMissionItems;
}

void MissionController::_calcPrevWaypointValues(double homeAlt, VisualMissionItem* currentItem, VisualMissionItem* prevItem, double* azimuth, double* distance, double* altDifference)
{
    QGeoCoordinate  currentCoord =  currentItem->coordinate();
    QGeoCoordinate  prevCoord =     prevItem->exitCoordinate();
    bool            distanceOk =    false;

    // Convert to fixed altitudes

    distanceOk = true;
    if (currentItem != _settingsItem && currentItem->coordinateHasRelativeAltitude()) {
        currentCoord.setAltitude(homeAlt + currentCoord.altitude());
    }
    if (prevItem != _settingsItem && prevItem->exitCoordinateHasRelativeAltitude()) {
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

CoordinateVector* MissionController::_createCoordinateVectorWorker(VisualItemPair& pair)
{
    CoordinateVector* coordVector = nullptr;

    // Create a new segment and wire update notifiers
    coordVector         = new CoordinateVector(pair.first->isSimpleItem() ? pair.first->coordinate() : pair.first->exitCoordinate(), pair.second->coordinate(), this);
    auto originNotifier = pair.first->isSimpleItem() ? &VisualMissionItem::coordinateChanged : &VisualMissionItem::exitCoordinateChanged;
    auto endNotifier    = &VisualMissionItem::coordinateChanged;

    // Use signals/slots to update the coordinate endpoints
    connect(pair.first,     originNotifier, coordVector, &CoordinateVector::setCoordinate1);
    connect(pair.second,    endNotifier,    coordVector, &CoordinateVector::setCoordinate2);

    // FIXME: We should ideally have signals for 2D position change, alt change, and 3D position change
    // Not optimal, but still pretty fast, do a full update of range/bearing/altitudes
    connect(pair.second, &VisualMissionItem::coordinateChanged, this, &MissionController::_recalcMissionFlightStatus);

    return coordVector;
}

CoordinateVector* MissionController::_addWaypointLineSegment(CoordVectHashTable& prevItemPairHashTable, VisualItemPair& pair)
{
    CoordinateVector* coordVector = nullptr;

    if (prevItemPairHashTable.contains(pair)) {
        // Pair already exists and connected, just re-use
        _linesTable[pair] = coordVector = prevItemPairHashTable.take(pair);
    } else {
        coordVector = _createCoordinateVectorWorker(pair);
        _linesTable[pair] = coordVector;
    }

    return coordVector;
}

void MissionController::_recalcROISpecialVisuals(void)
{
    return;
    VisualMissionItem*  lastCoordinateItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));
    bool                roiActive =             false;

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem*  visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem*  simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        VisualItemPair      viPair;

        if (simpleItem) {
            if (roiActive) {
                if (_isROICancelItem(simpleItem)) {
                    roiActive = false;
                }
            } else {
                if (_isROIBeginItem(simpleItem)) {
                    roiActive = true;
                }
            }
        }

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {
            viPair = VisualItemPair(lastCoordinateItem, visualItem);
            if (_linesTable.contains(viPair)) {
                _linesTable[viPair]->setSpecialVisual(roiActive);
            }
            lastCoordinateItem = visualItem;
        }
    }
}

void MissionController::_recalcWaypointLines(void)
{
    VisualItemPair      lastSegmentVisualItemPair;
    int                 segmentCount =                  0;
    bool                firstCoordinateNotFound =       true;
    VisualMissionItem*  lastCoordinateItemBeforeRTL =    qobject_cast<VisualMissionItem*>(_visualItems->get(0));
    bool                linkEndToHome =                 false;
    bool                linkStartToHome =               _managerVehicle->rover() ? true : false;
    bool                foundRTL =                      false;
    bool                homePositionValid =             _settingsItem->coordinate().isValid();
    bool                roiActive =                     false;
    bool                setupIncompleteItem =           false;
    VisualMissionItem*  startVIForIncompleteItem =      nullptr;

    qCDebug(MissionControllerLog) << "_recalcWaypointLines homePositionValid" << homePositionValid;

    CoordVectHashTable old_table = _linesTable;

    _linesTable.clear();
    _waypointPath.clear();

    _waypointLines.beginReset();
    _directionArrows.beginReset();
    _incompleteComplexItemLines.beginReset();

    _waypointLines.clear();
    _directionArrows.clear();
    _incompleteComplexItemLines.clearAndDeleteContents();

    // Grovel through the list of items keeping track of things needed to correctly draw waypoints lines

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem*  visualItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem*  simpleItem =    qobject_cast<SimpleMissionItem*>(visualItem);
        ComplexMissionItem* complexItem =   qobject_cast<ComplexMissionItem*>(visualItem);

        if (simpleItem) {
            if (roiActive) {
                if (_isROICancelItem(simpleItem)) {
                    roiActive = false;
                }
            } else {
                if (_isROIBeginItem(simpleItem)) {
                    roiActive = true;
                }
            }

            MAV_CMD command = simpleItem->mavCommand();
            switch (command) {
            case MAV_CMD_NAV_TAKEOFF:
            case MAV_CMD_NAV_VTOL_TAKEOFF:
                if (!linkEndToHome) {
                    // If we still haven't found the first coordinate item and we hit a takeoff command this means the mission starts from the ground.
                    // Link the first item back to home to show that.
                    if (firstCoordinateNotFound) {
                        linkStartToHome = true;
                    }
                }
                break;
            case MAV_CMD_NAV_RETURN_TO_LAUNCH:
                linkEndToHome = true;
                foundRTL = true;
                break;
            default:
                break;
            }
        }

        // No need to add waypoint segments after an RTL.
        if (foundRTL) {
            break;
        }

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {
            // Incomplete items are complex items which are waiting for the user to complete setup before there visuals can become valid.
            // For example a Survey which has no polygon set for it yet. For these cases we draw incomplete segment lines so that there
            // isn't a hole in the flight path lines.
            if (complexItem && complexItem->isIncomplete()) {
                setupIncompleteItem = true;
            } else {
                if (setupIncompleteItem) {
                    VisualItemPair viPair(startVIForIncompleteItem, visualItem);
                    CoordinateVector* coordVector = _createCoordinateVectorWorker(viPair);

                    _incompleteComplexItemLines.append(coordVector);
                    startVIForIncompleteItem = nullptr;
                    setupIncompleteItem = false;
                } else {
                    startVIForIncompleteItem = visualItem;
                }

                if (lastCoordinateItemBeforeRTL != _settingsItem || (homePositionValid && linkStartToHome)) {
                    // Direction arrows are added to the first segment and every 5 segments in the middle.
                    bool addDirectionArrow = false;
                    if (firstCoordinateNotFound || !lastCoordinateItemBeforeRTL->isSimpleItem() || !visualItem->isSimpleItem()) {
                        addDirectionArrow = true;
                    } else if (segmentCount > 5) {
                        segmentCount = 0;
                        addDirectionArrow = true;
                    }
                    segmentCount++;

                    lastSegmentVisualItemPair =  VisualItemPair(lastCoordinateItemBeforeRTL, visualItem);
                    if (!_flyView || addDirectionArrow) {
                        CoordinateVector* coordVector = _addWaypointLineSegment(old_table, lastSegmentVisualItemPair);
                        coordVector->setSpecialVisual(roiActive);
                        if (addDirectionArrow) {
                            _directionArrows.append(coordVector);
                        }
                    }
                }
                firstCoordinateNotFound = false;
                _waypointPath.append(QVariant::fromValue(visualItem->coordinate()));
                lastCoordinateItemBeforeRTL = visualItem;
            }
        }
    }

    if (linkStartToHome && homePositionValid) {
        _waypointPath.prepend(QVariant::fromValue(_settingsItem->coordinate()));
    }

    if (linkEndToHome && lastCoordinateItemBeforeRTL != _settingsItem && homePositionValid) {
        lastSegmentVisualItemPair =  VisualItemPair(lastCoordinateItemBeforeRTL, _settingsItem);
        if (_flyView) {
            _waypointPath.append(QVariant::fromValue(_settingsItem->coordinate()));
        }
        CoordinateVector* coordVector = _addWaypointLineSegment(old_table, lastSegmentVisualItemPair);
        coordVector->setSpecialVisual(roiActive);
    }

    // Add direction arrow to last segment
    if (lastSegmentVisualItemPair.first) {
        CoordinateVector* coordVector = nullptr;

        // The pair may not be in the hash, this can happen in the fly view where only segments with arrows on them are added to ahsh.
        // check for that first and add if needed

        if (_linesTable.contains(lastSegmentVisualItemPair)) {
            // Pair exists in the new table already just reuse it
            coordVector = _linesTable[lastSegmentVisualItemPair];
        } else if (old_table.contains(lastSegmentVisualItemPair)) {
            // Pair already exists in old table, pull from old to new and reuse
            _linesTable[lastSegmentVisualItemPair] = coordVector = old_table.take(lastSegmentVisualItemPair);
        } else {
            // Create a new segment. Since this is the fly view there is no need to wire change signals.
            coordVector = new CoordinateVector(lastSegmentVisualItemPair.first->isSimpleItem() ? lastSegmentVisualItemPair.first->coordinate() : lastSegmentVisualItemPair.first->exitCoordinate(), lastSegmentVisualItemPair.second->coordinate(), this);
            _linesTable[lastSegmentVisualItemPair] = coordVector;
        }

        _directionArrows.append(coordVector);
    }

    if (!_flyView) {
        // Create a temporary QObjectList and replace the model data
        QObjectList objs;
        objs.reserve(_linesTable.count());
        for(CoordinateVector *vect: _linesTable.values()) {
            objs.append(vect);
        }

        // We don't delete here because many links may still be valid
        _waypointLines.swapObjectList(objs);
    }

    _waypointLines.endReset();
    _directionArrows.endReset();
    _incompleteComplexItemLines.endReset();

    // Anything left in the old table is an obsolete line object that can go
    qDeleteAll(old_table);

    _recalcMissionFlightStatus();

    if (_waypointPath.count() == 0) {
        // MapPolyLine has a bug where if you change from a path which has elements to an empty path the line drawn
        // is not cleared from the map. This hack works around that since it causes the previous lines to be remove
        // as then doesn't draw anything on the map.
        _waypointPath.append(QVariant::fromValue(QGeoCoordinate(0, 0)));
        _waypointPath.append(QVariant::fromValue(QGeoCoordinate(0, 0)));
    }

    emit waypointPathChanged();
}

void MissionController::_updateBatteryInfo(int waypointIndex)
{
    if (_missionFlightStatus.mAhBattery != 0) {
        _missionFlightStatus.hoverAmpsTotal = (_missionFlightStatus.hoverTime / 60.0) * _missionFlightStatus.hoverAmps;
        _missionFlightStatus.cruiseAmpsTotal = (_missionFlightStatus.cruiseTime / 60.0) * _missionFlightStatus.cruiseAmps;
        _missionFlightStatus.batteriesRequired = ceil((_missionFlightStatus.hoverAmpsTotal + _missionFlightStatus.cruiseAmpsTotal) / _missionFlightStatus.ampMinutesAvailable);
        // FIXME: Battery change point code pretty much doesn't work. The reason is that is treats complex items as a black box. It needs to be able to look
        // inside complex items in order to determine a swap point that is interior to a complex item. Current the swap point display in PlanToolbar is
        // disabled to do this problem.
        if (waypointIndex != -1 && _missionFlightStatus.batteriesRequired == 2 && _missionFlightStatus.batteryChangePoint == -1) {
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

/// Adds the specified time to the appropriate hover or cruise time values.
///     @param vtolInHover true: vtol is currrent in hover mode
///     @param hoverTime    Amount of time tp add to hover
///     @param cruiseTime   Amount of time to add to cruise
///     @param extraTime    Amount of additional time to add to hover/cruise
///     @param seqNum       Sequence number of waypoint for these values, -1 for no waypoint associated
void MissionController::_addTimeDistance(bool vtolInHover, double hoverTime, double cruiseTime, double extraTime, double distance, int seqNum)
{
    if (_controllerVehicle->vtol()) {
        if (vtolInHover) {
            _addHoverTime(hoverTime, distance, seqNum);
            _addHoverTime(extraTime, 0, -1);
        } else {
            _addCruiseTime(cruiseTime, distance, seqNum);
            _addCruiseTime(extraTime, 0, -1);
        }
    } else {
        if (_controllerVehicle->multiRotor()) {
            _addHoverTime(hoverTime, distance, seqNum);
            _addHoverTime(extraTime, 0, -1);
        } else {
            _addCruiseTime(cruiseTime, distance, seqNum);
            _addCruiseTime(extraTime, 0, -1);
        }
    }
}

void MissionController::_recalcMissionFlightStatus()
{
    if (!_visualItems->count()) {
        return;
    }

    bool                firstCoordinateItem =           true;
    VisualMissionItem*  lastCoordinateItemBeforeRTL =   qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    bool homePositionValid = _settingsItem->coordinate().isValid();

    qCDebug(MissionControllerLog) << "_recalcMissionFlightStatus";

    // If home position is valid we can calculate distances between all waypoints.
    // If home position is not valid we can only calculate distances between waypoints which are
    // both relative altitude.

    // No values for first item
    lastCoordinateItemBeforeRTL->setAltDifference(0.0);
    lastCoordinateItemBeforeRTL->setAzimuth(0.0);
    lastCoordinateItemBeforeRTL->setDistance(0.0);

    double minAltSeen = 0.0;
    double maxAltSeen = 0.0;
    const double homePositionAltitude = _settingsItem->coordinate().altitude();
    minAltSeen = maxAltSeen = _settingsItem->coordinate().altitude();

    _resetMissionFlightStatus();

    bool vtolInHover = true;
    bool linkStartToHome = false;
    bool foundRTL = false;
    bool vehicleYawSpecificallySet = false;

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem*  item =          qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem*  simpleItem =    qobject_cast<SimpleMissionItem*>(item);
        ComplexMissionItem* complexItem =   qobject_cast<ComplexMissionItem*>(item);

        if (simpleItem && simpleItem->mavCommand() == MAV_CMD_NAV_RETURN_TO_LAUNCH) {
            foundRTL = true;
        }

        // Assume the worst
        item->setAzimuth(0.0);
        item->setDistance(0.0);

        // Look for speed changed
        double newSpeed = item->specifiedFlightSpeed();
        if (!qIsNaN(newSpeed)) {
            if (_controllerVehicle->multiRotor()) {
                _missionFlightStatus.hoverSpeed = newSpeed;
            } else if (_controllerVehicle->vtol()) {
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

        // ROI commands cancel out previous gimbal yaw/pitch
        if (simpleItem) {
            switch (simpleItem->command()) {
            case MAV_CMD_NAV_ROI:
            case MAV_CMD_DO_SET_ROI_LOCATION:
            case MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET:
                _missionFlightStatus.gimbalYaw = std::numeric_limits<double>::quiet_NaN();
                _missionFlightStatus.gimbalPitch = std::numeric_limits<double>::quiet_NaN();
                break;
            default:
                break;
            }
        }

        // Look for specific gimbal changes
        double gimbalYaw = item->specifiedGimbalYaw();
        if (!qIsNaN(gimbalYaw)) {
            _missionFlightStatus.gimbalYaw = gimbalYaw;
        }
        double gimbalPitch = item->specifiedGimbalPitch();
        if (!qIsNaN(gimbalPitch)) {
            _missionFlightStatus.gimbalPitch = gimbalPitch;
        }

        if (i == 0) {
            // We only process speed and gimbal from Mission Settings item
            continue;
        }

        if (foundRTL) {
            // No more vehicle distances after RTL
            continue;
        }

        // Link back to home if first item is takeoff and we have home position
        if (firstCoordinateItem && simpleItem && (simpleItem->mavCommand() == MAV_CMD_NAV_TAKEOFF || simpleItem->mavCommand() == MAV_CMD_NAV_VTOL_TAKEOFF)) {
            if (homePositionValid) {
                linkStartToHome = true;
                if (_controllerVehicle->multiRotor() || _controllerVehicle->vtol()) {
                    // We have to special case takeoff, assuming vehicle takes off straight up to specified altitude
                    double azimuth, distance, altDifference;
                    _calcPrevWaypointValues(homePositionAltitude, _settingsItem, simpleItem, &azimuth, &distance, &altDifference);
                    double takeoffTime = qAbs(altDifference) / _appSettings->offlineEditingAscentSpeed()->rawValue().toDouble();
                    _addHoverTime(takeoffTime, 0, -1);
                }
            }
        }

        // Update VTOL state
        if (simpleItem && _controllerVehicle->vtol()) {
            switch (simpleItem->command()) {
            case MAV_CMD_NAV_TAKEOFF:
                vtolInHover = false;
                break;
            case MAV_CMD_NAV_VTOL_TAKEOFF:
                vtolInHover = true;
                break;
            case MAV_CMD_NAV_LAND:
                vtolInHover = false;
                break;
            case MAV_CMD_NAV_VTOL_LAND:
                vtolInHover = true;
                break;
            case MAV_CMD_DO_VTOL_TRANSITION:
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

        _addTimeDistance(vtolInHover, 0, 0, item->additionalTimeDelay(), 0, -1);

        if (item->specifiesCoordinate()) {
            // Keep track of the min/max altitude for all waypoints so we can show altitudes as a percentage

            double absoluteAltitude = item->coordinate().altitude();
            if (item->coordinateHasRelativeAltitude()) {
                absoluteAltitude += homePositionAltitude;
            }
            minAltSeen = std::min(minAltSeen, absoluteAltitude);
            maxAltSeen = std::max(maxAltSeen, absoluteAltitude);

            double terrainAltitude = item->terrainAltitude();
            if (!qIsNaN(terrainAltitude)) {
                minAltSeen = std::min(minAltSeen, terrainAltitude);
                maxAltSeen = std::max(maxAltSeen, terrainAltitude);
            }

            if (!item->isStandaloneCoordinate()) {
                firstCoordinateItem = false;

                // Update vehicle yaw assuming direction to next waypoint and/or mission item change
                if (item != lastCoordinateItemBeforeRTL) {
                    if (simpleItem && !qIsNaN(simpleItem->specifiedVehicleYaw())) {
                        vehicleYawSpecificallySet = true;
                        _missionFlightStatus.vehicleYaw = simpleItem->specifiedVehicleYaw();
                    } else if (!vehicleYawSpecificallySet) {
                        _missionFlightStatus.vehicleYaw = lastCoordinateItemBeforeRTL->exitCoordinate().azimuthTo(item->coordinate());
                    }
                    lastCoordinateItemBeforeRTL->setMissionVehicleYaw(_missionFlightStatus.vehicleYaw);
                }

                if (lastCoordinateItemBeforeRTL != _settingsItem || linkStartToHome) {
                    // This is a subsequent waypoint or we are forcing the first waypoint back to home
                    double azimuth, distance, altDifference;

                    _calcPrevWaypointValues(homePositionAltitude, item, lastCoordinateItemBeforeRTL, &azimuth, &distance, &altDifference);
                    item->setAltDifference(altDifference);
                    item->setAzimuth(azimuth);
                    item->setDistance(distance);

                    _missionFlightStatus.maxTelemetryDistance = qMax(_missionFlightStatus.maxTelemetryDistance, _calcDistanceToHome(item, _settingsItem));

                    // Calculate time/distance
                    double hoverTime = distance / _missionFlightStatus.hoverSpeed;
                    double cruiseTime = distance / _missionFlightStatus.cruiseSpeed;
                    _addTimeDistance(vtolInHover, hoverTime, cruiseTime, 0, distance, item->sequenceNumber());
                }

                if (complexItem) {
                    // Add in distance/time inside complex items as well
                    double distance = complexItem->complexDistance();
                    _missionFlightStatus.maxTelemetryDistance = qMax(_missionFlightStatus.maxTelemetryDistance, complexItem->greatestDistanceTo(complexItem->exitCoordinate()));

                    double hoverTime = distance / _missionFlightStatus.hoverSpeed;
                    double cruiseTime = distance / _missionFlightStatus.cruiseSpeed;
                    _addTimeDistance(vtolInHover, hoverTime, cruiseTime, 0, distance, item->sequenceNumber());
                }

                item->setMissionFlightStatus(_missionFlightStatus);

                lastCoordinateItemBeforeRTL = item;
            }
        }
    }
    lastCoordinateItemBeforeRTL->setMissionVehicleYaw(_missionFlightStatus.vehicleYaw);

    // Add the information for the final segment back to home
    if (foundRTL && lastCoordinateItemBeforeRTL != _settingsItem && homePositionValid) {
        double azimuth, distance, altDifference;
        _calcPrevWaypointValues(homePositionAltitude, lastCoordinateItemBeforeRTL, _settingsItem, &azimuth, &distance, &altDifference);

        // Calculate time/distance
        double hoverTime = distance / _missionFlightStatus.hoverSpeed;
        double cruiseTime = distance / _missionFlightStatus.cruiseSpeed;
        double landTime = qAbs(altDifference) / _appSettings->offlineEditingDescentSpeed()->rawValue().toDouble();
        _addTimeDistance(vtolInHover, hoverTime, cruiseTime, distance, landTime, -1);
    }

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
                item->setTerrainPercent(qQNaN());
                item->setTerrainCollision(false);
            } else {
                item->setAltPercent((absoluteAltitude - minAltSeen) / altRange);
                double terrainAltitude = item->terrainAltitude();
                if (qIsNaN(terrainAltitude)) {
                    item->setTerrainPercent(qQNaN());
                    item->setTerrainCollision(false);
                } else {
                    item->setTerrainPercent((terrainAltitude - minAltSeen) / altRange);
                    item->setTerrainCollision(absoluteAltitude < terrainAltitude);
                }
            }
        }
    }

    _updateTimer.start(UPDATE_TIMEOUT);
}

// This will update the sequence numbers to be sequential starting from 0
void MissionController::_recalcSequence(void)
{
    if (_inRecalcSequence) {
        // Don't let this call recurse due to signalling
        return;
    }

    // Setup ascending sequence numbers for all visual items

    _inRecalcSequence = true;
    int sequenceNumber = 0;
    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        item->setSequenceNumber(sequenceNumber);
        sequenceNumber = item->lastSequenceNumber() + 1;
    }
    _inRecalcSequence = false;
}

// This will update the child item hierarchy
void MissionController::_recalcChildItems(void)
{
    VisualMissionItem* currentParentItem = qobject_cast<VisualMissionItem*>(_visualItems->get(0));

    currentParentItem->childItems()->clear();

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = _visualItems->value<VisualMissionItem*>(i);

        item->setParentItem(nullptr);
        item->setHasCurrentChildItem(false);

        // Set up non-coordinate item child hierarchy
        if (item->specifiesCoordinate()) {
            item->childItems()->clear();
            currentParentItem = item;
        } else if (item->isSimpleItem()) {
            item->setParentItem(currentParentItem);
            currentParentItem->childItems()->append(item);
            if (item->isCurrentItem()) {
                currentParentItem->setHasCurrentChildItem(true);
            }
        }
    }
}

void MissionController::_setPlannedHomePositionFromFirstCoordinate(const QGeoCoordinate& clickCoordinate)
{
    bool foundFirstCoordinate = false;
    QGeoCoordinate firstCoordinate;

    if (_settingsItem->coordinate().isValid()) {
        return;
    }

    // Set the planned home position to be a delta from first coordinate
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = _visualItems->value<VisualMissionItem*>(i);

        if (item->specifiesCoordinate() && item->coordinate().isValid()) {
            foundFirstCoordinate = true;
            firstCoordinate = item->coordinate();
            break;
        }
    }

    // No item specifying a coordinate was found, in this case it we have a clickCoordinate use that
    if (!foundFirstCoordinate) {
        firstCoordinate = clickCoordinate;
    }

    if (firstCoordinate.isValid()) {
        QGeoCoordinate plannedHomeCoord = firstCoordinate.atDistanceAndAzimuth(30, 0);
        plannedHomeCoord.setAltitude(0);
        _settingsItem->setInitialHomePositionFromUser(plannedHomeCoord);
    }
}

void MissionController::_recalcAllWithCoordinate(const QGeoCoordinate& coordinate)
{
    if (!_flyView) {
        _setPlannedHomePositionFromFirstCoordinate(coordinate);
    }
    _recalcSequence();
    _recalcChildItems();
    _recalcWaypointLines();
    _updateTimer.start(UPDATE_TIMEOUT);
}

void MissionController::_recalcAll(void)
{
    QGeoCoordinate emptyCoord;
    _recalcAllWithCoordinate(emptyCoord);
}

/// Initializes a new set of mission items
void MissionController::_initAllVisualItems(void)
{
    // Setup home position at index 0

    if (!_settingsItem) {
        _settingsItem = qobject_cast<MissionSettingsItem*>(_visualItems->get(0));
        if (!_settingsItem) {
            qWarning() << "First item not MissionSettingsItem";
            return;
        }
    }

    connect(_settingsItem, &MissionSettingsItem::coordinateChanged,     this, &MissionController::_recalcAll);
    connect(_settingsItem, &MissionSettingsItem::coordinateChanged,     this, &MissionController::plannedHomePositionChanged);

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        _initVisualItem(item);
    }

    _recalcAll();

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_visualItemsDirtyChanged);
    connect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::_updateContainsItems);

    emit visualItemsChanged();
    emit containsItemsChanged(containsItems());
    emit plannedHomePositionChanged(plannedHomePosition());

    if (!_flyView) {
        setCurrentPlanViewSeqNum(0, true);
    }

    setDirty(false);
}

void MissionController::_deinitAllVisualItems(void)
{
    disconnect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &MissionController::_recalcAll);
    disconnect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &MissionController::plannedHomePositionChanged);

    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    disconnect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::dirtyChanged);
    disconnect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::_updateContainsItems);
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    setDirty(false);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::coordinateHasRelativeAltitudeChanged,       this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::exitCoordinateHasRelativeAltitudeChanged,   this, &MissionController::_recalcWaypointLines);
    connect(visualItem, &VisualMissionItem::specifiedFlightSpeedChanged,                this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::specifiedGimbalYawChanged,                  this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::specifiedGimbalPitchChanged,                this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::specifiedVehicleYawChanged,                 this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::terrainAltitudeChanged,                     this, &MissionController::_recalcMissionFlightStatus);
    connect(visualItem, &VisualMissionItem::additionalTimeDelayChanged,                 this, &MissionController::_recalcMissionFlightStatus);
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
            connect(complexItem, &ComplexMissionItem::complexDistanceChanged,       this, &MissionController::_recalcMissionFlightStatus);
            connect(complexItem, &ComplexMissionItem::greatestDistanceToChanged,    this, &MissionController::_recalcMissionFlightStatus);
            connect(complexItem, &ComplexMissionItem::isIncompleteChanged,          this, &MissionController::_recalcWaypointLines);
        } else {
            qWarning() << "ComplexMissionItem not found";
        }
    }
}

void MissionController::_deinitVisualItem(VisualMissionItem* visualItem)
{
    // Disconnect all signals
    disconnect(visualItem, nullptr, nullptr, nullptr);
}

void MissionController::_itemCommandChanged(void)
{
    _recalcChildItems();
    _recalcWaypointLines();
}

void MissionController::managerVehicleChanged(Vehicle* managerVehicle)
{
    if (_managerVehicle) {
        _missionManager->disconnect(this);
        _managerVehicle->disconnect(this);
        _managerVehicle = nullptr;
        _missionManager = nullptr;
    }

    _managerVehicle = managerVehicle;
    if (!_managerVehicle) {
        qWarning() << "MissionController::managerVehicleChanged managerVehicle=NULL";
        return;
    }

    _missionManager = _managerVehicle->missionManager();
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &MissionController::_newMissionItemsAvailableFromVehicle);
    connect(_missionManager, &MissionManager::sendComplete,             this, &MissionController::_managerSendComplete);
    connect(_missionManager, &MissionManager::removeAllComplete,        this, &MissionController::_managerRemoveAllComplete);
    connect(_missionManager, &MissionManager::inProgressChanged,        this, &MissionController::_inProgressChanged);
    connect(_missionManager, &MissionManager::progressPct,              this, &MissionController::_progressPctChanged);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &MissionController::_currentMissionIndexChanged);
    connect(_missionManager, &MissionManager::lastCurrentIndexChanged,  this, &MissionController::resumeMissionIndexChanged);
    connect(_missionManager, &MissionManager::resumeMissionReady,       this, &MissionController::resumeMissionReady);
    connect(_missionManager, &MissionManager::resumeMissionUploadFail,  this, &MissionController::resumeMissionUploadFail);
    connect(_managerVehicle, &Vehicle::defaultCruiseSpeedChanged,       this, &MissionController::_recalcMissionFlightStatus);
    connect(_managerVehicle, &Vehicle::defaultHoverSpeedChanged,        this, &MissionController::_recalcMissionFlightStatus);
    connect(_managerVehicle, &Vehicle::vehicleTypeChanged,              this, &MissionController::complexMissionItemNamesChanged);

    emit complexMissionItemNamesChanged();
    emit resumeMissionIndexChanged();
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
}

bool MissionController::_findPreviousAltitude(int newIndex, double* prevAltitude, int* prevAltitudeMode)
{
    bool        found = false;
    double      foundAltitude = 0;
    int         foundAltitudeMode = QGroundControlQmlGlobal::AltitudeModeNone;

    if (newIndex > _visualItems->count()) {
        return false;
    }
    newIndex--;

    for (int i=newIndex; i>0; i--) {
        VisualMissionItem* visualItem = qobject_cast<VisualMissionItem*>(_visualItems->get(i));

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {
            if (visualItem->isSimpleItem()) {
                SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
                if (simpleItem->specifiesAltitude()) {
                    foundAltitude = simpleItem->altitude()->rawValue().toDouble();
                    foundAltitudeMode = simpleItem->altitudeMode();
                    found = true;
                    break;
                }
            }
        }
    }

    if (found) {
        *prevAltitude = foundAltitude;
        *prevAltitudeMode = foundAltitudeMode;
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
MissionSettingsItem* MissionController::_addMissionSettings(QmlObjectListModel* visualItems)
{
    qCDebug(MissionControllerLog) << "_addMissionSettings";

    MissionSettingsItem* settingsItem = new MissionSettingsItem(_managerVehicle, _flyView, visualItems);
    visualItems->insert(0, settingsItem);

    if (visualItems == _visualItems) {
        _settingsItem = settingsItem;
    }

    return settingsItem;
}

void MissionController::_centerHomePositionOnMissionItems(QmlObjectListModel *visualItems)
{
    qCDebug(MissionControllerLog) << "_centerHomePositionOnMissionItems";

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
            _settingsItem->setInitialHomePositionFromUser(QGeoCoordinate((south + ((north - south) / 2)) - 90.0, (west + ((east - west) / 2)) - 180.0, 0.0));
        }
    }
}

int MissionController::resumeMissionIndex(void) const
{

    int resumeIndex = 0;

    if (_flyView) {
        resumeIndex = _missionManager->lastCurrentIndex() + (_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle() ? 0 : 1);
        if (resumeIndex > 1 && resumeIndex != _visualItems->value<VisualMissionItem*>(_visualItems->count() - 1)->sequenceNumber()) {
            // Resume at the item previous to the item we were heading towards
            resumeIndex--;
        } else {
            resumeIndex = 0;
        }
    }

    return resumeIndex;
}

int MissionController::currentMissionIndex(void) const
{
    if (!_flyView) {
        return -1;
    } else {
        int currentIndex = _missionManager->currentIndex();
        if (!_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            currentIndex++;
        }
        return currentIndex;
    }
}

void MissionController::_currentMissionIndexChanged(int sequenceNumber)
{
    if (_flyView) {
        if (!_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            sequenceNumber++;
        }

        for (int i=0; i<_visualItems->count(); i++) {
            VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            item->setIsCurrentItem(item->sequenceNumber() == sequenceNumber);
        }
        emit currentMissionIndexChanged(currentMissionIndex());
    }
}

bool MissionController::syncInProgress(void) const
{
    return _missionManager->inProgress();
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

void MissionController::_scanForAdditionalSettings(QmlObjectListModel* visualItems, Vehicle* vehicle)
{
    // First we look for a Fixed Wing Landing Pattern which is at the end
    FixedWingLandingComplexItem::scanForItem(visualItems, _flyView, vehicle);

    int scanIndex = 0;
    while (scanIndex < visualItems->count()) {
        VisualMissionItem* visualItem = visualItems->value<VisualMissionItem*>(scanIndex);

        qCDebug(MissionControllerLog) << "MissionController::_scanForAdditionalSettings count:scanIndex" << visualItems->count() << scanIndex;

        if (!_flyView) {
            MissionSettingsItem* settingsItem = qobject_cast<MissionSettingsItem*>(visualItem);
            if (settingsItem) {
                scanIndex++;
                settingsItem->scanForMissionSettings(visualItems, scanIndex);
                continue;
            }
        }

        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
        if (simpleItem) {
            scanIndex++;
            simpleItem->scanForSections(visualItems, scanIndex, vehicle);
        } else {
            // Complex item, can't have sections
            scanIndex++;
        }
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
    if (_masterController->offline()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::removeAllFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCWarning(MissionControllerLog) << "MissionControllerLog::removeAllFromVehicle called while syncInProgress";
    } else {
        _itemsRequested = true;
        _missionManager->removeAll();
    }
}

QStringList MissionController::complexMissionItemNames(void) const
{
    QStringList complexItems;

    complexItems.append(patternSurveyName);
    complexItems.append(patternCorridorScanName);
    if (_controllerVehicle->multiRotor() || _controllerVehicle->vtol()) {
        complexItems.append(patternStructureScanName);
    }

    return qgcApp()->toolbox()->corePlugin()->complexMissionItemNames(_controllerVehicle, complexItems);
}

void MissionController::resumeMission(int resumeIndex)
{
    if (!_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
        resumeIndex--;
    }
    _missionManager->generateResumeMission(resumeIndex);
}

QGeoCoordinate MissionController::plannedHomePosition(void) const
{
    if (_settingsItem) {
        return _settingsItem->coordinate();
    } else {
        return QGeoCoordinate();
    }
}

void MissionController::applyDefaultMissionAltitude(void)
{
    double defaultAltitude = _appSettings->defaultMissionItemAltitude()->rawValue().toDouble();

    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem* item = _visualItems->value<VisualMissionItem*>(i);
        item->applyNewAltitude(defaultAltitude);
    }
}

void MissionController::_progressPctChanged(double progressPct)
{
    if (!qFuzzyCompare(progressPct, _progressPct)) {
        _progressPct = progressPct;
        emit progressPctChanged(progressPct);
    }
}

void MissionController::_visualItemsDirtyChanged(bool dirty)
{
    // We could connect signal to signal and not need this but this is handy for setting a breakpoint on
    emit dirtyChanged(dirty);
}

bool MissionController::showPlanFromManagerVehicle (void)
{
    qCDebug(MissionControllerLog) << "showPlanFromManagerVehicle _flyView" << _flyView;
    if (_masterController->offline()) {
        qCWarning(MissionControllerLog) << "MissionController::showPlanFromManagerVehicle called while offline";
        return true;    // stops further propagation of showPlanFromManagerVehicle due to error
    } else {
        if (!_managerVehicle->initialPlanRequestComplete()) {
            // The vehicle hasn't completed initial load, we can just wait for newMissionItemsAvailable to be signalled automatically
            qCDebug(MissionControllerLog) << "showPlanFromManagerVehicle: !initialPlanRequestComplete, wait for signal";
            return true;
        } else if (syncInProgress()) {
            // If the sync is already in progress, newMissionItemsAvailable will be signalled automatically when it is done. So no need to do anything.
            qCDebug(MissionControllerLog) << "showPlanFromManagerVehicle: syncInProgress wait for signal";
            return true;
        } else {
            // Fake a _newMissionItemsAvailable with the current items
            qCDebug(MissionControllerLog) << "showPlanFromManagerVehicle: sync complete simulate signal";
            _itemsRequested = true;
            _newMissionItemsAvailableFromVehicle(false /* removeAllRequested */);
            return false;
        }
    }
}

void MissionController::_managerSendComplete(bool error)
{
    // Fly view always reloads on send complete
    if (!error && _flyView) {
        showPlanFromManagerVehicle();
    }
}

void MissionController::_managerRemoveAllComplete(bool error)
{
    if (!error) {
        // Remove all from vehicle so we always update
        showPlanFromManagerVehicle();
    }
}

bool MissionController::_isROIBeginItem(SimpleMissionItem* simpleItem)
{
    return simpleItem->mavCommand() == MAV_CMD_DO_SET_ROI_LOCATION ||
            simpleItem->mavCommand() == MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET ||
            (simpleItem->mavCommand() == MAV_CMD_DO_SET_ROI &&
             static_cast<int>(simpleItem->missionItem().param1()) == MAV_ROI_LOCATION);
}

bool MissionController::_isROICancelItem(SimpleMissionItem* simpleItem)
{
    return simpleItem->mavCommand() == MAV_CMD_DO_SET_ROI_NONE ||
            (simpleItem->mavCommand() == MAV_CMD_DO_SET_ROI &&
             static_cast<int>(simpleItem->missionItem().param1()) == MAV_ROI_NONE);
}

void MissionController::setCurrentPlanViewSeqNum(int sequenceNumber, bool force)
{
    if (_visualItems && (force || sequenceNumber != _currentPlanViewSeqNum)) {
        bool    foundLand =         false;
        int     takeoffIndex =      -1;
        int     landIndex =         -1;

        _splitSegment =                 nullptr;
        _currentPlanViewItem  =         nullptr;
        _currentPlanViewSeqNum =        -1;
        _currentPlanViewVIIndex =       -1;
        _isInsertTakeoffValid =         true;
        _isInsertLandValid =            true;
        _isROIActive =                  false;
        _isROIBeginCurrentItem =        false;
        _flyThroughCommandsAllowed =    true;
        _previousCoordinate =           QGeoCoordinate();

        for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
            VisualMissionItem* pVI =        qobject_cast<VisualMissionItem*>(_visualItems->get(viIndex));
            SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(pVI);

            if (sequenceNumber != 0 && pVI->sequenceNumber() <= sequenceNumber) {
                if (pVI->specifiesCoordinate() && !pVI->isStandaloneCoordinate()) {
                    // Coordinate based flight commands prior to where the takeoff would be inserted
                    _isInsertTakeoffValid = false;
                }
            }

            if (qobject_cast<TakeoffMissionItem*>(pVI)) {
                takeoffIndex = viIndex;
                _isInsertTakeoffValid = false;
            }

            if (!foundLand) {
                if (simpleItem) {
                    switch (simpleItem->mavCommand()) {
                    case MAV_CMD_NAV_LAND:
                    case MAV_CMD_NAV_VTOL_LAND:
                    case MAV_CMD_DO_LAND_START:
                    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
                        foundLand = true;
                        landIndex = viIndex;
                        break;
                    default:
                        break;
                    }
                } else {
                    FixedWingLandingComplexItem* fwLanding = qobject_cast<FixedWingLandingComplexItem*>(pVI);
                    if (fwLanding) {
                        foundLand = true;
                        landIndex = viIndex;
                    }
                }
            }

            if (simpleItem) {
                // Remember previous coordinate
                if (pVI->sequenceNumber() < sequenceNumber && simpleItem->specifiesCoordinate() && !simpleItem->isStandaloneCoordinate()) {
                    _previousCoordinate = simpleItem->coordinate();
                }

                // ROI state handling
                if (pVI->sequenceNumber() <= sequenceNumber) {
                    if (_isROIActive) {
                        if (_isROICancelItem(simpleItem)) {
                            _isROIActive = false;
                        }
                    } else {
                        if (_isROIBeginItem(simpleItem)) {
                            _isROIActive = true;
                        }
                    }
                }
                if (pVI->sequenceNumber() == sequenceNumber && _isROIBeginItem(simpleItem)) {
                    _isROIBeginCurrentItem = true;
                }
            }

            if (pVI->sequenceNumber() == sequenceNumber) {
                pVI->setIsCurrentItem(true);
                pVI->setHasCurrentChildItem(false);

                _currentPlanViewItem  = pVI;
                _currentPlanViewSeqNum = sequenceNumber;
                _currentPlanViewVIIndex = viIndex;

                if (pVI->specifiesCoordinate()) {
                    if (!pVI->isStandaloneCoordinate()) {
                        // Determine split segment used to display line split editing ui.
                        for (int j=viIndex-1; j>0; j--) {
                            VisualMissionItem* pPrev = qobject_cast<VisualMissionItem*>(_visualItems->get(j));
                            if (pPrev->specifiesCoordinate() && !pPrev->isStandaloneCoordinate()) {
                                VisualItemPair splitPair(pPrev, pVI);
                                if (_linesTable.contains(splitPair)) {
                                    _splitSegment = _linesTable[splitPair];
                                }
                            }
                        }
                    }
                } else if (pVI->parentItem()) {
                    pVI->parentItem()->setHasCurrentChildItem(true);
                }
            } else {
                pVI->setIsCurrentItem(false);
            }
        }

        if (takeoffIndex != -1) {
            // Takeoff item was found which means mission starts from ground
            if (sequenceNumber < takeoffIndex) {
                // Land is only valid after the takeoff item.
                _isInsertLandValid = false;
                // Fly through commands are not allowed prior to the takeoff command
                _flyThroughCommandsAllowed = false;
            }
        }

        if (foundLand) {
            // Can't have more than one land sequence
            _isInsertLandValid = false;
            if (sequenceNumber >= landIndex) {
                // Can't have fly through commands after a land item
                _flyThroughCommandsAllowed = false;
            }
        }

        emit currentPlanViewSeqNumChanged();
        emit currentPlanViewVIIndexChanged();
        emit currentPlanViewItemChanged();
        emit splitSegmentChanged();
        emit isInsertTakeoffValidChanged();
        emit isInsertLandValidChanged();
        emit isROIActiveChanged();
        emit isROIBeginCurrentItemChanged();
        emit flyThroughCommandsAllowedChanged();
        emit previousCoordinateChanged();
    }
}

void MissionController::_updateTimeout()
{
    QGeoCoordinate firstCoordinate;
    QGeoCoordinate takeoffCoordinate;
    QGCGeoBoundingCube boundingCube;
    double north = 0.0;
    double south = 180.0;
    double east  = 0.0;
    double west  = 360.0;
    double minAlt = QGCGeoBoundingCube::MaxAlt;
    double maxAlt = QGCGeoBoundingCube::MinAlt;
    for (int i = 1; i < _visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        if(item->isSimpleItem()) {
            SimpleMissionItem* pSimpleItem = qobject_cast<SimpleMissionItem*>(item);
            if(pSimpleItem) {
                switch(pSimpleItem->command()) {
                case MAV_CMD_NAV_TAKEOFF:
                case MAV_CMD_NAV_WAYPOINT:
                case MAV_CMD_NAV_LAND:
                    if(pSimpleItem->coordinate().isValid()) {
                        if((MAV_CMD)pSimpleItem->command() == MAV_CMD_NAV_TAKEOFF) {
                            takeoffCoordinate = pSimpleItem->coordinate();
                        } else if(!firstCoordinate.isValid()) {
                            firstCoordinate = pSimpleItem->coordinate();
                        }
                        double lat = pSimpleItem->coordinate().latitude()  + 90.0;
                        double lon = pSimpleItem->coordinate().longitude() + 180.0;
                        double alt = pSimpleItem->coordinate().altitude();
                        north  = fmax(north, lat);
                        south  = fmin(south, lat);
                        east   = fmax(east,  lon);
                        west   = fmin(west,  lon);
                        minAlt = fmin(minAlt, alt);
                        maxAlt = fmax(maxAlt, alt);
                    }
                    break;
                default:
                    break;
                }
            }
        } else {
            ComplexMissionItem* pComplexItem = qobject_cast<ComplexMissionItem*>(item);
            if(pComplexItem) {
                QGCGeoBoundingCube bc = pComplexItem->boundingCube();
                if(bc.isValid()) {
                    if(!firstCoordinate.isValid() && pComplexItem->coordinate().isValid()) {
                        firstCoordinate = pComplexItem->coordinate();
                    }
                    north  = fmax(north, bc.pointNW.latitude()  + 90.0);
                    south  = fmin(south, bc.pointSE.latitude()  + 90.0);
                    east   = fmax(east,  bc.pointNW.longitude() + 180.0);
                    west   = fmin(west,  bc.pointSE.longitude() + 180.0);
                    minAlt = fmin(minAlt, bc.pointNW.altitude());
                    maxAlt = fmax(maxAlt, bc.pointSE.altitude());
                }
            }
        }
    }
    //-- Figure out where this thing is taking off from
    if(!takeoffCoordinate.isValid()) {
        if(firstCoordinate.isValid()) {
            takeoffCoordinate = firstCoordinate;
        } else {
            takeoffCoordinate = plannedHomePosition();
        }
    }
    //-- Build bounding "cube"
    boundingCube = QGCGeoBoundingCube(
                QGeoCoordinate(north - 90.0, west - 180.0, minAlt),
                QGeoCoordinate(south - 90.0, east - 180.0, maxAlt));
    if(_travelBoundingCube != boundingCube || _takeoffCoordinate != takeoffCoordinate) {
        _takeoffCoordinate  = takeoffCoordinate;
        _travelBoundingCube = boundingCube;
        emit missionBoundingCubeChanged();
        qCDebug(MissionControllerLog) << "Bounding cube:" << _travelBoundingCube.pointNW << _travelBoundingCube.pointSE;
    }
}

void MissionController::_complexBoundingBoxChanged()
{
    _updateTimer.start(UPDATE_TIMEOUT);
}

bool MissionController::isEmpty(void) const
{
    return _visualItems->count() <= 1;
}
