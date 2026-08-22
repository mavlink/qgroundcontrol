/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QPointF>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>

#include <vector>

class GeoMapCamera;
class GeoScene;
class QAbstractItemModel;
class SurfacePatchModel;

Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("SurfacePatchModel.h")
Q_MOC_INCLUDE("QtCore/QAbstractItemModel")

/// Batched screen-space marker layer: renders one identical sprite per model
/// row as a single scene-graph geometry node (one draw call, no per-marker
/// QObjects). Use instead of a Repeater of GeoMapItems for high-count marker
/// types (e.g. camera trigger points).
///
/// The model must expose a QObject per row at Qt::UserRole (QmlObjectListModel
/// contract) with a QGeoCoordinate "coordinate" property; alternatively set
/// coordinates to a plain list of QGeoCoordinates. Markers clamp to the
/// rendered terrain surface and are center-anchored at spriteSize pixels
/// square, in both 2D and 3D camera modes. sprite must be a texture provider
/// (an Item with layer.enabled or a ShaderEffectSource).
class GeoMapSpriteLayer : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(SurfacePatchModel* surfaceModel READ surfaceModel WRITE setSurfaceModel NOTIFY surfaceModelChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantList coordinates READ coordinates WRITE setCoordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(QQuickItem* sprite READ sprite WRITE setSprite NOTIFY spriteChanged)
    Q_PROPERTY(qreal spriteSize READ spriteSize WRITE setSpriteSize NOTIFY spriteSizeChanged)

public:
    explicit GeoMapSpriteLayer(QQuickItem* parent = nullptr);

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    SurfacePatchModel* surfaceModel() const { return _surfaceModel; }

    void setSurfaceModel(SurfacePatchModel* surfaceModel);

    QAbstractItemModel* model() const { return _model; }

    void setModel(QAbstractItemModel* model);

    /// Coordinate list alternative to model (model takes precedence when set)
    QVariantList coordinates() const { return _coordinates; }

    void setCoordinates(const QVariantList& coordinates);

    QQuickItem* sprite() const { return _sprite; }

    void setSprite(QQuickItem* sprite);

    /// On-screen sprite width/height in (logical) pixels
    qreal spriteSize() const { return _spriteSize; }

    void setSpriteSize(qreal spriteSize);

signals:
    void sceneChanged();
    void surfaceModelChanged();
    void modelChanged();
    void coordinatesChanged();
    void spriteChanged();
    void spriteSizeChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData) override;

private slots:
    void _markPointsDirty();
    void _scheduleTerrainResample();

private:
    void _connectCamera();
    void _rebuildPointCache();

    struct CachedPoint
    {
        QGeoCoordinate coordinate;
        QPointF world;        ///< mercator meters
        double terrainAlt{};  ///< sampled surface height (meters)
    };

    GeoScene* _scene = nullptr;
    SurfacePatchModel* _surfaceModel = nullptr;
    QAbstractItemModel* _model = nullptr;
    QVariantList _coordinates;
    QQuickItem* _sprite = nullptr;
    GeoMapCamera* _connectedCamera = nullptr;
    QTimer _terrainResampleTimer;  ///< coalesces per-patch terrain-height bursts into one cache rebuild
    qreal _spriteSize = 24.0;
    bool _pointsDirty = true;
    bool _warnedNotProvider = false;
    std::vector<CachedPoint> _points;
};
