#include "SurveyComplexItem.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QLineF>
#include <QtGui/QPainterPath>
#include <QtGui/QPainterPathStroker>
#include <QtGui/QPolygonF>

#include "AppMessages.h"
#include "AppSettings.h"
#include "GeoFenceController.h"
#include "JsonParsing.h"
#include "MissionItem.h"
#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "QGCFenceCircle.h"
#include "QGCFencePolygon.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {

constexpr double GEOMETRY_TOLERANCE = 1e-5;
constexpr double OBSTACLE_CLEARANCE = 2.0;
constexpr double ROUTING_TOLERANCE = 0.1;

QPainterPath polygonPath(const QPolygonF& polygon)
{
    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

QPainterPath expandedArea(const QPainterPath& area, double distance)
{
    if (distance <= 0) {
        return area;
    }
    QPainterPathStroker stroker;
    stroker.setWidth(distance * 2);
    stroker.setJoinStyle(Qt::MiterJoin);
    return area.united(stroker.createStroke(area));
}

class SurveyPathPlanner
{
public:
    explicit SurveyPathPlanner(const QPainterPath& area)
        : _area(area)
    {
        for (const QPolygonF& polygon : area.toSubpathPolygons()) {
            for (int i = 1; i < polygon.count(); ++i) {
                const QLineF edge(polygon[i - 1], polygon[i]);
                if (edge.length() > GEOMETRY_TOLERANCE) {
                    _edges.append(edge);
                    if (!_vertices.contains(edge.p1())) {
                        _vertices.append(edge.p1());
                    }
                }
            }
        }
    }

    QList<QLineF> clip(const QLineF& line) const
    {
        QList<QLineF> result;
        const QPointF direction = line.p2() - line.p1();
        const double lengthSquared = QPointF::dotProduct(direction, direction);
        if (lengthSquared < GEOMETRY_TOLERANCE * GEOMETRY_TOLERANCE) {
            return result;
        }
        QList<double> parameters{0, 1};
        const auto addParameter = [&](const QPointF& point) {
            const double t = QPointF::dotProduct(point - line.p1(), direction) / lengthSquared;
            if (t > 0 && t < 1) {
                parameters.append(t);
            }
        };
        for (const QLineF& edge : _edges) {
            QPointF intersection;
            if (line.intersects(edge, &intersection) == QLineF::BoundedIntersection) {
                addParameter(intersection);
            } else {
                // Collinear edges have no unique intersection in QLineF.
                if (_distanceToLine(edge.p1(), line) < GEOMETRY_TOLERANCE) {
                    addParameter(edge.p1());
                }
                if (_distanceToLine(edge.p2(), line) < GEOMETRY_TOLERANCE) {
                    addParameter(edge.p2());
                }
            }
        }
        std::sort(parameters.begin(), parameters.end());
        const double parameterTolerance = GEOMETRY_TOLERANCE / line.length();
        parameters.erase(
            std::unique(parameters.begin(), parameters.end(),
                        [parameterTolerance](double a, double b) { return std::abs(a - b) < parameterTolerance; }),
            parameters.end());
        for (int i = 1; i < parameters.count(); ++i) {
            if (_contains(line.pointAt((parameters[i - 1] + parameters[i]) * 0.5))) {
                const QLineF part(line.pointAt(parameters[i - 1]), line.pointAt(parameters[i]));
                if (!result.isEmpty() && QLineF(result.last().p2(), part.p1()).length() < GEOMETRY_TOLERANCE) {
                    result.last().setP2(part.p2());
                } else {
                    result.append(part);
                }
            }
        }
        return result;
    }

    QList<QPointF> shortestPath(const QPointF& start, const QPointF& end)
    {
        if (_clear(QLineF(start, end))) {
            return {start, end};
        }
        if (!_graphBuilt) {
            _buildGraph();
        }
        const double infinity = std::numeric_limits<double>::infinity();
        QList<double> distances(_vertices.count(), infinity);
        QList<double> toEnd(_vertices.count(), infinity);
        QList<int> previous(_vertices.count(), -1);
        QList<bool> visited(_vertices.count(), false);
        for (int i = 0; i < _vertices.count(); ++i) {
            const QLineF fromStart(start, _vertices[i]);
            const QLineF toGoal(_vertices[i], end);
            if (_clear(fromStart)) {
                distances[i] = fromStart.length();
            }
            if (_clear(toGoal)) {
                toEnd[i] = toGoal.length();
            }
        }
        double bestDistance = infinity;
        int goal = -1;
        for (int step = 0; step < _vertices.count(); ++step) {
            int current = -1;
            for (int i = 0; i < _vertices.count(); ++i) {
                if (!visited[i] && (current < 0 || distances[i] < distances[current])) {
                    current = i;
                }
            }
            if (current < 0 || !std::isfinite(distances[current]) || distances[current] >= bestDistance) {
                break;
            }
            visited[current] = true;
            if (distances[current] + toEnd[current] < bestDistance) {
                bestDistance = distances[current] + toEnd[current];
                goal = current;
            }
            for (const auto& [next, cost] : _graph[current]) {
                if (distances[current] + cost < distances[next]) {
                    distances[next] = distances[current] + cost;
                    previous[next] = current;
                }
            }
        }
        if (goal < 0) {
            return {};
        }
        QList<QPointF> result{end};
        for (int index = goal; index >= 0; index = previous[index]) {
            result.prepend(_vertices[index]);
        }
        result.prepend(start);
        return result;
    }

private:
    static double _distanceToLine(const QPointF& point, const QLineF& line)
    {
        const QPointF direction = line.p2() - line.p1();
        const double lengthSquared = QPointF::dotProduct(direction, direction);
        const double t = lengthSquared > 0
                             ? std::clamp(QPointF::dotProduct(point - line.p1(), direction) / lengthSquared, 0.0, 1.0)
                             : 0;
        return QLineF(point, line.pointAt(t)).length();
    }

    bool _contains(const QPointF& point) const
    {
        if (_area.contains(point)) {
            return true;
        }
        return std::any_of(_edges.cbegin(), _edges.cend(),
                           [&point](const QLineF& edge) { return _distanceToLine(point, edge) < GEOMETRY_TOLERANCE; });
    }

    bool _clear(const QLineF& line) const
    {
        if (line.length() < GEOMETRY_TOLERANCE) {
            return _contains(line.p1());
        }
        const QList<QLineF> parts = clip(line);
        return parts.count() == 1 && QLineF(parts.first().p1(), line.p1()).length() < GEOMETRY_TOLERANCE &&
               QLineF(parts.first().p2(), line.p2()).length() < GEOMETRY_TOLERANCE;
    }

    void _buildGraph()
    {
        _graph.resize(_vertices.count());
        for (int i = 0; i < _vertices.count(); ++i) {
            for (int j = i + 1; j < _vertices.count(); ++j) {
                const QLineF line(_vertices[i], _vertices[j]);
                if (_clear(line)) {
                    _graph[i].append({j, line.length()});
                    _graph[j].append({i, line.length()});
                }
            }
        }
        _graphBuilt = true;
    }

    QPainterPath _area;
    QList<QLineF> _edges;
    QList<QPointF> _vertices;
    QList<QList<std::pair<int, double>>> _graph;
    bool _graphBuilt = false;
};

}  // namespace

QGC_LOGGING_CATEGORY(SurveyComplexItemLog, "Plan.SurveyComplexItem")

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

    connect(&_gridAngleFact,            &Fact::rawValueChanged,                     this, &SurveyComplexItem::_setDirty);
    connect(&_flyAlternateTransectsFact,&Fact::rawValueChanged,                     this, &SurveyComplexItem::_setDirty);
    connect(&_splitConcavePolygonsFact, &Fact::rawValueChanged,                     this, &SurveyComplexItem::_setDirty);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_setDirty);

    connect(&_gridAngleFact,            &Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(&_flyAlternateTransectsFact,&Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(&_splitConcavePolygonsFact, &Fact::valueChanged,                        this, &SurveyComplexItem::_rebuildTransects);
    connect(this,                       &SurveyComplexItem::refly90DegreesChanged,  this, &SurveyComplexItem::_rebuildTransects);

    connect(&_surveyAreaPolygon,        &QGCMapPolygon::isValidChanged,             this, &SurveyComplexItem::_updateWizardMode);
    connect(&_surveyAreaPolygon,        &QGCMapPolygon::traceModeChanged,           this, &SurveyComplexItem::_updateWizardMode);

    connect(this, &SurveyComplexItem::lastSequenceNumberChanged, this, &SurveyComplexItem::waypointMarkersChanged);
    connect(this, &SurveyComplexItem::visualTransectPointsChanged, this, &SurveyComplexItem::waypointMarkersChanged);

    if (!flyView) {
        GeoFenceController* fences = masterController->geoFenceController();
        connect(fences->polygons(), &QmlObjectListModel::countChanged, this,
                &SurveyComplexItem::_updateObstacleConnections);
        connect(fences->circles(), &QmlObjectListModel::countChanged, this,
                &SurveyComplexItem::_updateObstacleConnections);
        _updateObstacleConnections();
    }

    if (!kmlOrShpFile.isEmpty()) {
        _surveyAreaPolygon.loadKMLOrSHPFile(kmlOrShpFile);
        _surveyAreaPolygon.setDirty(false);
    }
    setDirty(false);
}

QVariantList SurveyComplexItem::waypointMarkers()
{
    QObject missionItemParent;
    QList<MissionItem*> items;
    appendMissionItems(items, &missionItemParent);

    QVariantList markers;
    for (const MissionItem* item : items) {
        if (item->command() == MAV_CMD_NAV_WAYPOINT || item->command() == MAV_CMD_CONDITION_GATE) {
            markers.append(QVariantMap{{QStringLiteral("coordinate"), QVariant::fromValue(item->coordinate())},
                                       {QStringLiteral("sequenceNumber"), item->sequenceNumber()}});
        }
    }
    return markers;
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
        QGC::showAppMessage(tr("Internal Error: Preset load failed. Name: %1 Error: %2").arg(presetName).arg(errorString));
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

    if (!_flyView && _hasExclusionFences()) {
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

/// Reverse the order of the transects. First transect becomes last and so forth.
void SurveyComplexItem::_reverseTransectOrder(QList<QList<QGeoCoordinate>>& transects)
{
    QList<QList<QGeoCoordinate>> rgReversedTransects;
    for (int i=transects.count() - 1; i>=0; i--) {
        rgReversedTransects.append(transects[i]);
    }
    transects = rgReversedTransects;
}

/// Reverse the order of all points within each transect, First point becomes last and so forth.
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

        auto isLineBoundedIntersect = [&line, &intersectPoint](const QLineF& linePosition) {
            return line.intersects(linePosition, &intersectPoint) == QLineF::BoundedIntersection;
        };

        int foundCount = 0;
        if (isLineBoundedIntersect(topLine)) {
            intersectLine.setP1(intersectPoint);
            foundCount++;
        }
        if (isLineBoundedIntersect(rightLine)) {
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
        if (isLineBoundedIntersect(bottomLine)) {
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
        if (isLineBoundedIntersect(leftLine)) {
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

            for (int intersectionIndex=0; intersectionIndex<intersections.count(); intersectionIndex++) {
                for (int compareIndex=0; compareIndex<intersections.count(); compareIndex++) {
                    QLineF lineTest(intersections[intersectionIndex], intersections[compareIndex]);
                    double newMaxDistance = lineTest.length();
                    if (newMaxDistance > currentMaxDistance) {
                        firstPoint = intersections[intersectionIndex];
                        secondPoint = intersections[compareIndex];
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
    const QString previousError = _obstacleError;
    _obstacleError.clear();
    _obstacleAvoidanceActive = false;
    _rebuildTransectsPhase1WorkerSinglePolygon(false /* refly */);
    if (_refly90DegreesFact.rawValue().toBool() && !_transects.isEmpty()) {
        _rebuildTransectsPhase1WorkerSinglePolygon(true /* refly */);
    }
    _avoidObstacles();
    if (previousError != _obstacleError) {
        emit obstacleErrorChanged();
        emit readyForSaveStateChanged();
    }
}

bool SurveyComplexItem::_hasExclusionFences() const
{
    GeoFenceController* fences = _masterController->geoFenceController();
    for (int i = 0; i < fences->polygons()->count(); ++i) {
        const auto* polygon = fences->polygons()->value<QGCFencePolygon*>(i);
        if (polygon && !polygon->inclusion()) {
            return true;
        }
    }
    for (int i = 0; i < fences->circles()->count(); ++i) {
        const auto* circle = fences->circles()->value<QGCFenceCircle*>(i);
        if (circle && !circle->inclusion()) {
            return true;
        }
    }
    return false;
}

void SurveyComplexItem::_updateObstacleConnections()
{
    for (const QMetaObject::Connection& connection : _obstacleConnections) {
        disconnect(connection);
    }
    _obstacleConnections.clear();
    GeoFenceController* fences = _masterController->geoFenceController();
    for (int i = 0; i < fences->polygons()->count(); ++i) {
        auto* polygon = fences->polygons()->value<QGCFencePolygon*>(i);
        if (polygon) {
            _obstacleConnections.append(
                connect(polygon, &QGCMapPolygon::pathChanged, this, &SurveyComplexItem::_obstaclesChanged));
            _obstacleConnections.append(
                connect(polygon, &QGCFencePolygon::inclusionChanged, this, &SurveyComplexItem::_obstaclesChanged));
        }
    }
    for (int i = 0; i < fences->circles()->count(); ++i) {
        auto* circle = fences->circles()->value<QGCFenceCircle*>(i);
        if (circle) {
            _obstacleConnections.append(
                connect(circle, &QGCMapCircle::centerChanged, this, &SurveyComplexItem::_obstaclesChanged));
            _obstacleConnections.append(
                connect(circle->radius(), &Fact::rawValueChanged, this, &SurveyComplexItem::_obstaclesChanged));
            _obstacleConnections.append(
                connect(circle, &QGCFenceCircle::inclusionChanged, this, &SurveyComplexItem::_obstaclesChanged));
        }
    }
    _obstaclesChanged();
}

void SurveyComplexItem::_obstaclesChanged()
{
    if (!_ignoreRecalc && (_obstacleAvoidanceActive || _hasExclusionFences())) {
        _rebuildTransects();
        setDirty(true);
    }
}

void SurveyComplexItem::_avoidObstacles()
{
    if (_flyView || _transects.isEmpty() || !_hasExclusionFences()) {
        return;
    }
    const QGeoCoordinate origin = _surveyAreaPolygon.coordinateList().first();
    const auto toLocal = [&origin](const QGeoCoordinate& coordinate) {
        if (coordinate.latitude() == origin.latitude() && coordinate.longitude() == origin.longitude()) {
            return QPointF();
        }
        double north, east, down;
        QGCGeo::convertGeoToNed(coordinate, origin, north, east, down);
        return QPointF(east, north);
    };
    const auto toGeo = [&origin](const QPointF& point) {
        QGeoCoordinate coordinate;
        QGCGeo::convertNedToGeo(point.y(), point.x(), 0, origin, coordinate);
        return coordinate;
    };
    QPolygonF surveyPolygon;
    for (const QGeoCoordinate& coordinate : _surveyAreaPolygon.coordinateList()) {
        surveyPolygon.append(toLocal(coordinate));
    }
    const QPainterPath surveyArea = polygonPath(surveyPolygon);
    const double turnaround = qMax(0.0, _turnaroundDistance());
    const QPainterPath flightArea = expandedArea(surveyArea, turnaround);
    QPainterPath obstacles;
    GeoFenceController* fences = _masterController->geoFenceController();
    for (int i = 0; i < fences->polygons()->count(); ++i) {
        const auto* fence = fences->polygons()->value<QGCFencePolygon*>(i);
        if (!fence || fence->inclusion() || !fence->isValid()) {
            continue;
        }
        QPolygonF polygon;
        for (const QGeoCoordinate& coordinate : fence->coordinateList()) {
            polygon.append(toLocal(coordinate));
        }
        obstacles = obstacles.united(expandedArea(polygonPath(polygon), OBSTACLE_CLEARANCE + ROUTING_TOLERANCE));
    }
    for (int i = 0; i < fences->circles()->count(); ++i) {
        auto* fence = fences->circles()->value<QGCFenceCircle*>(i);
        if (!fence || fence->inclusion() || !fence->center().isValid()) {
            continue;
        }
        const double radius = fence->radius()->rawValue().toDouble();
        if (!std::isfinite(radius) || radius <= 0) {
            continue;
        }
        // A circumscribed polygon keeps every chord outside the circular fence and clearance.
        constexpr int CIRCLE_SIDES = 8;
        const double expandedRadius = (radius + OBSTACLE_CLEARANCE + ROUTING_TOLERANCE) / std::cos(M_PI / CIRCLE_SIDES);
        const double rotation = M_PI / CIRCLE_SIDES - _gridAngleFact.rawValue().toDouble() * M_PI / 180;
        const QPointF center = toLocal(fence->center());
        QPolygonF polygon;
        for (int vertex = 0; vertex < CIRCLE_SIDES; ++vertex) {
            const double angle = rotation + 2 * M_PI * vertex / CIRCLE_SIDES;
            polygon.append(center + QPointF(std::cos(angle), std::sin(angle)) * expandedRadius);
        }
        obstacles = obstacles.united(polygonPath(polygon));
    }
    if (!obstacles.intersects(flightArea)) {
        return;
    }
    _obstacleAvoidanceActive = true;
    SurveyPathPlanner coveragePlanner(surveyArea.subtracted(obstacles));
    SurveyPathPlanner routingPlanner(flightArea.subtracted(obstacles));

    struct CoverageSegment
    {
        QLineF coverage;
        QLineF flight;
    };

    QList<CoverageSegment> segments;
    for (const QList<CoordInfo_t>& transect : _transects) {
        const auto entry = std::find_if(transect.cbegin(), transect.cend(), [](const CoordInfo_t& point) {
            return point.coordType == CoordTypeSurveyEntry;
        });
        const auto exit = std::find_if(transect.cbegin(), transect.cend(),
                                       [](const CoordInfo_t& point) { return point.coordType == CoordTypeSurveyExit; });
        if (entry == transect.cend() || exit == transect.cend()) {
            continue;
        }
        for (const QLineF& coverage : coveragePlanner.clip(QLineF(toLocal(entry->coord), toLocal(exit->coord)))) {
            QLineF flight = coverage;
            if (turnaround > 0) {
                const QPointF extension = (coverage.p2() - coverage.p1()) * (turnaround / coverage.length());
                const QList<QLineF> before = routingPlanner.clip(QLineF(coverage.p1() - extension, coverage.p1()));
                if (!before.isEmpty() && QLineF(before.last().p2(), coverage.p1()).length() < GEOMETRY_TOLERANCE) {
                    flight.setP1(before.last().p1());
                }
                const QList<QLineF> after = routingPlanner.clip(QLineF(coverage.p2(), coverage.p2() + extension));
                if (!after.isEmpty() && QLineF(after.first().p1(), coverage.p2()).length() < GEOMETRY_TOLERANCE) {
                    flight.setP2(after.first().p2());
                }
            }
            segments.append({coverage, flight});
            if (segments.count() > maxTransectCount) {
                _obstacleError = tr("Exclusion fences create too many survey segments. Increase the transect spacing.");
                _transects.clear();
                return;
            }
        }
    }
    QList<QList<CoordInfo_t>> planned;
    QPointF current;
    for (const CoverageSegment& segment : segments) {
        if (!planned.isEmpty()) {
            const QList<QPointF> path = routingPlanner.shortestPath(current, segment.flight.p1());
            if (path.isEmpty()) {
                _obstacleError = tr("No safe path connects the survey transects around the exclusion fences.");
                _transects.clear();
                return;
            }
            for (int i = 1; i + 1 < path.count(); ++i) {
                planned.last().append({toGeo(path[i]), CoordTypeInterior});
            }
        }
        QList<CoordInfo_t> transect;
        if (turnaround > 0) {
            transect.append({toGeo(segment.flight.p1()), CoordTypeTurnaround});
        }
        const QGeoCoordinate entry = toGeo(segment.coverage.p1());
        const QGeoCoordinate exit = toGeo(segment.coverage.p2());
        transect.append({entry, CoordTypeSurveyEntry});
        if (triggerCamera() && hoverAndCaptureEnabled()) {
            const double length = entry.distanceTo(exit);
            const double azimuth = entry.azimuthTo(exit);
            for (double distance = triggerDistance(); distance < length; distance += triggerDistance()) {
                transect.append({entry.atDistanceAndAzimuth(distance, azimuth), CoordTypeInteriorHoverTrigger});
            }
        }
        transect.append({exit, CoordTypeSurveyExit});
        if (turnaround > 0) {
            transect.append({toGeo(segment.flight.p2()), CoordTypeTurnaround});
        }
        planned.append(transect);
        current = segment.flight.p2();
    }
    if (planned.isEmpty()) {
        _obstacleError = tr("The exclusion fences leave no survey area to fly.");
    }
    _transects = planned;
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

    QList<QLineF> lineList;

    // Sweep lines must extend beyond the polygon boundary regardless of grid angle.
    // The worst case is when the polygon is rotated 45° relative to the sweep direction,
    // where the required reach equals half the diagonal of the bounding rect.
    // We use diagonal * 1.5 to provide a 50% safety margin beyond that worst case.
    // Note: the old fixed +2000m was insufficient for polygons larger than ~10km.
    const double diagonal = qSqrt(boundingRect.width() * boundingRect.width() + boundingRect.height() * boundingRect.height());
    double maxWidth = diagonal * 1.5;
    if (maxWidth <= 0.0) {
        qCWarning(SurveyComplexItemLog) << "Degenerate polygon bounding rect (all vertices coincident or collinear), aborting transect rebuild";
        return;
    }

    if (gridSpacing <= 0) {
        // Invalid spacing: seed one center line so the < 2 fallback produces a single center transect.
        qCWarning(SurveyComplexItemLog) << "Grid spacing" << gridSpacing << "is invalid, falling back to single center transect";
        const double halfW = maxWidth / 2.0;
        lineList += QLineF(
            _rotatePoint(QPointF(boundingCenter.x(), boundingCenter.y() - halfW), boundingCenter, gridAngle),
            _rotatePoint(QPointF(boundingCenter.x(), boundingCenter.y() + halfW), boundingCenter, gridAngle));
    } else {
        // Cap spacing so the sweep never generates more than maxTransectCount transects.
        // Uses diagonal (not maxWidth) so the count reflects actual polygon-crossing transects.
        if (gridSpacing < diagonal / maxTransectCount) {
            qCWarning(SurveyComplexItemLog) << "Transect spacing" << gridSpacing << "raised to" << diagonal / maxTransectCount << "to limit transect count to" << maxTransectCount;
            gridSpacing = diagonal / maxTransectCount;
        }

        double halfWidth = maxWidth / 2.0;
        double transectX = boundingCenter.x() - halfWidth;
        double transectXMax = transectX + maxWidth;
        while (transectX < transectXMax) {
            double transectYTop = boundingCenter.y() - halfWidth;
            double transectYBottom = boundingCenter.y() + halfWidth;

            lineList += QLineF(_rotatePoint(QPointF(transectX, transectYTop), boundingCenter, gridAngle), _rotatePoint(QPointF(transectX, transectYBottom), boundingCenter, gridAngle));
            transectX += gridSpacing;
        }
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
    for (const QLineF& line : resultLines) {
        QGeoCoordinate          coord;
        QList<QGeoCoordinate>   transect;

        QGCGeo::convertNedToGeo(line.p1().y(), line.p1().x(), 0, tangentOrigin, coord);
        transect.append(coord);
        QGCGeo::convertNedToGeo(line.p2().y(), line.p2().x(), 0, tangentOrigin, coord);
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
                    if (_obstacleAvoidanceActive) {
                        int hoverShots = 0;
                        for (const CoordInfo_t& point : transect) {
                            if (point.coordType == CoordTypeSurveyEntry) {
                                firstCameraCoord = point.coord;
                            } else if (point.coordType == CoordTypeSurveyExit) {
                                lastCameraCoord = point.coord;
                            }
                            if (point.coordType == CoordTypeSurveyEntry || point.coordType == CoordTypeSurveyExit ||
                                point.coordType == CoordTypeInteriorHoverTrigger) {
                                ++hoverShots;
                            }
                        }
                        if (hoverAndCaptureEnabled()) {
                            _cameraShots += hoverShots;
                            continue;
                        }
                    } else if (_hasTurnaround() && !hoverAndCaptureEnabled()) {
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
    if (!_obstacleError.isEmpty()) {
        return NotReadyForSaveData;
    }
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
            if (_obstacleAvoidanceActive) {
                for (const CoordInfo_t& point : transect) {
                    if (point.coordType == CoordTypeSurveyEntry || point.coordType == CoordTypeSurveyExit ||
                        point.coordType == CoordTypeInteriorHoverTrigger) {
                        hoverTime += _hoverAndCaptureDelaySeconds;
                    }
                }
            } else {
                hoverTime += _hoverAndCaptureDelaySeconds * transect.count();
            }
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
