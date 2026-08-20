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
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtQmlIntegration/QtQmlIntegration>

#include "TileMath.h"

class GeoScene;
class GeoMapCamera;
class HeightField;
class HeightSource;
class QNetworkAccessManager;
class QTimer;
class SurfaceModel;
class TileImageSource;

Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("HeightField.h")

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
    Q_PROPERTY(bool statsEnabled READ statsEnabled WRITE setStatsEnabled NOTIFY statsEnabledChanged)
    Q_PROPERTY(QString statsText READ statsText NOTIFY statsTextChanged)
    Q_PROPERTY(bool capturing READ capturing NOTIFY capturingChanged)

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
        EdgeLodDeltasRole,               ///< {N,S,W,E} coarser-neighbor LOD deltas for edge stitching
        TileXRole,                       ///< slippy tile x of the patch key
        TileYRole,                       ///< slippy tile y of the patch key
    };

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    bool terrain() const { return _terrain; }

    /// Real terrain elevations (see TerrariumTileFetcher);
    /// false gives a flat z=0 surface. debugHills overrides either.
    void setTerrain(bool terrain);

    bool debugHills() const { return _debugHills; }

    void setDebugHills(bool debugHills);

    QString mapType() const { return _mapType; }

    /// Provider type string for tile imagery (see TileImageSource). Empty
    /// disables imagery: TileImageRole stays a null QImage for every patch.
    void setMapType(const QString& mapType);

    int gridSize() const;

    /// The shared terrain field patches sample from. Owned here; replaced
    /// whenever the surface model is rebuilt.
    HeightField* heightField() const { return _heightField; }

    /// Best-estimate terrain height (true meters) at a coordinate: real data
    /// where loaded, coarser estimate or 0 elsewhere (see HeightField). Emits
    /// terrainHeightsChanged as estimates improve.
    Q_INVOKABLE double terrainHeightAt(const QGeoCoordinate& coordinate) const;

    /// SurfaceModel::kMaxRangeMultiplier, exposed so the scene camera's far
    /// clip plane can cover the full retained patch range
    double maxRangeMultiplier() const;

    int patchCount() const { return _keys.count(); }

    int pendingCount() const;
    int maxZoomLevel() const;

    /// Tile image requests awaiting delivery or failure
    int pendingImageCount() const { return _imageRequestKey.count(); }

    /// Network manager for tile imagery fetches, applied at the next
    /// setMapType (test injection seam)
    void setTileImageNetworkManager(QNetworkAccessManager* networkManager)
    {
        _tileImageNetworkManager = networkManager;
    }

    bool statsEnabled() const { return _statsEnabled; }  // GCOVR_EXCL_LINE

    /// Enables the once-per-second perf counter sampling behind statsText:
    /// model update rate/duration, patch churn, and fallback composites
    void setStatsEnabled(bool enabled);

    QString statsText() const { return _statsText; }

    bool capturing() const { return _capturing; }

    /// Starts recording the per-second perf samples (plus camera context)
    Q_INVOKABLE void startCapture();

    /// Stops recording and writes the samples as CSV; returns the file path
    /// (empty on write failure)
    Q_INVOKABLE QString stopCapture();

    /// Diagnostic pass over the current patch set: reports coverage holes,
    /// boundary height cliffs, and bad height data, with the reason each
    /// exists (see SurfaceAnalysis). The report goes to the debug output.
    Q_INVOKABLE void analyzeSurface() const;

#ifdef QGC_UNITTEST_BUILD
    /// Test hook: completes all capped SurfaceModel update passes synchronously
    void drainUpdates();
#endif

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void sceneChanged();
    void terrainChanged();
    void debugHillsChanged();
    void mapTypeChanged();
    void statsChanged();
    void statsEnabledChanged();
    void statsTextChanged();
    void capturingChanged();
    /// terrainHeightAt answers changed somewhere: consumers re-query
    void terrainHeightsChanged();

private slots:
    void _patchAdded(const TileMath::TileKey& key);
    void _patchReady(const TileMath::TileKey& key);
    void _patchEdgeDeltasChanged(const TileMath::TileKey& key);
    void _patchRemoved(const TileMath::TileKey& key);
    void _sceneOriginChanged();
    void _tileImageReady(int requestId, const QImage& image);
    void _tileImageFailed(int requestId);
    void _statsTick();

private:
    void _rebuildSurfaceModel();
    void _resetImagery();
    void _requestTileImage(const TileMath::TileKey& key);
    void _retryFailedImages();
    void _notifyTileImageChanged(const TileMath::TileKey& key);
    void _invalidateFallbacks(const TileMath::TileKey& tile);
    void _notifyCoveredMayHaveChanged(const TileMath::TileKey& changedKey);
    void _scheduleStatsChanged();
    void _startStatsSampling();
    QImage _fallbackImage(const TileMath::TileKey& key) const;
    QImage _compositeFromChildren(const TileMath::TileKey& key) const;
    QImage _cropFromAncestor(const TileMath::TileKey& key) const;
    bool _fallbackAvailable(const TileMath::TileKey& key) const;
    GeoMapCamera* _camera() const;

    static constexpr int kMaxRetiredImages = 128;         ///< tiles kept after patch removal (fallback source)
    static constexpr int kMaxAncestorFallbackLevels = 8;  ///< how far up the quadtree fallback looks
    static constexpr int kImageRetryMs = 3000;            ///< pacing for re-requesting failed tile images

    GeoScene* _scene = nullptr;
    HeightSource* _heightSource = nullptr;
    HeightField* _heightField = nullptr;
    SurfaceModel* _surfaceModel = nullptr;
    bool _terrain = false;
    bool _debugHills = false;
    QString _mapType;
    TileImageSource* _tileSource = nullptr;
    QNetworkAccessManager* _tileImageNetworkManager = nullptr;  ///< test injection seam
    QHash<TileMath::TileKey, QImage> _tileImages;               ///< delivered images: live patches + retired fallbacks
    /// Built fallbacks (incl. null misses) keyed by patch; rebuilding on every
    /// data() call cost ~1ms per new patch during zoom churn. Invalidated when
    /// a source tile changes (see _invalidateFallbacks), dropped with the row.
    mutable QHash<TileMath::TileKey, QImage> _fallbackCache;
    QList<TileMath::TileKey> _retiredOrder;            ///< retired keys oldest-first, for eviction
    QHash<int, TileMath::TileKey> _imageRequestKey;    ///< in-flight image request id -> patch
    QHash<TileMath::TileKey, int> _imageRequestByKey;  ///< reverse map for cancellation
    QSet<TileMath::TileKey> _failedImageKeys;          ///< resident patches awaiting an image retry
    QTimer* _imageRetryTimer = nullptr;
    QList<TileMath::TileKey> _keys;                    ///< row order; data fetched from SurfaceModel by key

    // Perf counters sampled by _statsTick (see setStatsEnabled)
    QTimer* _statsTimer = nullptr;
    bool _statsEnabled = false;
    bool _statsNotifyPending = false;  ///< coalesces statsChanged to one emit per event-loop pass
    QString _statsText;
    int _statAdds = 0;
    int _statRemoves = 0;
    mutable int _statComposites = 0;  ///< fallback images built (data() is const)

    // Capture recording (see startCapture)
    bool _capturing = false;
    QStringList _captureRows;
    QElapsedTimer _captureClock;
    double _captureWorstMaxMs = 0.0;  ///< session-worst single update, for the stop verdict
    double _captureWorstAvgMs = 0.0;
};
