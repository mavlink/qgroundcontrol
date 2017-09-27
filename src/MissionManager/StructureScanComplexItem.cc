/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StructureScanComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(StructureScanComplexItemLog, "StructureScanComplexItemLog")

const char* StructureScanComplexItem::jsonComplexItemTypeValue =           "StructureScan-WIP";

const char* StructureScanComplexItem::_jsonGridObjectKey =                 "grid";
const char* StructureScanComplexItem::_jsonGridAltitudeKey =               "altitude";
const char* StructureScanComplexItem::_jsonGridAltitudeRelativeKey =       "relativeAltitude";
const char* StructureScanComplexItem::_jsonGridAngleKey =                  "angle";
const char* StructureScanComplexItem::_jsonGridSpacingKey =                "spacing";
const char* StructureScanComplexItem::_jsonGridEntryLocationKey =          "entryLocation";
const char* StructureScanComplexItem::_jsonTurnaroundDistKey =             "turnAroundDistance";
const char* StructureScanComplexItem::_jsonCameraTriggerDistanceKey =      "cameraTriggerDistance";
const char* StructureScanComplexItem::_jsonCameraTriggerInTurnaroundKey =  "cameraTriggerInTurnaround";
const char* StructureScanComplexItem::_jsonHoverAndCaptureKey =            "hoverAndCapture";
const char* StructureScanComplexItem::_jsonGroundResolutionKey =           "groundResolution";
const char* StructureScanComplexItem::_jsonFrontalOverlapKey =             "imageFrontalOverlap";
const char* StructureScanComplexItem::_jsonSideOverlapKey =                "imageSideOverlap";
const char* StructureScanComplexItem::_jsonCameraSensorWidthKey =          "sensorWidth";
const char* StructureScanComplexItem::_jsonCameraSensorHeightKey =         "sensorHeight";
const char* StructureScanComplexItem::_jsonCameraResolutionWidthKey =      "resolutionWidth";
const char* StructureScanComplexItem::_jsonCameraResolutionHeightKey =     "resolutionHeight";
const char* StructureScanComplexItem::_jsonCameraFocalLengthKey =          "focalLength";
const char* StructureScanComplexItem::_jsonCameraMinTriggerIntervalKey =   "minTriggerInterval";
const char* StructureScanComplexItem::_jsonCameraObjectKey =               "camera";
const char* StructureScanComplexItem::_jsonCameraNameKey =                 "name";
const char* StructureScanComplexItem::_jsonManualGridKey =                 "manualGrid";
const char* StructureScanComplexItem::_jsonCameraOrientationLandscapeKey = "orientationLandscape";
const char* StructureScanComplexItem::_jsonFixedValueIsAltitudeKey =       "fixedValueIsAltitude";
const char* StructureScanComplexItem::_jsonRefly90DegreesKey =             "refly90Degrees";

const char* StructureScanComplexItem::_altitudeFactName =               "Altitude";
const char* StructureScanComplexItem::_layersFactName =                 "Layers";
const char* StructureScanComplexItem::_layerDistanceFactName =          "Layer distance";
const char* StructureScanComplexItem::_cameraTriggerDistanceFactName =  "Trigger distance";

QMap<QString, FactMetaData*> StructureScanComplexItem::_metaDataMap;

StructureScanComplexItem::StructureScanComplexItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem        (vehicle, parent)
    , _sequenceNumber           (0)
    , _dirty                    (false)
    , _altitudeRelative         (true)
    , _entryVertex              (0)
    , _ignoreRecalc             (false)
    , _scanDistance             (0.0)
    , _cameraShots              (0)
    , _cameraMinTriggerInterval (0)
    , _altitudeFact             (0, _altitudeFactName,              FactMetaData::valueTypeDouble)
    , _layersFact               (0, _layersFactName,                FactMetaData::valueTypeUint32)
    , _layerDistanceFact        (0, _layerDistanceFactName,         FactMetaData::valueTypeDouble)
    , _cameraTriggerDistanceFact(0, _cameraTriggerDistanceFactName, FactMetaData::valueTypeDouble)
{
    _editorQml = "qrc:/qml/StructureScanEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/StructureScan.SettingsGroup.json"), this);
    }

    _altitudeFact.setMetaData               (_metaDataMap[_altitudeFactName]);
    _layersFact.setMetaData                 (_metaDataMap[_layersFactName]);
    _layerDistanceFact.setMetaData          (_metaDataMap[_layerDistanceFactName]);
    _cameraTriggerDistanceFact.setMetaData  (_metaDataMap[_cameraTriggerDistanceFactName]);

    _altitudeFact.setRawValue               (_altitudeFact.rawDefaultValue());
    _layersFact.setRawValue                 (_layersFact.rawDefaultValue());
    _layerDistanceFact.setRawValue          (_layerDistanceFact.rawDefaultValue());
    _cameraTriggerDistanceFact.setRawValue  (_cameraTriggerDistanceFact.rawDefaultValue());

    _altitudeFact.setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());

    connect(&_altitudeFact,                 &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_layersFact,                   &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_layerDistanceFact,            &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_cameraTriggerDistanceFact,    &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);

    connect(this, &StructureScanComplexItem::altitudeRelativeChanged, this, &StructureScanComplexItem::_setDirty);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged, this, &StructureScanComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged, this, &StructureScanComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(&_altitudeFact, &Fact::valueChanged, this, &StructureScanComplexItem::_updateCoordinateAltitudes);

    connect(&_mapPolygon, &QGCMapPolygon::dirtyChanged, this, &StructureScanComplexItem::_polygonDirtyChanged);
    connect(&_mapPolygon, &QGCMapPolygon::countChanged, this, &StructureScanComplexItem::_polygonCountChanged);
    connect(&_mapPolygon, &QGCMapPolygon::pathChanged,  this, &StructureScanComplexItem::_polygonPathChanged);
}

void StructureScanComplexItem::_setScanDistance(double scanDistance)
{
    if (!qFuzzyCompare(_scanDistance, scanDistance)) {
        _scanDistance = scanDistance;
        emit complexDistanceChanged(_scanDistance);
    }
}

void StructureScanComplexItem::_setCameraShots(int cameraShots)
{
    if (_cameraShots != cameraShots) {
        _cameraShots = cameraShots;
        emit cameraShotsChanged(this->cameraShots());
    }
}

void StructureScanComplexItem::_clearInternal(void)
{
    setDirty(true);

    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void StructureScanComplexItem::_polygonCountChanged(int count)
{
    Q_UNUSED(count);
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

int StructureScanComplexItem::lastSequenceNumber(void) const
{
    return _sequenceNumber +
            ((_mapPolygon.count() + 1) * _layersFact.rawValue().toInt()) + // 1 waypoint for each polygon vertex + 1 to go back to first polygon vertex
            1;  // Gimbal yaw command
}

void StructureScanComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void StructureScanComplexItem::save(QJsonArray&  missionItems)
{
    Q_UNUSED(missionItems);
#if 0
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    3;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_jsonManualGridKey] =                            _manualGridFact.rawValue().toBool();
    saveObject[_jsonFixedValueIsAltitudeKey] =                  _fixedValueIsAltitudeFact.rawValue().toBool();
    saveObject[_jsonHoverAndCaptureKey] =                       _hoverAndCaptureFact.rawValue().toBool();
    saveObject[_jsonRefly90DegreesKey] =                        _refly90Degrees;
    saveObject[_jsonCameraTriggerDistanceKey] =                 _cameraTriggerDistanceFact.rawValue().toDouble();

    QJsonObject gridObject;
    gridObject[_jsonGridAltitudeKey] =          _gridAltitudeFact.rawValue().toDouble();
    gridObject[_jsonGridAltitudeRelativeKey] =  _gridAltitudeRelativeFact.rawValue().toBool();
    gridObject[_jsonGridAngleKey] =             _gridAngleFact.rawValue().toDouble();
    gridObject[_jsonGridSpacingKey] =           _gridSpacingFact.rawValue().toDouble();
    gridObject[_jsonGridEntryLocationKey] =     _gridEntryLocationFact.rawValue().toDouble();
    gridObject[_jsonTurnaroundDistKey] =        _turnaroundDistFact.rawValue().toDouble();

    saveObject[_jsonGridObjectKey] = gridObject;

    if (!_manualGridFact.rawValue().toBool()) {
        QJsonObject cameraObject;
        cameraObject[_jsonCameraNameKey] =                  _cameraFact.rawValue().toString();
        cameraObject[_jsonCameraOrientationLandscapeKey] =  _cameraOrientationLandscapeFact.rawValue().toBool();
        cameraObject[_jsonCameraSensorWidthKey] =           _cameraSensorWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraSensorHeightKey] =          _cameraSensorHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionWidthKey] =       _cameraResolutionWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionHeightKey] =      _cameraResolutionHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraFocalLengthKey] =           _cameraFocalLengthFact.rawValue().toDouble();
        cameraObject[_jsonCameraMinTriggerIntervalKey] =    _cameraMinTriggerInterval;
        cameraObject[_jsonGroundResolutionKey] =            _groundResolutionFact.rawValue().toDouble();
        cameraObject[_jsonFrontalOverlapKey] =              _frontalOverlapFact.rawValue().toInt();
        cameraObject[_jsonSideOverlapKey] =                 _sideOverlapFact.rawValue().toInt();

        saveObject[_jsonCameraObjectKey] = cameraObject;
    }

    // Polygon shape
    _mapPolygon.saveToJson(saveObject);

    missionItems.append(saveObject);
#endif
}

void StructureScanComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool StructureScanComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
#if 0
    QJsonObject v2Object = complexObject;

    // We need to pull version first to determine what validation/conversion needs to be performed.
    QList<JsonHelper::KeyValidateInfo> versionKeyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(v2Object, versionKeyInfoList, errorString)) {
        return false;
    }

    int version = v2Object[JsonHelper::jsonVersionKey].toInt();
    if (version != 2 && version != 3) {
        errorString = tr("%1 does not support this version of survey items").arg(qgcApp()->applicationName());
        return false;
    }
    if (version == 2) {
        // Convert to v3
        if (v2Object.contains(VisualMissionItem::jsonTypeKey) && v2Object[VisualMissionItem::jsonTypeKey].toString() == QStringLiteral("survey")) {
            v2Object[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
            v2Object[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
        }
    }

    QList<JsonHelper::KeyValidateInfo> mainKeyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
        { _jsonGridObjectKey,                           QJsonValue::Object, true },
        { _jsonCameraObjectKey,                         QJsonValue::Object, false },
        { _jsonCameraTriggerDistanceKey,                QJsonValue::Double, true },
        { _jsonManualGridKey,                           QJsonValue::Bool,   true },
        { _jsonFixedValueIsAltitudeKey,                 QJsonValue::Bool,   true },
        { _jsonHoverAndCaptureKey,                      QJsonValue::Bool,   false },
        { _jsonRefly90DegreesKey,                       QJsonValue::Bool,   false },
    };
    if (!JsonHelper::validateKeys(v2Object, mainKeyInfoList, errorString)) {
        return false;
    }

    QString itemType = v2Object[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = v2Object[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _ignoreRecalc = true;

    _mapPolygon.clear();

    setSequenceNumber(sequenceNumber);

    _manualGridFact.setRawValue             (v2Object[_jsonManualGridKey].toBool(true));
    _fixedValueIsAltitudeFact.setRawValue   (v2Object[_jsonFixedValueIsAltitudeKey].toBool(true));
    _gridAltitudeRelativeFact.setRawValue   (v2Object[_jsonGridAltitudeRelativeKey].toBool(true));
    _hoverAndCaptureFact.setRawValue        (v2Object[_jsonHoverAndCaptureKey].toBool(false));

    _refly90Degrees = v2Object[_jsonRefly90DegreesKey].toBool(false);

    QList<JsonHelper::KeyValidateInfo> gridKeyInfoList = {
        { _jsonGridAltitudeKey,                 QJsonValue::Double, true },
        { _jsonGridAltitudeRelativeKey,         QJsonValue::Bool,   true },
        { _jsonGridAngleKey,                    QJsonValue::Double, true },
        { _jsonGridSpacingKey,                  QJsonValue::Double, true },
        { _jsonGridEntryLocationKey,            QJsonValue::Double, false },
        { _jsonTurnaroundDistKey,               QJsonValue::Double, true },
    };
    QJsonObject gridObject = v2Object[_jsonGridObjectKey].toObject();
    if (!JsonHelper::validateKeys(gridObject, gridKeyInfoList, errorString)) {
        return false;
    }
    _gridAltitudeFact.setRawValue           (gridObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue              (gridObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue            (gridObject[_jsonGridSpacingKey].toDouble());
    _turnaroundDistFact.setRawValue         (gridObject[_jsonTurnaroundDistKey].toDouble());
    _cameraTriggerDistanceFact.setRawValue  (v2Object[_jsonCameraTriggerDistanceKey].toDouble());
    if (gridObject.contains(_jsonGridEntryLocationKey)) {
        _gridEntryLocationFact.setRawValue(gridObject[_jsonGridEntryLocationKey].toDouble());
    } else {
        _gridEntryLocationFact.setRawValue(_gridEntryLocationFact.rawDefaultValue());
    }

    if (!_manualGridFact.rawValue().toBool()) {
        if (!v2Object.contains(_jsonCameraObjectKey)) {
            errorString = tr("%1 but %2 object is missing").arg("manualGrid = false").arg("camera");
            return false;
        }

        QJsonObject cameraObject = v2Object[_jsonCameraObjectKey].toObject();

        // Older code had typo on "imageSideOverlap" incorrectly being "imageSizeOverlap"
        QString incorrectImageSideOverlap = "imageSizeOverlap";
        if (cameraObject.contains(incorrectImageSideOverlap)) {
            cameraObject[_jsonSideOverlapKey] = cameraObject[incorrectImageSideOverlap];
            cameraObject.remove(incorrectImageSideOverlap);
        }

        QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
            { _jsonGroundResolutionKey,             QJsonValue::Double, true },
            { _jsonFrontalOverlapKey,               QJsonValue::Double, true },
            { _jsonSideOverlapKey,                  QJsonValue::Double, true },
            { _jsonCameraSensorWidthKey,            QJsonValue::Double, true },
            { _jsonCameraSensorHeightKey,           QJsonValue::Double, true },
            { _jsonCameraResolutionWidthKey,        QJsonValue::Double, true },
            { _jsonCameraResolutionHeightKey,       QJsonValue::Double, true },
            { _jsonCameraFocalLengthKey,            QJsonValue::Double, true },
            { _jsonCameraNameKey,                   QJsonValue::String, true },
            { _jsonCameraOrientationLandscapeKey,   QJsonValue::Bool,   true },
            { _jsonCameraMinTriggerIntervalKey,     QJsonValue::Double, false },
        };
        if (!JsonHelper::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            return false;
        }

        _cameraFact.setRawValue(cameraObject[_jsonCameraNameKey].toString());
        _cameraOrientationLandscapeFact.setRawValue(cameraObject[_jsonCameraOrientationLandscapeKey].toBool(true));

        _groundResolutionFact.setRawValue       (cameraObject[_jsonGroundResolutionKey].toDouble());
        _frontalOverlapFact.setRawValue         (cameraObject[_jsonFrontalOverlapKey].toInt());
        _sideOverlapFact.setRawValue            (cameraObject[_jsonSideOverlapKey].toInt());
        _cameraSensorWidthFact.setRawValue      (cameraObject[_jsonCameraSensorWidthKey].toDouble());
        _cameraSensorHeightFact.setRawValue     (cameraObject[_jsonCameraSensorHeightKey].toDouble());
        _cameraResolutionWidthFact.setRawValue  (cameraObject[_jsonCameraResolutionWidthKey].toDouble());
        _cameraResolutionHeightFact.setRawValue (cameraObject[_jsonCameraResolutionHeightKey].toDouble());
        _cameraFocalLengthFact.setRawValue      (cameraObject[_jsonCameraFocalLengthKey].toDouble());
        _cameraMinTriggerInterval =             cameraObject[_jsonCameraMinTriggerIntervalKey].toDouble(0);
    }

    // Polygon shape
    /// Load a polygon from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    if (!_mapPolygon.loadFromJson(v2Object, true /* required */, errorString)) {
        _mapPolygon.clear();
        return false;
    }

    _ignoreRecalc = false;
    _generateGrid();

    return true;
#else
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);
    Q_UNUSED(errorString);

    return false;
#endif
}

void StructureScanComplexItem::_polygonPathChanged(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
    emit greatestDistanceToChanged();
}

double StructureScanComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    QList<QGeoCoordinate> vertices = _mapPolygon.coordinateList();

    for (int i=0; i<vertices.count(); i++) {
        QGeoCoordinate vertex = vertices[i];
        double distance = vertex.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }

    return greatestDistance;
}

bool StructureScanComplexItem::specifiesCoordinate(void) const
{
    return _mapPolygon.count() > 2;
}

void StructureScanComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;
    double baseAltitude = _altitudeFact.rawValue().toDouble();

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_DO_MOUNT_CONTROL,
                                        MAV_FRAME_MISSION,
                                        0,                                  // Gimbal pitch
                                        0,                                  // Gimbal roll
                                        90,                                 // Gimbal yaw
                                        0, 0, 0,                            // param 4-6 not used
                                        MAV_MOUNT_MODE_MAVLINK_TARGETING,
                                        true,                               // autoContinue
                                        false,                              // isCurrentItem
                                        missionItemParent);
    items.append(item);

    for (int layer=0; layer<_layersFact.rawValue().toInt(); layer++) {
        double layerAltitude = baseAltitude + (layer * _layerDistanceFact.rawValue().toDouble());

        for (int i=0; i<_mapPolygon.count(); i++) {
            QGeoCoordinate vertexCoord = _mapPolygon.pathModel().value<QGCQGeoCoordinate*>(i)->coordinate();

            MissionItem* item = new MissionItem(seqNum++,
                                                MAV_CMD_NAV_WAYPOINT,
                                                _altitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                                                0,                                          // No hold time
                                                0.0,                                        // No acceptance radius specified
                                                0.0,                                        // Pass through waypoint
                                                90, //std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                                                vertexCoord.latitude(),
                                                vertexCoord.longitude(),
                                                layerAltitude,
                                                true,                                       // autoContinue
                                                false,                                      // isCurrentItem
                                                missionItemParent);
            items.append(item);
        }

        QGeoCoordinate vertexCoord = _mapPolygon.pathModel().value<QGCQGeoCoordinate*>(0)->coordinate();

        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_NAV_WAYPOINT,
                                            _altitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                                            0,                                          // No hold time
                                            0.0,                                        // No acceptance radius specified
                                            0.0,                                        // Pass through waypoint
                                            std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                                            vertexCoord.latitude(),
                                            vertexCoord.longitude(),
                                            layerAltitude,
                                            true,                                       // autoContinue
                                            false,                                      // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }
}

int StructureScanComplexItem::cameraShots(void) const
{
    return true /*_triggerCamera()*/ ? _cameraShots : 0;
}

void StructureScanComplexItem::setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    ComplexMissionItem::setMissionFlightStatus(missionFlightStatus);
    if (!qFuzzyCompare(_cruiseSpeed, missionFlightStatus.vehicleSpeed)) {
        _cruiseSpeed = missionFlightStatus.vehicleSpeed;
        emit timeBetweenShotsChanged();
    }
}

void StructureScanComplexItem::_setDirty(void)
{
    setDirty(true);
}

void StructureScanComplexItem::applyNewAltitude(double newAltitude)
{
    _altitudeFact.setRawValue(newAltitude);
}

void StructureScanComplexItem::_polygonDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

double StructureScanComplexItem::timeBetweenShots(void) const
{
    return _cruiseSpeed == 0 ? 0 :_cameraTriggerDistanceFact.rawValue().toDouble() / _cruiseSpeed;
}

QGeoCoordinate StructureScanComplexItem::coordinate(void) const
{
    if (_mapPolygon.count() > 0) {
        int entryVertex = qMax(qMin(_entryVertex, _mapPolygon.count() - 1), 0);
        return _mapPolygon.vertexCoordinate(entryVertex);
    } else {
        return QGeoCoordinate();
    }
}

QGeoCoordinate StructureScanComplexItem::exitCoordinate(void) const
{
    return coordinate();
}

void StructureScanComplexItem::_updateCoordinateAltitudes(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
}

void StructureScanComplexItem::rotateEntryPoint(void)
{
    _entryVertex++;
    if (_entryVertex >= _mapPolygon.count()) {
        _entryVertex = 0;
    }
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
}
