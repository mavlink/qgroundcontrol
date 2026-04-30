#pragma once

#include <QtQuick/QQuickItem>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtGraphs/QXYSeries>

class MissionController;
class QmlObjectListModel;
class FlightPathSegment;

Q_MOC_INCLUDE("MissionController.h")

class TerrainProfile : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    TerrainProfile(QQuickItem *parent = nullptr);

    Q_PROPERTY(double               visibleWidth        READ visibleWidth   WRITE setVisibleWidth       NOTIFY visibleWidthChanged)
    Q_PROPERTY(MissionController*   missionController   READ missionController  WRITE setMissionController  NOTIFY missionControllerChanged)
    Q_PROPERTY(double               pixelsPerMeter      MEMBER _pixelsPerMeter                              NOTIFY pixelsPerMeterChanged)
    Q_PROPERTY(double               minAMSLAlt          MEMBER _minAMSLAlt                                  NOTIFY minAMSLAltChanged)
    Q_PROPERTY(double               maxAMSLAlt          MEMBER _maxAMSLAlt                                  NOTIFY maxAMSLAltChanged)
    Q_PROPERTY(double               horizontalScale     MEMBER _horizontalScale)
    Q_PROPERTY(double               verticalScale       MEMBER _verticalScale)

    MissionController*  missionController(void) { return _missionController; }
    double              visibleWidth(void) const { return _visibleWidth; }

    void setMissionController(MissionController* missionController);
    void setVisibleWidth(double visibleWidth);

    /// Populates the given series with terrain/flight profile data.
    /// Call this from QML when profileChanged is emitted.
    Q_INVOKABLE void updateSeries(QXYSeries* terrainSeries, QXYSeries* flightSeries, QXYSeries* missingSeries, QXYSeries* collisionSeries);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

signals:
    void missionControllerChanged   (void);
    void visibleWidthChanged        (void);
    void pixelsPerMeterChanged      (void);
    void minAMSLAltChanged          (void);
    void maxAMSLAltChanged          (void);
    void profileChanged             (void);
    void _updateSignal              (void);

private slots:
    void _newVisualItems            (void);
    void _updateProfile             (void);

private:
    void    _updateSegmentCounts            (FlightPathSegment* segment, int& cFlightProfileSegments, int& cTerrainPoints, int& cMissingTerrainSegments, int& cTerrainCollisionSegments, double& minTerrainHeight, double& maxTerrainHeight);
    void    _addTerrainPoints               (FlightPathSegment* segment, double currentDistance, QList<QPointF>& points);
    void    _addFlightPoints                (FlightPathSegment* segment, double currentDistance, QList<QPointF>& points);
    void    _addMissingPoints               (FlightPathSegment* segment, double currentDistance, QList<QPointF>& points);
    void    _addCollisionPoints             (FlightPathSegment* segment, double currentDistance, QList<QPointF>& points);
    bool    _shouldAddFlightProfileSegment  (FlightPathSegment* segment);
    bool    _shouldAddMissingTerrainSegment (FlightPathSegment* segment);

    MissionController*  _missionController =    nullptr;
    QmlObjectListModel* _visualItems =          nullptr;
    double              _visibleWidth =         0;
    double              _pixelsPerMeter =       0;
    double              _minAMSLAlt =           0;
    double              _maxAMSLAlt =           0;
    double              _horizontalScale =      1;
    double              _verticalScale =        1;

    Q_DISABLE_COPY(TerrainProfile)
};
