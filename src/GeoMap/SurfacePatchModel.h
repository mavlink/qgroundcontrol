/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtQmlIntegration/QtQmlIntegration>

#include "TileMath.h"

class GeoScene;
class GeoMapCamera;
class HeightSource;
class SurfaceModel;
class TileImageSource;

Q_MOC_INCLUDE("GeoScene.h")

/// QML bridge between SurfaceModel and the Repeater3D that renders the surface:
/// one row per active patch, updated incrementally (no model resets on pan/zoom).
///
/// Patch positions are exposed in scene coordinates (see GeoScene): world
/// mercator meters relative to the scene's re-anchorable origin.
class SurfacePatchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(bool terrain READ terrain WRITE setTerrain NOTIFY terrainChanged)
    Q_PROPERTY(bool debugHills READ debugHills WRITE setDebugHills NOTIFY debugHillsChanged)
    Q_PROPERTY(QString mapType READ mapType WRITE setMapType NOTIFY mapTypeChanged)
    Q_PROPERTY(int gridSize READ gridSize CONSTANT)
    Q_PROPERTY(double maxRangeMultiplier READ maxRangeMultiplier CONSTANT)
    Q_PROPERTY(int patchCount READ patchCount NOTIFY statsChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY statsChanged)
    Q_PROPERTY(int maxZoomLevel READ maxZoomLevel NOTIFY statsChanged)

public:
    explicit SurfacePatchModel(QObject* parent = nullptr);
    ~SurfacePatchModel() override;

    enum Roles
    {
        CenterXRole = Qt::UserRole + 1,  ///< patch center x in scene meters
        CenterYRole,                     ///< patch center y in scene meters
        SpanRole,                        ///< patch edge length (m)
        ZoomRole,                        ///< quadtree zoom level
        HeightsRole,                     ///< vertex height grid (empty = flat)
        ReadyRole,
        CoveredRole,                     ///< pending but overlapped by a ready patch: delegate suppresses rendering
        TileImageRole,                   ///< drapable image: own tile, or ancestor/descendant fallback while loading
        HasTileImageRole,                ///< QImage is opaque to QML; bool validity for bindings
    };

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    bool terrain() const { return _terrain; }

    /// Real terrain elevations from QGC's terrain pipeline (see TerrainHeightSource);
    /// false gives a flat z=0 surface. debugHills overrides either.
    void setTerrain(bool terrain);

    bool debugHills() const { return _debugHills; }

    void setDebugHills(bool debugHills);

    QString mapType() const { return _mapType; }

    /// Provider type string for tile imagery (see TileImageSource). Empty
    /// disables imagery: TileImageRole stays a null QImage for every patch.
    void setMapType(const QString& mapType);

    int gridSize() const;

    /// SurfaceModel::kMaxRangeMultiplier, exposed so the scene camera's far
    /// clip plane can cover the full retained patch range
    double maxRangeMultiplier() const;

    int patchCount() const { return _keys.count(); }

    int pendingCount() const;
    int maxZoomLevel() const;

    /// Diagnostic pass over the current patch set: reports coverage holes,
    /// boundary height cliffs, and bad height data, with the reason each
    /// exists (see SurfaceAnalysis). The report goes to the debug output.
    Q_INVOKABLE void analyzeSurface() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void sceneChanged();
    void terrainChanged();
    void debugHillsChanged();
    void mapTypeChanged();
    void statsChanged();

private slots:
    void _patchAdded(const TileMath::TileKey& key);
    void _patchReady(const TileMath::TileKey& key);
    void _patchRemoved(const TileMath::TileKey& key);
    void _sceneOriginChanged();
    void _tileImageReady(int requestId, const QImage& image);
    void _tileImageFailed(int requestId);

private:
    void _rebuildSurfaceModel();
    void _resetImagery();
    void _requestTileImage(const TileMath::TileKey& key);
    void _notifyTileImageChanged(const TileMath::TileKey& key);
    void _notifyCoveredMayHaveChanged();
    QImage _fallbackImage(const TileMath::TileKey& key) const;
    bool _fallbackAvailable(const TileMath::TileKey& key) const;
    GeoMapCamera* _camera() const;

    static constexpr int kMaxRetiredImages = 128;         ///< tiles kept after patch removal (fallback source)
    static constexpr int kMaxAncestorFallbackLevels = 8;  ///< how far up the quadtree fallback looks

    GeoScene* _scene = nullptr;
    HeightSource* _heightSource = nullptr;
    SurfaceModel* _surfaceModel = nullptr;
    bool _terrain = false;
    bool _debugHills = false;
    QString _mapType;
    TileImageSource* _tileSource = nullptr;
    QHash<TileMath::TileKey, QImage> _tileImages;      ///< delivered images: live patches + retired fallbacks
    QList<TileMath::TileKey> _retiredOrder;            ///< retired keys oldest-first, for eviction
    QHash<int, TileMath::TileKey> _imageRequestKey;    ///< in-flight image request id -> patch
    QHash<TileMath::TileKey, int> _imageRequestByKey;  ///< reverse map for cancellation
    QList<TileMath::TileKey> _keys;                    ///< row order; data fetched from SurfaceModel by key
};
