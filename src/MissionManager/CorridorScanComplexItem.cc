/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(CorridorScanComplexItemLog, "CorridorScanComplexItemLog")

const char* CorridorScanComplexItem::_corridorWidthFactName =          "CorridorWidth";

const char* CorridorScanComplexItem::jsonComplexItemTypeValue =        "CorridorScan";
const char* CorridorScanComplexItem::_jsonCameraCalcKey =              "CameraCalc";

QMap<QString, FactMetaData*> CorridorScanComplexItem::_metaDataMap;

CorridorScanComplexItem::CorridorScanComplexItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem        (vehicle, parent)
    , _sequenceNumber           (0)
    , _dirty                    (false)
    , _corridorWidthFact        (0, _corridorWidthFactName, FactMetaData::valueTypeDouble)
    , _ignoreRecalc             (false)
    , _scanDistance             (0.0)
    , _cameraShots              (0)
    , _cameraMinTriggerInterval (0)
    , _cameraCalc               (vehicle)
{
    _editorQml = "qrc:/qml/CorridorScanEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CorridorScan.SettingsGroup.json"), NULL /* QObject parent */);
    }

    _corridorWidthFact.setMetaData(_metaDataMap[_corridorWidthFactName]);

    _corridorWidthFact.setRawValue(_corridorWidthFact.rawDefaultValue());

    connect(&_corridorWidthFact,    &Fact::valueChanged,                                this, &CorridorScanComplexItem::_setDirty);
    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,                       this, &CorridorScanComplexItem::_setDirty);
    connect(this,                   &CorridorScanComplexItem::altitudeRelativeChanged,  this, &CorridorScanComplexItem::_setDirty);

    connect(this, &CorridorScanComplexItem::altitudeRelativeChanged,       this, &CorridorScanComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(this, &CorridorScanComplexItem::altitudeRelativeChanged,       this, &CorridorScanComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(&_corridorPolyline, &QGCMapPolyline::dirtyChanged,  this, &CorridorScanComplexItem::_polylineDirtyChanged);
    connect(&_corridorPolyline, &QGCMapPolyline::countChanged,  this, &CorridorScanComplexItem::_polylineCountChanged);

    connect(_cameraCalc.adjustedFootprintSide(), &Fact::valueChanged, this, &CorridorScanComplexItem::_rebuildTransects);

    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,   this, &CorridorScanComplexItem::_rebuildCorridor);
    connect(&_corridorWidthFact,    &Fact::valueChanged,            this, &CorridorScanComplexItem::_rebuildCorridor);

    connect(&_corridorPolyline,                     &QGCMapPolyline::countChanged,  this, &CorridorScanComplexItem::_signalLastSequenceNumberChanged);
    connect(&_corridorWidthFact,                    &Fact::valueChanged,            this, &CorridorScanComplexItem::_signalLastSequenceNumberChanged);
    connect(_cameraCalc.adjustedFootprintSide(),    &Fact::valueChanged,            this, &CorridorScanComplexItem::_signalLastSequenceNumberChanged);

    connect(&_corridorPolygon, &QGCMapPolygon::pathChanged, this, &CorridorScanComplexItem::coveredAreaChanged);

    connect(this, &CorridorScanComplexItem::transectPointsChanged, this, &CorridorScanComplexItem::complexDistanceChanged);
    connect(this, &CorridorScanComplexItem::transectPointsChanged, this, &CorridorScanComplexItem::greatestDistanceToChanged);

    _rebuildCorridor();
}

void CorridorScanComplexItem::_setScanDistance(double scanDistance)
{
    if (!qFuzzyCompare(_scanDistance, scanDistance)) {
        _scanDistance = scanDistance;
        emit complexDistanceChanged();
    }
}

void CorridorScanComplexItem::_setCameraShots(int cameraShots)
{
    if (_cameraShots != cameraShots) {
        _cameraShots = cameraShots;
        emit cameraShotsChanged();
    }
}

void CorridorScanComplexItem::_clearInternal(void)
{
    setDirty(true);

    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void CorridorScanComplexItem::_polylineCountChanged(int count)
{
    Q_UNUSED(count);
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

int CorridorScanComplexItem::lastSequenceNumber(void) const
{
    return _sequenceNumber + ((_corridorPolyline.count() + 2 /* trigger start/stop */) * _transectCount());
}

void CorridorScanComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void CorridorScanComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    1;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_corridorWidthFactName] =                        _corridorWidthFact.rawValue().toDouble();

    QJsonObject cameraCalcObject;
    _cameraCalc.save(cameraCalcObject);
    saveObject[_jsonCameraCalcKey] = cameraCalcObject;

    _corridorPolyline.saveToJson(saveObject);

    missionItems.append(saveObject);
}

void CorridorScanComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool CorridorScanComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _corridorWidthFactName,                       QJsonValue::Double, true },
        { QGCMapPolyline::jsonPolylineKey,              QJsonValue::Array,  true },
        { _jsonCameraCalcKey,                           QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    _corridorPolyline.clear();

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

    if (!_cameraCalc.load(complexObject[_jsonCameraCalcKey].toObject(), errorString)) {
        return false;
    }

    if (!_corridorPolyline.loadFromJson(complexObject, true /* required */, errorString)) {
        _corridorPolyline.clear();
        _rebuildCorridor();
        return false;
    }

    _rebuildCorridor();

    return true;
}

double CorridorScanComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
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

bool CorridorScanComplexItem::specifiesCoordinate(void) const
{
    return _corridorPolyline.count() > 1;
}

int CorridorScanComplexItem::_transectCount(void) const
{
    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    return fullWidth > 0.0 ? qCeil(fullWidth / transectSpacing) : 1;
}

void CorridorScanComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum =        _sequenceNumber;
    int pointIndex =    0;

    while (pointIndex < _transectPoints.count()) {
        bool addTrigger = true;

        for (int i=0; i<_corridorPolyline.count(); i++) {
            QGeoCoordinate vertexCoord = _transectPoints[pointIndex++].value<QGeoCoordinate>();

            MissionItem* item = new MissionItem(seqNum++,
                                                MAV_CMD_NAV_WAYPOINT,
                                                MAV_FRAME_GLOBAL_RELATIVE_ALT,                          // FIXME: Manual camera should support AMSL alt
                                                0,                                                      // No hold time
                                                0.0,                                                    // No acceptance radius specified
                                                0.0,                                                    // Pass through waypoint
                                                std::numeric_limits<double>::quiet_NaN(),               // Yaw unchanged
                                                vertexCoord.latitude(),
                                                vertexCoord.longitude(),
                                                _cameraCalc.distanceToSurface()->rawValue().toDouble(), // Altitude
                                                true,                                                   // autoContinue
                                                false,                                                  // isCurrentItem
                                                missionItemParent);
            items.append(item);

            if (addTrigger) {
                addTrigger = false;
                item = new MissionItem(seqNum++,
                                       MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                       MAV_FRAME_MISSION,
                                       _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble(),   // trigger distance
                                       0,                                                               // shutter integration (ignore)
                                       1,                                                               // trigger immediately when starting
                                       0, 0, 0, 0,                                                      // param 4-7 unused
                                       true,                                                            // autoContinue
                                       false,                                                           // isCurrentItem
                                       missionItemParent);
                items.append(item);
            }
        }

        MissionItem* item = new MissionItem(seqNum++,
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

void CorridorScanComplexItem::setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    ComplexMissionItem::setMissionFlightStatus(missionFlightStatus);
    if (!qFuzzyCompare(_cruiseSpeed, missionFlightStatus.vehicleSpeed)) {
        _cruiseSpeed = missionFlightStatus.vehicleSpeed;
        emit timeBetweenShotsChanged();
    }
}

void CorridorScanComplexItem::_setDirty(void)
{
    setDirty(true);
}

void CorridorScanComplexItem::applyNewAltitude(double newAltitude)
{
    Q_UNUSED(newAltitude);
    // FIXME: NYI
    //_altitudeFact.setRawValue(newAltitude);
}

void CorridorScanComplexItem::_polylineDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

double CorridorScanComplexItem::timeBetweenShots(void)
{
    return _cruiseSpeed == 0 ? 0 : _cameraCalc.adjustedFootprintSide()->rawValue().toDouble() / _cruiseSpeed;
}

void CorridorScanComplexItem::_updateCoordinateAltitudes(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
}

void CorridorScanComplexItem::rotateEntryPoint(void)
{
#if 0
    _entryVertex++;
    if (_entryVertex >= _flightPolygon.count()) {
        _entryVertex = 0;
    }
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
#endif
}

void CorridorScanComplexItem::_rebuildCorridorPolygon(void)
{
    if (_corridorPolyline.count() < 2) {
        _corridorPolygon.clear();
        return;
    }

    double halfWidth = _corridorWidthFact.rawValue().toDouble() / 2.0;

    QList<QGeoCoordinate> firstSideVertices = _corridorPolyline.offsetPolyline(halfWidth);
    QList<QGeoCoordinate> secondSideVertices = _corridorPolyline.offsetPolyline(-halfWidth);

    _corridorPolygon.clear();
    foreach (const QGeoCoordinate& vertex, firstSideVertices) {
        _corridorPolygon.appendVertex(vertex);
    }
    for (int i=secondSideVertices.count() - 1; i >= 0; i--) {
        _corridorPolygon.appendVertex(secondSideVertices[i]);
    }
}

void CorridorScanComplexItem::_rebuildTransects(void)
{

    _transectPoints.clear();

    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    double halfWidth = fullWidth / 2.0;
    int transectCount = _transectCount();
    double normalizedTransectPosition = transectSpacing / 2.0;

    _cameraShots = 0;
    int singleTransectImageCount = qCeil(_corridorPolyline.length() / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());

    bool reverseVertices = false;
    for (int i=0; i<transectCount; i++) {
        _cameraShots += singleTransectImageCount;

        double offsetDistance;
        if (transectCount == 1) {
            // Single transect is flown over scan line
            offsetDistance = 0;
        } else {
            // Convert from normalized to absolute transect offset distance
            offsetDistance = halfWidth - normalizedTransectPosition;
        }

        QList<QGeoCoordinate> transectVertices = _corridorPolyline.offsetPolyline(offsetDistance);
        if (reverseVertices) {
            reverseVertices = false;
            QList<QGeoCoordinate> reversedVertices;
            for (int j=transectVertices.count()-1; j>=0; j--) {
                reversedVertices.append(transectVertices[j]);
            }
            transectVertices = reversedVertices;
        } else {
            reverseVertices = true;
        }
        for (int i=0; i<transectVertices.count(); i++) {
            _transectPoints.append(QVariant::fromValue((transectVertices[i])));
        }

        normalizedTransectPosition += transectSpacing;
    }

    _coordinate = _transectPoints.count() ? _transectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _transectPoints.count() ? _transectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();

    emit transectPointsChanged();
    emit cameraShotsChanged();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

void CorridorScanComplexItem::_rebuildCorridor(void)
{
    _rebuildCorridorPolygon();
    _rebuildTransects();
}

void CorridorScanComplexItem::_signalLastSequenceNumberChanged(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

double CorridorScanComplexItem::coveredArea(void) const
{
    return _corridorPolygon.area();
}
