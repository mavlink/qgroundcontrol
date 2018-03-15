/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Terrain.h"
#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QtLocation/private/qgeotilespec_p.h>

QGC_LOGGING_CATEGORY(ElevationProviderLog, "ElevationProviderLog")

Q_GLOBAL_STATIC(TerrainBatchManager, _terrainBatchManager)

TerrainBatchManager::TerrainBatchManager(void)
{
}

void TerrainBatchManager::addQuery(ElevationProvider* elevationProvider, const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() > 0) {
        QList<float> altitudes;

        if (!_getAltitudesForCoordinates(coordinates, altitudes)) {
            QueuedRequestInfo_t queuedRequestInfo = { elevationProvider, coordinates };
            _requestQueue.append(queuedRequestInfo);
        }

        qCDebug(ElevationProviderLog) << "All altitudes taken from cached data";
        elevationProvider->_signalTerrainData(coordinates.count() == altitudes.count(), altitudes);
    }
}

bool TerrainBatchManager::_getAltitudesForCoordinates(const QList<QGeoCoordinate>& coordinates, QList<float>& altitudes)
{
    foreach (const QGeoCoordinate& coordinate, coordinates) {
        QString tileHash = _getTileHash(coordinate);
        _tilesMutex.lock();
        if (!_tiles.contains(tileHash)) {
            qCDebug(ElevationProviderLog) << "Need to download tile " << tileHash;

            if (!_tileDownloadQueue.contains(tileHash)) {
                // Schedule the fetch task

                if (_state != State::Downloading) {
                    QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(UrlFactory::AirmapElevation, QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1), QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1), 1, &_networkManager);
                    QGeoTileSpec spec;
                    spec.setX(QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1));
                    spec.setY(QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1));
                    spec.setZoom(1);
                    spec.setMapId(UrlFactory::AirmapElevation);
                    QGeoTiledMapReplyQGC* reply = new QGeoTiledMapReplyQGC(&_networkManager, request, spec);
                    connect(reply, &QGeoTiledMapReplyQGC::finished, this, &TerrainBatchManager::_fetchedTile);
                    connect(reply, &QGeoTiledMapReplyQGC::aborted, this, &TerrainBatchManager::_fetchedTile);
                    _state = State::Downloading;
                }

                _tileDownloadQueue.append(tileHash);
            }
            _tilesMutex.unlock();

            return false;
        } else {
            if (_tiles[tileHash].isIn(coordinate)) {
                altitudes.push_back(_tiles[tileHash].elevation(coordinate));
            } else {
                qCDebug(ElevationProviderLog) << "Error: coordinate not in tile region";
                altitudes.push_back(-1.0);
            }
        }
        _tilesMutex.unlock();
    }
    return true;
}

void TerrainBatchManager::_tileFailed(void)
{
    QList<float>    noAltitudes;

    foreach (const QueuedRequestInfo_t& requestInfo, _requestQueue) {
        requestInfo.elevationProvider->_signalTerrainData(false, noAltitudes);
    }
    _requestQueue.clear();
}

void TerrainBatchManager::_fetchedTile()
{
    QGeoTiledMapReplyQGC* reply = qobject_cast<QGeoTiledMapReplyQGC*>(QObject::sender());

    if (!reply) {
        qCDebug(ElevationProviderLog) << "Elevation tile fetched but invalid reply data type.";
        return;
    }

    // remove from download queue
    QGeoTileSpec spec = reply->tileSpec();
    QString hash = QGCMapEngine::getTileHash(UrlFactory::AirmapElevation, spec.x(), spec.y(), spec.zoom());
    _tilesMutex.lock();
    if (_tileDownloadQueue.contains(hash)) {
        _tileDownloadQueue.removeOne(hash);
    } else {
        qCDebug(ElevationProviderLog) << "Loaded elevation tile, but not found in download queue.";
    }
    _tilesMutex.unlock();

    // handle potential errors
    if (reply->error() != QGeoTiledMapReply::NoError) {
        if (reply->error() == QGeoTiledMapReply::CommunicationError) {
            qCDebug(ElevationProviderLog) << "Elevation tile fetching returned communication error. " << reply->errorString();
        } else {
            qCDebug(ElevationProviderLog) << "Elevation tile fetching returned error. " << reply->errorString();
        }
        _tileFailed();
        reply->deleteLater();
        return;
    }
    if (!reply->isFinished()) {
        qCDebug(ElevationProviderLog) << "Error in fetching elevation tile. Not finished. " << reply->errorString();
        _tileFailed();
        reply->deleteLater();
        return;
    }

    // parse received data and insert into hash table
    QByteArray responseBytes = reply->mapImageData();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(ElevationProviderLog) << "Could not parse terrain tile " << parseError.errorString();
        qCDebug(ElevationProviderLog) << responseBytes;
        _tileFailed();
        reply->deleteLater();
        return;
    }

    TerrainTile* terrainTile = new TerrainTile(responseJson);
    if (terrainTile->isValid()) {
        _tilesMutex.lock();
        if (!_tiles.contains(hash)) {
            _tiles.insert(hash, *terrainTile);
        } else {
            delete terrainTile;
        }
        _tilesMutex.unlock();
    } else {
        qCDebug(ElevationProviderLog) << "Received invalid tile";
    }
    reply->deleteLater();

    // now try to query the data again
    for (int i = _requestQueue.count() - 1; i >= 0; i--) {
        QList<float> altitudes;
        if (_getAltitudesForCoordinates(_requestQueue[i].coordinates, altitudes)) {
            _requestQueue[i].elevationProvider->_signalTerrainData(_requestQueue[i].coordinates.count() == altitudes.count(), altitudes);
            _requestQueue.removeAt(i);
        }
    }
}

QString TerrainBatchManager::_getTileHash(const QGeoCoordinate& coordinate)
{
    QString ret = QGCMapEngine::getTileHash(UrlFactory::AirmapElevation, QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1), QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1), 1);
    qCDebug(ElevationProviderLog) << "Computing unique tile hash for " << coordinate << ret;

    return ret;
}

ElevationProvider::ElevationProvider(QObject* parent)
    : QObject(parent)
{

}

bool ElevationProvider::queryTerrainData(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return false;
    }

    _terrainBatchManager->addQuery(this, coordinates);

    return false;
}

void ElevationProvider::_signalTerrainData(bool success, QList<float>& altitudes)
{
    emit terrainData(success, altitudes);
}
