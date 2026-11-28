#include "TerrainTileManager.h"

#include <limits>
#include <memory>
#include <optional>

#include <QtCore/QDateTime>

#include "CopernicusTileFormat.h"
#include "ElevationMapProvider.h"
#include "FlightMapSettings.h"
#include "MapProviderRegistry.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "TerrainTile.h"
#include "TileReply.h"

QGC_LOGGING_CATEGORY(TerrainTileManagerLog, "Terrain.TerrainTileManager")

namespace {
    constexpr int kMaxCarpetGridSize = 10000;
}

Q_GLOBAL_STATIC(TerrainTileManager, _terrainTileManager)

TerrainTileManager *TerrainTileManager::instance()
{
    return _terrainTileManager();
}

TerrainTileManager::TerrainTileManager(QObject* parent)
    : QObject(parent)
{
    qCDebug(TerrainTileManagerLog) << this;

    _tiles.setMaxCost(kMaxCachedTiles);
}

TerrainTileManager::~TerrainTileManager()
{
    qCDebug(TerrainTileManagerLog) << this;
}

bool TerrainTileManager::getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error)
{
    PendingTiles pendingTiles;
    _lookUpAltitudes(coordinates, altitudes, error, pendingTiles);
    return pendingTiles.isEmpty();
}

void TerrainTileManager::_lookUpAltitudes(const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes,
                                          bool& error, PendingTiles& pendingTiles)
{
    error = false;

    const QString elevationProviderName = SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(elevationProviderName);
    if (!provider) {
        qCWarning(TerrainTileManagerLog) << "No elevation provider is named" << elevationProviderName;
        error = true;
        return;
    }

    for (qsizetype i = 0; i < coordinates.size(); i++) {
        const QGeoCoordinate& coordinate = coordinates.at(i);
        const int x = provider->long2tileX(coordinate.longitude(), 1);
        const int y = provider->lat2tileY(coordinate.latitude(), 1);
        const QString tileHash = provider->getTileHash(x, y, 1);
        qCDebug(TerrainTileManagerLog) << "hash:coordinate" << tileHash << coordinate;

        if (const TerrainTile* const tile = _tiles.object(tileHash)) {
            altitudes.push_back(_elevation(*tile, coordinate, error));
        } else if (_isFailedTile(tileHash)) {
            // Tile fetch failed recently; short-circuit to avoid hammering the server with repeated requests
            // (e.g. uninitialized 0,0 coordinates from MAVLink TERRAIN_REQUEST returning HTTP 500).
            error = true;
            altitudes.push_back(qQNaN());
        } else {
            altitudes.push_back(qQNaN());
            pendingTiles[tileHash].append(i);
            _fetchTile({tileHash, provider->mapId(), x, y});
        }
    }
}

double TerrainTileManager::_elevation(const TerrainTile& tile, const QGeoCoordinate& coordinate, bool& error)
{
    const double elevation = tile.elevation(coordinate);
    if (qIsNaN(elevation)) {
        error = true;
        qCWarning(TerrainTileManagerLog) << "Internal Error: missing elevation in tile cache";
    } else {
        qCDebug(TerrainTileManagerLog) << "returning elevation from tile cache" << elevation;
    }
    return elevation;
}

void TerrainTileManager::addCoordinateQuery(TerrainQueryInterface *terrainQueryInterface, const QList<QGeoCoordinate> &coordinates)
{
    qCDebug(TerrainTileManagerLog) << "count" << coordinates.count();

    if (coordinates.isEmpty()) {
        return;
    }

    _addQuery({terrainQueryInterface, TerrainQuery::QueryMode::QueryModeCoordinates, 0, 0, coordinates, false, 0, 0});
}

void TerrainTileManager::addPathQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint)
{
    double distanceBetween;
    double finalDistanceBetween;
    const QList<QGeoCoordinate> coordinates = _pathQueryToCoords(startPoint, endPoint, distanceBetween, finalDistanceBetween);

    _addQuery({terrainQueryInterface, TerrainQuery::QueryMode::QueryModePath, distanceBetween, finalDistanceBetween,
               coordinates, false, 0, 0});
}

void TerrainTileManager::addCarpetQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    if (swCoord.longitude() > neCoord.longitude() || swCoord.latitude() > neCoord.latitude()) {
        qCWarning(TerrainTileManagerLog) << "Invalid carpet bounds: SW must be south-west of NE";
        terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    const int gridSizeLat =
        qCeil((neCoord.latitude() - swCoord.latitude()) / CopernicusTileFormat::kValueSpacingDegrees);
    const int gridSizeLon =
        qCeil((neCoord.longitude() - swCoord.longitude()) / CopernicusTileFormat::kValueSpacingDegrees);

    if (gridSizeLat <= 0 || gridSizeLon <= 0) {
        qCWarning(TerrainTileManagerLog) << "Carpet area too small";
        terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    if (gridSizeLat > kMaxCarpetGridSize || gridSizeLon > kMaxCarpetGridSize) {
        qCWarning(TerrainTileManagerLog) << "Carpet area too large"
                                         << "gridSizeLat:" << gridSizeLat
                                         << "gridSizeLon:" << gridSizeLon
                                         << "maxGridSize:" << kMaxCarpetGridSize;
        terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    QList<QGeoCoordinate> coordinates;
    for (int latIdx = 0; latIdx <= gridSizeLat; latIdx++) {
        const double lat = swCoord.latitude() + (latIdx * CopernicusTileFormat::kValueSpacingDegrees);
        for (int lonIdx = 0; lonIdx <= gridSizeLon; lonIdx++) {
            const double lon = swCoord.longitude() + (lonIdx * CopernicusTileFormat::kValueSpacingDegrees);
            (void) coordinates.append(QGeoCoordinate(lat, lon));
        }
    }

    _addQuery({terrainQueryInterface, TerrainQuery::QueryMode::QueryModeCarpet, 0, 0, coordinates, statsOnly,
               gridSizeLat + 1, gridSizeLon + 1});
}

QList<QGeoCoordinate> TerrainTileManager::_pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double &distanceBetween, double &finalDistanceBetween)
{
    const double totalDistance = QGCGeo::geodesicDistance(fromCoord, toCoord);
    // TODO: get spacing from terrainQueryInterface
    const int numPoints = qMax(2, qCeil(totalDistance / CopernicusTileFormat::kValueSpacingMeters) + 1);

    QList<QGeoCoordinate> coordinates = QGCGeo::interpolatePath(fromCoord, toCoord, numPoints);

    if (coordinates.size() >= 2) {
        distanceBetween = QGCGeo::geodesicDistance(coordinates[0], coordinates[1]);
        finalDistanceBetween = QGCGeo::geodesicDistance(coordinates[coordinates.size() - 2], coordinates.last());
    } else {
        distanceBetween = finalDistanceBetween = totalDistance;
    }

    qCDebug(TerrainTileManagerLog) << "fromCoord:toCoord:distanceBetween:finalDistanceBetween:coordCount"
                                   << fromCoord << toCoord << distanceBetween << finalDistanceBetween << coordinates.count();

    return coordinates;
}

void TerrainTileManager::_addQuery(QueuedRequestInfo_t requestInfo)
{
    _lookUpAltitudes(requestInfo.coordinates, requestInfo.altitudes, requestInfo.error, requestInfo.pendingTiles);
    if (requestInfo.pendingTiles.isEmpty()) {
        _answerQuery(requestInfo);
        return;
    }

    _requestQueue.append(std::move(requestInfo));
    qCDebug(TerrainTileManagerLog) << "query queued, count" << _requestQueue.count();
}

void TerrainTileManager::_answerQuery(const QueuedRequestInfo_t& requestInfo)
{
    TerrainQueryInterface* const terrainQueryInterface = requestInfo.terrainQueryInterface.data();
    if (!terrainQueryInterface) {
        return;
    }

    const QList<double>& altitudes = requestInfo.altitudes;
    switch (requestInfo.queryMode) {
        case TerrainQuery::QueryMode::QueryModeCoordinates:
            if (requestInfo.error) {
                qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
                terrainQueryInterface->signalCoordinateHeights(false, QList<double>());
            } else {
                qCDebug(TerrainTileManagerLog) << "All altitudes taken from cached data";
                terrainQueryInterface->signalCoordinateHeights(requestInfo.coordinates.count() == altitudes.count(),
                                                               altitudes);
            }
            break;
        case TerrainQuery::QueryMode::QueryModePath:
            if (requestInfo.error) {
                qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
                terrainQueryInterface->signalPathHeights(false, requestInfo.distanceBetween,
                                                         requestInfo.finalDistanceBetween, QList<double>());
            } else {
                qCDebug(TerrainTileManagerLog) << "All altitudes taken from cached data";
                terrainQueryInterface->signalPathHeights(requestInfo.coordinates.count() == altitudes.count(),
                                                         requestInfo.distanceBetween, requestInfo.finalDistanceBetween,
                                                         altitudes);
            }
            break;
        case TerrainQuery::QueryMode::QueryModeCarpet:
            if (requestInfo.error) {
                qCWarning(TerrainTileManagerLog) << "signalling carpet failure due to internal error";
                terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
            } else {
                double minHeight, maxHeight;
                QList<QList<double>> carpet;
                _processCarpetResults(altitudes, requestInfo.carpetGridSizeLat, requestInfo.carpetGridSizeLon,
                                      requestInfo.carpetStatsOnly, minHeight, maxHeight, carpet);

                qCDebug(TerrainTileManagerLog)
                    << "carpet altitudes from cached data, min:" << minHeight << "max:" << maxHeight;
                terrainQueryInterface->signalCarpetHeights(true, minHeight, maxHeight, carpet);
            }
            break;
        default:
            break;
    }
}

void TerrainTileManager::_fetchTile(const TileFetch& fetch)
{
    if (_tileReplies.contains(fetch.hash) || _queuedTileHashes.contains(fetch.hash)) {
        return;
    }

    _tileFetchQueue.enqueue(fetch);
    (void) _queuedTileHashes.insert(fetch.hash);
    _startTileFetches();
}

void TerrainTileManager::_startTileFetches()
{
    while ((_tileReplies.size() < kMaxTileFetches) && !_tileFetchQueue.isEmpty()) {
        const TileFetch fetch = _tileFetchQueue.dequeue();
        (void) _queuedTileHashes.remove(fetch.hash);

        TileReply* const reply = new TileReply(fetch.mapId, fetch.x, fetch.y, 1, this);
        // What cannot be read as a terrain tile is neither delivered nor cached, and a tile of the cache that cannot
        // be read is fetched again from the server. The validator keeps the tile it read for the delivery.
        const auto tile = std::make_shared<std::optional<TerrainTile>>();
        reply->setValidator([tile](const QByteArray& data) {
            tile->emplace(data);
            return (*tile)->isValid();
        });
        _tileReplies.insert(fetch.hash, reply);
        (void) connect(reply, &TileReply::finished, this, [this, reply, hash = fetch.hash, tile] {
            _tileReplyFinished(reply, hash, tile->has_value() ? &tile->value() : nullptr);
        });
        reply->start();
    }
}

void TerrainTileManager::_tileReplyFinished(TileReply* reply, const QString& hash, const TerrainTile* tile)
{
    (void) _tileReplies.remove(hash);
    reply->deleteLater();

    // A request ends without an error only with a tile that its validator could read
    if ((reply->error() == TileReply::Error::NoError) && tile && tile->isValid()) {
        qCDebug(TerrainTileManagerLog) << "Received some bytes of terrain data:" << reply->data().size();
        _clearFailedTile(hash);
        // Copies share the elevations. The cache holds one before the queries are answered, as an answer can ask
        // for more.
        (void) _tiles.insert(hash, new TerrainTile(*tile));
        _tileReceived(hash, *tile);
    } else {
        const bool firstFailure = _recordFailedTile(hash);
        if (firstFailure) {
            qCWarning(TerrainTileManagerLog) << "Elevation tile fetching returned error:" << reply->errorString();
        } else {
            qCDebug(TerrainTileManagerLog)
                << "Elevation tile fetching returned error (suppressed):" << reply->errorString();
        }
        _tileFailed(hash);
    }

    _startTileFetches();
}

void TerrainTileManager::_tileReceived(const QString& hash, const TerrainTile& tile)
{
    for (qsizetype i = 0; i < _requestQueue.size();) {
        QueuedRequestInfo_t& requestInfo = _requestQueue[i];
        const auto pending = requestInfo.pendingTiles.constFind(hash);
        if (pending == requestInfo.pendingTiles.cend()) {
            i++;
            continue;
        }
        for (const qsizetype index : pending.value()) {
            requestInfo.altitudes[index] = _elevation(tile, requestInfo.coordinates.at(index), requestInfo.error);
        }
        (void) requestInfo.pendingTiles.erase(pending);
        if (!requestInfo.pendingTiles.isEmpty()) {
            i++;
            continue;
        }

        _answerQuery(_requestQueue.takeAt(i));
    }
}

void TerrainTileManager::_tileFailed(const QString& hash)
{
    for (qsizetype i = 0; i < _requestQueue.size();) {
        if (!_requestQueue.at(i).pendingTiles.contains(hash)) {
            i++;
            continue;
        }

        const QueuedRequestInfo_t requestInfo = _requestQueue.takeAt(i);
        TerrainQueryInterface* const terrainQueryInterface = requestInfo.terrainQueryInterface.data();
        if (!terrainQueryInterface) {
            continue;
        }
        switch (requestInfo.queryMode) {
            case TerrainQuery::QueryMode::QueryModeCoordinates:
                terrainQueryInterface->signalCoordinateHeights(false, QList<double>());
                break;
            case TerrainQuery::QueryMode::QueryModePath:
                terrainQueryInterface->signalPathHeights(false, requestInfo.distanceBetween,
                                                         requestInfo.finalDistanceBetween, QList<double>());
                break;
            case TerrainQuery::QueryMode::QueryModeCarpet:
                terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
                break;
            default:
                break;
        }
    }
}

bool TerrainTileManager::_isFailedTile(const QString &hash)
{
    const auto it = _failedTiles.constFind(hash);
    if (it == _failedTiles.constEnd()) {
        return false;
    }
    if (QDateTime::currentMSecsSinceEpoch() - it.value() < kFailedTileBackoffMs) {
        return true;
    }
    _failedTiles.erase(it);
    return false;
}

bool TerrainTileManager::_recordFailedTile(const QString &hash)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Opportunistic, throttled sweep: failed-tile entries are useless once their backoff
    // window expires, so prune stale ones while recording this one. Throttling to at
    // most once per backoff window keeps this O(n) sweep from running on every failure during
    // a burst of distinct failures.
    if (now - _lastFailedTileSweepMs >= kFailedTileBackoffMs) {
        _lastFailedTileSweepMs = now;
        for (auto it = _failedTiles.begin(); it != _failedTiles.end();) {
            if (now - it.value() >= kFailedTileBackoffMs) {
                it = _failedTiles.erase(it);
            } else {
                ++it;
            }
        }
    }

    const bool firstFailure = !_failedTiles.contains(hash);
    _failedTiles.insert(hash, now);
    return firstFailure;
}

void TerrainTileManager::_clearFailedTile(const QString &hash)
{
    _failedTiles.remove(hash);
}

void TerrainTileManager::_processCarpetResults(const QList<double> &altitudes, int gridSizeLat, int gridSizeLon,
                                               bool statsOnly, double &minHeight, double &maxHeight, QList<QList<double>> &carpet)
{
    minHeight = std::numeric_limits<double>::max();
    maxHeight = std::numeric_limits<double>::lowest();

    int idx = 0;
    for (int latIdx = 0; latIdx < gridSizeLat; latIdx++) {
        QList<double> row;
        for (int lonIdx = 0; lonIdx < gridSizeLon; lonIdx++) {
            const double height = altitudes[idx++];
            minHeight = qMin(minHeight, height);
            maxHeight = qMax(maxHeight, height);
            if (!statsOnly) {
                (void) row.append(height);
            }
        }
        if (!statsOnly) {
            (void) carpet.append(row);
        }
    }
}
