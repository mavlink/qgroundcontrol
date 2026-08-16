/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QVariant>
#include <QtCore/qnumeric.h>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class GeoMapCamera;

Q_MOC_INCLUDE("GeoMapCamera.h")

/// Owns the scene-space definition shared by everything GeoMap renders.
///
/// Scene coordinates are world mercator meters relative to sceneOrigin. The
/// origin re-anchors to the camera center whenever the camera strays more than
/// kReanchorDistance from it, keeping scene coordinates small enough for the
/// renderer's float precision.
///
/// All renderable content (surface patches, and later map items such as
/// vehicles, flight paths, and mission markers) must place itself through this
/// one origin so everything shifts consistently on re-anchor.
class GeoScene : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoMapCamera* camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QPointF sceneOrigin READ sceneOrigin NOTIFY sceneOriginChanged)
    Q_PROPERTY(qreal verticalScale READ verticalScale NOTIFY sceneOriginChanged)
    Q_PROPERTY(qreal terrainScale READ terrainScale WRITE setTerrainScale NOTIFY terrainScaleChanged)
    Q_PROPERTY(QObject* content3D READ content3D WRITE setContent3D NOTIFY content3DChanged)

public:
    explicit GeoScene(QObject* parent = nullptr);

    static constexpr double kReanchorDistance = 50000.0;  ///< scene-origin re-anchor threshold (m)

    GeoMapCamera* camera() const { return _camera; }

    void setCamera(GeoMapCamera* camera);

    /// Until a camera anchors the scene, the origin is the mercator origin
    /// (0, 0): results stay well-defined but are not precision-safe far from
    /// it. If the camera goes away, the origin retains its last anchor.
    QPointF sceneOrigin() const { return _origin; }

    /// Mercator scale factor at the scene origin: heights and altitudes are
    /// true meters, but scene x/y are mercator meters (stretched by
    /// 1/cos(lat)). Rendering must scale z by this factor or geometry appears
    /// vertically squashed away from the equator.
    qreal verticalScale() const;

    /// Terrain displacement factor: 1 in 3D, 0 in 2D (map stays flat).
    /// Everything placing content by altitude must scale z by it to stay on
    /// the rendered surface. Startup mode is 2D, so it starts at 0.
    qreal terrainScale() const { return _terrainScale; }

    void setTerrainScale(qreal scale);

    /// Container node inside the View3D for map item 3D content (a
    /// QQuick3DNode, typed QObject* to keep QtQuick3D out of this header).
    /// Items parent their delegate3D instances here.
    QObject* content3D() const { return _content3D; }

    void setContent3D(QObject* node);

    /// Scene position for a geographic coordinate (double math internally;
    /// once a camera has anchored the origin the result is origin-relative
    /// and small enough for floats). Altitude (true meters, NaN treated as 0)
    /// lands on z scaled to scene units.
    Q_INVOKABLE QVector3D scenePositionFor(const QGeoCoordinate& coordinate) const;

    /// Screen position (pixels) of a geographic coordinate through the
    /// camera, at its rendered height (altitude scaled to scene units and the
    /// terrain scale, matching GeoMapItem placement). Invalid QVariant when
    /// there is no camera or the point is at or behind the camera plane.
    Q_INVOKABLE QVariant screenPositionFor(const QGeoCoordinate& coordinate) const;

    /// Camera center that places coordinate at screenPos, solved at the
    /// coordinate's rendered height (altitude scaled to scene units and the
    /// terrain scale, NaN treated as 0) — the inverse of screenPositionFor.
    /// centerElevation (true meters, scaled to rendered height internally
    /// like altitude) overrides the camera pivot elevation for the solve —
    /// pass the terrain elevation at the destination center when the pivot
    /// follows terrain; NaN keeps the camera's current value.
    /// Invalid coordinate when there is no camera.
    Q_INVOKABLE QGeoCoordinate centerForCoordinateAtScreenPoint(const QGeoCoordinate& coordinate,
                                                                const QPointF& screenPos,
                                                                double centerElevation = qQNaN()) const;

signals:
    void cameraChanged();
    void sceneOriginChanged();
    void terrainScaleChanged();
    void content3DChanged();

private slots:
    void _maybeReanchor();

private:
    GeoMapCamera* _camera = nullptr;
    QPointF _origin;
    bool _originSet = false;
    qreal _terrainScale = 0.0;
    QObject* _content3D = nullptr;
};
