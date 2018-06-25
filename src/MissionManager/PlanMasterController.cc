/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "JsonHelper.h"
#include "MissionManager.h"
#include "KML.h"

#include <QDomDocument>
#include <QJsonDocument>
#include <QFileInfo>

QGC_LOGGING_CATEGORY(PlanMasterControllerLog, "PlanMasterControllerLog")

const int   PlanMasterController::_planFileVersion =            1;
const char* PlanMasterController::_planFileType =               "Plan";
const char* PlanMasterController::_jsonMissionObjectKey =       "mission";
const char* PlanMasterController::_jsonGeoFenceObjectKey =      "geoFence";
const char* PlanMasterController::_jsonRallyPointsObjectKey =   "rallyPoints";

PlanMasterController::PlanMasterController(QObject* parent)
    : QObject               (parent)
    , _multiVehicleMgr      (qgcApp()->toolbox()->multiVehicleManager())
    , _controllerVehicle    (new Vehicle((MAV_AUTOPILOT)qgcApp()->toolbox()->settingsManager()->appSettings()->offlineEditingFirmwareType()->rawValue().toInt(), (MAV_TYPE)qgcApp()->toolbox()->settingsManager()->appSettings()->offlineEditingVehicleType()->rawValue().toInt(), qgcApp()->toolbox()->firmwarePluginManager()))
    , _managerVehicle       (_controllerVehicle)
    , _flyView              (true)
    , _offline              (true)
    , _missionController    (this)
    , _geoFenceController   (this)
    , _rallyPointController (this)
    , _loadGeoFence         (false)
    , _loadRallyPoints      (false)
    , _sendGeoFence         (false)
    , _sendRallyPoints      (false)
{
    connect(&_missionController,    &MissionController::dirtyChanged,       this, &PlanMasterController::dirtyChanged);
    connect(&_geoFenceController,   &GeoFenceController::dirtyChanged,      this, &PlanMasterController::dirtyChanged);
    connect(&_rallyPointController, &RallyPointController::dirtyChanged,    this, &PlanMasterController::dirtyChanged);

    connect(&_missionController,    &MissionController::containsItemsChanged,       this, &PlanMasterController::containsItemsChanged);
    connect(&_geoFenceController,   &GeoFenceController::containsItemsChanged,      this, &PlanMasterController::containsItemsChanged);
    connect(&_rallyPointController, &RallyPointController::containsItemsChanged,    this, &PlanMasterController::containsItemsChanged);

    connect(&_missionController,    &MissionController::syncInProgressChanged,      this, &PlanMasterController::syncInProgressChanged);
    connect(&_geoFenceController,   &GeoFenceController::syncInProgressChanged,     this, &PlanMasterController::syncInProgressChanged);
    connect(&_rallyPointController, &RallyPointController::syncInProgressChanged,   this, &PlanMasterController::syncInProgressChanged);
}

PlanMasterController::~PlanMasterController()
{

}

void PlanMasterController::start(bool flyView)
{
    _flyView = flyView;
    _missionController.start(_flyView);
    _geoFenceController.start(_flyView);
    _rallyPointController.start(_flyView);

    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanMasterController::_activeVehicleChanged);
    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
}

void PlanMasterController::startStaticActiveVehicle(Vehicle* vehicle)
{
    _flyView = true;
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
        // Disconnect old vehicle
        disconnect(_managerVehicle->missionManager(),       &MissionManager::newMissionItemsAvailable,  this, &PlanMasterController::_loadMissionComplete);
        disconnect(_managerVehicle->geoFenceManager(),      &GeoFenceManager::loadComplete,             this, &PlanMasterController::_loadGeoFenceComplete);
        disconnect(_managerVehicle->rallyPointManager(),    &RallyPointManager::loadComplete,           this, &PlanMasterController::_loadRallyPointsComplete);
        disconnect(_managerVehicle->missionManager(),       &MissionManager::sendComplete,              this, &PlanMasterController::_sendMissionComplete);
        disconnect(_managerVehicle->geoFenceManager(),      &GeoFenceManager::sendComplete,             this, &PlanMasterController::_sendGeoFenceComplete);
        disconnect(_managerVehicle->rallyPointManager(),    &RallyPointManager::sendComplete,           this, &PlanMasterController::_sendRallyPointsComplete);
    }

    bool newOffline = false;
    if (activeVehicle == NULL) {
        // Since there is no longer an active vehicle we use the offline controller vehicle as the manager vehicle
        _managerVehicle = _controllerVehicle;
        newOffline = true;
    } else {
        newOffline = false;
        _managerVehicle = activeVehicle;

        // Update controllerVehicle to the currently connected vehicle
        AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
        appSettings->offlineEditingFirmwareType()->setRawValue(AppSettings::offlineEditingFirmwareTypeFromFirmwareType(_managerVehicle->firmwareType()));
        appSettings->offlineEditingVehicleType()->setRawValue(AppSettings::offlineEditingVehicleTypeFromVehicleType(_managerVehicle->vehicleType()));

        // We use these signals to sequence upload and download to the multiple controller/managers
        connect(_managerVehicle->missionManager(),      &MissionManager::newMissionItemsAvailable,  this, &PlanMasterController::_loadMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::loadComplete,             this, &PlanMasterController::_loadGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::loadComplete,           this, &PlanMasterController::_loadRallyPointsComplete);
        connect(_managerVehicle->missionManager(),      &MissionManager::sendComplete,              this, &PlanMasterController::_sendMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::sendComplete,             this, &PlanMasterController::_sendGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::sendComplete,           this, &PlanMasterController::_sendRallyPointsComplete);
    }

    _missionController.managerVehicleChanged(_managerVehicle);
    _geoFenceController.managerVehicleChanged(_managerVehicle);
    _rallyPointController.managerVehicleChanged(_managerVehicle);

    // Vehicle changed so we need to signal everything
    _offline = newOffline;
    emit containsItemsChanged(containsItems());
    emit syncInProgressChanged();
    emit dirtyChanged(dirty());
    emit offlineChanged(offline());

    if (!_flyView) {
        if (!offline()) {
            // We are in Plan view and we have a newly connected vehicle:
            //  - If there is no plan available in Plan view show the one from the vehicle
            //  - Otherwise leave the current plan alone
            if (!containsItems()) {
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan view is empty so loading from manager";
                _showPlanFromManagerVehicle();
            }
        }
    } else {
        if (offline()) {
            // No more active vehicle, clear mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly view is offline clearing plan";
            removeAll();
        } else {
            // Fly view has changed to a new active vehicle, update to show correct mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly view is online so loading from manager";
            _showPlanFromManagerVehicle();
        }
    }
}

void PlanMasterController::loadFromVehicle(void)
{
    if (_managerVehicle->highLatencyLink()) {
        qgcApp()->showMessage(tr("Download not supported on high latency links."));
        return;
    }

    if (offline()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while offline";
    } else if (_flyView) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called from Fly view";
    } else if (syncInProgress()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while syncInProgress";
    } else {
        _loadGeoFence = true;
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle calling _missionController.loadFromVehicle";
        _missionController.loadFromVehicle();
        setDirty(false);
    }
}


void PlanMasterController::_loadMissionComplete(void)
{
    if (!_flyView && _loadGeoFence) {
        _loadGeoFence = false;
        _loadRallyPoints = true;
        if (_geoFenceController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete calling _geoFenceController.loadFromVehicle";
            _geoFenceController.loadFromVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete GeoFence not supported skipping";
            _geoFenceController.removeAll();
            _loadGeoFenceComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_loadGeoFenceComplete(void)
{
    if (!_flyView && _loadRallyPoints) {
        _loadRallyPoints = false;
        if (_rallyPointController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadGeoFenceComplete calling _rallyPointController.loadFromVehicle";
            _rallyPointController.loadFromVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete Rally Points not supported skipping";
            _rallyPointController.removeAll();
            _loadRallyPointsComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_loadRallyPointsComplete(void)
{
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadRallyPointsComplete";
}

void PlanMasterController::_sendMissionComplete(void)
{
    if (_sendGeoFence) {
        _sendGeoFence = false;
        _sendRallyPoints = true;
        if (_geoFenceController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start GeoFence sendToVehicle";
            _geoFenceController.sendToVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle GeoFence not supported skipping";
            _sendGeoFenceComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_sendGeoFenceComplete(void)
{
    if (_sendRallyPoints) {
        _sendRallyPoints = false;
        if (_rallyPointController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start rally sendToVehicle";
            _rallyPointController.sendToVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Points not support skipping";
            _sendRallyPointsComplete();
        }
    }
}

void PlanMasterController::_sendRallyPointsComplete(void)
{
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Point send complete";
}

void PlanMasterController::sendToVehicle(void)
{
    if (_managerVehicle->highLatencyLink()) {
        qgcApp()->showMessage(tr("Upload not supported on high latency links."));
        return;
    }

    if (offline()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start mission sendToVehicle";
        _sendGeoFence = true;
        _missionController.sendToVehicle();
        setDirty(false);
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
        qgcApp()->showMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if(fileInfo.suffix() == AppSettings::planFileExtension) {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
            return;
        }

        int version;
        QJsonObject json = jsonDoc.object();
        if (!JsonHelper::validateQGCJsonFile(json, _planFileType, _planFileVersion, _planFileVersion, version, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
            return;
        }

        QList<JsonHelper::KeyValidateInfo> rgKeyInfo = {
            { _jsonMissionObjectKey,        QJsonValue::Object, true },
            { _jsonGeoFenceObjectKey,       QJsonValue::Object, true },
            { _jsonRallyPointsObjectKey,    QJsonValue::Object, true },
        };
        if (!JsonHelper::validateKeys(json, rgKeyInfo, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
            return;
        }

        if (!_missionController.load(json[_jsonMissionObjectKey].toObject(), errorString) ||
                !_geoFenceController.load(json[_jsonGeoFenceObjectKey].toObject(), errorString) ||
                !_rallyPointController.load(json[_jsonRallyPointsObjectKey].toObject(), errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::missionFileExtension) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else {
        //-- TODO: What then?
    }

    if(success){
        _currentPlanFile.sprintf("%s/%s.%s", fileInfo.path().toLocal8Bit().data(), fileInfo.completeBaseName().toLocal8Bit().data(), AppSettings::planFileExtension);
    } else {
        _currentPlanFile.clear();
    }
    emit currentPlanFileChanged();

    if (!offline()) {
        setDirty(true);
    }
}

QJsonDocument PlanMasterController::saveToJson()
{
    QJsonObject planJson;
    QJsonObject missionJson;
    QJsonObject fenceJson;
    QJsonObject rallyJson;
    JsonHelper::saveQGCJsonFileHeader(planJson, _planFileType, _planFileVersion);
    _missionController.save(missionJson);
    _geoFenceController.save(fenceJson);
    _rallyPointController.save(rallyJson);
    planJson[_jsonMissionObjectKey] = missionJson;
    planJson[_jsonGeoFenceObjectKey] = fenceJson;
    planJson[_jsonRallyPointsObjectKey] = rallyJson;
    return QJsonDocument(planJson);
}

void
PlanMasterController::saveToCurrent()
{
    if(!_currentPlanFile.isEmpty()) {
        saveToFile(_currentPlanFile);
    }
}

void PlanMasterController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString planFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        planFilename += QString(".%1").arg(fileExtension());
    }

    QFile file(planFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(tr("Plan save error %1 : %2").arg(filename).arg(file.errorString()));
        _currentPlanFile.clear();
        emit currentPlanFileChanged();
    } else {
        QJsonDocument saveDoc = saveToJson();
        file.write(saveDoc.toJson());
        if(_currentPlanFile != planFilename) {
            _currentPlanFile = planFilename;
            emit currentPlanFileChanged();
        }
    }

    // Only clear dirty bit if we are offline
    if (offline()) {
        setDirty(false);
    }
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
        qgcApp()->showMessage(tr("KML save error %1 : %2").arg(filename).arg(file.errorString()));
    } else {
        QDomDocument domDocument;
        _missionController.convertToKMLDocument(domDocument);
        QTextStream stream(&file);
        stream << domDocument.toString();
        file.close();
    }
}

void PlanMasterController::removeAll(void)
{
    _missionController.removeAll();
    _geoFenceController.removeAll();
    _rallyPointController.removeAll();
    if (_offline) {
        _missionController.setDirty(false);
        _geoFenceController.setDirty(false);
        _rallyPointController.setDirty(false);
        _currentPlanFile.clear();
        emit currentPlanFileChanged();
    }
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
        setDirty(false);
    } else {
        qWarning() << "PlanMasterController::removeAllFromVehicle called while offline";
    }
}

bool PlanMasterController::containsItems(void) const
{
    return _missionController.containsItems() || _geoFenceController.containsItems() || _rallyPointController.containsItems();
}

bool PlanMasterController::dirty(void) const
{
    return _missionController.dirty() || _geoFenceController.dirty() || _rallyPointController.dirty();
}

void PlanMasterController::setDirty(bool dirty)
{
    _missionController.setDirty(dirty);
    _geoFenceController.setDirty(dirty);
    _rallyPointController.setDirty(dirty);
}

QString PlanMasterController::fileExtension(void) const
{
    return AppSettings::planFileExtension;
}

QString PlanMasterController::kmlFileExtension(void) const
{
    return AppSettings::kmlFileExtension;
}

QStringList PlanMasterController::loadNameFilters(void) const
{
    QStringList filters;

    filters << tr("Supported types (*.%1 *.%2 *.%3 *.%4)").arg(AppSettings::planFileExtension).arg(AppSettings::missionFileExtension).arg(AppSettings::waypointsFileExtension).arg("txt") <<
               tr("All Files (*.*)");
    return filters;
}


QStringList PlanMasterController::saveNameFilters(void) const
{
    QStringList filters;

    filters << tr("Plan Files (*.%1)").arg(fileExtension()) << tr("All Files (*.*)");
    return filters;
}

QStringList PlanMasterController::fileKmlFilters(void) const
{
    QStringList filters;

    filters << tr("KML Files (*.%1)").arg(kmlFileExtension()) << tr("All Files (*.*)");
    return filters;
}

void PlanMasterController::sendPlanToVehicle(Vehicle* vehicle, const QString& filename)
{
    // Use a transient PlanMasterController to accomplish this
    PlanMasterController* controller = new PlanMasterController();
    controller->startStaticActiveVehicle(vehicle);
    controller->loadFromFile(filename);
    controller->sendToVehicle();
    delete controller;
}

void PlanMasterController::_showPlanFromManagerVehicle(void)
{
    if (!_managerVehicle->initialPlanRequestComplete() && !syncInProgress()) {
        // Something went wrong with initial load. All controllers are idle, so just force it off
        _managerVehicle->forceInitialPlanRequestComplete();
    }

    // The crazy if structure is to handle the load propogating by itself through the system
    if (!_missionController.showPlanFromManagerVehicle()) {
        if (!_geoFenceController.showPlanFromManagerVehicle()) {
            _rallyPointController.showPlanFromManagerVehicle();
        }
    }
}

bool PlanMasterController::syncInProgress(void) const
{
    return _missionController.syncInProgress() ||
            _geoFenceController.syncInProgress() ||
            _rallyPointController.syncInProgress();
}
