/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
const char* TransectStyleComplexItem::_jsonVisualTransectPointsKey =        "VisualTransectPoints";
const char* TransectStyleComplexItem::_jsonItemsKey =                       "Items";
const char* TransectStyleComplexItem::_jsonFollowTerrainKey =               "FollowTerrain";
const char* TransectStyleComplexItem::_jsonCameraShotsKey =                 "CameraShots";

const int   TransectStyleComplexItem::_terrainQueryTimeoutMsecs =           1000;

TransectStyleComplexItem::TransectStyleComplexItem(Vehicle* vehicle, bool flyView, QString settingsGroup, QObject* parent)
    : ComplexMissionItem                (vehicle, flyView, parent)
    , _sequenceNumber                   (0)
    , _terrainPolyPathQuery             (nullptr)
    , _ignoreRecalc                     (false)
    , _complexDistance                  (0)
    , _cameraShots                      (0)
    , _cameraCalc                       (vehicle, settingsGroup)
    , _followTerrain                    (false)
    , _loadedMissionItemsParent         (nullptr)
    , _metaDataMap                      (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/TransectStyle.SettingsGroup.json"), this))
    , _turnAroundDistanceFact           (settingsGroup, _metaDataMap[_vehicle->multiRotor() ? turnAroundDistanceMultiRotorName : turnAroundDistanceName])
    , _cameraTriggerInTurnAroundFact    (settingsGroup, _metaDataMap[cameraTriggerInTurnAroundName])
    , _hoverAndCaptureFact              (settingsGroup, _metaDataMap[hoverAndCaptureName])
    , _refly90DegreesFact               (settingsGroup, _metaDataMap[refly90DegreesName])
    , _terrainAdjustToleranceFact       (settingsGroup, _metaDataMap[terrainAdjustToleranceName])
    , _terrainAdjustMaxClimbRateFact    (settingsGroup, _metaDataMap[terrainAdjustMaxClimbRateName])
    , _terrainAdjustMaxDescentRateFact  (settingsGroup, _metaDataMap[terrainAdjustMaxDescentRateName])
{
    _terrainQueryTimer.setInterval(_terrainQueryTimeoutMsecs);
    _terrainQueryTimer.setSingleShot(true);
    connect(&_terrainQueryTimer, &QTimer::timeout, this, &TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo);

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(this,                      &TransectStyleComplexItem::followTerrainChanged, this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustMaxClimbRateFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustMaxDescentRateFact,          &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustToleranceFact,               &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_cameraTriggerInTurnAroundFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintSide(),        &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintFrontal(),     &Fact::valueChanged,            this, &TransectStyleComplexItem::_rebuildTransects);

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
    connect(this,                      &TransectStyleComplexItem::followTerrainChanged, this, &TransectStyleComplexItem::_setDirty);
    connect(&_terrainAdjustMaxClimbRateFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_terrainAdjustMaxDescentRateFact,          &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_terrainAdjustToleranceFact,               &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_setDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::dirtyChanged,   this, &TransectStyleComplexItem::_setIfDirty);
    connect(&_cameraCalc,                               &CameraCalc::dirtyChanged,      this, &TransectStyleComplexItem::_setIfDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::coveredAreaChanged);

    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged, this, &TransectStyleComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged, this, &TransectStyleComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(&_hoverAndCaptureFact,                      &Fact::rawValueChanged,         this, &TransectStyleComplexItem::_handleHoverAndCaptureEnabled);

    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::greatestDistanceToChanged);
    connect(this,                                       &TransectStyleComplexItem::followTerrainChanged,        this, &TransectStyleComplexItem::_followTerrainChanged);
    connect(this,                                       &TransectStyleComplexItem::wizardModeChanged,           this, &TransectStyleComplexItem::readyForSaveStateChanged);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::isValidChanged, this, &TransectStyleComplexItem::readyForSaveStateChanged);

    setDirty(false);
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
    innerObject[_jsonFollowTerrainKey] =            _followTerrain;
    innerObject[_jsonCameraShotsKey] =              _cameraShots;

    if (_followTerrain) {
        innerObject[terrainAdjustToleranceName] =       _terrainAdjustToleranceFact.rawValue().toDouble();
        innerObject[terrainAdjustMaxClimbRateName] =    _terrainAdjustMaxClimbRateFact.rawValue().toDouble();
        innerObject[terrainAdjustMaxDescentRateName] =  _terrainAdjustMaxDescentRateFact.rawValue().toDouble();
    }

    QJsonObject cameraCalcObject;
    _cameraCalc.save(cameraCalcObject);
    innerObject[_jsonCameraCalcKey] = cameraCalcObject;

    QJsonValue  transectPointsJson;

    // Save transects polyline
    JsonHelper::saveGeoCoordinateArray(_visualTransectPoints, false /* writeAltitude */, transectPointsJson);
    innerObject[_jsonVisualTransectPointsKey] = transectPointsJson;

    // Save the interal mission items
    QJsonArray  missionItemsJsonArray;
    QObject* missionItemParent = new QObject();
    QList<MissionItem*> missionItems;
    appendMissionItems(missionItems, missionItemParent);
    for (const MissionItem* missionItem: missionItems) {
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

bool TransectStyleComplexItem::_load(const QJsonObject& complexObject, bool forPresets, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { _jsonTransectStyleComplexItemKey, QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    // The TransectStyleComplexItem is a sub-object of the main complex item object
    QJsonObject innerObject = complexObject[_jsonTransectStyleComplexItemKey].toObject();

    if (innerObject.contains(JsonHelper::jsonVersionKey)) {
        int version = innerObject[JsonHelper::jsonVersionKey].toInt();
        if (version != 1) {
            errorString = tr("TransectStyleComplexItem version %2 not supported").arg(version);
            return false;
        }
    }

    QList<JsonHelper::KeyValidateInfo> innerKeyInfoList = {
        { JsonHelper::jsonVersionKey,       QJsonValue::Double, true },
        { turnAroundDistanceName,           QJsonValue::Double, true },
        { cameraTriggerInTurnAroundName,    QJsonValue::Bool,   true },
        { hoverAndCaptureName,              QJsonValue::Bool,   true },
        { refly90DegreesName,               QJsonValue::Bool,   true },
        { _jsonCameraCalcKey,               QJsonValue::Object, true },
        { _jsonVisualTransectPointsKey,     QJsonValue::Array,  !forPresets },
        { _jsonItemsKey,                    QJsonValue::Array,  !forPresets },
        { _jsonFollowTerrainKey,            QJsonValue::Bool,   true },
        { _jsonCameraShotsKey,              QJsonValue::Double, false },    // Not required since it was missing from initial implementation
    };
    if (!JsonHelper::validateKeys(innerObject, innerKeyInfoList, errorString)) {
        return false;
    }

    if (!forPresets) {
        // Load visual transect points
        if (!JsonHelper::loadGeoCoordinateArray(innerObject[_jsonVisualTransectPointsKey], false /* altitudeRequired */, _visualTransectPoints, errorString)) {
            return false;
        }
        _coordinate = _visualTransectPoints.count() ? _visualTransectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
        _exitCoordinate = _visualTransectPoints.count() ? _visualTransectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();
        _isIncomplete = false;

        // Load generated mission items
        _loadedMissionItemsParent = new QObject(this);
        QJsonArray missionItemsJsonArray = innerObject[_jsonItemsKey].toArray();
        for (const QJsonValue missionItemJson: missionItemsJsonArray) {
            MissionItem* missionItem = new MissionItem(_loadedMissionItemsParent);
            if (!missionItem->load(missionItemJson.toObject(), 0 /* sequenceNumber */, errorString)) {
                _loadedMissionItemsParent->deleteLater();
                _loadedMissionItemsParent = nullptr;
                return false;
            }
            _loadedMissionItems.append(missionItem);
        }
    }

    // Load CameraCalc data
    if (!_cameraCalc.load(innerObject[_jsonCameraCalcKey].toObject(), errorString)) {
        return false;
    }

    // Load TransectStyleComplexItem individual values
    _turnAroundDistanceFact.setRawValue         (innerObject[turnAroundDistanceName].toDouble());
    _cameraTriggerInTurnAroundFact.setRawValue  (innerObject[cameraTriggerInTurnAroundName].toBool());
    _hoverAndCaptureFact.setRawValue            (innerObject[hoverAndCaptureName].toBool());
    _refly90DegreesFact.setRawValue             (innerObject[refly90DegreesName].toBool());
    _followTerrain = innerObject[_jsonFollowTerrainKey].toBool();

    // These two keys where not included in initial implementation so they are optional. Without them the values will be
    // incorrect when loaded though.
    if (innerObject.contains(_jsonCameraShotsKey)) {
        _cameraShots = innerObject[_jsonCameraShotsKey].toInt();
    }

    if (_followTerrain) {
        QList<JsonHelper::KeyValidateInfo> followTerrainKeyInfoList = {
            { terrainAdjustToleranceName,       QJsonValue::Double, true },
            { terrainAdjustMaxClimbRateName,    QJsonValue::Double, true },
            { terrainAdjustMaxDescentRateName,  QJsonValue::Double, true },
        };
        if (!JsonHelper::validateKeys(innerObject, followTerrainKeyInfoList, errorString)) {
            return false;
        }

        _terrainAdjustToleranceFact.setRawValue         (innerObject[terrainAdjustToleranceName].toDouble());
        _terrainAdjustMaxClimbRateFact.setRawValue      (innerObject[terrainAdjustMaxClimbRateName].toDouble());
        _terrainAdjustMaxDescentRateFact.setRawValue    (innerObject[terrainAdjustMaxDescentRateName].toDouble());
    }

    return true;
}

double TransectStyleComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    for (int i=0; i<_visualTransectPoints.count(); i++) {
        QGeoCoordinate vertex = _visualTransectPoints[i].value<QGeoCoordinate>();
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

void TransectStyleComplexItem::_updateCoordinateAltitudes(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
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
    if (_ignoreRecalc) {
        return;
    }

    _rebuildTransectsPhase1();

    if (_followTerrain) {
        // Query the terrain data. Once available terrain heights will be calculated
        _queryTransectsPathHeightInfo();
    } else {
        // Not following terrain, just add requested altitude to coords
        double requestedAltitude = _cameraCalc.distanceToSurface()->rawValue().toDouble();

        for (int i=0; i<_transects.count(); i++) {
            QList<CoordInfo_t>& transect = _transects[i];

            for (int j=0; j<transect.count(); j++) {
                QGeoCoordinate& coord = transect[j].coord;

                coord.setAltitude(requestedAltitude);
            }
        }
    }

    // Calc bounding cube
    double north = 0.0;
    double south = 180.0;
    double east  = 0.0;
    double west  = 360.0;
    double bottom = 100000.;
    double top = 0.;
    // Generate the visuals transect representation
    _visualTransectPoints.clear();
    for (const QList<CoordInfo_t>& transect: _transects) {
        for (const CoordInfo_t& coordInfo: transect) {
            _visualTransectPoints.append(QVariant::fromValue(coordInfo.coord));
            double lat = coordInfo.coord.latitude()  + 90.0;
            double lon = coordInfo.coord.longitude() + 180.0;
            north   = fmax(north, lat);
            south   = fmin(south, lat);
            east    = fmax(east,  lon);
            west    = fmin(west,  lon);
            bottom  = fmin(bottom, coordInfo.coord.altitude());
            top     = fmax(top, coordInfo.coord.altitude());
        }
    }
    //-- Update bounding cube for airspace management control
    _setBoundingCube(QGCGeoBoundingCube(
        QGeoCoordinate(north - 90.0, west - 180.0, bottom),
        QGeoCoordinate(south - 90.0, east - 180.0, top)));
    emit visualTransectPointsChanged();

    _coordinate = _visualTransectPoints.count() ? _visualTransectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _visualTransectPoints.count() ? _visualTransectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);

    if (_isIncomplete) {
        _isIncomplete = false;
        emit isIncompleteChanged();
    }

    _recalcComplexDistance();
    _recalcCameraShots();

    emit lastSequenceNumberChanged(lastSequenceNumber());
    emit timeBetweenShotsChanged();
    emit additionalTimeDelayChanged();
}

void TransectStyleComplexItem::_queryTransectsPathHeightInfo(void)
{
    _transectsPathHeightInfo.clear();
    emit readyForSaveStateChanged();

    if (_transects.count()) {
        // We don't actually send the query until this timer times out. This way we only send
        // the latest request if we get a bunch in a row.
        _terrainQueryTimer.start();
    }
}

void TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo(void)
{
    // Clear any previous query
    if (_terrainPolyPathQuery) {
        // FIXME: We should really be blowing away any previous query here. But internally that is difficult to implement so instead we let
        // it complete and drop the results.
#if 0
        // Toss previous query
        _terrainPolyPathQuery->deleteLater();
#else
        // Let the signal fall on the floor
        disconnect(_terrainPolyPathQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
#endif
        _terrainPolyPathQuery = nullptr;
    }

    // Append all transects into a single PolyPath query

    QList<QGeoCoordinate> transectPoints;

    for (const QList<CoordInfo_t>& transect: _transects) {
        for (const CoordInfo_t& coordInfo: transect) {
            transectPoints.append(coordInfo.coord);
        }
    }

    if (transectPoints.count() > 1) {
        _terrainPolyPathQuery = new TerrainPolyPathQuery(this);
        connect(_terrainPolyPathQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
        _terrainPolyPathQuery->requestData(transectPoints);
    }
}

void TransectStyleComplexItem::_polyPathTerrainData(bool success, const QList<TerrainPathQuery::PathHeightInfo_t>& rgPathHeightInfo)
{
    _transectsPathHeightInfo.clear();
    emit readyForSaveStateChanged();

    if (success) {
        // Break out into individual transects
        int pathHeightIndex = 0;
        for (int i=0; i<_transects.count(); i++) {
            _transectsPathHeightInfo.append(QList<TerrainPathQuery::PathHeightInfo_t>());
            int cPathHeight = _transects[i].count() - 1;
            while (cPathHeight-- > 0) {
                _transectsPathHeightInfo[i].append(rgPathHeightInfo[pathHeightIndex++]);
            }
            pathHeightIndex++;  // There is an extra on between each transect
        }
        emit readyForSaveStateChanged();

        // Now that we have terrain data we can adjust
        _adjustTransectsForTerrain();
    }

    if (_terrainPolyPathQuery != sender()) {
        qWarning() << "TransectStyleComplexItem::_polyPathTerrainData _terrainPolyPathQuery != sender()";
    }
    disconnect(_terrainPolyPathQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
    _terrainPolyPathQuery = nullptr;
}

TransectStyleComplexItem::ReadyForSaveState TransectStyleComplexItem::readyForSaveState(void) const
{
    bool terrainReady = _followTerrain ? _transectsPathHeightInfo.count() : true;
    bool polygonNotReady = !_surveyAreaPolygon.isValid();
    return (polygonNotReady || _wizardMode) ?
                NotReadyForSaveData :
                (terrainReady ? ReadyForSave : NotReadyForSaveTerrain);
}

void TransectStyleComplexItem::_adjustTransectsForTerrain(void)
{
    if (_followTerrain) {
        if (readyForSaveState() != ReadyForSave) {
            qCWarning(TransectStyleComplexItemLog) << "_adjustTransectPointsForTerrain called when terrain data not ready";
            qgcApp()->showMessage(tr("INTERNAL ERROR: TransectStyleComplexItem::_adjustTransectPointsForTerrain called when terrain data not ready. Plan will be incorrect."));
            return;
        }

        // First step is add all interstitial points at max resolution
        for (int i=0; i<_transects.count(); i++) {
            _addInterstitialTerrainPoints(_transects[i], _transectsPathHeightInfo[i]);
        }

        for (int i=0; i<_transects.count(); i++) {
            _adjustForMaxRates(_transects[i]);
        }

        for (int i=0; i<_transects.count(); i++) {
            _adjustForTolerance(_transects[i]);
        }

        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

/// Returns the altitude in between the two points on a line.
///     @param precentTowardsTo Example: .25 = twenty five percent along the distance of from to to
double TransectStyleComplexItem::_altitudeBetweenCoords(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord, double percentTowardsTo)
{
    double fromAlt = fromCoord.altitude();
    double toAlt = toCoord.altitude();
    double altDiff = toAlt - fromAlt;
    return fromAlt + (altDiff * percentTowardsTo);
}

/// Determine the maximum height within the PathHeightInfo
///     @param fromIndex First height index to consider
///     @param fromIndex Last height index to consider
///     @param[out] maxHeight Maximum height
/// @return index within PathHeightInfo_t.heights which contains maximum height, -1 no height data in between from and to indices
int TransectStyleComplexItem::_maxPathHeight(const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo, int fromIndex, int toIndex, double& maxHeight)
{
    maxHeight = qQNaN();

    if (toIndex - fromIndex < 2) {
        return -1;
    }

    fromIndex++;
    toIndex--;

    int maxIndex = fromIndex;
    maxHeight = pathHeightInfo.heights[fromIndex];

    for (int i=fromIndex; i<=toIndex; i++) {
        if (pathHeightInfo.heights[i] > maxHeight) {
            maxIndex = i;
            maxHeight = pathHeightInfo.heights[i];
        }
    }

    return maxIndex;
}

void TransectStyleComplexItem::_adjustForMaxRates(QList<CoordInfo_t>& transect)
{
    double maxClimbRate = _terrainAdjustMaxClimbRateFact.rawValue().toDouble();
    double maxDescentRate = _terrainAdjustMaxDescentRateFact.rawValue().toDouble();
    double flightSpeed = _missionFlightStatus.vehicleSpeed;

    if (qIsNaN(flightSpeed) || (maxClimbRate == 0 && maxDescentRate == 0)) {
        if (qIsNaN(flightSpeed)) {
            qWarning() << "TransectStyleComplexItem::_adjustForMaxRates called with flightSpeed = NaN";
        }
        return;
    }

    if (maxClimbRate > 0) {
        // Adjust climb rates
        bool climbRateAdjusted;
        do {
            //qDebug() << "climbrate pass";
            climbRateAdjusted = false;
            for (int i=0; i<transect.count() - 1; i++) {
                QGeoCoordinate& fromCoord = transect[i].coord;
                QGeoCoordinate& toCoord = transect[i+1].coord;

                double altDifference = toCoord.altitude() - fromCoord.altitude();
                double distance = fromCoord.distanceTo(toCoord);
                double seconds = distance / flightSpeed;
                double climbRate = altDifference / seconds;

                //qDebug() << QString("Index:%1 altDifference:%2 distance:%3 seconds:%4 climbRate:%5").arg(i).arg(altDifference).arg(distance).arg(seconds).arg(climbRate);

                if (climbRate > 0 && climbRate - maxClimbRate > 0.1) {
                    double maxAltitudeDelta = maxClimbRate * seconds;
                    fromCoord.setAltitude(toCoord.altitude() - maxAltitudeDelta);
                    //qDebug() << "Adjusting";
                    climbRateAdjusted = true;
                }
            }
        } while (climbRateAdjusted);
    }

    if (maxDescentRate > 0) {
        // Adjust descent rates
        bool descentRateAdjusted;
        maxDescentRate = -maxDescentRate;
        do {
            //qDebug() << "descent rate pass";
            descentRateAdjusted = false;
            for (int i=1; i<transect.count(); i++) {
                QGeoCoordinate& fromCoord = transect[i-1].coord;
                QGeoCoordinate& toCoord = transect[i].coord;

                double altDifference = toCoord.altitude() - fromCoord.altitude();
                double distance = fromCoord.distanceTo(toCoord);
                double seconds = distance / flightSpeed;
                double descentRate = altDifference / seconds;

                //qDebug() << QString("Index:%1 altDifference:%2 distance:%3 seconds:%4 descentRate:%5").arg(i).arg(altDifference).arg(distance).arg(seconds).arg(descentRate);

                if (descentRate < 0 && descentRate - maxDescentRate < -0.1) {
                    double maxAltitudeDelta = maxDescentRate * seconds;
                    toCoord.setAltitude(fromCoord.altitude() + maxAltitudeDelta);
                    //qDebug() << "Adjusting";
                    descentRateAdjusted = true;
                }
            }
        } while (descentRateAdjusted);
    }
}

void TransectStyleComplexItem::_adjustForTolerance(QList<CoordInfo_t>& transect)
{
    QList<CoordInfo_t> adjustedPoints;

    double tolerance = _terrainAdjustToleranceFact.rawValue().toDouble();

    int coordIndex = 0;
    while (coordIndex < transect.count()) {
        const CoordInfo_t& fromCoordInfo = transect[coordIndex];

        adjustedPoints.append(fromCoordInfo);

        // Walk forward until we fall out of tolerence or find a fixed point
        while (++coordIndex < transect.count()) {
            const CoordInfo_t& toCoordInfo = transect[coordIndex];
            if (toCoordInfo.coordType != CoordTypeInteriorTerrainAdded || qAbs(fromCoordInfo.coord.altitude() - toCoordInfo.coord.altitude()) > tolerance) {
                adjustedPoints.append(toCoordInfo);
                coordIndex++;
                break;
            }
        }
    }

#if 0
    qDebug() << "_adjustForTolerance";
    for (const TransectStyleComplexItem::CoordInfo_t& coordInfo: adjustedPoints) {
        qDebug() << coordInfo.coordType;
    }
#endif

    transect = adjustedPoints;
}

void TransectStyleComplexItem::_addInterstitialTerrainPoints(QList<CoordInfo_t>& transect, const QList<TerrainPathQuery::PathHeightInfo_t>& transectPathHeightInfo)
{
    QList<CoordInfo_t> adjustedTransect;

    double requestedAltitude = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    for (int i=0; i<transect.count() - 1; i++) {
        CoordInfo_t fromCoordInfo = transect[i];
        CoordInfo_t toCoordInfo = transect[i+1];

        double azimuth = fromCoordInfo.coord.azimuthTo(toCoordInfo.coord);
        double distance = fromCoordInfo.coord.distanceTo(toCoordInfo.coord);

        const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo = transectPathHeightInfo[i];

        fromCoordInfo.coord.setAltitude(pathHeightInfo.heights.first() + requestedAltitude);
        toCoordInfo.coord.setAltitude(pathHeightInfo.heights.last() + requestedAltitude);

        if (i == 0) {
            adjustedTransect.append(fromCoordInfo);
        }

        int cHeights = pathHeightInfo.heights.count();
        for (int pathHeightIndex=1; pathHeightIndex<cHeights - 1; pathHeightIndex++) {
            double interstitialTerrainHeight = pathHeightInfo.heights[pathHeightIndex];
            double percentTowardsTo = (1.0 / (cHeights - 1)) * pathHeightIndex;

            CoordInfo_t interstitialCoordInfo;
            interstitialCoordInfo.coordType = CoordTypeInteriorTerrainAdded;
            interstitialCoordInfo.coord = fromCoordInfo.coord.atDistanceAndAzimuth(distance * percentTowardsTo, azimuth);
            interstitialCoordInfo.coord.setAltitude(interstitialTerrainHeight + requestedAltitude);

            adjustedTransect.append(interstitialCoordInfo);
        }

        adjustedTransect.append(toCoordInfo);
    }

    CoordInfo_t lastCoordInfo = transect.last();
    const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo = transectPathHeightInfo.last();
    lastCoordInfo.coord.setAltitude(pathHeightInfo.heights.last() + requestedAltitude);
    adjustedTransect.append(lastCoordInfo);

#if 0
    qDebug() << "_addInterstitialTerrainPoints";
    for (const TransectStyleComplexItem::CoordInfo_t& coordInfo: adjustedTransect) {
        qDebug() << coordInfo.coordType;
    }
#endif

    transect = adjustedTransect;
}

void TransectStyleComplexItem::setFollowTerrain(bool followTerrain)
{
    if (followTerrain != _followTerrain) {
        _followTerrain = followTerrain;
        emit followTerrainChanged(followTerrain);
    }
}

int TransectStyleComplexItem::lastSequenceNumber(void) const
{
    if (_loadedMissionItems.count()) {
        // We have stored mission items, just use those
        return _sequenceNumber + _loadedMissionItems.count() - 1;
    } else {
        // We have to determine from transects
        int itemCount = 0;

        for (const QList<CoordInfo_t>& rgCoordInfo: _transects) {
            itemCount += rgCoordInfo.count() * (hoverAndCaptureEnabled() ? 2 : 1);
        }


        if (!hoverAndCaptureEnabled() && triggerCamera()) {
            if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
                // One camera start/stop for beginning/end of entire survey
                itemCount += 2;
                // One camera start for each transect
                itemCount += _transects.count();
            } else {
                // Each transect will have a camera start and stop in it
                itemCount += _transects.count() * 2;
            }
        }

        return _sequenceNumber + itemCount - 1;
    }
}

bool TransectStyleComplexItem::coordinateHasRelativeAltitude(void) const
{
    return _cameraCalc.distanceToSurfaceRelative();
}

bool TransectStyleComplexItem::exitCoordinateHasRelativeAltitude(void) const
{
    return coordinateHasRelativeAltitude();
}

void TransectStyleComplexItem::_followTerrainChanged(bool followTerrain)
{
    _cameraCalc.setDistanceToSurfaceRelative(!followTerrain);
    if (followTerrain) {
        _refly90DegreesFact.setRawValue(false);
        _hoverAndCaptureFact.setRawValue(false);
    }
}

void TransectStyleComplexItem::_handleHoverAndCaptureEnabled(QVariant enabled)
{
    if (enabled.toBool() && _cameraTriggerInTurnAroundFact.rawValue().toBool()) {
        qDebug() << "_handleHoverAndCaptureEnabled";
        _cameraTriggerInTurnAroundFact.setRawValue(false);
    }
}
