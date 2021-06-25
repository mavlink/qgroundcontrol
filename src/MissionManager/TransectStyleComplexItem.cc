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
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(TransectStyleComplexItemLog, "TransectStyleComplexItemLog")

const char* TransectStyleComplexItem::turnAroundDistanceName                = "TurnAroundDistance";
const char* TransectStyleComplexItem::turnAroundDistanceMultiRotorName      = "TurnAroundDistanceMultiRotor";
const char* TransectStyleComplexItem::cameraTriggerInTurnAroundName         = "CameraTriggerInTurnAround";
const char* TransectStyleComplexItem::hoverAndCaptureName                   = "HoverAndCapture";
const char* TransectStyleComplexItem::refly90DegreesName                    = "Refly90Degrees";
const char* TransectStyleComplexItem::terrainAdjustToleranceName            = "TerrainAdjustTolerance";
const char* TransectStyleComplexItem::terrainAdjustMaxClimbRateName         = "TerrainAdjustMaxClimbRate";
const char* TransectStyleComplexItem::terrainAdjustMaxDescentRateName       = "TerrainAdjustMaxDescentRate";

const char* TransectStyleComplexItem::_jsonTransectStyleComplexItemKey      = "TransectStyleComplexItem";
const char* TransectStyleComplexItem::_jsonCameraCalcKey                    = "CameraCalc";
const char* TransectStyleComplexItem::_jsonVisualTransectPointsKey          = "VisualTransectPoints";
const char* TransectStyleComplexItem::_jsonItemsKey                         = "Items";
const char* TransectStyleComplexItem::_jsonTerrainFlightSpeed               = "TerrainFlightSpeed";
const char* TransectStyleComplexItem::_jsonCameraShotsKey                   = "CameraShots";

const char* TransectStyleComplexItem::_jsonTerrainFollowKeyDeprecated       = "FollowTerrain";

TransectStyleComplexItem::TransectStyleComplexItem(PlanMasterController* masterController, bool flyView, QString settingsGroup)
    : ComplexMissionItem                (masterController, flyView)
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
    _terrainPolyPathQueryTimer.setInterval(qgcApp()->runningUnitTests() ? 10 : _terrainQueryTimeoutMsecs);
    _terrainPolyPathQueryTimer.setSingleShot(true);
    connect(&_terrainPolyPathQueryTimer, &QTimer::timeout, this, &TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo);

    // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &TransectStyleComplexItem::_updateFlightPathSegmentsSignal, this, &TransectStyleComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&TransectStyleComplexItem::_updateFlightPathSegmentsSignal));

    connect(&_turnAroundDistanceFact,                   &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_hoverAndCaptureFact,                      &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_refly90DegreesFact,                       &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustMaxClimbRateFact,            &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustMaxDescentRateFact,          &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_terrainAdjustToleranceFact,               &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,        this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_cameraTriggerInTurnAroundFact,            &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintSide(),        &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.adjustedFootprintFrontal(),     &Fact::valueChanged,                this, &TransectStyleComplexItem::_rebuildTransects);
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,             this, &TransectStyleComplexItem::_rebuildTransects);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::_rebuildTransects);

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
    connect(&_terrainAdjustMaxClimbRateFact,            &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_terrainAdjustMaxDescentRateFact,          &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_terrainAdjustToleranceFact,               &Fact::valueChanged,            this, &TransectStyleComplexItem::_setDirty);
    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::_setDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::dirtyChanged,   this, &TransectStyleComplexItem::_setIfDirty);
    connect(&_cameraCalc,                               &CameraCalc::dirtyChanged,      this, &TransectStyleComplexItem::_setIfDirty);

    connect(&_surveyAreaPolygon,                        &QGCMapPolygon::pathChanged,    this, &TransectStyleComplexItem::coveredAreaChanged);

    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,             this, &TransectStyleComplexItem::_amslEntryAltChanged);
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,             this, &TransectStyleComplexItem::_amslExitAltChanged);
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,             this, &TransectStyleComplexItem::minAMSLAltitudeChanged);
    connect(_cameraCalc.distanceToSurface(),            &Fact::rawValueChanged,             this, &TransectStyleComplexItem::maxAMSLAltitudeChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::_amslEntryAltChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::_amslExitAltChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::minAMSLAltitudeChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::maxAMSLAltitudeChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::_updateFlightPathSegmentsSignal);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,   this, &TransectStyleComplexItem::_distanceModeChanged);
    connect(&_cameraCalc,                               &CameraCalc::distanceModeChanged,  _missionController, &MissionController::recalcTerrainProfile);

    connect(&_hoverAndCaptureFact,                      &Fact::rawValueChanged,         this, &TransectStyleComplexItem::_handleHoverAndCaptureEnabled);

    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::complexDistanceChanged);
    connect(this,                                       &TransectStyleComplexItem::visualTransectPointsChanged, this, &TransectStyleComplexItem::greatestDistanceToChanged);
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

    innerObject[JsonHelper::jsonVersionKey] =       2;
    innerObject[turnAroundDistanceName] =           _turnAroundDistanceFact.rawValue().toDouble();
    innerObject[cameraTriggerInTurnAroundName] =    _cameraTriggerInTurnAroundFact.rawValue().toBool();
    innerObject[hoverAndCaptureName] =              _hoverAndCaptureFact.rawValue().toBool();
    innerObject[refly90DegreesName] =               _refly90DegreesFact.rawValue().toBool();
    innerObject[_jsonCameraShotsKey] =              _cameraShots;

    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        innerObject[terrainAdjustToleranceName]         = _terrainAdjustToleranceFact.rawValue().toDouble();
        innerObject[terrainAdjustMaxClimbRateName]      = _terrainAdjustMaxClimbRateFact.rawValue().toDouble();
        innerObject[terrainAdjustMaxDescentRateName]    = _terrainAdjustMaxDescentRateFact.rawValue().toDouble();
        innerObject[_jsonTerrainFlightSpeed]            = _vehicleSpeed;
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

    int version = 0;
    bool v1FollowTerrain = false;
    if (innerObject.contains(JsonHelper::jsonVersionKey)) {
        version = innerObject[JsonHelper::jsonVersionKey].toInt();
    }
    if (version == 0) {
        // There are really old versions without version stamp
        version = 1;
    }
    if (version == 1) {
        // Version 1->2 differences
        //  - _jsonCameraShotsKey was optional in V1 since is was added later
        //  - _jsonTerrainFollowKeyDeprecated replaced by CameraCalc::distanceMode
        if (!innerObject.contains(_jsonCameraShotsKey)) {
            innerObject[_jsonCameraShotsKey] = 0;
        }
        v1FollowTerrain = innerObject[_jsonTerrainFollowKeyDeprecated].toBool();
        innerObject.remove(_jsonTerrainFollowKeyDeprecated);
        version = 2;
    }
    if (version != 2) {
        errorString = tr("TransectStyleComplexItem version %2 not supported").arg(version);
        return false;
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
        { _jsonCameraShotsKey,              QJsonValue::Double, true },
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
    if (!_cameraCalc.load(innerObject[_jsonCameraCalcKey].toObject(), v1FollowTerrain, errorString, forPresets)) {
        return false;
    }

    // Load TransectStyleComplexItem individual values
    _turnAroundDistanceFact.setRawValue         (innerObject[turnAroundDistanceName].toDouble());
    _cameraTriggerInTurnAroundFact.setRawValue  (innerObject[cameraTriggerInTurnAroundName].toBool());
    _hoverAndCaptureFact.setRawValue            (innerObject[hoverAndCaptureName].toBool());
    _refly90DegreesFact.setRawValue             (innerObject[refly90DegreesName].toBool());

    // These two keys where not included in initial implementation so they are optional. Without them the values will be
    // incorrect when loaded though.
    if (innerObject.contains(_jsonCameraShotsKey)) {
        _cameraShots = innerObject[_jsonCameraShotsKey].toInt();
    }

    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        QList<JsonHelper::KeyValidateInfo> followTerrainKeyInfoList = {
            { terrainAdjustToleranceName,       QJsonValue::Double, true },
            { terrainAdjustMaxClimbRateName,    QJsonValue::Double, true },
            { terrainAdjustMaxDescentRateName,  QJsonValue::Double, true },
            { _jsonTerrainFlightSpeed,          QJsonValue::Double, false },        // Not required since it was missing from initial implementation
        };
        if (!JsonHelper::validateKeys(innerObject, followTerrainKeyInfoList, errorString)) {
            return false;
        }

        _terrainAdjustToleranceFact.setRawValue         (innerObject[terrainAdjustToleranceName].toDouble());
        _terrainAdjustMaxClimbRateFact.setRawValue      (innerObject[terrainAdjustMaxClimbRateName].toDouble());
        _terrainAdjustMaxDescentRateFact.setRawValue    (innerObject[terrainAdjustMaxDescentRateName].toDouble());
        if (innerObject.contains(_jsonTerrainFlightSpeed)) {
            _vehicleSpeed = innerObject[_jsonTerrainFlightSpeed].toDouble();
        }

        if (!forPresets) {
            // We have to grovel through mission items to determine min/max alt
            _minAMSLAltitude = qQNaN();
            _maxAMSLAltitude = qQNaN();
            MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
            for (const MissionItem* missionItem: _loadedMissionItems) {
                const MissionCommandUIInfo* uiInfo = commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, missionItem->command());
                if (uiInfo && uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                    _minAMSLAltitude = std::fmin(_minAMSLAltitude, missionItem->param7());
                    _maxAMSLAltitude = std::fmax(_maxAMSLAltitude, missionItem->param7());
                }
            }
        }
    }

    if (!forPresets) {
        if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeTerrainFrame) {
            // Terrain frame requires terrain data in order to know AMSL coordinate heights for each mission item
            _queryMissionItemCoordHeights();
        } else {
            emit minAMSLAltitudeChanged();
            emit maxAMSLAltitudeChanged();
            _amslEntryAltChanged();
            _amslExitAltChanged();
            emit _updateFlightPathSegmentsSignal();
        }
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
    if (!QGC::fuzzyCompare(_vehicleSpeed, missionFlightStatus.vehicleSpeed)) {
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

    _transects.clear();
    _rgPathHeightInfo.clear();
    _rgFlightPathCoordInfo.clear();

    _rebuildTransectsPhase1();

    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: _rebuildTransects - invalid _cameraCalc.distanceMode()" << _cameraCalc.distanceMode();
        return;
    case QGroundControlQmlGlobal::AltitudeModeRelative:
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        // Not following terrain so we can build the flight path now
        _buildFlightPathCoordInfoFromTransects();
        break;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        // Query the terrain data. Once available flight path will be calculated
        _queryTransectsPathHeightInfo();
        break;
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

    emit minAMSLAltitudeChanged();
    emit maxAMSLAltitudeChanged();

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

    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: _updateFlightPathSegmentsDontCallDirectly - invalid _cameraCalc.distanceMode()" << _cameraCalc.distanceMode();
        return;
    case QGroundControlQmlGlobal::AltitudeModeRelative:
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
    {
        // Since we aren't following terrain all the transects are at the same height. We can use _visualTransectPoints to build the
        // flight path segments.
        QGeoCoordinate prevCoord;
        double surveyAlt = amslEntryAlt();
        for (const QVariant& varCoord: _visualTransectPoints) {
            QGeoCoordinate thisCoord = varCoord.value<QGeoCoordinate>();
            if (prevCoord.isValid()) {
                _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, prevCoord,  surveyAlt, thisCoord,  surveyAlt);
            }
            prevCoord = thisCoord;
        }
    }
        break;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain && _loadedMissionItems.count()) {
            // Build segments from loaded mission item data
            QGeoCoordinate prevCoord = QGeoCoordinate();
            double prevAlt = 0;
            for (const MissionItem* missionItem: _loadedMissionItems) {
                if (missionItem->command() == MAV_CMD_NAV_WAYPOINT || missionItem->command() == MAV_CMD_CONDITION_GATE) {
                    if (prevCoord.isValid()) {
                        _appendFlightPathSegment(FlightPathSegment::SegmentTypeGeneric, prevCoord, prevAlt, missionItem->coordinate(), missionItem->param7());
                    }
                    prevCoord = missionItem->coordinate();
                    prevAlt = missionItem->param7();
                }
            }
        } else {
            // We are either:
            //  - Working from live transect data of an interactive editing session
            //  - Working from loaded mission items which have had terrain heights queried for
            // In both cases _rgFlightPathCoordInfo will be set up for use
            if (_rgFlightPathCoordInfo.count()) {
                FlightPathSegment::SegmentType segmentType = _cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain ? FlightPathSegment::SegmentTypeGeneric : FlightPathSegment::SegmentTypeTerrainFrame;
                for (int i=0; i<_rgFlightPathCoordInfo.count() - 1; i++) {
                    const QGeoCoordinate& fromCoord = _rgFlightPathCoordInfo[i].coord;
                    const QGeoCoordinate& toCoord   = _rgFlightPathCoordInfo[i+1].coord;
                    _appendFlightPathSegment(segmentType, fromCoord, fromCoord.altitude(), toCoord, toCoord.altitude());
                }
            }
        }
        break;
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
    _rgPathHeightInfo.clear();
    emit readyForSaveStateChanged();

    if (_transects.count()) {
        // We don't actually send the query until this timer times out. This way we only send
        // the latest request if we get a bunch in a row.
        _terrainPolyPathQueryTimer.start();
    }
}

void TransectStyleComplexItem::_reallyQueryTransectsPathHeightInfo(void)
{
    qCDebug(TransectStyleComplexItemLog) << "_reallyQueryTransectsPathHeightInfo";

    // Clear any previous queries
    if (_currentTerrainPolyPathQuery) {
        disconnect(_currentTerrainPolyPathQuery);
        _currentTerrainPolyPathQuery = nullptr;
    }
    if (_currentTerrainAtCoordinateQuery) {
        disconnect(_currentTerrainAtCoordinateQuery);
        _currentTerrainAtCoordinateQuery = nullptr;
    }

    // Append all transects into a single PolyPath query
    QList<QGeoCoordinate> transectPoints;
    for (const QList<CoordInfo_t>& transect: _transects) {
        for (const CoordInfo_t& coordInfo: transect) {
            transectPoints.append(coordInfo.coord);
        }
    }

    if (transectPoints.count() > 1) {
        _currentTerrainPolyPathQuery = new TerrainPolyPathQuery(true /* autoDelete */);
        connect(_currentTerrainPolyPathQuery, &TerrainPolyPathQuery::terrainDataReceived, this, &TransectStyleComplexItem::_polyPathTerrainData);
        _currentTerrainPolyPathQuery->requestData(transectPoints);
    }
}

void TransectStyleComplexItem::_queryMissionItemCoordHeights(void)
{
    qCDebug(TransectStyleComplexItemLog) << "_queryMissionItemCoordHeights";

    _rgFlyThroughMissionItemCoords.clear();

    if (_currentTerrainAtCoordinateQuery) {
        qCWarning(TransectStyleComplexItemLog) << "Internal error: _queryMissionItemCoordHeights called multiple times";
        // We are already waiting on another query. We don't care about those results any more.
        disconnect(_currentTerrainPolyPathQuery);
        _currentTerrainPolyPathQuery = nullptr;
    }

    // We need terrain heights below each mission item we fly through which is terrain frame
    MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
    for (const MissionItem* missionItem: _loadedMissionItems) {
        if (missionItem->frame() == MAV_FRAME_GLOBAL_TERRAIN_ALT) {
            const MissionCommandUIInfo* uiInfo = commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, missionItem->command());
            if (uiInfo && uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                _rgFlyThroughMissionItemCoords.append(missionItem->coordinate());
            }
        }
    }

    if (_rgFlyThroughMissionItemCoords.count()) {
        _currentTerrainAtCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelete */);
        connect(_currentTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &TransectStyleComplexItem::_missionItemCoordTerrainData);
        _currentTerrainAtCoordinateQuery->requestData(_rgFlyThroughMissionItemCoords);
    }
}

void TransectStyleComplexItem::_polyPathTerrainData(bool success, const QList<TerrainPathQuery::PathHeightInfo_t>& rgPathHeightInfo)
{
    qCDebug(TransectStyleComplexItemLog) << "_polyPathTerrainData" << success;

    _rgPathHeightInfo.clear();
    emit readyForSaveStateChanged();

    if (success) {
        // Now that we have terrain data we can adjust
        _rgPathHeightInfo = rgPathHeightInfo;
        _adjustForAvailableTerrainData();
        emit readyForSaveStateChanged();
    }
    _currentTerrainPolyPathQuery = nullptr;
}

void TransectStyleComplexItem::_missionItemCoordTerrainData(bool success, QList<double> heights)
{
    qCDebug(TransectStyleComplexItemLog) << "_polyPathTerrainData" << success;

    _rgPathHeightInfo.clear();
    emit readyForSaveStateChanged();

    if (success) {
        // Now that we have terrain data we can adjust
        _rgFlyThroughMissionItemCoordsTerrainHeights = heights;
        _adjustForAvailableTerrainData();
        emit readyForSaveStateChanged();
    }
    _currentTerrainPolyPathQuery = nullptr;
}

TransectStyleComplexItem::ReadyForSaveState TransectStyleComplexItem::readyForSaveState(void) const
{
    bool terrainReady = false;
    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        if (_loadedMissionItems.count()) {
            // We have loaded mission items. Everything is ready to go.
            terrainReady = true;
        } else {
            // Survey is currently being designed. We aren't ready if we don't have terrain heights yet.
            terrainReady = _rgPathHeightInfo.count();
        }
    } else {
        // Not following terrain so always ready on terrain
        terrainReady = true;
    }
    bool polygonNotReady = !_surveyAreaPolygon.isValid();
    return (polygonNotReady || _wizardMode) ?
                NotReadyForSaveData :
                (terrainReady ? ReadyForSave : NotReadyForSaveTerrain);
}

void TransectStyleComplexItem::_adjustForAvailableTerrainData(void)
{
    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: _adjustForAvailableTerrainData - invalid _cameraCalc.distanceMode()" << _cameraCalc.distanceMode();
        return;
    case QGroundControlQmlGlobal::AltitudeModeRelative:
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        // No additional work needed
        return;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
        _buildFlightPathCoordInfoFromPathHeightInfoForCalcAboveTerrain();
        _adjustForMaxRates();
        _adjustForTolerance();
        _minAMSLAltitude = qQNaN();
        _maxAMSLAltitude = qQNaN();
        for (const CoordInfo_t& coordInfo: _rgFlightPathCoordInfo) {
            _minAMSLAltitude = std::fmin(_minAMSLAltitude, coordInfo.coord.altitude());
            _maxAMSLAltitude = std::fmax(_maxAMSLAltitude, coordInfo.coord.altitude());
        }
        emit lastSequenceNumberChanged(lastSequenceNumber());
        break;
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        if (_loadedMissionItems.count()) {
            _buildFlightPathCoordInfoFromMissionItems();
        } else {
            _buildFlightPathCoordInfoFromPathHeightInfoForTerrainFrame();
        }
        break;
    }

    _amslEntryAltChanged();
    _amslExitAltChanged();
    emit _updateFlightPathSegmentsSignal();
    emit minAMSLAltitudeChanged();
    emit maxAMSLAltitudeChanged();
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

void TransectStyleComplexItem::_adjustForMaxRates(void)
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
    if (maxClimbRate <= 0 && maxDescentRate <= 0) {
        return;
    }

    if (maxClimbRate > 0) {
        bool climbRateAdjusted;
        do {
            //qDebug() << "climb rate pass";
            climbRateAdjusted = false;
            for (int i=0; i<_rgFlightPathCoordInfo.count() - 1; i++) {
                QGeoCoordinate& fromCoord   = _rgFlightPathCoordInfo[i].coord;
                QGeoCoordinate& toCoord     = _rgFlightPathCoordInfo[i+1].coord;

                double altDifference    = toCoord.altitude() - fromCoord.altitude();
                double distance         = fromCoord.distanceTo(toCoord);
                double seconds          = distance / flightSpeed;
                double climbRate        = altDifference / seconds;

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
        bool descentRateAdjusted;
        maxDescentRate = -maxDescentRate;
        do {
            //qDebug() << "descent rate pass";
            descentRateAdjusted = false;
            for (int i=0; i<_rgFlightPathCoordInfo.count() - 1; i++) {
                QGeoCoordinate& fromCoord   = _rgFlightPathCoordInfo[i].coord;
                QGeoCoordinate& toCoord     = _rgFlightPathCoordInfo[i+1].coord;

                double altDifference    = toCoord.altitude() - fromCoord.altitude();
                double distance         = fromCoord.distanceTo(toCoord);
                double seconds          = distance / flightSpeed;
                double descentRate      = altDifference / seconds;

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

void TransectStyleComplexItem::_adjustForTolerance(void)
{
    QList<CoordInfo_t>  adjustedFlightPath;
    double              tolerance           = _terrainAdjustToleranceFact.rawValue().toDouble();
    CoordInfo_t&        lastCoordInfo       = _rgFlightPathCoordInfo.first();

    adjustedFlightPath.append(lastCoordInfo);

    int coordIndex = 1;
    while (coordIndex < _rgFlightPathCoordInfo.count()) {
        // Walk forward until we fall out of tolerence. When we fall out of tolerance add that point.
        // We always add non-interstitial points no matter what.
        const CoordInfo_t& nextCoordInfo = _rgFlightPathCoordInfo[coordIndex];
        if (nextCoordInfo.coordType != CoordTypeInteriorTerrainAdded || qAbs(lastCoordInfo.coord.altitude() - nextCoordInfo.coord.altitude()) > tolerance) {
            adjustedFlightPath.append(nextCoordInfo);
            lastCoordInfo = nextCoordInfo;
        }
        coordIndex++;
    }

    _rgFlightPathCoordInfo = adjustedFlightPath;
}

void TransectStyleComplexItem::_buildFlightPathCoordInfoFromTransects(void)
{
    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    double distanceToSurface = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    _rgFlightPathCoordInfo.clear();
    for (int transectIndex=0; transectIndex<_transects.count(); transectIndex++) {
        const QList<CoordInfo_t>& transect = _transects[transectIndex];

        for (int transectCoordIndex=0; transectCoordIndex<transect.count() - 1; transectCoordIndex++) {
            CoordInfo_t fromCoordInfo   = transect[transectCoordIndex];
            CoordInfo_t toCoordInfo     = transect[transectCoordIndex+1];

            fromCoordInfo.coord.setAltitude(distanceToSurface);
            toCoordInfo.coord.setAltitude(distanceToSurface);

            if (transectCoordIndex == 0) {
                _rgFlightPathCoordInfo.append(fromCoordInfo);
            }
            _rgFlightPathCoordInfo.append(toCoordInfo);
        }
    }
}

void TransectStyleComplexItem::_buildFlightPathCoordInfoFromPathHeightInfoForCalcAboveTerrain(void)
{
    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    if (_rgPathHeightInfo.count() == 0) {
        qCWarning(TransectStyleComplexItemLog) << "TransectStyleComplexItem::_buildFlightPathCoordInfoFromPathHeightInfo terrain height needed but _rgPathHeightInfo.count() == 0";
        return;
    }

    double distanceToSurface = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    _rgFlightPathCoordInfo.clear();
    int pathHeightIndex = 0;
    for (int transectIndex=0; transectIndex<_transects.count(); transectIndex++) {
        const QList<CoordInfo_t>& transect = _transects[transectIndex];

        // Build flight path for transect
        for (int transectCoordIndex=0; transectCoordIndex<transect.count() - 1; transectCoordIndex++) {
            CoordInfo_t fromCoordInfo   = transect[transectCoordIndex];
            CoordInfo_t toCoordInfo     = transect[transectCoordIndex+1];

            const auto& pathHeightInfo = _rgPathHeightInfo[pathHeightIndex++];

            fromCoordInfo.coord.setAltitude(distanceToSurface + pathHeightInfo.heights.first());
            toCoordInfo.coord.setAltitude(distanceToSurface + pathHeightInfo.heights.last());

            if (transectCoordIndex == 0) {
                _rgFlightPathCoordInfo.append(fromCoordInfo);
            }

            // Add interstitial points at max resolution of our terrain data
            int cHeights = pathHeightInfo.heights.count();

            double azimuth  = fromCoordInfo.coord.azimuthTo(toCoordInfo.coord);
            double distance = fromCoordInfo.coord.distanceTo(toCoordInfo.coord);

            for (int pathHeightIndex=1; pathHeightIndex<cHeights - 1; pathHeightIndex++) {
                double interstitialTerrainHeight = pathHeightInfo.heights[pathHeightIndex];
                double percentTowardsTo = (1.0 / (cHeights - 1)) * pathHeightIndex;

                CoordInfo_t interstitialCoordInfo;
                interstitialCoordInfo.coordType = CoordTypeInteriorTerrainAdded;
                interstitialCoordInfo.coord     = fromCoordInfo.coord.atDistanceAndAzimuth(distance * percentTowardsTo, azimuth);
                interstitialCoordInfo.coord.setAltitude(interstitialTerrainHeight + distanceToSurface);

                _rgFlightPathCoordInfo.append(interstitialCoordInfo);
            }

            _rgFlightPathCoordInfo.append(toCoordInfo);
        }

        // Build flight path for turnaround
        // Add terrain interstitial points to the turn segment if not the last transect
        if (transectIndex != _transects.count() - 1) {
            const auto& pathHeightInfo = _rgPathHeightInfo[pathHeightIndex++];

            int cHeights = pathHeightInfo.heights.count();

            CoordInfo_t fromCoordInfo   = _transects[transectIndex].last();
            CoordInfo_t toCoordInfo     = _transects[transectIndex+1].first();

            double azimuth  = fromCoordInfo.coord.azimuthTo(toCoordInfo.coord);
            double distance = fromCoordInfo.coord.distanceTo(toCoordInfo.coord);

            for (int pathHeightIndex=1; pathHeightIndex<cHeights - 1; pathHeightIndex++) {
                double interstitialTerrainHeight = pathHeightInfo.heights[pathHeightIndex];
                double percentTowardsTo = (1.0 / (cHeights - 1)) * pathHeightIndex;

                CoordInfo_t interstitialCoordInfo;
                interstitialCoordInfo.coordType = CoordTypeInteriorTerrainAdded;
                interstitialCoordInfo.coord     = fromCoordInfo.coord.atDistanceAndAzimuth(distance * percentTowardsTo, azimuth);
                interstitialCoordInfo.coord.setAltitude(interstitialTerrainHeight + distanceToSurface);

                _rgFlightPathCoordInfo.append(interstitialCoordInfo);
            }
        }
    }
}

void TransectStyleComplexItem::_buildFlightPathCoordInfoFromPathHeightInfoForTerrainFrame(void)
{
    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    if (_rgPathHeightInfo.count() == 0) {
        qCWarning(TransectStyleComplexItemLog) << "TransectStyleComplexItem::_buildFlightPathCoordInfoFromPathHeightInfoForTerrainFrame terrain height needed but _rgPathHeightInfo.count() == 0";
        return;
    }

    double distanceToSurface = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    _rgFlightPathCoordInfo.clear();
    int pathHeightIndex = 0;
    for (int transectIndex=0; transectIndex<_transects.count(); transectIndex++) {
        const QList<CoordInfo_t>& transect = _transects[transectIndex];

        // Build flight path for transect
        for (int transectCoordIndex=0; transectCoordIndex<transect.count() - 1; transectCoordIndex++) {
            CoordInfo_t fromCoordInfo   = transect[transectCoordIndex];
            CoordInfo_t toCoordInfo     = transect[transectCoordIndex+1];

            const auto& pathHeightInfo = _rgPathHeightInfo[pathHeightIndex++];

            fromCoordInfo.coord.setAltitude(distanceToSurface + pathHeightInfo.heights.first());
            toCoordInfo.coord.setAltitude(distanceToSurface + pathHeightInfo.heights.last());

            if (transectCoordIndex == 0) {
                _rgFlightPathCoordInfo.append(fromCoordInfo);
            }
            _rgFlightPathCoordInfo.append(toCoordInfo);
        }

        // Even though we don't use it we still have path heights for the turnaround segment
        pathHeightIndex++;
    }
}

void TransectStyleComplexItem::_buildFlightPathCoordInfoFromMissionItems(void)
{
    _minAMSLAltitude = _maxAMSLAltitude = qQNaN();

    if (_rgFlyThroughMissionItemCoordsTerrainHeights.count() == 0) {
        qCWarning(TransectStyleComplexItemLog) << "_buildFlightPathCoordInfoFromMissionItems - terrain height needed but _rgFlyThroughMissionItemCoordsTerrainHeights.count() == 0";
        return;
    }
    if (_rgFlyThroughMissionItemCoordsTerrainHeights.count() != _rgFlyThroughMissionItemCoords.count()) {
        qCWarning(TransectStyleComplexItemLog) << "_buildFlightPathCoordInfoFromMissionItems - _rgFlyThroughMissionItemCoordsTerrainHeights.count() != _rgFlyThroughMissionItemCoords.count()";
        return;
    }
    if (_rgFlyThroughMissionItemCoords.count() < 2) {
        qCWarning(TransectStyleComplexItemLog) << "_buildFlightPathCoordInfoFromMissionItems - rgFlyThroughMissionItemCoords.count() < 2";
        return;
    }

    _rgFlightPathCoordInfo.clear();
    for (int i=0; i<_rgFlyThroughMissionItemCoords.count() - 1; i++) {
        double heightAtCoord = _rgFlyThroughMissionItemCoordsTerrainHeights[i];

        // Since we are working from loading mission items the CoordInfo_t.coordType doesn't really matter
        CoordInfo_t fromCoordInfo   = { _rgFlyThroughMissionItemCoords[i],      CoordTypeInterior };
        CoordInfo_t toCoordInfo     = { _rgFlyThroughMissionItemCoords[i+1],    CoordTypeInterior };

        fromCoordInfo.coord.setAltitude(fromCoordInfo.coord.altitude() + heightAtCoord);
        toCoordInfo.coord.setAltitude(toCoordInfo.coord.altitude() + heightAtCoord);

        _rgFlightPathCoordInfo.append(fromCoordInfo);
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
    } else if (_rgFlightPathCoordInfo.count()) {
        int                         itemCount   = 0;
        BuildMissionItemsState_t    buildState  = _buildMissionItemsState();

        // Important Note: This code should match the logic in _buildAndAppendMissionItems
        for (int coordIndex=0; coordIndex<_rgFlightPathCoordInfo.count(); coordIndex++) {
            const CoordInfo_t& coordInfo = _rgFlightPathCoordInfo[coordIndex];
            switch (coordInfo.coordType) {
            case CoordTypeInterior:
            case CoordTypeInteriorTerrainAdded:
                itemCount++; // Waypoint only
                break;
            case CoordTypeTurnaround:
            {
                bool firstEntryTurnaround   = coordIndex == 0;
                bool lastExitTurnaround     = coordIndex == _rgFlightPathCoordInfo.count() - 1;
                if (buildState.addTriggerAtFirstAndLastPoint && (firstEntryTurnaround || lastExitTurnaround)) {
                    itemCount += 2; // Waypoint + camera trigger
                } else {
                    itemCount++; // Waypoint only
                }
            }
                break;
            case CoordTypeInteriorHoverTrigger:
                itemCount += 2; // Waypoint + camera trigger
                break;
            case CoordTypeSurveyEntry:
                if (triggerCamera()) {
                    itemCount += 2; // Waypoint + camera trigger
                } else {
                    itemCount++; // Waypoint only
                }
                break;
            case CoordTypeSurveyExit:
                bool lastSurveyExit = coordIndex == _rgFlightPathCoordInfo.count() - 1;
                if (triggerCamera()) {
                    if (hoverAndCaptureEnabled()) {
                        itemCount += 2; // Waypoint + camera trigger
                    } else if (buildState.addTriggerAtFirstAndLastPoint && !buildState.hasTurnarounds && lastSurveyExit) {
                        itemCount += 2; // Waypoint + camera trigger
                    } else if (buildState.imagesInTurnaround) {
                        itemCount++; // Waypoint only
                    } else {
                        itemCount += 2; // Waypoint + camera trigger
                    }
                } else {
                    itemCount++; // Waypoint only
                }
                break;
            }
        }

        return _sequenceNumber + itemCount - 1;
    } else {
        // We can end up hear if we are follow terrain and the flight path isn't ready yet. So we just return an inaccurate number until
        // we know the real one.
        int itemCount = 0;

        for (const QList<CoordInfo_t>& rgCoordInfo: _transects) {
            itemCount += rgCoordInfo.count();
        }

        return _sequenceNumber + itemCount - 1;
    }
}

void TransectStyleComplexItem::_distanceModeChanged(int distanceMode)
{
    if (static_cast<QGroundControlQmlGlobal::AltMode>(distanceMode) == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
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
    double altitude = _cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain ? coordinate.altitude() : _cameraCalc.distanceToSurface()->rawValue().toDouble();

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        mavFrame,
                                        holdTime,
                                        0.0,                                         // No acceptance radius specified
                                        0.0,                                         // Pass through waypoint
                                        std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        altitude,
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
    double altitude = _cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain ? coordinate.altitude() : _cameraCalc.distanceToSurface()->rawValue().toDouble();

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_CONDITION_GATE,
                                        mavFrame,
                                        0,                                           // Gate is orthogonal to path
                                        1,                                           // Use altitude
                                        0, 0,                                        // Param 3-4 ignored
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        altitude,
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

TransectStyleComplexItem::BuildMissionItemsState_t TransectStyleComplexItem::_buildMissionItemsState(void) const
{
    BuildMissionItemsState_t state;

    state.imagesInTurnaround        = _cameraTriggerInTurnAroundFact.rawValue().toBool();
    state.hasTurnarounds            = _turnAroundDistance() != 0;
    state.addTriggerAtFirstAndLastPoint  = !hoverAndCaptureEnabled() && state.imagesInTurnaround && triggerCamera();
    state.useConditionGate          = _controllerVehicle->firmwarePlugin()->supportedMissionCommands(QGCMAVLink::VehicleClassGeneric).contains(MAV_CMD_CONDITION_GATE) &&
            triggerCamera() &&
            !hoverAndCaptureEnabled();

    return state;
}

void TransectStyleComplexItem::_buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int                         seqNum      = _sequenceNumber;
    BuildMissionItemsState_t    buildState  = _buildMissionItemsState();
    MAV_FRAME                   mavFrame;

    qCDebug(TransectStyleComplexItemLog) << "_buildAndAppendMissionItems";

    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeRelative:
        mavFrame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
        break;
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
        mavFrame = MAV_FRAME_GLOBAL;
        break;
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        mavFrame = MAV_FRAME_GLOBAL_TERRAIN_ALT;
        break;
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: _buildAndAppendMissionItems incorrect _cameraCalc.distanceMode" << _cameraCalc.distanceMode();
        mavFrame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
        break;
    }

    // Note: The code below is written to be understable as opposed to being compact and/or remove all duplicate code
    for (int coordIndex=0; coordIndex<_rgFlightPathCoordInfo.count(); coordIndex++) {
        const CoordInfo_t& coordInfo = _rgFlightPathCoordInfo[coordIndex];
        switch (coordInfo.coordType) {
        case CoordTypeInterior:
        case CoordTypeInteriorTerrainAdded:
            _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordInfo.coord);
            break;
        case CoordTypeTurnaround:
        {
            bool firstEntryTurnaround   = coordIndex == 0;
            bool lastExitTurnaround     = coordIndex == _rgFlightPathCoordInfo.count() - 1;
            if (buildState.addTriggerAtFirstAndLastPoint && (firstEntryTurnaround || lastExitTurnaround)) {
                _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, coordInfo.coord, buildState.useConditionGate, firstEntryTurnaround ? triggerDistance() : 0);
            } else {
                _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordInfo.coord);
            }
        }
            break;
        case CoordTypeInteriorHoverTrigger:
            _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, coordInfo.coord);
            _appendSinglePhotoCapture(items, missionItemParent, seqNum);
            break;
        case CoordTypeSurveyEntry:
            if (triggerCamera()) {
                if (hoverAndCaptureEnabled()) {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, coordInfo.coord);
                    _appendSinglePhotoCapture(items, missionItemParent, seqNum);
                } else {
                    // We always add a trigger start to survey entry. Even for imagesInTurnaround = true. This allows you to resume a mission and refly a transect
                    _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, coordInfo.coord, buildState.useConditionGate, triggerDistance());
                }
            } else {
                _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordInfo.coord);
            }
            break;
        case CoordTypeSurveyExit:
            bool lastSurveyExit = coordIndex == _rgFlightPathCoordInfo.count() - 1;
            if (triggerCamera()) {
                if (hoverAndCaptureEnabled()) {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, _hoverAndCaptureDelaySeconds, coordInfo.coord);
                    _appendSinglePhotoCapture(items, missionItemParent, seqNum);
                } else if (buildState.addTriggerAtFirstAndLastPoint && !buildState.hasTurnarounds && lastSurveyExit) {
                    _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, coordInfo.coord, buildState.useConditionGate, 0 /* triggerDistance */);
                } else if (buildState.imagesInTurnaround) {
                    _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordInfo.coord);
                } else {
                    // If we get this far it means the camera is triggering start/stop for each transect
                    _appendCameraTriggerDistanceUpdatePoint(items, missionItemParent, seqNum, mavFrame, coordInfo.coord, buildState.useConditionGate, 0 /* triggerDistance */);
                }
            } else {
                _appendWaypoint(items, missionItemParent, seqNum, mavFrame, 0 /* holdTime */, coordInfo.coord);
            }
            break;
        }
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
    double alt                  = qQNaN();
    double distanceToSurface    = _cameraCalc.distanceToSurface()->rawValue().toDouble();

    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeRelative:
        alt = distanceToSurface + _missionController->plannedHomePosition().altitude();
        break;
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        alt = distanceToSurface;
        break;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        if (_loadedMissionItems.count()) {
            // The first item might not be a waypoint we have to find it.
            MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
            for (int i=0; i<_loadedMissionItems.count(); i++) {
                MissionItem* item = _loadedMissionItems[i];
                const MissionCommandUIInfo* uiInfo = commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, item->command());
                if (uiInfo && uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
                        // AltitudeModeCalcAboveTerrain has AMSL alt in param 7
                        alt = item->param7();
                    } else {
                        // AltitudeModeTerrainFrame has terrain frame relative alt in param 7. So we need terrain heights to calc AMSL.
                        if (_rgPathHeightInfo.count()) {
                            alt = item->param7() + _rgPathHeightInfo.first().heights.first();
                        }
                    }
                    break;
                }
            }
        } else {
            if (_rgFlightPathCoordInfo.count() != 0) {
                alt = _rgFlightPathCoordInfo.first().coord.altitude();
            }
        }
        break;
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: amslEntryAlt incorrect _cameraCalc.distanceMode" << _cameraCalc.distanceMode();
        break;
    }

    return alt;
}

double TransectStyleComplexItem::amslExitAlt(void) const
{
    double alt                  = qQNaN();

    switch (_cameraCalc.distanceMode()) {
    case QGroundControlQmlGlobal::AltitudeModeRelative:
    case QGroundControlQmlGlobal::AltitudeModeAbsolute:
        alt = amslEntryAlt();
        break;
    case QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain:
    case QGroundControlQmlGlobal::AltitudeModeTerrainFrame:
        if (_loadedMissionItems.count()) {
            // The last item might not be a waypoint we have to find it.
            MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();
            for (int i=_loadedMissionItems.count()-1; i>0; i--) {
                MissionItem* item = _loadedMissionItems[i];
                const MissionCommandUIInfo* uiInfo = commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, item->command());
                if (uiInfo && uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
                        // AltitudeModeCalcAboveTerrain has AMSL alt in param 7
                        alt = item->param7();
                    } else {
                        // AltitudeModeTerrainFrame has terrain frame relative alt in param 7. So we need terrain heights to calc AMSL.
                        if (_rgPathHeightInfo.count()) {
                            alt = item->param7() + _rgPathHeightInfo.last().heights.last();
                        }
                    }
                    break;
                }
            }
        } else {
            if (_rgFlightPathCoordInfo.count() != 0) {
                alt = _rgFlightPathCoordInfo.last().coord.altitude();
            }
        }
        break;
    case QGroundControlQmlGlobal::AltitudeModeMixed:
    case QGroundControlQmlGlobal::AltitudeModeNone:
        qCWarning(TransectStyleComplexItemLog) << "Internal Error: amslExitAlt incorrect _cameraCalc.distanceMode" << _cameraCalc.distanceMode();
        break;
    }

    return alt;
}

void TransectStyleComplexItem::applyNewAltitude(double newAltitude)
{
    _cameraCalc.valueSetIsDistance()->setRawValue(true);
    _cameraCalc.distanceToSurface()->setRawValue(newAltitude);
}

double TransectStyleComplexItem::minAMSLAltitude(void) const
{
    // FIXME: What about terrain frame

    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        return _minAMSLAltitude;
    } else {
        return _cameraCalc.distanceToSurface()->rawValue().toDouble() + (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeRelative ? _missionController->plannedHomePosition().altitude() : 0);
    }
}

double TransectStyleComplexItem::maxAMSLAltitude(void) const
{
    // FIXME: What about terrain frame

    if (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain) {
        return _maxAMSLAltitude;
    } else {
        return _cameraCalc.distanceToSurface()->rawValue().toDouble() + (_cameraCalc.distanceMode() == QGroundControlQmlGlobal::AltitudeModeRelative ? _missionController->plannedHomePosition().altitude() : 0);
    }
}
