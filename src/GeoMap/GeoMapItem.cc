/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoMapItem.h"

#include <QtQml/QQmlComponent>
#include <QtQuick3D/QQuick3DObject>

#include <algorithm>
#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "QGCLoggingCategory.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(GeoMapItemLog, "GeoMap.GeoMapItem")

// Far enough off-screen for any plausible viewport
static constexpr QPointF kParkedPosition{-1e6, -1e6};

GeoMapItem::GeoMapItem(QQuickItem* parent) : QQuickItem(parent)
{
    connect(this, &QQuickItem::visibleChanged, this, &GeoMapItem::_updateNode3DVisibility);
}

GeoMapItem::~GeoMapItem()
{
    delete _node3D;
}

void GeoMapItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate == _coordinate) {
        return;
    }
    _coordinate = coordinate;
    _updatePosition();
    _updateNode3DVisibility();
    emit coordinateChanged();
}

void GeoMapItem::setAnchorPoint(const QPointF& anchorPoint)
{
    if (anchorPoint == _anchorPoint) {
        return;
    }
    _anchorPoint = anchorPoint;
    _updatePosition();
    emit anchorPointChanged();
}

void GeoMapItem::setScene(GeoScene* scene)
{
    if (scene == _scene) {
        return;
    }
    qCDebug(GeoMapItemLog) << "scene" << (scene ? "set" : "cleared");
    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }
    _scene = scene;
    if (_scene) {
        connect(_scene, &GeoScene::sceneOriginChanged, this, &GeoMapItem::_updatePosition);
        connect(_scene, &GeoScene::terrainScaleChanged, this, &GeoMapItem::_updatePosition);
        connect(_scene, &GeoScene::terrainScaleChanged, this, &GeoMapItem::_updateCrossfade);
        connect(_scene, &GeoScene::cameraChanged, this, &GeoMapItem::_connectCamera);
        connect(_scene, &GeoScene::content3DChanged, this, &GeoMapItem::_rebuildNode3D);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            _connectCamera();
            _rebuildNode3D();
            emit sceneChanged();
        });
    }
    _connectCamera();
    _rebuildNode3D();
    emit sceneChanged();
}

void GeoMapItem::setSurfaceModel(SurfacePatchModel* surfaceModel)
{
    if (surfaceModel == _surfaceModel) {
        return;
    }
    if (_surfaceModel) {
        disconnect(_surfaceModel, nullptr, this, nullptr);
    }
    _surfaceModel = surfaceModel;
    if (_surfaceModel) {
        connect(_surfaceModel, &SurfacePatchModel::terrainHeightsChanged, this, &GeoMapItem::_terrainHeightsChanged);
        connect(_surfaceModel, &QObject::destroyed, this, [this] {
            _surfaceModel = nullptr;
            _updatePosition();
            emit surfaceModelChanged();
        });
    }
    _updatePosition();
    emit surfaceModelChanged();
}

void GeoMapItem::setAltitudeMode(AltitudeMode mode)
{
    if (mode == _altitudeMode) {
        return;
    }
    _altitudeMode = mode;
    _updatePosition();
    emit altitudeModeChanged();
}

void GeoMapItem::setDelegate3D(QQmlComponent* delegate3D)
{
    if (delegate3D == _delegate3D) {
        return;
    }
    qCDebug(GeoMapItemLog) << "delegate3D" << (delegate3D ? "set" : "cleared");
    if (_delegate3D) {
        disconnect(_delegate3D, nullptr, this, nullptr);
    }
    _delegate3D = delegate3D;
    if (_delegate3D) {
        connect(_delegate3D, &QObject::destroyed, this, [this] {
            _delegate3D = nullptr;
            _rebuildNode3D();
            emit delegate3DChanged();
        });
    }
    _rebuildNode3D();
    emit delegate3DChanged();
}

void GeoMapItem::setCrossfade3D(bool crossfade3D)
{
    if (crossfade3D == _crossfade3D) {
        return;
    }
    _crossfade3D = crossfade3D;
    _updateCrossfade();
    emit crossfade3DChanged();
}

void GeoMapItem::_connectCamera()
{
    GeoMapCamera* const camera = _scene ? _scene->camera() : nullptr;
    if (camera == _connectedCamera) {
        _updatePosition();
        return;
    }
    if (_connectedCamera) {
        disconnect(_connectedCamera, nullptr, this, nullptr);
    }
    _connectedCamera = camera;
    if (_connectedCamera) {
        connect(_connectedCamera, &GeoMapCamera::scenePoseChanged, this, &GeoMapItem::_updatePosition);
        connect(_connectedCamera, &GeoMapCamera::viewportSizeChanged, this, &GeoMapItem::_updatePosition);
        connect(_connectedCamera, &GeoMapCamera::fieldOfViewChanged, this, &GeoMapItem::_updatePosition);
        connect(_connectedCamera, &QObject::destroyed, this, [this] {
            _connectedCamera = nullptr;
            _updatePosition();
        });
    }
    _updatePosition();
}

void GeoMapItem::_terrainHeightsChanged()
{
    if (_altitudeMode == AltitudeMode::ClampToGround) {
        _updatePosition();
    }
}

void GeoMapItem::_setProjected(bool projected)
{
    if (projected == _projected) {
        return;
    }
    _projected = projected;
    if (!_projected) {
        // Park off-screen rather than toggling visible, which would fight
        // user bindings on this item's own visible property
        setPosition(kParkedPosition);
    }
    emit projectedChanged();
}

void GeoMapItem::_setScenePosition(const QVector3D& scenePosition)
{
    if (scenePosition == _scenePosition) {
        return;
    }
    _scenePosition = scenePosition;
    if (_node3D) {
        _node3D->setProperty("position", _scenePosition);
    }
    emit scenePositionChanged();
}

void GeoMapItem::_rebuildNode3D()
{
    const bool hadNode = _node3D != nullptr;
    if (_node3D) {
        // Unhook from the scene immediately; deleteLater is safer than delete
        // here since this can run from the container's destroyed signal
        if (auto* const object3D = qobject_cast<QQuick3DObject*>(_node3D)) {
            object3D->setParentItem(nullptr);
        }
        _node3D->deleteLater();
        _node3D = nullptr;
    }

    QQuick3DObject* const container = _scene ? qobject_cast<QQuick3DObject*>(_scene->content3D()) : nullptr;
    if (_delegate3D && container) {
        QObject* const node = _delegate3D->create(_delegate3D->creationContext());
        if (!node) {
            qCWarning(GeoMapItemLog) << "delegate3D failed to create:" << _delegate3D->errorString();
        } else if (auto* const object3D = qobject_cast<QQuick3DObject*>(node)) {
            node->setParent(container);
            object3D->setParentItem(container);
            node->setProperty("position", _scenePosition);
            _node3D = node;
            _updateNode3DVisibility();
        } else {
            // Caller contract: delegate3D must produce a QQuick3DNode
            qCWarning(GeoMapItemLog) << "delegate3D did not produce a QQuick3DObject";
            delete node;
        }
    }

    _updateCrossfade();
    if (hadNode || _node3D) {
        emit node3DChanged();
    }
}

void GeoMapItem::_updateNode3DVisibility()
{
    if (_node3D) {
        _node3D->setProperty("visible", isVisible() && _coordinate.isValid());
    }
}

void GeoMapItem::_updateCrossfade()
{
    if (!_crossfade3D) {
        if (_node3D) {
            _node3D->setProperty("opacity", 1.0);
        }
        if (_contentOpacity2D != 1.0) {
            _contentOpacity2D = 1.0;
            emit contentOpacity2DChanged();
        }
        return;
    }
    const qreal terrainScale = _scene ? std::clamp(_scene->terrainScale(), 0.0, 1.0) : 0.0;
    if (_node3D) {
        _node3D->setProperty("opacity", terrainScale);
    }
    const qreal contentOpacity2D = _node3D ? (1.0 - terrainScale) : 1.0;
    if (contentOpacity2D != _contentOpacity2D) {
        _contentOpacity2D = contentOpacity2D;
        emit contentOpacity2DChanged();
    }
}

void GeoMapItem::_updatePosition()
{
    if (!_scene || !_coordinate.isValid()) {
        _setScenePosition(QVector3D());
        _setProjected(false);
        return;
    }

    double altitude = 0.0;
    if (_altitudeMode == AltitudeMode::Absolute) {
        altitude = std::isfinite(_coordinate.altitude()) ? _coordinate.altitude() : 0.0;
    } else if (_surfaceModel) {
        altitude = _surfaceModel->terrainHeightAt(_coordinate);
    }

    const QPointF world = TileMath::geoToWorld(_coordinate);
    const double sceneZ = altitude * _scene->verticalScale() * _scene->terrainScale();
    const QPointF origin = _scene->sceneOrigin();
    _setScenePosition(QVector3D(static_cast<float>(world.x() - origin.x()), static_cast<float>(world.y() - origin.y()),
                                static_cast<float>(sceneZ)));

    GeoMapCamera* const camera = _scene->camera();
    if (!camera) {
        _setProjected(false);
        return;
    }
    const auto screen = camera->worldToScreen(world, sceneZ);
    if (!screen) {
        _setProjected(false);
        return;
    }
    setPosition(*screen - _anchorPoint);
    _setProjected(true);
}
