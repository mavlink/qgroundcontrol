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
#include <QtCore/QJsonDocument>
#include <QtCore/QLineF>
#include <QtCore/QtMath>
#include <QtCore/QSet>
#include <QtCore/QVariant>

#include <clipper2/clipper.h>

#include <algorithm>

QGC_LOGGING_CATEGORY(SprayComplexItemLog, "SprayComplexItemLog")

const QString SprayComplexItem::name(SprayComplexItem::tr("Spray"));

// --- Clipper2 helpers (NED coordinates: x = East, y = North) ---
namespace {
    Clipper2Lib::PathD polygonToPathD(const QPolygonF& poly)
    {
        Clipper2Lib::PathD path;
        const int n = poly.count();
        for (int i = 0; i < n; i++) {
            if (i == n - 1 && qAbs(poly[i].x() - poly[0].x()) < 1e-9 && qAbs(poly[i].y() - poly[0].y()) < 1e-9)
                continue; // skip duplicate closing point
            path.push_back(Clipper2Lib::PointD(poly[i].x(), poly[i].y()));
        }
        return path;
    }

    QPolygonF pathDToPolygon(const Clipper2Lib::PathD& path)
    {
        QPolygonF poly;
        for (const auto& pt : path)
            poly << QPointF(pt.x, pt.y);
        if (poly.count() > 1 && (poly.last() - poly.first()).manhattanLength() > 1e-9)
            poly << poly.first();
        return poly;
    }

    void allowedPathsToOuterAndHoles(const Clipper2Lib::PathsD& paths, QPolygonF& outer, QList<QPolygonF>& holes)
    {
        outer = QPolygonF();
        holes.clear();
        if (paths.empty()) return;
        // Outer = path with largest absolute area (field boundary). Rest = holes (obstacle boundaries).
        // Clipper2 / NED axis can flip sign of area, so we use absolute area to find the outer.
        int outerIdx = 0;
        double maxAbsArea = 0;
        for (size_t i = 0; i < paths.size(); i++) {
            double a = qAbs(Clipper2Lib::Area(paths[i]));
            if (a > maxAbsArea) {
                maxAbsArea = a;
                outerIdx = static_cast<int>(i);
            }
        }
        outer = pathDToPolygon(paths[static_cast<size_t>(outerIdx)]);
        for (size_t i = 0; i < paths.size(); i++) {
            if (static_cast<int>(i) == outerIdx) continue;
            holes.append(pathDToPolygon(paths[i]));
        }
    }

    void ensurePositiveArea(Clipper2Lib::PathD& path)
    {
        if (path.size() < 3) return;
        if (Clipper2Lib::Area(path) < 0)
            std::reverse(path.begin(), path.end());
    }
}


SprayComplexItem::SprayComplexItem(PlanMasterController* masterController, bool flyView)
    : ComplexMissionItem(masterController, flyView)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Spray.SettingsGroup.json"), this))
    , _speedFact(settingsGroup, _metaDataMap[speedName])
    , _altitudeFact(settingsGroup, _metaDataMap[altitudeName])
    , _sprayWidthFact(settingsGroup, _metaDataMap[sprayWidthName])
    , _gridAngleFact(settingsGroup, _metaDataMap[gridAngleName])
    , _turnAroundDistanceFact(settingsGroup, _metaDataMap[turnAroundDistanceName])
    , _obstacleBufferFact(settingsGroup, _metaDataMap[obstacleBufferName])
    , _obstaclePolygonsModel(new QmlObjectListModel(this))
{
     // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &SprayComplexItem::_updateFlightPathSegmentsSignal, this, &SprayComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&SprayComplexItem::_updateFlightPathSegmentsSignal));

    _editorQml = "qrc:/qml/QGroundControl/Controls/SprayItemEditor.qml";
    connect(&_altitudeFact,             &Fact::valueChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(&_sprayWidthFact,           &Fact::valueChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(&_gridAngleFact,            &Fact::valueChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(&_turnAroundDistanceFact,   &Fact::valueChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(&_obstacleBufferFact,      &Fact::valueChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(&_sprayAreaPolygon,        &QGCMapPolygon::pathChanged, this, &SprayComplexItem::_rebuildTransects);

    connect(&_altitudeFact,             &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_sprayWidthFact,           &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_gridAngleFact,            &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_turnAroundDistanceFact,   &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_obstacleBufferFact,       &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_speedFact,                &Fact::valueChanged, this, &SprayComplexItem::_setDirty);
    connect(&_sprayAreaPolygon,         &QGCMapPolygon::dirtyChanged, this, &SprayComplexItem::_setIfDirty);

    connect(&_speedFact,            &Fact::valueChanged,                    this,   &SprayComplexItem::specifiedFlightSpeedChanged);

    connect(&_sprayAreaPolygon,     &QGCMapPolygon::isValidChanged,         this,   &SprayComplexItem::_updateWizardMode);
    connect(&_sprayAreaPolygon,     &QGCMapPolygon::traceModeChanged,       this,   &SprayComplexItem::_updateWizardMode);

    connect(&_sprayAreaPolygon,     &QGCMapPolygon::isValidChanged,         this,   &SprayComplexItem::readyForSaveStateChanged);
    connect(this,                   &SprayComplexItem::wizardModeChanged,   this,   &SprayComplexItem::readyForSaveStateChanged);
    connect(_obstaclePolygonsModel,  &QmlObjectListModel::countChanged,     this,   &SprayComplexItem::_rebuildTransects);

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
void SprayComplexItem::_updateWizardMode(void)
{
    if (_sprayAreaPolygon.isValid() && !_sprayAreaPolygon.traceMode()) {
        setWizardMode(false);
    }
}

void SprayComplexItem::addObstaclePolygon(void)
{
    if (!_sprayAreaPolygon.isValid() || _sprayAreaPolygon.count() < 3) {
        return;
    }
    QGCMapPolygon* obst = new QGCMapPolygon(this);
    const qreal halfSideMeters = 10.0;  // 20x20 m square
    const qreal halfDiag = halfSideMeters * qSqrt(2.0);
    QGeoCoordinate cen = _sprayAreaPolygon.center();
    QList<QGeoCoordinate> coords;
    coords << cen.atDistanceAndAzimuth(halfDiag, 135)   // NW
           << cen.atDistanceAndAzimuth(halfDiag, 45)     // NE
           << cen.atDistanceAndAzimuth(halfDiag, 315)    // SE
           << cen.atDistanceAndAzimuth(halfDiag, 225);   // SW
    obst->appendVertices(coords);
    connect(obst, &QGCMapPolygon::pathChanged, this, &SprayComplexItem::_rebuildTransects);
    connect(obst, &QGCMapPolygon::dirtyChanged, this, &SprayComplexItem::_setIfDirty);
    _obstaclePolygonsModel->append(obst);
    setDirty(true);
}

void SprayComplexItem::addObstaclePolygonFromCoordinates(const QVariantList& coordinates)
{
    QGCMapPolygon* obst = new QGCMapPolygon(this);
    for (const QVariant& v : coordinates) {
        if (v.canConvert<QGeoCoordinate>()) {
            obst->appendVertex(v.value<QGeoCoordinate>());
        }
    }
    if (obst->count() >= 3) {
        connect(obst, &QGCMapPolygon::pathChanged, this, &SprayComplexItem::_rebuildTransects);
        connect(obst, &QGCMapPolygon::dirtyChanged, this, &SprayComplexItem::_setIfDirty);
        _obstaclePolygonsModel->append(obst);
        setDirty(true);
    } else {
        obst->deleteLater();
    }
}

void SprayComplexItem::removeObstaclePolygon(int index)
{
    if (index >= 0 && index < _obstaclePolygonsModel->count()) {
        QObject* obj = _obstaclePolygonsModel->removeAt(index);
        if (obj) {
            obj->deleteLater();
        }
        setDirty(true);
    }
}

QVariantList SprayComplexItem::obstacleBufferPolygons(void) const
{
    QVariantList result;
    for (const QList<QGeoCoordinate>& poly : _obstacleBufferPolygons) {
        QVariantList path;
        for (const QGeoCoordinate& c : poly) {
            path.append(QVariant::fromValue(c));
        }
        result.append(QVariant::fromValue(path));
    }
    return result;
}

void SprayComplexItem::setGridEntryLocation(int loc)
{
    int v = qBound(0, loc, 3);
    if (_entryPoint != v) {
        _entryPoint = v;
        emit gridEntryLocationChanged();
        _rebuildTransects();
        setDirty(true);
    }
}

void SprayComplexItem::rotateEntryPoint(void)
{
    _entryPoint = (_entryPoint + 1) % 4;
    emit gridEntryLocationChanged();
    _rebuildTransects();
    setDirty(true);
}

void SprayComplexItem::_reverseTransectOrder(QList<QList<QGeoCoordinate>>& transects, QList<QList<int>>& segmentStartsPerTransect)
{
    if (transects.isEmpty()) return;
    for (int i = 0, j = transects.count() - 1; i < j; i++, j--) {
        transects.swapItemsAt(i, j);
        segmentStartsPerTransect.swapItemsAt(i, j);
    }
}

void SprayComplexItem::_reverseInternalTransectPoints(QList<QList<QGeoCoordinate>>& transects, QList<QList<int>>& segmentStartsPerTransect)
{
    for (int i = 0; i < transects.count(); i++) {
        QList<QGeoCoordinate>& t = transects[i];
        QList<int>& seg = segmentStartsPerTransect[i];
        int n = t.count();
        for (int a = 0, b = n - 1; a < b; a++, b--) {
            t.swapItemsAt(a, b);
        }
        QList<int> newSeg;
        for (int k = seg.count() - 1; k >= 0; k--) {
            newSeg << (n - 1 - seg[k]);
        }
        seg = newSeg;
    }
}

void SprayComplexItem::_adjustTransectsToEntryPointLocation(QList<QList<QGeoCoordinate>>& transects, QList<QList<int>>& segmentStartsPerTransect)
{
    if (transects.isEmpty()) return;
    bool reversePoints = (_entryPoint == EntryLocationBottomLeft || _entryPoint == EntryLocationBottomRight);
    bool reverseTransects = (_entryPoint == EntryLocationTopRight || _entryPoint == EntryLocationBottomRight);
    if (reversePoints) {
        _reverseInternalTransectPoints(transects, segmentStartsPerTransect);
    }
    if (reverseTransects) {
        _reverseTransectOrder(transects, segmentStartsPerTransect);
    }
}


void SprayComplexItem::applyNewAltitude(double newAltitude)
{
    _altitudeFact.setRawValue(newAltitude);
}

double SprayComplexItem::specifiedFlightSpeed(){
    return _speedFact.rawValue().toDouble();
}

void SprayComplexItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    // Header
    saveObject[JsonHelper::jsonVersionKey] =                    3;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    saveObject[speedName] =               _speedFact.rawValue().toDouble();
    saveObject[altitudeName] =            _altitudeFact.rawValue().toDouble();
    saveObject[sprayWidthName] =          _sprayWidthFact.rawValue().toDouble();
    saveObject[gridAngleName] =           _gridAngleFact.rawValue().toDouble();
    saveObject[turnAroundDistanceName] =  _turnAroundDistanceFact.rawValue().toDouble();
    saveObject[obstacleBufferName] =      _obstacleBufferFact.rawValue().toDouble();
    saveObject[gridEntryLocationName] =   _entryPoint;

    _sprayAreaPolygon.saveToJson(saveObject);

    QJsonArray obstaclesArray;
    for (int i = 0; i < _obstaclePolygonsModel->count(); i++) {
        QGCMapPolygon* obst = _obstaclePolygonsModel->value<QGCMapPolygon*>(i);
        if (obst && obst->count() >= 3) {
            QJsonObject obstObj;
            obst->saveToJson(obstObj);
            obstaclesArray.append(obstObj);
        }
    }
    saveObject[_jsonObstaclesKey] = obstaclesArray;

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
        { sprayWidthName,                               QJsonValue::Double, true },
        { gridAngleName,                                QJsonValue::Double, false },
        { turnAroundDistanceName,                       QJsonValue::Double, false }
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
    _gridAngleFact.setRawValue              (complexObject.contains(gridAngleName) ? complexObject[gridAngleName].toDouble() : 0.0);
    _turnAroundDistanceFact.setRawValue     (complexObject.contains(turnAroundDistanceName) ? complexObject[turnAroundDistanceName].toDouble() : 30.0);
    _obstacleBufferFact.setRawValue         (complexObject.contains(obstacleBufferName) ? complexObject[obstacleBufferName].toDouble() : 1.5);
    _entryPoint = complexObject.contains(gridEntryLocationName) ? qBound(0, complexObject[gridEntryLocationName].toInt(), 3) : EntryLocationTopLeft;

    _obstaclePolygonsModel->clearAndDeleteContents();
    if (complexObject.contains(_jsonObstaclesKey) && complexObject[_jsonObstaclesKey].isArray()) {
        QJsonArray arr = complexObject[_jsonObstaclesKey].toArray();
        for (const QJsonValueRef& val : arr) {
            if (val.isObject()) {
                QGCMapPolygon* obst = new QGCMapPolygon(this);
                QString err;
                if (obst->loadFromJson(val.toObject(), false, err)) {
                    connect(obst, &QGCMapPolygon::pathChanged, this, &SprayComplexItem::_rebuildTransects);
                    _obstaclePolygonsModel->append(obst);
                } else {
                    obst->deleteLater();
                }
            }
        }
    }

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
    if (_visualTransectPoints.isEmpty()) {
        return _sequenceNumber + 2;  // takeoff + RTL
    }
    int nWaypoints = _visualTransectPoints.count();
    int nSpraySegments = _spraySegmentStartIndices.count();
    return _sequenceNumber + 2 + nWaypoints + nSpraySegments * 2;  // 2 = takeoff + RTL; each segment adds spray on + spray off
}

double SprayComplexItem::minAMSLAltitude(void) const
{
    if (_visualTransectPoints.isEmpty()) {
        return qQNaN();
    }
    double minAlt = _visualTransectPoints[0].altitude();
    for (int i=1; i<_visualTransectPoints.count(); i++) {
        minAlt = qMin(minAlt, _visualTransectPoints[i].altitude());
    }
    return minAlt;
}
double SprayComplexItem::maxAMSLAltitude(void) const
{
    if (_visualTransectPoints.isEmpty()) {
        return qQNaN();
    }
    double maxAlt = _visualTransectPoints[0].altitude();
    for (int i=1; i<_visualTransectPoints.count(); i++) {
        maxAlt = qMax(maxAlt, _visualTransectPoints[i].altitude());
    }
    return maxAlt;
}
double SprayComplexItem::amslEntryAlt(void) const
{
    if (_visualTransectPoints.isEmpty()) {
        return qQNaN();
    }
    return _visualTransectPoints[0].altitude();
}
double SprayComplexItem::amslExitAlt(void) const
{
    if (_visualTransectPoints.isEmpty()) {
        return qQNaN();
    }
    return _visualTransectPoints[_visualTransectPoints.size()-1].altitude();
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
    if (_ignoreRecalc) {
        return;
    }

    if (_sprayAreaPolygon.count() < 3){
        return;
    }

    _ignoreRecalc = true;

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

    // Apply grid angle (transect direction). Same logic as Survey.
    double gridAngle = _gridAngleFact.rawValue().toDouble();

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

        lineList += QLineF(_rotatePoint(QPointF(transectX, transectYTop), boundingCenter, gridAngle),
                           _rotatePoint(QPointF(transectX, transectYBottom), boundingCenter, gridAngle));
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

    // Build obstacle polygons in NED (original, for boundary paths)
    QList<QPolygonF> obstaclesNed;
    for (int o = 0; o < _obstaclePolygonsModel->count(); o++) {
        QGCMapPolygon* obst = _obstaclePolygonsModel->value<QGCMapPolygon*>(o);
        if (!obst || obst->count() < 3) continue;
        QPolygonF obstPoly;
        for (int i = 0; i < obst->count(); i++) {
            double y, x, down;
            QGeoCoordinate c = obst->pathModel().value<QGCQGeoCoordinate*>(i)->coordinate();
            QGCGeo::convertGeoToNed(c, tangentOrigin, y, x, down);
            obstPoly << QPointF(x, y);
        }
        obstPoly << obstPoly.first();
        obstaclesNed.append(obstPoly);
    }

    // Allowed area for clipping: field minus (union of obstacles inflated by buffer), via Clipper2
    QPolygonF allowedOuter;
    QList<QPolygonF> allowedHoles;
    _obstacleBufferPolygons.clear();
    if (!obstaclesNed.isEmpty()) {
        qreal margin = qMax(0.0, _obstacleBufferFact.rawValue().toDouble());
        const int decimalPrec = 2;
        Clipper2Lib::PathD fieldPath = polygonToPathD(polygon);
        ensurePositiveArea(fieldPath);
        Clipper2Lib::PathsD expandedObstacles;
        for (const QPolygonF& obst : obstaclesNed) {
            Clipper2Lib::PathD p = polygonToPathD(obst);
            if (p.size() < 3) continue;
            Clipper2Lib::PathsD inflated = Clipper2Lib::InflatePaths(Clipper2Lib::PathsD{p}, margin,
                Clipper2Lib::JoinType::Round, Clipper2Lib::EndType::Polygon, 2.0, decimalPrec);
            for (auto path : inflated) {
                ensurePositiveArea(path);
                expandedObstacles.push_back(path);
            }
        }
        // Convert expanded (buffered) obstacles to geo for map display
        for (const Clipper2Lib::PathD& path : expandedObstacles) {
            if (path.size() < 3) continue;
            QList<QGeoCoordinate> poly;
            for (size_t i = 0; i < path.size(); i++) {
                QGeoCoordinate coord;
                QGCGeo::convertNedToGeo(path[i].y, path[i].x, 0, tangentOrigin, coord);
                poly.append(coord);
            }
            _obstacleBufferPolygons.append(poly);
        }
        Clipper2Lib::PathsD unionObstacles;
        if (!expandedObstacles.empty()) {
            unionObstacles.push_back(expandedObstacles[0]);
            for (size_t i = 1; i < expandedObstacles.size(); i++) {
                Clipper2Lib::PathsD other(1, expandedObstacles[i]);
                unionObstacles = Clipper2Lib::Union(unionObstacles, other, Clipper2Lib::FillRule::NonZero, decimalPrec);
            }
        }
        Clipper2Lib::PathsD fieldPaths(1, fieldPath);
        Clipper2Lib::PathsD allowedPaths = Clipper2Lib::Difference(fieldPaths, unionObstacles, Clipper2Lib::FillRule::NonZero, decimalPrec);
        allowedPathsToOuterAndHoles(allowedPaths, allowedOuter, allowedHoles);
    }
    emit obstacleBufferPolygonsChanged();

    // Convert from NED to Geo; if obstacles exist, clip each line with Clipper2-built allowed area and add boundary paths
    QList<QList<QGeoCoordinate>> transects;
    QList<QList<int>> segmentStartsPerTransect;
    for (const QLineF& line : resultLines) {
        QList<QGeoCoordinate> transect;
        QList<int> segStarts;
        if (obstaclesNed.isEmpty()) {
            QGeoCoordinate coord;
            QGCGeo::convertNedToGeo(line.p1().y(), line.p1().x(), 0, tangentOrigin, coord);
            transect.append(coord);
            QGCGeo::convertNedToGeo(line.p2().y(), line.p2().x(), 0, tangentOrigin, coord);
            transect.append(coord);
            segStarts << 0;
        } else {
            QList<QLineF> segments = _clipLineByAllowedArea(line, allowedOuter, allowedHoles);
            if (segments.isEmpty()) continue;
            int pointIndex = 0;
            for (int s = 0; s < segments.size(); s++) {
                segStarts << pointIndex;
                QGeoCoordinate c1, c2;
                QGCGeo::convertNedToGeo(segments[s].p1().y(), segments[s].p1().x(), 0, tangentOrigin, c1);
                QGCGeo::convertNedToGeo(segments[s].p2().y(), segments[s].p2().x(), 0, tangentOrigin, c2);
                transect.append(c1);
                transect.append(c2);
                pointIndex += 2;
                if (s + 1 < segments.size()) {
                    int obstIdx;
                    QPointF cross1, cross2;
                    if (_findObstacleCrossing(segments[s].p2(), segments[s + 1].p1(), obstaclesNed, obstIdx, cross1, cross2)) {
                        QList<QPointF> boundary = _boundaryPathBetweenPoints(cross1, cross2, obstaclesNed[obstIdx], segments[s + 1].p1());
                        for (const QPointF& pt : boundary) {
                            QGeoCoordinate gc;
                            QGCGeo::convertNedToGeo(pt.y(), pt.x(), 0, tangentOrigin, gc);
                            transect.append(gc);
                            pointIndex++;
                        }
                    }
                }
            }
        }
        transects.append(transect);
        segmentStartsPerTransect.append(segStarts);
    }

     // Adjust to lawnmower pattern (reverse every other transect and remap segment starts)
    bool reverseVertices = false;
    for (int i = 0; i < transects.count(); i++) {
        QList<QGeoCoordinate>& transectVertices = transects[i];
        QList<int>& segStarts = segmentStartsPerTransect[i];
        if (reverseVertices) {
            reverseVertices = false;
            QList<QGeoCoordinate> reversedVertices;
            for (int j = transectVertices.count() - 1; j >= 0; j--) {
                reversedVertices.append(transectVertices[j]);
            }
            transectVertices = reversedVertices;
            QList<int> newStarts;
            for (int k = segStarts.count() - 1; k >= 0; k--) {
                newStarts << (transectVertices.count() - 1 - segStarts[k]);
            }
            segStarts = newStarts;
        } else {
            reverseVertices = true;
        }
    }

    // Apply entry point (transect direction) like Survey
    _adjustTransectsToEntryPointLocation(transects, segmentStartsPerTransect);

    // Optionally add turnaround points at each transect end (for large agro drones)
    double turnDist = _turnAroundDistance();
    double sprayAlt = _altitudeFact.rawValue().toDouble();
    if (turnDist > 0) {
        QList<QList<QGeoCoordinate>> transectsWithTurnaround;
        for (int i = 0; i < transects.count(); i++) {
            const QList<QGeoCoordinate>& transect = transects[i];
            QList<QGeoCoordinate> extended;
            double azimuth = transect[0].azimuthTo(transect[1]);
            QGeoCoordinate turnStart = transect[0].atDistanceAndAzimuth(-turnDist, azimuth);
            turnStart.setAltitude(sprayAlt);
            extended.append(turnStart);
            for (const QGeoCoordinate& c : transect) {
                QGeoCoordinate copy = c;
                copy.setAltitude(sprayAlt);
                extended.append(copy);
            }
            double azimuthBack = transect.last().azimuthTo(transect[transect.count() - 2]);
            QGeoCoordinate turnEnd = transect.last().atDistanceAndAzimuth(-turnDist, azimuthBack);
            turnEnd.setAltitude(sprayAlt);
            extended.append(turnEnd);
            transectsWithTurnaround.append(extended);
            for (int& idx : segmentStartsPerTransect[i])
                idx += 1;
        }
        transects = transectsWithTurnaround;
    } else {
        for (QList<QGeoCoordinate>& transect : transects) {
            for (QGeoCoordinate& c : transect) {
                c.setAltitude(sprayAlt);
            }
        }
    }

    // Flatten to _visualTransectPoints and _spraySegmentStartIndices
    _visualTransectPoints.clear();
    _spraySegmentStartIndices.clear();
    double north = 0.0, south = 180.0, east = 0.0, west = 360.0, bottom = 100000., top = 0.;
    for (int i = 0; i < transects.count(); i++) {
        const QList<QGeoCoordinate>& transect = transects[i];
        const QList<int>& segStarts = segmentStartsPerTransect[i];
        for (int j = 0; j < transect.count(); j++) {
            QGeoCoordinate coord = transect[j];
            if (qIsNaN(coord.altitude()))
                coord.setAltitude(sprayAlt);
            if (segStarts.contains(j))
                _spraySegmentStartIndices.append(_visualTransectPoints.count());
            _visualTransectPoints.append(coord);
            double lat = coord.latitude() + 90.0, lon = coord.longitude() + 180.0;
            north = qMax(north, lat);
            south = qMin(south, lat);
            east = qMax(east, lon);
            west = qMin(west, lon);
            if (!qIsNaN(coord.altitude())) {
                bottom = qMin(bottom, coord.altitude());
                top = qMax(top, coord.altitude());
            }
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

    _ignoreRecalc = false;
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
QPointF SprayComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * -angle;
    rotated.setX(((point.x() - origin.x()) * qCos(radians)) - ((point.y() - origin.y()) * qSin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * qSin(radians)) + ((point.y() - origin.y()) * qCos(radians)) + origin.y());
    return rotated;
}

bool SprayComplexItem::_hasTurnaround(void) const
{
    return _turnAroundDistance() > 0;
}

double SprayComplexItem::_turnAroundDistance(void) const
{
    return _turnAroundDistanceFact.rawValue().toDouble();
}

QList<QLineF> SprayComplexItem::_clipLineByAllowedArea(const QLineF& line, const QPolygonF& outer, const QList<QPolygonF>& holes) const
{
    QList<QLineF> result;
    if (outer.isEmpty()) return result;
    const qreal len = line.length();
    if (len < 1e-9) return result;

    QList<qreal> tValues;
    tValues << 0.0 << 1.0;

    auto addIntersections = [&line, len, &tValues](const QPolygonF& poly) {
        for (int i = 0; i < poly.count() - 1; i++) {
            QPointF isect;
            QLineF edge(poly[i], poly[i + 1]);
            if (line.intersects(edge, &isect) == QLineF::BoundedIntersection) {
                qreal t = QLineF(line.p1(), isect).length() / len;
                if (t >= -1e-9 && t <= 1.0 + 1e-9) {
                    t = qBound(0.0, t, 1.0);
                    bool found = false;
                    for (qreal tv : tValues) { if (qAbs(tv - t) < 1e-6) { found = true; break; } }
                    if (!found) tValues.append(t);
                }
            }
        }
    };

    addIntersections(outer);
    for (const QPolygonF& hole : holes)
        addIntersections(hole);

    std::sort(tValues.begin(), tValues.end());
    const qreal eps = 1e-6;
    for (int i = 0; i < tValues.size() - 1; i++) {
        if (tValues[i + 1] - tValues[i] < eps)
            continue;
        qreal tMid = (tValues[i] + tValues[i + 1]) * 0.5;
        QPointF mid = line.pointAt(qBound(0.0, tMid, 1.0));
        if (!outer.containsPoint(mid, Qt::OddEvenFill))
            continue;
        bool insideHole = false;
        for (const QPolygonF& hole : holes) {
            if (hole.count() >= 3 && hole.containsPoint(mid, Qt::OddEvenFill)) {
                insideHole = true;
                break;
            }
        }
        if (!insideHole)
            result.append(QLineF(line.pointAt(tValues[i]), line.pointAt(tValues[i + 1])));
    }
    return result;
}

bool SprayComplexItem::_findObstacleCrossing(const QPointF& p1, const QPointF& p2, const QList<QPolygonF>& obstaclesNed, int& obstacleIndex, QPointF& cross1, QPointF& cross2) const
{
    qreal segLen = QLineF(p1, p2).length();
    qreal extend = (segLen < 1e-6) ? 2000.0 : 2000.0;
    QPointF dir = (segLen > 1e-6) ? (p2 - p1) / segLen : QPointF(1, 0);
    QPointF a = p1 - dir * extend;
    QPointF b = p2 + dir * extend;
    QLineF longLine(a, b);
    for (int oi = 0; oi < obstaclesNed.size(); oi++) {
        const QPolygonF& poly = obstaclesNed[oi];
        QList<QPointF> hits;
        for (int i = 0; i < poly.count() - 1; i++) {
            QPointF isect;
            if (longLine.intersects(QLineF(poly[i], poly[i + 1]), &isect) == QLineF::BoundedIntersection) {
                bool dup = false;
                for (const QPointF& h : hits) {
                    if (QLineF(h, isect).length() < 1e-4) { dup = true; break; }
                }
                if (!dup) hits.append(isect);
            }
        }
        if (hits.size() >= 2) {
            std::sort(hits.begin(), hits.end(), [&p1](const QPointF& u, const QPointF& v) {
                return QLineF(p1, u).length() < QLineF(p1, v).length();
            });
            obstacleIndex = oi;
            cross1 = hits.first();
            cross2 = hits.last();
            return true;
        }
    }
    return false;
}

QList<QPointF> SprayComplexItem::_boundaryPathBetweenPoints(const QPointF& p1, const QPointF& p2, const QPolygonF& polygon, const QPointF& targetHint) const
{
    QList<QPointF> path;
    const int n = polygon.count() - 1;
    if (n < 2) return path;
    auto distToPoint = [](const QPointF& a, const QPointF& b) { return QLineF(a, b).length(); };
    // Find the edge that contains p1 (cross1) and the edge that contains p2 (cross2).
    // p1/p2 lie on the obstacle boundary (line-polygon intersection), so they lie on some edge.
    int e1 = -1, e2 = -1;
    const qreal eps = 1e-4;
    for (int i = 0; i < n; i++) {
        QLineF edge(polygon[i], polygon[i + 1]);
        qreal len = edge.length();
        if (len < 1e-9) continue;
        // Point on segment if dist(a,p)+dist(p,b) <= dist(a,b)+tolerance
        if (QLineF(polygon[i], p1).length() + QLineF(p1, polygon[i + 1]).length() <= len + eps)
            e1 = i;
        if (QLineF(polygon[i], p2).length() + QLineF(p2, polygon[i + 1]).length() <= len + eps)
            e2 = i;
    }
    if (e1 < 0 || e2 < 0) return path;
    // Path must be p1 -> [vertices along arc] -> p2 so we don't jump to obstacle vertices.
    path.append(p1);
    if (e1 == e2) {
        // Both crossings on the same edge: no vertices between.
        path.append(p2);
        return path;
    }
    // Two arcs: forward from e1 (via e1+1, e1+2, ... to e2) or backward (via e1, e1-1, ... to e2+1).
    // Choose the one whose first step goes toward targetHint.
    const int nextForward = (e1 + 1) % n;
    const int nextBackward = e1;
    qreal distForward = distToPoint(polygon[nextForward], targetHint);
    qreal distBackward = distToPoint(polygon[nextBackward], targetHint);
    if (distForward <= distBackward) {
        // Forward: add vertices (e1+1)..e2.
        int idx = (e1 + 1) % n;
        const int endIdx = e2;
        while (true) {
            path.append(polygon[idx]);
            if (idx == endIdx) break;
            idx = (idx + 1) % n;
        }
    } else {
        // Backward: add vertices e1, e1-1, ..., e2+1.
        int idx = e1;
        const int endIdx = (e2 + 1) % n;
        while (true) {
            path.append(polygon[idx]);
            if (idx == endIdx) break;
            idx = (idx - 1 + n) % n;
        }
    }
    path.append(p2);
    return path;
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
    int         seqNum   = _sequenceNumber;
    MAV_FRAME   mavFrame = MAV_FRAME_GLOBAL_TERRAIN_ALT;
    double      alt      = _altitudeFact.rawValue().toDouble();
    QSet<int> segmentStarts;
    for (int s : _spraySegmentStartIndices)
        segmentStarts.insert(s);

    MissionItem* item = nullptr;
    QGeoCoordinate firstCoord = _visualTransectPoints.isEmpty() ? QGeoCoordinate() : _visualTransectPoints.first();
    item = new MissionItem(seqNum++, MAV_CMD_NAV_TAKEOFF, mavFrame,
                           0, 0, 0, 0, firstCoord.latitude(), firstCoord.longitude(), alt,
                           true, false, missionItemParent);
    items.append(item);

    for (int i = 0; i < _visualTransectPoints.count(); i++) {
        const QGeoCoordinate& coord = _visualTransectPoints[i];
        item = new MissionItem(seqNum++, MAV_CMD_NAV_WAYPOINT, mavFrame,
                               0, 0, 0, std::numeric_limits<double>::quiet_NaN(),
                               coord.latitude(), coord.longitude(), alt,
                               true, false, missionItemParent);
        items.append(item);
        if (segmentStarts.contains(i)) {
            item = new MissionItem(seqNum++, MAV_CMD_DO_SPRAYER, mavFrame,
                                   1, 0, 0, 0, 0, 0, 0, true, false, missionItemParent);
            items.append(item);
        }
        if (i > 0 && segmentStarts.contains(i - 1)) {
            item = new MissionItem(seqNum++, MAV_CMD_DO_SPRAYER, mavFrame,
                                   0, 0, 0, 0, 0, 0, 0, true, false, missionItemParent);
            items.append(item);
        }
    }

    item = new MissionItem(seqNum++, MAV_CMD_NAV_RETURN_TO_LAUNCH, mavFrame,
                           0, 0, 0, 0, 0, 0, 0, true, false, missionItemParent);
    items.append(item);
}

SprayComplexItem::ReadyForSaveState SprayComplexItem::readyForSaveState(void) const
{
    return _sprayAreaPolygon.isValid() && !_wizardMode ? ReadyForSave : NotReadyForSaveData;
}