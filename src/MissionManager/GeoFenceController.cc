/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "GeoFenceController.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "JsonHelper.h"
#include "QGCQGeoCoordinate.h"
#include "AppSettings.h"

#ifndef __mobile__
#include "MainWindow.h"
#include "QGCQFileDialog.h"
#endif

#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(GeoFenceControllerLog, "GeoFenceControllerLog")

const char* GeoFenceController::_jsonFileTypeValue =    "GeoFence";
const char* GeoFenceController::_jsonBreachReturnKey =  "breachReturn";

GeoFenceController::GeoFenceController(QObject* parent)
    : PlanElementController(parent)
    , _dirty(false)
    , _mapPolygon(this)
{
    connect(_mapPolygon.qmlPathModel(), &QmlObjectListModel::countChanged, this, &GeoFenceController::_updateContainsItems);
    connect(_mapPolygon.qmlPathModel(), &QmlObjectListModel::dirtyChanged, this, &GeoFenceController::_polygonDirtyChanged);
}

GeoFenceController::~GeoFenceController()
{

}

void GeoFenceController::start(bool editMode)
{
    qCDebug(GeoFenceControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);
    _init();
}

void GeoFenceController::startStaticActiveVehicle(Vehicle* vehicle)
{
    qCDebug(GeoFenceControllerLog) << "startStaticActiveVehicle";

    PlanElementController::startStaticActiveVehicle(vehicle);
    _init();
}

void GeoFenceController::_init(void)
{

}

void GeoFenceController::setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint)
{
    if (_breachReturnPoint != breachReturnPoint) {
        _breachReturnPoint = breachReturnPoint;
        setDirty(true);
        emit breachReturnPointChanged(breachReturnPoint);
    }
}

void GeoFenceController::_signalAll(void)
{
    emit breachReturnSupportedChanged(breachReturnSupported());
    emit breachReturnPointChanged(breachReturnPoint());
    emit circleEnabledChanged(circleEnabled());
    emit circleRadiusFactChanged(circleRadiusFact());
    emit polygonEnabledChanged(polygonEnabled());
    emit polygonSupportedChanged(polygonSupported());
    emit dirtyChanged(dirty());
}

void GeoFenceController::_activeVehicleBeingRemoved(void)
{
    _activeVehicle->geoFenceManager()->disconnect(this);
}

void GeoFenceController::_activeVehicleSet(void)
{
    GeoFenceManager* geoFenceManager = _activeVehicle->geoFenceManager();
    connect(geoFenceManager, &GeoFenceManager::breachReturnSupportedChanged,    this, &GeoFenceController::breachReturnSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::circleEnabledChanged,            this, &GeoFenceController::circleEnabledChanged);
    connect(geoFenceManager, &GeoFenceManager::circleRadiusFactChanged,         this, &GeoFenceController::circleRadiusFactChanged);
    connect(geoFenceManager, &GeoFenceManager::polygonEnabledChanged,           this, &GeoFenceController::polygonEnabledChanged);
    connect(geoFenceManager, &GeoFenceManager::polygonSupportedChanged,         this, &GeoFenceController::polygonSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::loadComplete,                    this, &GeoFenceController::_loadComplete);
    connect(geoFenceManager, &GeoFenceManager::inProgressChanged,               this, &GeoFenceController::syncInProgressChanged);

    if (!geoFenceManager->inProgress()) {
        _loadComplete(geoFenceManager->breachReturnPoint(), geoFenceManager->polygon());
    }

    _signalAll();
}

bool GeoFenceController::_loadJsonFile(QJsonDocument& jsonDoc, QString& errorString)
{
    QJsonObject json = jsonDoc.object();

    int fileVersion;
    if (!JsonHelper::validateQGCJsonFile(json,
                                         _jsonFileTypeValue,    // expected file type
                                         1,                     // minimum supported version
                                         1,                     // maximum supported version
                                         fileVersion,
                                         errorString)) {
        return false;
    }

    if (!_activeVehicle->parameterManager()->loadFromJson(json, false /* required */, errorString)) {
        return false;
    }

    if (json.contains(_jsonBreachReturnKey)
            && !JsonHelper::loadGeoCoordinate(json[_jsonBreachReturnKey], false /* altitudeRequired */, _breachReturnPoint, errorString)) {
        return false;
    }

    if (!_mapPolygon.loadFromJson(json, true, errorString)) {
        return false;
    }
    _mapPolygon.setDirty(false);

    return true;
}

void GeoFenceController::loadFromFile(const QString& filename)
{
    QString errorString;

    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        _loadJsonFile(jsonDoc, errorString);
    }

    if (!errorString.isEmpty()) {
        qgcApp()->showMessage(errorString);
    }

    _signalAll();
    setDirty(true);
}

void GeoFenceController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString fenceFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        fenceFilename += QString(".%1").arg(AppSettings::fenceFileExtension);
    }

    QFile file(fenceFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject fenceFileObject;    // top level json object

        fenceFileObject[JsonHelper::jsonFileTypeKey] =      _jsonFileTypeValue;
        fenceFileObject[JsonHelper::jsonVersionKey] =       1;
        fenceFileObject[JsonHelper::jsonGroundStationKey] = JsonHelper::jsonGroundStationValue;

        QJsonValue jsonBreachReturn;
        JsonHelper::saveGeoCoordinate(_breachReturnPoint, false /* writeAltitude */, jsonBreachReturn);
        fenceFileObject[_jsonBreachReturnKey] = jsonBreachReturn;

        _mapPolygon.saveToJson(fenceFileObject);

        QJsonDocument saveDoc(fenceFileObject);
        file.write(saveDoc.toJson());
    }

    setDirty(false);
}

void GeoFenceController::removeAll(void)
{
    setBreachReturnPoint(QGeoCoordinate());
    _mapPolygon.clear();
}

void GeoFenceController::loadFromVehicle(void)
{
    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        _activeVehicle->geoFenceManager()->loadFromVehicle();
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle->parameterManager()->parametersReady() << syncInProgress();
    }
}

void GeoFenceController::sendToVehicle(void)
{
    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        _activeVehicle->geoFenceManager()->sendToVehicle(_breachReturnPoint, _mapPolygon.pathModel());
        _mapPolygon.setDirty(false);
        setDirty(false);
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle->parameterManager()->parametersReady() << syncInProgress();
    }
}

bool GeoFenceController::syncInProgress(void) const
{
    return _activeVehicle->geoFenceManager()->inProgress();
}

bool GeoFenceController::dirty(void) const
{
    return _dirty;
}


void GeoFenceController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        if (!dirty) {
            _mapPolygon.setDirty(dirty);
        }
        emit dirtyChanged(dirty);
    }
}

void GeoFenceController::_polygonDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

bool GeoFenceController::breachReturnSupported(void) const
{
    return _activeVehicle->geoFenceManager()->breachReturnSupported();
}

bool GeoFenceController::circleEnabled(void) const
{
    return _activeVehicle->geoFenceManager()->circleEnabled();
}

Fact* GeoFenceController::circleRadiusFact(void) const
{
    return _activeVehicle->geoFenceManager()->circleRadiusFact();
}

bool GeoFenceController::polygonSupported(void) const
{
    return _activeVehicle->geoFenceManager()->polygonSupported();
}

bool GeoFenceController::polygonEnabled(void) const
{
    return _activeVehicle->geoFenceManager()->polygonEnabled();
}

QVariantList GeoFenceController::params(void) const
{
    return _activeVehicle->geoFenceManager()->params();
}

QStringList GeoFenceController::paramLabels(void) const
{
    return _activeVehicle->geoFenceManager()->paramLabels();
}

void GeoFenceController::_setDirty(void)
{
    setDirty(true);
}

void GeoFenceController::_setPolygonFromManager(const QList<QGeoCoordinate>& polygon)
{
    _mapPolygon.clear();
    for (int i=0; i<polygon.count(); i++) {
        _mapPolygon.appendVertex(polygon[i]);
    }
    _mapPolygon.setDirty(false);
}

void GeoFenceController::_setReturnPointFromManager(QGeoCoordinate breachReturnPoint)
{
    _breachReturnPoint = breachReturnPoint;
    emit breachReturnPointChanged(_breachReturnPoint);
}

void GeoFenceController::_loadComplete(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon)
{
    _setReturnPointFromManager(breachReturn);
    _setPolygonFromManager(polygon);
    setDirty(false);
    emit loadComplete();
}

QString GeoFenceController::fileExtension(void) const
{
    return AppSettings::fenceFileExtension;
}

bool GeoFenceController::containsItems(void) const
{
    return _mapPolygon.count() > 2;
}

void GeoFenceController::_updateContainsItems(void)
{
    emit containsItemsChanged(containsItems());
}

void GeoFenceController::removeAllFromVehicle(void)
{
    _activeVehicle->geoFenceManager()->removeAll();
}
