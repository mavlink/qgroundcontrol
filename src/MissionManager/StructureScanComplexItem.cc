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

const char* StructureScanComplexItem::settingsGroup =               "StructureScan";
const char* StructureScanComplexItem::altitudeName =                "Altitude";
const char* StructureScanComplexItem::structureHeightName =         "StructureHeight";
const char* StructureScanComplexItem::layersName =                  "Layers";
const char* StructureScanComplexItem::gimbalPitchName =             "GimbalPitch";

const char* StructureScanComplexItem::jsonComplexItemTypeValue =    "StructureScan";
const char* StructureScanComplexItem::_jsonCameraCalcKey =          "CameraCalc";
const char* StructureScanComplexItem::_jsonAltitudeRelativeKey =    "altitudeRelative";

StructureScanComplexItem::StructureScanComplexItem(Vehicle* vehicle, bool flyView, const QString& kmlFile, QObject* parent)
    : ComplexMissionItem        (vehicle, flyView, parent)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/StructureScan.SettingsGroup.json"), this /* QObject parent */))
    , _sequenceNumber           (0)
    , _dirty                    (false)
    , _altitudeRelative         (true)
    , _entryVertex              (0)
    , _ignoreRecalc             (false)
    , _scanDistance             (0.0)
    , _cameraShots              (0)
    , _cameraCalc               (vehicle, settingsGroup)
    , _altitudeFact             (settingsGroup, _metaDataMap[altitudeName])
    , _structureHeightFact      (settingsGroup, _metaDataMap[structureHeightName])
    , _layersFact               (settingsGroup, _metaDataMap[layersName])
    , _gimbalPitchFact          (settingsGroup, _metaDataMap[gimbalPitchName])
{
    _editorQml = "qrc:/qml/StructureScanEditor.qml";

    _altitudeFact.setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());

    connect(&_altitudeFact,     &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_layersFact,       &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_gimbalPitchFact,  &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);

    connect(&_layersFact,                           &Fact::valueChanged,    this, &StructureScanComplexItem::_recalcLayerInfo);
    connect(&_structureHeightFact,                  &Fact::valueChanged,    this, &StructureScanComplexItem::_recalcLayerInfo);
    connect(_cameraCalc.adjustedFootprintFrontal(), &Fact::valueChanged,    this, &StructureScanComplexItem::_recalcLayerInfo);

    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::_setDirty);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(&_altitudeFact, &Fact::valueChanged, this, &StructureScanComplexItem::_updateCoordinateAltitudes);

    connect(&_structurePolygon, &QGCMapPolygon::dirtyChanged,   this, &StructureScanComplexItem::_polygonDirtyChanged);
    connect(&_structurePolygon, &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_rebuildFlightPolygon);

    connect(&_structurePolygon, &QGCMapPolygon::countChanged,   this, &StructureScanComplexItem::_updateLastSequenceNumber);
    connect(&_layersFact,       &Fact::valueChanged,            this, &StructureScanComplexItem::_updateLastSequenceNumber);

    connect(&_flightPolygon,    &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_flightPathChanged);

    connect(_cameraCalc.distanceToSurface(),    &Fact::valueChanged,                this, &StructureScanComplexItem::_rebuildFlightPolygon);

    connect(&_flightPolygon,                        &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_recalcCameraShots);
    connect(_cameraCalc.adjustedFootprintSide(),    &Fact::valueChanged,            this, &StructureScanComplexItem::_recalcCameraShots);
    connect(&_layersFact,                           &Fact::valueChanged,            this, &StructureScanComplexItem::_recalcCameraShots);

    connect(&_cameraCalc, &CameraCalc::isManualCameraChanged, this, &StructureScanComplexItem::_updateGimbalPitch);

    _recalcLayerInfo();

    if (!kmlFile.isEmpty()) {
        _structurePolygon.loadKMLFile(kmlFile);
        _structurePolygon.setDirty(false);
    }

    setDirty(false);
}

void StructureScanComplexItem::_setScanDistance(double scanDistance)
{
    if (!qFuzzyCompare(_scanDistance, scanDistance)) {
        _scanDistance = scanDistance;
        emit complexDistanceChanged();
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

void StructureScanComplexItem::_updateLastSequenceNumber(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

int StructureScanComplexItem::lastSequenceNumber(void) const
{
    // Each structure layer contains:
    //  1 waypoint for each polygon vertex + 1 to go back to first polygon vertex for each layer
    //  Two commands for camera trigger start/stop
    int layerItemCount = _flightPolygon.count() + 1 + 2;

    int multiLayerItemCount = layerItemCount * _layersFact.rawValue().toInt();

    int itemCount = multiLayerItemCount + 2;    // +2 for ROI_WPNEXT_OFFSET and ROI_NONE commands

    return _sequenceNumber + itemCount - 1;
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
    QJsonObject saveObject;

    // Header
    saveObject[JsonHelper::jsonVersionKey] =                    2;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    saveObject[altitudeName] =              _altitudeFact.rawValue().toDouble();
    saveObject[structureHeightName] =       _structureHeightFact.rawValue().toDouble();
    saveObject[_jsonAltitudeRelativeKey] =  _altitudeRelative;
    saveObject[layersName] =                _layersFact.rawValue().toDouble();
    saveObject[gimbalPitchName] =           _gimbalPitchFact.rawValue().toDouble();

    QJsonObject cameraCalcObject;
    _cameraCalc.save(cameraCalcObject);
    saveObject[_jsonCameraCalcKey] = cameraCalcObject;

    _structurePolygon.saveToJson(saveObject);

    missionItems.append(saveObject);
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
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
        { altitudeName,                                 QJsonValue::Double, true },
        { structureHeightName,                          QJsonValue::Double, true },
        { _jsonAltitudeRelativeKey,                     QJsonValue::Bool,   true },
        { layersName,                                   QJsonValue::Double, true },
        { gimbalPitchName,                              QJsonValue::Double, false },    // This value was added after initial implementation so may be missing from older files
        { _jsonCameraCalcKey,                           QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    _structurePolygon.clear();

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version != 2) {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    // Load CameraCalc first since it will trigger camera name change which will trounce gimbal angles
    if (!_cameraCalc.load(complexObject[_jsonCameraCalcKey].toObject(), errorString)) {
        return false;
    }

    _altitudeFact.setRawValue   (complexObject[altitudeName].toDouble());
    _layersFact.setRawValue     (complexObject[layersName].toDouble());
    _altitudeRelative =         complexObject[_jsonAltitudeRelativeKey].toBool(true);

    double gimbalPitchValue = 0;
    if (complexObject.contains(gimbalPitchName)) {
        gimbalPitchValue = complexObject[gimbalPitchName].toDouble();
    }
    _gimbalPitchFact.setRawValue(gimbalPitchValue);

    if (!_structurePolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _structurePolygon.clear();
        return false;
    }

    return true;
}

void StructureScanComplexItem::_flightPathChanged(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
    emit greatestDistanceToChanged();
}

double StructureScanComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    QList<QGeoCoordinate> vertices = _flightPolygon.coordinateList();

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
    return _flightPolygon.count() > 2;
}

void StructureScanComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;
    double baseAltitude = _altitudeFact.rawValue().toDouble();

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET,
                                        MAV_FRAME_MISSION,
                                        0, 0, 0, 0,                             // param 1-4 not used
                                        _gimbalPitchFact.rawValue().toDouble(),
                                        0,                                      // Roll stays in standard orientation
                                        90,                                     // 90 degreee yaw offset to point to structure
                                        true,                                   // autoContinue
                                        false,                                  // isCurrentItem
                                        missionItemParent);
    items.append(item);

    for (int layer=0; layer<_layersFact.rawValue().toInt(); layer++) {
        bool addTriggerStart = true;
        // baseAltitude is the bottom of the first layer. Hence we need to move up half the distance of the camera footprint to center the camera
        // within the layer.
        double layerAltitude = baseAltitude + (_cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble() / 2.0) + (layer * _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());

        for (int i=0; i<_flightPolygon.count(); i++) {
            QGeoCoordinate vertexCoord = _flightPolygon.vertexCoordinate(i);

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

            if (addTriggerStart) {
                addTriggerStart = false;
                item = new MissionItem(seqNum++,
                                       MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                       MAV_FRAME_MISSION,
                                       _cameraCalc.adjustedFootprintSide()->rawValue().toDouble(),  // trigger distance
                                       0,                                                           // shutter integration (ignore)
                                       1,                                                           // trigger immediately when starting
                                       0, 0, 0, 0,                                                  // param 4-7 unused
                                       true,                                                        // autoContinue
                                       false,                                                       // isCurrentItem
                                       missionItemParent);
                items.append(item);
            }
        }

        QGeoCoordinate vertexCoord = _flightPolygon.vertexCoordinate(0);

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

        item = new MissionItem(seqNum++,
                               MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                               MAV_FRAME_MISSION,
                               0,           // stop triggering
                               0,           // shutter integration (ignore)
                               0,           // trigger immediately when starting
                               0, 0, 0, 0,  // param 4-7 unused
                               true,        // autoContinue
                               false,       // isCurrentItem
                               missionItemParent);
        items.append(item);
    }

    item = new MissionItem(seqNum++,
                           MAV_CMD_DO_SET_ROI_NONE,
                           MAV_FRAME_MISSION,
                           0, 0, 0,0, 0, 0, 0,                 // param 1-7 not used
                           true,                               // autoContinue
                           false,                              // isCurrentItem
                           missionItemParent);
    items.append(item);
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

double StructureScanComplexItem::timeBetweenShots(void)
{
    return _cruiseSpeed == 0 ? 0 : _cameraCalc.adjustedFootprintSide()->rawValue().toDouble() / _cruiseSpeed;
}

QGeoCoordinate StructureScanComplexItem::coordinate(void) const
{
    if (_flightPolygon.count() > 0) {
        int entryVertex = qMax(qMin(_entryVertex, _flightPolygon.count() - 1), 0);
        return _flightPolygon.vertexCoordinate(entryVertex);
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
    if (_entryVertex >= _flightPolygon.count()) {
        _entryVertex = 0;
    }
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
}

void StructureScanComplexItem::_rebuildFlightPolygon(void)
{
    _flightPolygon = _structurePolygon;
    _flightPolygon.offset(_cameraCalc.distanceToSurface()->rawValue().toDouble());
}

void StructureScanComplexItem::_recalcCameraShots(void)
{
    double triggerDistance = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    if (triggerDistance == 0) {
        _setCameraShots(0);
        return;
    }

    if (_flightPolygon.count() < 3) {
        _setCameraShots(0);
        return;
    }

    // Determine the distance for each polygon traverse
    double distance = 0;
    for (int i=0; i<_flightPolygon.count(); i++) {
        QGeoCoordinate coord1 = _flightPolygon.vertexCoordinate(i);
        QGeoCoordinate coord2 = _flightPolygon.vertexCoordinate(i + 1 == _flightPolygon.count() ? 0 : i + 1);
        distance += coord1.distanceTo(coord2);
    }
    if (distance == 0.0) {
        _setCameraShots(0);
        return;
    }

    int cameraShots = distance / triggerDistance;
    _setCameraShots(cameraShots * _layersFact.rawValue().toInt());
}

void StructureScanComplexItem::setAltitudeRelative(bool altitudeRelative)
{
    if (altitudeRelative != _altitudeRelative) {
        _altitudeRelative = altitudeRelative;
        emit altitudeRelativeChanged(altitudeRelative);
    }
}

void StructureScanComplexItem::_recalcLayerInfo(void)
{
    if (_cameraCalc.isManualCamera()) {
        // Structure height is calculated from layer count, layer height.
        _structureHeightFact.setSendValueChangedSignals(false);
        _structureHeightFact.setRawValue(_layersFact.rawValue().toInt() * _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());
        _structureHeightFact.clearDeferredValueChangeSignal();
        _structureHeightFact.setSendValueChangedSignals(true);
    } else {
        // Layer count is calculated from structure and layer heights
        _layersFact.setSendValueChangedSignals(false);
        _layersFact.setRawValue(qCeil(_structureHeightFact.rawValue().toDouble() / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble()));
        _layersFact.clearDeferredValueChangeSignal();
        _layersFact.setSendValueChangedSignals(true);
    }
}

void StructureScanComplexItem::_updateGimbalPitch(void)
{
    if (!_cameraCalc.isManualCamera()) {
        _gimbalPitchFact.setRawValue(0);
    }
}
