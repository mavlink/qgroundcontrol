/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainHeightSource.h"

#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <cmath>

#include "TerrainQueryInterface.h"
#include "TerrainTileCopernicus.h"

namespace {

/// Max 0.01-degree elevation tiles a patch may touch per axis; patches spanning
/// more sample sparsely and interpolate (the extra detail is sub-pixel there)
constexpr int kMaxFetchTilesPerAxis = 4;

/// Bilinear upsample of a square vertex grid (row-major from NW) to a finer one.
/// Requires sparseGridSize >= 1 and fullGridSize > sparseGridSize (the caller's
/// gridSize < 1 boundary check guarantees this).
QList<float> upsampleGrid(const QList<float>& sparse, int sparseGridSize, int fullGridSize)
{
    const int sparseEdge = sparseGridSize + 1;
    const int fullEdge = fullGridSize + 1;
    QList<float> full;
    full.reserve(qsizetype(fullEdge) * fullEdge);
    for (int row = 0; row < fullEdge; row++) {
        const double v = (static_cast<double>(row) * sparseGridSize) / fullGridSize;
        const int v0 = qMin(static_cast<int>(v), sparseGridSize - 1);
        const double fv = v - v0;
        for (int col = 0; col < fullEdge; col++) {
            const double u = (static_cast<double>(col) * sparseGridSize) / fullGridSize;
            const int u0 = qMin(static_cast<int>(u), sparseGridSize - 1);
            const double fu = u - u0;
            const double north = (sparse.at((qsizetype(v0) * sparseEdge) + u0) * (1.0 - fu)) +
                                 (sparse.at((qsizetype(v0) * sparseEdge) + u0 + 1) * fu);
            const double south = (sparse.at((qsizetype(v0 + 1) * sparseEdge) + u0) * (1.0 - fu)) +
                                 (sparse.at((qsizetype(v0 + 1) * sparseEdge) + u0 + 1) * fu);
            full.append(static_cast<float>((north * (1.0 - fv)) + (south * fv)));
        }
    }
    return full;
}

}  // namespace

TerrainHeightSource::TerrainHeightSource(QObject* parent) : HeightSource(parent) {}

double TerrainHeightSource::heightScaleForZoom(int zoom)
{
    if (zoom < kMinTerrainZoom) {
        return 0.0;
    }
    const int bandSteps = kFullTerrainZoom - kMinTerrainZoom + 1;
    return qMin(1.0, static_cast<double>(zoom - kMinTerrainZoom + 1) / bandSteps);
}

int TerrainHeightSource::requestPatchHeights(const TileMath::TileKey& key, int gridSize)
{
    const int requestId = _nextRequestId();
    if (gridSize < 1) {
        QTimer::singleShot(0, this, [this, requestId] { emit patchHeightsFailed(requestId); });
        return requestId;
    }

    const int expectedCount = (gridSize + 1) * (gridSize + 1);

    if (key.zoom < kMinTerrainZoom) {
        // Flat zeros for coarse patches: see class comment
        _localPending.insert(requestId, QList<float>(expectedCount, 0.0f));
        QTimer::singleShot(0, this, [this, requestId] { _deliverLocal(requestId); });
        return requestId;
    }

    const QPointF minCorner = TileMath::tileMinCorner(key);
    const double span = TileMath::tileSpanAtZoom(key.zoom);

    // Cap elevation tiles touched: patches spanning many fixed 0.01-degree tiles
    // sample a sparse vertex grid instead (upsampled to the full grid on delivery)
    const double lonSpanDeg = (span / TileMath::worldSize()) * 360.0;
    int sampleGridSize = gridSize;
    if (lonSpanDeg > (kMaxFetchTilesPerAxis * TerrainTileCopernicus::kTileSizeDegrees)) {
        sampleGridSize = qMin(gridSize, kMaxFetchTilesPerAxis - 1);
    }

    const int verticesPerEdge = sampleGridSize + 1;
    const double step = span / sampleGridSize;
    constexpr double cell = TerrainTileCopernicus::kTileValueSpacingDegrees;

    QList<QGeoCoordinate> coordinates;  // four surrounding grid-cell samples per vertex
    QList<QPointF> fractions;
    coordinates.reserve(qsizetype(4) * verticesPerEdge * verticesPerEdge);
    fractions.reserve(qsizetype(verticesPerEdge) * verticesPerEdge);
    // Row-major from the north-west corner, matching the HeightSource contract
    for (int row = 0; row < verticesPerEdge; row++) {
        const double y = minCorner.y() + span - (row * step);
        for (int col = 0; col < verticesPerEdge; col++) {
            const QGeoCoordinate vertex = TileMath::worldToGeo(QPointF(minCorner.x() + (col * step), y));
            const double latCells = vertex.latitude() / cell;
            const double lonCells = vertex.longitude() / cell;
            const double lat0 = std::floor(latCells);
            const double lon0 = std::floor(lonCells);
            fractions.append(QPointF(lonCells - lon0, latCells - lat0));
            // Sample cell centers so each query floor-indexes to the intended cell
            for (int dLat = 0; dLat <= 1; dLat++) {
                for (int dLon = 0; dLon <= 1; dLon++) {
                    coordinates.append(QGeoCoordinate((lat0 + dLat + 0.5) * cell, (lon0 + dLon + 0.5) * cell));
                }
            }
        }
    }

    TerrainOfflineQuery* const query = new TerrainOfflineQuery(this);
    _queryPending.insert(requestId,
                         PendingQuery{query, fractions, heightScaleForZoom(key.zoom), sampleGridSize, gridSize});
    // Queued: the terrain pipeline answers synchronously on cache hits, but the
    // HeightSource contract promises delivery only after this call returns
    connect(
        query, &TerrainQueryInterface::coordinateHeightsReceived, this,
        [this, requestId](bool success, const QList<double>& heights) { _queryFinished(requestId, success, heights); },
        Qt::QueuedConnection);
    _lastQueryCoordinateCount = coordinates.count();
    query->requestCoordinateHeights(coordinates);
    return requestId;
}

void TerrainHeightSource::cancelRequest(int requestId)
{
    _localPending.remove(requestId);
    const auto it = _queryPending.constFind(requestId);
    if (it != _queryPending.constEnd()) {
        // TerrainTileManager tracks queries via QPointer, so deleting one cancels safely
        it.value().query->deleteLater();
        _queryPending.erase(it);
    }
}

void TerrainHeightSource::_deliverLocal(int requestId)
{
    const auto it = _localPending.constFind(requestId);
    if (it == _localPending.constEnd()) {
        return;  // cancelled
    }
    const QList<float> heights = it.value();
    _localPending.erase(it);
    emit patchHeightsReady(requestId, heights);
}

void TerrainHeightSource::_queryFinished(int requestId, bool success, const QList<double>& heights)
{
    const auto it = _queryPending.constFind(requestId);
    if (it == _queryPending.constEnd()) {
        return;  // cancelled after the result was already queued
    }
    const PendingQuery pending = it.value();
    _queryPending.erase(it);
    pending.query->deleteLater();

    if (!success || (heights.count() != (qsizetype(4) * pending.fractions.count()))) {
        emit patchHeightsFailed(requestId);
        return;
    }

    // Bilinear blend of the four grid-cell samples surrounding each vertex
    QList<float> converted;
    converted.reserve(pending.fractions.count());
    for (qsizetype vertex = 0; vertex < pending.fractions.count(); vertex++) {
        const double fLon = pending.fractions.at(vertex).x();
        const double fLat = pending.fractions.at(vertex).y();
        const qsizetype base = vertex * 4;
        const double south = (heights.at(base) * (1.0 - fLon)) + (heights.at(base + 1) * fLon);
        const double north = (heights.at(base + 2) * (1.0 - fLon)) + (heights.at(base + 3) * fLon);
        converted.append(static_cast<float>(((south * (1.0 - fLat)) + (north * fLat)) * pending.heightScale));
    }
    if (pending.sampleGridSize < pending.gridSize) {
        converted = upsampleGrid(converted, pending.sampleGridSize, pending.gridSize);
    }
    emit patchHeightsReady(requestId, converted);
}
