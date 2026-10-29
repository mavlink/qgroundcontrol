#include "RallyPointController.h"
#include "RallyPoint.h"
#include "Vehicle.h"
#include "GeoJsonHelper.h"
#include "JsonParsing.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanMasterController.h"
#include "RallyPointManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>

QGC_LOGGING_CATEGORY(RallyPointControllerLog, "PlanManager.RallyPointController")

RallyPointController::RallyPointController(PlanMasterController* masterController, QObject* parent)
    : PlanElementController (masterController, parent)
    , _managerVehicle               (masterController->managerVehicle())
    , _rallyPointManager    (masterController->managerVehicle()->rallyPointManager())
{
    connect(&_points, &QmlObjectListModel::countChanged, this, &RallyPointController::containsItemsChanged);
    connect(&_points, &QmlObjectListModel::dirtyChanged, this, &RallyPointController::_pointsDirtyChanged);
}

RallyPointController::~RallyPointController()
{

}

void RallyPointController::start(bool flyView)
{
    qCDebug(RallyPointControllerLog) << "start flyView" << flyView;

    _managerVehicleChanged(_masterController->managerVehicle());
    connect(_masterController, &PlanMasterController::managerVehicleChanged, this, &RallyPointController::_managerVehicleChanged);

    PlanElementController::start(flyView);
}

void RallyPointController::_managerVehicleChanged(Vehicle* managerVehicle)
{
    if (_managerVehicle) {
        _rallyPointManager->disconnect(this);
        _managerVehicle->disconnect(this);
        _managerVehicle = nullptr;
        _rallyPointManager = nullptr;
    }

    _managerVehicle = managerVehicle;
    if (!_managerVehicle) {
        qWarning() << "RallyPointController::managerVehicleChanged managerVehicle=NULL";
        return;
    }

    _rallyPointManager = _managerVehicle->rallyPointManager();
    connect(_rallyPointManager, &RallyPointManager::loadComplete,       this, &RallyPointController::_managerLoadComplete);
    connect(_rallyPointManager, &RallyPointManager::sendComplete,       this, &RallyPointController::_managerSendComplete);
    connect(_rallyPointManager, &RallyPointManager::removeAllComplete,  this, &RallyPointController::_managerRemoveAllComplete);
    connect(_rallyPointManager, &RallyPointManager::inProgressChanged,  this, &RallyPointController::syncInProgressChanged);

    (void) connect(_managerVehicle, &Vehicle::capabilityBitsChanged, this, [this](uint64_t capabilityBits) {
        Q_UNUSED(capabilityBits);
        emit supportedChanged(supported());
    });

    emit supportedChanged(supported());
}

bool RallyPointController::load(const QJsonObject& json, QString& errorString)
{
    removeAll();

    errorString.clear();

    if (json.contains(JsonParsing::jsonVersionKey) && json[JsonParsing::jsonVersionKey].toInt() == 1) {
        // We just ignore old version 1 data
        return true;
    }

    QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        { JsonParsing::jsonVersionKey,   QJsonValue::Double, true },
        { _jsonPointsKey,               QJsonValue::Array,  true },
    };
    if (!JsonParsing::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    QString errorStr;
    QString errorMessage = tr("Rally: %1");

    const int version = json[JsonParsing::jsonVersionKey].toInt();
    if (version < _jsonOldestSupportedVersion || version > _jsonCurrentVersion) {
        errorString =
            tr("Rally Points supports versions %1 to %2").arg(_jsonOldestSupportedVersion).arg(_jsonCurrentVersion);
        return false;
    }

    // Version 2 points are [lat, lon, alt] arrays with no frame, their altitudes are relative to home
    auto loadPoint = [version](const QJsonValue& jsonPoint, QGeoCoordinate& coordinate,
                               QGroundControlQmlGlobal::AltitudeFrame& altitudeFrame, QString& error) {
        altitudeFrame = QGroundControlQmlGlobal::AltitudeFrameRelative;
        if (version < 3) {
            return GeoJsonHelper::loadGeoCoordinate(jsonPoint, true /* altitudeRequired */, coordinate, error);
        }

        if (!jsonPoint.isObject()) {
            error = tr("Each point must be an object");
            return false;
        }
        const QJsonObject jsonPointObject = jsonPoint.toObject();
        const QList<JsonParsing::KeyValidateInfo> pointKeyInfoList = {
            {_jsonCoordinateKey, QJsonValue::Array, true},
            {_jsonFrameKey, QJsonValue::Double, true},
        };
        if (!JsonParsing::validateKeys(jsonPointObject, pointKeyInfoList, error) ||
            !GeoJsonHelper::loadGeoCoordinate(jsonPointObject[_jsonCoordinateKey], true /* altitudeRequired */,
                                              coordinate, error)) {
            return false;
        }

        const int frame = jsonPointObject[_jsonFrameKey].toInt(-1);
        if (frame != MAV_FRAME_GLOBAL && frame != MAV_FRAME_GLOBAL_RELATIVE_ALT &&
            frame != MAV_FRAME_GLOBAL_TERRAIN_ALT) {
            error = tr("Unsupported frame %1").arg(frame);
            return false;
        }
        altitudeFrame = RallyPoint::altitudeFrameFromMavFrame(static_cast<MAV_FRAME>(frame));
        return true;
    };

    QObjectList pointList;
    const QJsonArray jsonPoints = json[_jsonPointsKey].toArray();
    for (const QJsonValue& jsonPoint : jsonPoints) {
        QGeoCoordinate coordinate;
        QGroundControlQmlGlobal::AltitudeFrame altitudeFrame;
        if (!loadPoint(jsonPoint, coordinate, altitudeFrame, errorStr)) {
            qDeleteAll(pointList);
            errorString = errorMessage.arg(errorStr);
            return false;
        }
        pointList.append(new RallyPoint(coordinate, altitudeFrame, this));
    }
    _points.swapObjectList(pointList);

    setDirty(false);
    _setFirstPointCurrent();

    return true;
}

void RallyPointController::save(QJsonObject& json)
{
    json[JsonParsing::jsonVersionKey] = _jsonCurrentVersion;

    QJsonArray rgPoints;
    for (int i=0; i<_points.count(); i++) {
        const RallyPoint* rallyPoint = qobject_cast<RallyPoint*>(_points[i]);
        QJsonValue jsonCoordinate;
        GeoJsonHelper::saveGeoCoordinate(rallyPoint->coordinate(), true /* writeAltitude */, jsonCoordinate);
        QJsonObject jsonPoint;
        jsonPoint[_jsonCoordinateKey] = jsonCoordinate;
        jsonPoint[_jsonFrameKey] = static_cast<int>(RallyPoint::mavFrame(rallyPoint->altitudeFrame()));
        rgPoints.append(jsonPoint);
    }
    json[_jsonPointsKey] = QJsonValue(rgPoints);
}

void RallyPointController::removeAll(void)
{
    _points.clearAndDeleteContents();
    setDirty(true);
    setCurrentRallyPoint(nullptr);
}

void RallyPointController::removeAllFromVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::removeAllFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::removeAllFromVehicle called while syncInProgress";
    } else {
        _rallyPointManager->removeAll();
    }
}

void RallyPointController::loadFromVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::loadFromVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::loadFromVehicle called while syncInProgress";
    } else {
        _itemsRequested = true;
        _rallyPointManager->loadFromVehicle();
    }
}

void RallyPointController::sendToVehicle(void)
{
    if (_masterController->offline()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(RallyPointControllerLog) << "RallyPointController::sendToVehicle";
        setDirty(false);
        QList<RallyPointData> rgPoints;
        for (int i=0; i<_points.count(); i++) {
            const RallyPoint* rallyPoint = qobject_cast<RallyPoint*>(_points[i]);
            rgPoints.append({rallyPoint->coordinate(), RallyPoint::mavFrame(rallyPoint->altitudeFrame())});
        }
        _rallyPointManager->sendToVehicle(rgPoints);
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
        if (!dirty) {
            _points.setDirty(false);
        }
        emit dirtyChanged(dirty);
    }
}

void RallyPointController::_pointsDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

QString RallyPointController::editorQml(void) const
{
    return _rallyPointManager->editorQml();
}

void RallyPointController::_managerLoadComplete(void)
{
    // Fly view always reloads on _loadComplete
    // Plan view only reloads if:
    //  - Load was specifically requested
    //  - There is no current Plan
    if (_flyView || _itemsRequested || isEmpty()) {
        _points.clearAndDeleteContents();
        QObjectList pointList;
        for (const RallyPointData& pointData : _rallyPointManager->points()) {
            pointList.append(
                new RallyPoint(pointData.coordinate, RallyPoint::altitudeFrameFromMavFrame(pointData.frame), this));
        }
        _points.swapObjectList(pointList);
        setDirty(false);
        _setFirstPointCurrent();
        emit loadComplete();
    }
    _itemsRequested = false;
}

void RallyPointController::_managerSendComplete(bool error)
{
    // Fly view always reloads after send
    if (!error && _flyView) {
        showPlanFromManagerVehicle();
    }
}

void RallyPointController::_managerRemoveAllComplete(bool error)
{
    if (!error) {
        // Remove all from vehicle so we always update
        showPlanFromManagerVehicle();
    }
}

void RallyPointController::addPoint(QGeoCoordinate point)
{
    Vehicle* const vehicle = _masterController->controllerVehicle();
    const bool px4 = vehicle->px4Firmware();

    double defaultAlt;
    QGroundControlQmlGlobal::AltitudeFrame altitudeFrame = QGroundControlQmlGlobal::AltitudeFrameRelative;
    if (px4) {
        // The altitude is the ground height at the point, it is seeded from terrain below
        defaultAlt = RallyPoint::getDefaultFactAltitude();
    } else if (_points.count()) {
        const RallyPoint* lastPoint = qobject_cast<RallyPoint*>(_points[_points.count() - 1]);
        defaultAlt = lastPoint->coordinate().altitude();
        altitudeFrame = lastPoint->altitudeFrame();
    } else if (vehicle->apmFirmware() || vehicle->fixedWing()) {
        defaultAlt = SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble();
    } else {
        defaultAlt = RallyPoint::getDefaultFactAltitude();
    }
    point.setAltitude(defaultAlt);
    RallyPoint* newPoint = new RallyPoint(point, altitudeFrame, this);
    _points.append(newPoint);
    setCurrentRallyPoint(newPoint);
    setDirty(true);

    if (px4) {
        // Stays relative with the default altitude if terrain data is not available
        newPoint->useTerrainAltitude();
    }
}

bool RallyPointController::terrainQueryPending(void) const
{
    for (int i = 0; i < _points.count(); i++) {
        const RallyPoint* const rallyPoint = qobject_cast<const RallyPoint*>(_points[i]);
        if (rallyPoint && rallyPoint->terrainQueryPending()) {
            return true;
        }
    }

    return false;
}

bool RallyPointController::supported(void) const
{
    return _managerVehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
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
        setCurrentRallyPoint(nullptr);
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
    setCurrentRallyPoint(_points.count() ? _points[0] : nullptr);
}

bool RallyPointController::containsItems(void) const
{
    return _points.count() > 0;
}

bool RallyPointController::showPlanFromManagerVehicle (void)
{
    qCDebug(RallyPointControllerLog) << "showPlanFromManagerVehicle _flyView" << _flyView;
    if (_masterController->offline()) {
        qCCritical(RallyPointControllerLog) << "RallyPointController::showPlanFromManagerVehicle called while offline";
        return true;    // stops further propagation of showPlanFromManagerVehicle due to error
    } else {
        if (!_managerVehicle->initialPlanRequestComplete()) {
            // The vehicle hasn't completed initial load, we can just wait for loadComplete to be signalled automatically
            qCDebug(RallyPointControllerLog) << "showPlanFromManagerVehicle: !initialPlanRequestComplete, wait for signal";
            return true;
        } else if (syncInProgress()) {
            // If the sync is already in progress, _loadComplete will be called automatically when it is done. So no need to do anything.
            qCDebug(RallyPointControllerLog) << "showPlanFromManagerVehicle: syncInProgress wait for signal";
            return true;
        } else {
            qCDebug(RallyPointControllerLog) << "showPlanFromManagerVehicle: sync complete";
            _itemsRequested = true;
            _managerLoadComplete();
            return false;
        }
    }
}

bool RallyPointController::isEmpty(void) const
{
    return _points.count() == 0;
}
