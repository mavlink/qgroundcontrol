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
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(SurveyMissionItemLog)

class SurveyMissionItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    SurveyMissionItem(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(Fact*                gridAltitude                READ gridAltitude                   CONSTANT)
    Q_PROPERTY(bool                 gridAltitudeRelative        MEMBER _gridAltitudeRelative        NOTIFY gridAltitudeRelativeChanged)
    Q_PROPERTY(Fact*                gridAngle                   READ gridAngle                      CONSTANT)
    Q_PROPERTY(Fact*                gridSpacing                 READ gridSpacing                    CONSTANT)
    Q_PROPERTY(Fact*                turnaroundDist              READ turnaroundDist                 CONSTANT)
    Q_PROPERTY(bool                 cameraTrigger               MEMBER _cameraTrigger               NOTIFY cameraTriggerChanged)
    Q_PROPERTY(Fact*                cameraTriggerDistance       READ cameraTriggerDistance          CONSTANT)
    Q_PROPERTY(Fact*                groundResolution            READ groundResolution               CONSTANT)
    Q_PROPERTY(Fact*                frontalOverlap              READ frontalOverlap                 CONSTANT)
    Q_PROPERTY(Fact*                sideOverlap                 READ sideOverlap                    CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorWidth           READ cameraSensorWidth              CONSTANT)
    Q_PROPERTY(Fact*                cameraSensorHeight          READ cameraSensorHeight             CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionWidth       READ cameraResolutionWidth          CONSTANT)
    Q_PROPERTY(Fact*                cameraResolutionHeight      READ cameraResolutionHeight         CONSTANT)
    Q_PROPERTY(Fact*                cameraFocalLength           READ cameraFocalLength              CONSTANT)
    Q_PROPERTY(QVariantList         polygonPath                 READ polygonPath                    NOTIFY polygonPathChanged)
    Q_PROPERTY(QVariantList         gridPoints                  READ gridPoints                     NOTIFY gridPointsChanged)
    Q_PROPERTY(int                  cameraShots                 READ cameraShots                    NOTIFY cameraShotsChanged)
    Q_PROPERTY(double               coveredArea                 READ coveredArea                    NOTIFY coveredAreaChanged)
    Q_PROPERTY(bool                 fixedValueIsAltitude        MEMBER _fixedValueIsAltitude        NOTIFY fixedValueIsAltitudeChanged)
    Q_PROPERTY(bool                 cameraOrientationLandscape  MEMBER _cameraOrientationLandscape  NOTIFY cameraOrientationLandscapeChanged)
    Q_PROPERTY(bool                 manualGrid                  MEMBER _manualGrid                  NOTIFY manualGridChanged)
    Q_PROPERTY(QString              camera                      MEMBER _camera                      NOTIFY cameraChanged)
    Q_PROPERTY(double               timeBetweenShots            READ timeBetweenShots               NOTIFY timeBetweenShotsChanged)

    Q_INVOKABLE void clearPolygon(void);
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate coordinate);
    Q_INVOKABLE void adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate);

    QVariantList polygonPath(void) { return _polygonPath; }
    QVariantList gridPoints (void) { return _gridPoints; }

    Fact* gridAltitude              (void) { return &_gridAltitudeFact; }
    Fact* gridAngle                 (void) { return &_gridAngleFact; }
    Fact* gridSpacing               (void) { return &_gridSpacingFact; }
    Fact* turnaroundDist            (void) { return &_turnaroundDistFact; }
    Fact* cameraTriggerDistance     (void) { return &_cameraTriggerDistanceFact; }
    Fact* groundResolution          (void) { return &_groundResolutionFact; }
    Fact* frontalOverlap            (void) { return &_frontalOverlapFact; }
    Fact* sideOverlap               (void) { return &_sideOverlapFact; }
    Fact* cameraSensorWidth         (void) { return &_cameraSensorWidthFact; }
    Fact* cameraSensorHeight        (void) { return &_cameraSensorHeightFact; }
    Fact* cameraResolutionWidth     (void) { return &_cameraResolutionWidthFact; }
    Fact* cameraResolutionHeight    (void) { return &_cameraResolutionHeightFact; }
    Fact* cameraFocalLength         (void) { return &_cameraFocalLengthFact; }

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

    bool coordinateHasRelativeAltitude      (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void setTurnaroundDist  (double dist) { _turnaroundDistFact.setRawValue(dist); }
    void save               (QJsonObject& saveObject) const final;

    static const char* jsonComplexItemTypeValue;

signals:
    void polygonPathChanged                 (void);
    void altitudeChanged                    (double altitude);
    void gridAngleChanged                   (double gridAngle);
    void gridPointsChanged                  (void);
    void cameraTriggerChanged               (bool cameraTrigger);
    void gridAltitudeRelativeChanged        (bool gridAltitudeRelative);
    void cameraShotsChanged                 (int cameraShots);
    void coveredAreaChanged                 (double coveredArea);
    void cameraValueChanged                 (void);
    void fixedValueIsAltitudeChanged        (bool fixedValueIsAltitude);
    void gridTypeChanged                    (QString gridType);
    void cameraOrientationLandscapeChanged  (bool cameraOrientationLandscape);
    void cameraChanged                      (QString camera);
    void manualGridChanged                  (bool manualGrid);
    void timeBetweenShotsChanged            (void);

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

    int             _sequenceNumber;
    bool            _dirty;
    QVariantList    _polygonPath;
    QVariantList    _gridPoints;
    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _exitCoordinate;
    double          _altitude;
    bool            _cameraTrigger;
    bool            _gridAltitudeRelative;
    bool            _manualGrid;
    QString         _camera;
    bool            _cameraOrientationLandscape;
    bool            _fixedValueIsAltitude;

    double          _surveyDistance;
    int             _cameraShots;
    double          _coveredArea;
    double          _timeBetweenShots;
    double          _cruiseSpeed;

    Fact            _gridAltitudeFact;
    Fact            _gridAngleFact;
    Fact            _gridSpacingFact;
    Fact            _turnaroundDistFact;
    Fact            _cameraTriggerDistanceFact;
    Fact            _groundResolutionFact;
    Fact            _frontalOverlapFact;
    Fact            _sideOverlapFact;
    Fact            _cameraSensorWidthFact;
    Fact            _cameraSensorHeightFact;
    Fact            _cameraResolutionWidthFact;
    Fact            _cameraResolutionHeightFact;
    Fact            _cameraFocalLengthFact;

    static QMap<QString, FactMetaData*> _metaDataMap;

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

    static const char* _gridAltitudeFactName;
    static const char* _gridAngleFactName;
    static const char* _gridSpacingFactName;
    static const char* _turnaroundDistFactName;
    static const char* _cameraTriggerDistanceFactName;
    static const char* _groundResolutionFactName;
    static const char* _frontalOverlapFactName;
    static const char* _sideOverlapFactName;
    static const char* _cameraSensorWidthFactName;
    static const char* _cameraSensorHeightFactName;
    static const char* _cameraResolutionWidthFactName;
    static const char* _cameraResolutionHeightFactName;
    static const char* _cameraFocalLengthFactName;
};

#endif
