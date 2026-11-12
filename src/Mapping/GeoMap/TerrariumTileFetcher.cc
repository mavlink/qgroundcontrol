/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrariumTileFetcher.h"

#include <memory>
#include <utility>

#include <QtCore/QVarLengthArray>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>

#include "BilinearUV.h"
#include "ElevationMapProvider.h"
#include "HeightField.h"
#include "PatchGeometry.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileReply.h"

QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherLog, "GeoMap.TerrariumTileFetcher")
QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherVerboseLog, "GeoMap.TerrariumTileFetcher.Verbose")

static_assert(TerrariumTileFetcher::kMaxGridSize == PatchGeometry::kMaxGridSize,
              "fetcher grid cap must match the patch mesh cap");
static_assert((TerrariumTileFetcher::kMaxTileZoom == TerrariumElevationProvider::kMaxZoom) &&
                  (TerrariumTileFetcher::kAnchorZoom == TerrariumElevationProvider::kAnchorZoom),
              "offline map sets must download the zooms the fetcher requests");

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

/// kAnchorZoom ancestors of a fetched tile and of its 8 neighbors, own first:
/// a tile on an anchor boundary shares edges with patches the neighboring
/// anchor backs. Empty when the tile is already that coarse.
QVarLengthArray<TileMath::TileKey, 4> anchorKeysFor(const TileMath::TileKey& fetchKey)
{
    QVarLengthArray<TileMath::TileKey, 4> anchors;
    if (fetchKey.zoom <= TerrariumTileFetcher::kAnchorZoom) {
        return anchors;
    }
    const int shift = fetchKey.zoom - TerrariumTileFetcher::kAnchorZoom;
    anchors.append(TileMath::TileKey{fetchKey.x >> shift, fetchKey.y >> shift, TerrariumTileFetcher::kAnchorZoom});
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const TileMath::TileKey neighbor{fetchKey.x + dx, fetchKey.y + dy, fetchKey.zoom};
            if (!TileMath::isValidKey(neighbor)) {
                continue;
            }
            const TileMath::TileKey anchor{neighbor.x >> shift, neighbor.y >> shift, TerrariumTileFetcher::kAnchorZoom};
            if (!anchors.contains(anchor)) {
                anchors.append(anchor);
            }
        }
    }
    return anchors;
}

}  // namespace

TerrariumTileFetcher::TerrariumTileFetcher(QObject* parent, QNetworkAccessManager* networkManager)
    : HeightSource(parent)
    , _networkManager(networkManager ? networkManager : new QNetworkAccessManager(this))
    , _mapId(UrlFactory::getMapIdFromProviderType(terrariumProviderType()))
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

    _startFetch(pending.fetchKey);
    return requestId;
}

bool TerrariumTileFetcher::requestTile(const TileMath::TileKey& key)
{
    if (!_heightField || !TileMath::isValidKey(key)) {
        qCWarning(GeoMapTerrariumTileFetcherLog)
            << "requestTile rejected: key" << key << "heightFieldSet" << (_heightField != nullptr);
        return false;
    }

    const TileMath::TileKey fetchKey = fetchKeyFor(key);
    // Anchors first: their fetches queue ahead, and _insertIntoField holds this tile until they land.
    // Also for a tile that is already here: an anchor that failed did not hold it back, and is fetched again.
    for (const TileMath::TileKey& anchor : anchorKeysFor(fetchKey)) {
        (void) requestTile(anchor);
    }
    if (_heightField->hasTile(fetchKey) || _fieldRequests.contains(fetchKey) || _held.contains(fetchKey)) {
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

    _startFetch(fetchKey);
    return true;
}

bool TerrariumTileFetcher::_fetchInFlight(const TileMath::TileKey& fetchKey) const
{
    return _waiters.contains(fetchKey) || _fieldRequests.contains(fetchKey);
}

void TerrariumTileFetcher::_startFetch(const TileMath::TileKey& fetchKey)
{
    QGCTileReply* const reply = new QGCTileReply(_networkManager, _mapId, fetchKey.x, fetchKey.y, fetchKey.zoom, this);
    // A body that is not a terrarium tile (a corrupt cache entry, an HTTP-200 error page) must be neither delivered
    // nor cached. The validator keeps the image it decoded for the delivery.
    const auto image = std::make_shared<QImage>();
    reply->setValidator([image](const QByteArray& data) {
        *image = decodeTile(data);
        return !image->isNull();
    });
    connect(reply, &QGCTileReply::finished, this, [this, fetchKey, reply, image] {
        reply->deleteLater();
        _fetches.remove(fetchKey);
        if (reply->error() != QGCTileReply::Error::NoError) {
            _failAll(fetchKey, reply->errorString());
            return;
        }
        _deliverAll(fetchKey, *image);
    });

    _fetches.insert(fetchKey, reply);
    reply->start();
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
    QGCTileReply* const reply = _fetches.take(fetchKey);
    if (reply) {
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog) << "cancelRequest: aborting fetch for" << fetchKey;
        reply->abort();  // an aborted reply does not finish
        reply->deleteLater();
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

void TerrariumTileFetcher::_deliverAll(const TileMath::TileKey& fetchKey, const QImage& image)
{
    const bool forField = _fieldRequests.remove(fetchKey) && _heightField;
    if (forField) {
        _insertIntoField(fetchKey, gridFromImage(image));
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
    const bool forField = _fieldRequests.remove(fetchKey);
    // Better fine data with a cliff risk than none at all
    const int released = _releaseHeld(fetchKey);
    if (released > 0) {
        qCDebug(GeoMapTerrariumTileFetcherLog)
            << "anchor" << fetchKey << "failed, released" << released << "held tiles";
    }

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
    if (forField) {
        emit tileFailed(fetchKey);
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

void TerrariumTileFetcher::_insertIntoField(const TileMath::TileKey& fetchKey, ElevationTilePyramid::Grid grid)
{
    QSet<TileMath::TileKey> missingAnchors;
    for (const TileMath::TileKey& anchor : anchorKeysFor(fetchKey)) {
        if (!_heightField->hasTile(anchor) && _fieldRequests.contains(anchor)) {
            missingAnchors.insert(anchor);
        }
    }
    if (!missingAnchors.isEmpty()) {
        qCDebug(GeoMapTerrariumTileFetcherVerboseLog)
            << "holding tile" << fetchKey << "until anchors" << missingAnchors;
        _held.insert(fetchKey, HeldTile{std::move(grid), std::move(missingAnchors)});
        return;
    }
    _heightField->insertTile(fetchKey, std::move(grid));
    (void) _releaseHeld(fetchKey);
}

int TerrariumTileFetcher::_releaseHeld(const TileMath::TileKey& anchor)
{
    QList<std::pair<TileMath::TileKey, ElevationTilePyramid::Grid>> ready;
    for (auto it = _held.begin(); it != _held.end();) {
        if (it->anchors.remove(anchor) && it->anchors.isEmpty()) {
            ready.append({it.key(), std::move(it->grid)});
            it = _held.erase(it);
        } else {
            ++it;
        }
    }
    // Inserted after the walk: regionChanged handlers may re-enter requestTile
    for (auto& [key, grid] : ready) {
        _heightField->insertTile(key, std::move(grid));
    }
    return int(ready.count());
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
