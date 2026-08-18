/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoMapProjectedPath.h"

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

GeoMapProjectedPath::GeoMapProjectedPath(QObject* parent) : QObject(parent) {}

void GeoMapProjectedPath::setCoordinates(const QVariantList& coordinates)
{
    if (coordinates == _coordinates) {
        return;
    }
    _coordinates = coordinates;
    _rebuild();
    emit coordinatesChanged();
}

void GeoMapProjectedPath::setScene(GeoScene* scene)
{
    if (scene == _scene) {
        return;
    }
    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }
    _scene = scene;
    if (_scene) {
        connect(_scene, &GeoScene::sceneOriginChanged, this, &GeoMapProjectedPath::_rebuild);
        connect(_scene, &GeoScene::terrainScaleChanged, this, &GeoMapProjectedPath::_rebuild);
        connect(_scene, &GeoScene::cameraChanged, this, &GeoMapProjectedPath::_connectCamera);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            _connectCamera();
            emit sceneChanged();
        });
    }
    _connectCamera();
    emit sceneChanged();
}

void GeoMapProjectedPath::setSurfaceModel(SurfacePatchModel* surfaceModel)
{
    if (surfaceModel == _surfaceModel) {
        return;
    }
    if (_surfaceModel) {
        disconnect(_surfaceModel, nullptr, this, nullptr);
    }
    _surfaceModel = surfaceModel;
    if (_surfaceModel) {
        connect(_surfaceModel, &SurfacePatchModel::terrainHeightsChanged, this,
                &GeoMapProjectedPath::_terrainHeightsChanged);
        connect(_surfaceModel, &QObject::destroyed, this, [this] {
            _surfaceModel = nullptr;
            _rebuild();
            emit surfaceModelChanged();
        });
    }
    _rebuild();
    emit surfaceModelChanged();
}

void GeoMapProjectedPath::setAltitudeMode(GeoMapItem::AltitudeMode mode)
{
    if (mode == _altitudeMode) {
        return;
    }
    _altitudeMode = mode;
    _rebuild();
    emit altitudeModeChanged();
}

void GeoMapProjectedPath::_connectCamera()
{
    GeoMapCamera* const camera = _scene ? _scene->camera() : nullptr;
    if (camera == _connectedCamera) {
        _rebuild();
        return;
    }
    if (_connectedCamera) {
        disconnect(_connectedCamera, nullptr, this, nullptr);
    }
    _connectedCamera = camera;
    if (_connectedCamera) {
        connect(_connectedCamera, &GeoMapCamera::scenePoseChanged, this, &GeoMapProjectedPath::_rebuild);
        connect(_connectedCamera, &GeoMapCamera::viewportSizeChanged, this, &GeoMapProjectedPath::_rebuild);
        connect(_connectedCamera, &GeoMapCamera::fieldOfViewChanged, this, &GeoMapProjectedPath::_rebuild);
        connect(_connectedCamera, &QObject::destroyed, this, [this] {
            _connectedCamera = nullptr;
            _rebuild();
        });
    }
    _rebuild();
}

void GeoMapProjectedPath::_terrainHeightsChanged()
{
    if (_altitudeMode == GeoMapItem::AltitudeMode::ClampToGround) {
        _rebuild();
    }
}

void GeoMapProjectedPath::_rebuild()
{
    QVariantList points;
    points.reserve(_coordinates.size());
    bool allProjected = !_coordinates.isEmpty();

    GeoMapCamera* const camera = _scene ? _scene->camera() : nullptr;

    for (const QVariant& variant : std::as_const(_coordinates)) {
        const QGeoCoordinate coordinate = variant.value<QGeoCoordinate>();
        if (!_scene || !camera || !coordinate.isValid()) {
            allProjected = false;
            continue;
        }

        double altitude = 0.0;
        if (_altitudeMode == GeoMapItem::AltitudeMode::Absolute) {
            altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : 0.0;
        } else if (_surfaceModel) {
            altitude = _surfaceModel->terrainHeightAt(coordinate);
        }

        const QPointF world = TileMath::geoToWorld(coordinate);
        const double sceneZ = altitude * _scene->verticalScale() * _scene->terrainScale();
        const auto screen = camera->worldToScreen(world, sceneZ);
        if (!screen) {
            allProjected = false;
            continue;
        }
        points.append(*screen);
    }

    if (points != _screenPoints) {
        _screenPoints = points;
        emit screenPointsChanged();
    }
    if (allProjected != _projected) {
        _projected = allProjected;
        emit projectedChanged();
    }
}

QVariantList GeoMapProjectedPath::circleCoordinates(const QGeoCoordinate& center, double radiusMeters, int segments)
{
    QVariantList coordinates;
    if (!center.isValid() || radiusMeters <= 0.0 || segments < 3) {
        return coordinates;
    }
    coordinates.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        const double azimuth = (360.0 * i) / segments;
        coordinates.append(QVariant::fromValue(center.atDistanceAndAzimuth(radiusMeters, azimuth)));
    }
    return coordinates;
}
