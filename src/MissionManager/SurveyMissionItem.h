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
    Q_PROPERTY(Fact*                turnaroundDist              READ turnaroundDist                 CONSTANT)
    Q_PROPERTY(Fact*                cameraTrigger               READ cameraTrigger                  CONSTANT)
    Q_PROPERTY(Fact*                cameraTriggerDistance       READ cameraTriggerDistance          CONSTANT)
    Q_PROPERTY(Fact*                groundResolution            READ groundResolution               CONSTANT)
    Q_PROPERTY(Fact*                frontalOverlap              READ frontalOverlap                 CONSTANT)
    Q_PROPERTY(Fact*                sideOverlap                 READ sideOverlap                    CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorWidth           READ cameraSensorWidth              CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorHeight          READ cameraSensorHeight             CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionWidth       READ cameraResolutionWidth          CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionHeight      READ cameraResolutionHeight         CONSTANT)
    Q_PROPERTY(Fact*                cameraFocalLength           READ cameraFocalLength              CONSTANT)
    Q_PROPERTY(Fact*                fixedValueIsAltitude        READ fixedValueIsAltitude           CONSTANT)
    Q_PROPERTY(Fact*                cameraOrientationLandscape  READ cameraOrientationLandscape     CONSTANT)
    Q_PROPERTY(Fact*                manualGrid                  READ manualGrid                     CONSTANT)
    Q_PROPERTY(Fact*                camera                      READ camera                         CONSTANT)

    Q_PROPERTY(bool                 cameraOrientationFixed      MEMBER _cameraOrientationFixed      NOTIFY cameraOrientationFixedChanged)

    Q_PROPERTY(double               timeBetweenShots            READ timeBetweenShots               NOTIFY timeBetweenShotsChanged)
    Q_PROPERTY(QVariantList         polygonPath                 READ polygonPath                    NOTIFY polygonPathChanged)
    Q_PROPERTY(QVariantList         gridPoints                  READ gridPoints                     NOTIFY gridPointsChanged)
    Q_PROPERTY(int                  cameraShots                 READ cameraShots                    NOTIFY cameraShotsChanged)
    Q_PROPERTY(double               coveredArea                 READ coveredArea                    NOTIFY coveredAreaChanged)

    // The polygon vertices are also exposed as a list mode since MapItemView will only work with a QAbstractItemModel as
    // opposed to polygonPath which is a QVariantList.
    Q_PROPERTY(QmlObjectListModel*  polygonModel                READ polygonModel                   CONSTANT)

    Q_INVOKABLE void clearPolygon(void);
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate coordinate);
    Q_INVOKABLE void adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate);
    Q_INVOKABLE void removePolygonVertex(int vertexIndex);

    // Splits the segment between vertextIndex and the next vertex in half
    Q_INVOKABLE void splitPolygonSegment(int vertexIndex);

    QVariantList        polygonPath (void) { return _polygonPath; }
    QmlObjectListModel* polygonModel(void) { return &_polygonModel; }

    QVariantList gridPoints (void) { return _gridPoints; }

    Fact* manualGrid                (void) { return &_manualGridFact; }
    Fact* gridAltitude              (void) { return &_gridAltitudeFact; }
    Fact* gridAltitudeRelative      (void) { return &_gridAltitudeRelativeFact; }
    Fact* gridAngle                 (void) { return &_gridAngleFact; }
    Fact* gridSpacing               (void) { return &_gridSpacingFact; }
    Fact* turnaroundDist            (void) { return &_turnaroundDistFact; }
    Fact* cameraTrigger             (void) { return &_cameraTriggerFact; }
    Fact* cameraTriggerDistance     (void) { return &_cameraTriggerDistanceFact; }
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

    int     cameraShots(void) const;
    double  coveredArea(void) const { return _coveredArea; }
    double  timeBetweenShots(void) const;

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final { return _surveyDistance; }
    int                 lastSequenceNumber  (void) const final;
    QmlObjectListModel* getMissionItems     (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    void                setCruiseSpeed      (double cruiseSpeed) final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("SurveyMapVisual.qml"); }


    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    QString         commandDescription      (void) const final { return "Survey"; }
    QString         commandName             (void) const final { return "Survey"; }
    QString         abbreviation            (void) const final { return "S"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          flightSpeed             (void) final { return std::numeric_limits<double>::quiet_NaN(); }

    bool coordinateHasRelativeAltitude      (void) const final { return _gridAltitudeRelativeFact.rawValue().toBool(); }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _gridAltitudeRelativeFact.rawValue().toBool(); }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void setTurnaroundDist  (double dist) { _turnaroundDistFact.setRawValue(dist); }
    void save               (QJsonArray&  missionItems) const final;

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* manualGridName;
    static const char* gridAltitudeName;
    static const char* gridAltitudeRelativeName;
    static const char* gridAngleName;
    static const char* gridSpacingName;
    static const char* turnaroundDistName;
    static const char* cameraTriggerDistanceName;
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
    void polygonPathChanged             (void);
    void gridPointsChanged              (void);
    void cameraShotsChanged             (int cameraShots);
    void coveredAreaChanged             (double coveredArea);
    void cameraValueChanged             (void);
    void gridTypeChanged                (QString gridType);
    void timeBetweenShotsChanged        (void);
    void cameraOrientationFixedChanged  (bool cameraOrientationFixed);

private slots:
    void _cameraTriggerChanged(void);

private:
    void _clear(void);
    void _setExitCoordinate(const QGeoCoordinate& coordinate);
    void _clearGrid(void);
    void _generateGrid(void);
    void _updateCoordinateAltitude(void);
    void _gridGenerator(const QList<QPointF>& polygonPoints, QList<QPointF>& gridPoints);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    void _setSurveyDistance(double surveyDistance);
    void _setCameraShots(int cameraShots);
    void _setCoveredArea(double coveredArea);
    void _cameraValueChanged(void);

    int                 _sequenceNumber;
    bool                _dirty;
    QVariantList        _polygonPath;
    QmlObjectListModel  _polygonModel;
    QVariantList        _gridPoints;
    QGeoCoordinate      _coordinate;
    QGeoCoordinate      _exitCoordinate;
    bool                _cameraOrientationFixed;

    double          _surveyDistance;
    int             _cameraShots;
    double          _coveredArea;
    double          _timeBetweenShots;
    double          _cruiseSpeed;

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact    _manualGridFact;
    SettingsFact    _gridAltitudeFact;
    SettingsFact    _gridAltitudeRelativeFact;
    SettingsFact    _gridAngleFact;
    SettingsFact    _gridSpacingFact;
    SettingsFact    _turnaroundDistFact;
    SettingsFact    _cameraTriggerFact;
    SettingsFact    _cameraTriggerDistanceFact;
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

    static const char* _jsonPolygonObjectKey;
    static const char* _jsonGridObjectKey;
    static const char* _jsonGridAltitudeKey;
    static const char* _jsonGridAltitudeRelativeKey;
    static const char* _jsonGridAngleKey;
    static const char* _jsonGridSpacingKey;
    static const char* _jsonTurnaroundDistKey;
    static const char* _jsonCameraTriggerKey;
    static const char* _jsonCameraTriggerDistanceKey;
    static const char* _jsonGroundResolutionKey;
    static const char* _jsonFrontalOverlapKey;
    static const char* _jsonSideOverlapKey;
    static const char* _jsonCameraSensorWidthKey;
    static const char* _jsonCameraSensorHeightKey;
    static const char* _jsonCameraResolutionWidthKey;
    static const char* _jsonCameraResolutionHeightKey;
    static const char* _jsonCameraFocalLengthKey;
    static const char* _jsonManualGridKey;
    static const char* _jsonCameraObjectKey;
    static const char* _jsonCameraNameKey;
    static const char* _jsonCameraOrientationLandscapeKey;
    static const char* _jsonFixedValueIsAltitudeKey;
};

#endif
