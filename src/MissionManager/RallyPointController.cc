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
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#ifndef __mobile__
#include "QGCQFileDialog.h"
#endif

#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(RallyPointControllerLog, "RallyPointControllerLog")

const char* RallyPointController::_jsonFileTypeValue =  "RallyPoints";
const char* RallyPointController::_jsonPointsKey =      "points";

RallyPointController::RallyPointController(PlanMasterController* masterController, QObject* parent)
    : PlanElementController(masterController, parent)
    , _rallyPointManager(_managerVehicle->rallyPointManager())
    , _dirty(false)
    , _currentRallyPoint(NULL)
{
    connect(&_points, &QmlObjectListModel::countChanged, this, &RallyPointController::_updateContainsItems);

    managerVehicleChanged(_managerVehicle);
}

RallyPointController::~RallyPointController()
{

}

void RallyPointController::managerVehicleChanged(Vehicle* managerVehicle)
{
    if (_managerVehicle) {
        _rallyPointManager->disconnect(this);
        _managerVehicle = NULL;
        _rallyPointManager = NULL;
    }

    _managerVehicle = managerVehicle;
    if (!_managerVehicle) {
        qWarning() << "RallyPointController::managerVehicleChanged managerVehicle=NULL";
        return;
    }

    _rallyPointManager = _managerVehicle->rallyPointManager();
    connect(_rallyPointManager, &RallyPointManager::loadComplete,       this, &RallyPointController::_loadComplete);
    connect(_rallyPointManager, &RallyPointManager::inProgressChanged,  this, &RallyPointController::syncInProgressChanged);

    emit rallyPointsSupportedChanged(rallyPointsSupported());
}

bool RallyPointController::load(const QJsonObject& json, QString& errorString)
{
    QString errorStr;
    QString errorMessage = tr("Rally: %1");

    // Check for required keys
    QStringList requiredKeys = { _jsonPointsKey };
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }

    QList<QGeoCoordinate> rgPoints;
    if (!JsonHelper::loadGeoCoordinateArray(json[_jsonPointsKey], true /* altitudeRequired */, rgPoints, errorStr)) {
        errorString = errorMessage.arg(errorStr);
        return false;
    }
    _points.clearAndDeleteContents();
    QObjectList pointList;
    for (int i=0; i<rgPoints.count(); i++) {
        pointList.append(new RallyPoint(rgPoints[i], this));
    }
    _points.swapObjectList(pointList);

    setDirty(false);
    _setFirstPointCurrent();

    return true;
}

void RallyPointController::save(QJsonObject& json)
{
    json[JsonHelper::jsonVersionKey] = 1;

    QJsonArray rgPoints;
    QJsonValue jsonPoint;
    for (int i=0; i<_points.count(); i++) {
        JsonHelper::saveGeoCoordinate(qobject_cast<RallyPoint*>(_points[i])->coordinate(), true /* writeAltitude */, jsonPoint);
        rgPoints.append(jsonPoint);
    }
    json[_jsonPointsKey] = QJsonValue(rgPoints);
}

void RallyPointController::removeAll(void)
{
    _points.clearAndDeleteContents();
    setDirty(true);
    setCurrentRallyPoint(NULL);
}

void RallyPointController::loadFromVehicle(void)
{
    if (!syncInProgress()) {
        _rallyPointManager->loadFromVehicle();
    } else {
        qCWarning(RallyPointControllerLog) << "RallyPointController::loadFromVehicle call while syncInProgress";
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
        _rallyPointManager->sendToVehicle(rgPoints);
    } else {
        qCWarning(RallyPointControllerLog) << "RallyPointController::loadFromVehicle while syncInProgress";
    }
}

bool RallyPointController::syncInProgress(void) const
{
    return _rallyPointManager->inProgress();
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
    return _rallyPointManager->editorQml();
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

void RallyPointController::addPoint(QGeoCoordinate point)
{
    double defaultAlt;
    if (_points.count()) {
        defaultAlt = qobject_cast<RallyPoint*>(_points[_points.count() - 1])->coordinate().altitude();
    } else {
        defaultAlt = qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble();
    }
    point.setAltitude(defaultAlt);
    RallyPoint* newPoint = new RallyPoint(point, this);
    _points.append(newPoint);
    setCurrentRallyPoint(newPoint);
    setDirty(true);
}

bool RallyPointController::rallyPointsSupported(void) const
{
    return _rallyPointManager->rallyPointsSupported();
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

bool RallyPointController::containsItems(void) const
{
    return _points.count() > 0;
}

void RallyPointController::_updateContainsItems(void)
{
    emit containsItemsChanged(containsItems());
}

void RallyPointController::removeAllFromVehicle(void)
{
    _rallyPointManager->removeAll();
}
