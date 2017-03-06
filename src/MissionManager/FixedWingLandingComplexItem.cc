/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FixedWingLandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(FixedWingLandingComplexItemLog, "FixedWingLandingComplexItemLog")

const char* FixedWingLandingComplexItem::jsonComplexItemTypeValue = "fwLandingPattern";

const char* FixedWingLandingComplexItem::_loiterToLandDistanceName =    "Landing distance";
const char* FixedWingLandingComplexItem::_landingHeadingName =          "Landing heading";
const char* FixedWingLandingComplexItem::_loiterAltitudeName =          "Loiter altitude";
const char* FixedWingLandingComplexItem::_loiterRadiusName =            "Loiter radius";
const char* FixedWingLandingComplexItem::_landingAltitudeName =         "Landing altitude";

const char* FixedWingLandingComplexItem::_jsonLoiterCoordinateKey =         "loiterCoordinate";
const char* FixedWingLandingComplexItem::_jsonLoiterRadiusKey =             "loiterRadius";
const char* FixedWingLandingComplexItem::_jsonLoiterClockwiseKey =          "loiterClockwise";
const char* FixedWingLandingComplexItem::_jsonLoiterAltitudeRelativeKey =   "loiterAltitudeRelative";
const char* FixedWingLandingComplexItem::_jsonLandingCoordinateKey =        "landCoordinate";
const char* FixedWingLandingComplexItem::_jsonLandingAltitudeRelativeKey =  "landAltitudeRelative";

QMap<QString, FactMetaData*> FixedWingLandingComplexItem::_metaDataMap;

FixedWingLandingComplexItem::FixedWingLandingComplexItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
    , _landingCoordSet(false)
    , _ignoreRecalcSignals(false)
    , _loiterToLandDistanceFact (0, _loiterToLandDistanceName,  FactMetaData::valueTypeDouble)
    , _loiterAltitudeFact       (0, _loiterAltitudeName,        FactMetaData::valueTypeDouble)
    , _loiterRadiusFact         (0, _loiterRadiusName,          FactMetaData::valueTypeDouble)
    , _landingHeadingFact       (0, _landingHeadingName,        FactMetaData::valueTypeDouble)
    , _landingAltitudeFact      (0, _landingAltitudeName,       FactMetaData::valueTypeDouble)
    , _loiterClockwise(true)
    , _loiterAltitudeRelative(true)
    , _landingAltitudeRelative(true)
{
    _editorQml = "qrc:/qml/FWLandingPatternEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FWLandingPattern.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _loiterToLandDistanceFact.setMetaData   (_metaDataMap[_loiterToLandDistanceName]);
    _loiterAltitudeFact.setMetaData         (_metaDataMap[_loiterAltitudeName]);
    _loiterRadiusFact.setMetaData           (_metaDataMap[_loiterRadiusName]);
    _landingHeadingFact.setMetaData         (_metaDataMap[_landingHeadingName]);
    _landingAltitudeFact.setMetaData        (_metaDataMap[_landingAltitudeName]);

    _loiterToLandDistanceFact.setRawValue   (_loiterToLandDistanceFact.rawDefaultValue());
    _loiterAltitudeFact.setRawValue         (_loiterAltitudeFact.rawDefaultValue());
    _loiterRadiusFact.setRawValue           (_loiterRadiusFact.rawDefaultValue());
    _landingHeadingFact.setRawValue         (_landingHeadingFact.rawDefaultValue());
    _landingAltitudeFact.setRawValue        (_landingAltitudeFact.rawDefaultValue());

    connect(&_loiterAltitudeFact,       &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact);
    connect(&_landingAltitudeFact,       &Fact::valueChanged,                                   this, &FixedWingLandingComplexItem::_updateLandingCoodinateAltitudeFromFact);
    connect(&_loiterToLandDistanceFact, &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcLoiterCoordFromFacts);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcLoiterCoordFromFacts);
    connect(this,                       &FixedWingLandingComplexItem::loiterCoordinateChanged,  this, &FixedWingLandingComplexItem::_recalcFactsFromCoords);
    connect(this,                       &FixedWingLandingComplexItem::landingCoordinateChanged, this, &FixedWingLandingComplexItem::_recalcFactsFromCoords);
}

int FixedWingLandingComplexItem::lastSequenceNumber(void) const
{
    // land start, loiter, land
    return _sequenceNumber + 2;
}

void FixedWingLandingComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void FixedWingLandingComplexItem::save(QJsonObject& saveObject) const
{
    saveObject[JsonHelper::jsonVersionKey] =                    1;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    QGeoCoordinate coordinate;
    QJsonValue jsonCoordinate;

    coordinate = _loiterCoordinate;
    coordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonLoiterCoordinateKey] = jsonCoordinate;

    coordinate = _landingCoordinate;
    coordinate.setAltitude(_landingAltitudeFact.rawValue().toDouble());
    JsonHelper::saveGeoCoordinate(coordinate, true /* writeAltitude */, jsonCoordinate);
    saveObject[_jsonLandingCoordinateKey] = jsonCoordinate;

    saveObject[_jsonLoiterRadiusKey] =              _loiterRadiusFact.rawValue().toDouble();
    saveObject[_jsonLoiterClockwiseKey] =           _loiterClockwise;
    saveObject[_jsonLoiterAltitudeRelativeKey] =    _loiterAltitudeRelative;
    saveObject[_jsonLandingAltitudeRelativeKey] =   _landingAltitudeRelative;
}

void FixedWingLandingComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool FixedWingLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonLoiterCoordinateKey,                     QJsonValue::Array,  true },
        { _jsonLoiterRadiusKey,                         QJsonValue::Double, true },
        { _jsonLoiterClockwiseKey,                      QJsonValue::Bool,   true },
        { _jsonLoiterAltitudeRelativeKey,               QJsonValue::Bool,   true },
        { _jsonLandingCoordinateKey,                    QJsonValue::Array,  true },
        { _jsonLandingAltitudeRelativeKey,              QJsonValue::Bool,   true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("QGroundControl does not support loading this complex mission item type: %1:2").arg(itemType).arg(complexType);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    QGeoCoordinate coordinate;
    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLoiterCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _loiterCoordinate = coordinate;
    _loiterAltitudeFact.setRawValue(coordinate.altitude());

    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLandingCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _landingCoordinate = coordinate;
    _landingAltitudeFact.setRawValue(coordinate.altitude());

    _loiterRadiusFact.setRawValue(complexObject[_jsonLoiterRadiusKey].toDouble());
    _loiterClockwise  = complexObject[_jsonLoiterClockwiseKey].toBool();
    _loiterAltitudeRelative = complexObject[_jsonLoiterAltitudeRelativeKey].toBool();
    _landingAltitudeRelative = complexObject[_jsonLandingAltitudeRelativeKey].toBool();

    _landingCoordSet = true;
    _recalcFactsFromCoords();

    return true;
}

double FixedWingLandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    return qMax(_loiterCoordinate.distanceTo(other),_landingCoordinate.distanceTo(other));
}

bool FixedWingLandingComplexItem::specifiesCoordinate(void) const
{
    return true;
}

QmlObjectListModel* FixedWingLandingComplexItem::getMissionItems(void) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

    int seqNum = _sequenceNumber;

    MissionItem* item = new MissionItem(seqNum++,                           // sequence number
                                        MAV_CMD_DO_LAND_START,              // MAV_CMD
                                        MAV_FRAME_MISSION,                  // MAV_FRAME
                                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  // param 1-7
                                        true,                               // autoContinue
                                        false,                              // isCurrentItem
                                        pMissionItems);                     // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    float loiterRadius = _loiterRadiusFact.rawValue().toDouble() * (_loiterClockwise ? 1.0 : -1.0);
    item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_LOITER_TO_ALT,
                           _loiterAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           1.0,                             // Heading required = true
                           loiterRadius,                    // Loiter radius
                           0.0,                             // param 3 - unused
                           1.0,                             // Exit crosstrack - tangent of loiter to land point
                           _loiterCoordinate.latitude(),
                           _loiterCoordinate.longitude(),
                           _loiterAltitudeFact.rawValue().toDouble(),
                           true,                            // autoContinue
                           false,                           // isCurrentItem
                           pMissionItems);                  // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_LAND,
                           _landingAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                           0.0, 0.0, 0.0, 0.0,                 // param 1-4
                           _landingCoordinate.latitude(),
                           _landingCoordinate.longitude(),
                           0.0,                                // altitude
                           true,                               // autoContinue
                           false,                              // isCurrentItem
                           pMissionItems);                     // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    return pMissionItems;
}

double FixedWingLandingComplexItem::complexDistance(void) const
{
    return _loiterCoordinate.distanceTo(_landingCoordinate);
}

void FixedWingLandingComplexItem::setCruiseSpeed(double cruiseSpeed)
{
    // We don't care about cruise speed
    Q_UNUSED(cruiseSpeed);
}

void FixedWingLandingComplexItem::setLandingCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _landingCoordinate) {
        _landingCoordinate = coordinate;
        if (_landingCoordSet) {
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
        } else {
            _ignoreRecalcSignals = true;
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
            _ignoreRecalcSignals = false;
            _landingCoordSet = true;
            _recalcLoiterCoordFromFacts();
            emit landingCoordSetChanged(true);
        }
    }
}

void FixedWingLandingComplexItem::setLoiterCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _loiterCoordinate) {
        _loiterCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit loiterCoordinateChanged(coordinate);
    }
}

void FixedWingLandingComplexItem::_recalcLoiterCoordFromFacts(void)
{
    if (!_ignoreRecalcSignals && _landingCoordSet) {
        double          north, east, down;
        QGeoCoordinate  tangentOrigin = _landingCoordinate;

        convertGeoToNed(_landingCoordinate, tangentOrigin, &north, &east, &down);

        // Heading is from loiter to land, so we need to rotate angle 180 degrees and go the opposite direction
        double heading = _landingHeadingFact.rawValue().toDouble();
        heading += 180.0;
        heading *= -1.0;

        QPointF originPoint(east, north);
        north += _loiterToLandDistanceFact.rawValue().toDouble();
        QPointF loiterPoint(east, north);
        QPointF rotatedLoiterPoint = _rotatePoint(loiterPoint, originPoint, heading);

        convertNedToGeo(rotatedLoiterPoint.y(), rotatedLoiterPoint.x(), down, tangentOrigin, &_loiterCoordinate);

        _ignoreRecalcSignals = true;
        emit loiterCoordinateChanged(_loiterCoordinate);
        emit coordinateChanged(_loiterCoordinate);
        _ignoreRecalcSignals = false;
    }
}

QPointF FixedWingLandingComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void FixedWingLandingComplexItem::_recalcFactsFromCoords(void)
{
    if (!_ignoreRecalcSignals && _landingCoordSet) {

        // Prevent signal recursion
        _ignoreRecalcSignals = true;

        // Calc new distance

        double          northLand, eastLand, down;
        double          northLoiter, eastLoiter;
        QGeoCoordinate  tangentOrigin = _landingCoordinate;

        convertGeoToNed(_landingCoordinate, tangentOrigin, &northLand, &eastLand, &down);
        convertGeoToNed(_loiterCoordinate, tangentOrigin, &northLoiter, &eastLoiter, &down);

        double newDistance = sqrt(pow(eastLoiter - eastLand, 2.0) + pow(northLoiter - northLand, 2.0));
        _loiterToLandDistanceFact.setRawValue(newDistance);

        // Calc new heading

        QPointF vector(eastLand - eastLoiter, northLand - northLoiter);
        double radians = atan2(vector.y(), vector.x());
        double degrees = qRadiansToDegrees(radians);
        // Change angle to north up = 0 degrees
        degrees -= 90;
        // Reverse the angle direction to go from mathematic angle (counter-clockwise) to compass heading (clockwise)
        degrees *= -1.0;
        // Bring with 0-360 range
        if (degrees < 0.0) {
            degrees += 360.0;
        } else if (degrees > 360.0) {
            degrees -= 360.0;
        }
        _landingHeadingFact.setRawValue(degrees);

        _ignoreRecalcSignals = false;
    }
}

void FixedWingLandingComplexItem::_updateLoiterCoodinateAltitudeFromFact(void)
{
    _loiterCoordinate.setAltitude(_loiterAltitudeFact.rawValue().toDouble());
}

void FixedWingLandingComplexItem::_updateLandingCoodinateAltitudeFromFact(void)
{
    _landingCoordinate.setAltitude(_landingAltitudeFact.rawValue().toDouble());
}
