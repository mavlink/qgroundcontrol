#include "PlanMasterController.h"
#include "AppMessages.h"
#include "QGCCorePlugin.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "JsonParsing.h"
#include "MissionManager.h"
#include "KMLPlanDomDocument.h"
#include "PlanCreator.h"
#include "QmlObjectListModel.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>

QGC_LOGGING_CATEGORY(PlanMasterControllerLog, "PlanManager.PlanMasterController")

PlanMasterController::PlanMasterController(QObject* parent)
    : QObject               (parent)
    , _multiVehicleMgr      (MultiVehicleManager::instance())
    , _controllerVehicle    (new Vehicle(Vehicle::MAV_AUTOPILOT_TRACK, Vehicle::MAV_TYPE_TRACK, this))
    , _managerVehicle       (_controllerVehicle)
    , _missionController    (this)
    , _geoFenceController   (this)
    , _rallyPointController (this)
{
    _commonInit();
}

#ifdef QGC_UNITTEST_BUILD
PlanMasterController::PlanMasterController(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, QObject* parent)
    : QObject               (parent)
    , _multiVehicleMgr      (MultiVehicleManager::instance())
    , _controllerVehicle    (new Vehicle(firmwareType, vehicleType))
    , _managerVehicle       (_controllerVehicle)
    , _missionController    (this)
    , _geoFenceController   (this)
    , _rallyPointController (this)
{
    _commonInit();
}
#endif

void PlanMasterController::_commonInit(void)
{
    connect(&_missionController,    &MissionController::dirtyChanged,               this, &PlanMasterController::_updateOverallDirty);
    connect(&_geoFenceController,   &GeoFenceController::dirtyChanged,              this, &PlanMasterController::_updateOverallDirty);
    connect(&_rallyPointController, &RallyPointController::dirtyChanged,            this, &PlanMasterController::_updateOverallDirty);

    connect(&_missionController,    &MissionController::containsItemsChanged,       this, &PlanMasterController::containsItemsChanged);
    connect(&_geoFenceController,   &GeoFenceController::containsItemsChanged,      this, &PlanMasterController::containsItemsChanged);
    connect(&_rallyPointController, &RallyPointController::containsItemsChanged,    this, &PlanMasterController::containsItemsChanged);

    connect(this, &PlanMasterController::containsItemsChanged, this, &PlanMasterController::_updateShowCreateFromTemplate);

    connect(&_missionController,    &MissionController::syncInProgressChanged,      this, &PlanMasterController::syncInProgressChanged);
    connect(&_geoFenceController,   &GeoFenceController::syncInProgressChanged,     this, &PlanMasterController::syncInProgressChanged);
    connect(&_rallyPointController, &RallyPointController::syncInProgressChanged,   this, &PlanMasterController::syncInProgressChanged);

    // Offline vehicle can change firmware/vehicle type
    connect(_controllerVehicle,     &Vehicle::vehicleTypeChanged,                   this, &PlanMasterController::_updatePlanCreatorsList);
}


PlanMasterController::~PlanMasterController()
{

}

void PlanMasterController::start(void)
{
    _missionController.start    (_flyView);
    _geoFenceController.start   (_flyView);
    _rallyPointController.start (_flyView);

    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanMasterController::_activeVehicleChanged);

    _updatePlanCreatorsList();
}

void PlanMasterController::startStaticActiveVehicle(Vehicle* vehicle, bool deleteWhenSendCompleted)
{
    _flyView = true;
    _deleteWhenSendCompleted = deleteWhenSendCompleted;
    _missionController.start(_flyView);
    _geoFenceController.start(_flyView);
    _rallyPointController.start(_flyView);
    _activeVehicleChanged(vehicle);
}

void PlanMasterController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_managerVehicle == activeVehicle) {
        // We are already setup for this vehicle
        return;
    }

    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged" << activeVehicle;

    if (_managerVehicle) {
        // Disconnect old vehicle. Be careful of wildcarding disconnect too much since _managerVehicle may equal _controllerVehicle
        disconnect(_managerVehicle,                         &Vehicle::initialPlanRequestCompleteChanged, this, nullptr);
        disconnect(_managerVehicle->missionManager(),       nullptr, this, nullptr);
        disconnect(_managerVehicle->geoFenceManager(),      nullptr, this, nullptr);
        disconnect(_managerVehicle->rallyPointManager(),    nullptr, this, nullptr);

        // Any in-flight transfer chain can never complete against the new vehicle's managers
        _loadSequence = SyncSequence::Idle;
        _sendSequence = SyncSequence::Idle;
    }

    bool newOffline = false;
    if (activeVehicle == nullptr) {
        // Since there is no longer an active vehicle we use the offline controller vehicle as the manager vehicle
        _managerVehicle = _controllerVehicle;
        newOffline = true;
    } else {
        newOffline = false;
        _managerVehicle = activeVehicle;

        // Update controllerVehicle to the currently connected vehicle
        AppSettings* appSettings = SettingsManager::instance()->appSettings();
        appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(_managerVehicle->firmwareType()));
        appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(_managerVehicle->vehicleType()));

        // We use these signals to sequence upload and download to the multiple controller/managers
        connect(_managerVehicle,                        &Vehicle::initialPlanRequestCompleteChanged, this, &PlanMasterController::_initialPlanRequestCompleteChanged);
        connect(_managerVehicle->missionManager(),      &MissionManager::newMissionItemsAvailable,  this, &PlanMasterController::_loadMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::loadComplete,             this, &PlanMasterController::_loadGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::loadComplete,           this, &PlanMasterController::_loadRallyPointsComplete);
        connect(_managerVehicle->missionManager(),      &MissionManager::sendComplete,              this, &PlanMasterController::_sendMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::sendComplete,             this, &PlanMasterController::_sendGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::sendComplete,           this, &PlanMasterController::_sendRallyPointsComplete);
    }

    _offline = newOffline;
    emit offlineChanged(offline());
    emit managerVehicleChanged(_managerVehicle);

    if (_flyView) {
        // We are in the Fly View
        if (newOffline) {
            // No active vehicle, clear mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly View - No active vehicle, clearing stale plan";
            removeAll();
        } else {
            // Fly view has changed to a new active vehicle, update to show correct mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly View - New active vehicle, loading new plan from manager vehicle";
            _showPlanFromManagerVehicle();
        }
    } else {
        // We are in the Plan view.
        if (containsItems()) {
            // We have a plan which is from a different vehicle than the new active vehicle. By definition this plan requires and upload.
            _setDirtyForUpload(true);

            // The plan view has a stale plan in it
            if (dirtyForSave()) {
                // Plan is dirty, the user must decide what to do in all cases
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous dirty plan exists, no new active vehicle, sending promptForPlanUsageOnVehicleChange signal";
                emit promptForPlanUsageOnVehicleChange();
            } else {
                // Plan is not dirty
                if (newOffline) {
                    // The active vehicle went away with no new active vehicle
                    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous clean plan exists, no new active vehicle, clear stale plan";
                    removeAll();
                } else {
                    // We are transitioning from one active vehicle to another. Show the plan from the new vehicle.
                    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous clean plan exists, new active vehicle, loading from new manager vehicle";
                    _showPlanFromManagerVehicle();
                }
            }
        } else {
            // There is no previous Plan in the view
            _setDirtyStates(false, false);
            if (newOffline) {
                // Nothing special to do in this case
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - No previous plan, no longer connected to vehicle, nothing to do";
            } else {
                // Just show the plan from the new vehicle
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - No previous plan, new active vehicle, loading from new manager vehicle";
                _showPlanFromManagerVehicle();
            }
        }
    }

    // Vehicle changed so we need to signal everything
    emit containsItemsChanged();
    emit syncInProgressChanged();
    emit dirtyForSaveChanged(dirtyForSave());
    emit dirtyForUploadChanged(dirtyForUpload());

    _updatePlanCreatorsList();
}

void PlanMasterController::loadFromVehicle(void)
{
    SharedLinkInterfacePtr sharedLink = _managerVehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        if (sharedLink->linkConfiguration()->isHighLatency()) {
            QGC::showAppMessage(tr("Download not supported on high latency links."));
            return;
        }
    } else {
        // Vehicle is shutting down
        return;
    }

    if (offline()) {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while offline";
    } else if (_flyView) {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called from Fly view";
    } else if (syncInProgress()) {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while syncInProgress";
    } else {
        _loadSequence = SyncSequence::Mission;
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle calling _missionController.loadFromVehicle";
        _missionController.loadFromVehicle();
    }
}


void PlanMasterController::_loadMissionComplete(void)
{
    if (_loadSequence != SyncSequence::Mission) {
        return;
    }
    _loadSequence = SyncSequence::GeoFence;
    if (_geoFenceController.supported()) {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete calling _geoFenceController.loadFromVehicle";
        _geoFenceController.loadFromVehicle();
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete GeoFence not supported skipping";
        _geoFenceController.removeAll();
        _loadGeoFenceComplete();
    }
}

void PlanMasterController::_loadGeoFenceComplete(void)
{
    if (_loadSequence != SyncSequence::GeoFence) {
        return;
    }
    _loadSequence = SyncSequence::RallyPoints;
    if (_rallyPointController.supported()) {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadGeoFenceComplete calling _rallyPointController.loadFromVehicle";
        _rallyPointController.loadFromVehicle();
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadGeoFenceComplete Rally Points not supported skipping";
        _rallyPointController.removeAll();
        _loadRallyPointsComplete();
    }
}

void PlanMasterController::_loadRallyPointsComplete(void)
{
    if (_loadSequence != SyncSequence::RallyPoints) {
        return;
    }
    _loadSequence = SyncSequence::Idle;
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadRallyPointsComplete";
    // A plan just downloaded from the vehicle reflects exactly what is on the vehicle.
    // The user has made no edits, so it must not be dirty for save or upload. Any previous
    // file association no longer describes the editor contents.
    _clearCurrentPlanFile();
    _setDirtyStates(false /* dirtyForSave */, false /* dirtyForUpload */);
}

void PlanMasterController::_sendMissionComplete(void)
{
    if (_sendSequence != SyncSequence::Mission) {
        return;
    }
    _sendSequence = SyncSequence::GeoFence;
    if (_geoFenceController.supported()) {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start GeoFence sendToVehicle";
        _geoFenceController.sendToVehicle();
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle GeoFence not supported skipping";
        _sendGeoFenceComplete();
    }
}

void PlanMasterController::_sendGeoFenceComplete(void)
{
    if (_sendSequence != SyncSequence::GeoFence) {
        return;
    }
    _sendSequence = SyncSequence::RallyPoints;
    if (_rallyPointController.supported()) {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start rally sendToVehicle";
        _rallyPointController.sendToVehicle();
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Points not support skipping";
        _sendRallyPointsComplete();
    }
}

void PlanMasterController::_sendRallyPointsComplete(void)
{
    if (_sendSequence != SyncSequence::RallyPoints) {
        return;
    }
    _sendSequence = SyncSequence::Idle;
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Point send complete";
    _setDirtyForUpload(false);
    if (_deleteWhenSendCompleted) {
        this->deleteLater();
    }
}

void PlanMasterController::sendToVehicle(void)
{
    SharedLinkInterfacePtr sharedLink = _managerVehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        if (sharedLink->linkConfiguration()->isHighLatency()) {
            QGC::showAppMessage(tr("Upload not supported on high latency links."));
            return;
        }
    } else {
        // Vehicle is shutting down
        return;
    }

    if (offline()) {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start mission sendToVehicle";
        _sendSequence = SyncSequence::Mission;
        _missionController.sendToVehicle();
    }
}

void PlanMasterController::loadFromFile(const QString& filename)
{
    QString errorString;
    QString errorMessage = tr("Error loading Plan file (%1). %2").arg(filename).arg("%1");

    if (filename.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        QGC::showAppMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        success = _missionController.loadTextFile(file, errorString);
    } else {
        success = _loadPlanJson(file.readAll(), errorString);
    }
    if (!success) {
        QGC::showAppMessage(errorMessage.arg(errorString));
    }

    if (success){
        _currentPlanFile = QString::asprintf("%s/%s.%s", fileInfo.path().toLocal8Bit().data(), fileInfo.completeBaseName().toLocal8Bit().data(), AppSettings::planFileExtension);
        emit currentPlanFileChanged();
        _setDirtyStates(false /* dirtyForSave */, true /* dirtyForUpload */);
    } else {
        _clearCurrentPlanFile();
    }
}

bool PlanMasterController::_loadPlanJson(const QByteArray& bytes, QString& errorString)
{
    QJsonDocument jsonDoc;
    if (!JsonParsing::isJsonFile(bytes, jsonDoc, errorString)) {
        return false;
    }

    QJsonObject json = jsonDoc.object();
    //-- Allow plugins to pre process the load
    QGCCorePlugin::instance()->preLoadFromJson(this, json);

    int version;
    if (!JsonParsing::validateExternalQGCJsonFile(json, kPlanFileType, kPlanFileVersion, kPlanFileVersion, version, errorString)) {
        return false;
    }

    const QList<JsonParsing::KeyValidateInfo> rgKeyInfo = {
        { kJsonMissionObjectKey,        QJsonValue::Object, true },
        { kJsonGeoFenceObjectKey,       QJsonValue::Object, true },
        { kJsonRallyPointsObjectKey,    QJsonValue::Object, true },
    };
    if (!JsonParsing::validateKeys(json, rgKeyInfo, errorString)) {
        return false;
    }

    if (!_missionController.load(json[kJsonMissionObjectKey].toObject(), errorString) ||
            !_geoFenceController.load(json[kJsonGeoFenceObjectKey].toObject(), errorString) ||
            !_rallyPointController.load(json[kJsonRallyPointsObjectKey].toObject(), errorString)) {
        return false;
    }

    //-- Allow plugins to post process the load
    QGCCorePlugin::instance()->postLoadFromJson(this, json);
    return true;
}

QJsonDocument PlanMasterController::saveToJson()
{
    QJsonObject planJson;
    QGCCorePlugin::instance()->preSaveToJson(this, planJson);
    QJsonObject missionJson;
    QJsonObject fenceJson;
    QJsonObject rallyJson;
    JsonParsing::saveQGCJsonFileHeader(planJson, kPlanFileType, kPlanFileVersion);
    //-- Allow plugin to preemptly add its own keys to mission
    QGCCorePlugin::instance()->preSaveToMissionJson(this, missionJson);
    _missionController.save(missionJson);
    //-- Allow plugin to add its own keys to mission
    QGCCorePlugin::instance()->postSaveToMissionJson(this, missionJson);
    _geoFenceController.save(fenceJson);
    _rallyPointController.save(rallyJson);
    planJson[kJsonMissionObjectKey] = missionJson;
    planJson[kJsonGeoFenceObjectKey] = fenceJson;
    planJson[kJsonRallyPointsObjectKey] = rallyJson;
    QGCCorePlugin::instance()->postSaveToJson(this, planJson);
    return QJsonDocument(planJson);
}

bool
PlanMasterController::saveToCurrent()
{
    if (!_currentPlanFile.isEmpty()) {
        const bool saveSuccess = saveToFile(_currentPlanFile);
        return saveSuccess;
    }

    return false;
}

bool PlanMasterController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return false;
    }

    QString planFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        planFilename += QString(".%1").arg(fileExtension());
    }

    QFile file(planFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QGC::showAppMessage(tr("Plan save error %1 : %2").arg(filename).arg(file.errorString()));
        return false;
    } else {
        const QByteArray saveBytes = saveToJson().toJson();
        const qint64 bytesWritten = file.write(saveBytes);
        if (bytesWritten != saveBytes.size()) {
            QGC::showAppMessage(tr("Plan save error %1 : %2").arg(filename).arg(file.errorString()));
            return false;
        }
        if(_currentPlanFile != planFilename) {
            _currentPlanFile = planFilename;
            emit currentPlanFileChanged();
        }
        _setDirtyForSave(false);
    }

    return true;
}

void PlanMasterController::saveToKml(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString kmlFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        kmlFilename += QString(".%1").arg(kmlFileExtension());
    }

    QFile file(kmlFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QGC::showAppMessage(tr("KML save error %1 : %2").arg(filename).arg(file.errorString()));
    } else {
        KMLPlanDomDocument planKML;
        _missionController.addMissionToKML(planKML);
        QTextStream stream(&file);
        stream << planKML.toString();
        file.close();
    }
}

void PlanMasterController::removeAll(void)
{
    _suppressOverallDirtyUpdate = true;
    _missionController.removeAll();
    _geoFenceController.removeAll();
    _rallyPointController.removeAll();
    _missionController.setDirty(false);
    _geoFenceController.setDirty(false);
    _rallyPointController.setDirty(false);
    _suppressOverallDirtyUpdate = false;

    _setDirtyStates(false, false);
    if (_offline) {
        _clearCurrentPlanFile();
    }
    setUserSelectedManualCreation(false);
}

void PlanMasterController::removeAllFromVehicle(void)
{
    if (!offline()) {
        _missionController.removeAllFromVehicle();
        if (_geoFenceController.supported()) {
            _geoFenceController.removeAllFromVehicle();
        }
        if (_rallyPointController.supported()) {
            _rallyPointController.removeAllFromVehicle();
        }
        _setDirtyForUpload(false);
        _clearCurrentPlanFile();
    } else {
        qCCritical(PlanMasterControllerLog) << "PlanMasterController::removeAllFromVehicle called while offline";
    }
    setUserSelectedManualCreation(false);
}

bool PlanMasterController::containsItems(void) const
{
    return _missionController.containsItems() || _geoFenceController.containsItems() || _rallyPointController.containsItems();
}

void PlanMasterController::_updateShowCreateFromTemplate(void)
{
    // When the plan becomes empty, always return to template-selection mode regardless
    // of how the items were removed.
    if (!containsItems() && _userSelectedManualCreation) {
        _userSelectedManualCreation = false;
        emit userSelectedManualCreationChanged();
    }
    const bool show = showCreateFromTemplate();
    if (show != _showCreateFromTemplate) {
        _showCreateFromTemplate = show;
        emit showCreateFromTemplateChanged();
    }
}

QString PlanMasterController::fileExtension(void) const
{
    return AppSettings::planFileExtension;
}

QString PlanMasterController::currentPlanFileName(void) const
{
    return _currentPlanFile.isEmpty() ? QString() : QFileInfo(_currentPlanFile).completeBaseName();
}

void PlanMasterController::_clearCurrentPlanFile()
{
    if (!_currentPlanFile.isEmpty()) {
        _currentPlanFile.clear();
        emit currentPlanFileChanged();
    }
}

QString PlanMasterController::kmlFileExtension(void) const
{
    return AppSettings::kmlFileExtension;
}

QStringList PlanMasterController::loadNameFilters(void) const
{
    QStringList filters;

    filters << tr("Supported types (*.%1 *.%2 *.%3)").arg(AppSettings::planFileExtension).arg(AppSettings::waypointsFileExtension).arg("txt") <<
               tr("All Files (*)");
    return filters;
}


QStringList PlanMasterController::saveNameFilters(void) const
{
    QStringList filters;

    filters << tr("Plan Files (*.%1)").arg(fileExtension()) << tr("All Files (*)");
    return filters;
}

void PlanMasterController::sendPlanToVehicle(Vehicle* vehicle, const QString& filename)
{
    // Use a transient PlanMasterController to accomplish this
    PlanMasterController* controller = new PlanMasterController();
    controller->startStaticActiveVehicle(vehicle, true /* deleteWhenSendCompleted */);
    controller->loadFromFile(filename);
    controller->sendToVehicle();
}

void PlanMasterController::_showPlanFromManagerVehicle(void)
{
    if (!_managerVehicle->initialPlanRequestComplete()) {
        // The sub-controllers pick up the editor contents as the initial download arrives
        _loadSequence = SyncSequence::InitialPlanLoad;
        return;
    }

    // The crazy if structure is to handle the load propagating by itself through the system
    if (!_missionController.showPlanFromManagerVehicle()) {
        if (!_geoFenceController.showPlanFromManagerVehicle()) {
            _rallyPointController.showPlanFromManagerVehicle();
        }
    }

    // The editor now shows the vehicle's plan: not dirty, and any previous file
    // association no longer describes the contents
    _missionController.setDirty(false);
    _geoFenceController.setDirty(false);
    _rallyPointController.setDirty(false);
    _clearCurrentPlanFile();
    _setDirtyStates(false, false);
}

void PlanMasterController::_initialPlanRequestCompleteChanged(bool initialPlanRequestComplete)
{
    if (!initialPlanRequestComplete || _loadSequence != SyncSequence::InitialPlanLoad) {
        return;
    }
    _loadSequence = SyncSequence::Idle;
    qCDebug(PlanMasterControllerLog) << "_initialPlanRequestCompleteChanged: initial download complete";
    // Import the manager data into the editor (non-empty sub-controllers reject the automatic
    // manager updates during download) and scrub dirty/file association.
    _showPlanFromManagerVehicle();
}

bool PlanMasterController::syncInProgress(void) const
{
    return _missionController.syncInProgress() ||
            _geoFenceController.syncInProgress() ||
            _rallyPointController.syncInProgress();
}

bool PlanMasterController::isEmpty(void) const
{
    return _missionController.isEmpty() &&
            _geoFenceController.isEmpty() &&
            _rallyPointController.isEmpty();
}

void PlanMasterController::_updateOverallDirty(void)
{
    if (syncInProgress() || _suppressOverallDirtyUpdate) {
        return;
    }

    const bool saveDirty = _missionController.dirty() || _geoFenceController.dirty() || _rallyPointController.dirty();
    if (saveDirty) {
        _setDirtyForSave(true);
    }
}

void PlanMasterController::_setDirtyForSave(bool dirtyForSave)
{
    if (_dirtyForSave != dirtyForSave) {
        _dirtyForSave = dirtyForSave;
        emit dirtyForSaveChanged(_dirtyForSave);

        if (_dirtyForSave) {
            _setDirtyForUpload(true);
        }
    }
}

void PlanMasterController::_setDirtyForUpload(bool dirtyForUpload)
{
    if (_dirtyForUpload != dirtyForUpload) {
        _dirtyForUpload = dirtyForUpload;
        emit dirtyForUploadChanged(_dirtyForUpload);
    }
}

void PlanMasterController::_setDirtyStates(bool dirtyForSave, bool dirtyForUpload)
{
    const bool saveChanged = (_dirtyForSave != dirtyForSave);
    const bool uploadChanged = (_dirtyForUpload != dirtyForUpload);

    _dirtyForSave = dirtyForSave;
    _dirtyForUpload = dirtyForUpload;

    if (saveChanged) {
        emit dirtyForSaveChanged(_dirtyForSave);
    }
    if (uploadChanged) {
        emit dirtyForUploadChanged(_dirtyForUpload);
    }
}

void PlanMasterController::_updatePlanCreatorsList(void)
{
    if (_flyView) {
        return;
    }

    const auto vehicleClass = _managerVehicle->vehicleClass();

    // Only rebuild if the vehicle class actually changed
    if (_planCreators && _planCreatorsVehicleClass == vehicleClass) {
        return;
    }

    if (!_planCreators) {
        _planCreators = new QmlObjectListModel(this);
    } else {
        _planCreators->clearAndDeleteContents();
    }

    _planCreatorsVehicleClass = vehicleClass;

    // Allow custom builds to provide their own list of plan creators
    const QList<PlanCreator*> creators = QGCCorePlugin::instance()->planCreators(this);

    // Filter by vehicle class and add to the model
    for (PlanCreator* creator : creators) {
        if (creator->supportsVehicleClass(vehicleClass)) {
            _planCreators->append(creator);
        } else {
            delete creator;
        }
    }

    emit planCreatorsChanged(_planCreators);
}

void PlanMasterController::showPlanFromManagerVehicle(void)
{
    if (offline()) {
        // There is no new vehicle so clear any previous plan
        qCDebug(PlanMasterControllerLog) << "showPlanFromManagerVehicle: Plan View - No new vehicle, clear any previous plan";
        removeAll();
    } else {
        // We have a new active vehicle, show the plan from that
        qCDebug(PlanMasterControllerLog) << "showPlanFromManagerVehicle: Plan View - New vehicle available, show plan from new manager vehicle";
        _showPlanFromManagerVehicle();
    }
}

void PlanMasterController::setUserSelectedManualCreation(bool userSelectedManualCreation)
{
    if (_userSelectedManualCreation != userSelectedManualCreation) {
        _userSelectedManualCreation = userSelectedManualCreation;
        emit userSelectedManualCreationChanged();
        // Update showCreateFromTemplate directly — do not go through _updateShowCreateFromTemplate,
        // which would immediately auto-clear the flag if the plan happens to be empty right now.
        const bool show = showCreateFromTemplate();
        if (show != _showCreateFromTemplate) {
            _showCreateFromTemplate = show;
            emit showCreateFromTemplateChanged();
        }
    }
}
