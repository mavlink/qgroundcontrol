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
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"
#include "QGCApplication.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"

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

TransectStyleComplexItem::TransectStyleComplexItem(PlanMasterController* masterController, bool flyView, QString settingsGroup, QObject* parent)
    : ComplexMissionItem                (masterController, flyView, parent)
    , _cameraCalc                       (masterController, settingsGroup)
    , _metaDataMap                      (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/TransectStyle.SettingsGroup.json"), this))
    , _turnAroundDistanceFact           (settingsGroup, _metaDataMap[_controllerVehicle->multiRotor() ? turnAroundDistanceMultiRotorName : turnAroundDistanceName])
    , _cameraTriggerInTurnAroundFact    (settingsGroup, _metaDataMap[cameraTriggerInTurnAroundName])
    , _hoverAndCaptureFact              (settingsGroup, _metaDataMap[hoverAndCaptureName])
    , _refly90DegreesFact               (settingsGroup, _metaDataMap[refly90DegreesName])
    , _terrainAdjustToleranceFact       (settingsGroup, _metaDataMap[terrainAdjustToleranceName])
    , _terrainAdjustMaxClimbRateFact    (settingsGroup, _metaDataMap[terrainAdjustMaxClimbRateName])
    , _terrainAdjustMaxDescentRateFact  (settingsGroup, _metaDataMap[terrainAdjustMaxDescentRateName])
{
    _terrainQueryTimer.setInterval(qgcApp()->runningUnitTests() ? 10 : _terrainQueryTimeoutMsecs);
    _terrainQueryTimer.setSingleShot(true);
    connect(&_terrainQueryTimer, &QTimer::timeout, this, &TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo);

    // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &TransectStyleComplexItem::_updateFlightPathSegmentsSignal, this, &TransectStyleComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&TransectStyleComplexItem::_updateFlightPathSegmentsSignal));

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
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,         this, &TransectStyleComplexItem::_rebuildTransects);

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

    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,                         this, &TransectStyleComplexItem::_amslEntryAltChanged);
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,                         this, &TransectStyleComplexItem::_amslExitAltChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged,  this, &TransectStyleComplexItem::_amslEntryAltChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged,  this, &TransectStyleComplexItem::_amslExitAltChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged,  this, &TransectStyleComplexItem::_updateFlightPathSegmentsSignal);

    connect(&_cameraCalc,                               &CameraCalc::distanceToSurfaceRelativeChanged,  _missionController, &MissionController::recalcTerrainProfile);

    connect(&_hoverAndCaptureFact,                      &Fact::rawValueChanged,         this, &TransectStyleComplexItem::_handleHoverAndCaptureEnabled);

    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::greatestDistanceToChanged);
    connect(this,                                       &TransectStyleComplexItem::followTerrainChanged,        this, &TransectStyleComplexItem::_followTerrainChanged);
    connect(this,                                       &TransectStyleComplexItem::wizardModeChanged,           this, &TransectStyleComplexItem::readyForSaveStateChanged);

    connect(_missionController,                         &MissionController::plannedHomePositionChanged,         this, &TransectStyleComplexItem::_amslEntryAltChanged);
    connect(_missionController,                         &MissionController::plannedHomePositionChanged,         this, &TransectStyleComplexItem::_amslExitAltChanged);

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

        if (!forPresets) {
            // We have to grovel through mission items to determine min/max alt
            _minAMSLAltitude = 0;
            _maxAMSLAltitude = 0;
            for (const MissionItem* missionItem: _loadedMissionItems) {
                if (missionItem->command() == MAV_CMD_NAV_WAYPOINT || missionItem->command() == MAV_CMD_CONDITION_GATE) {
                    _minAMSLAltitude = qMin(_minAMSLAltitude, missionItem->param7());
                    _maxAMSLAltitude = qMax(_maxAMSLAltitude, missionItem->param7());
                }
            }
        }
    } else if (!forPresets) {
        _minAMSLAltitude = _maxAMSLAltitude = _cameraCalc.distanceToSurface()->rawValue().toDouble() + (_cameraCalc.distanceToSurfaceRelative() ? _missionController->plannedHomePosition().altitude() : 0);
    }

    if (!forPresets) {
        emit minAMSLAltitudeChanged(_minAMSLAltitude);
        emit maxAMSLAltitudeChanged(_maxAMSLAltitude);
        _amslEntryAltChanged();
        _amslExitAltChanged();
        emit _updateFlightPathSegmentsSignal();
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
    if (!qFuzzyCompare(_vehicleSpeed, missionFlightStatus.vehicleSpeed)) {
        _vehicleSpeed = missionFlightStatus.vehicleSpeed;
        // Vehicle speed change affects max climb/descent rates calcs for terrain so we need to re-adjust
        _rebuildTransects();
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
    return _turnAroundDistance() > 0;
}

double TransectStyleComplexItem::_turnAroundDistance(void) const
{
    return _turnAroundDistanceFact.rawValue().toDouble();
}

bool TransectStyleComplexItem::hoverAndCaptureAllowed(void) const
{
    return _controllerVehicle->multiRotor() || _controllerVehicle->vtol();
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
        // We won't know min/max till were done
        _minAMSLAltitude = _maxAMSLAltitude = qQNaN();
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

        _minAMSLAltitude = _maxAMSLAltitude = requestedAltitude + (_cameraCalc.distanceToSurfaceRelative() ? _missionController->plannedHomePosition().altitude() : 0);
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

    emit minAMSLAltitudeChanged(_minAMSLAltitude);
    emit maxAMSLAltitudeChanged(_maxAMSLAltitude);

    emit _updateFlightPathSegmentsSignal();
    _amslEntryAltChanged();
    _amslExitAltChanged();
}

void TransectStyleComplexItem::_segmentTerrainCollisionChanged(bool terrainCollision)
{
    ComplexMissionItem::_segmentTerrainCollisionChanged(terrainCollision);
    _surveyAreaPolygon.setShowAltColor(_cTerrainCollisionSegments != 0);
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void TransectStyleComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
{
    if (_cTerrainCollisionSegments != 0) {
        _cTerrainCollisionSegments = 0;
        emit terrainCollisionChanged(false);
        _surveyAreaPolygon.setShowAltColor(false);
    }

    _flightPathSegments.beginReset();
    _flightPathSegments.clearAndDeleteContents();

    QGeoCoordinate lastTransectExit = QGeoCoordinate();
    if (_followTerrain) {
        if (_loadedMissionItems.count()) {
            // We are working from loaded mission items from a plan. We have to grovel through the mission items
            // building up segments from waypoints.
            QGeoCoordinate prevCoord = QGeoCoordinate();
            double prevAlt = 0;
            for (const MissionItem* missionItem: _loadedMissionItems) {
                if (missionItem->command() == MAV_CMD_NAV_WAYPOINT || missionItem->command() == MAV_CMD_CONDITION_GATE) {
                    if (prevCoord.isValid()) {
                        _appendFlightPathSegment(prevCoord, prevAlt, missionItem->coordinate(), missionItem->param7());
                    }
                    prevCoord = missionItem->coordinate();
                    prevAlt = missionItem->param7();
                }
            }
        } else {
            // We are working for live transect data. We don't show flight path segments until terrain data is back and recalced
            if (_transectsPathHeightInfo.count()) {
                // The altitudes of the flight path segments for follow terrain can all occur at different altitudes. Because of that we
                // need to to add FlightPathSegment's for every section in order to get good terrain collision data and flight path profile.
                for (const QList<CoordInfo_t>& transect: _transects) {
                    // Turnaround segment
                    if (lastTransectExit.isValid()) {
                        const QGeoCoordinate& coord2 = transect.first().coord;
                        _appendFlightPathSegment(lastTransectExit, lastTransectExit.altitude(), coord2, coord2.altitude());
                    }

                    QGeoCoordinate prevCoordInTransect = QGeoCoordinate();
                    for (const CoordInfo_t& coordInfo: transect) {
                        if (prevCoordInTransect.isValid()) {
                            const QGeoCoordinate& coord2 = coordInfo.coord;
                            _appendFlightPathSegment(prevCoordInTransect, prevCoordInTransect.altitude(), coord2, coord2.altitude());
                        }
                        prevCoordInTransect = coordInfo.coord;
                    }

                    lastTransectExit = transect.last().coord;
                }
            }
        }
    } else {
        // Since we aren't following terrain all the transects are at the same height. We can use _visualTransectPoints to build the
        // flight path segments. The benefit of _visualTransectPoints is that it is also available when a Plan is loaded from a file
        // and we are working from  stored mission items. In that case we don't have _transects set up for use.
        QGeoCoordinate prevCoord;
        double surveyAlt = amslEntryAlt();
        for (const QVariant& varCoord: _visualTransectPoints) {
            QGeoCoordinate thisCoord = varCoord.value<QGeoCoordinate>();
            if (prevCoord.isValid()) {
                _appendFlightPathSegment(prevCoord,  surveyAlt, thisCoord,  surveyAlt);
            }
            prevCoord = thisCoord;
        }
    }

    _flightPathSegments.endReset();

    if (_cTerrainCollisionSegments != 0) {
        emit terrainCollisionChanged(true);
        _surveyAreaPolygon.setShowAltColor(true);
    }

    _masterController->missionController()->recalcTerrainProfile();
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
    if (_currentTerrainFollowQuery) {
        // We are already waiting on another query. We don't care about those results any more.
        disconnect(_currentTerrainFollowQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
        _currentTerrainFollowQuery = nullptr;
    }

    // Append all transects into a single PolyPath query
    QList<QGeoCoordinate> transectPoints;
    for (const QList<CoordInfo_t>& transect: _transects) {
        for (const CoordInfo_t& coordInfo: transect) {
            transectPoints.append(coordInfo.coord);
        }
    }

    if (transectPoints.count() > 1) {
        _currentTerrainFollowQuery = new TerrainPolyPathQuery(true /* autoDelete */);
        connect(_currentTerrainFollowQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
        _currentTerrainFollowQuery->requestData(transectPoints);
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


    QObject* object = qobject_cast<QObject*>(sender());
    if (object) {
        object->deleteLater();
    }
    _currentTerrainFollowQuery = nullptr;
}

TransectStyleComplexItem::ReadyForSaveState TransectStyleComplexItem::readyForSaveState(void) const
{
    bool terrainReady = false;
    if (_followTerrain) {
        if (_loadedMissionItems.count()) {
            // We have loaded mission items. Everything is ready to go.
            terrainReady = true;
        } else {
            // Survey is currently being designed. We aren't ready if we don't have terrain heights yet.
            terrainReady = _transectsPathHeightInfo.count();
        }
    } else {
        // Now following terrain so always ready on terrain
        terrainReady = true;
    }
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
            qgcApp()->showAppMessage(tr("INTERNAL ERROR: TransectStyleComplexItem::_adjustTransectPointsForTerrain called when terrain data not ready. Plan will be incorrect."));
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
        emit _updateFlightPathSegmentsSignal();

        _amslEntryAltChanged();
        _amslExitAltChanged();

        _minAMSLAltitude = 0;
        _maxAMSLAltitude = 0;
        for (const QList<CoordInfo_t>& transect: _transects) {
            for (const CoordInfo_t& coordInfo: transect) {
                _minAMSLAltitude = qMin(_minAMSLAltitude, coordInfo.coord.altitude());
                _maxAMSLAltitude = qMax(_maxAMSLAltitude, coordInfo.coord.altitude());
            }
        }
        emit minAMSLAltitudeChanged(_minAMSLAltitude);
        emit maxAMSLAltitudeChanged(_maxAMSLAltitude);
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
    double maxClimbRate     = _terrainAdjustMaxClimbRateFact.rawValue().toDouble();
    double maxDescentRate   = _terrainAdjustMaxDescentRateFact.rawValue().toDouble();
    double flightSpeed      = _vehicleSpeed;

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

    if (transect.count()) {
        double          tolerance =     _terrainAdjustToleranceFact.rawValue().toDouble();
        CoordInfo_t&    lastCoordInfo = transect.first();

        adjustedPoints.append(lastCoordInfo);

        int coordIndex = 1;
        while (coordIndex < transect.count()) {
            // Walk forward until we fall out of tolerence. When we fall out of tolerance add that point.
            // We always add non-interstitial points no matter what.
            const CoordInfo_t& nextCoordInfo = transect[coordIndex];
            if (nextCoordInfo.coordType != CoordTypeInteriorTerrainAdded || qAbs(lastCoordInfo.coord.altitude() - nextCoordInfo.coord.altitude()) > tolerance) {
                adjustedPoints.append(nextCoordInfo);
                lastCoordInfo = nextCoordInfo;
            }
            coordIndex++;
        }
    }

    transect = adjustedPoints;
}

void TransectStyleComplexItem::_addInterstitialTerrainPoints(QList<CoordInfo_t>& transect, const QList<TerrainPathQuery::PathHeightInfo_t>& transectPathHeightInfo)
{
    QList<CoordInfo_t> adjustedTransect;

    double distanceToSurface = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    for (int i=0; i<transect.count() - 1; i++) {
        CoordInfo_t fromCoordInfo = transect[i];
        CoordInfo_t toCoordInfo = transect[i+1];

        double azimuth = fromCoordInfo.coord.azimuthTo(toCoordInfo.coord);
        double distance = fromCoordInfo.coord.distanceTo(toCoordInfo.coord);

        const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo = transectPathHeightInfo[i];

        fromCoordInfo.coord.setAltitude(pathHeightInfo.heights.first() + distanceToSurface);
        toCoordInfo.coord.setAltitude(pathHeightInfo.heights.last() + distanceToSurface);

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
            interstitialCoordInfo.coord.setAltitude(interstitialTerrainHeight + distanceToSurface);

            adjustedTransect.append(interstitialCoordInfo);
        }

        adjustedTransect.append(toCoordInfo);
    }

    CoordInfo_t lastCoordInfo = transect.last();
    const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo = transectPathHeightInfo.last();
    lastCoordInfo.coord.setAltitude(pathHeightInfo.heights.last() + distanceToSurface);
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
    } else if (_transects.count() == 0) {
        // Polygon has not yet been set so we just return back a one item complex item for now
        return _sequenceNumber;
    } else {
        // We have to determine from transects
        int itemCount = 0;

        for (const QList<CoordInfo_t>& rgCoordInfo: _transects) {
            int commandsPerCoord = 1; // Waypoint command
            if (hoverAndCaptureEnabled()) {
                commandsPerCoord++; // Camera trigger
            }
            itemCount += rgCoordInfo.count() * commandsPerCoord;
            if (hoverAndCaptureEnabled() && _turnAroundDistance() != 0) {
                // The turnaround points do not have camera triggers on them
                itemCount -= 2;
            }
        }


        if (!hoverAndCaptureEnabled() && triggerCamera()) {
            if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
                itemCount += _transects.count();    // One camera start for each transect entry
                itemCount++;                        // Single camera stop and the very end
                if (_turnAroundDistance() != 0) {
                    // If there are turnarounds then there is an additional camera start on the first turnaround
                    itemCount++;
                }
            } else {
                // Each transect will have a camera start and stop in it
                itemCount += _transects.count() * 2;
            }
        }

        return _sequenceNumber + itemCount - 1;
    }
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
        _cameraTriggerInTurnAroundFact.setRawValue(false);
    }
}

void TransectStyleComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    if (_loadedMissionItems.count()) {
        // We have mission items from the loaded plan, use those
        _appendLoadedMissionItems(items, missionItemParent);
    } else {
        // Build the mission items on the fly
        _buildAndAppendMissionItems(items, missionItemParent);
    }
}

void TransectStyleComplexItem::_appendWaypoint(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, MAV_FRAME mavFrame, float holdTime, const QGeoCoordinate& coordinate)
{
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        mavFrame,
                                        holdTime,
                                        0.0,                                         // No acceptance radius specified
                                        0.0,                                         // Pass through waypoint
                                        std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        coordinate.altitude(),
                                        true,                                        // autoContinue
                                        false,                                       // isCurrentItem
                                        missionItemParent);
    items.append(item);
}

void TransectStyleComplexItem::_appendSinglePhotoCapture(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum)
{
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_IMAGE_START_CAPTURE,
                                        MAV_FRAME_MISSION,
                                        0,                                   // Reserved (Set to 0)
                                        0,                                   // Interval (none)
                                        1,                                   // Take 1 photo
                                        qQNaN(), qQNaN(), qQNaN(), qQNaN(),  // param 4-7 reserved
                                        true,                                // autoContinue
                                        false,                               // isCurrentItem
                                        missionItemParent);
    items.append(item);
}

void TransectStyleComplexItem::_appendConditionGate(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, MAV_FRAME mavFrame, const QGeoCoordinate& coordinate)
{
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_CONDITION_GATE,
                                        mavFrame,
                                        0,                                           // Gate is orthogonal to path
                                        0,                                           // Ignore altitude
                                        0, 0,                                        // Param 3-4 ignored
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        0,                                           // No altitude
                                        true,                                        // autoContinue
                                        false,                                       // isCurrentItem
                                        missionItemParent);
    items.append(item);
}

void TransectStyleComplexItem::_appendCameraTriggerDistance(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, float triggerDistance)
{
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                        MAV_FRAME_MISSION,
                                        triggerDistance,
                                        0,                              // shutter integration (ignore)
                                        1,                              // 1 - trigger one image immediately, both and entry and exit to get full coverage
                                        0, 0, 0, 0,                     // param 4-7 unused
                                        true,                           // autoContinue
                                        false,                          // isCurrentItem
                                        missionItemParent);
    items.append(item);
}

void TransectStyleComplexItem::_appendCameraTriggerDistanceUpdatePoint(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, MAV_FRAME mavFrame, const QGeoCoordinate& coordinate, bool useConditionGate, float triggerDistance)
{
    if (useConditionGate) {
        _appendConditionGate(items, missionItemParent, seqNum, mavFrame, coordinate);
    } else {
        _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordinate);
    }
    _appendCameraTriggerDistance(items, missionItemParent, seqNum, triggerDistance);
}

void TransectStyleComplexItem::_buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(TransectStyleComplexItemLog) << "_buildAndAppendMissionItems";

    // Now build the mission items from the transect points

    int seqNum =                    _sequenceNumber;
    bool imagesInTurnaround =       _cameraTriggerInTurnAroundFact.rawValue().toBool();
    bool hasTurnarounds =           _turnAroundDistance() != 0;
    bool addTriggerAtBeginningEnd = !hoverAndCaptureEnabled() && imagesInTurnaround && triggerCamera();
    bool useConditionGate =         _controllerVehicle->firmwarePlugin()->supportedMissionCommands(QGCMAVLink::VehicleClassGeneric).contains(MAV_CMD_CONDITION_GATE) &&
            triggerCamera() &&
            !hoverAndCaptureEnabled();

    MAV_FRAME mavFrame = followTerrain() || !_cameraCalc.distanceToSurfaceRelative() ? MAV_FRAME_GLOBAL : MAV_FRAME_GLOBAL_RELATIVE_ALT;

    // Note: The code below is written to be understable as oppose to being compact and/or remove duplicate code
    int transectIndex = 0;
    for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
        bool entryTurnaround = true;
        for (const CoordInfo_t& transectCoordInfo: transect) {
            switch (transectCoordInfo.coordType) {
            case CoordTypeTurnaround:
            {
                bool firstEntryTurnaround = transectIndex == 0 && entryTurnaround;
                bool lastExitTurnaround = transectIndex == _transects.count() - 1 && !entryTurnaround;
                if (addTriggerAtBeginningEnd && (firstEntryTurnaround || lastExitTurnaround)) {
                    _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, transectCoordInfo.coord, useConditionGate, firstEntryTurnaround ? triggerDistance() : 0);
                } else {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, transectCoordInfo.coord);
                }
                entryTurnaround = false;
            }
                break;
            case CoordTypeInterior:
            case CoordTypeInteriorTerrainAdded:
                _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, transectCoordInfo.coord);
                break;
            case CoordTypeInteriorHoverTrigger:
                _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, transectCoordInfo.coord);
                _appendSinglePhotoCapture(items, missionItemParent, seqNum);
                break;
            case CoordTypeSurveyEntry:
                if (triggerCamera()) {
                    if (hoverAndCaptureEnabled()) {
                        _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, transectCoordInfo.coord);
                        _appendSinglePhotoCapture(items, missionItemParent, seqNum);
                    } else {
                        // We always add a trigger start to survey entry. Even for imagesInTurnaround = true. This allows you to resume a mission and refly a transect
                        _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, transectCoordInfo.coord, useConditionGate, triggerDistance());
                    }
                } else {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, transectCoordInfo.coord);
                }
                break;
            case CoordTypeSurveyExit:
                bool lastSurveyExit = transectIndex == _transects.count() - 1;
                if (triggerCamera()) {
                    if (hoverAndCaptureEnabled()) {
                        _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, transectCoordInfo.coord);
                        _appendSinglePhotoCapture(items, missionItemParent, seqNum);
                    } else if (addTriggerAtBeginningEnd && !hasTurnarounds && lastSurveyExit) {
                        _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, transectCoordInfo.coord, useConditionGate, 0 /* triggerDistance */);
                    } else if (imagesInTurnaround) {
                        _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, transectCoordInfo.coord);
                    } else {
                        // If we get this far it means the camera is triggering start/stop for each transect
                        _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, transectCoordInfo.coord, useConditionGate, 0 /* triggerDistance */);
                    }
                } else {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, transectCoordInfo.coord);
                }
                break;
            }
        }
        transectIndex++;
    }
}

void TransectStyleComplexItem::_appendLoadedMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(TransectStyleComplexItemLog) << "_appendLoadedMissionItems";

    int seqNum = _sequenceNumber;

    for (const MissionItem* loadedMissionItem: _loadedMissionItems) {
        MissionItem* item = new MissionItem(*loadedMissionItem, missionItemParent);
        item->setSequenceNumber(seqNum++);
        items.append(item);
    }
}

void TransectStyleComplexItem::addKMLVisuals(KMLPlanDomDocument& domDocument)
{
    // We add the survey area polygon as a Placemark

    QDomElement placemarkElement = domDocument.addPlacemark(QStringLiteral("Survey Area"), true);
    QDomElement polygonElement = _surveyAreaPolygon.kmlPolygonElement(domDocument);

    placemarkElement.appendChild(polygonElement);
    domDocument.addTextElement(placemarkElement, "styleUrl", QStringLiteral("#%1").arg(domDocument.surveyPolygonStyleName));
    domDocument.appendChildToRoot(placemarkElement);
}

void TransectStyleComplexItem::_recalcComplexDistance(void)
{
    _complexDistance = 0;
    for (int i=0; i<_visualTransectPoints.count() - 1; i++) {
        _complexDistance += _visualTransectPoints[i].value<QGeoCoordinate>().distanceTo(_visualTransectPoints[i+1].value<QGeoCoordinate>());
    }
    emit complexDistanceChanged();
}

double TransectStyleComplexItem::amslEntryAlt(void) const
{
    if (_followTerrain) {
        if (_loadedMissionItems.count()) {
            return _loadedMissionItems.first()->param7();
        } else {
            if (_transectCount() == 0) {
                return qQNaN();
            } else {
                bool addHomeAlt = !followTerrain() && _cameraCalc.distanceToSurfaceRelative();

                return _transects.first().first().coord.altitude() + (addHomeAlt ?  _missionController->plannedHomePosition().altitude() : 0);
            }
        }
    } else {
        return _cameraCalc.distanceToSurface()->rawValue().toDouble() + (_cameraCalc.distanceToSurfaceRelative() ? _missionController->plannedHomePosition().altitude() : 0) ;
    }
}

double TransectStyleComplexItem::amslExitAlt(void) const
{
    if (_followTerrain) {
        if (_loadedMissionItems.count()) {
            return _loadedMissionItems.last()->param7();
        } else {
            if (_transectCount() == 0) {
                return qQNaN();
            } else {
                bool addHomeAlt = !followTerrain() && _cameraCalc.distanceToSurfaceRelative();

                return _transects.last().last().coord.altitude() + (addHomeAlt ?  _missionController->plannedHomePosition().altitude() : 0);
            }
        }
    } else {
        return _cameraCalc.distanceToSurface()->rawValue().toDouble() + (_cameraCalc.distanceToSurfaceRelative() ? _missionController->plannedHomePosition().altitude() : 0) ;
    }
}

void TransectStyleComplexItem::applyNewAltitude(double newAltitude)
{
    _cameraCalc.valueSetIsDistance()->setRawValue(true);
    _cameraCalc.distanceToSurface()->setRawValue(newAltitude);
    _cameraCalc.setDistanceToSurfaceRelative(true);
}
