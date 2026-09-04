#include "SurveyComplexItem.h"
#include "JsonParsing.h"
#include "QGCGeo.h"
#include "QGCPolygonClipper.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanMasterController.h"
#include "MissionItem.h"
#include "AppMessages.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtGui/QPolygonF>
#include <QtCore/QJsonArray>
#include <QtCore/QLineF>

#include <algorithm>

QGC_LOGGING_CATEGORY(SurveyComplexItemLog, "Plan.SurveyComplexItem")

namespace {

using GeoTransect = QList<QGeoCoordinate>;
using GeoTransectGroup = QList<GeoTransect>;
using GeoTransectGroups = QList<GeoTransectGroup>;

void reverseGroupDirection(GeoTransectGroup& group)
{
    std::reverse(group.begin(), group.end());
    for (GeoTransect& transect : group) {
        std::reverse(transect.begin(), transect.end());
    }
}

void reverseAllGroupDirections(GeoTransectGroups& groups)
{
    for (GeoTransectGroup& group : groups) {
        reverseGroupDirection(group);
    }
}

void optimizeGroupsForShortestDistance(const QGeoCoordinate& distanceCoordinate, GeoTransectGroups& groups)
{
    if (groups.isEmpty()) {
        return;
    }

    const GeoTransectGroup& firstGroup = groups.first();
    const GeoTransectGroup& lastGroup = groups.last();
    const double groupDistances[] = {
        firstGroup.first().first().distanceTo(distanceCoordinate),
        firstGroup.last().last().distanceTo(distanceCoordinate),
        lastGroup.first().first().distanceTo(distanceCoordinate),
        lastGroup.last().last().distanceTo(distanceCoordinate),
    };

    int shortestIndex = 0;
    for (int index = 1; index < 4; ++index) {
        if (groupDistances[index] < groupDistances[shortestIndex]) {
            shortestIndex = index;
        }
    }

    if (shortestIndex > 1) {
        std::reverse(groups.begin(), groups.end());
    }
    if (shortestIndex & 1) {
        reverseAllGroupDirections(groups);
    }
}

qsizetype transectCount(const QList<QList<QLineF>>& lineGroups)
{
    qsizetype count = 0;
    for (const QList<QLineF>& group : lineGroups) {
        count += group.size();
    }
    return count;
}

void joinLineFragments(QList<QList<QLineF>>& lineGroups)
{
    for (QList<QLineF>& group : lineGroups) {
        if (group.size() > 1) {
            group = {QLineF(group.first().p1(), group.last().p2())};
        }
    }
}

}  // namespace

SurveyComplexItem::SurveyComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile)
    : TransectStyleComplexItem  (masterController, flyView, settingsGroup)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Survey.SettingsGroup.json"), this))
    , _gridAngleFact            (settingsGroup, _metaDataMap[gridAngleName])
    , _flyAlternateTransectsFact(settingsGroup, _metaDataMap[flyAlternateTransectsName])
    , _splitConcavePolygonsFact (settingsGroup, _metaDataMap[splitConcavePolygonsName])
    , _entryPoint               (EntryLocationTopLeft)
{
    _editorQml = "qrc:/qml/QGroundControl/PlanView/SurveyItemEditor.qml";

    if (_controllerVehicle && !(_controllerVehicle->fixedWing() || _controllerVehicle->vtol())) {
        // Only fixed wing flight paths support alternate transects
        _flyAlternateTransectsFact.setRawValue(false);
    }

    // We override the altitude to the mission default
    if (_cameraCalc.isManualCamera() || !_cameraCalc.valueSetIsDistance()->rawValue().toBool()) {
        _cameraCalc.distanceToSurface()->setRawValue(SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue());
    }

    connect(&_gridAngleFact,            &Fact::valueChanged,                        this, &SurveyComplexItem::_setDirty);
    connect(&_flyAlternateTransectsFact,&Fact::valueChanged,                        this, &SurveyComplexItem::_setDirty);
    connect(&_splitConcavePolygonsFact, &Fact::valueChanged,                        this, &SurveyComplexItem::_setDirty);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_setDirty);

    connect(&_gridAngleFact,            &Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(&_flyAlternateTransectsFact,&Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(&_splitConcavePolygonsFact, &Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_rebuildTransects);

    connect(&_surveyAreaPolygon,        &QGCMapPolygon::isValidChanged,             this, &SurveyComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon,        &QGCMapPolygon::traceModeChanged,           this, &SurveyComplexItem::_updateWizardMode);

    if (!kmlOrShpFile.isEmpty()) {
        _surveyAreaPolygon.loadKMLOrSHPFile(kmlOrShpFile);
        _surveyAreaPolygon.setDirty(false);
    }
    setDirty(false);
}

void SurveyComplexItem::save(QJsonArray&  planItems)
{
    QJsonObject saveObject;

    _saveCommon(saveObject);
    planItems.append(saveObject);
}

void SurveyComplexItem::savePreset(const QString& presetName)
{
    QJsonObject saveObject;

    _saveCommon(saveObject);
    _savePresetJson(presetName, saveObject);
}

void SurveyComplexItem::_saveCommon(QJsonObject& saveObject)
{
    TransectStyleComplexItem::_save(saveObject);

    saveObject[JsonParsing::jsonVersionKey] =                    5;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_jsonGridAngleKey] =                             _gridAngleFact.rawValue().toDouble();
    saveObject[_jsonFlyAlternateTransectsKey] =                 _flyAlternateTransectsFact.rawValue().toBool();
    saveObject[_jsonSplitConcavePolygonsKey] =                  _splitConcavePolygonsFact.rawValue().toBool();
    saveObject[_jsonEntryPointKey] =                            _entryPoint;

    // Polygon shape
    _surveyAreaPolygon.saveToJson(saveObject);
}

void SurveyComplexItem::loadPreset(const QString& presetName)
{
    QString errorString;

    QJsonObject presetObject = _loadPresetJson(presetName);
    if (!_loadV4V5(presetObject, 0, errorString, 5, true /* forPresets */)) {
        QGC::showAppMessage(QStringLiteral("Internal Error: Preset load failed. Name: %1 Error: %2").arg(presetName).arg(errorString));
    }
    _rebuildTransects();
}

void SurveyComplexItem::applyPreviousAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame prevAltFrame, double prevAltitude)
{
    Q_UNUSED(prevAltitude);
    _cameraCalc.setDistanceMode(prevAltFrame);
}

bool SurveyComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    // We need to pull version first to determine what validation/conversion needs to be performed
    QList<JsonParsing::KeyValidateInfo> versionKeyInfoList = {
        { JsonParsing::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonParsing::validateKeys(complexObject, versionKeyInfoList, errorString)) {
        return false;
    }

    int version = complexObject[JsonParsing::jsonVersionKey].toInt();
    if (version < 2 || version > 5) {
        errorString = tr("Survey items do not support version %1").arg(version);
        return false;
    }

    if (version == 4 || version == 5) {
        if (!_loadV4V5(complexObject, sequenceNumber, errorString, version, false /* forPresets */)) {
            return false;
        }

        _recalcComplexDistance();
        if (_cameraShots == 0) {
            // Shot count was possibly not available from plan file
            _recalcCameraShots();
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

        // V2/3 doesn't include individual items so we need to rebuild manually
        _rebuildTransects();
    }

    return true;
}

bool SurveyComplexItem::_loadV4V5(const QJsonObject& complexObject, int sequenceNumber, QString& errorString, int version, bool forPresets)
{
    QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonEntryPointKey,                           QJsonValue::Double, true },
        { _jsonGridAngleKey,                            QJsonValue::Double, true },
        { _jsonFlyAlternateTransectsKey,                QJsonValue::Bool,   false },
    };

    if(version == 5) {
        JsonParsing::KeyValidateInfo jSplitPolygon = { _jsonSplitConcavePolygonsKey, QJsonValue::Bool, true };
        keyInfoList.append(jSplitPolygon);
    }

    if (!JsonParsing::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _ignoreRecalc = !forPresets;

    if (!forPresets) {
        setSequenceNumber(sequenceNumber);

        if (!_surveyAreaPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
            _surveyAreaPolygon.clear();
            return false;
        }
    }

    if (!TransectStyleComplexItem::_load(complexObject, forPresets, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _gridAngleFact.setRawValue              (complexObject[_jsonGridAngleKey].toDouble());
    _flyAlternateTransectsFact.setRawValue  (complexObject[_jsonFlyAlternateTransectsKey].toBool(false));

    if (version == 5) {
        _splitConcavePolygonsFact.setRawValue   (complexObject[_jsonSplitConcavePolygonsKey].toBool(true));
    }

    _entryPoint = complexObject[_jsonEntryPointKey].toInt();

    _ignoreRecalc = false;

    return true;
}

bool SurveyComplexItem::_loadV3(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QList<JsonParsing::KeyValidateInfo> mainKeyInfoList = {
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
    if (!JsonParsing::validateKeys(complexObject, mainKeyInfoList, errorString)) {
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
    _cameraCalc.setDistanceMode(complexObject[_jsonV3GridAltitudeRelativeKey].toBool(true) ? QGroundControlQmlGlobal::AltitudeFrameRelative : QGroundControlQmlGlobal::AltitudeFrameAbsolute);

    bool manualGrid = complexObject[_jsonV3ManualGridKey].toBool(true);

    QList<JsonParsing::KeyValidateInfo> gridKeyInfoList = {
        { _jsonV3GridAltitudeKey,           QJsonValue::Double, true },
        { _jsonV3GridAltitudeRelativeKey,   QJsonValue::Bool,   true },
        { _jsonV3GridAngleKey,              QJsonValue::Double, true },
        { _jsonV3GridSpacingKey,            QJsonValue::Double, true },
        { _jsonEntryPointKey,               QJsonValue::Double, false },
        { _jsonV3TurnaroundDistKey,         QJsonValue::Double, true },
    };
    QJsonObject gridObject = complexObject[_jsonV3GridObjectKey].toObject();
    if (!JsonParsing::validateKeys(gridObject, gridKeyInfoList, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _gridAngleFact.setRawValue          (gridObject[_jsonV3GridAngleKey].toDouble());
    _turnAroundDistanceFact.setRawValue (gridObject[_jsonV3TurnaroundDistKey].toDouble());

    if (gridObject.contains(_jsonEntryPointKey)) {
        _entryPoint = gridObject[_jsonEntryPointKey].toInt();
    } else {
        _entryPoint = EntryLocationTopRight;
    }

    _cameraCalc.distanceToSurface()->setRawValue        (gridObject[_jsonV3GridAltitudeKey].toDouble());
    _cameraCalc.adjustedFootprintSide()->setRawValue    (gridObject[_jsonV3GridSpacingKey].toDouble());
    _cameraCalc.adjustedFootprintFrontal()->setRawValue (complexObject[_jsonV3CameraTriggerDistanceKey].toDouble());

    if (manualGrid) {
        _cameraCalc.setCameraBrand(CameraCalc::canonicalManualCameraName());
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

        QList<JsonParsing::KeyValidateInfo> cameraKeyInfoList = {
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
        if (!JsonParsing::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            _ignoreRecalc = false;
            return false;
        }

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
        _cameraCalc._setCameraNameFromV3TransectLoad    (cameraObject[_jsonV3CameraNameKey].toString());
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

QPointF SurveyComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * -angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void SurveyComplexItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon,
                                                   QList<QList<QLineF>>& resultLineGroups)
{
    resultLineGroups.clear();

    const QList<QList<QLineF>> clippedLineGroups = QGCPolygonClipper::intersectLines(lineList, polygon);
    const bool splitConcavePolygons = _splitConcavePolygonsFact.rawValue().toBool();
    for (const QList<QLineF>& clippedLines : clippedLineGroups) {
        if (clippedLines.isEmpty()) {
            continue;
        }

        if (splitConcavePolygons) {
            resultLineGroups.append(clippedLines);
        } else {
            resultLineGroups.append({QLineF(clippedLines.first().p1(), clippedLines.last().p2())});
        }
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

bool SurveyComplexItem::_nextTransectCoord(const QList<QGeoCoordinate>& transectPoints, int pointIndex, QGeoCoordinate& coord)
{
    if (pointIndex > transectPoints.count()) {
        qWarning() << "Bad grid generation";
        return false;
    }

    coord = transectPoints[pointIndex];
    return true;
}

bool SurveyComplexItem::_hasTurnaround(void) const
{
    return _turnAroundDistance() > 0;
}

double SurveyComplexItem::_turnaroundDistance(void) const
{
    return _turnAroundDistanceFact.rawValue().toDouble();
}

void SurveyComplexItem::_rebuildTransectsPhase1(void)
{
    _rebuildTransectsPhase1WorkerSinglePolygon(false /* refly */);
    if (_refly90DegreesFact.rawValue().toBool()) {
        _rebuildTransectsPhase1WorkerSinglePolygon(true /* refly */);
    }
}

void SurveyComplexItem::_rebuildTransectsPhase1WorkerSinglePolygon(bool refly)
{
    if (_ignoreRecalc) {
        return;
    }

    // If the transects are getting rebuilt then any previously loaded mission items are now invalid
    if (_loadedMissionItemsParent) {
        _loadedMissionItems.clear();
        _loadedMissionItemsParent->deleteLater();
        _loadedMissionItemsParent = nullptr;
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
            QGCGeo::convertGeoToNed(vertex, tangentOrigin, y, x, down);
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

    // Sweep lines must extend beyond the polygon boundary regardless of grid angle.
    // The worst case is when the polygon is rotated 45° relative to the sweep direction,
    // where the required reach equals half the diagonal of the bounding rect.
    // We use diagonal * 1.5 to provide a 50% safety margin beyond that worst case.
    // Note: the old fixed +2000m was insufficient for polygons larger than ~10km.
    const double diagonal =
        qSqrt(boundingRect.width() * boundingRect.width() + boundingRect.height() * boundingRect.height());
    double maxWidth = diagonal * 1.5;
    if (maxWidth <= 0.0) {
        qCWarning(SurveyComplexItemLog)
            << "Degenerate polygon bounding rect (all vertices coincident or collinear), aborting transect rebuild";
        return;
    }

    const auto generateSweepLines = [this, boundingCenter, gridAngle, maxWidth](double spacing) {
        QList<QLineF> lines;
        const double halfWidth = maxWidth / 2.0;
        const double transectXMax = boundingCenter.x() + halfWidth;
        for (double transectX = boundingCenter.x() - halfWidth; transectX < transectXMax; transectX += spacing) {
            const double transectYTop = boundingCenter.y() - halfWidth;
            const double transectYBottom = boundingCenter.y() + halfWidth;
            lines.append(QLineF(_rotatePoint(QPointF(transectX, transectYTop), boundingCenter, gridAngle),
                                _rotatePoint(QPointF(transectX, transectYBottom), boundingCenter, gridAngle)));
        }
        return lines;
    };

    QList<QLineF> lineList;
    if (gridSpacing <= 0) {
        // Invalid spacing: seed one center line so the < 2 fallback produces a single center transect.
        qCWarning(SurveyComplexItemLog) << "Grid spacing" << gridSpacing
                                        << "is invalid, falling back to single center transect";
        const double halfW = maxWidth / 2.0;
        lineList +=
            QLineF(_rotatePoint(QPointF(boundingCenter.x(), boundingCenter.y() - halfW), boundingCenter, gridAngle),
                   _rotatePoint(QPointF(boundingCenter.x(), boundingCenter.y() + halfW), boundingCenter, gridAngle));
    } else {
        // Cap spacing so the sweep never generates more than maxTransectCount transects.
        // Uses diagonal (not maxWidth) so the count reflects actual polygon-crossing transects.
        if (gridSpacing < diagonal / maxTransectCount) {
            qCWarning(SurveyComplexItemLog)
                << "Transect spacing" << gridSpacing << "raised to" << diagonal / maxTransectCount
                << "to limit transect count to" << maxTransectCount;
            gridSpacing = diagonal / maxTransectCount;
        }
        lineList = generateSweepLines(gridSpacing);
    }

    // Keep fragments from the same sweep line grouped so route ordering can reverse a complete
    // sweep without joining separate fragments across a concave gap.
    QList<QList<QLineF>> intersectLineGroups;
    _intersectLinesWithPolygon(lineList, polygon, intersectLineGroups);

    if (_splitConcavePolygonsFact.rawValue().toBool() && (gridSpacing > 0.0) &&
        (transectCount(intersectLineGroups) > maxTransectCount)) {
        const double originalGridSpacing = gridSpacing;
        for (int attempt = 0; (attempt < 8) && (transectCount(intersectLineGroups) > maxTransectCount); ++attempt) {
            const double scale = static_cast<double>(transectCount(intersectLineGroups)) / maxTransectCount;
            gridSpacing *= (std::max) (scale * 1.01, 1.01);
            lineList = generateSweepLines(gridSpacing);
            _intersectLinesWithPolygon(lineList, polygon, intersectLineGroups);
        }
        qCWarning(SurveyComplexItemLog) << "Transect spacing" << originalGridSpacing << "raised to" << gridSpacing
                                        << "to limit split transect count to" << maxTransectCount;
    }

    // Less than two transects intersected with the polygon:
    //      Create a single transect which goes through the center of the polygon
    //      Intersect it with the polygon
    if (transectCount(intersectLineGroups) < 2) {
        QLineF firstLine = lineList.first();
        QPointF lineCenter = firstLine.pointAt(0.5);
        QPointF centerOffset = boundingCenter - lineCenter;
        firstLine.translate(centerOffset);
        lineList.clear();
        lineList.append(firstLine);
        _intersectLinesWithPolygon(lineList, polygon, intersectLineGroups);
    }

    if (transectCount(intersectLineGroups) > maxTransectCount) {
        qCWarning(SurveyComplexItemLog) << "Unable to limit split transect count to" << maxTransectCount
                                        << "joining fragments instead";
        joinLineFragments(intersectLineGroups);
        if (transectCount(intersectLineGroups) > maxTransectCount) {
            qCWarning(SurveyComplexItemLog) << "Unable to limit joined transect count to" << maxTransectCount;
            return;
        }
    }

    // Convert from NED to Geo
    GeoTransectGroups transectGroups;
    transectGroups.reserve(intersectLineGroups.size());
    for (const QList<QLineF>& lineGroup : intersectLineGroups) {
        GeoTransectGroup transectGroup;
        transectGroup.reserve(lineGroup.size());
        for (const QLineF& line : lineGroup) {
            QGeoCoordinate coord;
            GeoTransect transect;

            QGCGeo::convertNedToGeo(line.p1().y(), line.p1().x(), 0, tangentOrigin, coord);
            transect.append(coord);
            QGCGeo::convertNedToGeo(line.p2().y(), line.p2().x(), 0, tangentOrigin, coord);
            transect.append(coord);
            transectGroup.append(transect);
        }
        transectGroups.append(transectGroup);
    }

    if (transectGroups.isEmpty()) {
        return;
    }

    if (_entryPoint == EntryLocationBottomLeft || _entryPoint == EntryLocationBottomRight) {
        reverseAllGroupDirections(transectGroups);
    }
    if (_entryPoint == EntryLocationTopRight || _entryPoint == EntryLocationBottomRight) {
        std::reverse(transectGroups.begin(), transectGroups.end());
    }

    if (refly) {
        if (_transects.isEmpty()) {
            return;
        }
        optimizeGroupsForShortestDistance(_transects.last().last().coord, transectGroups);
    }

    if (_flyAlternateTransectsFact.rawValue().toBool()) {
        GeoTransectGroups alternatingGroups;
        alternatingGroups.reserve(transectGroups.size());
        for (int i = 0; i < transectGroups.size(); ++i) {
            if (!(i & 1)) {
                alternatingGroups.append(transectGroups[i]);
            }
        }
        for (int i = transectGroups.size() - 1; i > 0; --i) {
            if (i & 1) {
                alternatingGroups.append(transectGroups[i]);
            }
        }
        transectGroups = alternatingGroups;
    }

    for (qsizetype index = 1; index < transectGroups.size(); index += 2) {
        reverseGroupDirection(transectGroups[index]);
    }

    QList<GeoTransect> transects;
    transects.reserve(transectCount(intersectLineGroups));
    for (const GeoTransectGroup& group : transectGroups) {
        for (const GeoTransect& transect : group) {
            transects.append(transect);
        }
    }

    // Convert to CoordInfo transects and append to _transects
    for (const QList<QGeoCoordinate>& transect : transects) {
        QGeoCoordinate                                  coord;
        QList<TransectStyleComplexItem::CoordInfo_t>    coordInfoTransect;
        TransectStyleComplexItem::CoordInfo_t           coordInfo;

        coordInfo = { transect[0], CoordTypeSurveyEntry };
        coordInfoTransect.append(coordInfo);
        coordInfo = { transect[1], CoordTypeSurveyExit };
        coordInfoTransect.append(coordInfo);

        // For hover and capture we need points for each camera location within the transect
        if (triggerCamera() && hoverAndCaptureEnabled()) {
            double transectLength = transect[0].distanceTo(transect[1]);
            double transectAzimuth = transect[0].azimuthTo(transect[1]);
            if (triggerDistance() < transectLength) {
                int cInnerHoverPoints = static_cast<int>(floor(transectLength / triggerDistance()));
                qCDebug(SurveyComplexItemLog) << "cInnerHoverPoints" << cInnerHoverPoints;
                for (int i=0; i<cInnerHoverPoints; i++) {
                    QGeoCoordinate hoverCoord = transect[0].atDistanceAndAzimuth(triggerDistance() * (i + 1), transectAzimuth);
                    TransectStyleComplexItem::CoordInfo_t hoverCoordInfo = { hoverCoord, CoordTypeInteriorHoverTrigger };
                    coordInfoTransect.insert(1 + i, hoverCoordInfo);
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
            TransectStyleComplexItem::CoordInfo_t turnaroundCoordInfo = { turnaroundCoord, CoordTypeTurnaround };
            coordInfoTransect.prepend(turnaroundCoordInfo);

            azimuth = transect.last().azimuthTo(transect[transect.count() - 2]);
            turnaroundCoord = transect.last().atDistanceAndAzimuth(-turnAroundDistance, azimuth);
            turnaroundCoord.setAltitude(qQNaN());
            coordInfo = { turnaroundCoord, CoordTypeTurnaround };
            coordInfoTransect.append(coordInfo);
        }

        _transects.append(coordInfoTransect);
    }
}


void SurveyComplexItem::_recalcCameraShots(void)
{
    double triggerDistance = this->triggerDistance();

    if (triggerDistance == 0) {
        _cameraShots = 0;
    } else {
        if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
            _cameraShots = qCeil(_complexDistance / triggerDistance);
        } else {
            _cameraShots = 0;

            if (_loadedMissionItemsParent) {
                // We have to do it the hard way based on the mission items themselves
                if (hoverAndCaptureEnabled()) {
                    // Count the number of camera triggers in the mission items
                    for (const MissionItem* missionItem: _loadedMissionItems) {
                        _cameraShots += missionItem->command() == MAV_CMD_IMAGE_START_CAPTURE ? 1 : 0;
                    }
                } else {
                    bool waitingForTriggerStop = false;
                    QGeoCoordinate distanceStartCoord;
                    QGeoCoordinate distanceEndCoord;
                    for (const MissionItem* missionItem: _loadedMissionItems) {
                        if (missionItem->command() == MAV_CMD_NAV_WAYPOINT) {
                            if (waitingForTriggerStop) {
                                distanceEndCoord = QGeoCoordinate(missionItem->param5(), missionItem->param6());
                            } else {
                                distanceStartCoord = QGeoCoordinate(missionItem->param5(), missionItem->param6());
                            }
                        } else if (missionItem->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
                            if (missionItem->param1() > 0) {
                                // Trigger start
                                waitingForTriggerStop = true;
                            } else {
                                // Trigger stop
                                waitingForTriggerStop = false;
                                _cameraShots += qCeil(distanceEndCoord.distanceTo(distanceStartCoord) / triggerDistance);
                                distanceStartCoord = QGeoCoordinate();
                                distanceEndCoord = QGeoCoordinate();
                            }
                        }
                    }

                }
            } else {
                // We have transects available, calc from those
                for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
                    QGeoCoordinate firstCameraCoord, lastCameraCoord;
                    if (_hasTurnaround() && !hoverAndCaptureEnabled()) {
                        firstCameraCoord = transect[1].coord;
                        lastCameraCoord = transect[transect.count() - 2].coord;
                    } else {
                        firstCameraCoord = transect.first().coord;
                        lastCameraCoord = transect.last().coord;
                    }
                    _cameraShots += qCeil(firstCameraCoord.distanceTo(lastCameraCoord) / triggerDistance);
                }
            }
        }
    }

    emit cameraShotsChanged();
}

SurveyComplexItem::ReadyForSaveState SurveyComplexItem::readyForSaveState(void) const
{
    return TransectStyleComplexItem::readyForSaveState();
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
    return _vehicleSpeed == 0 ? 0 : triggerDistance() / _vehicleSpeed;
}

double SurveyComplexItem::additionalTimeDelay (void) const
{
    double hoverTime = 0;

    if (hoverAndCaptureEnabled()) {
        for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
            hoverTime += _hoverAndCaptureDelaySeconds * transect.count();
        }
    }

    return hoverTime;
}

void SurveyComplexItem::_updateWizardMode(void)
{
    if (_surveyAreaPolygon.isValid() && !_surveyAreaPolygon.traceMode()) {
        setWizardMode(false);
    }
}
