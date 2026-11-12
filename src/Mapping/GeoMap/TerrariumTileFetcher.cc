/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrariumTileFetcher.h"

#include <utility>

#include <QtCore/QVarLengthArray>
#include <QtGui/QImage>

#include "DecodedTileFetcher.h"
#include "ElevationMapProvider.h"
#include "HeightField.h"
#include "QGCLoggingCategory.h"
#include "UrlFactory.h"

QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherLog, "GeoMap.TerrariumTileFetcher")
QGC_LOGGING_CATEGORY(GeoMapTerrariumTileFetcherVerboseLog, "GeoMap.TerrariumTileFetcher.Verbose")

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
    , _fetcher(new DecodedTileFetcher(networkManager, UrlFactory::getMapIdFromProviderType(terrariumProviderType()),
                                      &decodeTile, &GeoMapTerrariumTileFetcherLog, this))
{
    connect(_fetcher, &DecodedTileFetcher::fetched, this,
            [this](int, const TileMath::TileKey& fetchKey, const QImage& image) { _tileFetched(fetchKey, image); });
    connect(_fetcher, &DecodedTileFetcher::failed, this, [this](int, const TileMath::TileKey& fetchKey) {
        // The field keeps its current estimate, and a later request fetches the tile again
        _tileNotFetched(fetchKey);
        emit tileFailed(fetchKey);
    });
    connect(_fetcher, &DecodedTileFetcher::noTile, this, [this](int, const TileMath::TileKey& fetchKey) {
        if (_missingTiles.count() >= kMaxMissingTiles) {
            _missingTiles.clear();
        }
        _missingTiles.insert(fetchKey);
        _tileNotFetched(fetchKey);
    });
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
    if (_heightField->hasTile(fetchKey) || _fieldRequests.contains(fetchKey) || _held.contains(fetchKey) ||
        _missingTiles.contains(fetchKey)) {
        return true;
    }

    _fieldRequests.insert(fetchKey);
    _lastFetchKey = fetchKey;
    qCDebug(GeoMapTerrariumTileFetcherVerboseLog) << "requestTile: fetchKey" << fetchKey;
    (void) _fetcher->fetch(fetchKey);
    return true;
}

void TerrariumTileFetcher::_tileFetched(const TileMath::TileKey& fetchKey, const QImage& image)
{
    _fieldRequests.remove(fetchKey);
    emit tileFetched(fetchKey);
    if (_heightField) {
        _insertIntoField(fetchKey, gridFromImage(image));
    }
}

void TerrariumTileFetcher::_tileNotFetched(const TileMath::TileKey& fetchKey)
{
    _fieldRequests.remove(fetchKey);
    // Better fine data with a cliff risk than none at all
    const int released = _releaseHeld(fetchKey);
    if (released > 0) {
        qCDebug(GeoMapTerrariumTileFetcherLog)
            << "anchor" << fetchKey << "did not come, released" << released << "held tiles";
    }
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
