/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoScene.h"

#include <cmath>

#include "GeoMapCamera.h"
#include "QGCLoggingCategory.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(GeoMapGeoSceneLog, "GeoMap.GeoScene")

GeoScene::GeoScene(QObject* parent) : QObject(parent) {}

void GeoScene::setCamera(GeoMapCamera* camera)
{
    if (camera == _camera) {
        return;
    }
    qCDebug(GeoMapGeoSceneLog) << "camera" << (camera ? "set" : "cleared");
    if (_camera) {
        disconnect(_camera, nullptr, this, nullptr);
    }
    _camera = camera;
    _originSet = false;  // new camera anchors fresh; a stale origin breaks float precision silently

    if (_camera) {
        connect(_camera, &GeoMapCamera::centerChanged, this, &GeoScene::_maybeReanchor);
        connect(_camera, &QObject::destroyed, this, [this] {
            _camera = nullptr;
            _originSet = false;
            emit cameraChanged();
        });
        // Anchor before announcing: cameraChanged listeners must never see an
        // origin inconsistent with the new camera
        _maybeReanchor();
    }
    emit cameraChanged();
}

qreal GeoScene::verticalScale() const
{
    return TileMath::mercatorScale(TileMath::worldToGeo(_origin).latitude());
}

QVector3D GeoScene::scenePositionFor(const QGeoCoordinate& coordinate) const
{
    const QPointF world = TileMath::geoToWorld(coordinate);
    const double altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : 0.0;
    return QVector3D(static_cast<float>(world.x() - _origin.x()), static_cast<float>(world.y() - _origin.y()),
                     static_cast<float>(altitude * verticalScale()));
}

QVariant GeoScene::screenPositionFor(const QGeoCoordinate& coordinate) const
{
    if (!_camera) {
        return {};
    }
    const QPointF world = TileMath::geoToWorld(coordinate);
    const double altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : 0.0;
    const auto screen = _camera->worldToScreen(world, altitude * verticalScale());
    return screen ? QVariant(*screen) : QVariant();
}

void GeoScene::_maybeReanchor()
{
    if (!_camera) {
        return;
    }
    const QPointF cameraWorld = TileMath::geoToWorld(_camera->center());
    if (_originSet) {
        const QPointF offset = cameraWorld - _origin;
        if (QPointF::dotProduct(offset, offset) < (kReanchorDistance * kReanchorDistance)) {
            return;
        }
    }
    _origin = cameraWorld;
    _originSet = true;
    qCDebug(GeoMapGeoSceneLog) << "scene origin re-anchored to" << _origin;
    emit sceneOriginChanged();
}
