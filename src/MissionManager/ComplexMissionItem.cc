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

const char* ComplexMissionItem::_jsonVersionKey =       "version";
const char* ComplexMissionItem::_jsonTypeKey =          "type";
const char* ComplexMissionItem::_jsonPolygonKey =       "polygon";
const char* ComplexMissionItem::_jsonIdKey =            "id";
const char* ComplexMissionItem::_jsonGridAltitudeKey =  "gridAltitude";
const char* ComplexMissionItem::_jsonGridAngleKey =     "gridAngle";
const char* ComplexMissionItem::_jsonGridSpacingKey =   "gridSpacing";

const char* ComplexMissionItem::_complexType = "survey";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
    , _gridAltitudeFact (0, "Altitude:",        FactMetaData::valueTypeDouble)
    , _gridAngleFact    (0, "Grid angle:",      FactMetaData::valueTypeDouble)
    , _gridSpacingFact  (0, "Grid spacing:",    FactMetaData::valueTypeDouble)
{
    _gridAltitudeFact.setRawValue(25);
    _gridSpacingFact.setRawValue(5);

    connect(&_gridSpacingFact,  &Fact::valueChanged, this, &ComplexMissionItem::_generateGrid);
    connect(&_gridAngleFact,    &Fact::valueChanged, this, &ComplexMissionItem::_generateGrid);
}

ComplexMissionItem::ComplexMissionItem(const ComplexMissionItem& other, QObject* parent)
    : VisualMissionItem(other, parent)
    , _sequenceNumber(other.sequenceNumber())
    , _dirty(false)
{

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

    emit specifiesCoordinateChanged();
}

void ComplexMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();

    int pointCount = _polygonPath.count();
    if (pointCount == 1) {
        setCoordinate(coordinate);
    } else if (pointCount == 3) {
        emit specifiesCoordinateChanged();
    }
    _setExitCoordinate(coordinate);

    _generateGrid();
}

int ComplexMissionItem::lastSequenceNumber(void) const
{
    int lastSeq = _sequenceNumber;

    if (_gridPoints.count()) {
        lastSeq += _gridPoints.count() - 1;
    }
    return lastSeq;
}

void ComplexMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
        _setExitCoordinate(coordinate);
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
    saveObject[_jsonVersionKey] =       1;
    saveObject[_jsonTypeKey] =          _complexType;
    saveObject[_jsonIdKey] =            sequenceNumber();
    saveObject[_jsonGridAltitudeKey] =  _gridAltitudeFact.rawValue().toDouble();
    saveObject[_jsonGridAngleKey] =     _gridAngleFact.rawValue().toDouble();
    saveObject[_jsonGridSpacingKey] =   _gridSpacingFact.rawValue().toDouble();

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
    requiredKeys << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << _jsonGridAltitudeKey << _jsonGridAngleKey << _jsonGridSpacingKey;
    if (!JsonHelper::validateRequiredKeys(complexObject, requiredKeys, errorString)) {
        _clear();
        return false;
    }

    // Validate types
    QStringList keyList;
    QList<QJsonValue::Type> typeList;
    keyList << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << _jsonGridAltitudeKey << _jsonGridAngleKey << _jsonGridSpacingKey;
    typeList << QJsonValue::Double << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Double << QJsonValue::Double<< QJsonValue::Double;
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
    _gridAltitudeFact.setRawValue(complexObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue(complexObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue(complexObject[_jsonGridSpacingKey].toDouble());

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
    emit gridPointsChanged();

}

void ComplexMissionItem::_generateGrid(void)
{
    if (_polygonPath.count() < 3) {
        _clearGrid();
        return;
    }

    _gridPoints.clear();

    QList<Point_t> polygonPoints;
    QList<Point_t> gridPoints;

    // Convert polygon to NED
    qDebug() << "Convert polygon";
    QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
    for (int i=0; i<_polygonPath.count(); i++) {
        double x, y, z;
        convertGeoToNed(_polygonPath[i].value<QGeoCoordinate>(), tangentOrigin, &x, &y, &z);
        polygonPoints += Point_t(x, y);
        qDebug() << _polygonPath[i].value<QGeoCoordinate>() << polygonPoints.last().x << polygonPoints.last().y;
    }

    // Generate grid
    _gridGenerator(polygonPoints, gridPoints);

    // Convert to Geo and set altitude
    for (int i=0; i<gridPoints.count(); i++) {
        Point_t& point = gridPoints[i];

        QGeoCoordinate geoCoord;
        convertNedToGeo(point.x, point.y, 0, tangentOrigin, &geoCoord);
        _gridPoints += QVariant::fromValue(geoCoord);
    }
    emit gridPointsChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());

    if (_gridPoints.count()) {
        setCoordinate(_gridPoints.first().value<QGeoCoordinate>());
        _setExitCoordinate(_gridPoints.last().value<QGeoCoordinate>());
    }

}

void ComplexMissionItem::_gridGenerator(const QList<Point_t>& polygonPoints, QList<Point_t>& gridPoints)
{
    // FIXME: Hack implementataion

    gridPoints.clear();

    // Convert polygon to bounding square

    Point_t upperLeft = polygonPoints[0];
    Point_t lowerRight = polygonPoints[0];
    for (int i=1; i<polygonPoints.count(); i++) {
        const Point_t& point = polygonPoints[i];

        upperLeft.x = std::min(upperLeft.x, point.x);
        upperLeft.y = std::max(upperLeft.y, point.y);
        lowerRight.x = std::max(lowerRight.x, point.x);
        lowerRight.y = std::min(lowerRight.y, point.y);
    }
    qDebug() << "bounding rect" << upperLeft.x <<  upperLeft.y << lowerRight.x << lowerRight.y;

    // Create simplistic set of parallel lines

    QList<QPair<Point_t, Point_t>> lineList;

    double x = upperLeft.x;
    double gridSpacing = _gridSpacingFact.rawValue().toDouble();
    while (x < lowerRight.x) {
        double yTop = upperLeft.y;
        double yBottom = lowerRight.y;

        lineList += qMakePair(Point_t(x, yTop), Point_t(x, yBottom));
        qDebug() << "line" << lineList.last().first.x<< lineList.last().first.y<< lineList.last().second.x<< lineList.last().second.y;

        x += gridSpacing;
    }

    // Turn into a path

    for (int i=0; i<lineList.count(); i++) {
        const QPair<Point_t, Point_t> points = lineList[i];

        if (i & 1) {
            gridPoints << points.second << points.first;
        } else {
            gridPoints << points.first << points.second;
        }
    }
}

QmlObjectListModel*  ComplexMissionItem::getMissionItems(void) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

    int seqNum = _sequenceNumber;
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate coord = _gridPoints[i].value<QGeoCoordinate>();
        double altitude = _gridAltitudeFact.rawValue().toDouble();

        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                            MAV_FRAME_GLOBAL_RELATIVE_ALT,  // MAV_FRAME
                                            0.0, 0.0, 0.0, 0.0,             // param 1-4
                                            coord.latitude(),
                                            coord.longitude(),
                                            altitude,
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }

    return pMissionItems;
}
