/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SurveyMissionItem_H
#define SurveyMissionItem_H

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "SettingsFact.h"
#include "QGCLoggingCategory.h"
#include "QGCMapPolygon.h"

Q_DECLARE_LOGGING_CATEGORY(SurveyMissionItemLog)

class SurveyMissionItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    SurveyMissionItem(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(Fact*                gridAltitude                READ gridAltitude                   CONSTANT)
    Q_PROPERTY(Fact*                gridAltitudeRelative        READ gridAltitudeRelative           CONSTANT)
    Q_PROPERTY(Fact*                gridAngle                   READ gridAngle                      CONSTANT)
    Q_PROPERTY(Fact*                gridSpacing                 READ gridSpacing                    CONSTANT)
    Q_PROPERTY(Fact*                gridEntryLocation           READ gridEntryLocation              CONSTANT)
    Q_PROPERTY(Fact*                turnaroundDist              READ turnaroundDist                 CONSTANT)
    Q_PROPERTY(Fact*                cameraTriggerDistance       READ cameraTriggerDistance          CONSTANT)
    Q_PROPERTY(Fact*                cameraTriggerInTurnaround   READ cameraTriggerInTurnaround      CONSTANT)
    Q_PROPERTY(Fact*                hoverAndCapture             READ hoverAndCapture                CONSTANT)
    Q_PROPERTY(Fact*                groundResolution            READ groundResolution               CONSTANT)
    Q_PROPERTY(Fact*                frontalOverlap              READ frontalOverlap                 CONSTANT)
    Q_PROPERTY(Fact*                sideOverlap                 READ sideOverlap                    CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorWidth           READ cameraSensorWidth              CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorHeight          READ cameraSensorHeight             CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionWidth       READ cameraResolutionWidth          CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionHeight      READ cameraResolutionHeight         CONSTANT)
    Q_PROPERTY(Fact*                cameraFocalLength           READ cameraFocalLength              CONSTANT)
    Q_PROPERTY(Fact*                cameraOrientationLandscape  READ cameraOrientationLandscape     CONSTANT)
    Q_PROPERTY(Fact*                fixedValueIsAltitude        READ fixedValueIsAltitude           CONSTANT)
    Q_PROPERTY(Fact*                manualGrid                  READ manualGrid                     CONSTANT)
    Q_PROPERTY(Fact*                camera                      READ camera                         CONSTANT)

    Q_PROPERTY(bool                 cameraOrientationFixed      MEMBER _cameraOrientationFixed      NOTIFY cameraOrientationFixedChanged)
    Q_PROPERTY(bool                 hoverAndCaptureAllowed      READ hoverAndCaptureAllowed         CONSTANT)
    Q_PROPERTY(bool                 refly90Degrees              READ refly90Degrees WRITE setRefly90Degrees NOTIFY refly90DegreesChanged)
    Q_PROPERTY(double               cameraMinTriggerInterval    MEMBER _cameraMinTriggerInterval    NOTIFY cameraMinTriggerIntervalChanged)

    Q_PROPERTY(double               timeBetweenShots            READ timeBetweenShots               NOTIFY timeBetweenShotsChanged)
    Q_PROPERTY(QVariantList         gridPoints                  READ gridPoints                     NOTIFY gridPointsChanged)
    Q_PROPERTY(int                  cameraShots                 READ cameraShots                    NOTIFY cameraShotsChanged)
    Q_PROPERTY(double               coveredArea                 READ coveredArea                    NOTIFY coveredAreaChanged)

    Q_PROPERTY(QGCMapPolygon*       mapPolygon                  READ mapPolygon                     CONSTANT)

    QVariantList gridPoints (void) { return _simpleGridPoints; }

    Fact* manualGrid                (void) { return &_manualGridFact; }
    Fact* gridAltitude              (void) { return &_gridAltitudeFact; }
    Fact* gridAltitudeRelative      (void) { return &_gridAltitudeRelativeFact; }
    Fact* gridAngle                 (void) { return &_gridAngleFact; }
    Fact* gridSpacing               (void) { return &_gridSpacingFact; }
    Fact* gridEntryLocation         (void) { return &_gridEntryLocationFact; }
    Fact* turnaroundDist            (void) { return &_turnaroundDistFact; }
    Fact* cameraTriggerDistance     (void) { return &_cameraTriggerDistanceFact; }
    Fact* cameraTriggerInTurnaround (void) { return &_cameraTriggerInTurnaroundFact; }
    Fact* hoverAndCapture           (void) { return &_hoverAndCaptureFact; }
    Fact* groundResolution          (void) { return &_groundResolutionFact; }
    Fact* frontalOverlap            (void) { return &_frontalOverlapFact; }
    Fact* sideOverlap               (void) { return &_sideOverlapFact; }
    Fact* cameraSensorWidth         (void) { return &_cameraSensorWidthFact; }
    Fact* cameraSensorHeight        (void) { return &_cameraSensorHeightFact; }
    Fact* cameraResolutionWidth     (void) { return &_cameraResolutionWidthFact; }
    Fact* cameraResolutionHeight    (void) { return &_cameraResolutionHeightFact; }
    Fact* cameraFocalLength         (void) { return &_cameraFocalLengthFact; }
    Fact* cameraOrientationLandscape(void) { return &_cameraOrientationLandscapeFact; }
    Fact* fixedValueIsAltitude      (void) { return &_fixedValueIsAltitudeFact; }
    Fact* camera                    (void) { return &_cameraFact; }

    int             cameraShots             (void) const;
    double          coveredArea             (void) const { return _coveredArea; }
    double          timeBetweenShots        (void) const;
    bool            hoverAndCaptureAllowed  (void) const;
    bool            refly90Degrees          (void) const { return _refly90Degrees; }
    QGCMapPolygon*  mapPolygon              (void) { return &_mapPolygon; }

    void setRefly90Degrees(bool refly90Degrees);

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final { return _surveyDistance; }
    QGCGeoBoundingCube  boundingCube        (void) const final { return _boundingCube; }
    double              additionalTimeDelay (void) const final { return _additionalFlightDelaySeconds; }
    int                 lastSequenceNumber  (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("SurveyMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QString         commandDescription      (void) const final { return "Survey"; }
    QString         commandName             (void) const final { return "Survey"; }
    QString         abbreviation            (void) const final { return "S"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedFlightSpeed    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalYaw      (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalPitch    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            setMissionFlightStatus  (MissionController::MissionFlightStatus_t& missionFlightStatus) final;
    void            applyNewAltitude        (double newAltitude) final;

    bool coordinateHasRelativeAltitude      (void) const final { return _gridAltitudeRelativeFact.rawValue().toBool(); }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _gridAltitudeRelativeFact.rawValue().toBool(); }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void setTurnaroundDist  (double dist) { _turnaroundDistFact.setRawValue(dist); }
    void save               (QJsonArray&  missionItems) final;

    // Must match json spec for GridEntryLocation
    enum EntryLocation {
        EntryLocationTopLeft,
        EntryLocationTopRight,
        EntryLocationBottomLeft,
        EntryLocationBottomRight,
    };

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* manualGridName;
    static const char* gridAltitudeName;
    static const char* gridAltitudeRelativeName;
    static const char* gridAngleName;
    static const char* gridSpacingName;
    static const char* gridEntryLocationName;
    static const char* turnaroundDistName;
    static const char* cameraTriggerDistanceName;
    static const char* cameraTriggerInTurnaroundName;
    static const char* hoverAndCaptureName;
    static const char* groundResolutionName;
    static const char* frontalOverlapName;
    static const char* sideOverlapName;
    static const char* cameraSensorWidthName;
    static const char* cameraSensorHeightName;
    static const char* cameraResolutionWidthName;
    static const char* cameraResolutionHeightName;
    static const char* cameraFocalLengthName;
    static const char* cameraTriggerName;
    static const char* cameraOrientationLandscapeName;
    static const char* fixedValueIsAltitudeName;
    static const char* cameraName;

signals:
    void gridPointsChanged                  (void);
    void cameraShotsChanged                 (int cameraShots);
    void coveredAreaChanged                 (double coveredArea);
    void cameraValueChanged                 (void);
    void gridTypeChanged                    (QString gridType);
    void timeBetweenShotsChanged            (void);
    void cameraOrientationFixedChanged      (bool cameraOrientationFixed);
    void refly90DegreesChanged              (bool refly90Degrees);
    void cameraMinTriggerIntervalChanged    (double cameraMinTriggerInterval);

private slots:
    void _setDirty(void);
    void _polygonDirtyChanged(bool dirty);
    void _clearInternal(void);

private:
    enum CameraTriggerCode {
        CameraTriggerNone,
        CameraTriggerOn,
        CameraTriggerOff,
        CameraTriggerHoverAndCapture
    };

    void _setExitCoordinate(const QGeoCoordinate& coordinate);
    void _generateGrid(void);
    void _updateCoordinateAltitude(void);
    int _gridGenerator(const QList<QPointF>& polygonPoints, QList<QList<QPointF>>& transectSegments, bool refly);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    void _setSurveyDistance(double surveyDistance);
    void _setBoundingCube(QGCGeoBoundingCube bc);
    void _setCameraShots(int cameraShots);
    void _setCoveredArea(double coveredArea);
    void _cameraValueChanged(void);
    int _appendWaypointToMission(QList<MissionItem*>& items, int seqNum, QGeoCoordinate& coord, CameraTriggerCode cameraTrigger, QObject* missionItemParent);
    bool _nextTransectCoord(const QList<QGeoCoordinate>& transectPoints, int pointIndex, QGeoCoordinate& coord);
    double _triggerDistance(void) const;
    bool _triggerCamera(void) const;
    bool _imagesEverywhere(void) const;
    bool _hoverAndCaptureEnabled(void) const;
    bool _hasTurnaround(void) const;
    double _turnaroundDistance(void) const;
    void _convertTransectToGeo(const QList<QList<QPointF>>& transectSegmentsNED, const QGeoCoordinate& tangentOrigin, QList<QList<QGeoCoordinate>>& transectSegmentsGeo);
    bool _appendMissionItemsWorker(QList<MissionItem*>& items, QObject* missionItemParent, int& seqNum, bool hasRefly, bool buildRefly);
    void _optimizeTransectsForShortestDistance(const QGeoCoordinate& distanceCoord, QList<QList<QGeoCoordinate>>& transects);
    void _appendGridPointsFromTransects(QList<QList<QGeoCoordinate>>& rgTransectSegments);
    qreal _ccw(QPointF pt1, QPointF pt2, QPointF pt3);
    qreal _dp(QPointF pt1, QPointF pt2);
    void _swapPoints(QList<QPointF>& points, int index1, int index2);
    void _reverseTransectOrder(QList<QList<QGeoCoordinate>>& transects);
    void _reverseInternalTransectPoints(QList<QList<QGeoCoordinate>>& transects);
    void _adjustTransectsToEntryPointLocation(QList<QList<QGeoCoordinate>>& transects);
    bool _gridAngleIsNorthSouthTransects();
    double _clampGridAngle90(double gridAngle);
    int _calcMissionCommandCount(QList<QList<QGeoCoordinate>>& transectSegments);
    void _calcBoundingCube();

    int                             _sequenceNumber;
    bool                            _dirty;
    QGCMapPolygon                   _mapPolygon;
    QVariantList                    _simpleGridPoints;      ///< Grid points for drawing simple grid visuals
    QList<QList<QGeoCoordinate>>    _transectSegments;      ///< Internal transect segments including grid exit, turnaround and internal camera points
    QList<QList<QGeoCoordinate>>    _reflyTransectSegments; ///< Refly segments
    QGeoCoordinate                  _coordinate;
    QGeoCoordinate                  _exitCoordinate;
    bool                            _cameraOrientationFixed;
    int                             _missionCommandCount;
    bool                            _refly90Degrees;
    double                          _additionalFlightDelaySeconds;
    double                          _cameraMinTriggerInterval;

    bool            _ignoreRecalc;
    double          _surveyDistance;
    int             _cameraShots;
    double          _coveredArea;
    double          _timeBetweenShots;
    double          _cruiseSpeed;

    QGCGeoBoundingCube _boundingCube;
    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact    _manualGridFact;
    SettingsFact    _gridAltitudeFact;
    SettingsFact    _gridAltitudeRelativeFact;
    SettingsFact    _gridAngleFact;
    SettingsFact    _gridSpacingFact;
    SettingsFact    _gridEntryLocationFact;
    SettingsFact    _turnaroundDistFact;
    SettingsFact    _cameraTriggerDistanceFact;
    SettingsFact    _cameraTriggerInTurnaroundFact;
    SettingsFact    _hoverAndCaptureFact;
    SettingsFact    _groundResolutionFact;
    SettingsFact    _frontalOverlapFact;
    SettingsFact    _sideOverlapFact;
    SettingsFact    _cameraSensorWidthFact;
    SettingsFact    _cameraSensorHeightFact;
    SettingsFact    _cameraResolutionWidthFact;
    SettingsFact    _cameraResolutionHeightFact;
    SettingsFact    _cameraFocalLengthFact;
    SettingsFact    _cameraOrientationLandscapeFact;
    SettingsFact    _fixedValueIsAltitudeFact;
    SettingsFact    _cameraFact;

    static const char* _jsonGridObjectKey;
    static const char* _jsonGridAltitudeKey;
    static const char* _jsonGridAltitudeRelativeKey;
    static const char* _jsonGridAngleKey;
    static const char* _jsonGridSpacingKey;
    static const char* _jsonGridEntryLocationKey;
    static const char* _jsonTurnaroundDistKey;
    static const char* _jsonCameraTriggerDistanceKey;
    static const char* _jsonCameraTriggerInTurnaroundKey;
    static const char* _jsonHoverAndCaptureKey;
    static const char* _jsonGroundResolutionKey;
    static const char* _jsonFrontalOverlapKey;
    static const char* _jsonSideOverlapKey;
    static const char* _jsonCameraSensorWidthKey;
    static const char* _jsonCameraSensorHeightKey;
    static const char* _jsonCameraResolutionWidthKey;
    static const char* _jsonCameraResolutionHeightKey;
    static const char* _jsonCameraFocalLengthKey;
    static const char* _jsonCameraMinTriggerIntervalKey;
    static const char* _jsonManualGridKey;
    static const char* _jsonCameraObjectKey;
    static const char* _jsonCameraNameKey;
    static const char* _jsonCameraOrientationLandscapeKey;
    static const char* _jsonFixedValueIsAltitudeKey;
    static const char* _jsonRefly90DegreesKey;

    static const int _hoverAndCaptureDelaySeconds = 4;
};

#endif
