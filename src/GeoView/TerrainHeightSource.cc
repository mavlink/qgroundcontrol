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

    const int verticesPerEdge = gridSize + 1;
    const int expectedCount = verticesPerEdge * verticesPerEdge;

    if (key.zoom < kMinTerrainZoom) {
        // Flat zeros for coarse patches: see class comment
        _localPending.insert(requestId, QList<float>(expectedCount, 0.0f));
        QTimer::singleShot(0, this, [this, requestId] { _deliverLocal(requestId); });
        return requestId;
    }

    const QPointF minCorner = TileMath::tileMinCorner(key);
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const double step = span / gridSize;
    constexpr double cell = TerrainTileCopernicus::kTileValueSpacingDegrees;

    QList<QGeoCoordinate> coordinates;  // four surrounding grid-cell samples per vertex
    QList<QPointF> fractions;
    coordinates.reserve(qsizetype(4) * expectedCount);
    fractions.reserve(expectedCount);
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
    _queryPending.insert(requestId, PendingQuery{query, fractions, heightScaleForZoom(key.zoom)});
    // Queued: the terrain pipeline answers synchronously on cache hits, but the
    // HeightSource contract promises delivery only after this call returns
    connect(
        query, &TerrainQueryInterface::coordinateHeightsReceived, this,
        [this, requestId](bool success, const QList<double>& heights) { _queryFinished(requestId, success, heights); },
        Qt::QueuedConnection);
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
    emit patchHeightsReady(requestId, converted);
}
