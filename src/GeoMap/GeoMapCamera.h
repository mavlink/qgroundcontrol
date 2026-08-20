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
#include <QtCore/qnumeric.h>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include <optional>

/// Camera pose model for the GeoMap engine.
///
/// Owns the pose as (center, heading, tilt, distance) where center is a geographic
/// coordinate on the ground plane. Implements cursor-anchored pan, orbit, rotate,
/// tilt and zoom: the ground point under the cursor at gesture start stays under
/// the cursor for the duration of the gesture.
///
/// Scene space is Web Mercator meters (TileMath), z up, ground plane z=0.
/// Camera position convention (matches the Viewer3D orbit camera):
///   cameraPosition = center + distance * (sin(tilt)sin(heading), -sin(tilt)cos(heading), cos(tilt))
/// tilt 0 = top-down, tilt kMaxTilt = most oblique 3D view. Angles in degrees.
class GeoMapCamera : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(qreal heading READ heading WRITE setHeading NOTIFY headingChanged)
    Q_PROPERTY(qreal tilt READ tilt WRITE setTilt NOTIFY tiltChanged)
    Q_PROPERTY(qreal distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(qreal centerElevation READ centerElevation WRITE setCenterElevation NOTIFY centerElevationChanged)
    Q_PROPERTY(QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(qreal fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)
    Q_PROPERTY(qreal verticalFieldOfView READ verticalFieldOfView NOTIFY verticalFieldOfViewChanged)
    Q_PROPERTY(qreal unitsPerPixelAtUnitDistance READ unitsPerPixelAtUnitDistance NOTIFY unitsPerPixelAtUnitDistanceChanged)
    Q_PROPERTY(bool isTopDown READ isTopDown NOTIFY tiltChanged)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(qreal default3DTilt READ default3DTilt CONSTANT)
    Q_PROPERTY(QPointF sceneOrigin READ sceneOrigin WRITE setSceneOrigin NOTIFY scenePoseChanged)
    Q_PROPERTY(QVector3D scenePosition READ scenePosition NOTIFY scenePoseChanged)
    Q_PROPERTY(QQuaternion sceneRotation READ sceneRotation NOTIFY scenePoseChanged)

public:
    explicit GeoMapCamera(QObject* parent = nullptr);

    /// Map mode: 2D locks tilt against gestures (the map stays top-down until
    /// the user explicitly switches to 3D). setTilt itself is not gated so the
    /// QML-side mode-transition animation can drive it.
    enum class Mode
    {
        Mode2D,
        Mode3D
    };
    Q_ENUM(Mode)

    static constexpr qreal kMinDistance = 10.0;
    static constexpr qreal kMaxDistance = 40000000.0;
    static constexpr qreal kMinTilt = 0.0;
    static constexpr qreal kMaxTilt = 85.0;
    static constexpr qreal kDefaultDistance = 1500.0;
    // Applied to the larger viewport axis (see verticalFieldOfView). Moderate
    // angle: wide-angle values skew level circles into tilted ellipses at the
    // screen edges (off-axis rectilinear distortion)
    static constexpr qreal kDefaultFieldOfView = 45.0;
    static constexpr qreal kDefault3DTilt = 55.0;

    QGeoCoordinate center() const;
    void setCenter(const QGeoCoordinate& center);

    /// False until the camera is explicitly positioned (setCenter/lookAt). The
    /// construction-default pose is null island; consumers must not treat it as
    /// a real location (e.g. by fetching terrain there).
    bool isPositioned() const { return _positioned; }

    qreal heading() const { return _heading; }

    void setHeading(qreal heading);

    qreal tilt() const { return _tilt; }

    void setTilt(qreal tilt);

    qreal distance() const { return _distance; }

    void setDistance(qreal distance);

    /// World-z of the look-at point (scene units). The camera orbits distance
    /// meters around the center at this height; normally driven from the
    /// terrain height at the center so close-zoom 3D never dives under the
    /// rendered surface. Ground-plane math (screenToGround, pan/zoom anchors)
    /// still intersects z=0, with rays cast from the elevated camera.
    qreal centerElevation() const { return _centerElevation; }

    void setCenterElevation(qreal elevation);

    QSizeF viewportSize() const { return _viewportSize; }

    void setViewportSize(const QSizeF& size);

    qreal fieldOfView() const { return _fieldOfView; }

    void setFieldOfView(qreal fov);

    /// fieldOfView applied to the larger viewport axis (Cesium-style): on wide
    /// viewports the vertical FOV narrows instead of the horizontal FOV
    /// exploding into fisheye distortion. All projection math uses this.
    qreal verticalFieldOfView() const;

    /// Scene units spanned by one pixel per unit of camera distance; multiply
    /// by a point's camera distance for its units-per-pixel (constant apparent
    /// size scaling). Returns 0 when the viewport size is not set.
    qreal unitsPerPixelAtUnitDistance() const;

    /// Pose predicate: the camera currently looks straight down (tilt ~0).
    /// Distinct from mode(): Mode2D can be mid-transition with tilt > 0, and a
    /// Mode3D user can gesture tilt down to 0.
    bool isTopDown() const { return qFuzzyIsNull(_tilt); }

    Mode mode() const { return _mode; }

    void setMode(Mode mode);

    qreal default3DTilt() const { return kDefault3DTilt; }

    /// Restore the default pose at the current center (heading 0, tilt 0, default distance)
    Q_INVOKABLE void reset();

    /// Set the full pose in one call. tilt/distance are clamped.
    Q_INVOKABLE void lookAt(const QGeoCoordinate& center, qreal heading, qreal tilt, qreal distance);

    /// Instant 2D<->3D switch: sets the mode and snaps tilt (the animated
    /// transition is driven QML-side via mode + tilt/terrain animations)
    Q_INVOKABLE void goTo2D()
    {
        setMode(Mode::Mode2D);
        setTilt(kMinTilt);
    }

    Q_INVOKABLE void goTo3D()
    {
        setMode(Mode::Mode3D);
        setTilt(kDefault3DTilt);
    }

    /// Start a pan gesture at the given screen position (pixels). anchorZ is the
    /// world-z (scene units) of the grabbed surface point: gestures must anchor to
    /// the rendered terrain surface, not the z=0 ground plane, or the map slides
    /// off the cursor over elevated terrain.
    Q_INVOKABLE void beginPan(const QPointF& screenPos, double anchorZ = 0.0);
    /// Continue a pan gesture: the surface point picked at beginPan stays under the cursor
    Q_INVOKABLE void panTo(const QPointF& screenPos);

    /// Start an orbit gesture at the given screen position (pixels); anchorZ as in beginPan
    Q_INVOKABLE void beginOrbit(const QPointF& screenPos, double anchorZ = 0.0);
    /// Continue an orbit gesture: rotate rigidly about the surface point picked at beginOrbit,
    /// which stays pinned to its on-screen position from gesture start.
    /// Full viewport width of drag = 360 deg heading, full height = 180 deg tilt.
    Q_INVOKABLE void orbitTo(const QPointF& screenPos);

    /// Start a first-person look gesture at the given screen position (pixels)
    Q_INVOKABLE void beginLook(const QPointF& screenPos);
    /// Continue a look gesture: rotate the view about the camera position picked at
    /// beginLook, which stays fixed while heading/tilt (and thus center and distance)
    /// change (Google Earth Ctrl+drag first-person perspective). Same drag ratios as
    /// orbitTo. 2D mode locks tilt, leaving a rotation about the camera's vertical axis.
    Q_INVOKABLE void lookTo(const QPointF& screenPos);

    /// Rotate the camera rigidly by degrees about the ground point under screenPos
    /// (positive = counterclockwise heading change). Used for two-finger twist gestures.
    Q_INVOKABLE void rotateBy(qreal degrees, const QPointF& screenPos);

    /// Tilt the camera by degrees (clamped), keeping the ground point under screenPos
    /// fixed on screen. Used for two-finger vertical swipe gestures.
    Q_INVOKABLE void tiltBy(qreal degrees, const QPointF& screenPos);

    /// Zoom toward/away from the ground point under screenPos. amount is in wheel
    /// angleDelta units (positive = zoom in). Distance is clamped.
    Q_INVOKABLE void zoom(qreal amount, const QPointF& screenPos);

    /// Scale the camera distance by factor (e.g. 0.5 halves the distance), keeping the
    /// ground point under screenPos fixed on screen. Used for pinch gestures.
    Q_INVOKABLE void zoomBy(qreal factor, const QPointF& screenPos);

    /// Ground-plane world meters spanned by one horizontal pixel at screen center.
    /// Returns 0 when the viewport size is not set.
    Q_INVOKABLE qreal sceneUnitsPerPixel() const;

    /// Camera distance that renders the scene at the scale of a slippy-map zoom
    /// level (matches QtLocation map zoomLevel semantics when top-down). Clamped
    /// to the distance limits; returns the default distance when the viewport is
    /// not set.
    Q_INVOKABLE qreal distanceForZoomLevel(qreal zoomLevel) const;

    /// Center that places coordinate at screenPos with the current heading/tilt/
    /// distance (rigid translation solve on the horizontal plane at worldZ, the
    /// point's rendered height in world units — 0 for a ground point). Returns the
    /// current center when screenPos misses the plane or the viewport is not set.
    /// centerElevation overrides the pivot elevation for the solve — pass the
    /// elevation the pivot will settle at when it follows terrain to the new
    /// center; NaN uses the current centerElevation.
    Q_INVOKABLE QGeoCoordinate centerForCoordinateAtScreenPoint(const QGeoCoordinate& coordinate,
                                                                const QPointF& screenPos, double worldZ = 0.0,
                                                                double centerElevation = qQNaN()) const;

    /// Derived camera position in world space (mercator meters, z up).
    /// Float precision — for rendering/debug display; internal math is double.
    QVector3D cameraPosition() const;

    /// Ground projection of the camera position (world mercator meters) in
    /// double precision, for consumers whose math must not lose meters at
    /// world scale (LOD/culling distances).
    QPointF cameraGroundPosition() const;

    /// World-space reference origin for scene coordinates (set by the renderer,
    /// normally bound to GeoScene::sceneOrigin)
    QPointF sceneOrigin() const { return _sceneOrigin; }

    void setSceneOrigin(const QPointF& origin);

    /// Camera position in scene coordinates (world minus sceneOrigin, z up),
    /// for binding to the View3D camera node
    QVector3D scenePosition() const;

    /// Camera orientation for the View3D camera node: identity = top-down with
    /// north up (matches the pose convention R = Rz(heading) * Rx(tilt))
    QQuaternion sceneRotation() const;

    /// Intersect the pick ray through screenPos (pixels) with the horizontal plane
    /// at planeZ (scene units, default the ground plane z=0). Returns the hit point
    /// in world meters, or std::nullopt when the ray misses (at/above the horizon or
    /// the plane is behind the camera) or the viewport is not set.
    std::optional<QPointF> screenToGround(const QPointF& screenPos, double planeZ = 0.0) const;

    /// QML-facing screenToGround: geographic coordinate of the pick-ray hit on the
    /// horizontal plane at worldZ (scene units, same as screenToGround's planeZ).
    /// Invalid coordinate when the ray misses.
    Q_INVOKABLE QGeoCoordinate coordinateAtScreenPoint(const QPointF& screenPos, double worldZ = 0.0) const;

    /// Like screenToGround, but never fails for horizon misses: the result is capped
    /// at maxRange ground meters from the camera's ground position, along the ray's
    /// horizontal direction. Used for visible-region estimation. Returns std::nullopt
    /// only when the viewport is not set.
    std::optional<QPointF> groundPointCapped(const QPointF& screenPos, double maxRange) const;

    /// Projects a world-space point (mercator meters, z in scene units) to screen
    /// pixels. Double precision throughout. Returns std::nullopt when the point is
    /// at or behind the camera plane or the viewport is not set.
    std::optional<QPointF> worldToScreen(const QPointF& worldGround, double worldZ = 0.0) const;

signals:
    void centerChanged();
    void headingChanged();
    void tiltChanged();
    void distanceChanged();
    void centerElevationChanged();
    void viewportSizeChanged();
    void fieldOfViewChanged();
    void verticalFieldOfViewChanged();
    void unitsPerPixelAtUnitDistanceChanged();
    void modeChanged();
    /// Emitted whenever scenePosition/sceneRotation may have changed (any pose
    /// component or the scene origin)
    void scenePoseChanged();

private:
    void _setCenterWorld(const QPointF& world);
    /// Shift the center so the point anchorWorld (at height anchorZ) appears under screenPos
    void _anchorToScreen(const QPointF& anchorWorld, const QPointF& screenPos, double anchorZ = 0.0);
    static qreal _normalizedHeading(qreal heading);

    QPointF _centerWorld;  // mercator meters
    bool _positioned = false;
    QPointF _sceneOrigin;  // mercator meters
    Mode _mode = Mode::Mode2D;
    qreal _heading = 0.0;
    qreal _tilt = 0.0;
    qreal _distance = kDefaultDistance;
    qreal _centerElevation = 0.0;
    QSizeF _viewportSize;
    qreal _fieldOfView = kDefaultFieldOfView;

    // Gesture state
    std::optional<QPointF> _panAnchorWorld;
    double _panAnchorZ = 0.0;
    std::optional<QPointF> _orbitAnchorWorld;
    double _orbitAnchorZ = 0.0;
    QPointF _orbitAnchorScreen;
    QPointF _orbitStartScreen;
    qreal _orbitStartHeading = 0.0;
    qreal _orbitStartTilt = 0.0;
    std::optional<QPointF> _lookCameraGround;  // fixed camera ground position (world meters)
    qreal _lookCameraZ = 0.0;
    QPointF _lookStartScreen;
    qreal _lookStartHeading = 0.0;
    qreal _lookStartTilt = 0.0;
};
