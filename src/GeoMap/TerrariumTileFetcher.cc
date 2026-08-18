/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrariumTileFetcher.h"

#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <memory>

#include "BilinearUV.h"
#include "ElevationMapProvider.h"
#include "HeightField.h"
#include "PatchGeometry.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTileFetcherQGC.h"

QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherLog, "GeoMap.TerrariumTileFetcher")
QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherVerboseLog, "GeoMap.TerrariumTileFetcher.Verbose")

static_assert(TerrariumTileFetcher::kMaxGridSize == PatchGeometry::kMaxGridSize,
              "fetcher grid cap must match the patch mesh cap");

namespace {

QString terrariumProviderType()
{
    return QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
}

/// Terrarium RGB decode at one pixel: height = (R*256 + G + B/256) - 32768 meters
double heightAtPixel(const QImage& image, int px, int py)
{
    const QRgb rgb = image.pixel(px, py);
    return (qRed(rgb) * 256.0) + qGreen(rgb) + (qBlue(rgb) / 256.0) - 32768.0;
}

/// Bilinear height at a unit-UV position (origin NW corner), pixel-center
/// convention, clamped at tile edges; terrarium decode per pixel
double heightAtUV(const QImage& image, double u, double v)
{
    const auto at = [&image](int x, int y) { return heightAtPixel(image, x, y); };
    return bilinearAtUV(image.width(), image.height(), u, v, at);
}

/// Samples the (gridSize+1)^2 vertex grid, row-major from the NW corner, from the
/// tile sub-window covering the patch
QList<float> sampleGrid(const QImage& image, const QRectF& subWindow, int gridSize)
{
    QList<float> heights;
    heights.reserve(qsizetype(gridSize + 1) * (gridSize + 1));
    for (int row = 0; row <= gridSize; row++) {
        const double v = subWindow.y() + (subWindow.height() * row / gridSize);
        for (int col = 0; col <= gridSize; col++) {
            const double u = subWindow.x() + (subWindow.width() * col / gridSize);
            heights.append(static_cast<float>(heightAtUV(image, u, v)));
        }
    }
    return heights;
}

/// Terrarium tiles are always 256x256; anything else (e.g. a CDN placeholder
/// image) is not elevation data
constexpr int kTileSizePixels = 256;

/// Decoded sampling-ready tile image; null when the body isn't a valid tile
QImage decodeTile(const QByteArray& data)
{
    QImage image;
    if (!image.loadFromData(data) || image.width() != kTileSizePixels || image.height() != kTileSizePixels) {
        return QImage();
    }
    return image.convertToFormat(QImage::Format_RGB32);
}

/// Full-tile decode to a pyramid grid, row-major from the NW corner
ElevationTilePyramid::Grid gridFromImage(const QImage& image)
{
    ElevationTilePyramid::Grid grid;
    grid.width = image.width();
    grid.height = image.height();
    grid.heights.reserve(qsizetype(grid.width) * grid.height);
    for (int py = 0; py < grid.height; py++) {
        for (int px = 0; px < grid.width; px++) {
            grid.heights.append(static_cast<float>(heightAtPixel(image, px, py)));
        }
    }
    return grid;
}

/// Tile actually fetched for a key: the key itself, or its ancestor at the
/// dataset's top zoom for deeper keys
TileMath::TileKey fetchKeyFor(const TileMath::TileKey& key)
{
    if (key.zoom <= TerrariumTileFetcher::kMaxTileZoom) {
        return key;
    }
    const int shift = key.zoom - TerrariumTileFetcher::kMaxTileZoom;
    return TileMath::TileKey{key.x >> shift, key.y >> shift, TerrariumTileFetcher::kMaxTileZoom};
}

}  // namespace

TerrariumTileFetcher::TerrariumTileFetcher(QObject* parent, QNetworkAccessManager* networkManager)
    : HeightSource(parent),
      _networkManager(networkManager ? networkManager : new QNetworkAccessManager(this)),
      _mapId(UrlFactory::getQtMapIdFromProviderType(terrariumProviderType()))
{}

int TerrariumTileFetcher::requestPatchHeights(const TileMath::TileKey& key, int gridSize)
{
    const int requestId = _nextRequestId();

    if ((gridSize <= 0) || (gridSize > kMaxGridSize) || !TileMath::isValidKey(key)) {
        qCWarning(GeoMapTerrariumTileFetcherLog)
            << "requestPatchHeights rejected: key" << key << "gridSize" << gridSize;
        _pending.insert(requestId, PendingRequest{});
        _failAsync(requestId);
        return requestId;
    }
    if (_mapId < 0) {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "requestPatchHeights: elevation provider not registered";
        _pending.insert(requestId, PendingRequest{});
        _failAsync(requestId);
        return requestId;
    }

    // Deeper than the dataset: fetch the top-zoom ancestor and sample the
    // patch's sub-window of it
    PendingRequest pending;
    pending.gridSize = gridSize;
    pending.fetchKey = fetchKeyFor(key);
    if (key.zoom <= kMaxTileZoom) {
        pending.subWindow = QRectF(0, 0, 1, 1);
    } else {
        const int shift = key.zoom - kMaxTileZoom;
        const double scale = 1.0 / (1LL << shift);
        pending.subWindow = QRectF((key.x - (qint64(pending.fetchKey.x) << shift)) * scale,
                                   (key.y - (qint64(pending.fetchKey.y) << shift)) * scale, scale, scale);
    }
    _pending.insert(requestId, pending);
    _lastFetchKey = pending.fetchKey;

    const bool inFlight = _fetchInFlight(pending.fetchKey);
    _waiters[pending.fetchKey].append(requestId);
    qCDebug(GeoMapTerrariumTileFetcherVerboseLog)
        << "requestPatchHeights: key" << key << "requestId" << requestId << "fetchKey" << pending.fetchKey
        << (inFlight ? "(joining in-flight fetch)" : "(starting fetch)");
    if (inFlight) {
        return requestId;  // a fetch for this tile is already in flight; it serves all waiters
    }

    if (!_startFetch(pending.fetchKey)) {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "requestPatchHeights: could not start fetch for" << pending.fetchKey;
        _waiters.remove(pending.fetchKey);
        _failAsync(requestId);
    }
    return requestId;
}

bool TerrariumTileFetcher::requestTile(const TileMath::TileKey& key)
{
    if (!_heightField || !TileMath::isValidKey(key)) {
        qCWarning(GeoMapTerrariumTileFetcherLog)
            << "requestTile rejected: key" << key << "heightFieldSet" << (_heightField != nullptr);
        return false;
    }
    if (_mapId < 0) {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "requestTile: elevation provider not registered";
        return false;
    }

    const TileMath::TileKey fetchKey = fetchKeyFor(key);
    if (_heightField->hasTile(fetchKey) || _fieldRequests.contains(fetchKey)) {
        return true;
    }

    const bool inFlight = _fetchInFlight(fetchKey);
    _fieldRequests.insert(fetchKey);
    _lastFetchKey = fetchKey;
    qCDebug(GeoMapTerrariumTileFetcherVerboseLog)
        << "requestTile: fetchKey" << fetchKey << (inFlight ? "(joining in-flight fetch)" : "(starting fetch)");
    if (inFlight) {
        return true;  // piggyback on the fetch already serving patch waiters
    }

    if (!_startFetch(fetchKey)) {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "requestTile: could not start fetch for" << fetchKey;
        _fieldRequests.remove(fetchKey);
        return false;
    }
    return true;
}

bool TerrariumTileFetcher::_fetchInFlight(const TileMath::TileKey& fetchKey) const
{
    return _waiters.contains(fetchKey) || _fieldRequests.contains(fetchKey);
}

bool TerrariumTileFetcher::_startFetch(const TileMath::TileKey& fetchKey)
{
    const QString providerType = terrariumProviderType();
    QGCFetchTileTask* const task =
        QGeoFileTileCacheQGC::createFetchTileTask(providerType, fetchKey.x, fetchKey.y, fetchKey.zoom);
    connect(task, &QGCFetchTileTask::tileFetched, this, [this, fetchKey](QGCCacheTile* tile) {
        const std::unique_ptr<QGCCacheTile> guard(tile);  // caller-owned per task contract
        if (!_fetchInFlight(fetchKey)) {
            return;                                       // all interest cancelled while the lookup was in flight
        }
        // An unusable cached body (empty or corrupt) is a miss, not a
        // failure: failing here would re-read the same bytes on every retry,
        // blocking the network fallback until cache eviction
        const QImage image = tile ? decodeTile(tile->img) : QImage();
        if (image.isNull()) {
            qCDebug(GeoMapTerrariumTileFetcherLog)
                << "tile" << fetchKey << "cached entry unusable, fetching from network";
            _fetchFromNetwork(fetchKey);
            return;
        }
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog) << "tile" << fetchKey << "served from cache";
        _deliverAll(fetchKey, image);
    });
    connect(task, &QGCMapTask::error, this, [this, fetchKey](QGCMapTask::TaskType, const QString&) {
        if (!_fetchInFlight(fetchKey)) {
            return;
        }
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog) << "tile" << fetchKey << "not cached, fetching from network";
        _fetchFromNetwork(fetchKey);
    });

    if (!getQGCMapEngine()->addTask(task)) {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "could not queue cache lookup for" << fetchKey;
        task->deleteLater();  // never enqueued: the worker will not clean it up
        return false;
    }
    return true;
}

void TerrariumTileFetcher::cancelRequest(int requestId)
{
    const auto pendingIt = _pending.constFind(requestId);
    if (pendingIt == _pending.cend()) {
        return;
    }
    const TileMath::TileKey fetchKey = pendingIt->fetchKey;
    _pending.erase(pendingIt);

    const auto waitersIt = _waiters.find(fetchKey);
    if (waitersIt == _waiters.end()) {
        return;
    }
    waitersIt->removeOne(requestId);
    if (!waitersIt->isEmpty()) {
        return;  // other requests still wait on this tile's fetch
    }
    _waiters.erase(waitersIt);
    if (_fieldRequests.contains(fetchKey)) {
        return;  // the field still wants this tile: keep the fetch alive
    }
    QNetworkReply* const reply = _activeReplies.take(fetchKey);
    if (reply) {
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog) << "cancelRequest: aborting network fetch for" << fetchKey;
        reply->abort();        // finished handler is a no-op once no waiters remain
        reply->deleteLater();  // don't rely on abort emitting finished for cleanup
    }
}

void TerrariumTileFetcher::_failAsync(int requestId)
{
    // Deliver asynchronously so callers see one consistent flow
    QMetaObject::invokeMethod(
        this,
        [this, requestId] {
            if (_pending.contains(requestId)) {
                _finishFailed(requestId);
            }
        },
        Qt::QueuedConnection);
}

void TerrariumTileFetcher::_fetchFromNetwork(const TileMath::TileKey& fetchKey)
{
    if (_activeReplies.contains(fetchKey)) {
        return;  // a reply is already in flight for this tile
    }
    const QNetworkRequest request =
        QGeoTileFetcherQGC::getNetworkRequest(_mapId, fetchKey.x, fetchKey.y, fetchKey.zoom);
    QNetworkReply* const reply = _networkManager->get(request);
    _activeReplies.insert(fetchKey, reply);

    connect(reply, &QNetworkReply::finished, this, [this, fetchKey, reply] {
        reply->deleteLater();
        if (_activeReplies.value(fetchKey) != reply) {
            return;  // stale: cancelled (and possibly superseded by a newer fetch)
        }
        _activeReplies.remove(fetchKey);
        if (!_fetchInFlight(fetchKey)) {
            return;  // all interest cancelled (abort also lands here)
        }
        if (reply->error() != QNetworkReply::NoError) {
            _failAll(fetchKey, QStringLiteral("network error: %1 (http %2)")
                                   .arg(reply->errorString())
                                   .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            _failAll(fetchKey, QStringLiteral("network fetch returned empty body"));
            return;
        }
        // Never cache bodies that delivery would reject (e.g. HTTP-200 error
        // pages): a cached invalid tile would fail every retry from then on
        const QImage image = decodeTile(data);
        if (image.isNull()) {
            _failAll(fetchKey, QStringLiteral("network body is not a terrarium tile, %1 bytes").arg(data.size()));
            return;
        }
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog)
            << "tile" << fetchKey << "fetched from network," << data.size() << "bytes, caching";
        // Store back so the shared cache serves this tile from now on
        const QString providerType = terrariumProviderType();
        QGeoFileTileCacheQGC::cacheTile(providerType, fetchKey.x, fetchKey.y, fetchKey.zoom, data,
                                        UrlFactory::getImageFormat(providerType, data));
        _deliverAll(fetchKey, image);
    });
}

void TerrariumTileFetcher::_deliverAll(const TileMath::TileKey& fetchKey, const QImage& image)
{
    const bool forField = _fieldRequests.remove(fetchKey) && _heightField;
    if (forField) {
        _heightField->insertTile(fetchKey, gridFromImage(image));
    }

    const QList<int> requestIds = _waiters.take(fetchKey);
    qCDebug(GeoMapTerrariumTileFetcherVerboseLog)
        << "delivering tile" << fetchKey << "to" << requestIds.count() << "waiters, field insert:" << forField;
    for (int requestId : requestIds) {
        if (_pending.contains(requestId)) {
            _deliver(requestId, image);
        }
    }
}

void TerrariumTileFetcher::_failAll(const TileMath::TileKey& fetchKey, const QString& reason)
{
    // A failed tile inserts nothing: the field keeps its current estimate, and
    // clearing the in-flight key lets a later request retry
    _fieldRequests.remove(fetchKey);

    const QList<int> requestIds = _waiters.take(fetchKey);
    if (_shouldWarnFailure()) {
        qCWarning(GeoMapTerrariumTileFetcherLog)
            << "tile" << fetchKey << "failed:" << reason << "- failing" << requestIds.count() << "waiters";
    } else {
        qCDebug(GeoMapTerrariumTileFetcherLog) << "tile" << fetchKey << "failed:" << reason << "- failing"
                                               << requestIds.count() << "waiters (warning suppressed)";
    }
    for (int requestId : requestIds) {
        if (_pending.contains(requestId)) {
            _finishFailed(requestId);
        }
    }
}

bool TerrariumTileFetcher::_shouldWarnFailure()
{
    if (_failureWarnTimer.isValid() && (_failureWarnTimer.elapsed() < kFailureWarnIntervalMs)) {
        return false;
    }
    _failureWarnTimer.restart();
    return true;
}

void TerrariumTileFetcher::_deliver(int requestId, const QImage& image)
{
    const PendingRequest pending = _pending.take(requestId);
    emit patchHeightsReady(requestId, sampleGrid(image, pending.subWindow, pending.gridSize));
}

void TerrariumTileFetcher::_finishFailed(int requestId)
{
    _pending.remove(requestId);
    emit patchHeightsFailed(requestId);
}
