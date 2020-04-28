/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QQuickItem>
#include <QTimer>
#include <QSGGeometryNode>
#include <QSGGeometry>

class MissionController;
class QmlObjectListModel;
class FlightPathSegment;

class TerrainProfile : public QQuickItem
{
    Q_OBJECT

public:
    TerrainProfile(QQuickItem *parent = nullptr);

    Q_PROPERTY(double               horizontalMargin    MEMBER _horizontalMargin                            NOTIFY horizontalMarginChanged)
    Q_PROPERTY(double               visibleWidth        MEMBER _visibleWidth                                NOTIFY visibleWidthChanged)
    Q_PROPERTY(MissionController*   missionController   READ missionController  WRITE setMissionController  NOTIFY missionControllerChanged)
    Q_PROPERTY(double               pixelsPerMeter      MEMBER _pixelsPerMeter                              NOTIFY pixelsPerMeterChanged)
    Q_PROPERTY(double               minAMSAlt           READ minAMSLAlt                                     NOTIFY minAMSLAltChanged)
    Q_PROPERTY(double               maxAMSAlt           MEMBER _maxAMSLAlt                                  NOTIFY maxAMSLAltChanged)

    MissionController*  missionController   (void) { return _missionController; }
    double              minAMSLAlt          (void);
    double              maxAMSLAlt          (void) { return _maxAMSLAlt; }

    void setMissionController(MissionController* missionController);

    // Overrides from QQuickItem
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* updatePaintNodeData);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

signals:
    void missionControllerChanged   (void);
    void visibleWidthChanged        (void);
    void horizontalMarginChanged    (void);
    void pixelsPerMeterChanged      (void);
    void minAMSLAltChanged          (void);
    void maxAMSLAltChanged          (void);
    void _updateSignal              (void);

private slots:
    void _newVisualItems            (void);

private:
    void    _createGeometry             (QSGGeometryNode*& geometryNode, QSGGeometry*& geometry, int vertices, QSGGeometry::DrawingMode drawingMode, const QColor& color);
    void    _updateSegmentCounts        (FlightPathSegment* segment, int& cTerrainPoints, int& cMissingTerrainSegments, int& cTerrainCollisionSegments, double& maxTerrainHeight);
    void    _addTerrainProfileSegment   (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainVertices, int& terrainVertexIndex);
    void    _addMissingTerrainSegment   (FlightPathSegment* segment, double currentDistance, QSGGeometry::Point2D* missingTerrainVertices, int& missingTerrainVertexIndex);
    void    _addTerrainCollisionSegment (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainCollisionVertices, int& terrainCollisionVertexIndex);
    void    _addFlightProfileSegment    (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* flightProfileVertices, int& flightProfileVertexIndex);
    double  _availableHeight            (void) const;
    void    _setVertex                  (QSGGeometry::Point2D& vertex, double x, double y);

    MissionController*  _missionController =    nullptr;
    QmlObjectListModel* _visualItems =          nullptr;
    double              _visibleWidth =         0;
    double              _horizontalMargin =     0;
    double              _pixelsPerMeter =       0;
    double              _minAMSLAlt =           0;
    double              _maxAMSLAlt =           0;

    static const int _lineWidth =       7;
    static const int _verticalMargin =  0;

    Q_DISABLE_COPY(TerrainProfile)
};

QML_DECLARE_TYPE(TerrainProfile)
