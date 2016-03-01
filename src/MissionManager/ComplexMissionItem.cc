/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "ComplexMissionItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(ComplexMissionItemLog, "ComplexMissionItemLog")

const char* ComplexMissionItem::_jsonVersionKey =               "version";
const char* ComplexMissionItem::_jsonTypeKey =                  "type";
const char* ComplexMissionItem::_jsonPolygonKey =               "polygon";
const char* ComplexMissionItem::_jsonIdKey =                    "id";
const char* ComplexMissionItem::_jsonGridAltitudeKey =          "gridAltitude";
const char* ComplexMissionItem::_jsonGridAltitudeRelativeKey =  "gridAltitudeRelative";
const char* ComplexMissionItem::_jsonGridAngleKey =             "gridAngle";
const char* ComplexMissionItem::_jsonGridSpacingKey =           "gridSpacing";
const char* ComplexMissionItem::_jsonCameraTriggerKey =         "cameraTrigger";
const char* ComplexMissionItem::_jsonCameraTriggerDistanceKey = "cameraTriggerDistance";

const char* ComplexMissionItem::_complexType = "survey";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
    , _cameraTrigger(false)
    , _gridAltitudeRelative(true)
    , _gridAltitudeFact (0, "Altitude:",        FactMetaData::valueTypeDouble)
    , _gridAngleFact    (0, "Grid angle:",      FactMetaData::valueTypeDouble)
    , _gridSpacingFact  (0, "Grid spacing:",    FactMetaData::valueTypeDouble)
    , _cameraTriggerDistanceFact(0, "Camera trigger distance", FactMetaData::valueTypeDouble)
{
    _gridAltitudeFact.setRawValue(25);
    _gridSpacingFact.setRawValue(10);
    _cameraTriggerDistanceFact.setRawValue(25);

    connect(&_gridSpacingFact,  &Fact::valueChanged, this, &ComplexMissionItem::_generateGrid);
    connect(&_gridAngleFact,    &Fact::valueChanged, this, &ComplexMissionItem::_generateGrid);

    connect(this, &ComplexMissionItem::cameraTriggerChanged, this, &ComplexMissionItem::_cameraTriggerChanged);
}

void ComplexMissionItem::clearPolygon(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit polygonPathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in MapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();

    _clearGrid();
    setDirty(true);

    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void ComplexMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();

    int pointCount = _polygonPath.count();
    if (pointCount >= 3) {
        if (pointCount == 3) {
            emit specifiesCoordinateChanged();
        }
        _generateGrid();
    }
    setDirty(true);
}

int ComplexMissionItem::lastSequenceNumber(void) const
{
    int lastSeq = _sequenceNumber;

    if (_gridPoints.count()) {
        lastSeq += _gridPoints.count() - 1;
        if (_cameraTrigger) {
            // Account for two trigger messages
            lastSeq += 2;
        }
    }

    return lastSeq;
}

void ComplexMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
    }
}

void ComplexMissionItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void ComplexMissionItem::save(QJsonObject& saveObject) const
{
    saveObject[_jsonVersionKey] =               1;
    saveObject[_jsonTypeKey] =                  _complexType;
    saveObject[_jsonIdKey] =                    sequenceNumber();
    saveObject[_jsonGridAltitudeKey] =          _gridAltitudeFact.rawValue().toDouble();
    saveObject[_jsonGridAltitudeRelativeKey] =  _gridAltitudeRelative;
    saveObject[_jsonGridAngleKey] =             _gridAngleFact.rawValue().toDouble();
    saveObject[_jsonGridSpacingKey] =           _gridSpacingFact.rawValue().toDouble();
    saveObject[_jsonCameraTriggerKey] =         _cameraTrigger;
    saveObject[_jsonCameraTriggerDistanceKey] = _cameraTriggerDistanceFact.rawValue().toDouble();

    // Polygon shape

    QJsonArray polygonArray;

    for (int i=0; i<_polygonPath.count(); i++) {
        const QVariant& polyVar = _polygonPath[i];

        QJsonValue jsonValue;
        JsonHelper::writeQGeoCoordinate(jsonValue, polyVar.value<QGeoCoordinate>(), false /* writeAltitude */);
        polygonArray += jsonValue;
    }

    saveObject[_jsonPolygonKey] = polygonArray;
}

void ComplexMissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

void ComplexMissionItem::_clear(void)
{
    clearPolygon();
    _clearGrid();
}


bool ComplexMissionItem::load(const QJsonObject& complexObject, QString& errorString)
{
    _clear();

    // Validate requires keys
    QStringList requiredKeys;
    requiredKeys << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << _jsonGridAltitudeKey << _jsonGridAngleKey << _jsonGridSpacingKey <<
                    _jsonCameraTriggerKey << _jsonCameraTriggerDistanceKey << _jsonGridAltitudeRelativeKey;
    if (!JsonHelper::validateRequiredKeys(complexObject, requiredKeys, errorString)) {
        _clear();
        return false;
    }

    // Validate types
    QStringList keyList;
    QList<QJsonValue::Type> typeList;
    keyList << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << _jsonGridAltitudeKey << _jsonGridAngleKey << _jsonGridSpacingKey <<
               _jsonCameraTriggerKey << _jsonCameraTriggerDistanceKey << _jsonGridAltitudeRelativeKey;
    typeList << QJsonValue::Double << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Double << QJsonValue::Double<< QJsonValue::Double <<
                QJsonValue::Bool << QJsonValue::Double << QJsonValue::Bool;
    if (!JsonHelper::validateKeyTypes(complexObject, keyList, typeList, errorString)) {
        _clear();
        return false;
    }

    // Version check
    if (complexObject[_jsonVersionKey].toInt() != 1) {
        errorString = tr("QGroundControl does not support this version of survey items");
        _clear();
        return false;
    }
    QString complexType = complexObject[_jsonTypeKey].toString();
    if (complexType != _complexType) {
        errorString = tr("QGroundControl does not support loading this complex mission item type: %1").arg(complexType);
        _clear();
        return false;
    }

    setSequenceNumber(complexObject[_jsonIdKey].toInt());

    _cameraTrigger =        complexObject[_jsonCameraTriggerKey].toBool();
    _gridAltitudeRelative = complexObject[_jsonGridAltitudeRelativeKey].toBool();

    _gridAltitudeFact.setRawValue           (complexObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue              (complexObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue            (complexObject[_jsonGridSpacingKey].toDouble());
    _cameraTriggerDistanceFact.setRawValue  (complexObject[_jsonCameraTriggerDistanceKey].toDouble());

    // Polygon shape
    QJsonArray polygonArray(complexObject[_jsonPolygonKey].toArray());
    for (int i=0; i<polygonArray.count(); i++) {
        const QJsonValue& pointValue = polygonArray[i];

        QGeoCoordinate pointCoord;
        if (!JsonHelper::toQGeoCoordinate(pointValue, pointCoord, false /* altitudeRequired */, errorString)) {
            _clear();
            return false;
        }
        _polygonPath << QVariant::fromValue(pointCoord);
    }

    _generateGrid();

    return true;
}

void ComplexMissionItem::_setExitCoordinate(const QGeoCoordinate& coordinate)
{
    if (_exitCoordinate != coordinate) {
        _exitCoordinate = coordinate;
        emit exitCoordinateChanged(coordinate);
    }
}

bool ComplexMissionItem::specifiesCoordinate(void) const
{
    return _polygonPath.count() > 2;
}

void ComplexMissionItem::_clearGrid(void)
{
    // Bug workaround
    while (_gridPoints.count() > 1) {
        _gridPoints.takeLast();
    }
    emit gridPointsChanged();
    _gridPoints.clear();
}

void ComplexMissionItem::_generateGrid(void)
{
    if (_polygonPath.count() < 3) {
        _clearGrid();
        return;
    }

    _gridPoints.clear();

    QList<QPointF> polygonPoints;
    QList<QPointF> gridPoints;

    // Convert polygon to Qt coordinate system (y positive is down)
    qCDebug(ComplexMissionItemLog) << "Convert polygon";
    QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
    for (int i=0; i<_polygonPath.count(); i++) {
        double y, x, down;
        convertGeoToNed(_polygonPath[i].value<QGeoCoordinate>(), tangentOrigin, &y, &x, &down);
        polygonPoints += QPointF(x, -y);
        qCDebug(ComplexMissionItemLog) << _polygonPath[i].value<QGeoCoordinate>() << polygonPoints.last().x() << polygonPoints.last().y();
    }

    // Generate grid
    _gridGenerator(polygonPoints, gridPoints);

    // Convert to Geo and set altitude
    for (int i=0; i<gridPoints.count(); i++) {
        QPointF& point = gridPoints[i];

        QGeoCoordinate geoCoord;
        convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &geoCoord);
        _gridPoints += QVariant::fromValue(geoCoord);
    }
    emit gridPointsChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());

    if (_gridPoints.count()) {
        setCoordinate(_gridPoints.first().value<QGeoCoordinate>());
        _setExitCoordinate(_gridPoints.last().value<QGeoCoordinate>());
    }
}

QPointF ComplexMissionItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void ComplexMissionItem::_intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines)
{
    QLineF topLine      (boundRect.topLeft(),       boundRect.topRight());
    QLineF bottomLine   (boundRect.bottomLeft(),    boundRect.bottomRight());
    QLineF leftLine     (boundRect.topLeft(),       boundRect.bottomLeft());
    QLineF rightLine    (boundRect.topRight(),      boundRect.bottomRight());

    for (int i=0; i<lineList.count(); i++) {
        QPointF intersectPoint;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        int foundCount = 0;
        if (line.intersect(topLine, &intersectPoint) == QLineF::BoundedIntersection) {
            intersectLine.setP1(intersectPoint);
            foundCount++;
        }
        if (line.intersect(rightLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(bottomLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(leftLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

void ComplexMissionItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines)
{
    for (int i=0; i<lineList.count(); i++) {
        int foundCount = 0;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        for (int j=0; j<polygon.count()-1; j++) {
            QPointF intersectPoint;
            QLineF polygonLine = QLineF(polygon[j], polygon[j+1]);
            if (line.intersect(polygonLine, &intersectPoint) == QLineF::BoundedIntersection) {
                if (foundCount == 0) {
                    foundCount++;
                    intersectLine.setP1(intersectPoint);
                } else {
                    foundCount++;
                    intersectLine.setP2(intersectPoint);
                    break;
                }
            }
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

void ComplexMissionItem::_gridGenerator(const QList<QPointF>& polygonPoints,  QList<QPointF>& gridPoints)
{
    double gridAngle = _gridAngleFact.rawValue().toDouble();

    gridPoints.clear();

    // Convert polygon to bounding rect

    qCDebug(ComplexMissionItemLog) << "Polygon";
    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        qCDebug(ComplexMissionItemLog) << polygonPoints[i];
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF smallBoundRect = polygon.boundingRect();
    QPointF center = smallBoundRect.center();
    qCDebug(ComplexMissionItemLog) << "Bounding rect" << smallBoundRect.topLeft().x() << smallBoundRect.topLeft().y() << smallBoundRect.bottomRight().x() << smallBoundRect.bottomRight().y();

    // Rotate the bounding rect around it's center to generate the larger bounding rect
    QPolygonF boundPolygon;
    boundPolygon << _rotatePoint(smallBoundRect.topLeft(),       center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.topRight(),      center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomRight(),   center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomLeft(),    center, gridAngle);
    boundPolygon << boundPolygon[0];
    QRectF largeBoundRect = boundPolygon.boundingRect();
    qCDebug(ComplexMissionItemLog) << "Rotated bounding rect" << largeBoundRect.topLeft().x() << largeBoundRect.topLeft().y() << largeBoundRect.bottomRight().x() << largeBoundRect.bottomRight().y();

    // Create set of rotated parallel lines within the expanded bounding rect. Make the lines larger than the
    // bounding box to guarantee intersection.
    QList<QLineF> lineList;
    float x = largeBoundRect.topLeft().x();
    float gridSpacing = _gridSpacingFact.rawValue().toDouble();
    while (x < largeBoundRect.bottomRight().x()) {
        float yTop =    largeBoundRect.topLeft().y() - 100.0;
        float yBottom = largeBoundRect.bottomRight().y() + 100.0;

        lineList += QLineF(_rotatePoint(QPointF(x, yTop), center, gridAngle), _rotatePoint(QPointF(x, yBottom), center, gridAngle));
        qCDebug(ComplexMissionItemLog) << "line" << lineList.last().x1() << lineList.last().y1() << lineList.last().x2() << lineList.last().y2();

        x += gridSpacing;
    }

    // Now intesect the lines with the smaller bounding rect
    QList<QLineF> resultLines;
    //_intersectLinesWithRect(lineList, smallBoundRect, resultLines);
    _intersectLinesWithPolygon(lineList, polygon, resultLines);

    // Turn into a path
    for (int i=0; i<resultLines.count(); i++) {
        const QLineF& line = resultLines[i];

        if (i & 1) {
            gridPoints << line.p2() << line.p1();
        } else {
            gridPoints << line.p1() << line.p2();
        }
    }
}

QmlObjectListModel* ComplexMissionItem::getMissionItems(void) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

    int seqNum = _sequenceNumber;
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate coord = _gridPoints[i].value<QGeoCoordinate>();
        double altitude = _gridAltitudeFact.rawValue().toDouble();

        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                            _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                            0.0, 0.0, 0.0, 0.0,             // param 1-4
                                            coord.latitude(),
                                            coord.longitude(),
                                            altitude,
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);

        if (_cameraTrigger && i == 0) {
            MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                MAV_FRAME_MISSION,              // MAV_FRAME
                                                _cameraTriggerDistanceFact.rawValue().toDouble(),   // trigger distance
                                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 2-7
                                                true,                           // autoContinue
                                                false,                          // isCurrentItem
                                                pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
            pMissionItems->append(item);
        }
    }

    if (_cameraTrigger) {
        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                            MAV_FRAME_MISSION,              // MAV_FRAME
                                            0.0,                            // trigger distance
                                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 2-7
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }

    return pMissionItems;
}

void ComplexMissionItem::_cameraTriggerChanged(void)
{
    setDirty(true);
    if (_gridPoints.count()) {
        // If we have grid turn on/off camera trigger will add/remove two camera trigger mission items
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}
