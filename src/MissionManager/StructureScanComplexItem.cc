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

const char* StructureScanComplexItem::_altitudeFactName =               "Altitude";
const char* StructureScanComplexItem::_layersFactName =                 "Layers";
const char* StructureScanComplexItem::_gimbalPitchFactName =            "GimbalPitch";
const char* StructureScanComplexItem::_gimbalYawFactName =              "GimbalYaw";

const char* StructureScanComplexItem::jsonComplexItemTypeValue =        "StructureScan";
const char* StructureScanComplexItem::_jsonCameraCalcKey =              "CameraCalc";
const char* StructureScanComplexItem::_jsonAltitudeRelativeKey =        "altitudeRelative";
const char* StructureScanComplexItem::_jsonYawVehicleToStructureKey =   "yawVehicleToStructure";

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
    , _cameraCalc               (vehicle)
    , _yawVehicleToStructure    (false)
    , _altitudeFact             (0, _altitudeFactName,              FactMetaData::valueTypeDouble)
    , _layersFact               (0, _layersFactName,                FactMetaData::valueTypeUint32)
    , _gimbalPitchFact          (0, _gimbalPitchFactName,                   FactMetaData::valueTypeDouble)
    , _gimbalYawFact            (0, _gimbalYawFactName,                     FactMetaData::valueTypeDouble)
{
    _editorQml = "qrc:/qml/StructureScanEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/StructureScan.SettingsGroup.json"), NULL /* QObject parent */);
    }

    _altitudeFact.setMetaData   (_metaDataMap[_altitudeFactName]);
    _layersFact.setMetaData     (_metaDataMap[_layersFactName]);
    _gimbalPitchFact.setMetaData(_metaDataMap[_gimbalPitchFactName]);
    _gimbalYawFact.setMetaData  (_metaDataMap[_gimbalYawFactName]);

    _altitudeFact.setRawValue   (_altitudeFact.rawDefaultValue());
    _layersFact.setRawValue     (_layersFact.rawDefaultValue());
    _gimbalPitchFact.setRawValue(_gimbalPitchFact.rawDefaultValue());
    _gimbalYawFact.setRawValue  (_gimbalYawFact.rawDefaultValue());

    _altitudeFact.setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());

    connect(&_altitudeFact,     &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_layersFact,       &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_gimbalPitchFact,  &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);
    connect(&_gimbalYawFact,    &Fact::valueChanged, this, &StructureScanComplexItem::_setDirty);

    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::_setDirty);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(this, &StructureScanComplexItem::altitudeRelativeChanged,       this, &StructureScanComplexItem::exitCoordinateHasRelativeAltitudeChanged);
    connect(this, &StructureScanComplexItem::yawVehicleToStructureChanged,  this, &StructureScanComplexItem::_setDirty);

    connect(&_altitudeFact, &Fact::valueChanged, this, &StructureScanComplexItem::_updateCoordinateAltitudes);

    connect(&_structurePolygon, &QGCMapPolygon::dirtyChanged,   this, &StructureScanComplexItem::_polygonDirtyChanged);
    connect(&_structurePolygon, &QGCMapPolygon::countChanged,   this, &StructureScanComplexItem::_polygonCountChanged);
    connect(&_structurePolygon, &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_rebuildFlightPolygon);

    connect(&_flightPolygon,    &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_flightPathChanged);

    connect(_cameraCalc.distanceToSurface(),    &Fact::valueChanged,                this, &StructureScanComplexItem::_rebuildFlightPolygon);
    connect(&_cameraCalc,                       &CameraCalc::cameraNameChanged,     this, &StructureScanComplexItem::_resetGimbal);

    connect(&_flightPolygon,                        &QGCMapPolygon::pathChanged,    this, &StructureScanComplexItem::_recalcCameraShots);
    connect(_cameraCalc.adjustedFootprintSide(),    &Fact::valueChanged,            this, &StructureScanComplexItem::_recalcCameraShots);
    connect(&_layersFact,                           &Fact::valueChanged,            this, &StructureScanComplexItem::_recalcCameraShots);
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
            (_layersFact.rawValue().toInt() *
                ((_flightPolygon.count() + 1) + // 1 waypoint for each polygon vertex + 1 to go back to first polygon vertex for each layer
                 2)) +                          // Camera trigger start/stop for each layer
            1;                                  // Gimbal control command
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
    saveObject[JsonHelper::jsonVersionKey] =                    1;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    saveObject[_gimbalPitchFactName] =          _gimbalPitchFact.rawValue().toDouble();
    saveObject[_gimbalYawFactName] =            _gimbalYawFact.rawValue().toDouble();
    saveObject[_altitudeFactName] =             _altitudeFact.rawValue().toDouble();
    saveObject[_jsonAltitudeRelativeKey] =      _altitudeRelative;
    saveObject[_layersFactName] =               _layersFact.rawValue().toDouble();
    saveObject[_jsonYawVehicleToStructureKey] = _yawVehicleToStructure;

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
        { _gimbalPitchFactName,                         QJsonValue::Double, true },
        { _gimbalYawFactName,                           QJsonValue::Double, true },
        { _altitudeFactName,                            QJsonValue::Double, true },
        { _jsonAltitudeRelativeKey,                     QJsonValue::Bool,   false },
        { _layersFactName,                              QJsonValue::Double, true },
        { _jsonCameraCalcKey,                           QJsonValue::Object, true },
        { _jsonYawVehicleToStructureKey,                QJsonValue::Bool, true },
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
    if (version != 1) {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    // Load CameraCalc first since it will trigger camera name change which will trounce gimbal angles
    if (!_cameraCalc.load(complexObject[_jsonCameraCalcKey].toObject(), errorString)) {
        return false;
    }

    _gimbalPitchFact.setRawValue(complexObject[_gimbalPitchFactName].toDouble());
    _gimbalYawFact.setRawValue  (complexObject[_gimbalYawFactName].toDouble());
    _altitudeFact.setRawValue   (complexObject[_altitudeFactName].toDouble());
    _layersFact.setRawValue     (complexObject[_layersFactName].toDouble());
    _altitudeRelative =         complexObject[_jsonAltitudeRelativeKey].toBool(true);
    _yawVehicleToStructure =    complexObject[_jsonYawVehicleToStructureKey].toBool(true);

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

    if (_yawVehicleToStructure) {
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_CONDITION_YAW,
                                            MAV_FRAME_MISSION,
                                            90.0,                               // Target angle
                                            0,                                  // Use default turn rate
                                            1,                                  // Clockwise turn
                                            0,                                  // Absolute angle specified
                                            0, 0, 0,                            // param 5-7 not used
                                            true,                               // autoContinue
                                            false,                              // isCurrentItem
                                            missionItemParent);
        items.append(item);
    } else {
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_MOUNT_CONTROL,
                                            MAV_FRAME_MISSION,
                                            _gimbalPitchFact.rawValue().toDouble(),
                                            0,                                  // Gimbal roll
                                            _gimbalYawFact.rawValue().toDouble(),
                                            0, 0, 0,                            // param 4-6 not used
                                            MAV_MOUNT_MODE_MAVLINK_TARGETING,
                                            true,                               // autoContinue
                                            false,                              // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

    for (int layer=0; layer<_layersFact.rawValue().toInt(); layer++) {
        bool addTriggerStart = true;
        double layerAltitude = baseAltitude + (layer * _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());

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

    int cameraShots = distance / _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    _setCameraShots(cameraShots * _layersFact.rawValue().toInt());
}

void StructureScanComplexItem::_resetGimbal(void)
{
    _gimbalPitchFact.setCookedValue(0);
    _gimbalYawFact.setCookedValue(90);
}

void StructureScanComplexItem::setAltitudeRelative(bool altitudeRelative)
{
    if (altitudeRelative != _altitudeRelative) {
        _altitudeRelative = altitudeRelative;
        emit altitudeRelativeChanged(altitudeRelative);
    }
}

void StructureScanComplexItem::setYawVehicleToStructure(bool yawVehicleToStructure)
{
    if (yawVehicleToStructure != _yawVehicleToStructure) {
        _yawVehicleToStructure = yawVehicleToStructure;
        emit yawVehicleToStructureChanged(yawVehicleToStructure);
    }
}
