#include "GeoMapCamera.h"

#include <QtCore/QtMath>

#include <algorithm>
#include <cmath>

#include "QGCLoggingCategory.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(GeoMapCameraLog, "GeoMap.GeoMapCamera")

namespace {

struct Vec3
{
    double x = 0;
    double y = 0;
    double z = 0;
};

// Camera-to-world rotation for the pose convention R = Rz(heading) * Rx(tilt),
// in double precision (QQuaternion/QVector3D floats lose meters at mercator world scale).
Vec3 rotateCameraToWorld(const Vec3& v, qreal headingDeg, qreal tiltDeg)
{
    const double t = qDegreesToRadians(tiltDeg);
    const double h = qDegreesToRadians(headingDeg);

    // Rx(tilt)
    const Vec3 a{v.x, (v.y * std::cos(t)) - (v.z * std::sin(t)), (v.y * std::sin(t)) + (v.z * std::cos(t))};
    // Rz(heading)
    return Vec3{(a.x * std::cos(h)) - (a.y * std::sin(h)), (a.x * std::sin(h)) + (a.y * std::cos(h)), a.z};
}

QPointF rotated(const QPointF& vec, qreal degrees)
{
    const double rad = qDegreesToRadians(degrees);
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    return QPointF((vec.x() * c) - (vec.y() * s), (vec.x() * s) + (vec.y() * c));
}

struct Ray
{
    Vec3 origin;
    Vec3 dir;
};

// Camera offset from the look-at center for the pose convention: the camera
// sits distance meters along the back-rotated view axis (top-down at tilt 0,
// moving south and down-range as tilt grows, swung around by heading)
Vec3 cameraOffset(qreal headingDeg, qreal tiltDeg, qreal distance)
{
    const double tiltRad = qDegreesToRadians(tiltDeg);
    const double headingRad = qDegreesToRadians(headingDeg);
    const double sinTilt = std::sin(tiltRad);
    return Vec3{distance * sinTilt * std::sin(headingRad), -distance * sinTilt * std::cos(headingRad),
                distance * std::cos(tiltRad)};
}

Ray pickRay(const QPointF& centerWorld, qreal heading, qreal tilt, qreal distance, qreal centerElevation,
            const QSizeF& viewport, qreal fov, const QPointF& screenPos)
{
    const double aspect = viewport.width() / viewport.height();
    const double tanHalfFov = std::tan(qDegreesToRadians(fov) / 2.0);
    const double ndcX = ((2.0 * screenPos.x()) / viewport.width()) - 1.0;
    const double ndcY = 1.0 - ((2.0 * screenPos.y()) / viewport.height());

    const Vec3 dir = rotateCameraToWorld(Vec3{ndcX * tanHalfFov * aspect, ndcY * tanHalfFov, -1.0}, heading, tilt);

    const Vec3 offset = cameraOffset(heading, tilt, distance);
    return Ray{Vec3{centerWorld.x() + offset.x, centerWorld.y() + offset.y, centerElevation + offset.z}, dir};
}

}  // namespace

GeoMapCamera::GeoMapCamera(QObject* parent) : QObject(parent) {}

QGeoCoordinate GeoMapCamera::center() const
{
    return TileMath::worldToGeo(_centerWorld);
}

void GeoMapCamera::setCenter(const QGeoCoordinate& center)
{
    _setCenterWorld(TileMath::geoToWorld(center));
}

void GeoMapCamera::_setCenterWorld(const QPointF& world)
{
    const double half = TileMath::worldSize() / 2.0;
    const QPointF clamped(std::clamp(world.x(), -half, half), std::clamp(world.y(), -half, half));
    const bool firstPosition = !_positioned;
    _positioned = true;  // any explicit center counts, even one equal to the default
    if ((clamped == _centerWorld) && !firstPosition) {
        return;
    }
    _centerWorld = clamped;
    emit centerChanged();
    emit scenePoseChanged();
}

void GeoMapCamera::setHeading(qreal heading)
{
    const qreal normalized = _normalizedHeading(heading);
    if (qFuzzyCompare(normalized, _heading)) {
        return;
    }
    _heading = normalized;
    emit headingChanged();
    emit scenePoseChanged();
}

void GeoMapCamera::setTilt(qreal tilt)
{
    const qreal clamped = std::clamp(tilt, kMinTilt, kMaxTilt);
    if (qFuzzyCompare(clamped, _tilt)) {
        return;
    }
    _tilt = clamped;
    emit tiltChanged();
    emit scenePoseChanged();
}

void GeoMapCamera::setDistance(qreal distance)
{
    const qreal clamped = std::clamp(distance, kMinDistance, kMaxDistance);
    if (qFuzzyCompare(clamped, _distance)) {
        return;
    }
    _distance = clamped;
    emit distanceChanged();
    emit scenePoseChanged();
}

void GeoMapCamera::setCenterElevation(qreal elevation)
{
    // Exact compare: values repeat from the same terrain lookup, and
    // qFuzzyCompare misbehaves near zero
    if (elevation == _centerElevation) {
        return;
    }
    _centerElevation = elevation;
    emit centerElevationChanged();
    emit scenePoseChanged();
}

void GeoMapCamera::setViewportSize(const QSizeF& size)
{
    if (size == _viewportSize) {
        return;
    }
    const qreal previousVerticalFov = verticalFieldOfView();
    _viewportSize = size;
    emit viewportSizeChanged();
    if (!qFuzzyCompare(verticalFieldOfView(), previousVerticalFov)) {
        emit verticalFieldOfViewChanged();
    }
    emit unitsPerPixelAtUnitDistanceChanged();
}

void GeoMapCamera::setFieldOfView(qreal fov)
{
    const qreal clamped = std::clamp(fov, 10.0, 120.0);
    if (qFuzzyCompare(clamped, _fieldOfView)) {
        return;
    }
    _fieldOfView = clamped;
    emit fieldOfViewChanged();
    emit verticalFieldOfViewChanged();
    emit unitsPerPixelAtUnitDistanceChanged();
}

qreal GeoMapCamera::verticalFieldOfView() const
{
    if (_viewportSize.isEmpty() || _viewportSize.width() <= _viewportSize.height()) {
        return _fieldOfView;
    }
    const double aspect = _viewportSize.width() / _viewportSize.height();
    return qRadiansToDegrees(2.0 * std::atan(std::tan(qDegreesToRadians(_fieldOfView) / 2.0) / aspect));
}

qreal GeoMapCamera::unitsPerPixelAtUnitDistance() const
{
    if (_viewportSize.height() <= 0) {
        return 0.0;
    }
    return 2.0 * std::tan(qDegreesToRadians(verticalFieldOfView()) / 2.0) / _viewportSize.height();
}

void GeoMapCamera::setMode(Mode mode)
{
    if (mode == _mode) {
        return;
    }
    qCDebug(GeoMapCameraLog) << "mode" << _mode << "->" << mode;
    _mode = mode;
    emit modeChanged();
}

void GeoMapCamera::reset()
{
    setHeading(0);
    setTilt(kMinTilt);
    setDistance(kDefaultDistance);
}

void GeoMapCamera::lookAt(const QGeoCoordinate& center, qreal heading, qreal tilt, qreal distance)
{
    setCenter(center);
    setHeading(heading);
    setTilt(tilt);
    setDistance(distance);
}

QVector3D GeoMapCamera::cameraPosition() const
{
    const Vec3 offset = cameraOffset(_heading, _tilt, _distance);
    return QVector3D(static_cast<float>(_centerWorld.x() + offset.x), static_cast<float>(_centerWorld.y() + offset.y),
                     static_cast<float>(_centerElevation + offset.z));
}

QPointF GeoMapCamera::cameraGroundPosition() const
{
    const Vec3 offset = cameraOffset(_heading, _tilt, _distance);
    return QPointF(_centerWorld.x() + offset.x, _centerWorld.y() + offset.y);
}

void GeoMapCamera::setSceneOrigin(const QPointF& origin)
{
    if (origin == _sceneOrigin) {
        return;
    }
    _sceneOrigin = origin;
    emit scenePoseChanged();
}

QVector3D GeoMapCamera::scenePosition() const
{
    // World offset computed in doubles before the float cast
    const Vec3 offset = cameraOffset(_heading, _tilt, _distance);
    return QVector3D(static_cast<float>((_centerWorld.x() - _sceneOrigin.x()) + offset.x),
                     static_cast<float>((_centerWorld.y() - _sceneOrigin.y()) + offset.y),
                     static_cast<float>(_centerElevation + offset.z));
}

QQuaternion GeoMapCamera::sceneRotation() const
{
    // R = Rz(heading) * Rx(tilt): identity is top-down (camera -z look direction
    // points down the scene z axis) with north (+y) up on screen
    return QQuaternion::fromAxisAndAngle(0, 0, 1, static_cast<float>(_heading)) *
           QQuaternion::fromAxisAndAngle(1, 0, 0, static_cast<float>(_tilt));
}

std::optional<QPointF> GeoMapCamera::screenToGround(const QPointF& screenPos, double planeZ) const
{
    if (_viewportSize.isEmpty()) {
        return std::nullopt;
    }

    const Ray ray =
        pickRay(_centerWorld, _heading, _tilt, _distance, _centerElevation, _viewportSize, verticalFieldOfView(), screenPos);
    if (ray.dir.z >= 0.0) {
        return std::nullopt;  // at or above the horizon
    }

    const double s = (planeZ - ray.origin.z) / ray.dir.z;
    if (s <= 0.0) {
        return std::nullopt;  // plane is behind/above the camera
    }
    return QPointF(ray.origin.x + (s * ray.dir.x), ray.origin.y + (s * ray.dir.y));
}

QGeoCoordinate GeoMapCamera::coordinateAtScreenPoint(const QPointF& screenPos, double worldZ) const
{
    const auto hit = screenToGround(screenPos, worldZ);
    if (!hit) {
        return QGeoCoordinate();
    }
    return TileMath::worldToGeo(*hit);
}

std::optional<QPointF> GeoMapCamera::groundPointCapped(const QPointF& screenPos, double maxRange) const
{
    if (_viewportSize.isEmpty()) {
        return std::nullopt;
    }

    const Ray ray =
        pickRay(_centerWorld, _heading, _tilt, _distance, _centerElevation, _viewportSize, verticalFieldOfView(), screenPos);
    const QPointF cameraGround(ray.origin.x, ray.origin.y);

    if (ray.dir.z < 0.0) {
        const double s = -ray.origin.z / ray.dir.z;
        const QPointF hit(ray.origin.x + (s * ray.dir.x), ray.origin.y + (s * ray.dir.y));
        const QPointF offset = hit - cameraGround;
        const double range = std::hypot(offset.x(), offset.y());
        if (range <= maxRange) {
            return hit;
        }
        return cameraGround + (offset * (maxRange / range));
    }

    // Above the horizon: cap along the ray's horizontal direction
    const double horizontal = std::hypot(ray.dir.x, ray.dir.y);
    if (horizontal < 1e-12) {
        return cameraGround;  // straight up
    }
    return cameraGround + (QPointF(ray.dir.x, ray.dir.y) * (maxRange / horizontal));
}

std::optional<QPointF> GeoMapCamera::worldToScreen(const QPointF& worldGround, double worldZ) const
{
    if (_viewportSize.isEmpty()) {
        return std::nullopt;
    }

    const Vec3 offset = cameraOffset(_heading, _tilt, _distance);
    const Vec3 d{worldGround.x() - (_centerWorld.x() + offset.x), worldGround.y() - (_centerWorld.y() + offset.y),
                 worldZ - (_centerElevation + offset.z)};

    // World-to-camera: inverse of the pose rotation, R^T = Rx(-tilt) * Rz(-heading)
    const double h = qDegreesToRadians(_heading);
    const double t = qDegreesToRadians(_tilt);
    const Vec3 a{(d.x * std::cos(h)) + (d.y * std::sin(h)), (-d.x * std::sin(h)) + (d.y * std::cos(h)), d.z};
    const Vec3 c{a.x, (a.y * std::cos(t)) + (a.z * std::sin(t)), (-a.y * std::sin(t)) + (a.z * std::cos(t))};

    // Camera looks along -z; the epsilon guards the projection divide (not a near plane)
    const double depth = -c.z;
    if (depth <= 1e-9) {
        return std::nullopt;
    }

    const double aspect = _viewportSize.width() / _viewportSize.height();
    const double tanHalfFov = std::tan(qDegreesToRadians(verticalFieldOfView()) / 2.0);
    const double ndcX = c.x / (depth * tanHalfFov * aspect);
    const double ndcY = c.y / (depth * tanHalfFov);
    return QPointF(((ndcX + 1.0) / 2.0) * _viewportSize.width(), ((1.0 - ndcY) / 2.0) * _viewportSize.height());
}

qreal GeoMapCamera::sceneUnitsPerPixel() const
{
    if (_viewportSize.isEmpty()) {
        return 0;
    }
    const QPointF screenCenter(_viewportSize.width() / 2.0, _viewportSize.height() / 2.0);
    const auto p0 = screenToGround(screenCenter);
    const auto p1 = screenToGround(screenCenter + QPointF(1, 0));
    if (!p0 || !p1) {
        return 0;
    }
    const QPointF d = *p1 - *p0;
    return std::hypot(d.x(), d.y());
}

qreal GeoMapCamera::distanceForZoomLevel(qreal zoomLevel) const
{
    if (_viewportSize.isEmpty()) {
        return kDefaultDistance;
    }
    const double metersPerPixel = TileMath::worldSize() / (TileMath::kTilePixels * std::exp2(zoomLevel));
    const double tanHalfFov = std::tan(qDegreesToRadians(verticalFieldOfView()) / 2.0);
    return std::clamp((metersPerPixel * _viewportSize.height()) / (2.0 * tanHalfFov), kMinDistance, kMaxDistance);
}

QGeoCoordinate GeoMapCamera::centerForCoordinateAtScreenPoint(const QGeoCoordinate& coordinate,
                                                              const QPointF& screenPos, double worldZ,
                                                              double centerElevation) const
{
    if (_viewportSize.isEmpty()) {
        return center();
    }

    // Intersect the pick ray with the horizontal plane at worldZ: solving on the
    // ground plane instead would center the coordinate's ground footprint, leaving
    // an elevated point (e.g. a flying vehicle) high on screen under a tilted camera.
    const double pivotElevation = std::isfinite(centerElevation) ? centerElevation : _centerElevation;
    const Ray ray =
        pickRay(_centerWorld, _heading, _tilt, _distance, pivotElevation, _viewportSize, verticalFieldOfView(), screenPos);
    if (ray.dir.z >= 0.0) {
        return center();  // at or above the horizon
    }
    const double s = (worldZ - ray.origin.z) / ray.dir.z;
    if (s <= 0.0) {
        return center();  // plane is behind the camera
    }

    const QPointF hit(ray.origin.x + (s * ray.dir.x), ray.origin.y + (s * ray.dir.y));
    const QPointF delta = TileMath::geoToWorld(coordinate) - hit;
    return TileMath::worldToGeo(_centerWorld + delta);
}

void GeoMapCamera::beginPan(const QPointF& screenPos, double anchorZ)
{
    _panAnchorZ = anchorZ;
    _panAnchorWorld = screenToGround(screenPos, anchorZ);
}

void GeoMapCamera::panTo(const QPointF& screenPos)
{
    if (!_panAnchorWorld) {
        return;
    }
    _anchorToScreen(*_panAnchorWorld, screenPos, _panAnchorZ);
}

void GeoMapCamera::beginOrbit(const QPointF& screenPos, double anchorZ)
{
    _orbitAnchorZ = anchorZ;
    _orbitAnchorWorld = screenToGround(screenPos, anchorZ);
    _orbitAnchorScreen = screenPos;
    _orbitStartScreen = screenPos;
    _orbitStartHeading = _heading;
    _orbitStartTilt = _tilt;
}

void GeoMapCamera::orbitTo(const QPointF& screenPos)
{
    if (!_orbitAnchorWorld || _viewportSize.isEmpty()) {
        return;
    }

    const QPointF delta = screenPos - _orbitStartScreen;
    const qreal headingDelta = (delta.x() / _viewportSize.width()) * 360.0;
    const qreal tiltDelta = (-delta.y() / _viewportSize.height()) * 180.0;

    // Heading change is a rigid rotation of the camera about the vertical axis
    // through the anchor: rotate the center around the anchor by the same angle.
    const qreal newHeading = _normalizedHeading(_orbitStartHeading + headingDelta);
    const qreal appliedHeadingDelta = std::remainder(newHeading - _heading, 360.0);
    _setCenterWorld(*_orbitAnchorWorld + rotated(_centerWorld - *_orbitAnchorWorld, appliedHeadingDelta));
    setHeading(newHeading);
    if (_mode == Mode::Mode3D) {
        setTilt(_orbitStartTilt + tiltDelta);
    }

    // Keep the anchor pinned to its on-screen position from gesture start
    _anchorToScreen(*_orbitAnchorWorld, _orbitAnchorScreen, _orbitAnchorZ);
}

void GeoMapCamera::beginLook(const QPointF& screenPos)
{
    const Vec3 offset = cameraOffset(_heading, _tilt, _distance);
    _lookCameraGround = QPointF(_centerWorld.x() + offset.x, _centerWorld.y() + offset.y);
    _lookCameraZ = _centerElevation + offset.z;
    _lookStartScreen = screenPos;
    _lookStartHeading = _heading;
    _lookStartTilt = _tilt;
}

void GeoMapCamera::lookTo(const QPointF& screenPos)
{
    if (!_lookCameraGround || _viewportSize.isEmpty()) {
        return;
    }

    const QPointF delta = screenPos - _lookStartScreen;
    const qreal headingDelta = (delta.x() / _viewportSize.width()) * 360.0;
    const qreal tiltDelta = (-delta.y() / _viewportSize.height()) * 180.0;

    const qreal newHeading = _normalizedHeading(_lookStartHeading + headingDelta);
    const qreal newTilt = (_mode == Mode::Mode3D) ? std::clamp(_lookStartTilt + tiltDelta, kMinTilt, kMaxTilt) : _tilt;

    // The new center is the fixed camera's forward ray intersected with the
    // pivot-elevation plane; solved via the pose convention so camera z tracks
    // _lookCameraZ exactly even when centerElevation moves mid-gesture.
    // When the consumer re-samples centerElevation on centerChanged (GeoMap
    // terrain-following), that update lands after this solve: camera z is off
    // by the per-event elevation delta until the next lookTo re-solves — a
    // one-event lag, self-correcting while the drag continues.
    const double height = _lookCameraZ - _centerElevation;
    if (height <= 0.0) {
        return;  // camera at/below the pivot plane: no forward ground intersection
    }
    // Range-safety clamp: when it engages, the fixed-camera invariant yields
    // and the camera slides along the view axis to stay within distance limits
    const qreal newDistance = std::clamp(height / std::cos(qDegreesToRadians(newTilt)), kMinDistance, kMaxDistance);

    const Vec3 offset = cameraOffset(newHeading, newTilt, newDistance);
    _setCenterWorld(QPointF(_lookCameraGround->x() - offset.x, _lookCameraGround->y() - offset.y));
    setHeading(newHeading);
    setTilt(newTilt);
    setDistance(newDistance);
}

void GeoMapCamera::rotateBy(qreal degrees, const QPointF& screenPos)
{
    const auto anchor = screenToGround(screenPos);
    if (!anchor) {
        return;
    }
    _setCenterWorld(*anchor + rotated(_centerWorld - *anchor, degrees));
    setHeading(_heading + degrees);
}

void GeoMapCamera::tiltBy(qreal degrees, const QPointF& screenPos)
{
    if (_mode != Mode::Mode3D) {
        return;  // 2D locks tilt against gestures
    }
    const auto anchor = screenToGround(screenPos);
    setTilt(_tilt + degrees);
    if (anchor) {
        _anchorToScreen(*anchor, screenPos);
    }
}

void GeoMapCamera::zoom(qreal amount, const QPointF& screenPos)
{
    // 120 angleDelta units (one wheel notch) scales distance by ~0.8
    zoomBy(std::pow(0.8, amount / 120.0), screenPos);
}

void GeoMapCamera::zoomBy(qreal factor, const QPointF& screenPos)
{
    if (factor <= 0.0) {
        return;
    }
    const auto anchor = screenToGround(screenPos);
    setDistance(_distance * factor);
    if (anchor) {
        _anchorToScreen(*anchor, screenPos);
    }
}

void GeoMapCamera::_anchorToScreen(const QPointF& anchorWorld, const QPointF& screenPos, double anchorZ)
{
    const auto current = screenToGround(screenPos, anchorZ);
    if (!current) {
        return;
    }
    _setCenterWorld(_centerWorld + (anchorWorld - *current));
}

qreal GeoMapCamera::_normalizedHeading(qreal heading)
{
    const qreal wrapped = std::fmod(heading, 360.0);
    return (wrapped < 0.0) ? (wrapped + 360.0) : wrapped;
}
