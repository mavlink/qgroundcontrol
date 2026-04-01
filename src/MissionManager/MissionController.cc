#include "MissionController.h"
#include "Vehicle.h"
#include "VehicleSupports.h"
#include "MissionManager.h"
#include "FlightPathSegment.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "SimpleMissionItem.h"
#include "SurveyComplexItem.h"
#include "FixedWingLandingComplexItem.h"
#include "VTOLLandingComplexItem.h"
#include "StructureScanComplexItem.h"
#include "CorridorScanComplexItem.h"
#include "JsonHelper.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionSettingsItem.h"
#include "PlanMasterController.h"
#include "KMLPlanDomDocument.h"
#include "QGCCorePlugin.h"
#include "TakeoffMissionItem.h"
#include "PlanViewSettings.h"
#include "MissionCommandTree.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtMath>

#define UPDATE_TIMEOUT 5000 ///< How often we check for bounding box changes

QGC_LOGGING_CATEGORY(MissionControllerLog, "PlanManager.MissionController")

MissionController::MissionController(PlanMasterController* masterController, QObject *parent)
    : PlanElementController (masterController, parent)
    , _controllerVehicle    (masterController->controllerVehicle())
    , _managerVehicle       (masterController->managerVehicle())
    , _missionManager       (masterController->managerVehicle()->missionManager())
    , _visualItems          (new QmlObjectListModel(this))
    , _planViewSettings     (SettingsManager::instance()->planViewSettings())
    , _appSettings          (SettingsManager::instance()->appSettings())
{
    _resetMissionFlightStatus();

    _updateTimer.setSingleShot(true);

    connect(&_updateTimer,                                      &QTimer::timeout,                                       this, &MissionController::_updateTimeout);
    connect(_planViewSettings->takeoffItemNotRequired(),        &Fact::rawValueChanged,                                 this, &MissionController::_forceRecalcOfAllowedBits);
    connect(_planViewSettings->allowMultipleLandingPatterns(),  &Fact::rawValueChanged,                                 this, &MissionController::multipleLandPatternsAllowedChanged);
    connect(_masterController,                                  &PlanMasterController::managerVehicleChanged,           this, &MissionController::multipleLandPatternsAllowedChanged);
    connect(this,                                               &MissionController::multipleLandPatternsAllowedChanged, this, &MissionController::_forceRecalcOfAllowedBits);
    connect(this,                                               &MissionController::missionPlannedDistanceChanged,      this, &MissionController::recalcTerrainProfile);

    // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &MissionController::_recalcMissionFlightStatusSignal, this, &MissionController::_recalcMissionFlightStatus,   Qt::QueuedConnection);
    connect(this, &MissionController::_recalcFlightPathSegmentsSignal,  this, &MissionController::_recalcFlightPathSegments,    Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&MissionController::_recalcMissionFlightStatusSignal));
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&MissionController::_recalcFlightPathSegmentsSignal));
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&MissionController::recalcTerrainProfile));
}

MissionController::~MissionController()
{

}

void MissionController::_resetMissionFlightStatus(void)
{
    _flightStatusCalc.reset(_controllerVehicle, _managerVehicle, _missionContainsVTOLTakeoff);
    _missionFlightStatus = _flightStatusCalc.status();

    emit missionPlannedDistanceChanged(_missionFlightStatus.plannedDistance);
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

    _managerVehicleChanged(_managerVehicle);
    connect(_masterController, &PlanMasterController::managerVehicleChanged, this, &MissionController::_managerVehicleChanged);

    PlanElementController::start(flyView);
    _init();
}

void MissionController::_init(void)
{
    // We start with an empty mission
    _addMissionSettings(_visualItems);

    // Set up the tree model structure once (groups + static children)
    _setupTreeModel();

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

        _setupNewVisualItems();

        const QList<MissionItem*>& newMissionItems = _missionManager->missionItems();
        qCDebug(MissionControllerLog) << "loading from vehicle: count"<< newMissionItems.count();

        int i=0;
        if (_controllerVehicle->firmwarePlugin()->sendHomePositionToVehicle() && newMissionItems.count() != 0) {
            // First item is fake home position
            MissionItem* fakeHomeItem = newMissionItems[0];
            if (fakeHomeItem->coordinate().latitude() != 0 || fakeHomeItem->coordinate().longitude() != 0) {
                _settingsItem->setCoordinate(fakeHomeItem->coordinate());
            }
            i = 1;
        }

        bool weHaveItemsFromVehicle = false;
        for (; i < newMissionItems.count(); i++) {
            weHaveItemsFromVehicle = true;
            const MissionItem* missionItem = newMissionItems[i];
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_masterController, _flyView, *missionItem);
            if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(simpleItem->command()))) {
                // This needs to be a TakeoffMissionItem
                _takeoffMissionItem = new TakeoffMissionItem(*missionItem, _masterController, _flyView, _settingsItem, false /* forLoad */);
                _takeoffMissionItem->setWizardMode(false);
                simpleItem->deleteLater();
                simpleItem = _takeoffMissionItem;
            }
            _visualItems->append(simpleItem);
        }

        // We set Altitude frame to mixed, otherwise if we need a non relative altitude frame we won't be able to change it
        setGlobalAltitudeFrame(weHaveItemsFromVehicle ? QGroundControlQmlGlobal::AltitudeFrameMixed : QGroundControlQmlGlobal::AltitudeFrameRelative);

        MissionController::_scanForAdditionalSettings(_visualItems, _masterController);

        _initAllVisualItems();

        emit newItemsFromVehicle();

        emit containsItemsChanged();
    }
    _itemsRequested = false;
}

void MissionController::loadFromVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::loadFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::loadFromVehicle called while syncInProgress";
    } else {
        _itemsRequested = true;
        _managerVehicle->missionManager()->loadFromVehicle();
    }
}

void MissionController::sendToVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(MissionControllerLog) << "MissionControllerLog::sendToVehicle";
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

void MissionController::addMissionToKML(KMLPlanDomDocument& planKML)
{
    QObject*            deleteParent = new QObject();
    QList<MissionItem*> rgMissionItems;

    _convertToMissionItems(_visualItems, rgMissionItems, deleteParent);
    planKML.addMission(_controllerVehicle, _visualItems, rgMissionItems);
    deleteParent->deleteLater();
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
    SimpleMissionItem * newItem = new SimpleMissionItem(_masterController, _flyView, false /* forLoad */);
    newItem->setSequenceNumber(sequenceNumber);
    newItem->setCoordinate(coordinate);
    newItem->setCommand(command);
    _initVisualItem(newItem);

    if (newItem->specifiesAltitude()) {
        if (!MissionCommandTree::instance()->isLandCommand(command)) {
            double                              prevAltitude;
            QGroundControlQmlGlobal::AltitudeFrame    prevAltFrame;

            if (_findPreviousAltitude(visualItemIndex, &prevAltitude, &prevAltFrame)) {
                newItem->altitude()->setRawValue(prevAltitude);
                if (globalAltitudeFrame() == QGroundControlQmlGlobal::AltitudeFrameMixed) {
                    // We are in mixed altitude frames, so copy from previous. Otherwise altitude frame will be set from global setting.
                    newItem->setAltitudeFrame(static_cast<QGroundControlQmlGlobal::AltitudeFrame>(prevAltFrame));
                }
            }
        }
    }
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

    _firstItemAdded();

    return newItem;
}


VisualMissionItem* MissionController::insertSimpleMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    return _insertSimpleMissionItemWorker(coordinate, MAV_CMD_NAV_WAYPOINT, visualItemIndex, makeCurrentItem);
}

VisualMissionItem* MissionController::insertTakeoffItem(QGeoCoordinate /*coordinate*/, int visualItemIndex, bool makeCurrentItem)
{
    int sequenceNumber = _nextSequenceNumber();
    _takeoffMissionItem = new TakeoffMissionItem(_controllerVehicle->vtol() ? MAV_CMD_NAV_VTOL_TAKEOFF : MAV_CMD_NAV_TAKEOFF, _masterController, _flyView, _settingsItem, false /* forLoad */);
    _takeoffMissionItem->setSequenceNumber(sequenceNumber);
    _initVisualItem(_takeoffMissionItem);

    if (_takeoffMissionItem->specifiesAltitude()) {
        double                              prevAltitude;
        QGroundControlQmlGlobal::AltitudeFrame    prevAltFrame;

        if (_findPreviousAltitude(visualItemIndex, &prevAltitude, &prevAltFrame)) {
            _takeoffMissionItem->altitude()->setRawValue(prevAltitude);
            _takeoffMissionItem->setAltitudeFrame(static_cast<QGroundControlQmlGlobal::AltitudeFrame>(prevAltFrame));
        }
    }
    if (visualItemIndex == -1) {
        _visualItems->append(_takeoffMissionItem);
    } else {
        _visualItems->insert(visualItemIndex, _takeoffMissionItem);
    }

    _recalcAll();

    if (makeCurrentItem) {
        setCurrentPlanViewSeqNum(_takeoffMissionItem->sequenceNumber(), true);
    }

    _firstItemAdded();

    return _takeoffMissionItem;
}

bool MissionController::multipleLandPatternsAllowed(void) const {
    // Can't have more than one land sequence unless allowed in settings and
    // supported by the firmware
    return _planViewSettings->allowMultipleLandingPatterns()
               ->rawValue().toBool() &&
           !_masterController->managerVehicle()->px4Firmware();
}

VisualMissionItem* MissionController::insertLandItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    if (_controllerVehicle->fixedWing()) {
        FixedWingLandingComplexItem* fwLanding = qobject_cast<FixedWingLandingComplexItem*>(insertComplexMissionItem(FixedWingLandingComplexItem::name, coordinate, visualItemIndex, makeCurrentItem));
        return fwLanding;
    } else if (_controllerVehicle->vtol()) {
        VTOLLandingComplexItem* vtolLanding = qobject_cast<VTOLLandingComplexItem*>(insertComplexMissionItem(VTOLLandingComplexItem::name, coordinate, visualItemIndex, makeCurrentItem));
        return vtolLanding;
    } else {
        return _insertSimpleMissionItemWorker(coordinate, _controllerVehicle->vtol() ? MAV_CMD_NAV_VTOL_LAND : MAV_CMD_NAV_RETURN_TO_LAUNCH, visualItemIndex, makeCurrentItem);
    }
}

VisualMissionItem* MissionController::insertROIMissionItem(QGeoCoordinate coordinate, int visualItemIndex, bool makeCurrentItem)
{
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(_insertSimpleMissionItemWorker(coordinate, MAV_CMD_DO_SET_ROI_LOCATION, visualItemIndex, makeCurrentItem));

    if (!_controllerVehicle->firmwarePlugin()->supportedMissionCommands(QGCMAVLink::VehicleClassGeneric).contains(MAV_CMD_DO_SET_ROI_LOCATION)) {
        simpleItem->setCommand(MAV_CMD_DO_SET_ROI)  ;
        simpleItem->missionItem().setParam1(MAV_ROI_LOCATION);
    }
    return simpleItem;
}

VisualMissionItem* MissionController::insertCancelROIMissionItem(int visualItemIndex, bool makeCurrentItem)
{
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(_insertSimpleMissionItemWorker(QGeoCoordinate(), MAV_CMD_DO_SET_ROI_NONE, visualItemIndex, makeCurrentItem));

    if (!_controllerVehicle->firmwarePlugin()->supportedMissionCommands(QGCMAVLink::VehicleClassGeneric).contains(MAV_CMD_DO_SET_ROI_NONE)) {
        simpleItem->setCommand(MAV_CMD_DO_SET_ROI)  ;
        simpleItem->missionItem().setParam1(MAV_ROI_NONE);
    }
    return simpleItem;
}

VisualMissionItem* MissionController::insertComplexMissionItem(QString itemName, QGeoCoordinate mapCenterCoordinate, int visualItemIndex, bool makeCurrentItem)
{
    ComplexMissionItem* newItem = nullptr;

    if (itemName == SurveyComplexItem::name) {
        newItem = new SurveyComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
        newItem->setCoordinate(mapCenterCoordinate);

        double                              prevAltitude;
        QGroundControlQmlGlobal::AltitudeFrame    prevAltFrame;
        if (globalAltitudeFrame() == QGroundControlQmlGlobal::AltitudeFrameMixed) {
            // We are in mixed altitude frames, so copy from previous. Otherwise alt mode will be set from global setting in constructor.
            if (_findPreviousAltitude(visualItemIndex, &prevAltitude, &prevAltFrame)) {
                qobject_cast<SurveyComplexItem*>(newItem)->cameraCalc()->setDistanceMode(prevAltFrame);
            }
        }
    } else if (itemName == FixedWingLandingComplexItem::name) {
        newItem = new FixedWingLandingComplexItem(_masterController, _flyView);
    } else if (itemName == VTOLLandingComplexItem::name) {
        newItem = new VTOLLandingComplexItem(_masterController, _flyView);
    } else if (itemName == StructureScanComplexItem::name) {
        newItem = new StructureScanComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
    } else if (itemName == CorridorScanComplexItem::name) {
        newItem = new CorridorScanComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
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

    if (itemName == SurveyComplexItem::name) {
        newItem = new SurveyComplexItem(_masterController, _flyView, file);
    } else if (itemName == StructureScanComplexItem::name) {
        newItem = new StructureScanComplexItem(_masterController, _flyView, file);
    } else if (itemName == CorridorScanComplexItem::name) {
        newItem = new CorridorScanComplexItem(_masterController, _flyView, file);
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
    if (!qobject_cast<VTOLLandingComplexItem*>(complexItem)) {
        complexItem->setWizardMode(true);
    }
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
    _firstItemAdded();
}

int MissionController::visualItemIndexForObject(QObject* object) const
{
    return _visualItems ? _visualItems->indexOf(object) : -1;
}

void MissionController::removeVisualItem(int viIndex)
{
    if (viIndex <= 0 || viIndex >= _visualItems->count()) {
        qWarning() << "MissionController::removeVisualItem called with bad index - count:index" << _visualItems->count() << viIndex;
        return;
    }

    bool removeSurveyStyle = _visualItems->value<SurveyComplexItem*>(viIndex) || _visualItems->value<CorridorScanComplexItem*>(viIndex);
    VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->removeAt(viIndex));

    if (item == _takeoffMissionItem) {
        _takeoffMissionItem = nullptr;
    }

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

    // Adjust current item
    int newVIIndex;
    if (viIndex >= _visualItems->count()) {
        newVIIndex = _visualItems->count() - 1;
    } else {
        newVIIndex = viIndex;
    }
    setCurrentPlanViewSeqNum(_visualItems->value<VisualMissionItem*>(newVIIndex)->sequenceNumber(), true);

    setDirty(true);

    if (_visualItems->count() == 1) {
        _allItemsRemoved();
    }
}

void MissionController::_setupNewVisualItems(QmlObjectListModel* newItems)
{
    QmlObjectListModel* oldItems = _visualItems;

    if (oldItems) {
        _deinitAllVisualItems();

        // Destroy old items after a delay — TreeView delegates are torn down
        // asynchronously during a polish cycle and may still hold bindings.
        QTimer::singleShot(1000, oldItems, [oldItems] {
            oldItems->clearAndDeleteContents();
            oldItems->deleteLater();
        });
    }

    _settingsItem = nullptr;
    _takeoffMissionItem = nullptr;

    if (newItems) {
        _visualItems = newItems;
        if (_visualItems->count() == 0) {
            _addMissionSettings(_visualItems);
        } else {
            _settingsItem = _visualItems->value<MissionSettingsItem*>(0);
        }
    } else {
        _visualItems = new QmlObjectListModel(this);
        _addMissionSettings(_visualItems);
    }
}

void MissionController::removeAll(void)
{
    _setupNewVisualItems();
    _initAllVisualItems();
    setDirty(true);
    _resetMissionFlightStatus();
    _allItemsRemoved();
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
        { _jsonGlobalPlanAltitudeModeKey,   QJsonValue::Double, false },
    };
    if (!JsonHelper::validateKeys(json, rootKeyInfoList, errorString)) {
        return false;
    }

    setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameMixed);

    qCDebug(MissionControllerLog) << "MissionController::_loadJsonMissionFileV2 itemCount:" << json[_jsonItemsKey].toArray().count();

    AppSettings* appSettings = SettingsManager::instance()->appSettings();

    // Get the firmware/vehicle type from the plan file
    MAV_AUTOPILOT   planFileFirmwareType =  static_cast<MAV_AUTOPILOT>(json[_jsonFirmwareTypeKey].toInt());
    MAV_TYPE        planFileVehicleType =   static_cast<MAV_TYPE>     (QGCMAVLink::vehicleClassToMavType(appSettings->offlineEditingVehicleClass()->rawValue().toInt()));
    if (json.contains(_jsonVehicleTypeKey)) {
        planFileVehicleType = static_cast<MAV_TYPE>(json[_jsonVehicleTypeKey].toInt());
    }

    // Update firmware/vehicle offline settings if we aren't connect to a vehicle
    if (_masterController->offline()) {
        appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(static_cast<MAV_AUTOPILOT>(json[_jsonFirmwareTypeKey].toInt())));
        if (json.contains(_jsonVehicleTypeKey)) {
            appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(planFileVehicleType));
        }
    }

    // The controller vehicle always tracks the Plan file firmware/vehicle types so update it
    _controllerVehicle->stopTrackingFirmwareVehicleTypeChanges();
    _controllerVehicle->_offlineFirmwareTypeSettingChanged(planFileFirmwareType);
    _controllerVehicle->_offlineVehicleTypeSettingChanged(planFileVehicleType);

    if (json.contains(_jsonCruiseSpeedKey)) {
        appSettings->offlineEditingCruiseSpeed()->setRawValue(json[_jsonCruiseSpeedKey].toDouble());
    }
    if (json.contains(_jsonHoverSpeedKey)) {
        appSettings->offlineEditingHoverSpeed()->setRawValue(json[_jsonHoverSpeedKey].toDouble());
    }
    if (json.contains(_jsonGlobalPlanAltitudeModeKey)) {
        setGlobalAltitudeFrame(json[_jsonGlobalPlanAltitudeModeKey].toVariant().value<QGroundControlQmlGlobal::AltitudeFrame>());
    }

    QGeoCoordinate homeCoordinate;
    if (!JsonHelper::loadGeoCoordinate(json[_jsonPlannedHomePositionKey], true /* altitudeRequired */, homeCoordinate, errorString)) {
        return false;
    }
    MissionSettingsItem* settingsItem = new MissionSettingsItem(_masterController, _flyView);
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
            SimpleMissionItem* simpleItem = new SimpleMissionItem(_masterController, _flyView, true /* forLoad */);
            if (simpleItem->load(itemObject, nextSequenceNumber, errorString)) {
                if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(simpleItem->command()))) {
                    // This needs to be a TakeoffMissionItem
                    TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(_masterController, _flyView, settingsItem, true /* forLoad */);
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
                SurveyComplexItem* surveyItem = new SurveyComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
                if (!surveyItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = surveyItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Survey load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(surveyItem);
            } else if (complexItemType == FixedWingLandingComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Fixed Wing Landing Pattern: nextSequenceNumber" << nextSequenceNumber;
                FixedWingLandingComplexItem* landingItem = new FixedWingLandingComplexItem(_masterController, _flyView);
                if (!landingItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = landingItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "FW Landing Pattern load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(landingItem);
            } else if (complexItemType == VTOLLandingComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading VTOL Landing Pattern: nextSequenceNumber" << nextSequenceNumber;
                VTOLLandingComplexItem* landingItem = new VTOLLandingComplexItem(_masterController, _flyView);
                if (!landingItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = landingItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "VTOL Landing Pattern load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(landingItem);
            } else if (complexItemType == StructureScanComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Structure Scan: nextSequenceNumber" << nextSequenceNumber;
                StructureScanComplexItem* structureItem = new StructureScanComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
                if (!structureItem->load(itemObject, nextSequenceNumber++, errorString)) {
                    return false;
                }
                nextSequenceNumber = structureItem->lastSequenceNumber() + 1;
                qCDebug(MissionControllerLog) << "Structure Scan load complete: nextSequenceNumber" << nextSequenceNumber;
                visualItems->append(structureItem);
            } else if (complexItemType == CorridorScanComplexItem::jsonComplexItemTypeValue) {
                qCDebug(MissionControllerLog) << "Loading Corridor Scan: nextSequenceNumber" << nextSequenceNumber;
                CorridorScanComplexItem* corridorItem = new CorridorScanComplexItem(_masterController, _flyView, QString() /* kmlOrShpFile */);
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
            SimpleMissionItem* item = new SimpleMissionItem(_masterController, _flyView, true /* forLoad */);
            if (item->load(stream)) {
                if (firstItem && plannedHomePositionInFile) {
                    settingsItem->setCoordinate(item->coordinate());
                } else {
                    if (TakeoffMissionItem::isTakeoffCommand(static_cast<MAV_CMD>(item->command()))) {
                        // This needs to be a TakeoffMissionItem
                        TakeoffMissionItem* takeoffItem = new TakeoffMissionItem(item->missionItem(), _masterController, _flyView, settingsItem, false /* forLoad */);
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
        errorString = tr("The mission file is not compatible with this version of %1.").arg(QCoreApplication::applicationName());
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
    _setupNewVisualItems(loadedVisualItems);

    MissionController::_scanForAdditionalSettings(_visualItems, _masterController);

    _initAllVisualItems();

    if (_visualItems->count() > 1) {
        _firstItemAdded();
    } else {
        _allItemsRemoved();
    }
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

bool MissionController::loadTextFile(QFile& file, QString& errorString)
{
    QString     errorStr;
    QString     errorMessage = tr("Mission: %1");
    QByteArray  bytes = file.readAll();
    QTextStream stream(bytes);

    setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameMixed);

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
    json[_jsonPlannedHomePositionKey]       = coordinateValue;
    json[_jsonFirmwareTypeKey]              = _controllerVehicle->firmwareType();
    json[_jsonVehicleTypeKey]               = _controllerVehicle->vehicleType();
    json[_jsonCruiseSpeedKey]               = _controllerVehicle->defaultCruiseSpeed();
    json[_jsonHoverSpeedKey]                = _controllerVehicle->defaultHoverSpeed();
    json[_jsonGlobalPlanAltitudeModeKey]    = _globalAltFrame;

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

FlightPathSegment* MissionController::_createFlightPathSegmentWorker(VisualItemPair& pair, bool mavlinkTerrainFrame)
{
    // The takeoff goes straight up from ground to alt and then over to specified position at same alt. Which means
    // that coord 1 altitude is the same as coord altitude.
    bool                takeoffStraightUp   = pair.second->isTakeoffItem() && !_controllerVehicle->fixedWing();

    QGeoCoordinate      coord1              = pair.first->exitCoordinate();
    QGeoCoordinate      coord2              = pair.second->entryCoordinate();
    double              coord2AMSLAlt       = pair.second->amslEntryAlt();
    double              coord1AMSLAlt       = takeoffStraightUp ? coord2AMSLAlt : pair.first->amslExitAlt();

    FlightPathSegment::SegmentType segmentType = mavlinkTerrainFrame ? FlightPathSegment::SegmentTypeTerrainFrame : FlightPathSegment::SegmentTypeGeneric;
    if (pair.second->isTakeoffItem()) {
        segmentType = FlightPathSegment::SegmentTypeTakeoff;
    } else if (pair.second->isLandCommand()) {
        segmentType = FlightPathSegment::SegmentTypeLand;
    }

    FlightPathSegment* segment = new FlightPathSegment(segmentType, coord1, coord1AMSLAlt, coord2, coord2AMSLAlt, !_flyView /* queryTerrainData */,  this);

    if (takeoffStraightUp) {
        connect(pair.second, &VisualMissionItem::amslEntryAltChanged, segment, &FlightPathSegment::setCoord1AMSLAlt);
    } else {
        connect(pair.first, &VisualMissionItem::amslExitAltChanged, segment, &FlightPathSegment::setCoord1AMSLAlt);
    }
    connect(pair.first,  &VisualMissionItem::exitCoordinateChanged,     segment,    &FlightPathSegment::setCoordinate1);
    connect(pair.second, &VisualMissionItem::entryCoordinateChanged,    segment,    &FlightPathSegment::setCoordinate2);
    connect(pair.second, &VisualMissionItem::amslEntryAltChanged,       segment,    &FlightPathSegment::setCoord2AMSLAlt);

    connect(pair.second, &VisualMissionItem::entryCoordinateChanged,    this,       &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(pair.second, &VisualMissionItem::exitCoordinateChanged,     this,       &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);

    connect(segment,    &FlightPathSegment::totalDistanceChanged,       this,       &MissionController::recalcTerrainProfile,             Qt::QueuedConnection);
    connect(segment,    &FlightPathSegment::coord1AMSLAltChanged,       this,       &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(segment,    &FlightPathSegment::coord2AMSLAltChanged,       this,       &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(segment,    &FlightPathSegment::amslTerrainHeightsChanged,  this,       &MissionController::recalcTerrainProfile,             Qt::QueuedConnection);
    connect(segment,    &FlightPathSegment::terrainCollisionChanged,    this,       &MissionController::recalcTerrainProfile,             Qt::QueuedConnection);

    return segment;
}

FlightPathSegment* MissionController::_addFlightPathSegment(FlightPathSegmentHashTable& prevItemPairHashTable, VisualItemPair& pair, bool mavlinkTerrainFrame)
{
    FlightPathSegment* segment = nullptr;

    if (prevItemPairHashTable.contains(pair) && (prevItemPairHashTable[pair]->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) != mavlinkTerrainFrame) {
        // Pair already exists and connected, just re-use
        _flightPathSegmentHashTable[pair] = segment = prevItemPairHashTable.take(pair);
    } else {
        segment = _createFlightPathSegmentWorker(pair, mavlinkTerrainFrame);
        _flightPathSegmentHashTable[pair] = segment;
    }

    _simpleFlightPathSegments.append(segment);

    return segment;
}

void MissionController::_recalcFlightPathSegments(void)
{
    VisualItemPair      lastSegmentVisualItemPair;
    int                 segmentCount =              0;
    bool                firstCoordinateNotFound =   true;
    VisualMissionItem*  lastFlyThroughVI =          qobject_cast<VisualMissionItem*>(_visualItems->get(0));
    bool                linkEndToHome =             false;
    bool                linkStartToHome =           _controllerVehicle->rover() ? true : false;
    bool                foundRTL =                  false;
    bool                homePositionValid =         _settingsItem->coordinate().isValid();
    bool                roiActive =                 false;
    bool                previousItemIsIncomplete =  false;
    bool                signalSplitSegmentChanged = false;

    qCDebug(MissionControllerLog) << "_recalcFlightPathSegments homePositionValid" << homePositionValid;

    FlightPathSegmentHashTable oldSegmentTable = _flightPathSegmentHashTable;

    _missionContainsVTOLTakeoff = false;
    _flightPathSegmentHashTable.clear();

    _simpleFlightPathSegments.beginResetModel();
    _directionArrows.beginResetModel();

    _simpleFlightPathSegments.clear();
    _directionArrows.clear();

    // Mission Settings item needs to start with no segment
    lastFlyThroughVI->clearSimpleFlighPathSegment();

    // We need to clear the simple flight path segments on all items since they are going to be rebuilt. We can't just do this in the main loop
    // below since that loop won't always process all items.
    for (int i=1; i<_visualItems->count(); i++) {
        qobject_cast<VisualMissionItem*>(_visualItems->get(i))->clearSimpleFlighPathSegment();
    }

    // Grovel through the list of items keeping track of things needed to correctly draw waypoints lines
    for (int i=1; i<_visualItems->count(); i++) {
        VisualMissionItem*  visualItem =    qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        SimpleMissionItem*  simpleItem =    qobject_cast<SimpleMissionItem*>(visualItem);
        ComplexMissionItem* complexItem =   qobject_cast<ComplexMissionItem*>(visualItem);

        visualItem->clearSimpleFlighPathSegment();

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
                _missionContainsVTOLTakeoff = command == MAV_CMD_NAV_VTOL_TAKEOFF;
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

        // Don't draw segments immediately after a landing item
        if (lastFlyThroughVI->isLandCommand()) {
            lastFlyThroughVI = visualItem;
            continue;
        }

        if (visualItem->specifiesCoordinate() && !visualItem->isStandaloneCoordinate()) {
            // Incomplete items are complex items which are waiting for the user to complete setup before there visuals can become valid.
            // They may not yet have valid entry/exit coordinates associated with them while in the incomplete state.
            // For examples a Survey item which has no polygon set yet.
            if (complexItem && complexItem->isIncomplete()) {
                // We don't link lines from a valid item to an incomplete item
                previousItemIsIncomplete = true;
            } else if (previousItemIsIncomplete) {
                // We also don't link lines from an incomplete item to a valid item.
                previousItemIsIncomplete = false;
                firstCoordinateNotFound = false;
                lastFlyThroughVI = visualItem;
            } else {
                if (lastFlyThroughVI != _settingsItem || (homePositionValid && linkStartToHome)) {
                    bool addDirectionArrow = false;
                    if (i != 1) {
                        // Direction arrows are added to the second segment and every 5 segments thereafter.
                        // The reason for start with second segment is to prevent an arrow being added in between the home position
                        // and a takeoff item which may be right over each other. In that case the arrow points in a random direction.
                        if (firstCoordinateNotFound || !lastFlyThroughVI->isSimpleItem() || !visualItem->isSimpleItem()) {
                            addDirectionArrow = true;
                        } else if (segmentCount > 5) {
                            segmentCount = 0;
                            addDirectionArrow = true;
                        }
                        segmentCount++;
                    }

                    lastSegmentVisualItemPair =  VisualItemPair(lastFlyThroughVI, visualItem);
                    SimpleMissionItem* lastSimpleItem = qobject_cast<SimpleMissionItem*>(lastFlyThroughVI);
                    bool mavlinkTerrainFrame = lastSimpleItem ? lastSimpleItem->missionItem().frame() == MAV_FRAME_GLOBAL_TERRAIN_ALT : false;
                    FlightPathSegment* segment = _addFlightPathSegment(oldSegmentTable, lastSegmentVisualItemPair, mavlinkTerrainFrame);
                    segment->setSpecialVisual(roiActive);
                    if (addDirectionArrow) {
                        _directionArrows.append(segment);
                    }
                    if (visualItem->isCurrentItem() && _delayedSplitSegmentUpdate) {
                        _splitSegment = segment;
                        _delayedSplitSegmentUpdate = false;
                        signalSplitSegmentChanged = true;
                    }
                    lastFlyThroughVI->setSimpleFlighPathSegment(segment);
                }
                firstCoordinateNotFound = false;
                lastFlyThroughVI = visualItem;
            }
        }
    }

    if (linkEndToHome && lastFlyThroughVI != _settingsItem && homePositionValid) {
        lastSegmentVisualItemPair = VisualItemPair(lastFlyThroughVI, _settingsItem);
        FlightPathSegment* segment = _addFlightPathSegment(oldSegmentTable, lastSegmentVisualItemPair, false /* mavlinkTerrainFrame */);
        segment->setSpecialVisual(roiActive);
        lastFlyThroughVI->setSimpleFlighPathSegment(segment);
    }

    // Add direction arrow to last segment
    if (lastSegmentVisualItemPair.first) {
        FlightPathSegment* coordVector = nullptr;

        // The pair may not be in the hash, this can happen in the fly view where only segments with arrows on them are added to hash.
        // check for that first and add if needed

        if (_flightPathSegmentHashTable.contains(lastSegmentVisualItemPair)) {
            // Pair exists in the new table already just reuse it
            coordVector = _flightPathSegmentHashTable[lastSegmentVisualItemPair];
        } else if (oldSegmentTable.contains(lastSegmentVisualItemPair)) {
            // Pair already exists in old table, pull from old to new and reuse
            _flightPathSegmentHashTable[lastSegmentVisualItemPair] = coordVector = oldSegmentTable.take(lastSegmentVisualItemPair);
        } else {
            // Create a new segment. Since this is the fly view there is no need to wire change signals or worry about correct SegmentType
            coordVector = new FlightPathSegment(
                        FlightPathSegment::SegmentTypeGeneric,
                        lastSegmentVisualItemPair.first->isSimpleItem() ? lastSegmentVisualItemPair.first->entryCoordinate() : lastSegmentVisualItemPair.first->exitCoordinate(),
                        lastSegmentVisualItemPair.first->isSimpleItem() ? lastSegmentVisualItemPair.first->amslEntryAlt() : lastSegmentVisualItemPair.first->amslExitAlt(),
                        lastSegmentVisualItemPair.second->entryCoordinate(),
                        lastSegmentVisualItemPair.second->amslEntryAlt(),
                        !_flyView /* queryTerrainData */,
                        this);
            _flightPathSegmentHashTable[lastSegmentVisualItemPair] = coordVector;
        }

        _directionArrows.append(coordVector);
    }

    _simpleFlightPathSegments.endResetModel();
    _directionArrows.endResetModel();

    // Anything left in the old table is an obsolete line object that can go
    qDeleteAll(oldSegmentTable);

    emit _recalcMissionFlightStatusSignal();

    emit recalcTerrainProfile();
    if (signalSplitSegmentChanged) {
        emit splitSegmentChanged();
    }
}

void MissionController::_recalcMissionFlightStatus()
{
    if (!_visualItems->count()) {
        return;
    }

    qCDebug(MissionControllerLog) << "_recalcMissionFlightStatus";

    _flightStatusCalc.recalc(_visualItems, _settingsItem, _controllerVehicle, _managerVehicle, _appSettings, _planViewSettings, _missionContainsVTOLTakeoff);
    _missionFlightStatus = _flightStatusCalc.status();
    _minAMSLAltitude = _flightStatusCalc.minAMSLAltitude();
    _maxAMSLAltitude = _flightStatusCalc.maxAMSLAltitude();

    emit missionMaxTelemetryChanged     (_missionFlightStatus.maxTelemetryDistance);
    emit missionTotalDistanceChanged    (_missionFlightStatus.totalDistance);
    emit missionPlannedDistanceChanged  (_missionFlightStatus.plannedDistance);
    emit missionHoverDistanceChanged    (_missionFlightStatus.hoverDistance);
    emit missionCruiseDistanceChanged   (_missionFlightStatus.cruiseDistance);
    emit missionTimeChanged             ();
    emit missionHoverTimeChanged        ();
    emit missionCruiseTimeChanged       ();
    emit batteryChangePointChanged      (_missionFlightStatus.batteryChangePoint);
    emit batteriesRequiredChanged       (_missionFlightStatus.batteriesRequired);
    emit minAMSLAltitudeChanged         (_minAMSLAltitude);
    emit maxAMSLAltitudeChanged         (_maxAMSLAltitude);

    _updateTimer.start(UPDATE_TIMEOUT);

    emit recalcTerrainProfile();
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

void MissionController::_setupTreeModel(void)
{
    _visualItemsTree.beginResetModel();

    _visualItemsTree.clear();

    // ── Plan File group ──
    _planFileGroupNode.setObjectName(tr("Plan Info"));
    _visualItemsTree.appendItem(&_planFileGroupNode, QModelIndex(), QStringLiteral("planFileGroup"));

    _planFileInfoMarker.setObjectName(QStringLiteral("planFileInfo"));
    _visualItemsTree.appendItem(&_planFileInfoMarker,
        _visualItemsTree.indexForObject(&_planFileGroupNode), QStringLiteral("planFileInfo"));

    // ── Defaults group ──
    _defaultsGroupNode.setObjectName(tr("Defaults"));
    _visualItemsTree.appendItem(&_defaultsGroupNode, QModelIndex(), QStringLiteral("defaultsGroup"));

    _defaultsInfoMarker.setObjectName(QStringLiteral("defaultsInfo"));
    _visualItemsTree.appendItem(&_defaultsInfoMarker,
        _visualItemsTree.indexForObject(&_defaultsGroupNode), QStringLiteral("defaultsInfo"));

    // ── Mission Items group ──
    _missionItemsGroupNode.setObjectName(tr("Mission Items"));
    _visualItemsTree.appendItem(&_missionItemsGroupNode, QModelIndex(), QStringLiteral("missionGroup"));

    // ── GeoFence group ──
    _fenceGroupNode.setObjectName(tr("GeoFence"));
    _visualItemsTree.appendItem(&_fenceGroupNode, QModelIndex(), QStringLiteral("fenceGroup"));

    // Single marker child — delegate loads the full GeoFenceEditor
    _fenceEditorMarker.setObjectName(QStringLiteral("fenceEditor"));
    _visualItemsTree.appendItem(&_fenceEditorMarker,
        _visualItemsTree.indexForObject(&_fenceGroupNode), QStringLiteral("fenceEditor"));

    // ── Rally Points group ──
    _rallyGroupNode.setObjectName(tr("Rally Points"));
    _visualItemsTree.appendItem(&_rallyGroupNode, QModelIndex(), QStringLiteral("rallyGroup"));

    // Marker child for the rally header / instructions
    _rallyHeaderMarker.setObjectName(QStringLiteral("rallyHeader"));
    _visualItemsTree.appendItem(&_rallyHeaderMarker,
        _visualItemsTree.indexForObject(&_rallyGroupNode), QStringLiteral("rallyHeader"));

    // ── Transform group ──
    _transformGroupNode.setObjectName(tr("Transform"));
    _visualItemsTree.appendItem(&_transformGroupNode, QModelIndex(), QStringLiteral("transformGroup"));

    // Single marker child — delegate loads the inline TransformEditor
    _transformEditorMarker.setObjectName(QStringLiteral("transformEditor"));
    _visualItemsTree.appendItem(&_transformEditorMarker,
        _visualItemsTree.indexForObject(&_transformGroupNode), QStringLiteral("transformEditor"));

    _visualItemsTree.endResetModel();

    // Capture persistent indexes after the reset so they aren't invalidated
    _planFileGroupIndex  = QPersistentModelIndex(_visualItemsTree.indexForObject(&_planFileGroupNode));
    _defaultsGroupIndex  = QPersistentModelIndex(_visualItemsTree.indexForObject(&_defaultsGroupNode));
    _missionGroupIndex   = QPersistentModelIndex(_visualItemsTree.indexForObject(&_missionItemsGroupNode));
    _fenceGroupIndex     = QPersistentModelIndex(_visualItemsTree.indexForObject(&_fenceGroupNode));
    _rallyGroupIndex     = QPersistentModelIndex(_visualItemsTree.indexForObject(&_rallyGroupNode));
    _transformGroupIndex = QPersistentModelIndex(_visualItemsTree.indexForObject(&_transformGroupNode));
}

//-----------------------------------------------------------------------------
// Incremental tree model sync — Mission Items
//-----------------------------------------------------------------------------

void MissionController::_syncTreeMissionItemsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    for (int i = first; i <= last; i++) {
        auto* item = _visualItems->value<VisualMissionItem*>(i);
        if (item) {
            _visualItemsTree.insertItem(i, item, _missionGroupIndex, QStringLiteral("missionItem"));
        }
    }
}

void MissionController::_syncTreeMissionItemsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    // Remove in reverse order so indexes stay valid
    for (int i = last; i >= first; i--) {
        _visualItemsTree.removeAt(_missionGroupIndex, i);
    }
}

void MissionController::_syncTreeMissionItemsReset(void)
{
    // removeChildren + appendItem each fire their own row-level signals scoped
    // to _missionGroupIndex, so persistent indexes for other groups survive.
    _visualItemsTree.removeChildren(_missionGroupIndex);

    if (_visualItems) {
        for (int i = 0; i < _visualItems->count(); i++) {
            auto* item = _visualItems->value<VisualMissionItem*>(i);
            if (item) {
                _visualItemsTree.appendItem(item, _missionGroupIndex, QStringLiteral("missionItem"));
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Incremental tree model sync — Rally Points
//-----------------------------------------------------------------------------

void MissionController::_syncTreeRallyPointsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    auto* rallyController = _masterController->rallyPointController();
    if (!rallyController) return;

    // If this is the first rally point, remove the header marker
    if (first == 0 && _visualItemsTree.rowCount(_rallyGroupIndex) == 1) {
        const QModelIndex child = _visualItemsTree.index(0, 0, _rallyGroupIndex);
        if (_visualItemsTree.data(child, QmlObjectTreeModel::NodeTypeRole).toString() == QStringLiteral("rallyHeader")) {
            _visualItemsTree.removeItem(child);
        }
    }

    auto* pts = rallyController->points();
    for (int i = first; i <= last; i++) {
        _visualItemsTree.insertItem(i, (*pts)[i], _rallyGroupIndex, QStringLiteral("rallyItem"));
    }
}

void MissionController::_syncTreeRallyPointsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    for (int i = last; i >= first; i--) {
        _visualItemsTree.removeAt(_rallyGroupIndex, i);
    }
}

void MissionController::_syncTreeRallyPointsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);

    // Re-add the header marker once the source model has finished removing rows
    auto* rallyController = _masterController->rallyPointController();
    if (rallyController && rallyController->points()->count() == 0) {
        _rallyHeaderMarker.setObjectName(QStringLiteral("rallyHeader"));
        _visualItemsTree.appendItem(&_rallyHeaderMarker, _rallyGroupIndex, QStringLiteral("rallyHeader"));
    }
}

void MissionController::_syncTreeRallyPointsReset(void)
{
    // removeChildren + appendItem each fire their own row-level signals scoped
    // to _rallyGroupIndex, so persistent indexes for other groups survive.
    _visualItemsTree.removeChildren(_rallyGroupIndex);

    // Repopulate — either rally items or the header marker
    auto* rallyController = _masterController->rallyPointController();
    if (rallyController && rallyController->points()->count() > 0) {
        auto* pts = rallyController->points();
        for (int i = 0; i < pts->count(); i++) {
            _visualItemsTree.appendItem((*pts)[i], _rallyGroupIndex, QStringLiteral("rallyItem"));
        }
    } else {
        _rallyHeaderMarker.setObjectName(QStringLiteral("rallyHeader"));
        _visualItemsTree.appendItem(&_rallyHeaderMarker, _rallyGroupIndex, QStringLiteral("rallyHeader"));
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

        if (item->specifiesCoordinate() && item->entryCoordinate().isValid()) {
            foundFirstCoordinate = true;
            firstCoordinate = item->entryCoordinate();
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
        _settingsItem->setCoordinate(plannedHomeCoord);
    }
}

void MissionController::_recalcAllWithCoordinate(const QGeoCoordinate& coordinate)
{
    if (!_flyView) {
        _setPlannedHomePositionFromFirstCoordinate(coordinate);
    }
    _recalcSequence();
    _recalcChildItems();
    emit _recalcFlightPathSegmentsSignal();
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

    connect(_settingsItem, &MissionSettingsItem::coordinateChanged,     this, &MissionController::_recalcMissionFlightStatus);
    connect(_settingsItem, &MissionSettingsItem::coordinateChanged,     this, &MissionController::plannedHomePositionChanged);

    for (int i=0; i<_visualItems->count(); i++) {
        VisualMissionItem* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        _initVisualItem(item);

        TakeoffMissionItem* takeoffItem = qobject_cast<TakeoffMissionItem*>(item);
        if (takeoffItem) {
            _takeoffMissionItem = takeoffItem;
        }
    }

    _recalcAll();

    connect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_visualItemsDirtyChanged);
    connect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::containsItemsChanged);

    // Connect for incremental tree model sync
    connect(_visualItems, &QAbstractItemModel::rowsInserted, this, &MissionController::_syncTreeMissionItemsInserted);
    connect(_visualItems, &QAbstractItemModel::rowsAboutToBeRemoved, this, &MissionController::_syncTreeMissionItemsAboutToBeRemoved);
    connect(_visualItems, &QAbstractItemModel::modelReset, this, &MissionController::_syncTreeMissionItemsReset);

    // Populate tree's mission group from current _visualItems
    _syncTreeMissionItemsReset();

    // Connect rally controller for incremental tree model sync
    auto* rallyController = _masterController->rallyPointController();
    if (rallyController) {
        auto* pts = rallyController->points();
        connect(pts, &QAbstractItemModel::rowsInserted, this, &MissionController::_syncTreeRallyPointsInserted);
        connect(pts, &QAbstractItemModel::rowsAboutToBeRemoved, this, &MissionController::_syncTreeRallyPointsAboutToBeRemoved);
        connect(pts, &QAbstractItemModel::rowsRemoved, this, &MissionController::_syncTreeRallyPointsRemoved);
        connect(pts, &QAbstractItemModel::modelReset, this, &MissionController::_syncTreeRallyPointsReset);

        // Populate any existing rally points
        _syncTreeRallyPointsReset();
    }

    emit visualItemsReset();
    emit containsItemsChanged();
    emit plannedHomePositionChanged(plannedHomePosition());

    if (!_flyView) {
        setCurrentPlanViewSeqNum(0, true);
    }

    setDirty(false);
}

void MissionController::_deinitAllVisualItems(void)
{
    disconnect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &MissionController::_recalcMissionFlightStatus);
    disconnect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &MissionController::plannedHomePositionChanged);

    for (int i=0; i<_visualItems->count(); i++) {
        _deinitVisualItem(qobject_cast<VisualMissionItem*>(_visualItems->get(i)));
    }

    disconnect(_visualItems, &QmlObjectListModel::dirtyChanged, this, &MissionController::_visualItemsDirtyChanged);
    disconnect(_visualItems, &QmlObjectListModel::countChanged, this, &MissionController::containsItemsChanged);

    // Disconnect incremental tree model sync
    disconnect(_visualItems, &QAbstractItemModel::rowsInserted, this, &MissionController::_syncTreeMissionItemsInserted);
    disconnect(_visualItems, &QAbstractItemModel::rowsAboutToBeRemoved, this, &MissionController::_syncTreeMissionItemsAboutToBeRemoved);
    disconnect(_visualItems, &QAbstractItemModel::modelReset, this, &MissionController::_syncTreeMissionItemsReset);

    // Disconnect rally controller tree model sync
    auto* rallyController = _masterController->rallyPointController();
    if (rallyController) {
        auto* pts = rallyController->points();
        disconnect(pts, &QAbstractItemModel::rowsInserted, this, &MissionController::_syncTreeRallyPointsInserted);
        disconnect(pts, &QAbstractItemModel::rowsAboutToBeRemoved, this, &MissionController::_syncTreeRallyPointsAboutToBeRemoved);
        disconnect(pts, &QAbstractItemModel::rowsRemoved, this, &MissionController::_syncTreeRallyPointsRemoved);
        disconnect(pts, &QAbstractItemModel::modelReset, this, &MissionController::_syncTreeRallyPointsReset);
    }
}

void MissionController::_initVisualItem(VisualMissionItem* visualItem)
{
    setDirty(false);

    connect(visualItem, &VisualMissionItem::specifiesCoordinateChanged,                 this, &MissionController::_recalcFlightPathSegmentsSignal,  Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::specifiedFlightSpeedChanged,                this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::specifiedGimbalYawChanged,                  this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::specifiedGimbalPitchChanged,                this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::specifiedVehicleYawChanged,                 this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::terrainAltitudeChanged,                     this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::additionalTimeDelayChanged,                 this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(visualItem, &VisualMissionItem::currentVTOLModeChanged,                     this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
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
            connect(complexItem, &ComplexMissionItem::complexDistanceChanged,       this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
            connect(complexItem, &ComplexMissionItem::greatestDistanceToChanged,    this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
            connect(complexItem, &ComplexMissionItem::minAMSLAltitudeChanged,       this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
            connect(complexItem, &ComplexMissionItem::maxAMSLAltitudeChanged,       this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
            connect(complexItem, &ComplexMissionItem::isIncompleteChanged,          this, &MissionController::_recalcFlightPathSegmentsSignal,  Qt::QueuedConnection);
        } else {
            qWarning() << "ComplexMissionItem not found";
        }
    }
}

void MissionController::_deinitVisualItem(VisualMissionItem* visualItem)
{
    // Disconnect only signals connected to this MissionController, not all receivers.
    // A full wildcard disconnect(obj, 0, 0, 0) tears out internal destroyed-signal
    // connections that Qt (and the tree/list models) rely on for cleanup.
    disconnect(visualItem, nullptr, this, nullptr);
}

void MissionController::_itemCommandChanged(void)
{
    _recalcChildItems();
    emit _recalcFlightPathSegmentsSignal();
}

void MissionController::_managerVehicleChanged(Vehicle* managerVehicle)
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
    connect(_missionManager, &MissionManager::progressPctChanged,       this, &MissionController::_progressPctChanged);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &MissionController::_currentMissionIndexChanged);
    connect(_missionManager, &MissionManager::lastCurrentIndexChanged,  this, &MissionController::resumeMissionIndexChanged);
    connect(_missionManager, &MissionManager::resumeMissionReady,       this, &MissionController::resumeMissionReady);
    connect(_missionManager, &MissionManager::resumeMissionUploadFail,  this, &MissionController::resumeMissionUploadFail);
    connect(_managerVehicle, &Vehicle::defaultCruiseSpeedChanged,       this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(_managerVehicle, &Vehicle::defaultHoverSpeedChanged,        this, &MissionController::_recalcMissionFlightStatusSignal, Qt::QueuedConnection);
    connect(_managerVehicle, &Vehicle::vehicleTypeChanged,              this, &MissionController::complexMissionItemNamesChanged);

    emit complexMissionItemNamesChanged();
    emit resumeMissionIndexChanged();
}

void MissionController::_inProgressChanged(bool inProgress)
{
    emit syncInProgressChanged(inProgress);
}

bool MissionController::_findPreviousAltitude(int newIndex, double* prevAltitude, QGroundControlQmlGlobal::AltitudeFrame* prevAltitudeMode)
{
    bool                                found = false;
    double                              foundAltitude = 0;
    QGroundControlQmlGlobal::AltitudeFrame    foundAltFrame = QGroundControlQmlGlobal::AltitudeFrameNone;

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
                    foundAltitude   = simpleItem->altitude()->rawValue().toDouble();
                    foundAltFrame    = simpleItem->altitudeFrame();
                    found           = true;
                    break;
                }
            }
        }
    }

    if (found) {
        *prevAltitude       = foundAltitude;
        *prevAltitudeMode   = foundAltFrame;
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

    MissionSettingsItem* settingsItem = new MissionSettingsItem(_masterController, _flyView);
    visualItems->insert(0, settingsItem);

    if (visualItems == _visualItems) {
        _settingsItem = settingsItem;
    }

    return settingsItem;
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

void MissionController::_scanForAdditionalSettings(QmlObjectListModel* visualItems, PlanMasterController* masterController)
{
    // First we look for Landing Patterns which are at the end
    if (!FixedWingLandingComplexItem::scanForItems(visualItems, _flyView, masterController)) {
        VTOLLandingComplexItem::scanForItems(visualItems, _flyView, masterController);
    }

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
            simpleItem->scanForSections(visualItems, scanIndex, masterController);
        } else {
            // Complex item, can't have sections
            scanIndex++;
        }
    }
}

bool MissionController::containsItems(void) const
{
    return _visualItems ? _visualItems->count() > 1 : false;
}

void MissionController::removeAllFromVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::removeAllFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(MissionControllerLog) << "MissionControllerLog::removeAllFromVehicle called while syncInProgress";
    } else {
        _itemsRequested = true;
        _missionManager->removeAll();
    }
}

QStringList MissionController::complexMissionItemNames(void) const
{
    QStringList complexItems;

    complexItems.append(SurveyComplexItem::name);
    complexItems.append(CorridorScanComplexItem::name);
    if (_controllerVehicle->multiRotor() || _controllerVehicle->vtol()) {
        complexItems.append(StructureScanComplexItem::name);
    }

    // Note: The landing pattern items are not added here since they have there own button which adds them

    return QGCCorePlugin::instance()->complexMissionItemNames(_controllerVehicle, complexItems);
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
    if (!QGC::fuzzyCompare(progressPct, _progressPct)) {
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
        qCCritical(MissionControllerLog) << "MissionController::showPlanFromManagerVehicle called while offline";
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
            // Sync has already completed, fake a _newMissionItemsAvailable with the current items
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
        qCDebug(MissionControllerLog) << "setCurrentPlanViewSeqNum";
        bool    foundLand =             false;
        int     takeoffSeqNum =         -1;
        int     landSeqNum =            -1;
        int     lastFlyThroughSeqNum =  -1;

        _splitSegment =                 nullptr;
        _currentPlanViewItem  =         nullptr;
        _currentPlanViewSeqNum =        -1;
        _currentPlanViewVIIndex =       -1;
        _onlyInsertTakeoffValid =       false;
        _isInsertTakeoffValid =         true;
        _isInsertLandValid =            true;
        _isROIActive =                  false;
        _isROIBeginCurrentItem =        false;
        _flyThroughCommandsAllowed =    true;
        _previousCoordinate =           QGeoCoordinate();

        bool noItemsAddedYet = _visualItems->count() == 1;
        if (_masterController->controllerVehicle()->supports()->takeoffMissionCommand() && !_planViewSettings->takeoffItemNotRequired()->rawValue().toBool() && noItemsAddedYet) {
            _onlyInsertTakeoffValid = true;
        }

        for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
            VisualMissionItem*  pVI =        qobject_cast<VisualMissionItem*>(_visualItems->get(viIndex));
            SimpleMissionItem*  simpleItem = qobject_cast<SimpleMissionItem*>(pVI);
            int                 currentSeqNumber = pVI->sequenceNumber();

            if (sequenceNumber != 0 && currentSeqNumber <= sequenceNumber) {
                if (pVI->specifiesCoordinate() && !pVI->isStandaloneCoordinate()) {
                    // Coordinate based flight commands prior to where the takeoff would be inserted
                    _isInsertTakeoffValid = false;
                }
            }

            if (qobject_cast<TakeoffMissionItem*>(pVI)) {
                takeoffSeqNum = currentSeqNumber;
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
                        landSeqNum = currentSeqNumber;
                        break;
                    default:
                        break;
                    }
                } else {
                    FixedWingLandingComplexItem* fwLanding = qobject_cast<FixedWingLandingComplexItem*>(pVI);
                    if (fwLanding) {
                        foundLand = true;
                        landSeqNum = currentSeqNumber;
                    }
                }
            }

            if (simpleItem) {
                // Remember previous coordinate
                if (currentSeqNumber < sequenceNumber && simpleItem->specifiesCoordinate() && !simpleItem->isStandaloneCoordinate()) {
                    _previousCoordinate = simpleItem->coordinate();
                }

                // ROI state handling
                if (currentSeqNumber <= sequenceNumber) {
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
                if (currentSeqNumber == sequenceNumber && _isROIBeginItem(simpleItem)) {
                    _isROIBeginCurrentItem = true;
                }
            }

            if (viIndex != 0) {
                // Complex items are assumed to be fly through
                if (!simpleItem || (simpleItem->specifiesCoordinate() && !simpleItem->isStandaloneCoordinate())) {
                    lastFlyThroughSeqNum = currentSeqNumber;
                }
            }

            if (currentSeqNumber == sequenceNumber) {
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
                                if (_flightPathSegmentHashTable.contains(splitPair)) {
                                    _splitSegment = _flightPathSegmentHashTable[splitPair];
                                } else {
                                    // The recalc of flight path segments hasn't happened yet since it is delayed and compressed.
                                    // So we need to register the fact that we need a split segment update and it will happen in the recalc instead.
                                    _delayedSplitSegmentUpdate = true;
                                }
                                break;
                            }
                        }
                    }
                } else if (pVI->parentItem()) {
                    pVI->parentItem()->setHasCurrentChildItem(true);
                }
            } else {
                pVI->setIsCurrentItem(false);
                // Child items will always be later in the mission sequence, so this should be correct, to be sure the HasCurrentChildItem is cleared
                pVI->setHasCurrentChildItem(false);
            }
        }

        if (takeoffSeqNum != -1) {
            // Takeoff item was found which means mission starts from ground
            if (sequenceNumber < takeoffSeqNum) {
                // Land is only valid after the takeoff item.
                _isInsertLandValid = false;
                // Fly through commands are not allowed prior to the takeoff command
                _flyThroughCommandsAllowed = false;
            }
        }

        if (lastFlyThroughSeqNum != -1) {
            // Land item must be after any fly through coordinates
            if (sequenceNumber < lastFlyThroughSeqNum) {
                _isInsertLandValid = false;
            }
        }

        if (foundLand && !multipleLandPatternsAllowed()) {
            _isInsertLandValid = false;
            if (sequenceNumber >= landSeqNum) {
                // Can't have fly through commands after a land item
                _flyThroughCommandsAllowed = false;
            }
        }

        if (_hasLandItem != foundLand) {
            // Update hasLandItem if the presence of a landing item has changed
            _hasLandItem = foundLand;
            emit hasLandItemChanged();
        }

        // These are not valid when only takeoff is allowed
        _isInsertLandValid =            _isInsertLandValid && !_onlyInsertTakeoffValid;
        _flyThroughCommandsAllowed =    _flyThroughCommandsAllowed && !_onlyInsertTakeoffValid;

        // These 10 properties are all recomputed together above, so a single signal is sufficient.
        // QML property bindings list planViewStateChanged as their NOTIFY signal which means
        // one emit re-evaluates all dependent bindings in one pass instead of 10 separate updates.
        // splitSegmentChanged is kept separate because PlanView.qml has an explicit onSplitSegmentChanged handler.
        emit planViewStateChanged();
        emit splitSegmentChanged();
    }
}

void MissionController::repositionMission(const QGeoCoordinate& newHome,
                                          bool repositionTakeoffItems,
                                          bool repositionLandingItems)
{
    if (!newHome.isValid()) {
        qCWarning(MissionControllerLog) << "Cannot reposition mission to an invalid coordinate";
        return;
    }

    if (!_settingsItem || !_settingsItem->coordinate().isValid()) {
        qCWarning(MissionControllerLog) << "Cannot reposition mission while home is invalid";
        return;
    }

    const QGeoCoordinate oldHome = _settingsItem->coordinate();

    for (int i = 0; i < _visualItems->count(); ++i) {
        auto* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        if (!item || !item->specifiesCoordinate() || item->isStandaloneCoordinate()) {
            continue;
        }

        if ((!repositionTakeoffItems && item->isTakeoffItem()) ||
            (!repositionLandingItems && item->isLandCommand())) {
            continue;
        }

        const QGeoCoordinate oldCoord = item->coordinate();
        if (!oldCoord.isValid()) {
            continue;
        }

        const double distanceMeters = oldHome.distanceTo(oldCoord);
        const double azimuthDegrees = oldHome.azimuthTo(oldCoord);

        QGeoCoordinate newCoord = newHome.atDistanceAndAzimuth(distanceMeters, azimuthDegrees);
        item->setCoordinate(newCoord);
    }

    setDirty(true);
}

void MissionController::offsetMission(double eastMeters,
                                      double northMeters,
                                      double upMeters,
                                      bool offsetTakeoffItems,
                                      bool offsetLandingItems)
{
    if (!qFuzzyIsNull(eastMeters) || !qFuzzyIsNull(northMeters)) {
        const double distanceMeters = qSqrt(eastMeters * eastMeters + northMeters * northMeters);
        double azimuthDegrees = 0.0;
        if (!qFuzzyIsNull(distanceMeters)) {
            const double azimuthRadians = qAtan2(eastMeters, northMeters);
            azimuthDegrees = qRadiansToDegrees(azimuthRadians);
        }

        for (int i = 0; i < _visualItems->count(); ++i) {
            auto* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            if (!item || !item->specifiesCoordinate() || item->isStandaloneCoordinate()) {
                continue;
            }

            if ((!offsetTakeoffItems && item->isTakeoffItem()) ||
                (!offsetLandingItems && item->isLandCommand())) {
                continue;
            }

            const QGeoCoordinate oldCoord = item->coordinate();
            if (!oldCoord.isValid()) {
                continue;
            }

            item->setCoordinate(oldCoord.atDistanceAndAzimuth(distanceMeters, azimuthDegrees));
        }

        setDirty(true);
    }

    if (!qFuzzyIsNull(upMeters)) {
        for (int i = 0; i < _visualItems->count(); ++i) {
            auto* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
            if (!item || item == _settingsItem) {
                continue;
            }

            if ((!offsetTakeoffItems && item->isTakeoffItem()) ||
                (!offsetLandingItems && item->isLandCommand())) {
                continue;
            }

            if (!item->specifiesCoordinate() && !item->specifiesAltitudeOnly()) {
                continue;
            }

            if (!qIsNaN(item->editableAlt())) {
                item->applyNewAltitude(item->editableAlt() + upMeters);
            }
        }

        setDirty(true);
    }
}

void MissionController::rotateMission(double degreesCW,
                                      bool rotateTakeoffItems,
                                      bool rotateLandingItems)
{
    if (qFuzzyIsNull(degreesCW)) {
        return;
    }

    if (!_settingsItem || !_settingsItem->coordinate().isValid()) {
        qCWarning(MissionControllerLog) << "Cannot rotate mission while home is invalid";
        return;
    }

    const QGeoCoordinate home = _settingsItem->coordinate();

    for (int i = 0; i < _visualItems->count(); ++i) {
        auto* item = qobject_cast<VisualMissionItem*>(_visualItems->get(i));
        if (!item || !item->specifiesCoordinate() || item->isStandaloneCoordinate()) {
            continue;
        }

        if ((!rotateTakeoffItems && item->isTakeoffItem()) ||
            (!rotateLandingItems && item->isLandCommand())) {
            continue;
        }

        const QGeoCoordinate oldCoord = item->coordinate();
        if (!oldCoord.isValid()) {
            continue;
        }

        const double distanceMeters = home.distanceTo(oldCoord);
        if (qFuzzyIsNull(distanceMeters)) {
            continue;
        }
        const double azimuthDegrees = home.azimuthTo(oldCoord);

        QGeoCoordinate newCoord = home.atDistanceAndAzimuth(distanceMeters, azimuthDegrees + degreesCW);
        item->setCoordinate(newCoord);
    }

    setDirty(true);
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
                        double alt = 0.0;
                        if (!pSimpleItem->altitude()->rawValue().isNull() && !qIsNaN(pSimpleItem->altitude()->rawValue().toDouble())) {
                            alt = pSimpleItem->altitude()->rawValue().toDouble();
                        }
                        if((MAV_CMD)pSimpleItem->command() == MAV_CMD_NAV_TAKEOFF) {
                            takeoffCoordinate = pSimpleItem->coordinate();
                            takeoffCoordinate.setAltitude(alt);
                            minAlt = maxAlt = alt;
                        } else if(!firstCoordinate.isValid()) {
                            firstCoordinate = pSimpleItem->coordinate();
                        }
                        double lat = pSimpleItem->coordinate().latitude()  + 90.0;
                        double lon = pSimpleItem->coordinate().longitude() + 180.0;
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
                    if(!firstCoordinate.isValid() && pComplexItem->entryCoordinate().isValid()) {
                        firstCoordinate = pComplexItem->entryCoordinate();
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

bool MissionController::isFirstLandingComplexItem(const LandingComplexItem *item) const {
    const int count = _visualItems->count();

    for (int i = 0; i < count; i++) {
        QObject *obj = _visualItems->get(i);

        LandingComplexItem *landingItem = qobject_cast<LandingComplexItem *>(obj);
        if (landingItem) {
            return landingItem == item;
        }
    }

    return false;
}

bool MissionController::isEmpty(void) const
{
    return _visualItems->count() <= 1;
}

void MissionController::_forceRecalcOfAllowedBits(void)
{
    // Force a recalc of allowed bits
    setCurrentPlanViewSeqNum(_currentPlanViewSeqNum, true /* force */);
}

QString MissionController::surveyComplexItemName(void) const
{
    return SurveyComplexItem::name;
}

QString MissionController::corridorScanComplexItemName(void) const
{
    return CorridorScanComplexItem::name;
}

QString MissionController::structureScanComplexItemName(void) const
{
    return StructureScanComplexItem::name;
}

void MissionController::_allItemsRemoved(void)
{
    // When there are no mission items we track changes to firmware/vehicle type. This allows a vehicle connection
    // to adjust these items.
    _controllerVehicle->trackFirmwareVehicleTypeChanges();
}

void MissionController::_firstItemAdded(void)
{
    // As soon as the first item is added we lock the firmware/vehicle type to current values. So if you then connect a vehicle
    // it will not affect these values.
    _controllerVehicle->stopTrackingFirmwareVehicleTypeChanges();
}

MissionController::SendToVehiclePreCheckState MissionController::sendToVehiclePreCheck(void)
{
    if (_managerVehicle->isOfflineEditingVehicle()) {
        return SendToVehiclePreCheckStateNoActiveVehicle;
    }
    if (_managerVehicle->armed() && _managerVehicle->flightMode() == _managerVehicle->missionFlightMode()) {
        return SendToVehiclePreCheckStateActiveMission;
    }
    if (_controllerVehicle->firmwareType() != _managerVehicle->firmwareType() || QGCMAVLink::vehicleClass(_controllerVehicle->vehicleType()) != QGCMAVLink::vehicleClass(_managerVehicle->vehicleType())) {
        return SendToVehiclePreCheckStateFirwmareVehicleMismatch;
    }
    return SendToVehiclePreCheckStateOk;
}

QGroundControlQmlGlobal::AltitudeFrame MissionController::globalAltitudeFrame(void)
{
    return _globalAltFrame;
}

QGroundControlQmlGlobal::AltitudeFrame MissionController::globalAltitudeFrameDefault(void)
{
    if (_globalAltFrame == QGroundControlQmlGlobal::AltitudeFrameMixed) {
        return QGroundControlQmlGlobal::AltitudeFrameRelative;
    } else {
        return _globalAltFrame;
    }
}

void MissionController::setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altFrame)
{
    if (_globalAltFrame != altFrame) {
        _globalAltFrame = altFrame;
        emit globalAltitudeFrameChanged();
    }
}
