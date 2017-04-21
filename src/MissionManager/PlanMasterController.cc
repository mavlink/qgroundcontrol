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

#include <QJsonDocument>
#include <QFileInfo>

const int   PlanMasterController::_planFileVersion =            1;
const char* PlanMasterController::_planFileType =               "Plan";
const char* PlanMasterController::_jsonMissionObjectKey =       "mission";
const char* PlanMasterController::_jsonGeoFenceObjectKey =      "geoFence";
const char* PlanMasterController::_jsonRallyPointsObjectKey =   "rallyPoints";

PlanMasterController::PlanMasterController(QObject* parent)
    : QObject(parent)
    , _multiVehicleMgr(qgcApp()->toolbox()->multiVehicleManager())
    , _controllerVehicle(new Vehicle((MAV_AUTOPILOT)qgcApp()->toolbox()->settingsManager()->appSettings()->offlineEditingFirmwareType()->rawValue().toInt(), (MAV_TYPE)qgcApp()->toolbox()->settingsManager()->appSettings()->offlineEditingVehicleType()->rawValue().toInt(), qgcApp()->toolbox()->firmwarePluginManager()))
    , _managerVehicle(_controllerVehicle)
    , _editMode(false)
    , _offline(true)
    , _missionController(this)
    , _geoFenceController(this)
    , _rallyPointController(this)
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

void PlanMasterController::start(bool editMode)
{
    _editMode = editMode;
    _missionController.start(editMode);
    _geoFenceController.start(editMode);
    _rallyPointController.start(editMode);

    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanMasterController::_activeVehicleChanged);
    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
}

void PlanMasterController::startStaticActiveVehicle(Vehicle* vehicle)
{
    _editMode = false;
    _activeVehicleChanged(vehicle);
}

void PlanMasterController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_managerVehicle == activeVehicle) {
        // We are already setup for this vehicle
        return;
    }

    bool newOffline = false;
    if (activeVehicle == NULL) {
        // Since there is no longer an active vehicle we use the offline controller vehicle as the manager vehicle
        _managerVehicle = _controllerVehicle;
        newOffline = true;
    } else {
        // FIXME: Check for vehicle compatibility. (edit mode only)
        _managerVehicle = activeVehicle;
        newOffline = false;
    }
    if (newOffline != _offline) {
        _offline = newOffline;
        emit offlineEditingChanged(newOffline);
    }

    _missionController.managerVehicleChanged(_managerVehicle);
    _geoFenceController.managerVehicleChanged(_managerVehicle);
    _rallyPointController.managerVehicleChanged(_managerVehicle);

    if (!_editMode && _offline) {
        // Fly view has changed to a new active vehicle
        loadFromVehicle();
    }

    // Whenever manager changes we need to update syncInProgress
    emit syncInProgressChanged(syncInProgress());
}

void PlanMasterController::loadFromVehicle(void)
{
    // FIXME: Hack implementation
    _missionController.loadFromVehicle();
    _geoFenceController.loadFromVehicle();
    _rallyPointController.loadFromVehicle();
    setDirty(false);
}

void PlanMasterController::sendToVehicle(void)
{
    // FIXME: Hack implementation
    _missionController.sendToVehicle();
    _geoFenceController.sendToVehicle();
    _rallyPointController.sendToVehicle();
    setDirty(false);
}


void PlanMasterController::loadFromFile(const QString& filename)
{
    QString errorString;
    QString errorMessage = tr("Error reading Plan file (%1). %2").arg(filename).arg("%1");

    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        return;
    }

    QString fileExtension(".%1");
    if (filename.endsWith(fileExtension.arg(AppSettings::planFileExtension))) {
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
        }
    } else if (filename.endsWith(fileExtension.arg(AppSettings::missionFileExtension))) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
        }
    } else if (filename.endsWith(fileExtension.arg(AppSettings::waypointsFileExtension)) ||
               filename.endsWith(fileExtension.arg(QStringLiteral("txt")))) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showMessage(errorMessage.arg(errorString));
        }
    }

    if (!offline()) {
        setDirty(true);
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
    } else {
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

        QJsonDocument saveDoc(planJson);
        file.write(saveDoc.toJson());
    }

    // Only clear dirty bit if we are offline
    if (offline()) {
        setDirty(false);
    }
}

void PlanMasterController::removeAll(void)
{

}

void PlanMasterController::removeAllFromVehicle(void)
{
    _missionController.removeAllFromVehicle();
    _geoFenceController.removeAllFromVehicle();
    _rallyPointController.removeAllFromVehicle();
}

bool PlanMasterController::containsItems(void) const
{
    return _missionController.containsItems() || _geoFenceController.containsItems() || _rallyPointController.containsItems();
}

bool PlanMasterController::syncInProgress(void) const
{
    return _missionController.syncInProgress() || _geoFenceController.syncInProgress() || _rallyPointController.syncInProgress();
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

QStringList PlanMasterController::loadNameFilters(void) const
{
    QStringList filters;

    filters << tr("Supported types (*.%1 *.%2 *.%3 *.%4)").arg(AppSettings::planFileExtension).arg(AppSettings::missionFileExtension).arg(AppSettings::waypointsFileExtension).arg("*.txt") <<
               tr("All Files (*.*)");
    return filters;
}


QStringList PlanMasterController::saveNameFilters(void) const
{
    QStringList filters;

    filters << tr("Plan Files (*.%1)").arg(fileExtension()) << tr("All Files (*.*)");
    return filters;
}

void PlanMasterController::sendPlanToVehicle(Vehicle* vehicle, const QString& filename)
{
    // Use a transient PlanMasterController to accomplish this
    PlanMasterController* controller = new PlanMasterController();
    controller->startStaticActiveVehicle(vehicle);
    controller->loadFromFile(filename);
    delete controller;
}

