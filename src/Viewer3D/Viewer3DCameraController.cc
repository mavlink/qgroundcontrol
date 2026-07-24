/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DCameraController.h"

#include <QtCore/QtMath>
#include <QtGui/QQuaternion>
#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kYawFullDragDegrees = 360.0;   ///< full viewport width of drag
constexpr qreal kTiltFullDragDegrees = 180.0;  ///< full viewport height of drag
constexpr qreal kZoomSensitivity = 0.001;      ///< per wheel angleDelta unit

/// Unit vector from orbit center toward the camera for the given tilt/heading (degrees)
QVector3D centerToCameraDirection(qreal tilt, qreal heading)
{
    const qreal tiltRad = qDegreesToRadians(tilt);
    const qreal headingRad = qDegreesToRadians(heading);
    return QVector3D(std::sin(tiltRad) * std::sin(headingRad), -std::sin(tiltRad) * std::cos(headingRad),
                     std::cos(tiltRad));
}

}  // namespace

Viewer3DCameraController::Viewer3DCameraController(QObject* parent) : QObject(parent) {}

void Viewer3DCameraController::setOrbitCenter(const QVector3D& center)
{
    if (center == _orbitCenter) {
        return;
    }
    _orbitCenter = center;
    emit orbitCenterChanged();
}

void Viewer3DCameraController::setHeading(qreal heading)
{
    if (heading == _heading) {
        return;
    }
    _heading = heading;
    emit headingChanged();
}

void Viewer3DCameraController::setTilt(qreal tilt)
{
    tilt = std::clamp(tilt, kMinTilt, kMaxTilt);
    if (tilt == _tilt) {
        return;
    }
    _tilt = tilt;
    emit tiltChanged();
}

void Viewer3DCameraController::setDistance(qreal distance)
{
    distance = std::clamp(distance, kMinDistance, kMaxDistance);
    if (distance == _distance) {
        return;
    }
    _distance = distance;
    emit distanceChanged();
}

void Viewer3DCameraController::setViewportSize(const QSizeF& size)
{
    if (size == _viewportSize) {
        return;
    }
    _viewportSize = size;
    emit viewportSizeChanged();
}

void Viewer3DCameraController::setFieldOfView(qreal fov)
{
    if (fov == _fieldOfView) {
        return;
    }
    _fieldOfView = fov;
    emit fieldOfViewChanged();
}

void Viewer3DCameraController::reset()
{
    lookAt(QVector3D(), 0.0, 0.0, kDefaultDistance);
}

void Viewer3DCameraController::lookAt(const QVector3D& center, qreal heading, qreal tilt, qreal distance)
{
    setOrbitCenter(center);
    setHeading(heading);
    setTilt(tilt);
    setDistance(distance);
}

void Viewer3DCameraController::beginPan(const QPointF& screenPos)
{
    const auto ground = screenToGround(screenPos);
    _panAnchorValid = ground.has_value();
    if (_panAnchorValid) {
        _panAnchor = *ground;
    }
}

void Viewer3DCameraController::panTo(const QPointF& screenPos)
{
    if (!_panAnchorValid) {
        return;
    }
    const auto ground = screenToGround(screenPos);
    if (!ground.has_value()) {
        return;
    }
    // Translate so the anchor returns under the cursor. Ground points move with
    // the camera, so the required translation is exact in a single step.
    setOrbitCenter(_orbitCenter + (_panAnchor - *ground));
}

void Viewer3DCameraController::beginOrbit(const QPointF& screenPos)
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        _orbitAnchorValid = false;
        return;
    }
    const auto ground = screenToGround(screenPos);
    // When the pick ray misses the ground, orbit about the current center instead
    _orbitAnchor = ground.value_or(_orbitCenter);
    _orbitAnchorValid = true;
    _lastOrbitPos = screenPos;
}

void Viewer3DCameraController::orbitTo(const QPointF& screenPos)
{
    if (!_orbitAnchorValid) {
        return;
    }

    const QPointF delta = screenPos - _lastOrbitPos;
    _lastOrbitPos = screenPos;

    const qreal headingDelta = (delta.x() / _viewportSize.width()) * kYawFullDragDegrees;
    const qreal tiltDelta = -((delta.y() / _viewportSize.height()) * kTiltFullDragDegrees);

    _rotateAboutAnchor(_orbitAnchor, headingDelta, tiltDelta);
}

void Viewer3DCameraController::rotateBy(qreal degrees, const QPointF& screenPos)
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return;
    }
    // When the pick ray misses the ground, rotate about the current center instead
    const QVector3D anchor = screenToGround(screenPos).value_or(_orbitCenter);
    _rotateAboutAnchor(anchor, degrees, 0.0);
}

void Viewer3DCameraController::tiltBy(qreal degrees, const QPointF& screenPos)
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return;
    }
    const QVector3D anchor = screenToGround(screenPos).value_or(_orbitCenter);
    _rotateAboutAnchor(anchor, 0.0, degrees);
}

void Viewer3DCameraController::_rotateAboutAnchor(const QVector3D& anchor, qreal headingDelta, qreal tiltDelta)
{
    tiltDelta = std::clamp(_tilt + tiltDelta, kMinTilt, kMaxTilt) - _tilt;

    QVector3D center = _orbitCenter;

    // Yaw: rigid rotation about the world-z axis through the anchor
    if (headingDelta != 0.0) {
        const QQuaternion yaw = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), static_cast<float>(headingDelta));
        center = anchor + yaw.rotatedVector(center - anchor);
        setHeading(_heading + headingDelta);
    }

    // Pitch: rigid rotation about the camera-right axis through the anchor.
    // Rotating by the tilt delta about right = (cos h, sin h, 0) advances the
    // center-to-camera direction from u(tilt) to u(tilt + delta).
    if (tiltDelta != 0.0) {
        const qreal headingRad = qDegreesToRadians(_heading);
        const QVector3D rightAxis(static_cast<float>(std::cos(headingRad)), static_cast<float>(std::sin(headingRad)),
                                  0.0f);
        const QQuaternion pitch = QQuaternion::fromAxisAndAngle(rightAxis, static_cast<float>(tiltDelta));
        center = anchor + pitch.rotatedVector(center - anchor);
        setTilt(_tilt + tiltDelta);
    }

    setOrbitCenter(center);
}

void Viewer3DCameraController::zoom(qreal amount, const QPointF& screenPos)
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return;
    }

    // Guard exp() underflow for very large zoom amounts: clamp to a tiny
    // positive factor so zoomBy still clamps the distance to the minimum.
    const qreal factor = std::max(std::exp(-amount * kZoomSensitivity), kMinDistance / kMaxDistance);
    zoomBy(factor, screenPos);
}

void Viewer3DCameraController::zoomBy(qreal factor, const QPointF& screenPos)
{
    if ((factor <= 0.0) || !_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return;
    }

    const qreal newDistance = std::clamp(_distance * factor, kMinDistance, kMaxDistance);
    factor = newDistance / _distance;
    if (factor == 1.0) {
        return;
    }

    const auto ground = screenToGround(screenPos);
    if (ground.has_value()) {
        // Move toward/away from the point under the cursor so it stays put on screen
        setOrbitCenter(*ground + ((_orbitCenter - *ground) * static_cast<float>(factor)));
    }
    setDistance(newDistance);
}

qreal Viewer3DCameraController::sceneUnitsPerPixel() const
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return 0.0;
    }
    // Ground span per screen-x pixel at screen center. The camera-right axis is
    // ground-parallel, so this is tilt-independent.
    const qreal tanHalfFov = std::tan(qDegreesToRadians(_fieldOfView / 2.0));
    return (2.0 * _distance * tanHalfFov) / _viewportSize.height();
}

QVector3D Viewer3DCameraController::cameraPosition() const
{
    return _orbitCenter + (centerToCameraDirection(_tilt, _heading) * static_cast<float>(_distance));
}

std::optional<QVector3D> Viewer3DCameraController::screenToGround(const QPointF& screenPos) const
{
    if (!_viewportSize.isValid() || _viewportSize.isEmpty()) {
        return std::nullopt;
    }

    const qreal tiltRad = qDegreesToRadians(_tilt);
    const qreal headingRad = qDegreesToRadians(_heading);
    const qreal sinT = std::sin(tiltRad);
    const qreal cosT = std::cos(tiltRad);
    const qreal sinH = std::sin(headingRad);
    const qreal cosH = std::cos(headingRad);

    // Camera basis in scene space (z-up)
    const QVector3D forward(static_cast<float>(-sinT * sinH), static_cast<float>(sinT * cosH),
                            static_cast<float>(-cosT));
    const QVector3D right(static_cast<float>(cosH), static_cast<float>(sinH), 0.0f);
    const QVector3D up(static_cast<float>(-sinH * cosT), static_cast<float>(cosH * cosT), static_cast<float>(sinT));

    // Pick ray direction through the pixel (vertical field of view)
    const qreal tanHalfFov = std::tan(qDegreesToRadians(_fieldOfView / 2.0));
    const qreal aspect = _viewportSize.width() / _viewportSize.height();
    const qreal ndcX = ((2.0 * screenPos.x()) / _viewportSize.width()) - 1.0;
    const qreal ndcY = 1.0 - ((2.0 * screenPos.y()) / _viewportSize.height());
    const QVector3D rayDir = (right * static_cast<float>(ndcX * tanHalfFov * aspect)) +
                             (up * static_cast<float>(ndcY * tanHalfFov)) + forward;

    if (rayDir.z() >= 0.0f) {
        return std::nullopt;
    }

    const QVector3D rayOrigin = cameraPosition();
    const float s = -rayOrigin.z() / rayDir.z();
    if (s <= 0.0f) {
        return std::nullopt;
    }
    return rayOrigin + (rayDir * s);
}
