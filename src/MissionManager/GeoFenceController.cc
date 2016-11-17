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

#ifndef __mobile__
#include "QGCFileDialog.h"
#endif

#include <QJsonDocument>

QGC_LOGGING_CATEGORY(GeoFenceControllerLog, "GeoFenceControllerLog")

const char* GeoFenceController::_jsonFileTypeValue =    "GeoFence";
const char* GeoFenceController::_jsonBreachReturnKey =  "breachReturn";

GeoFenceController::GeoFenceController(QObject* parent)
    : PlanElementController(parent)
    , _dirty(false)
{

}

GeoFenceController::~GeoFenceController()
{

}

void GeoFenceController::start(bool editMode)
{
    qCDebug(GeoFenceControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);

    connect(&_polygon, &QGCMapPolygon::dirtyChanged, this, &GeoFenceController::_polygonDirtyChanged);
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
    emit fenceSupportedChanged(fenceSupported());
    emit fenceEnabledChanged(fenceEnabled());
    emit circleSupportedChanged(circleSupported());
    emit polygonSupportedChanged(polygonSupported());
    emit breachReturnSupportedChanged(breachReturnSupported());
    emit breachReturnPointChanged(breachReturnPoint());
    emit circleRadiusChanged(circleRadius());
    emit paramsChanged(params());
    emit paramLabelsChanged(paramLabels());
    emit editorQmlChanged(editorQml());
    emit dirtyChanged(dirty());
}

void GeoFenceController::_activeVehicleBeingRemoved(void)
{
    _activeVehicle->geoFenceManager()->disconnect(this);
}

void GeoFenceController::_activeVehicleSet(void)
{
    GeoFenceManager* geoFenceManager = _activeVehicle->geoFenceManager();
    connect(geoFenceManager, &GeoFenceManager::fenceSupportedChanged,           this, &GeoFenceController::fenceSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::fenceEnabledChanged,             this, &GeoFenceController::fenceEnabledChanged);
    connect(geoFenceManager, &GeoFenceManager::circleSupportedChanged,          this, &GeoFenceController::_setDirty);
    connect(geoFenceManager, &GeoFenceManager::polygonSupportedChanged,         this, &GeoFenceController::_setDirty);
    connect(geoFenceManager, &GeoFenceManager::circleSupportedChanged,          this, &GeoFenceController::circleSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::polygonSupportedChanged,         this, &GeoFenceController::polygonSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::breachReturnSupportedChanged,    this, &GeoFenceController::breachReturnSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::circleRadiusChanged,             this, &GeoFenceController::circleRadiusChanged);
    connect(geoFenceManager, &GeoFenceManager::paramsChanged,                   this, &GeoFenceController::paramsChanged);
    connect(geoFenceManager, &GeoFenceManager::paramLabelsChanged,              this, &GeoFenceController::paramLabelsChanged);
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

    // Check for required keys
    QStringList requiredKeys;
    requiredKeys << JsonHelper::jsonVersionKey << JsonHelper::jsonFileTypeKey;
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        return false;
    }

#if 0
    // Validate base key types
    QStringList             keyList;
    QList<QJsonValue::Type> typeList;
    keyList << jsonSimpleItemsKey << _jsonVersionKey << _jsonGroundStationKey << _jsonMavAutopilotKey << _jsonComplexItemsKey << _jsonPlannedHomePositionKey;
    typeList << QJsonValue::Array << QJsonValue::String << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Object;
    if (!JsonHelper::validateKeyTypes(json, keyList, typeList, errorString)) {
        return false;
    }
#endif

    // Version check
    if (json[JsonHelper::jsonVersionKey].toString() != "1.0") {
        errorString = QStringLiteral("QGroundControl does not support this file version");
        return false;
    }

    if (!_activeVehicle->parameterManager()->loadFromJson(json, false /* required */, errorString)) {
        return false;
    }

    if (breachReturnSupported()) {
        if (json.contains(_jsonBreachReturnKey)
                && !JsonHelper::loadGeoCoordinate(json[_jsonBreachReturnKey], false /* altitudeRequired */, _breachReturnPoint, errorString)) {
            return false;
        }
    } else {
        _breachReturnPoint = QGeoCoordinate();
    }

    if (polygonSupported()) {
        if (!_polygon.loadFromJson(json, false /* reauired */, errorString)) {
            return false;
        }
    } else {
        _polygon.clear();
    }
    _polygon.setDirty(false);

    return true;
}

#if 0
// NYI
bool GeoFenceController::_loadTextFile(QTextStream& stream, QmlObjectListModel* visualItems, QString& errorString)
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
#endif

void GeoFenceController::loadFromFile(const QString& filename)
{
    QString errorString;

    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString();
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (JsonHelper::isJsonFile(bytes, jsonDoc)) {
            _loadJsonFile(jsonDoc, errorString);
        } else {
            // FIXME: No MP file format support
            qgcApp()->showMessage("GeoFence file is in incorrect format.");
            return;
        }
    }

    if (!errorString.isEmpty()) {
        qgcApp()->showMessage(errorString);
    }

    _signalAll();
    setDirty(true);
}

void GeoFenceController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select GeoFence File to load", QString(), "Fence file (*.fence);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    loadFromFile(filename);
#endif
}

void GeoFenceController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString fenceFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        fenceFilename += QString(".%1").arg(QGCApplication::fenceFileExtension);
    }

    QFile file(fenceFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject fenceFileObject;    // top level json object

        fenceFileObject[JsonHelper::jsonFileTypeKey] =      _jsonFileTypeValue;
        fenceFileObject[JsonHelper::jsonVersionKey] =       QStringLiteral("1.0");
        fenceFileObject[JsonHelper::jsonGroundStationKey] = JsonHelper::jsonGroundStationValue;

        QStringList         paramNames;
        ParameterManager*   paramMgr = _activeVehicle->parameterManager();
        GeoFenceManager*    fenceMgr = _activeVehicle->geoFenceManager();
        QVariantList        params = fenceMgr->params();

        for (int i=0; i< params.count(); i++) {
            paramNames.append(params[i].value<Fact*>()->name());
        }
        if (paramNames.count() > 0) {
            paramMgr->saveToJson(paramMgr->defaultComponentId(), paramNames, fenceFileObject);
        }

        if (breachReturnSupported()) {
            QJsonValue jsonBreachReturn;
            JsonHelper::saveGeoCoordinate(_breachReturnPoint, false /* writeAltitude */, jsonBreachReturn);
            fenceFileObject[_jsonBreachReturnKey] = jsonBreachReturn;
        }

        if (polygonSupported()) {
            _polygon.saveToJson(fenceFileObject);
        }

        QJsonDocument saveDoc(fenceFileObject);
        file.write(saveDoc.toJson());
    }

    setDirty(false);
}

void GeoFenceController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(NULL, "Select file to save GeoFence to", QString(), "Fence file (*.fence);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    saveToFile(filename);
#endif
}

void GeoFenceController::removeAll(void)
{
    setBreachReturnPoint(QGeoCoordinate());
    _polygon.clear();
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
        _activeVehicle->geoFenceManager()->sendToVehicle(_breachReturnPoint, _polygon.coordinateList());
        _polygon.setDirty(false);
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
    return _dirty | _polygon.dirty();
}


void GeoFenceController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        if (!dirty) {
            _polygon.setDirty(dirty);
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

bool GeoFenceController::fenceSupported(void) const
{
    return _activeVehicle->geoFenceManager()->fenceSupported();
}

bool GeoFenceController::fenceEnabled(void) const
{
    return _activeVehicle->geoFenceManager()->fenceEnabled();
}

bool GeoFenceController::circleSupported(void) const
{
    return _activeVehicle->geoFenceManager()->circleSupported();
}

bool GeoFenceController::polygonSupported(void) const
{
    return _activeVehicle->geoFenceManager()->polygonSupported();
}

bool GeoFenceController::breachReturnSupported(void) const
{
    return _activeVehicle->geoFenceManager()->breachReturnSupported();
}

void GeoFenceController::_setDirty(void)
{
    setDirty(true);
}

void GeoFenceController::_setPolygonFromManager(const QList<QGeoCoordinate>& polygon)
{
    _polygon.setPath(polygon);
    _polygon.setDirty(false);
    emit polygonPathChanged(_polygon.path());
}

void GeoFenceController::_setReturnPointFromManager(QGeoCoordinate breachReturnPoint)
{
    _breachReturnPoint = breachReturnPoint;
    emit breachReturnPointChanged(_breachReturnPoint);
}

float GeoFenceController::circleRadius(void) const
{
    return _activeVehicle->geoFenceManager()->circleRadius();
}

QVariantList GeoFenceController::params(void) const
{
    return _activeVehicle->geoFenceManager()->params();
}

QStringList GeoFenceController::paramLabels(void) const
{
    return _activeVehicle->geoFenceManager()->paramLabels();
}

QString GeoFenceController::editorQml(void) const
{
    return _activeVehicle->geoFenceManager()->editorQml();
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
    return QGCApplication::fenceFileExtension;
}
