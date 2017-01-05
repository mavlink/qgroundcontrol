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

#include "RallyPointController.h"
#include "RallyPoint.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "JsonHelper.h"
#include "SimpleMissionItem.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#endif

#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(RallyPointControllerLog, "RallyPointControllerLog")

const char* RallyPointController::_jsonFileTypeValue =  "RallyPoints";
const char* RallyPointController::_jsonPointsKey =      "points";

RallyPointController::RallyPointController(QObject* parent)
    : PlanElementController(parent)
    , _dirty(false)
    , _currentRallyPoint(NULL)
{

}

RallyPointController::~RallyPointController()
{

}

void RallyPointController::_activeVehicleBeingRemoved(void)
{
    _activeVehicle->rallyPointManager()->disconnect(this);
    _points.clearAndDeleteContents();
}

void RallyPointController::_activeVehicleSet(void)
{
    RallyPointManager* rallyPointManager = _activeVehicle->rallyPointManager();
    connect(rallyPointManager, &RallyPointManager::loadComplete,        this, &RallyPointController::_loadComplete);
    connect(rallyPointManager, &RallyPointManager::inProgressChanged,   this, &RallyPointController::syncInProgressChanged);

    if (!rallyPointManager->inProgress()) {
        _loadComplete(rallyPointManager->points());
    }
    emit rallyPointsSupportedChanged(rallyPointsSupported());
}

bool RallyPointController::_loadJsonFile(QJsonDocument& jsonDoc, QString& errorString)
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

    // Check for required keys
    QStringList requiredKeys = { _jsonPointsKey };
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        return false;
    }

    // Load points

    QList<QGeoCoordinate> rgPoints;
    if (!JsonHelper::loadGeoCoordinateArray(json[_jsonPointsKey], true /* altitudeRequired */, rgPoints, errorString)) {
        return false;
    }    
    _points.clearAndDeleteContents();
    QObjectList pointList;
    for (int i=0; i<rgPoints.count(); i++) {
        pointList.append(new RallyPoint(rgPoints[i], this));
    }
    _points.swapObjectList(pointList);

    return true;
}

void RallyPointController::loadFromFile(const QString& filename)
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
            qgcApp()->showMessage("Rall Point file is in incorrect format.");
            return;
        }
    }

    if (!errorString.isEmpty()) {
        qgcApp()->showMessage(errorString);
    }

    setDirty(true);
    _setFirstPointCurrent();
}

void RallyPointController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select Rally Point File to load", QString(), "Rally point file (*.rally);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    loadFromFile(filename);
#endif
}

void RallyPointController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString rallyFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        rallyFilename += QString(".%1").arg(QGCApplication::rallyPointFileExtension);
    }

    QFile file(rallyFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showMessage(file.errorString());
    } else {
        QJsonObject jsonObject;

        jsonObject[JsonHelper::jsonFileTypeKey] =       _jsonFileTypeValue;
        jsonObject[JsonHelper::jsonVersionKey] =        1;
        jsonObject[JsonHelper::jsonGroundStationKey] =  JsonHelper::jsonGroundStationValue;

        QJsonArray rgPoints;
        QJsonValue jsonPoint;
        for (int i=0; i<_points.count(); i++) {
            JsonHelper::saveGeoCoordinate(qobject_cast<RallyPoint*>(_points[i])->coordinate(), true /* writeAltitude */, jsonPoint);
            rgPoints.append(jsonPoint);
        }
        jsonObject[_jsonPointsKey] = QJsonValue(rgPoints);

        QJsonDocument saveDoc(jsonObject);
        file.write(saveDoc.toJson());
    }

    setDirty(false);
}

void RallyPointController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString filename = QGCFileDialog::getSaveFileName(NULL, "Select file to save Rally Points to", QString(), "Rally point file (*.rally);;All Files (*.*)");

    if (filename.isEmpty()) {
        return;
    }
    saveToFile(filename);
#endif
}

void RallyPointController::removeAll(void)
{
    _points.clearAndDeleteContents();
    setDirty(true);
    setCurrentRallyPoint(NULL);
}

void RallyPointController::loadFromVehicle(void)
{
    if (_activeVehicle->parameterManager()->parametersReady() && !syncInProgress()) {
        _activeVehicle->rallyPointManager()->loadFromVehicle();
    } else {
        qCWarning(RallyPointControllerLog) << "RallyPointController::loadFromVehicle call at wrong time" << _activeVehicle->parameterManager()->parametersReady() << syncInProgress();
    }
}

void RallyPointController::sendToVehicle(void)
{
    if (!syncInProgress()) {
        setDirty(false);
        QList<QGeoCoordinate> rgPoints;
        for (int i=0; i<_points.count(); i++) {
            rgPoints.append(qobject_cast<RallyPoint*>(_points[i])->coordinate());
        }
        _activeVehicle->rallyPointManager()->sendToVehicle(rgPoints);
    } else {
        qCWarning(RallyPointControllerLog) << "RallyPointController::loadFromVehicle call at wrong time" << _activeVehicle->parameterManager()->parametersReady() << syncInProgress();
    }
}

bool RallyPointController::syncInProgress(void) const
{
    return _activeVehicle->rallyPointManager()->inProgress();
}

void RallyPointController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

QString RallyPointController::editorQml(void) const
{
    return _activeVehicle->rallyPointManager()->editorQml();
}

void RallyPointController::_loadComplete(const QList<QGeoCoordinate> rgPoints)
{
    _points.clearAndDeleteContents();
    QObjectList pointList;
    for (int i=0; i<rgPoints.count(); i++) {
        pointList.append(new RallyPoint(rgPoints[i], this));
    }
    _points.swapObjectList(pointList);
    setDirty(false);
    _setFirstPointCurrent();
    emit loadComplete();
}

QString RallyPointController::fileExtension(void) const
{
    return QGCApplication::rallyPointFileExtension;
}

void RallyPointController::addPoint(QGeoCoordinate point)
{
    double defaultAlt;
    if (_points.count()) {
        defaultAlt = qobject_cast<RallyPoint*>(_points[_points.count() - 1])->coordinate().altitude();
    } else {
        defaultAlt = SimpleMissionItem::defaultAltitude;
    }
    point.setAltitude(defaultAlt);
    RallyPoint* newPoint = new RallyPoint(point, this);
    _points.append(newPoint);
    setCurrentRallyPoint(newPoint);
    setDirty(true);
}

bool RallyPointController::rallyPointsSupported(void) const
{
    return _activeVehicle->rallyPointManager()->rallyPointsSupported();
}

void RallyPointController::removePoint(QObject* rallyPoint)
{
    int foundIndex = 0;
    for (foundIndex=0; foundIndex<_points.count(); foundIndex++) {
        if (_points[foundIndex] == rallyPoint) {
            _points.removeOne(rallyPoint);
            rallyPoint->deleteLater();
        }
    }

    if (_points.count()) {
        int newIndex = qMin(foundIndex, _points.count() - 1);
        newIndex = qMax(newIndex, 0);
        setCurrentRallyPoint(_points[newIndex]);
    } else {
        setCurrentRallyPoint(NULL);
    }
}

void RallyPointController::setCurrentRallyPoint(QObject* rallyPoint)
{
    if (_currentRallyPoint != rallyPoint) {
        _currentRallyPoint = rallyPoint;
        emit currentRallyPointChanged(rallyPoint);
    }
}

void RallyPointController::_setFirstPointCurrent(void)
{
    setCurrentRallyPoint(_points.count() ? _points[0] : NULL);
}
