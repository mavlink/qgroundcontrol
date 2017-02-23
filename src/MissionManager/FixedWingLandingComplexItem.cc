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
const char* FixedWingLandingComplexItem::_loiterClockwiseName =         "Clockwise loiter";

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
    , _loiterClockwiseFact      (0, _loiterClockwiseName,       FactMetaData::valueTypeBool)
    , _landingHeadingFact       (0, _landingHeadingName,        FactMetaData::valueTypeDouble)
{
    _editorQml = "qrc:/qml/FWLandingPatternEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FWLandingPattern.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _loiterToLandDistanceFact.setMetaData   (_metaDataMap[_loiterToLandDistanceName]);
    _loiterAltitudeFact.setMetaData         (_metaDataMap[_loiterAltitudeName]);
    _loiterRadiusFact.setMetaData           (_metaDataMap[_loiterRadiusName]);
    _loiterClockwiseFact.setMetaData        (_metaDataMap[_loiterClockwiseName]);
    _landingHeadingFact.setMetaData         (_metaDataMap[_landingHeadingName]);

    _loiterToLandDistanceFact.setRawValue   (_loiterToLandDistanceFact.rawDefaultValue());
    _loiterAltitudeFact.setRawValue         (_loiterAltitudeFact.rawDefaultValue());
    _loiterRadiusFact.setRawValue           (_loiterRadiusFact.rawDefaultValue());
    _loiterClockwiseFact.setRawValue        (_loiterClockwiseFact.rawDefaultValue());
    _landingHeadingFact.setRawValue         (_landingHeadingFact.rawDefaultValue());

    connect(&_loiterToLandDistanceFact, &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcLoiterCoordFromFacts);
    connect(&_landingHeadingFact,       &Fact::valueChanged,                                    this, &FixedWingLandingComplexItem::_recalcLoiterCoordFromFacts);
    connect(this,                       &FixedWingLandingComplexItem::loiterCoordinateChanged,  this, &FixedWingLandingComplexItem::_recalcFactsFromCoords);
    connect(this,                       &FixedWingLandingComplexItem::landingCoordinateChanged, this, &FixedWingLandingComplexItem::_recalcFactsFromCoords);

    _textFieldFacts << QVariant::fromValue(&_loiterToLandDistanceFact) << QVariant::fromValue(&_loiterAltitudeFact) << QVariant::fromValue(&_loiterRadiusFact) << QVariant::fromValue(&_landingHeadingFact);
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

    // FIXME: Need real implementation
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
    // FIXME: Need real implementation
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);

    errorString = "NYI";
    return false;
}

double FixedWingLandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    // FIXME: Need real implementation
    Q_UNUSED(other);

    double greatestDistance = 0.0;

#if 0
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate currentCoord = _gridPoints[i].value<QGeoCoordinate>();
        double distance = currentCoord.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }
#endif

    return greatestDistance;
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
                                        MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  // param 1-7
                                        true,                               // autoContinue
                                        false,                              // isCurrentItem
                                        pMissionItems);                     // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    float loiterRadius = _loiterRadiusFact.rawValue().toDouble() * (_loiterClockwiseFact.rawValue().toBool() ? 1.0 : -1.0);
    item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_LOITER_TO_ALT,
                           MAV_FRAME_GLOBAL_RELATIVE_ALT,
                           1.0,                             // Heading required = true
                           loiterRadius,                    // Loiter radius
                           0.0,                             // param 3 - unused
                           0.0,                             // Exit crosstrack - center of waypoint
                           _loiterCoordinate.latitude(),
                           _loiterCoordinate.longitude(),
                           _loiterCoordinate.altitude(),
                           true,                            // autoContinue
                           false,                           // isCurrentItem
                           pMissionItems);                  // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_LAND,
                           MAV_FRAME_GLOBAL_RELATIVE_ALT,
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
    // FIXME: Need real implementation
    return 0;
}

void FixedWingLandingComplexItem::setCruiseSpeed(double cruiseSpeed)
{
    // FIXME: Need real implementation
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

        QPointF originPoint(east, north);
        north += _loiterToLandDistanceFact.rawValue().toDouble();
        QPointF loiterPoint(east, north);
        QPointF rotatedLoiterPoint = _rotatePoint(loiterPoint, originPoint, _landingHeadingFact.rawValue().toDouble());

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

        QPointF vector(eastLoiter - eastLand, northLoiter - northLand);
        double radians = atan2(vector.y(), vector.x());
        double degrees = qRadiansToDegrees(radians);
        degrees -= 90; // north up
        if (degrees < 0.0) {
            degrees += 360.0;
        } else if (degrees > 360.0) {
            degrees -= 360.0;
        }
        _landingHeadingFact.setRawValue(degrees);

        _ignoreRecalcSignals = false;
    }
}
