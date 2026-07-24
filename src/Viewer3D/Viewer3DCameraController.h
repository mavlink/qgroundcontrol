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
#include <QtCore/QSizeF>
#include <QtGui/QVector3D>
#include <QtQmlIntegration/QtQmlIntegration>
#include <optional>

/// Gazebo-style orbit camera controller for the 3D viewer.
///
/// Owns the camera pose as (orbitCenter, heading, tilt, distance) and implements
/// cursor-anchored pan, orbit and zoom: the ground point under the cursor at
/// gesture start stays under the cursor for the duration of the gesture.
///
/// Coordinate conventions (scene space, z-up):
///   cameraPosition = orbitCenter + distance * (sin(tilt)sin(heading), -sin(tilt)cos(heading), cos(tilt))
/// tilt 0 = top-down, tilt 90 = horizontal. Angles in degrees.
class Viewer3DCameraController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVector3D orbitCenter READ orbitCenter WRITE setOrbitCenter NOTIFY orbitCenterChanged)
    Q_PROPERTY(qreal heading READ heading WRITE setHeading NOTIFY headingChanged)
    Q_PROPERTY(qreal tilt READ tilt WRITE setTilt NOTIFY tiltChanged)
    Q_PROPERTY(qreal distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(qreal fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)

    friend class Viewer3DCameraControllerTest;

public:
    explicit Viewer3DCameraController(QObject* parent = nullptr);

    static constexpr qreal kMinDistance = 50.0;
    static constexpr qreal kMaxDistance = 50000.0;
    static constexpr qreal kMinTilt = 0.0;
    static constexpr qreal kMaxTilt = 85.0;
    static constexpr qreal kDefaultDistance = 1500.0;
    static constexpr qreal kDefaultFieldOfView = 60.0;

    QVector3D orbitCenter() const { return _orbitCenter; }

    void setOrbitCenter(const QVector3D& center);

    qreal heading() const { return _heading; }

    void setHeading(qreal heading);

    qreal tilt() const { return _tilt; }

    void setTilt(qreal tilt);

    qreal distance() const { return _distance; }

    void setDistance(qreal distance);

    QSizeF viewportSize() const { return _viewportSize; }

    void setViewportSize(const QSizeF& size);

    qreal fieldOfView() const { return _fieldOfView; }

    void setFieldOfView(qreal fov);

    /// Restore the default pose (center origin, heading 0, tilt 0, default distance)
    Q_INVOKABLE void reset();

    /// Set the full pose in one call. tilt/distance are clamped.
    Q_INVOKABLE void lookAt(const QVector3D& center, qreal heading, qreal tilt, qreal distance);

    /// Start a pan gesture at the given screen position (pixels)
    Q_INVOKABLE void beginPan(const QPointF& screenPos);
    /// Continue a pan gesture: the ground point picked at beginPan stays under the cursor
    Q_INVOKABLE void panTo(const QPointF& screenPos);

    /// Start an orbit gesture at the given screen position (pixels)
    Q_INVOKABLE void beginOrbit(const QPointF& screenPos);
    /// Continue an orbit gesture: rotate rigidly about the ground point picked at beginOrbit.
    /// Full viewport width of drag = 360 deg yaw, full height = 180 deg tilt (Gazebo behavior).
    Q_INVOKABLE void orbitTo(const QPointF& screenPos);

    /// Rotate the camera rigidly by degrees about the ground point under screenPos
    /// (positive = counterclockwise heading change). The anchored scene point stays
    /// put on screen. Used for two-finger twist gestures.
    Q_INVOKABLE void rotateBy(qreal degrees, const QPointF& screenPos);

    /// Tilt the camera rigidly by degrees (clamped) about the ground point under
    /// screenPos. The anchored scene point stays put on screen. Used for
    /// two-finger vertical swipe gestures.
    Q_INVOKABLE void tiltBy(qreal degrees, const QPointF& screenPos);

    /// Zoom toward/away from the ground point under screenPos. amount is in wheel
    /// angleDelta units (positive = zoom in). Distance is clamped.
    Q_INVOKABLE void zoom(qreal amount, const QPointF& screenPos);

    /// Scale the camera distance by factor (e.g. 0.5 halves the distance), keeping the
    /// ground point under screenPos fixed on screen. Used for pinch gestures.
    Q_INVOKABLE void zoomBy(qreal factor, const QPointF& screenPos);

    /// Scene units spanned by one horizontal pixel on the ground plane at screen center.
    /// Returns 0 when the viewport size is not set.
    Q_INVOKABLE qreal sceneUnitsPerPixel() const;

    /// Derived camera position in scene space
    QVector3D cameraPosition() const;

    /// Intersect the pick ray through screenPos (pixels) with the ground plane z=0.
    /// Returns std::nullopt when the ray misses (at/above the horizon) or the viewport is not set.
    std::optional<QVector3D> screenToGround(const QPointF& screenPos) const;

signals:
    void orbitCenterChanged();
    void headingChanged();
    void tiltChanged();
    void distanceChanged();
    void viewportSizeChanged();
    void fieldOfViewChanged();

private:
    QVector3D _orbitCenter;
    qreal _heading = 0.0;
    qreal _tilt = 0.0;
    qreal _distance = kDefaultDistance;
    QSizeF _viewportSize;
    qreal _fieldOfView = kDefaultFieldOfView;

    QVector3D _panAnchor;    ///< ground point picked at beginPan
    bool _panAnchorValid = false;
    QVector3D _orbitAnchor;  ///< ground point picked at beginOrbit
    bool _orbitAnchorValid = false;
    QPointF _lastOrbitPos;   ///< last screen position of the orbit gesture

    /// Rigid yaw/pitch rotation about anchor: applies the (clamped) angle deltas
    /// to heading/tilt and moves the orbit center so anchor stays fixed in space.
    void _rotateAboutAnchor(const QVector3D& anchor, qreal headingDelta, qreal tiltDelta);
};
