/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrariumHeightSource.h"

#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <cmath>
#include <memory>

#include "ElevationMapProvider.h"
#include "QGCCacheTile.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTileFetcherQGC.h"

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
/// convention, clamped at tile edges
double heightAtUV(const QImage& image, double u, double v)
{
    const int w = image.width();
    const int h = image.height();
    const double px = (u * w) - 0.5;
    const double py = (v * h) - 0.5;
    const int x0 = qBound(0, static_cast<int>(std::floor(px)), w - 1);
    const int y0 = qBound(0, static_cast<int>(std::floor(py)), h - 1);
    const int x1 = qMin(x0 + 1, w - 1);
    const int y1 = qMin(y0 + 1, h - 1);
    const double fx = qBound(0.0, px - x0, 1.0);
    const double fy = qBound(0.0, py - y0, 1.0);

    const double north = (heightAtPixel(image, x0, y0) * (1.0 - fx)) + (heightAtPixel(image, x1, y0) * fx);
    const double south = (heightAtPixel(image, x0, y1) * (1.0 - fx)) + (heightAtPixel(image, x1, y1) * fx);
    return (north * (1.0 - fy)) + (south * fy);
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

}  // namespace

TerrariumHeightSource::TerrariumHeightSource(QObject* parent, QNetworkAccessManager* networkManager)
    : HeightSource(parent),
      _networkManager(networkManager ? networkManager : new QNetworkAccessManager(this)),
      _mapId(UrlFactory::getQtMapIdFromProviderType(terrariumProviderType()))
{}

int TerrariumHeightSource::requestPatchHeights(const TileMath::TileKey& key, int gridSize)
{
    const int requestId = _nextRequestId();

    if ((gridSize <= 0) || (_mapId < 0)) {
        _pending.insert(requestId, PendingRequest{});
        _failAsync(requestId);
        return requestId;
    }

    // Deeper than the dataset: fetch the top-zoom ancestor and sample the
    // patch's sub-window of it
    PendingRequest pending;
    pending.gridSize = gridSize;
    if (key.zoom <= kMaxTileZoom) {
        pending.fetchKey = key;
        pending.subWindow = QRectF(0, 0, 1, 1);
    } else {
        const int shift = key.zoom - kMaxTileZoom;
        pending.fetchKey = TileMath::TileKey{key.x >> shift, key.y >> shift, kMaxTileZoom};
        const double scale = 1.0 / (1 << shift);
        pending.subWindow = QRectF((key.x - (pending.fetchKey.x << shift)) * scale,
                                   (key.y - (pending.fetchKey.y << shift)) * scale, scale, scale);
    }
    _pending.insert(requestId, pending);
    _lastFetchKey = pending.fetchKey;

    QList<int>& waiters = _waiters[pending.fetchKey];
    waiters.append(requestId);
    if (waiters.size() > 1) {
        return requestId;  // a fetch for this tile is already in flight; it serves all waiters
    }

    const QString providerType = terrariumProviderType();
    QGCFetchTileTask* const task = QGeoFileTileCacheQGC::createFetchTileTask(providerType, pending.fetchKey.x,
                                                                             pending.fetchKey.y, pending.fetchKey.zoom);
    const TileMath::TileKey fetchKey = pending.fetchKey;
    connect(task, &QGCFetchTileTask::tileFetched, this, [this, fetchKey](QGCCacheTile* tile) {
        const std::unique_ptr<QGCCacheTile> guard(tile);  // caller-owned per task contract
        if (!_waiters.contains(fetchKey)) {
            return;                                       // all waiters cancelled while the lookup was in flight
        }
        if (!tile || tile->img.isEmpty()) {
            _failAll(fetchKey);
            return;
        }
        const QImage image = decodeTile(tile->img);
        if (image.isNull()) {
            _failAll(fetchKey);
            return;
        }
        _deliverAll(fetchKey, image);
    });
    connect(task, &QGCMapTask::error, this, [this, fetchKey](QGCMapTask::TaskType, const QString&) {
        if (!_waiters.contains(fetchKey)) {
            return;
        }
        _fetchFromNetwork(fetchKey);
    });

    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();  // never enqueued: the worker will not clean it up
        _waiters.remove(fetchKey);
        _failAsync(requestId);
    }
    return requestId;
}

void TerrariumHeightSource::cancelRequest(int requestId)
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
    QNetworkReply* const reply = _activeReplies.take(fetchKey);
    if (reply) {
        reply->abort();  // finished handler is a no-op once no waiters remain
    }
}

void TerrariumHeightSource::_failAsync(int requestId)
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

void TerrariumHeightSource::_fetchFromNetwork(const TileMath::TileKey& fetchKey)
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
        if (!_waiters.contains(fetchKey)) {
            return;  // all waiters cancelled (abort also lands here)
        }
        if (reply->error() != QNetworkReply::NoError) {
            _failAll(fetchKey);
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            _failAll(fetchKey);
            return;
        }
        // Never cache bodies that delivery would reject (e.g. HTTP-200 error
        // pages): a cached invalid tile would fail every retry from then on
        const QImage image = decodeTile(data);
        if (image.isNull()) {
            _failAll(fetchKey);
            return;
        }
        // Store back so the shared cache serves this tile from now on
        const QString providerType = terrariumProviderType();
        QGeoFileTileCacheQGC::cacheTile(providerType, fetchKey.x, fetchKey.y, fetchKey.zoom, data,
                                        UrlFactory::getImageFormat(providerType, data));
        _deliverAll(fetchKey, image);
    });
}

void TerrariumHeightSource::_deliverAll(const TileMath::TileKey& fetchKey, const QImage& image)
{
    const QList<int> requestIds = _waiters.take(fetchKey);
    for (int requestId : requestIds) {
        if (_pending.contains(requestId)) {
            _deliver(requestId, image);
        }
    }
}

void TerrariumHeightSource::_failAll(const TileMath::TileKey& fetchKey)
{
    const QList<int> requestIds = _waiters.take(fetchKey);
    for (int requestId : requestIds) {
        if (_pending.contains(requestId)) {
            _finishFailed(requestId);
        }
    }
}

void TerrariumHeightSource::_deliver(int requestId, const QImage& image)
{
    const PendingRequest pending = _pending.take(requestId);
    emit patchHeightsReady(requestId, sampleGrid(image, pending.subWindow, pending.gridSize));
}

void TerrariumHeightSource::_finishFailed(int requestId)
{
    _pending.remove(requestId);
    emit patchHeightsFailed(requestId);
}
