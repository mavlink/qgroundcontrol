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

    const SurveyMissionItem& operator=(const SurveyMissionItem& other);

    Q_PROPERTY(Fact*                gridAltitude            READ gridAltitude               CONSTANT)
    Q_PROPERTY(bool                 gridAltitudeRelative    MEMBER _gridAltitudeRelative    NOTIFY gridAltitudeRelativeChanged)
    Q_PROPERTY(Fact*                gridAngle               READ gridAngle                  CONSTANT)
    Q_PROPERTY(Fact*                gridSpacing             READ gridSpacing                CONSTANT)
    Q_PROPERTY(bool                 cameraTrigger           MEMBER _cameraTrigger           NOTIFY cameraTriggerChanged)
    Q_PROPERTY(Fact*                cameraTriggerDistance   READ cameraTriggerDistance      CONSTANT)
    Q_PROPERTY(QVariantList         polygonPath             READ polygonPath                NOTIFY polygonPathChanged)
    Q_PROPERTY(QVariantList         gridPoints              READ gridPoints                 NOTIFY gridPointsChanged)
    Q_PROPERTY(int                  cameraShots             READ cameraShots                NOTIFY cameraShotsChanged)
    Q_PROPERTY(double               coveredArea             READ coveredArea                NOTIFY coveredAreaChanged)

    Q_INVOKABLE void clearPolygon(void);
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate coordinate);
    Q_INVOKABLE void adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate);

    QVariantList polygonPath(void) { return _polygonPath; }
    QVariantList gridPoints (void) { return _gridPoints; }

    Fact* gridAltitude(void)    { return &_gridAltitudeFact; }
    Fact* gridAngle(void)       { return &_gridAngleFact; }
    Fact* gridSpacing(void)     { return &_gridSpacingFact; }
    Fact* cameraTriggerDistance(void) { return &_cameraTriggerDistanceFact; }

    int     cameraShots(void) const;
    double  coveredArea(void) const { return _coveredArea; }

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final { return _surveyDistance; }
    int                 lastSequenceNumber  (void) const final;
    QmlObjectListModel* getMissionItems     (void) const final;
    bool                load                (const QJsonObject& complexObject, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;

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

    bool coordinateHasRelativeAltitude      (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return _gridAltitudeRelative; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonObject& saveObject) const final;

signals:
    void polygonPathChanged             (void);
    void altitudeChanged                (double altitude);
    void gridAngleChanged               (double gridAngle);
    void gridPointsChanged              (void);
    void cameraTriggerChanged           (bool cameraTrigger);
    void gridAltitudeRelativeChanged    (bool gridAltitudeRelative);
    void cameraShotsChanged             (int cameraShots);
    void coveredAreaChanged             (double coveredArea);

private slots:
    void _cameraTriggerChanged(void);

private:
    void _clear(void);
    void _setExitCoordinate(const QGeoCoordinate& coordinate);
    void _clearGrid(void);
    void _generateGrid(void);
    void _gridGenerator(const QList<QPointF>& polygonPoints, QList<QPointF>& gridPoints);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    void _setSurveyDistance(double surveyDistance);
    void _setCameraShots(int cameraShots);
    void _setCoveredArea(double coveredArea);

    int                 _sequenceNumber;
    bool                _dirty;
    QVariantList        _polygonPath;
    QVariantList        _gridPoints;
    QGeoCoordinate      _coordinate;
    QGeoCoordinate      _exitCoordinate;
    double              _altitude;
    double              _gridAngle;
    bool                _cameraTrigger;
    bool                _gridAltitudeRelative;

    double              _surveyDistance;
    int                 _cameraShots;
    double              _coveredArea;

    Fact            _gridAltitudeFact;
    Fact            _gridAngleFact;
    Fact            _gridSpacingFact;
    Fact            _cameraTriggerDistanceFact;
    FactMetaData    _gridAltitudeMetaData;
    FactMetaData    _gridAngleMetaData;
    FactMetaData    _gridSpacingMetaData;
    FactMetaData    _cameraTriggerDistanceMetaData;

    static const char* _jsonVersionKey;
    static const char* _jsonTypeKey;
    static const char* _jsonPolygonKey;
    static const char* _jsonIdKey;
    static const char* _jsonGridAltitudeKey;
    static const char* _jsonGridAltitudeRelativeKey;
    static const char* _jsonGridAngleKey;
    static const char* _jsonGridSpacingKey;
    static const char* _jsonCameraTriggerKey;
    static const char* _jsonCameraTriggerDistanceKey;

    static const char* _complexType;
};

#endif
