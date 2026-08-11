/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QRectF>

#include "HeightSource.h"

class QImage;
class QNetworkAccessManager;
class QNetworkReply;

/// HeightSource over the AWS Open Data Terrain Tiles (terrarium encoding): real
/// elevations at every patch zoom from one slippy PNG tile per patch, cache-first
/// through QGC's shared tile database with a direct network fallback on miss
/// (fetched tiles are stored back).
///
/// A patch at z/x/y maps to the terrarium tile at the same z/x/y, so fetch cost
/// is constant across the LOD range — no flat floor or blend band like the fixed
/// resolution Copernicus pipeline needed. Above the dataset's top zoom the z15
/// ancestor tile is fetched and its sub-window sampled instead.
///
/// Vertex heights are sampled bilinearly between pixel centers, clamped at tile
/// edges (terrarium tiles don't share edge pixels; the ~half-pixel clamp error
/// can show as hairline cracks between neighboring patches — a neighbor-stitch
/// pass can replace the clamp later without changing this interface).
class TerrariumHeightSource : public HeightSource
{
    Q_OBJECT

public:
    /// \a networkManager overrides the internally created one (test injection seam)
    explicit TerrariumHeightSource(QObject* parent = nullptr, QNetworkAccessManager* networkManager = nullptr);

    /// Highest zoom the terrarium dataset serves (deeper patches sample the ancestor)
    static constexpr int kMaxTileZoom = 15;

    int requestPatchHeights(const TileMath::TileKey& key, int gridSize) final;
    void cancelRequest(int requestId) final;

    int pendingCount() const { return _pending.count(); }

    /// Tile key submitted by the most recent fetch (test observability for the
    /// deep-zoom ancestor path)
    TileMath::TileKey lastFetchKey() const { return _lastFetchKey; }

private:
    struct PendingRequest
    {
        TileMath::TileKey fetchKey;  ///< tile actually fetched (ancestor for deep zooms)
        QRectF subWindow;            ///< patch extent within the fetched tile, unit UV from the NW corner
        int gridSize = 0;
    };

    void _failAsync(int requestId);
    void _fetchFromNetwork(const TileMath::TileKey& fetchKey);
    void _deliverAll(const TileMath::TileKey& fetchKey, const QImage& image);
    void _failAll(const TileMath::TileKey& fetchKey);
    void _deliver(int requestId, const QImage& image);
    void _finishFailed(int requestId);

    QNetworkAccessManager* _networkManager = nullptr;
    const int _mapId;
    QHash<int, PendingRequest> _pending;
    QHash<TileMath::TileKey, QList<int>> _waiters;            ///< request ids sharing the in-flight fetch of a tile
    QHash<TileMath::TileKey, QNetworkReply*> _activeReplies;  ///< in-flight network fetches by tile
    TileMath::TileKey _lastFetchKey;
};
