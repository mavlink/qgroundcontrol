/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SurveyComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(SurveyComplexItemLog, "SurveyComplexItemLog")

const char* SurveyComplexItem::jsonComplexItemTypeValue =   "survey";
const char* SurveyComplexItem::jsonV3ComplexItemTypeValue = "survey";

const char* SurveyComplexItem::settingsGroup =              "Survey";
const char* SurveyComplexItem::gridAngleName =              "GridAngle";
const char* SurveyComplexItem::gridEntryLocationName =      "GridEntryLocation";
const char* SurveyComplexItem::flyAlternateTransectsName =  "FlyAlternateTransects";

const char* SurveyComplexItem::_jsonGridAngleKey =          "angle";
const char* SurveyComplexItem::_jsonEntryPointKey =         "entryLocation";

const char* SurveyComplexItem::_jsonV3GridObjectKey =                   "grid";
const char* SurveyComplexItem::_jsonV3GridAltitudeKey =                 "altitude";
const char* SurveyComplexItem::_jsonV3GridAltitudeRelativeKey =         "relativeAltitude";
const char* SurveyComplexItem::_jsonV3GridAngleKey =                    "angle";
const char* SurveyComplexItem::_jsonV3GridSpacingKey =                  "spacing";
const char* SurveyComplexItem::_jsonV3EntryPointKey =                   "entryLocation";
const char* SurveyComplexItem::_jsonV3TurnaroundDistKey =               "turnAroundDistance";
const char* SurveyComplexItem::_jsonV3CameraTriggerDistanceKey =        "cameraTriggerDistance";
const char* SurveyComplexItem::_jsonV3CameraTriggerInTurnaroundKey =    "cameraTriggerInTurnaround";
const char* SurveyComplexItem::_jsonV3HoverAndCaptureKey =              "hoverAndCapture";
const char* SurveyComplexItem::_jsonV3GroundResolutionKey =             "groundResolution";
const char* SurveyComplexItem::_jsonV3FrontalOverlapKey =               "imageFrontalOverlap";
const char* SurveyComplexItem::_jsonV3SideOverlapKey =                  "imageSideOverlap";
const char* SurveyComplexItem::_jsonV3CameraSensorWidthKey =            "sensorWidth";
const char* SurveyComplexItem::_jsonV3CameraSensorHeightKey =           "sensorHeight";
const char* SurveyComplexItem::_jsonV3CameraResolutionWidthKey =        "resolutionWidth";
const char* SurveyComplexItem::_jsonV3CameraResolutionHeightKey =       "resolutionHeight";
const char* SurveyComplexItem::_jsonV3CameraFocalLengthKey =            "focalLength";
const char* SurveyComplexItem::_jsonV3CameraMinTriggerIntervalKey =     "minTriggerInterval";
const char* SurveyComplexItem::_jsonV3CameraObjectKey =                 "camera";
const char* SurveyComplexItem::_jsonV3CameraNameKey =                   "name";
const char* SurveyComplexItem::_jsonV3ManualGridKey =                   "manualGrid";
const char* SurveyComplexItem::_jsonV3CameraOrientationLandscapeKey =   "orientationLandscape";
const char* SurveyComplexItem::_jsonV3FixedValueIsAltitudeKey =         "fixedValueIsAltitude";
const char* SurveyComplexItem::_jsonV3Refly90DegreesKey =               "refly90Degrees";
const char* SurveyComplexItem::_jsonFlyAlternateTransectsKey =          "flyAlternateTransects";

SurveyComplexItem::SurveyComplexItem(Vehicle* vehicle, bool flyView, const QString& kmlFile, QObject* parent)
    : TransectStyleComplexItem  (vehicle, flyView, settingsGroup, parent)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Survey.SettingsGroup.json"), this))
    , _gridAngleFact            (settingsGroup, _metaDataMap[gridAngleName])
    , _flyAlternateTransectsFact(settingsGroup, _metaDataMap[flyAlternateTransectsName])
    , _entryPoint               (EntryLocationTopLeft)
{
    _editorQml = "qrc:/qml/SurveyItemEditor.qml";

    // If the user hasn't changed turnaround from the default (which is a fixed wing default) and we are multi-rotor set the multi-rotor default.
    // NULL check since object creation during unit testing passes NULL for vehicle
    if (_vehicle && _vehicle->multiRotor() && _turnAroundDistanceFact.rawValue().toDouble() == _turnAroundDistanceFact.rawDefaultValue().toDouble()) {
        // Note this is set to 10 meters to work around a problem with PX4 Pro turnaround behavior. Don't change unless firmware gets better as well.
        _turnAroundDistanceFact.setRawValue(10);
    }

    // We override the altitude to the mission default
    if (_cameraCalc.isManualCamera() || !_cameraCalc.valueSetIsDistance()->rawValue().toBool()) {
        _cameraCalc.distanceToSurface()->setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());
    }

    connect(&_gridAngleFact,            &Fact::valueChanged,                        this, &SurveyComplexItem::_setDirty);
    connect(&_flyAlternateTransectsFact,&Fact::valueChanged,                        this, &SurveyComplexItem::_setDirty);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_setDirty);

    connect(&_gridAngleFact,            &Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(&_flyAlternateTransectsFact,&Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_rebuildTransects);

    // FIXME: Shouldn't these be in TransectStyleComplexItem? They are also in CorridorScanComplexItem constructur
    connect(&_cameraCalc, &CameraCalc::distanceToSurfaceRelativeChanged, this, &SurveyComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(&_cameraCalc, &CameraCalc::distanceToSurfaceRelativeChanged, this, &SurveyComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    if (!kmlFile.isEmpty()) {
        _surveyAreaPolygon.loadKMLFile(kmlFile);
        _surveyAreaPolygon.setDirty(false);
    }
    setDirty(false);
}

void SurveyComplexItem::save(QJsonArray&  planItems)
{
    QJsonObject saveObject;

    _save(saveObject);

    saveObject[JsonHelper::jsonVersionKey] =                    4;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_jsonGridAngleKey] =                             _gridAngleFact.rawValue().toDouble();
    saveObject[_jsonFlyAlternateTransectsKey] =                 _flyAlternateTransectsFact.rawValue().toBool();
    saveObject[_jsonEntryPointKey] =                            _entryPoint;

    // Polygon shape
    _surveyAreaPolygon.saveToJson(saveObject);

    planItems.append(saveObject);
}

bool SurveyComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    // We need to pull version first to determine what validation/conversion needs to be performed
    QList<JsonHelper::KeyValidateInfo> versionKeyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(complexObject, versionKeyInfoList, errorString)) {
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version < 2 || version > 4) {
        errorString = tr("Survey items do not support version %1").arg(version);
        return false;
    }

    if (version == 4) {
        if (!_loadV4(complexObject, sequenceNumber, errorString)) {
            return false;
        }
    } else {
        // Must be v2 or v3
        QJsonObject v3ComplexObject = complexObject;
        if (version == 2) {
            // Convert to v3
            if (v3ComplexObject.contains(VisualMissionItem::jsonTypeKey) && v3ComplexObject[VisualMissionItem::jsonTypeKey].toString() == QStringLiteral("survey")) {
                v3ComplexObject[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
                v3ComplexObject[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
            }
        }
        if (!_loadV3(complexObject, sequenceNumber, errorString)) {
            return false;
        }
    }

    _rebuildTransects();

    return true;
}

bool SurveyComplexItem::_loadV4(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonEntryPointKey,                           QJsonValue::Double, true },
        { _jsonGridAngleKey,                            QJsonValue::Double, true },
        { _jsonFlyAlternateTransectsKey,                QJsonValue::Bool,   false },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _ignoreRecalc = true;

    setSequenceNumber(sequenceNumber);

    if (!_surveyAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _surveyAreaPolygon.clear();
        return false;
    }

    if (!_load(complexObject, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _gridAngleFact.setRawValue              (complexObject[_jsonGridAngleKey].toDouble());
    _flyAlternateTransectsFact.setRawValue  (complexObject[_jsonFlyAlternateTransectsKey].toBool(false));

    _entryPoint = complexObject[_jsonEntryPointKey].toInt();

    _ignoreRecalc = false;

    return true;
}

bool SurveyComplexItem::_loadV3(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonHelper::KeyValidateInfo> mainKeyInfoList = {
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
        { _jsonV3GridObjectKey,                         QJsonValue::Object, true },
        { _jsonV3CameraObjectKey,                       QJsonValue::Object, false },
        { _jsonV3CameraTriggerDistanceKey,              QJsonValue::Double, true },
        { _jsonV3ManualGridKey,                         QJsonValue::Bool,   true },
        { _jsonV3FixedValueIsAltitudeKey,               QJsonValue::Bool,   true },
        { _jsonV3HoverAndCaptureKey,                    QJsonValue::Bool,   false },
        { _jsonV3Refly90DegreesKey,                     QJsonValue::Bool,   false },
        { _jsonV3CameraTriggerInTurnaroundKey,          QJsonValue::Bool,   false },    // Should really be required, but it was missing from initial code due to bug
    };
    if (!JsonHelper::validateKeys(complexObject, mainKeyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonV3ComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _ignoreRecalc = true;

    setSequenceNumber(sequenceNumber);

    _hoverAndCaptureFact.setRawValue            (complexObject[_jsonV3HoverAndCaptureKey].toBool(false));
    _refly90DegreesFact.setRawValue             (complexObject[_jsonV3Refly90DegreesKey].toBool(false));
    _cameraTriggerInTurnAroundFact.setRawValue  (complexObject[_jsonV3CameraTriggerInTurnaroundKey].toBool(true));

    _cameraCalc.valueSetIsDistance()->setRawValue   (complexObject[_jsonV3FixedValueIsAltitudeKey].toBool(true));
    _cameraCalc.setDistanceToSurfaceRelative        (complexObject[_jsonV3GridAltitudeRelativeKey].toBool(true));

    bool manualGrid = complexObject[_jsonV3ManualGridKey].toBool(true);

    QList<JsonHelper::KeyValidateInfo> gridKeyInfoList = {
        { _jsonV3GridAltitudeKey,           QJsonValue::Double, true },
        { _jsonV3GridAltitudeRelativeKey,   QJsonValue::Bool,   true },
        { _jsonV3GridAngleKey,              QJsonValue::Double, true },
        { _jsonV3GridSpacingKey,            QJsonValue::Double, true },
        { _jsonEntryPointKey,      QJsonValue::Double, false },
        { _jsonV3TurnaroundDistKey,         QJsonValue::Double, true },
    };
    QJsonObject gridObject = complexObject[_jsonV3GridObjectKey].toObject();
    if (!JsonHelper::validateKeys(gridObject, gridKeyInfoList, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _gridAngleFact.setRawValue          (gridObject[_jsonV3GridAngleKey].toDouble());
    _turnAroundDistanceFact.setRawValue (gridObject[_jsonV3TurnaroundDistKey].toDouble());

    if (gridObject.contains(_jsonEntryPointKey)) {
        _entryPoint = gridObject[_jsonEntryPointKey].toDouble();
    } else {
        _entryPoint = EntryLocationTopRight;
    }

    _cameraCalc.distanceToSurface()->setRawValue        (gridObject[_jsonV3GridAltitudeKey].toDouble());
    _cameraCalc.adjustedFootprintSide()->setRawValue    (gridObject[_jsonV3GridSpacingKey].toDouble());
    _cameraCalc.adjustedFootprintFrontal()->setRawValue (complexObject[_jsonV3CameraTriggerDistanceKey].toDouble());

    if (manualGrid) {
        _cameraCalc.cameraName()->setRawValue(_cameraCalc.manualCameraName());
    } else {
        if (!complexObject.contains(_jsonV3CameraObjectKey)) {
            errorString = tr("%1 but %2 object is missing").arg("manualGrid = false").arg("camera");
            _ignoreRecalc = false;
            return false;
        }

        QJsonObject cameraObject = complexObject[_jsonV3CameraObjectKey].toObject();

        // Older code had typo on "imageSideOverlap" incorrectly being "imageSizeOverlap"
        QString incorrectImageSideOverlap = "imageSizeOverlap";
        if (cameraObject.contains(incorrectImageSideOverlap)) {
            cameraObject[_jsonV3SideOverlapKey] = cameraObject[incorrectImageSideOverlap];
            cameraObject.remove(incorrectImageSideOverlap);
        }

        QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
            { _jsonV3GroundResolutionKey,           QJsonValue::Double, true },
            { _jsonV3FrontalOverlapKey,             QJsonValue::Double, true },
            { _jsonV3SideOverlapKey,                QJsonValue::Double, true },
            { _jsonV3CameraSensorWidthKey,          QJsonValue::Double, true },
            { _jsonV3CameraSensorHeightKey,         QJsonValue::Double, true },
            { _jsonV3CameraResolutionWidthKey,      QJsonValue::Double, true },
            { _jsonV3CameraResolutionHeightKey,     QJsonValue::Double, true },
            { _jsonV3CameraFocalLengthKey,          QJsonValue::Double, true },
            { _jsonV3CameraNameKey,                 QJsonValue::String, true },
            { _jsonV3CameraOrientationLandscapeKey, QJsonValue::Bool,   true },
            { _jsonV3CameraMinTriggerIntervalKey,   QJsonValue::Double, false },
        };
        if (!JsonHelper::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            _ignoreRecalc = false;
            return false;
        }

        _cameraCalc.cameraName()->setRawValue           (cameraObject[_jsonV3CameraNameKey].toString());
        _cameraCalc.landscape()->setRawValue            (cameraObject[_jsonV3CameraOrientationLandscapeKey].toBool(true));
        _cameraCalc.frontalOverlap()->setRawValue       (cameraObject[_jsonV3FrontalOverlapKey].toInt());
        _cameraCalc.sideOverlap()->setRawValue          (cameraObject[_jsonV3SideOverlapKey].toInt());
        _cameraCalc.sensorWidth()->setRawValue          (cameraObject[_jsonV3CameraSensorWidthKey].toDouble());
        _cameraCalc.sensorHeight()->setRawValue         (cameraObject[_jsonV3CameraSensorHeightKey].toDouble());
        _cameraCalc.focalLength()->setRawValue          (cameraObject[_jsonV3CameraFocalLengthKey].toDouble());
        _cameraCalc.imageWidth()->setRawValue           (cameraObject[_jsonV3CameraResolutionWidthKey].toInt());
        _cameraCalc.imageHeight()->setRawValue          (cameraObject[_jsonV3CameraResolutionHeightKey].toInt());
        _cameraCalc.minTriggerInterval()->setRawValue   (cameraObject[_jsonV3CameraMinTriggerIntervalKey].toDouble(0));
        _cameraCalc.imageDensity()->setRawValue         (cameraObject[_jsonV3GroundResolutionKey].toDouble());
        _cameraCalc.fixedOrientation()->setRawValue     (false);
    }

    // Polygon shape
    /// Load a polygon from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    if (!_surveyAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        _surveyAreaPolygon.clear();
        _ignoreRecalc = false;
        return false;
    }

    _ignoreRecalc = false;

    return true;
}

/// Reverse the order of the transects. First transect becomes last and so forth.
void SurveyComplexItem::_reverseTransectOrder(QList<QList<QGeoCoordinate>>& transects)
{
    QList<QList<QGeoCoordinate>> rgReversedTransects;
    for (int i=transects.count() - 1; i>=0; i--) {
        rgReversedTransects.append(transects[i]);
    }
    transects = rgReversedTransects;
}

/// Reverse the order of all points withing each transect, First point becomes last and so forth.
void SurveyComplexItem::_reverseInternalTransectPoints(QList<QList<QGeoCoordinate>>& transects)
{
    for (int i=0; i<transects.count(); i++) {
        QList<QGeoCoordinate> rgReversedCoords;
        QList<QGeoCoordinate>& rgOriginalCoords = transects[i];
        for (int j=rgOriginalCoords.count()-1; j>=0; j--) {
            rgReversedCoords.append(rgOriginalCoords[j]);
        }
        transects[i] = rgReversedCoords;
    }
}

/// Reorders the transects such that the first transect is the shortest distance to the specified coordinate
/// and the first point within that transect is the shortest distance to the specified coordinate.
///     @param distanceCoord Coordinate to measure distance against
///     @param transects Transects to test and reorder
void SurveyComplexItem::_optimizeTransectsForShortestDistance(const QGeoCoordinate& distanceCoord, QList<QList<QGeoCoordinate>>& transects)
{
    double rgTransectDistance[4];
    rgTransectDistance[0] = transects.first().first().distanceTo(distanceCoord);
    rgTransectDistance[1] = transects.first().last().distanceTo(distanceCoord);
    rgTransectDistance[2] = transects.last().first().distanceTo(distanceCoord);
    rgTransectDistance[3] = transects.last().last().distanceTo(distanceCoord);

    int shortestIndex = 0;
    double shortestDistance = rgTransectDistance[0];
    for (int i=1; i<3; i++) {
        if (rgTransectDistance[i] < shortestDistance) {
            shortestIndex = i;
            shortestDistance = rgTransectDistance[i];
        }
    }

    if (shortestIndex > 1) {
        // We need to reverse the order of segments
        _reverseTransectOrder(transects);
    }
    if (shortestIndex & 1) {
        // We need to reverse the points within each segment
        _reverseInternalTransectPoints(transects);
    }
}

qreal SurveyComplexItem::_ccw(QPointF pt1, QPointF pt2, QPointF pt3)
{
    return (pt2.x()-pt1.x())*(pt3.y()-pt1.y()) - (pt2.y()-pt1.y())*(pt3.x()-pt1.x());
}

qreal SurveyComplexItem::_dp(QPointF pt1, QPointF pt2)
{
    return (pt2.x()-pt1.x())/qSqrt((pt2.x()-pt1.x())*(pt2.x()-pt1.x()) + (pt2.y()-pt1.y())*(pt2.y()-pt1.y()));
}

void SurveyComplexItem::_swapPoints(QList<QPointF>& points, int index1, int index2)
{
    QPointF temp = points[index1];
    points[index1] = points[index2];
    points[index2] = temp;
}

/// Returns true if the current grid angle generates north/south oriented transects
bool SurveyComplexItem::_gridAngleIsNorthSouthTransects()
{
    // Grid angle ranges from -360<->360
    double gridAngle = qAbs(_gridAngleFact.rawValue().toDouble());
    return gridAngle < 45.0 || (gridAngle > 360.0 - 45.0) || (gridAngle > 90.0 + 45.0 && gridAngle < 270.0 - 45.0);
}

void SurveyComplexItem::_adjustTransectsToEntryPointLocation(QList<QList<QGeoCoordinate>>& transects)
{
    if (transects.count() == 0) {
        return;
    }

    bool reversePoints = false;
    bool reverseTransects = false;

    if (_entryPoint == EntryLocationBottomLeft || _entryPoint == EntryLocationBottomRight) {
        reversePoints = true;
    }
    if (_entryPoint == EntryLocationTopRight || _entryPoint == EntryLocationBottomRight) {
        reverseTransects = true;
    }

    if (reversePoints) {
        qCDebug(SurveyComplexItemLog) << "_adjustTransectsToEntryPointLocation Reverse Points";
        _reverseInternalTransectPoints(transects);
    }
    if (reverseTransects) {
        qCDebug(SurveyComplexItemLog) << "_adjustTransectsToEntryPointLocation Reverse Transects";
        _reverseTransectOrder(transects);
    }

    qCDebug(SurveyComplexItemLog) << "_adjustTransectsToEntryPointLocation Modified entry point:entryLocation" << transects.first().first() << _entryPoint;
}

QPointF SurveyComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * -angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void SurveyComplexItem::_intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines)
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

void SurveyComplexItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines)
{
    resultLines.clear();

    for (int i=0; i<lineList.count(); i++) {
        const QLineF& line = lineList[i];
        QList<QPointF> intersections;

        // Intersect the line with all the polygon edges
        for (int j=0; j<polygon.count()-1; j++) {
            QPointF intersectPoint;
            QLineF polygonLine = QLineF(polygon[j], polygon[j+1]);
            if (line.intersect(polygonLine, &intersectPoint) == QLineF::BoundedIntersection) {
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
                    \
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
void SurveyComplexItem::_adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines)
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

double SurveyComplexItem::_clampGridAngle90(double gridAngle)
{
    // Clamp grid angle to -90<->90. This prevents transects from being rotated to a reversed order.
    if (gridAngle > 90.0) {
        gridAngle -= 180.0;
    } else if (gridAngle < -90.0) {
        gridAngle += 180;
    }
    return gridAngle;
}

#if 0
int SurveyComplexItem::_gridGenerator(const QList<QPointF>& polygonPoints,  QList<QList<QPointF>>& transectSegments, bool refly)
{
    int cameraShots = 0;

    double gridAngle = _gridAngleFact.rawValue().toDouble();
    double gridSpacing = _gridSpacingFact.rawValue().toDouble();

    gridAngle = _clampGridAngle90(gridAngle);
    gridAngle += refly ? 90 : 0;
    qCDebug(SurveyComplexItemLog) << "Clamped grid angle" << gridAngle;

    qCDebug(SurveyComplexItemLog) << "SurveyComplexItem::_gridGenerator gridSpacing:gridAngle:refly" << gridSpacing << gridAngle << refly;

    transectSegments.clear();

    // Convert polygon to bounding rect

    qCDebug(SurveyComplexItemLog) << "Polygon";
    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        qCDebug(SurveyComplexItemLog) << polygonPoints[i];
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF boundingRect = polygon.boundingRect();
    QPointF boundingCenter = boundingRect.center();
    qCDebug(SurveyComplexItemLog) << "Bounding rect" << boundingRect.topLeft().x() << boundingRect.topLeft().y() << boundingRect.bottomRight().x() << boundingRect.bottomRight().y();

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

        lineList += QLineF(_rotatePoint(QPointF(transectX, transectYTop), boundingCenter, gridAngle), _rotatePoint(QPointF(transectX, transectYBottom), boundingCenter, gridAngle));
        transectX += gridSpacing;
    }

    // Now intersect the lines with the polygon
    QList<QLineF> intersectLines;
#if 1
    _intersectLinesWithPolygon(lineList, polygon, intersectLines);
#else
    // This is handy for debugging grid problems, not for release
    intersectLines = lineList;
#endif

    // Less than two transects intersected with the polygon:
    //      Create a single transect which goes through the center of the polygon
    //      Intersect it with the polygon
    if (intersectLines.count() < 2) {
        _mapPolygon.center();
        QLineF firstLine = lineList.first();
        QPointF lineCenter = firstLine.pointAt(0.5);
        QPointF centerOffset = boundingCenter - lineCenter;
        firstLine.translate(centerOffset);
        lineList.clear();
        lineList.append(firstLine);
        intersectLines = lineList;
        _intersectLinesWithPolygon(lineList, polygon, intersectLines);
    }

    // Make sure all lines are going to same direction. Polygon intersection leads to line which
    // can be in varied directions depending on the order of the intesecting sides.
    QList<QLineF> resultLines;
    _adjustLineDirection(intersectLines, resultLines);

    // Calc camera shots here if there are no images in turnaround
    if (_triggerCamera() && !_imagesEverywhere()) {
        for (int i=0; i<resultLines.count(); i++) {
            cameraShots += (int)floor(resultLines[i].length() / _triggerDistance());
            // Take into account immediate camera trigger at waypoint entry
            cameraShots++;
        }
    }

    // Turn into a path
    for (int i=0; i<resultLines.count(); i++) {
        QLineF          transectLine;
        QList<QPointF>  transectPoints;
        const QLineF&   line = resultLines[i];

        float turnaroundPosition = _turnaroundDistance() / line.length();

        if (i & 1) {
            transectLine = QLineF(line.p2(), line.p1());
        } else {
            transectLine = QLineF(line.p1(), line.p2());
        }

        // Build the points along the transect

        if (_hasTurnaround()) {
            transectPoints.append(transectLine.pointAt(-turnaroundPosition));
        }

        // Polygon entry point
        transectPoints.append(transectLine.p1());

        // For hover and capture we need points for each camera location
        if (_triggerCamera() && _hoverAndCaptureEnabled()) {
            if (_triggerDistance() < transectLine.length()) {
                int innerPoints = floor(transectLine.length() / _triggerDistance());
                qCDebug(SurveyComplexItemLog) << "innerPoints" << innerPoints;
                float transectPositionIncrement = _triggerDistance() / transectLine.length();
                for (int i=0; i<innerPoints; i++) {
                    transectPoints.append(transectLine.pointAt(transectPositionIncrement * (i + 1)));
                }
            }
        }

        // Polygon exit point
        transectPoints.append(transectLine.p2());

        if (_hasTurnaround()) {
            transectPoints.append(transectLine.pointAt(1 + turnaroundPosition));
        }

        transectSegments.append(transectPoints);
    }

    return cameraShots;
}
#endif

#if 0
int SurveyComplexItem::_appendWaypointToMission(QList<MissionItem*>& items, int seqNum, QGeoCoordinate& coord, CameraTriggerCode cameraTrigger, QObject* missionItemParent)
{
    double  altitude =          _gridAltitudeFact.rawValue().toDouble();
    bool    altitudeRelative =  _gridAltitudeRelativeFact.rawValue().toBool();

    qCDebug(SurveyComplexItemLog) << "_appendWaypointToMission seq:trigger" << seqNum << (cameraTrigger != CameraTriggerNone);

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        altitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                                        cameraTrigger == CameraTriggerHoverAndCapture ? _hoverAndCaptureDelaySeconds : 0,  // Hold time (delay for hover and capture to settle vehicle before image is taken)
                                        0.0, 0.0,
                                        std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                                        coord.latitude(),
                                        coord.longitude(),
                                        altitude,
                                        true,                                       // autoContinue
                                        false,                                      // isCurrentItem
                                        missionItemParent);
    items.append(item);

    switch (cameraTrigger) {
    case CameraTriggerOff:
    case CameraTriggerOn:
        item = new MissionItem(seqNum++,
                               MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                               MAV_FRAME_MISSION,
                               cameraTrigger == CameraTriggerOn ? _triggerDistance() : 0,
                               0,                                           // shutter integration (ignore)
                               cameraTrigger == CameraTriggerOn ? 1 : 0,    // trigger immediately when starting
                               0, 0, 0, 0,                                  // param 4-7 unused
                               true,                                        // autoContinue
                               false,                                       // isCurrentItem
                               missionItemParent);
        items.append(item);
        break;
    case CameraTriggerHoverAndCapture:
        item = new MissionItem(seqNum++,
                               MAV_CMD_IMAGE_START_CAPTURE,
                               MAV_FRAME_MISSION,
                               0,                           // Reserved (Set to 0)
                               0,                           // Interval (none)
                               1,                           // Take 1 photo
                               NAN, NAN, NAN, NAN,          // param 4-7 reserved
                               true,                        // autoContinue
                               false,                       // isCurrentItem
                               missionItemParent);
        items.append(item);
#if 0
        // This generates too many commands. Pulling out for now, to see if image quality is still high enough.
        item = new MissionItem(seqNum++,
                               MAV_CMD_NAV_DELAY,
                               MAV_FRAME_MISSION,
                               0.5,                // Delay in seconds, give some time for image to be taken
                               -1, -1, -1,         // No time
                               0, 0, 0,            // Param 5-7 unused
                               true,               // autoContinue
                               false,              // isCurrentItem
                               missionItemParent);
        items.append(item);
#endif
    default:
        break;
    }

    return seqNum;
}
#endif

bool SurveyComplexItem::_nextTransectCoord(const QList<QGeoCoordinate>& transectPoints, int pointIndex, QGeoCoordinate& coord)
{
    if (pointIndex > transectPoints.count()) {
        qWarning() << "Bad grid generation";
        return false;
    }

    coord = transectPoints[pointIndex];
    return true;
}

void SurveyComplexItem::_buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(SurveyComplexItemLog) << "_buildAndAppendMissionItems";

    // Now build the mission items from the transect points

    MissionItem* item;
    int seqNum =                    _sequenceNumber;
    bool imagesEverywhere =         _cameraTriggerInTurnAroundFact.rawValue().toBool();
    bool addTriggerAtBeginning =    !hoverAndCaptureEnabled() && imagesEverywhere;
    bool firstOverallPoint =        true;

    MAV_FRAME mavFrame = followTerrain() || !_cameraCalc.distanceToSurfaceRelative() ? MAV_FRAME_GLOBAL : MAV_FRAME_GLOBAL_RELATIVE_ALT;

    for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
        bool entryPoint = true;

        for (const CoordInfo_t& transectCoordInfo: transect) {
            item = new MissionItem(seqNum++,
                                   MAV_CMD_NAV_WAYPOINT,
                                   mavFrame,
                                   hoverAndCaptureEnabled() ?
                                       _hoverAndCaptureDelaySeconds : 0,        // Hold time (delay for hover and capture to settle vehicle before image is taken)
                                   0.0,                                         // No acceptance radius specified
                                   0.0,                                         // Pass through waypoint
                                   std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                                   transectCoordInfo.coord.latitude(),
                                   transectCoordInfo.coord.longitude(),
                                   transectCoordInfo.coord.altitude(),
                                   true,                                        // autoContinue
                                   false,                                       // isCurrentItem
                                   missionItemParent);
            items.append(item);
            if (hoverAndCaptureEnabled()) {
                item = new MissionItem(seqNum++,
                                       MAV_CMD_IMAGE_START_CAPTURE,
                                       MAV_FRAME_MISSION,
                                       0,                           // Reserved (Set to 0)
                                       0,                           // Interval (none)
                                       1,                           // Take 1 photo
                                       NAN, NAN, NAN, NAN,          // param 4-7 reserved
                                       true,                        // autoContinue
                                       false,                       // isCurrentItem
                                       missionItemParent);
                items.append(item);
            }

            if (firstOverallPoint && addTriggerAtBeginning) {
                // Start triggering
                addTriggerAtBeginning = false;
                item = new MissionItem(seqNum++,
                                       MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                       MAV_FRAME_MISSION,
                                       triggerDistance(),   // trigger distance
                                       0,                   // shutter integration (ignore)
                                       1,                   // trigger immediately when starting
                                       0, 0, 0, 0,          // param 4-7 unused
                                       true,                // autoContinue
                                       false,               // isCurrentItem
                                       missionItemParent);
                items.append(item);
            }
            firstOverallPoint = false;

            if (transectCoordInfo.coordType == TransectStyleComplexItem::CoordTypeSurveyEdge && triggerCamera() && !hoverAndCaptureEnabled() && !imagesEverywhere) {
                if (entryPoint) {
                    // Start of transect, start triggering
                    item = new MissionItem(seqNum++,
                                           MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                           MAV_FRAME_MISSION,
                                           triggerDistance(),   // trigger distance
                                           0,                   // shutter integration (ignore)
                                           1,                   // trigger immediately when starting
                                           0, 0, 0, 0,          // param 4-7 unused
                                           true,                // autoContinue
                                           false,               // isCurrentItem
                                           missionItemParent);
                    items.append(item);
                } else {
                    // End of transect, stop triggering
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
                entryPoint = !entryPoint;
            }
        }
    }

    if (triggerCamera() && !hoverAndCaptureEnabled() &&  imagesEverywhere) {
        // Stop triggering
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

#if 0
bool SurveyComplexItem::_buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;
    bool firstWaypointTrigger = false;

#if 1
    // FIXME: Hack
    bool hasRefly = false;
    bool buildRefly = false;
#endif

    qCDebug(SurveyComplexItemLog) << QStringLiteral("hasTurnaround(%1) triggerCamera(%2) hoverAndCapture(%3) imagesEverywhere(%4) hasRefly(%5) buildRefly(%6) ").arg(_hasTurnaround()).arg(_triggerCamera()).arg(_hoverAndCaptureEnabled()).arg(_imagesEverywhere()).arg(hasRefly).arg(buildRefly);

#if 0
    QList<QList<QGeoCoordinate>>& transectSegments = buildRefly ? _reflyTransectSegments : _transectSegments;
#endif

    if (!buildRefly && _imagesEverywhere()) {
        firstWaypointTrigger = true;
    }

    for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
        int pointIndex = 0;
        QGeoCoordinate coord;
        CameraTriggerCode cameraTrigger;

        qCDebug(SurveyComplexItemLog) << "transect.count" << transect.count();

        if (_hasTurnaround()) {
            // Add entry turnaround point
            if (!_nextTransectCoord(segment, pointIndex++, coord)) {
                return false;
            }
            seqNum = _appendWaypointToMission(items, seqNum, coord, firstWaypointTrigger ? CameraTriggerOn : CameraTriggerNone, missionItemParent);
            firstWaypointTrigger = false;
        }

        // Add polygon entry point
        if (!_nextTransectCoord(segment, pointIndex++, coord)) {
            return false;
        }
        if (firstWaypointTrigger) {
            cameraTrigger = CameraTriggerOn;
        } else {
            cameraTrigger = _imagesEverywhere() || !_triggerCamera() ? CameraTriggerNone : (_hoverAndCaptureEnabled() ? CameraTriggerHoverAndCapture : CameraTriggerOn);
        }
        seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);
        firstWaypointTrigger = false;

        // Add internal hover and capture points
        if (_hoverAndCaptureEnabled()) {
            int lastHoverAndCaptureIndex = segment.count() - 1 - (_hasTurnaround() ? 1 : 0);
            qCDebug(SurveyComplexItemLog) << "lastHoverAndCaptureIndex" << lastHoverAndCaptureIndex;
            for (; pointIndex < lastHoverAndCaptureIndex; pointIndex++) {
                if (!_nextTransectCoord(segment, pointIndex, coord)) {
                    return false;
                }
                seqNum = _appendWaypointToMission(items, seqNum, coord, CameraTriggerHoverAndCapture, missionItemParent);
            }
        }

        // Add polygon exit point
        if (!_nextTransectCoord(segment, pointIndex++, coord)) {
            return false;
        }
        cameraTrigger = _imagesEverywhere() || !_triggerCamera() ? CameraTriggerNone : (_hoverAndCaptureEnabled() ? CameraTriggerNone : CameraTriggerOff);
        seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);

        if (_hasTurnaround()) {
            // Add exit turnaround point
            if (!_nextTransectCoord(segment, pointIndex++, coord)) {
                return false;
            }
            seqNum = _appendWaypointToMission(items, seqNum, coord, CameraTriggerNone, missionItemParent);
        }

        qCDebug(SurveyComplexItemLog) << "last PointIndex" << pointIndex;
    }

    if (((hasRefly && buildRefly) || !hasRefly) && _imagesEverywhere()) {
        // Turn off camera at end of survey
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                            MAV_FRAME_MISSION,
                                            0.0,                    // trigger distance (off)
                                            0, 0, 0, 0, 0, 0,       // param 2-7 unused
                                            true,                   // autoContinue
                                            false,                  // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

    return true;
}
#endif

#if 0
void SurveyComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;
    bool refly = refly90Degrees()->rawValue().toBool();

    if (!_appendMissionItemsWorker(items, missionItemParent, seqNum, refly, false /* buildRefly */)) {
        return;
    }

    if (refly) {
        _appendMissionItemsWorker(items, missionItemParent, seqNum, refly, true /* buildRefly */);
    }
}
#endif

bool SurveyComplexItem::_hasTurnaround(void) const
{
    return _turnaroundDistance() > 0;
}

double SurveyComplexItem::_turnaroundDistance(void) const
{
    return _turnAroundDistanceFact.rawValue().toDouble();
}

#if 0
bool SurveyComplexItem::_triggerCamera(void) const
{
    return _triggerDistance() > 0;
}

bool SurveyComplexItem::_imagesEverywhere(void) const
{
    return _triggerCamera() && _cameraTriggerInTurnaroundFact.rawValue().toBool();
}

bool SurveyComplexItem::_hoverAndCaptureEnabled(void) const
{
    return hoverAndCaptureAllowed() && !_imagesEverywhere() && _triggerCamera() && _hoverAndCaptureFact.rawValue().toBool();
}
#endif

void SurveyComplexItem::_rebuildTransectsPhase1(void)
{
    _rebuildTransectsPhase1Worker(false /* refly */);
    if (_refly90DegreesFact.rawValue().toBool()) {
        _rebuildTransectsPhase1Worker(true /* refly */);
    }
}

void SurveyComplexItem::_rebuildTransectsPhase1Worker(bool refly)
{
    if (_ignoreRecalc) {
        return;
    }

    // If the transects are getting rebuilt then any previously loaded mission items are now invalid
    if (_loadedMissionItemsParent) {
        _loadedMissionItems.clear();
        _loadedMissionItemsParent->deleteLater();
        _loadedMissionItemsParent = NULL;
    }

    // First pass will clear old transect data, refly will append to existing data
    if (!refly) {
        _transects.clear();
        _transectsPathHeightInfo.clear();
    }

    if (_surveyAreaPolygon.count() < 3) {
        return;
    }

    // Convert polygon to NED

    QList<QPointF> polygonPoints;
    QGeoCoordinate tangentOrigin = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(0)->coordinate();
    qCDebug(SurveyComplexItemLog) << "_rebuildTransectsPhase1 Convert polygon to NED - _surveyAreaPolygon.count():tangentOrigin" << _surveyAreaPolygon.count() << tangentOrigin;
    for (int i=0; i<_surveyAreaPolygon.count(); i++) {
        double y, x, down;
        QGeoCoordinate vertex = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i)->coordinate();
        if (i == 0) {
            // This avoids a nan calculation that comes out of convertGeoToNed
            x = y = 0;
        } else {
            convertGeoToNed(vertex, tangentOrigin, &y, &x, &down);
        }
        polygonPoints += QPointF(x, y);
        qCDebug(SurveyComplexItemLog) << "_rebuildTransectsPhase1 vertex:x:y" << vertex << polygonPoints.last().x() << polygonPoints.last().y();
    }

    // Generate transects

    double gridAngle = _gridAngleFact.rawValue().toDouble();
    double gridSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();

    gridAngle = _clampGridAngle90(gridAngle);
    gridAngle += refly ? 90 : 0;
    qCDebug(SurveyComplexItemLog) << "_rebuildTransectsPhase1 Clamped grid angle" << gridAngle;

    qCDebug(SurveyComplexItemLog) << "_rebuildTransectsPhase1 gridSpacing:gridAngle:refly" << gridSpacing << gridAngle << refly;

    // Convert polygon to bounding rect

    qCDebug(SurveyComplexItemLog) << "_rebuildTransectsPhase1 Polygon";
    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        qCDebug(SurveyComplexItemLog) << "Vertex" << polygonPoints[i];
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF boundingRect = polygon.boundingRect();
    QPointF boundingCenter = boundingRect.center();
    qCDebug(SurveyComplexItemLog) << "Bounding rect" << boundingRect.topLeft().x() << boundingRect.topLeft().y() << boundingRect.bottomRight().x() << boundingRect.bottomRight().y();

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

        lineList += QLineF(_rotatePoint(QPointF(transectX, transectYTop), boundingCenter, gridAngle), _rotatePoint(QPointF(transectX, transectYBottom), boundingCenter, gridAngle));
        transectX += gridSpacing;
    }

    // Now intersect the lines with the polygon
    QList<QLineF> intersectLines;
#if 1
    _intersectLinesWithPolygon(lineList, polygon, intersectLines);
#else
    // This is handy for debugging grid problems, not for release
    intersectLines = lineList;
#endif

    // Less than two transects intersected with the polygon:
    //      Create a single transect which goes through the center of the polygon
    //      Intersect it with the polygon
    if (intersectLines.count() < 2) {
        _surveyAreaPolygon.center();
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
    for (const QLineF& line: resultLines) {
        QGeoCoordinate          coord;
        QList<QGeoCoordinate>   transect;

        convertNedToGeo(line.p1().y(), line.p1().x(), 0, tangentOrigin, &coord);
        transect.append(coord);
        convertNedToGeo(line.p2().y(), line.p2().x(), 0, tangentOrigin, &coord);
        transect.append(coord);

        transects.append(transect);
    }

    _adjustTransectsToEntryPointLocation(transects);

    if (refly) {
        _optimizeTransectsForShortestDistance(_transects.last().last().coord, transects);
    }

    if (_flyAlternateTransectsFact.rawValue().toBool()) {
        QList<QList<QGeoCoordinate>> alternatingTransects;
        for (int i=0; i<transects.count(); i++) {
            if (!(i & 1)) {
                alternatingTransects.append(transects[i]);
            }
        }
        for (int i=transects.count()-1; i>0; i--) {
            if (i & 1) {
                alternatingTransects.append(transects[i]);
            }
        }
        transects = alternatingTransects;
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

    // Convert to CoordInfo transects and append to _transects
    for (const QList<QGeoCoordinate>& transect: transects) {
        QGeoCoordinate                                  coord;
        QList<TransectStyleComplexItem::CoordInfo_t>    coordInfoTransect;
        TransectStyleComplexItem::CoordInfo_t           coordInfo;

        coordInfo = { transect[0], CoordTypeSurveyEdge };
        coordInfoTransect.append(coordInfo);
        coordInfo = { transect[1], CoordTypeSurveyEdge };
        coordInfoTransect.append(coordInfo);

        // For hover and capture we need points for each camera location within the transect
        if (triggerCamera() && hoverAndCaptureEnabled()) {
            double transectLength = transect[0].distanceTo(transect[1]);
            double transectAzimuth = transect[0].azimuthTo(transect[1]);
            if (triggerDistance() < transectLength) {
                int cInnerHoverPoints = floor(transectLength / triggerDistance());
                qCDebug(SurveyComplexItemLog) << "cInnerHoverPoints" << cInnerHoverPoints;
                for (int i=0; i<cInnerHoverPoints; i++) {
                    QGeoCoordinate hoverCoord = transect[0].atDistanceAndAzimuth(triggerDistance() * (i + 1), transectAzimuth);
                    TransectStyleComplexItem::CoordInfo_t coordInfo = { hoverCoord, CoordTypeInteriorHoverTrigger };
                    coordInfoTransect.insert(1 + i, coordInfo);
                }
            }
        }

        // Extend the transect ends for turnaround
        if (_hasTurnaround()) {
            QGeoCoordinate turnaroundCoord;
            double turnAroundDistance = _turnAroundDistanceFact.rawValue().toDouble();

            double azimuth = transect[0].azimuthTo(transect[1]);
            turnaroundCoord = transect[0].atDistanceAndAzimuth(-turnAroundDistance, azimuth);
            turnaroundCoord.setAltitude(qQNaN());
            TransectStyleComplexItem::CoordInfo_t coordInfo = { turnaroundCoord, CoordTypeTurnaround };
            coordInfoTransect.prepend(coordInfo);

            azimuth = transect.last().azimuthTo(transect[transect.count() - 2]);
            turnaroundCoord = transect.last().atDistanceAndAzimuth(-turnAroundDistance, azimuth);
            turnaroundCoord.setAltitude(qQNaN());
            coordInfo = { turnaroundCoord, CoordTypeTurnaround };
            coordInfoTransect.append(coordInfo);
        }

        _transects.append(coordInfoTransect);
    }
}

void SurveyComplexItem::_rebuildTransectsPhase2(void)
{
    // Calculate distance flown for complex item
    _complexDistance = 0;
    for (int i=0; i<_visualTransectPoints.count() - 1; i++) {
        _complexDistance += _visualTransectPoints[i].value<QGeoCoordinate>().distanceTo(_visualTransectPoints[i+1].value<QGeoCoordinate>());
    }

    if (triggerDistance() == 0) {
        _cameraShots = 0;
    } else {
        if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
            _cameraShots = qCeil(_complexDistance / triggerDistance());
        } else {
            _cameraShots = 0;
            for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
                QGeoCoordinate firstCameraCoord, lastCameraCoord;
                if (_hasTurnaround()) {
                    firstCameraCoord = transect[1].coord;
                    lastCameraCoord = transect[transect.count() - 2].coord;
                } else {
                    firstCameraCoord = transect.first().coord;
                    lastCameraCoord = transect.last().coord;
                }
                _cameraShots += qCeil(firstCameraCoord.distanceTo(lastCameraCoord) / triggerDistance());
            }
        }
    }

    _coordinate = _visualTransectPoints.count() ? _visualTransectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _visualTransectPoints.count() ? _visualTransectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();

    emit cameraShotsChanged();
    emit complexDistanceChanged();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

// FIXME: This same exact code is in Corridor Scan. Move to TransectStyleComplex?
void SurveyComplexItem::applyNewAltitude(double newAltitude)
{
    _cameraCalc.valueSetIsDistance()->setRawValue(true);
    _cameraCalc.distanceToSurface()->setRawValue(newAltitude);
    _cameraCalc.setDistanceToSurfaceRelative(true);
}

bool SurveyComplexItem::readyForSave(void) const
{
    return TransectStyleComplexItem::readyForSave();
}

void SurveyComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    if (_loadedMissionItems.count()) {
        // We have mission items from the loaded plan, use those
        _appendLoadedMissionItems(items, missionItemParent);
    } else {
        // Build the mission items on the fly
        _buildAndAppendMissionItems(items, missionItemParent);
    }
}

void SurveyComplexItem::_appendLoadedMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(SurveyComplexItemLog) << "_appendLoadedMissionItems";

    int seqNum = _sequenceNumber;

    for (const MissionItem* loadedMissionItem: _loadedMissionItems) {
        MissionItem* item = new MissionItem(*loadedMissionItem, missionItemParent);
        item->setSequenceNumber(seqNum++);
        items.append(item);
    }
}

void SurveyComplexItem::rotateEntryPoint(void)
{
    if (_entryPoint == EntryLocationLast) {
        _entryPoint = EntryLocationFirst;
    } else {
        _entryPoint++;
    }

    _rebuildTransects();

    setDirty(true);
}

double SurveyComplexItem::timeBetweenShots(void)
{
    return _cruiseSpeed == 0 ? 0 : triggerDistance() / _cruiseSpeed;
}
