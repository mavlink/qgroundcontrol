/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SprayComplexItem.h"
#include "JsonHelper.h"
#include "QGCGeo.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanMasterController.h"
#include "MissionItem.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtGui/QPolygonF>
#include <QtCore/QJsonArray>
#include <QtCore/QLineF>

QGC_LOGGING_CATEGORY(SprayComplexItemLog, "SprayComplexItemLog")

const QString SprayComplexItem::name(SprayComplexItem::tr("Spray"));

SprayComplexItem::SprayComplexItem(PlanMasterController* masterController, bool flyView)
    : ComplexMissionItem(masterController, flyView)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Spray.SettingsGroup.json"), this))
    , _speedFact(settingsGroup, _metaDataMap[speedName])
    , _altitudeFact(settingsGroup, _metaDataMap[altitudeName])
    , _sprayWidthFact(settingsGroup, _metaDataMap[sprayWidthName])
{
     // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &SprayComplexItem::_updateFlightPathSegmentsSignal, this, &SprayComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&SprayComplexItem::_updateFlightPathSegmentsSignal));

    _editorQml = "qrc:/qml/QGroundControl/Controls/SprayItemEditor.qml";
    connect(&_altitudeFact,         &Fact::valueChanged,         this,       &SprayComplexItem::_rebuildTransects);
    connect(&_sprayWidthFact,       &Fact::valueChanged,         this,       &SprayComplexItem::_rebuildTransects);
    connect(&_sprayAreaPolygon,     &QGCMapPolygon::pathChanged, this,       &SprayComplexItem::_rebuildTransects);


    connect(&_altitudeFact,         &Fact::valueChanged,         this,       &SprayComplexItem::_setDirty);
    connect(&_sprayWidthFact,       &Fact::valueChanged,         this,       &SprayComplexItem::_setDirty);
    connect(&_speedFact,            &Fact::valueChanged,         this,       &SprayComplexItem::_setDirty);
    connect(&_sprayAreaPolygon,     &QGCMapPolygon::dirtyChanged,this,       &SprayComplexItem::_setIfDirty);

    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::minAMSLAltitudeChanged);
    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::maxAMSLAltitudeChanged);
    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::_amslEntryAltChanged);
    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::_amslExitAltChanged);
    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::complexDistanceChanged);
    connect(this, &SprayComplexItem::visualTransectPointsChanged, this, &SprayComplexItem::greatestDistanceToChanged);
    

    setDirty(false);
}

void SprayComplexItem::_setDirty(void)
{
    setDirty(true);
}
void SprayComplexItem::_setIfDirty(bool dirty)
{
    if(dirty){
        setDirty(true);
    }
}
void SprayComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void SprayComplexItem::applyNewAltitude(double newAltitude)
{
    _altitudeFact.setRawValue(newAltitude);
}

void SprayComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    // Header
    saveObject[JsonHelper::jsonVersionKey] =                    3;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    saveObject[speedName] =      _speedFact.rawValue().toDouble();
    saveObject[altitudeName] =     _altitudeFact.rawValue().toDouble();
    saveObject[sprayWidthName] =   _sprayWidthFact.rawValue().toDouble();


    _sprayAreaPolygon.saveToJson(saveObject);

    missionItems.append(saveObject);
}
bool SprayComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
        { speedName,                                    QJsonValue::Double, true },
        { altitudeName,                                 QJsonValue::Double, true },
        { sprayWidthName,                               QJsonValue::Double, true }
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    _sprayAreaPolygon.clear();

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version != 3) {
        errorString = tr("%1 version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    _speedFact.setRawValue                  (complexObject[speedName].toDouble());
    _altitudeFact.setRawValue               (complexObject[altitudeName].toDouble());
    _sprayWidthFact.setRawValue             (complexObject[sprayWidthName].toDouble());

    if (!_sprayAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _sprayAreaPolygon.clear();
        return false;
    }

    return true;
}

void SprayComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}
int SprayComplexItem::lastSequenceNumber(void) const
{
    int numOfItems = 0;
    numOfItems += _visualTransectPoints.count() ? _visualTransectPoints.count()*2 + 1 : 0; 
    return _sequenceNumber + numOfItems;
}

double SprayComplexItem::minAMSLAltitude(void) const
{
    double minAlt = 9999.0;
    for (int i=0; i<_visualTransectPoints.count(); i++) {
        minAlt = qMin(minAlt, _visualTransectPoints[i].altitude());
    }
    return 0; //figure out how to calc terrain altitude
}
double SprayComplexItem::maxAMSLAltitude(void) const
{
    double maxAlt = -9999.0;
    for (int i=0; i<_visualTransectPoints.count(); i++) {
        maxAlt = qMax(maxAlt, _visualTransectPoints[i].altitude());
    }
    return 0; //figure out how to calc terrain altitude
}
double SprayComplexItem::amslEntryAlt(void) const
{
    return 0; //figure out how to calc terrain altitude
}
double SprayComplexItem::amslExitAlt(void) const
{
    return 0; //figure out how to calc terrain altitude
}

double SprayComplexItem::complexDistance(void) const
{
    double _complexDistance = 0.0;
    for (int i=0; i<_visualTransectPoints.count() - 1; i++) {
        _complexDistance += _visualTransectPoints[i].distanceTo(_visualTransectPoints[i+1]);
    }
    return _complexDistance;
}
double SprayComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    for (int i=0; i<_visualTransectPoints.count(); i++) {
        greatestDistance = qMax(greatestDistance, _visualTransectPoints[i].distanceTo(other));
    }
    return greatestDistance;
}

QGeoCoordinate SprayComplexItem::coordinate(void) const
{
    return _visualTransectPoints.count() ? _visualTransectPoints.first(): QGeoCoordinate();
} 
QGeoCoordinate SprayComplexItem::exitCoordinate(void) const
{
    return _visualTransectPoints.count() ? _visualTransectPoints.last(): QGeoCoordinate();
} 




void SprayComplexItem::_rebuildTransects(void)
{
    if (_sprayAreaPolygon.count() < 3){
        return;
    }

    //Convert polygon to NED
    QList<QPointF> polygonPoints;
    QGeoCoordinate tangentOrigin = _sprayAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(0)->coordinate();
    for (int i=0; i<_sprayAreaPolygon.count(); i++) {
        double y, x, down;
        QGeoCoordinate vertex = _sprayAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i)->coordinate();
        if (i == 0) {
            // This avoids a nan calculation that comes out of convertGeoToNed
            x = y = 0;
        } else {
            QGCGeo::convertGeoToNed(vertex, tangentOrigin, y, x, down);
        }
        polygonPoints += QPointF(x, y);
    }

    double gridSpacing = _sprayWidthFact.rawValue().toDouble();
    if (gridSpacing < 0.5) {
        // We can't let gridSpacing get too small otherwise we will end up with too many transects.
        // So we limit to 0.5 meter spacing as min and set to huge value which will cause a single
        // transect to be added.
        gridSpacing = 100000;
    }

    //Creating bounding rect
    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF boundingRect = polygon.boundingRect();
    QPointF boundingCenter = boundingRect.center();

    // Create set of rotated parallel lines within the expanded bounding rect. Make the lines larger than the
    // bounding box to guarantee intersection.

    QList<QLineF> lineList;

    // Transects are generated to be as long as the largest width/height of the bounding rect plus some fudge factor.
    // This way they will always be guaranteed to intersect with a polygon edge no matter what angle they are rotated to.
    // They are initially generated with the transects flowing from west to east and then points within the transect north to south.
    double maxWidth = qMax(boundingRect.width(), boundingRect.height()) + 2000.0;
    double halfWidth = maxWidth / 2.0;
    double transectX = boundingCenter.x() - halfWidth;
    double transectXMax = transectX + maxWidth;
    while (transectX < transectXMax) {
        double transectYTop = boundingCenter.y() - halfWidth;
        double transectYBottom = boundingCenter.y() + halfWidth;

        lineList += QLineF(QPointF(transectX, transectYTop), QPointF(transectX, transectYBottom));
        transectX += gridSpacing;
    }

    // Now intersect the lines with the polygon
    QList<QLineF> intersectLines;
    _intersectLinesWithPolygon(lineList, polygon, intersectLines);
    // Less than two transects intersected with the polygon:
    //      Create a single transect which goes through the center of the polygon
    //      Intersect it with the polygon
    if (intersectLines.count() < 2) {
        _sprayAreaPolygon.center();
        QLineF firstLine = lineList.first();
        QPointF lineCenter = firstLine.pointAt(0.5);
        QPointF centerOffset = boundingCenter - lineCenter;
        firstLine.translate(centerOffset);
        lineList.clear();
        lineList.append(firstLine);
        intersectLines = lineList;
        _intersectLinesWithPolygon(lineList, polygon, intersectLines);
    }


    // Make sure all lines are going the same direction. Polygon intersection leads to lines which
    // can be in varied directions depending on the order of the intesecting sides.
    QList<QLineF> resultLines;
    _adjustLineDirection(intersectLines, resultLines);

    // Convert from NED to Geo
    QList<QList<QGeoCoordinate>> transects;
    for (const QLineF& line : resultLines) {
        QGeoCoordinate          coord;
        QList<QGeoCoordinate>   transect;

        QGCGeo::convertNedToGeo(line.p1().y(), line.p1().x(), 0, tangentOrigin, coord);
        transect.append(coord);
        QGCGeo::convertNedToGeo(line.p2().y(), line.p2().x(), 0, tangentOrigin, coord);
        transect.append(coord);

        transects.append(transect);
    }

     // Adjust to lawnmower pattern
    bool reverseVertices = false;
    for (int i=0; i<transects.count(); i++) {
        // We must reverse the vertices for every other transect in order to make a lawnmower pattern
        QList<QGeoCoordinate> transectVertices = transects[i];
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
        transects[i] = transectVertices;
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
    for (const QList<QGeoCoordinate>& transect: transects) {
        for (const QGeoCoordinate& coord: transect) {
            _visualTransectPoints.append(coord);
            double lat = coord.latitude()  + 90.0;
            double lon = coord.longitude() + 180.0;
            north   = fmax(north, lat);
            south   = fmin(south, lat);
            east    = fmax(east,  lon);
            west    = fmin(west,  lon);
            bottom  = fmin(bottom, coord.altitude());
            top     = fmax(top, coord.altitude());
        }
    }
    //-- Update bounding cube for airspace management control
    _setBoundingCube(QGCGeoBoundingCube(
                         QGeoCoordinate(north - 90.0, west - 180.0, bottom),
                         QGeoCoordinate(south - 90.0, east - 180.0, top)));




    //necessary signal emission
    emit visualTransectPointsChanged();
    emit coordinateChanged(SprayComplexItem::coordinate());
    emit exitCoordinateChanged(SprayComplexItem::exitCoordinate());
    emit lastSequenceNumberChanged(lastSequenceNumber());

    emit _updateFlightPathSegmentsSignal();

}
void SprayComplexItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines)
{
    resultLines.clear();

    for (int i=0; i<lineList.count(); i++) {
        const QLineF& line = lineList[i];
        QList<QPointF> intersections;

        // Intersect the line with all the polygon edges
        for (int j=0; j<polygon.count()-1; j++) {
            QPointF intersectPoint;
            QLineF polygonLine = QLineF(polygon[j], polygon[j+1]);

            auto intersect = line.intersects(polygonLine, &intersectPoint);
            if (intersect == QLineF::BoundedIntersection) {
                if (!intersections.contains(intersectPoint)) {
                    intersections.append(intersectPoint);
                }
            }
        }

        // We now have one or more intersection points all along the same line. Find the two
        // which are furthest away from each other to form the transect.
        if (intersections.count() > 1) {
            QPointF firstPoint;
            QPointF secondPoint;
            double currentMaxDistance = 0;

            for (int i=0; i<intersections.count(); i++) {
                for (int j=0; j<intersections.count(); j++) {
                    QLineF lineTest(intersections[i], intersections[j]);
                    
                    double newMaxDistance = lineTest.length();
                    if (newMaxDistance > currentMaxDistance) {
                        firstPoint = intersections[i];
                        secondPoint = intersections[j];
                        currentMaxDistance = newMaxDistance;
                    }
                }
            }

            resultLines += QLineF(firstPoint, secondPoint);
        }
    }
}
/// Adjust the line segments such that they are all going the same direction with respect to going from P1->P2
void SprayComplexItem::_adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines)
{
    qreal firstAngle = 0;
    for (int i=0; i<lineList.count(); i++) {
        const QLineF& line = lineList[i];
        QLineF adjustedLine;

        if (i == 0) {
            firstAngle = line.angle();
        }

        if (qAbs(line.angle() - firstAngle) > 1.0) {
            adjustedLine.setP1(line.p2());
            adjustedLine.setP2(line.p1());
        } else {
            adjustedLine = line;
        }

        resultLines += adjustedLine;
    }
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void SprayComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
{
    if (_cTerrainCollisionSegments != 0) {
        _cTerrainCollisionSegments = 0;
        emit terrainCollisionChanged(false);
    }

    _flightPathSegments.beginResetModel();
    _flightPathSegments.clearAndDeleteContents();


    if (_visualTransectPoints.count()) {
        FlightPathSegment::SegmentType segmentType = FlightPathSegment::SegmentTypeTerrainFrame;
        for (int i=0; i<_visualTransectPoints.count() - 1; i++) {
            const QGeoCoordinate& fromCoord = _visualTransectPoints[i];
            const QGeoCoordinate& toCoord   = _visualTransectPoints[i+1];
            _appendFlightPathSegment(segmentType, fromCoord, fromCoord.altitude(), toCoord, toCoord.altitude());
        }
    }

    _flightPathSegments.endResetModel();

    if (_cTerrainCollisionSegments != 0) {
        emit terrainCollisionChanged(true);
    }

    _masterController->missionController()->recalcTerrainProfile();
}

void SprayComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int                         seqNum      = _sequenceNumber;
    MAV_FRAME                   mavFrame    = MAV_FRAME_GLOBAL_TERRAIN_ALT;

    MissionItem* item = nullptr;
    
    for (int coordIndex=0; coordIndex<_visualTransectPoints.count(); coordIndex+=2) {
        QGeoCoordinate sprayEntryCoord = _visualTransectPoints[coordIndex];
        QGeoCoordinate sprayExitCoord = _visualTransectPoints[coordIndex+1];
        item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_WAYPOINT,
                           mavFrame,
                           0,0,0,                                      // No hold time
                           std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                           sprayEntryCoord.latitude(),
                           sprayEntryCoord.longitude(),
                           sprayEntryCoord.altitude(),
                           true,                                       // autoContinue
                           false,                                      // isCurrentItem
                           missionItemParent);
        items.append(item);
        item = new MissionItem(seqNum++,
                           MAV_CMD_DO_SPRAYER,
                           mavFrame,
                           1,                                          //enable spray
                           0,0,0,0,0,0,                                //param 2-7 not used
                           true,                                       // autoContinue
                           false,                                      // isCurrentItem
                           missionItemParent);
        items.append(item);
        item = new MissionItem(seqNum++,
                           MAV_CMD_NAV_WAYPOINT,
                           mavFrame,
                           0,0,0,                                      // No hold time
                           std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                           sprayExitCoord.latitude(),
                           sprayExitCoord.longitude(),
                           sprayExitCoord.altitude(),
                           true,                                       // autoContinue
                           false,                                      // isCurrentItem
                           missionItemParent);
        items.append(item);
        item = new MissionItem(seqNum++,
                           MAV_CMD_DO_SPRAYER,
                           mavFrame,
                           0,                                          //enable spray
                           0,0,0,0,0,0,                                //param 2-7 not used
                           true,                                       // autoContinue
                           false,                                      // isCurrentItem
                           missionItemParent);
        items.append(item);
    }
}

SprayComplexItem::ReadyForSaveState SprayComplexItem::readyForSaveState(void) const
{
    return _sprayAreaPolygon.isValid() && !_wizardMode ? ReadyForSave : NotReadyForSaveData;
}