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
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class GeoViewCamera;

Q_MOC_INCLUDE("GeoViewCamera.h")

/// Owns the scene-space definition shared by everything GeoView renders.
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
    Q_PROPERTY(GeoViewCamera* camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QPointF sceneOrigin READ sceneOrigin NOTIFY sceneOriginChanged)
    Q_PROPERTY(qreal verticalScale READ verticalScale NOTIFY sceneOriginChanged)

public:
    explicit GeoScene(QObject* parent = nullptr);

    static constexpr double kReanchorDistance = 50000.0;  ///< scene-origin re-anchor threshold (m)

    GeoViewCamera* camera() const { return _camera; }

    void setCamera(GeoViewCamera* camera);

    /// Until a camera anchors the scene, the origin is the mercator origin
    /// (0, 0): results stay well-defined but are not precision-safe far from
    /// it. If the camera goes away, the origin retains its last anchor.
    QPointF sceneOrigin() const { return _origin; }

    /// Mercator scale factor at the scene origin: heights and altitudes are
    /// true meters, but scene x/y are mercator meters (stretched by
    /// 1/cos(lat)). Rendering must scale z by this factor or geometry appears
    /// vertically squashed away from the equator.
    qreal verticalScale() const;

    /// Scene position for a geographic coordinate (double math internally;
    /// once a camera has anchored the origin the result is origin-relative
    /// and small enough for floats). Altitude (true meters, NaN treated as 0)
    /// lands on z scaled to scene units.
    Q_INVOKABLE QVector3D scenePositionFor(const QGeoCoordinate& coordinate) const;

    /// Screen position (pixels) of a geographic coordinate through the
    /// camera. Invalid QVariant when there is no camera or the point is at or
    /// behind the camera plane.
    Q_INVOKABLE QVariant screenPositionFor(const QGeoCoordinate& coordinate) const;

signals:
    void cameraChanged();
    void sceneOriginChanged();

private slots:
    void _maybeReanchor();

private:
    GeoViewCamera* _camera = nullptr;
    QPointF _origin;
    bool _originSet = false;
};
