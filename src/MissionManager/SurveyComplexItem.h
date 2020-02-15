/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TransectStyleComplexItem.h"
#include "MissionItem.h"
#include "SettingsFact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(SurveyComplexItemLog)

class SurveyComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param vehicle Vehicle which this is being contructed for
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlOrShpFile Polygon comes from this file, empty for default polygon
    SurveyComplexItem(Vehicle* vehicle, bool flyView, const QString& kmlOrShpFile, QObject* parent);

    Q_PROPERTY(Fact* gridAngle              READ gridAngle              CONSTANT)
    Q_PROPERTY(Fact* flyAlternateTransects  READ flyAlternateTransects  CONSTANT)
    Q_PROPERTY(Fact* splitConcavePolygons   READ splitConcavePolygons   CONSTANT)

    Fact* gridAngle             (void) { return &_gridAngleFact; }
    Fact* flyAlternateTransects (void) { return &_flyAlternateTransectsFact; }
    Fact* splitConcavePolygons  (void) { return &_splitConcavePolygonsFact; }

    Q_INVOKABLE void rotateEntryPoint(void);

    // Overrides from ComplexMissionItem
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString mapVisualQML        (void) const final { return QStringLiteral("SurveyMapVisual.qml"); }
    QString presetsSettingsGroup(void) { return settingsGroup; }
    void    savePreset          (const QString& name);
    void    loadPreset          (const QString& name);

    // Overrides from TransectStyleComplexItem
    void    save                (QJsonArray&  planItems) final;
    bool    specifiesCoordinate (void) const final { return true; }
    void    appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void    applyNewAltitude    (double newAltitude) final;
    double  timeBetweenShots    (void) final;

    // Overrides from VisualMissionionItem
    QString             commandDescription  (void) const final { return tr("Survey"); }
    QString             commandName         (void) const final { return tr("Survey"); }
    QString             abbreviation        (void) const final { return tr("S"); }
    ReadyForSaveState   readyForSaveState    (void) const final;
    double              additionalTimeDelay (void) const final;

    // Must match json spec for GridEntryLocation
    enum EntryLocation {
        EntryLocationFirst,
        EntryLocationTopLeft = EntryLocationFirst,
        EntryLocationTopRight,
        EntryLocationBottomLeft,
        EntryLocationBottomRight,
        EntryLocationLast = EntryLocationBottomRight
    };

    static const char* jsonComplexItemTypeValue;
    static const char* settingsGroup;
    static const char* gridAngleName;
    static const char* gridEntryLocationName;
    static const char* flyAlternateTransectsName;
    static const char* splitConcavePolygonsName;

    static const char* jsonV3ComplexItemTypeValue;

signals:
    void refly90DegreesChanged(bool refly90Degrees);

private slots:
    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1    (void) final;
    void _recalcComplexDistance     (void) final;
    void _recalcCameraShots         (void) final;

private:
    enum CameraTriggerCode {
        CameraTriggerNone,
        CameraTriggerOn,
        CameraTriggerOff,
        CameraTriggerHoverAndCapture
    };

    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    int _appendWaypointToMission(QList<MissionItem*>& items, int seqNum, QGeoCoordinate& coord, CameraTriggerCode cameraTrigger, QObject* missionItemParent);
    bool _nextTransectCoord(const QList<QGeoCoordinate>& transectPoints, int pointIndex, QGeoCoordinate& coord);
    bool _appendMissionItemsWorker(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, bool hasRefly, bool buildRefly);
    void _optimizeTransectsForShortestDistance(const QGeoCoordinate& distanceCoord, QList<QList<QGeoCoordinate>>& transects);
    qreal _ccw(QPointF pt1, QPointF pt2, QPointF pt3);
    qreal _dp(QPointF pt1, QPointF pt2);
    void _swapPoints(QList<QPointF>& points, int index1, int index2);
    void _reverseTransectOrder(QList<QList<QGeoCoordinate>>& transects);
    void _reverseInternalTransectPoints(QList<QList<QGeoCoordinate>>& transects);
    void _adjustTransectsToEntryPointLocation(QList<QList<QGeoCoordinate>>& transects);
    bool _gridAngleIsNorthSouthTransects();
    double _clampGridAngle90(double gridAngle);
    void _buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent);
    void _appendLoadedMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent);
    bool _imagesEverywhere(void) const;
    bool _triggerCamera(void) const;
    bool _hasTurnaround(void) const;
    double _turnaroundDistance(void) const;
    bool _hoverAndCaptureEnabled(void) const;
    bool _loadV3(const QJsonObject& complexObject, int sequenceNumber, QString& errorString);
    bool _loadV4V5(const QJsonObject& complexObject, int sequenceNumber, QString& errorString, int version, bool forPresets);
    void _saveWorker(QJsonObject& complexObject);
    void _rebuildTransectsPhase1Worker(bool refly);
    void _rebuildTransectsPhase1WorkerSinglePolygon(bool refly);
    void _rebuildTransectsPhase1WorkerSplitPolygons(bool refly);
    /// Adds to the _transects array from one polygon
    void _rebuildTransectsFromPolygon(bool refly, const QPolygonF& polygon, const QGeoCoordinate& tangentOrigin, const QPointF* const transitionPoint);
    // Decompose polygon into list of convex sub polygons
    void _PolygonDecomposeConvex(const QPolygonF& polygon, QList<QPolygonF>& decomposedPolygons);
    // return true if vertex a can see vertex b
    bool _VertexCanSeeOther(const QPolygonF& polygon, const QPointF* vertexA, const QPointF* vertexB);
    bool _VertexIsReflex(const QPolygonF& polygon, const QPointF* vertex);

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact    _gridAngleFact;
    SettingsFact    _flyAlternateTransectsFact;
    SettingsFact    _splitConcavePolygonsFact;
    int             _entryPoint;

    static const char* _jsonGridAngleKey;
    static const char* _jsonEntryPointKey;
    static const char* _jsonFlyAlternateTransectsKey;
    static const char* _jsonSplitConcavePolygonsKey;

    static const char* _jsonV3GridObjectKey;
    static const char* _jsonV3GridAltitudeKey;
    static const char* _jsonV3GridAltitudeRelativeKey;
    static const char* _jsonV3GridAngleKey;
    static const char* _jsonV3GridSpacingKey;
    static const char* _jsonV3EntryPointKey;
    static const char* _jsonV3TurnaroundDistKey;
    static const char* _jsonV3CameraTriggerDistanceKey;
    static const char* _jsonV3CameraTriggerInTurnaroundKey;
    static const char* _jsonV3HoverAndCaptureKey;
    static const char* _jsonV3GroundResolutionKey;
    static const char* _jsonV3FrontalOverlapKey;
    static const char* _jsonV3SideOverlapKey;
    static const char* _jsonV3CameraSensorWidthKey;
    static const char* _jsonV3CameraSensorHeightKey;
    static const char* _jsonV3CameraResolutionWidthKey;
    static const char* _jsonV3CameraResolutionHeightKey;
    static const char* _jsonV3CameraFocalLengthKey;
    static const char* _jsonV3CameraMinTriggerIntervalKey;
    static const char* _jsonV3ManualGridKey;
    static const char* _jsonV3CameraObjectKey;
    static const char* _jsonV3CameraNameKey;
    static const char* _jsonV3CameraOrientationLandscapeKey;
    static const char* _jsonV3FixedValueIsAltitudeKey;
    static const char* _jsonV3Refly90DegreesKey;


    static const int _hoverAndCaptureDelaySeconds = 4;
};
