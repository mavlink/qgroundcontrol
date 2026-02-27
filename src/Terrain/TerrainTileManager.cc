#include "TerrainTileManager.h"
#include "TerrainTile.h"
#include "TerrainTileCopernicus.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoMapReplyQGC.h"
#include "QGCMapUrlEngine.h"
#include "ElevationMapProvider.h"
#include "SettingsManager.h"
#include "FlightMapSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCGeo.h"

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <limits>

#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(TerrainTileManagerLog, "Terrain.TerrainTileManager")

namespace {
    constexpr int kMaxCarpetGridSize = 10000;
}

Q_GLOBAL_STATIC(TerrainTileManager, _terrainTileManager)

TerrainTileManager *TerrainTileManager::instance()
{
    return _terrainTileManager();
}

TerrainTileManager::TerrainTileManager(QObject *parent)
    : QObject(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    qCDebug(TerrainTileManagerLog) << this;

    QGCNetworkHelper::configureProxy(_networkManager);
}

TerrainTileManager::~TerrainTileManager()
{
    qDeleteAll(_tiles);

    qCDebug(TerrainTileManagerLog) << this;
}

bool TerrainTileManager::getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error)
{
    error = false;

    const QString elevationProviderName = SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(elevationProviderName);
    for (const QGeoCoordinate &coordinate: coordinates) {
        const QString tileHash = UrlFactory::getTileHash(
            provider->getMapName(),
            provider->long2tileX(coordinate.longitude(), 1),
            provider->lat2tileY(coordinate.latitude(), 1),
            1
        );
        qCDebug(TerrainTileManagerLog) << "hash:coordinate" << tileHash << coordinate;

        TerrainTile* const tile = _getCachedTile(tileHash);
        if (tile) {
            const double elevation = tile->elevation(coordinate);
            if (qIsNaN(elevation)) {
                error = true;
                qCWarning(TerrainTileManagerLog) << "Internal Error: missing elevation in tile cache";
            } else {
                qCDebug(TerrainTileManagerLog) << "returning elevation from tile cache" << elevation;
            }
            altitudes.push_back(elevation);
        } else if (_state != TerrainQuery::State::Downloading) {
            QGeoTileSpec spec;
            spec.setX(provider->long2tileX(coordinate.longitude(), 1));
            spec.setY(provider->lat2tileY(coordinate.latitude(), 1));
            spec.setZoom(1);
            spec.setMapId(provider->getMapId());
            const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom());
            QGeoTiledMapReplyQGC *reply = new QGeoTiledMapReplyQGC(_networkManager, request, spec, this);
            (void) connect(reply, &QGeoTiledMapReplyQGC::finished, this, &TerrainTileManager::_terrainDone);
            if (reply->init()) {
                _state = TerrainQuery::State::Downloading;
            } else {
                reply->deleteLater();
            }
            return false;
        } else {
            return false;
        }
    }

    return true;
}

void TerrainTileManager::addCoordinateQuery(TerrainQueryInterface *terrainQueryInterface, const QList<QGeoCoordinate> &coordinates)
{
    qCDebug(TerrainTileManagerLog) << "count" << coordinates.count();

    if (coordinates.isEmpty()) {
        return;
    }

    bool error;
    QList<double> altitudes;
    if (!getAltitudesForCoordinates(coordinates, altitudes, error)) {
        qCDebug(TerrainTileManagerLog) << "queue count" << _requestQueue.count();
        const QueuedRequestInfo_t queuedRequestInfo = {
            terrainQueryInterface,
            TerrainQuery::QueryMode::QueryModeCoordinates,
            0,
            0,
            coordinates,
            false,
            0,
            0
        };
        _requestQueue.enqueue(queuedRequestInfo);
        return;
    }

    if (error) {
        QList<double> noAltitudes;
        qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
        terrainQueryInterface->signalCoordinateHeights(false, noAltitudes);
        return;
    }

    qCDebug(TerrainTileManagerLog) << "all altitudes taken from cached data";
    terrainQueryInterface->signalCoordinateHeights((coordinates.count() == altitudes.count()), altitudes);
}

void TerrainTileManager::addPathQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint)
{
    double distanceBetween;
    double finalDistanceBetween;
    const QList<QGeoCoordinate> coordinates = _pathQueryToCoords(startPoint, endPoint, distanceBetween, finalDistanceBetween);

    bool error;
    QList<double> altitudes;
    if (!getAltitudesForCoordinates(coordinates, altitudes, error)) {
        qCDebug(TerrainTileManagerLog) << "queue count" << _requestQueue.count();
        const QueuedRequestInfo_t queuedRequestInfo = {
            terrainQueryInterface,
            TerrainQuery::QueryMode::QueryModePath,
            distanceBetween,
            finalDistanceBetween,
            coordinates,
            false,
            0,
            0
        };
        _requestQueue.enqueue(queuedRequestInfo);
        return;
    }

    if (error) {
        QList<double> noAltitudes;
        qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
        terrainQueryInterface->signalPathHeights(false, distanceBetween, finalDistanceBetween, noAltitudes);
        return;
    }

    qCDebug(TerrainTileManagerLog) << "all altitudes taken from cached data";
    terrainQueryInterface->signalPathHeights((coordinates.count() == altitudes.count()), distanceBetween, finalDistanceBetween, altitudes);
}

void TerrainTileManager::addCarpetQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
    if (swCoord.longitude() > neCoord.longitude() || swCoord.latitude() > neCoord.latitude()) {
        qCWarning(TerrainTileManagerLog) << "Invalid carpet bounds: SW must be south-west of NE";
        terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    const int gridSizeLat = qCeil((neCoord.latitude() - swCoord.latitude()) / TerrainTileCopernicus::kTileValueSpacingDegrees);
    const int gridSizeLon = qCeil((neCoord.longitude() - swCoord.longitude()) / TerrainTileCopernicus::kTileValueSpacingDegrees);

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
        const double lat = swCoord.latitude() + (latIdx * TerrainTileCopernicus::kTileValueSpacingDegrees);
        for (int lonIdx = 0; lonIdx <= gridSizeLon; lonIdx++) {
            const double lon = swCoord.longitude() + (lonIdx * TerrainTileCopernicus::kTileValueSpacingDegrees);
            (void) coordinates.append(QGeoCoordinate(lat, lon));
        }
    }

    bool error;
    QList<double> altitudes;
    if (!getAltitudesForCoordinates(coordinates, altitudes, error)) {
        qCDebug(TerrainTileManagerLog) << "carpet query queued, count" << _requestQueue.count();
        const QueuedRequestInfo_t queuedRequestInfo = {
            terrainQueryInterface,
            TerrainQuery::QueryMode::QueryModeCarpet,
            0,
            0,
            coordinates,
            statsOnly,
            gridSizeLat + 1,
            gridSizeLon + 1
        };
        _requestQueue.enqueue(queuedRequestInfo);
        return;
    }

    if (error) {
        qCWarning(TerrainTileManagerLog) << "signalling carpet failure due to internal error";
        terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    double minHeight, maxHeight;
    QList<QList<double>> carpet;
    _processCarpetResults(altitudes, gridSizeLat + 1, gridSizeLon + 1, statsOnly, minHeight, maxHeight, carpet);

    qCDebug(TerrainTileManagerLog) << "carpet altitudes from cached data, min:" << minHeight << "max:" << maxHeight;
    terrainQueryInterface->signalCarpetHeights(true, minHeight, maxHeight, carpet);
}

QList<QGeoCoordinate> TerrainTileManager::_pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double &distanceBetween, double &finalDistanceBetween)
{
    const double totalDistance = QGCGeo::geodesicDistance(fromCoord, toCoord);
    // TODO: get spacing from terrainQueryInterface
    const int numPoints = qMax(2, qCeil(totalDistance / TerrainTileCopernicus::kTileValueSpacingMeters) + 1);

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

void TerrainTileManager::_tileFailed()
{
    QList<double> noAltitudes;

    for (const QueuedRequestInfo_t &requestInfo: _requestQueue) {
        if (requestInfo.terrainQueryInterface.isNull()) {
            continue;
        }
        switch (requestInfo.queryMode) {
        case TerrainQuery::QueryMode::QueryModeCoordinates:
            requestInfo.terrainQueryInterface->signalCoordinateHeights(false, noAltitudes);
            break;
        case TerrainQuery::QueryMode::QueryModePath:
            requestInfo.terrainQueryInterface->signalPathHeights(false, requestInfo.distanceBetween, requestInfo.finalDistanceBetween, noAltitudes);
            break;
        case TerrainQuery::QueryMode::QueryModeCarpet:
            requestInfo.terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
            break;
        default:
            continue;
        }
    }

    _requestQueue.clear();
}

void TerrainTileManager::_terrainDone()
{
    _state = TerrainQuery::State::Idle;

    QGeoTiledMapReplyQGC* const reply = qobject_cast<QGeoTiledMapReplyQGC*>(QObject::sender());
    if (!reply) {
        qCWarning(TerrainTileManagerLog) << "Elevation tile fetched but invalid reply data type.";
        return;
    }
    reply->deleteLater();

    const QByteArray responseBytes = reply->mapImageData();
    const QGeoTileSpec spec = reply->tileSpec();

    if (reply->error() != QGeoTiledMapReplyQGC::NoError) {
        qCWarning(TerrainTileManagerLog) << "Elevation tile fetching returned error:" << reply->errorString();
        _tileFailed();
        return;
    }

    if (responseBytes.isEmpty()) {
        qCWarning(TerrainTileManagerLog) << "Error in fetching elevation tile. Empty response.";
        _tileFailed();
        return;
    }

    qCDebug(TerrainTileManagerLog) << "Received some bytes of terrain data:" << responseBytes.size();

    const QString hash = UrlFactory::getTileHash(UrlFactory::getProviderTypeFromQtMapId(spec.mapId()), spec.x(), spec.y(), spec.zoom());
    _cacheTile(responseBytes, hash);

    for (qsizetype i = _requestQueue.count() - 1; i >= 0; i--) {
        bool error;
        QList<double> altitudes;
        QueuedRequestInfo_t &requestInfo = _requestQueue[i];

        if (requestInfo.terrainQueryInterface.isNull()) {
            _requestQueue.removeAt(i);
            continue;
        }

        if (!getAltitudesForCoordinates(requestInfo.coordinates, altitudes, error)) {
            continue;
        }

        switch (requestInfo.queryMode) {
        case TerrainQuery::QueryMode::QueryModeCoordinates:
            if (error) {
                qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
                QList<double> noAltitudes;
                requestInfo.terrainQueryInterface->signalCoordinateHeights(false, noAltitudes);
            } else {
                qCDebug(TerrainTileManagerLog) << "All altitudes taken from cached data";
                requestInfo.terrainQueryInterface->signalCoordinateHeights(requestInfo.coordinates.count() == altitudes.count(), altitudes);
            }
            break;
        case TerrainQuery::QueryMode::QueryModePath:
            if (error) {
                qCWarning(TerrainTileManagerLog) << "signalling failure due to internal error";
                QList<double> noAltitudes;
                requestInfo.terrainQueryInterface->signalPathHeights(false, requestInfo.distanceBetween, requestInfo.finalDistanceBetween, noAltitudes);
            } else {
                qCDebug(TerrainTileManagerLog) << "All altitudes taken from cached data";
                requestInfo.terrainQueryInterface->signalPathHeights(requestInfo.coordinates.count() == altitudes.count(), requestInfo.distanceBetween, requestInfo.finalDistanceBetween, altitudes);
            }
            break;
        case TerrainQuery::QueryMode::QueryModeCarpet:
            if (error) {
                qCWarning(TerrainTileManagerLog) << "signalling carpet failure due to internal error";
                requestInfo.terrainQueryInterface->signalCarpetHeights(false, qQNaN(), qQNaN(), QList<QList<double>>());
            } else {
                double minHeight, maxHeight;
                QList<QList<double>> carpet;
                _processCarpetResults(altitudes, requestInfo.carpetGridSizeLat, requestInfo.carpetGridSizeLon,
                                      requestInfo.carpetStatsOnly, minHeight, maxHeight, carpet);

                qCDebug(TerrainTileManagerLog) << "carpet altitudes from cached data, min:" << minHeight << "max:" << maxHeight;
                requestInfo.terrainQueryInterface->signalCarpetHeights(true, minHeight, maxHeight, carpet);
            }
            break;
        default:
            break;
        }

        _requestQueue.removeAt(i);
    }
}

void TerrainTileManager::_cacheTile(const QByteArray &data, const QString &hash)
{
    TerrainTile* const terrainTile = new TerrainTile(data);
    if (!terrainTile->isValid()) {
        delete terrainTile;
        qCWarning(TerrainTileManagerLog) << "Received invalid tile";
        return;
    }

    QMutexLocker locker(&_tilesMutex);
    if (!_tiles.contains(hash)) {
        (void) _tiles.insert(hash, terrainTile);
    } else {
        delete terrainTile;
    }
}

TerrainTile *TerrainTileManager::_getCachedTile(const QString &hash)
{
    QMutexLocker locker(&_tilesMutex);

    if (!_tiles.contains(hash)) {
        return nullptr;
    }

    TerrainTile* const tile = _tiles[hash];
    if (!tile->isValid()) {
        return nullptr;
    }

    return tile;
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
