/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoMapSpriteLayer.h"

#include <QtCore/QAbstractItemModel>
#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGTextureMaterial>
#include <QtQuick/QSGTextureProvider>

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "QGCLoggingCategory.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(GeoMapSpriteLayerLog, "GeoMap.GeoMapSpriteLayer")

GeoMapSpriteLayer::GeoMapSpriteLayer(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    _terrainResampleTimer.setSingleShot(true);
    _terrainResampleTimer.setInterval(250);
    connect(&_terrainResampleTimer, &QTimer::timeout, this, &GeoMapSpriteLayer::_markPointsDirty);
}

void GeoMapSpriteLayer::setScene(GeoScene* scene)
{
    if (scene == _scene) {
        return;
    }
    qCDebug(GeoMapSpriteLayerLog) << "scene" << (scene ? "set" : "cleared");
    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }
    _scene = scene;
    if (_scene) {
        connect(_scene, &GeoScene::sceneOriginChanged, this, &QQuickItem::update);
        connect(_scene, &GeoScene::terrainScaleChanged, this, &QQuickItem::update);
        connect(_scene, &GeoScene::cameraChanged, this, &GeoMapSpriteLayer::_connectCamera);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            _connectCamera();
            emit sceneChanged();
        });
    }
    _connectCamera();
    emit sceneChanged();
}

void GeoMapSpriteLayer::setSurfaceModel(SurfacePatchModel* surfaceModel)
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
                &GeoMapSpriteLayer::_scheduleTerrainResample);
        connect(_surfaceModel, &QObject::destroyed, this, [this] {
            _surfaceModel = nullptr;
            _markPointsDirty();
            emit surfaceModelChanged();
        });
    }
    _markPointsDirty();
    emit surfaceModelChanged();
}

void GeoMapSpriteLayer::setModel(QAbstractItemModel* model)
{
    if (model == _model) {
        return;
    }
    qCDebug(GeoMapSpriteLayerLog) << "model" << (model ? "set" : "cleared");
    if (_model) {
        disconnect(_model, nullptr, this, nullptr);
    }
    _model = model;
    if (_model) {
        connect(_model, &QAbstractItemModel::rowsInserted, this, &GeoMapSpriteLayer::_markPointsDirty);
        connect(_model, &QAbstractItemModel::rowsRemoved, this, &GeoMapSpriteLayer::_markPointsDirty);
        connect(_model, &QAbstractItemModel::modelReset, this, &GeoMapSpriteLayer::_markPointsDirty);
        connect(_model, &QAbstractItemModel::dataChanged, this, &GeoMapSpriteLayer::_markPointsDirty);
        connect(_model, &QAbstractItemModel::layoutChanged, this, &GeoMapSpriteLayer::_markPointsDirty);
        connect(_model, &QObject::destroyed, this, [this] {
            _model = nullptr;
            _markPointsDirty();
            emit modelChanged();
        });
    }
    _markPointsDirty();
    emit modelChanged();
}

void GeoMapSpriteLayer::setCoordinates(const QVariantList& coordinates)
{
    if (coordinates == _coordinates) {
        return;
    }
    _coordinates = coordinates;
    _markPointsDirty();
    emit coordinatesChanged();
}

void GeoMapSpriteLayer::setSprite(QQuickItem* sprite)
{
    if (sprite == _sprite) {
        return;
    }
    if (_sprite) {
        disconnect(_sprite, nullptr, this, nullptr);
    }
    _sprite = sprite;
    _warnedNotProvider = false;
    if (_sprite) {
        connect(_sprite, &QObject::destroyed, this, [this] {
            _sprite = nullptr;
            update();
            emit spriteChanged();
        });
    }
    update();
    emit spriteChanged();
}

void GeoMapSpriteLayer::setSpriteSize(qreal spriteSize)
{
    if (spriteSize == _spriteSize) {
        return;
    }
    _spriteSize = spriteSize;
    update();
    emit spriteSizeChanged();
}

void GeoMapSpriteLayer::_markPointsDirty()
{
    _pointsDirty = true;
    update();
}

void GeoMapSpriteLayer::_scheduleTerrainResample()
{
    // Terrain streams in per patch: batch the burst into one resample
    if (!_terrainResampleTimer.isActive()) {
        _terrainResampleTimer.start();
    }
}

void GeoMapSpriteLayer::_connectCamera()
{
    GeoMapCamera* const camera = _scene ? _scene->camera() : nullptr;
    if (camera == _connectedCamera) {
        update();
        return;
    }
    if (_connectedCamera) {
        disconnect(_connectedCamera, nullptr, this, nullptr);
    }
    _connectedCamera = camera;
    if (_connectedCamera) {
        connect(_connectedCamera, &GeoMapCamera::scenePoseChanged, this, &QQuickItem::update);
        connect(_connectedCamera, &GeoMapCamera::viewportSizeChanged, this, &QQuickItem::update);
        connect(_connectedCamera, &GeoMapCamera::fieldOfViewChanged, this, &QQuickItem::update);
        connect(_connectedCamera, &QObject::destroyed, this, [this] {
            _connectedCamera = nullptr;
            update();
        });
    }
    update();
}

void GeoMapSpriteLayer::_rebuildPointCache()
{
    _pointsDirty = false;
    _points.clear();

    const auto appendCoordinate = [this](const QGeoCoordinate& coordinate) {
        if (!coordinate.isValid()) {
            return;
        }
        CachedPoint point;
        point.coordinate = coordinate;
        point.world = TileMath::geoToWorld(coordinate);
        point.terrainAlt = _surfaceModel ? _surfaceModel->terrainHeightAt(coordinate) : 0.0;
        _points.push_back(point);
    };

    if (_model) {
        const int rows = _model->rowCount();
        _points.reserve(rows);
        for (int row = 0; row < rows; ++row) {
            const QObject* const object = _model->data(_model->index(row, 0), Qt::UserRole).value<QObject*>();
            if (object) {
                appendCoordinate(object->property("coordinate").value<QGeoCoordinate>());
            }
        }
    } else {
        _points.reserve(_coordinates.size());
        for (const QVariant& variant : _coordinates) {
            appendCoordinate(variant.value<QGeoCoordinate>());
        }
    }
    qCDebug(GeoMapSpriteLayerLog) << "point cache rebuilt:" << _points.size() << "points";
}

// Runs on the render thread with the GUI thread blocked: safe to read the
// scene/camera/model and (re)build the point cache here.
QSGNode* GeoMapSpriteLayer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* /*updatePaintNodeData*/)
{
    auto* node = static_cast<QSGGeometryNode*>(oldNode);

    QSGTexture* texture = nullptr;
    if (_sprite) {
        if (_sprite->isTextureProvider()) {
            texture = _sprite->textureProvider()->texture();
            if (texture) {
                // FBO-layer textures (ShaderEffectSource) only render their
                // content when a consumer asks for it (as ShaderEffect does)
                if (auto* const dynamicTexture = qobject_cast<QSGDynamicTexture*>(texture)) {
                    dynamicTexture->updateTexture();
                }
            } else {
                // Provider exists but its layer has not rendered yet: retry next frame
                QMetaObject::invokeMethod(this, qOverload<>(&QQuickItem::update), Qt::QueuedConnection);
            }
        } else if (!_warnedNotProvider) {
            // Caller contract: sprite must have layer.enabled or be a ShaderEffectSource
            qCWarning(GeoMapSpriteLayerLog) << "sprite item is not a texture provider";
            _warnedNotProvider = true;
        }
    }

    if (_pointsDirty) {
        _rebuildPointCache();
    }

    GeoMapCamera* const camera = _scene ? _scene->camera() : nullptr;
    if (!texture || !camera || _points.empty()) {
        delete node;
        return nullptr;
    }

    const double zScale = _scene->verticalScale() * _scene->terrainScale();
    std::vector<QPointF> screenPositions;
    screenPositions.reserve(_points.size());
    for (const CachedPoint& point : _points) {
        const auto screen = camera->worldToScreen(point.world, point.terrainAlt * zScale);
        if (screen) {
            screenPositions.push_back(*screen);
        }
    }
    if (screenPositions.empty()) {
        delete node;
        return nullptr;
    }

    QSGGeometry* geometry = nullptr;
    if (!node) {
        node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);
        auto* const material = new QSGTextureMaterial;
        material->setFiltering(QSGTexture::Linear);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    } else {
        geometry = node->geometry();
    }

    auto* const material = static_cast<QSGTextureMaterial*>(node->material());
    if (material->texture() != texture) {
        material->setTexture(texture);
        node->markDirty(QSGNode::DirtyMaterial);
    }

    const QRectF uv = texture->normalizedTextureSubRect();
    const float u0 = static_cast<float>(uv.left());
    const float v0 = static_cast<float>(uv.top());
    const float u1 = static_cast<float>(uv.right());
    const float v1 = static_cast<float>(uv.bottom());
    const float half = static_cast<float>(_spriteSize / 2.0);

    geometry->allocate(static_cast<int>(screenPositions.size()) * 6);
    QSGGeometry::TexturedPoint2D* vertex = geometry->vertexDataAsTexturedPoint2D();
    for (const QPointF& position : screenPositions) {
        const float cx = static_cast<float>(position.x());
        const float cy = static_cast<float>(position.y());
        vertex[0].set(cx - half, cy - half, u0, v0);
        vertex[1].set(cx + half, cy - half, u1, v0);
        vertex[2].set(cx - half, cy + half, u0, v1);
        vertex[3].set(cx + half, cy - half, u1, v0);
        vertex[4].set(cx + half, cy + half, u1, v1);
        vertex[5].set(cx - half, cy + half, u0, v1);
        vertex += 6;
    }
    node->markDirty(QSGNode::DirtyGeometry);

    return node;
}
