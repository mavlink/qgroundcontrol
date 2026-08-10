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
#include <QtCore/QList>
#include <QtCore/QPointF>

#include "HeightSource.h"

class TerrainOfflineQuery;

/// HeightSource adapter over QGC's shared terrain pipeline (TerrainTileManager via
/// TerrainOfflineQuery): real AMSL elevations from the terrain tile cache, downloading
/// missing elevation tiles as needed.
///
/// The pipeline returns one value per 1-arc-second elevation grid cell (floor-indexed,
/// no interpolation), which renders as terraced steps once mesh vertices are spaced
/// closer than the ~30m cells. Each vertex therefore samples its four surrounding
/// grid cells and blends them bilinearly.
///
/// Patches coarser than kMinTerrainZoom get flat zero heights delivered locally: their
/// sample grid would touch hundreds of 0.01-degree elevation tiles each (a fetch storm),
/// while terrain relief at those view distances is only a few pixels. To avoid a single
/// full-height cliff at that boundary, heights ramp in across the blend band from
/// kMinTerrainZoom (scaled down) to kFullTerrainZoom (true height): each ring boundary
/// then steps by only a fraction of the local terrain height.
class TerrainHeightSource : public HeightSource
{
    Q_OBJECT

public:
    explicit TerrainHeightSource(QObject* parent = nullptr);

    /// Coarsest patch zoom that queries real terrain data (fetch-cost/visual tradeoff)
    static constexpr int kMinTerrainZoom = 12;
    /// First zoom delivered at full height (end of the blend band)
    static constexpr int kFullTerrainZoom = 14;

    /// Height scale for a patch zoom: 0 below the terrain threshold, ramping to 1
    /// at kFullTerrainZoom
    static double heightScaleForZoom(int zoom);

    int requestPatchHeights(const TileMath::TileKey& key, int gridSize) final;
    void cancelRequest(int requestId) final;

private:
    void _deliverLocal(int requestId);
    void _queryFinished(int requestId, bool success, const QList<double>& heights);

    struct PendingQuery
    {
        TerrainOfflineQuery* query = nullptr;
        QList<QPointF> fractions;  ///< per vertex: position within its grid cell (x=lon, y=lat)
        double heightScale = 1.0;  ///< blend-band scale for the patch zoom
    };

    QHash<int, QList<float>> _localPending;  ///< coarse-zoom results awaiting queued delivery
    QHash<int, PendingQuery> _queryPending;  ///< in-flight terrain queries
};
