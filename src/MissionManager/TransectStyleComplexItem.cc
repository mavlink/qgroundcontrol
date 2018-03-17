/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TransectStyleComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(TransectStyleComplexItemLog, "TransectStyleComplexItemLog")

const char* TransectStyleComplexItem::turnAroundDistanceName =              "TurnAroundDistance";
const char* TransectStyleComplexItem::turnAroundDistanceMultiRotorName =    "TurnAroundDistanceMultiRotor";
const char* TransectStyleComplexItem::cameraTriggerInTurnAroundName =       "CameraTriggerInTurnAround";
const char* TransectStyleComplexItem::hoverAndCaptureName =                 "HoverAndCapture";
const char* TransectStyleComplexItem::refly90DegreesName =                  "Refly90Degrees";
const char* TransectStyleComplexItem::terrainAdjustToleranceName =          "TerrainAdjustTolerance";
const char* TransectStyleComplexItem::terrainAdjustMaxClimbRateName =       "TerrainAdjustMaxClimbRate";
const char* TransectStyleComplexItem::terrainAdjustMaxDescentRateName =     "TerrainAdjustMaxDescentRate";

const char* TransectStyleComplexItem::_jsonTransectStyleComplexItemKey =    "TransectStyleComplexItem";
const char* TransectStyleComplexItem::_jsonCameraCalcKey =                  "CameraCalc";
const char* TransectStyleComplexItem::_jsonTransectPointsKey =              "TransectPoints";
const char* TransectStyleComplexItem::_jsonItemsKey =                       "Items";

const int       TransectStyleComplexItem::_terrainQueryTimeoutMsecs =   500;
const double    TransectStyleComplexItem::_surveyEdgeIndicator =        -10;

TransectStyleComplexItem::TransectStyleComplexItem(Vehicle* vehicle, QString settingsGroup, QObject* parent)
    : ComplexMissionItem                (vehicle, parent)
    , _settingsGroup                    (settingsGroup)
    , _sequenceNumber                   (0)
    , _dirty                            (false)
    , _terrainPolyPathQuery             (NULL)
    , _ignoreRecalc                     (false)
    , _complexDistance                  (0)
    , _cameraShots                      (0)
    , _cameraMinTriggerInterval         (0)
    , _cameraCalc                       (vehicle)
    , _followTerrain                    (false)
    , _loadedMissionItemsParent         (NULL)
    , _metaDataMap                      (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/TransectStyle.SettingsGroup.json"), this))
    , _turnAroundDistanceFact           (_settingsGroup, _metaDataMap[_vehicle->multiRotor() ? turnAroundDistanceMultiRotorName : turnAroundDistanceName])
    , _cameraTriggerInTurnAroundFact    (_settingsGroup, _metaDataMap[cameraTriggerInTurnAroundName])
    , _hoverAndCaptureFact              (_settingsGroup, _metaDataMap[hoverAndCaptureName])
    , _refly90DegreesFact               (_settingsGroup, _metaDataMap[refly90DegreesName])
    , _terrainAdjustToleranceFact       (_settingsGroup, _metaDataMap[terrainAdjustToleranceName])
    , _terrainAdjustMaxClimbRateFact    (_settingsGroup, _metaDataMap[terrainAdjustMaxClimbRateName])
    , _terrainAdjustMaxDescentRateFact  (_settingsGroup, _metaDataMap[terrainAdjustMaxDescentRateName])
{
    _terrainQueryTimer.setInterval(_terrainQueryTimeoutMsecs);
    _terrainQueryTimer.setSingleShot(true);
    connect(&_terrainQueryTimer, &QTimer::timeout, this, &TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_cameraTriggerInTurnAroundFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintSide(),        &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintFrontal(),     &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(&_cameraTriggerInTurnAroundFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(_cameraCalc.adjustedFootprintSide(),        &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);
    connect(_cameraCalc.adjustedFootprintFrontal(),     &Fact::valueChanged,            this, &TransectStyleComplexItem::_signalLastSequenceNumberChanged);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::complexDistanceChanged);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::greatestDistanceToChanged);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::greatestDistanceToChanged);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::greatestDistanceToChanged);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::greatestDistanceToChanged);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_cameraTriggerInTurnAroundFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_setDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::dirtyChanged,   this, &TransectStyleComplexItem::_setIfDirty);
    connect(&_cameraCalc,                               &CameraCalc::dirtyChanged,      this, &TransectStyleComplexItem::_setIfDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::coveredAreaChanged);

    connect(this,                                       &TransectStyleComplexItem::transectPointsChanged, this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(this,                                       &TransectStyleComplexItem::transectPointsChanged, this, &TransectStyleComplexItem::greatestDistanceToChanged);
}

void TransectStyleComplexItem::_setCameraShots(int cameraShots)
{
    if (_cameraShots != cameraShots) {
        _cameraShots = cameraShots;
        emit cameraShotsChanged();
    }
}

void TransectStyleComplexItem::setDirty(bool dirty)
{
    if (!dirty) {
        _surveyAreaPolygon.setDirty(false);
        _cameraCalc.setDirty(false);
    }
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void TransectStyleComplexItem::_save(QJsonObject& complexObject)
{
    QJsonObject innerObject;

    innerObject[JsonHelper::jsonVersionKey] =       1;
    innerObject[turnAroundDistanceName] =           _turnAroundDistanceFact.rawValue().toDouble();
    innerObject[cameraTriggerInTurnAroundName] =    _cameraTriggerInTurnAroundFact.rawValue().toBool();
    innerObject[hoverAndCaptureName] =              _hoverAndCaptureFact.rawValue().toBool();
    innerObject[refly90DegreesName] =               _refly90DegreesFact.rawValue().toBool();

    QJsonObject cameraCalcObject;
    _cameraCalc.save(cameraCalcObject);
    innerObject[_jsonCameraCalcKey] = cameraCalcObject;

    QJsonValue  transectPointsJson;

    // Save transects polyline
    JsonHelper::saveGeoCoordinateArray(_transectPoints, false /* writeAltitude */, transectPointsJson);
    innerObject[_jsonTransectPointsKey] = transectPointsJson;

    // Save the interal mission items
    QJsonArray  missionItemsJsonArray;
    QObject* missionItemParent = new QObject();
    QList<MissionItem*> missionItems;
    appendMissionItems(missionItems, missionItemParent);
    foreach (const MissionItem* missionItem, missionItems) {
        QJsonObject missionItemJsonObject;
        missionItem->save(missionItemJsonObject);
        missionItemsJsonArray.append(missionItemJsonObject);
    }
    missionItemParent->deleteLater();
    innerObject[_jsonItemsKey] = missionItemsJsonArray;

    complexObject[_jsonTransectStyleComplexItemKey] = innerObject;
}

void TransectStyleComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool TransectStyleComplexItem::_load(const QJsonObject& complexObject, QString& errorString)
{

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { _jsonTransectStyleComplexItemKey, QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    // The TransectStyleComplexItem is a sub-object of the main complex item object
    QJsonObject innerObject = complexObject[_jsonTransectStyleComplexItemKey].toObject();

    QList<JsonHelper::KeyValidateInfo> innerKeyInfoList = {
        { JsonHelper::jsonVersionKey,       QJsonValue::Double, true },
        { turnAroundDistanceName,           QJsonValue::Double, true },
        { cameraTriggerInTurnAroundName,    QJsonValue::Bool,   true },
        { hoverAndCaptureName,              QJsonValue::Bool,   true },
        { refly90DegreesName,               QJsonValue::Bool,   true },
        { _jsonCameraCalcKey,               QJsonValue::Object, true },
        { _jsonTransectPointsKey,           QJsonValue::Array,  true },
        { _jsonItemsKey,                    QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(innerObject, innerKeyInfoList, errorString)) {
        return false;
    }

    int version = innerObject[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        errorString = tr("TransectStyleComplexItem version %2 not supported").arg(version);
        return false;
    }

    // Load transect points
    if (!JsonHelper::loadGeoCoordinateArray(innerObject[_jsonTransectPointsKey], false /* altitudeRequired */, _transectPoints, errorString)) {
        return false;
    }

    // Load generated mission items
    _loadedMissionItemsParent = new QObject(this);
    QJsonArray missionItemsJsonArray = innerObject[_jsonItemsKey].toArray();
    foreach (const QJsonValue& missionItemJson, missionItemsJsonArray) {
        MissionItem* missionItem = new MissionItem(_loadedMissionItemsParent);
        if (!missionItem->load(missionItemJson.toObject(), 0 /* sequenceNumber */, errorString)) {
            _loadedMissionItemsParent->deleteLater();
            _loadedMissionItemsParent = NULL;
            return false;
        }
        _loadedMissionItems.append(missionItem);
    }

    // Load CameraCalc data
    if (!_cameraCalc.load(innerObject[_jsonCameraCalcKey].toObject(), errorString)) {
        return false;
    }

    // Load TransectStyleComplexItem individual values
    _turnAroundDistanceFact.setRawValue         (innerObject[turnAroundDistanceName].toDouble());
    _cameraTriggerInTurnAroundFact.setRawValue  (innerObject[cameraTriggerInTurnAroundName].toBool());
    _hoverAndCaptureFact.setRawValue            (innerObject[hoverAndCaptureName].toBool());
    _hoverAndCaptureFact.setRawValue            (innerObject[refly90DegreesName].toBool());

    return true;
}

double TransectStyleComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    for (int i=0; i<_transectPoints.count(); i++) {
        QGeoCoordinate vertex = _transectPoints[i].value<QGeoCoordinate>();
        double distance = vertex.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }

    return greatestDistance;
}

void TransectStyleComplexItem::setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    ComplexMissionItem::setMissionFlightStatus(missionFlightStatus);
    if (!qFuzzyCompare(_cruiseSpeed, missionFlightStatus.vehicleSpeed)) {
        _cruiseSpeed = missionFlightStatus.vehicleSpeed;
        emit timeBetweenShotsChanged();
    }
}

void TransectStyleComplexItem::_setDirty(void)
{
    setDirty(true);
}

void TransectStyleComplexItem::_setIfDirty(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void TransectStyleComplexItem::applyNewAltitude(double newAltitude)
{
    Q_UNUSED(newAltitude);
    // FIXME: NYI
    //_altitudeFact.setRawValue(newAltitude);
}

double TransectStyleComplexItem::timeBetweenShots(void)
{
    return _cruiseSpeed == 0 ? 0 : _cameraCalc.adjustedFootprintSide()->rawValue().toDouble() / _cruiseSpeed;
}

void TransectStyleComplexItem::_updateCoordinateAltitudes(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
}

void TransectStyleComplexItem::_signalLastSequenceNumberChanged(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

double TransectStyleComplexItem::coveredArea(void) const
{
    return _surveyAreaPolygon.area();
}

bool TransectStyleComplexItem::_hasTurnaround(void) const
{
    return _turnaroundDistance() > 0;
}

double TransectStyleComplexItem::_turnaroundDistance(void) const
{
    return _turnAroundDistanceFact.rawValue().toDouble();
}

bool TransectStyleComplexItem::hoverAndCaptureAllowed(void) const
{
    return _vehicle->multiRotor() || _vehicle->vtol();
}

void TransectStyleComplexItem::_rebuildTransects(void)
{
    _rebuildTransectsPhase1();
    _rebuildTransectsPhase2();
}

void TransectStyleComplexItem::_queryTransectsPathHeightInfo(void)
{
    _transectsPathHeightInfo.clear();
    if (_terrainPolyPathQuery) {
        // Toss previous query
        _terrainPolyPathQuery->deleteLater();
        _terrainPolyPathQuery = NULL;
    }

    if (_transectPoints.count() > 1) {
        // We don't actually send the query until this timer times out. This way we only send
        // the laset request if we get a bunch in a row.
        _terrainQueryTimer.start();
    }
}

void TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo(void)
{
    if (_transectPoints.count() > 1) {
        _terrainPolyPathQuery = new TerrainPolyPathQuery(this);
        connect(_terrainPolyPathQuery, &TerrainPolyPathQuery::terrainData, this, &TransectStyleComplexItem::_polyPathTerrainData);
        _terrainPolyPathQuery->requestData(_transectPoints);
    }
}

void TransectStyleComplexItem::_polyPathTerrainData(bool success, const QList<TerrainPathQuery::PathHeightInfo_t>& rgPathHeightInfo)
{
    if (success) {
        _transectsPathHeightInfo = rgPathHeightInfo;
    } else {
        _transectsPathHeightInfo.clear();
    }
}

bool TransectStyleComplexItem::readyForSave(void) const
{
    // Make sure we have the terrain data we need
    return _transectPoints.count() > 1 ? _transectsPathHeightInfo.count() : false;
}

void TransectStyleComplexItem::_adjustTransectPointsForTerrain(void)
{
    if (_followTerrain && !readyForSave()) {
        qCWarning(TransectStyleComplexItemLog) << "_adjustTransectPointsForTerrain called when terrain data not ready";
        qgcApp()->showMessage(tr("INTERNAL ERROR: TransectStyleComplexItem::_adjustTransectPointsForTerrain called when terrain data not ready. Plan will be incorrect."));
        return;
    }

    double altitude = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    for (int i=0; i<_transectPoints.count() - 1; i++) {
        QGeoCoordinate transectPoint = _transectPoints[i].value<QGeoCoordinate>();

        bool surveyEdgeIndicator = transectPoint.altitude() == _surveyEdgeIndicator;
        if (_followTerrain) {
            transectPoint.setAltitude(_transectsPathHeightInfo[i].rgHeight[0] + altitude);
        } else {
            transectPoint.setAltitude(altitude);
        }
        if (surveyEdgeIndicator) {
            // Use to indicate survey edge
            transectPoint.setAltitude(-transectPoint.altitude());
        }

        _transectPoints[i] = QVariant::fromValue(transectPoint);
    }

    // Take care of last point
    QGeoCoordinate transectPoint = _transectPoints.last().value<QGeoCoordinate>();
    if (_followTerrain){
        transectPoint.setAltitude(_transectsPathHeightInfo.last().rgHeight.last() + altitude);
    } else {
        transectPoint.setAltitude(altitude);
    }
    _transectPoints[_transectPoints.count() - 1] = QVariant::fromValue(transectPoint);
}

void TransectStyleComplexItem::setFollowTerrain(bool followTerrain)
{
    if (followTerrain != _followTerrain) {
        _followTerrain = followTerrain;
        emit followTerrainChanged(followTerrain);
    }
}
