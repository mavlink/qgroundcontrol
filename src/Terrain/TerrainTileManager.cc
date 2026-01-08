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

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(TerrainTileManagerLog, "Terrain.TerrainTileManager")

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
            coordinates
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
            coordinates
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

QList<QGeoCoordinate> TerrainTileManager::_pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double &distanceBetween, double &finalDistanceBetween)
{
    const double lat = fromCoord.latitude();
    const double lon = fromCoord.longitude();
    const int steps = qCeil(toCoord.distanceTo(fromCoord) / TerrainTileCopernicus::kTileValueSpacingMeters); // TODO: get spacing from terrainQueryInterface
    const double latDiff = toCoord.latitude() - lat;
    const double lonDiff = toCoord.longitude() - lon;

    QList<QGeoCoordinate> coordinates;
    if (steps == 0) {
        (void) coordinates.append(fromCoord);
        (void) coordinates.append(toCoord);
        distanceBetween = finalDistanceBetween = coordinates[0].distanceTo(coordinates[1]);
    } else {
        for (int i = 0; i <= steps; i++) {
            const double latStep = lat + ((latDiff * static_cast<double>(i)) / static_cast<double>(steps));
            const double lonStep = lon + ((lonDiff * static_cast<double>(i)) / static_cast<double>(steps));
            (void) coordinates.append(QGeoCoordinate(latStep, lonStep));
        }

        // We always have one too many and we always want the last one to be the endpoint
        coordinates.last() = toCoord;
        distanceBetween = coordinates[0].distanceTo(coordinates[1]);
        finalDistanceBetween = coordinates[coordinates.count() - 2].distanceTo(coordinates.last());
    }

    qCDebug(TerrainTileManagerLog) << "fromCoord:toCoord:distanceBetween:finalDisanceBetween:coordCount" << fromCoord << toCoord << distanceBetween << finalDistanceBetween << coordinates.count();

    return coordinates;
}

void TerrainTileManager::_tileFailed()
{
    QList<double> noAltitudes;

    for (const QueuedRequestInfo_t &requestInfo: _requestQueue) {
        switch (requestInfo.queryMode) {
        case TerrainQuery::QueryMode::QueryModeCoordinates:
            requestInfo.terrainQueryInterface->signalCoordinateHeights(false, noAltitudes);
            break;
        case TerrainQuery::QueryMode::QueryModePath:
            requestInfo.terrainQueryInterface->signalPathHeights(false, requestInfo.distanceBetween, requestInfo.finalDistanceBetween, noAltitudes);
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
        default:
            break;
        }

        _requestQueue.removeAt(i);
    }
}

void TerrainTileManager::_cacheTile(const QByteArray &data, const QString &hash)
{
    TerrainTile* const terrainTile = new TerrainTile(data);
    if (terrainTile->isValid()) {
        _tilesMutex.lock();
        if (!_tiles.contains(hash)) {
            (void) _tiles.insert(hash, terrainTile);
        } else {
            delete terrainTile;
        }
        _tilesMutex.unlock();
    } else {
        delete terrainTile;
        qCWarning(TerrainTileManagerLog) << "Received invalid tile";
    }
}

TerrainTile *TerrainTileManager::_getCachedTile(const QString &hash)
{
    _tilesMutex.lock();
    if (!_tiles.contains(hash)) {
        _tilesMutex.unlock();
        return nullptr;
    }

    TerrainTile* const tile = _tiles[hash];

    if (!tile->isValid()) {
        _tilesMutex.unlock();
        return nullptr;
    }

    _tilesMutex.unlock();

    return tile;
}
