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

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(TerrainProfileLog)

class MissionController;
class QmlObjectListModel;
class FlightPathSegment;

class TerrainProfile : public QQuickItem
{
    Q_OBJECT

public:
    TerrainProfile(QQuickItem *parent = nullptr);

    Q_PROPERTY(double               visibleWidth        MEMBER _visibleWidth                                NOTIFY visibleWidthChanged)
    Q_PROPERTY(MissionController*   missionController   READ missionController  WRITE setMissionController  NOTIFY missionControllerChanged)
    Q_PROPERTY(double               pixelsPerMeter      MEMBER _pixelsPerMeter                              NOTIFY pixelsPerMeterChanged)
    Q_PROPERTY(double               minAMSLAlt          MEMBER _minAMSLAlt                                  NOTIFY minAMSLAltChanged)
    Q_PROPERTY(double               maxAMSLAlt          MEMBER _maxAMSLAlt                                  NOTIFY maxAMSLAltChanged)

    MissionController*  missionController(void) { return _missionController; }

    void setMissionController(MissionController* missionController);

    // Overrides from QQuickItem
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* updatePaintNodeData);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

signals:
    void missionControllerChanged   (void);
    void visibleWidthChanged        (void);
    void pixelsPerMeterChanged      (void);
    void minAMSLAltChanged          (void);
    void maxAMSLAltChanged          (void);
    void _updateSignal              (void);

private slots:
    void _newVisualItems            (void);

private:
    void    _createGeometry                 (QSGGeometryNode*& geometryNode, QSGGeometry*& geometry, QSGGeometry::DrawingMode drawingMode, const QColor& color);
    void    _updateSegmentCounts            (FlightPathSegment* segment, int& cFlightProfileSegments, int& cTerrainPoints, int& cMissingTerrainSegments, int& cTerrainCollisionSegments, double& minTerrainHeight, double& maxTerrainHeight);
    void    _addTerrainProfileSegment       (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainProfileVertices, int& terrainVertexIndex);
    void    _addMissingTerrainSegment       (FlightPathSegment* segment, double currentDistance, QSGGeometry::Point2D* missingTerrainVertices, int& missingTerrainVertexIndex);
    void    _addTerrainCollisionSegment     (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainCollisionVertices, int& terrainCollisionVertexIndex);
    void    _addFlightProfileSegment        (FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* flightProfileVertices, int& flightProfileVertexIndex);
    bool    _shouldAddFlightProfileSegment  (FlightPathSegment* segment);
    bool    _shouldAddMissingTerrainSegment (FlightPathSegment* segment);

    MissionController*  _missionController =    nullptr;
    QmlObjectListModel* _visualItems =          nullptr;
    double              _visibleWidth =         0;
    double              _pixelsPerMeter =       0;
    double              _minAMSLAlt =           0;
    double              _maxAMSLAlt =           0;

    static const int _lineWidth =       7;

    Q_DISABLE_COPY(TerrainProfile)
};

QML_DECLARE_TYPE(TerrainProfile)
