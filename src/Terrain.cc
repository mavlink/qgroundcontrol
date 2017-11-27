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
#include <QtLocation/private/qgeotilespec_p.h>

QGC_LOGGING_CATEGORY(TerrainLog, "TerrainLog")

QMutex                          ElevationProvider::_tilesMutex;
QHash<QString, TerrainTile>     ElevationProvider::_tiles;
QStringList                     ElevationProvider::_downloadQueue;

ElevationProvider::ElevationProvider(QObject* parent)
    : QObject(parent)
{

}

bool ElevationProvider::queryTerrainDataPoints(const QList<QGeoCoordinate>& coordinates)
{
    if (_state != State::Idle || coordinates.length() == 0) {
        return false;
    }

    QUrlQuery query;
    QString points = "";
    for (const auto& coordinate : coordinates) {
        points += QString::number(coordinate.latitude(), 'f', 10) + ","
                + QString::number(coordinate.longitude(), 'f', 10) + ",";
    }
    points = points.mid(0, points.length() - 1); // remove the last ','

    query.addQueryItem(QStringLiteral("points"), points);
    QUrl url(QStringLiteral("https://api.airmap.com/elevation/stage/srtm1/ele"));
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.get(request);
    if (!networkReply) {
        return false;
    }

    connect(networkReply, &QNetworkReply::finished, this, &ElevationProvider::_requestFinished);

    _state = State::Downloading;
    return true;
}

bool ElevationProvider::queryTerrainData(const QList<QGeoCoordinate>& coordinates)
{
    if (_state != State::Idle || coordinates.length() == 0) {
        return false;
    }

    QList<float> altitudes;
    bool needToDownload = false;
    foreach (QGeoCoordinate coordinate, coordinates) {
        QString tileHash = _getTileHash(coordinate);
        _tilesMutex.lock();
        if (!_tiles.contains(tileHash)) {
            qCDebug(TerrainLog) << "Need to download tile " << tileHash;

            if (!_downloadQueue.contains(tileHash)) {
                // Schedule the fetch task

                QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(UrlFactory::AirmapElevation, QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1), QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1), 1, &_networkManager);
                QGeoTileSpec spec;
                spec.setX(QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1));
                spec.setY(QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1));
                spec.setZoom(1);
                spec.setMapId(UrlFactory::AirmapElevation);
                QGeoTiledMapReplyQGC* reply = new QGeoTiledMapReplyQGC(&_networkManager, request, spec);
                connect(reply, &QGeoTiledMapReplyQGC::finished, this, &ElevationProvider::_fetchedTile);
                connect(reply, &QGeoTiledMapReplyQGC::aborted, this, &ElevationProvider::_fetchedTile);

                _downloadQueue.append(tileHash);
            }
            needToDownload = true;
        } else {
            if (!needToDownload) {
                if (_tiles[tileHash].isIn(coordinate)) {
                    altitudes.push_back(_tiles[tileHash].elevation(coordinate));
                } else {
                    qCDebug(TerrainLog) << "Error: coordinate not in tile region";
                    altitudes.push_back(-1.0);
                }
            }
        }
        _tilesMutex.unlock();
    }

    if (!needToDownload) {
        qCDebug(TerrainLog) << "All altitudes taken from cached data";
        emit terrainData(true, altitudes);
        _coordinates.clear();
        return true;
    }

    _coordinates = coordinates;

    return false;
}

void ElevationProvider::_requestFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    QList<float> altitudes;
    _state = State::Idle;

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
        qCDebug(TerrainLog) << "Received " <<  responseJson;
        emit terrainData(false, altitudes);
        return;
    }

    QByteArray responseBytes = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit terrainData(false, altitudes);
        return;
    }
    QJsonObject rootObject = responseJson.object();
    QString status = rootObject["status"].toString();
    if (status == "success") {
        const QJsonArray& dataArray = rootObject["data"].toArray();
        for (int i = 0; i < dataArray.count(); i++) {
            altitudes.push_back(dataArray[i].toDouble());
        }

        emit terrainData(true, altitudes);
    }
    emit terrainData(false, altitudes);
}

void ElevationProvider::_fetchedTile()
{
    QGeoTiledMapReplyQGC* reply = qobject_cast<QGeoTiledMapReplyQGC*>(QObject::sender());

    if (!reply) {
        qCDebug(TerrainLog) << "Elevation tile fetched but invalid reply data type.";
        return;
    }

    // remove from download queue
    QGeoTileSpec spec = reply->tileSpec();
    QString hash = QGCMapEngine::getTileHash(UrlFactory::AirmapElevation, spec.x(), spec.y(), spec.zoom());
    _tilesMutex.lock();
    if (_downloadQueue.contains(hash)) {
        _downloadQueue.removeOne(hash);
    }
    _tilesMutex.unlock();

    // handle potential errors
    if (reply->error() != QGeoTiledMapReply::NoError) {
        if (reply->error() == QGeoTiledMapReply::CommunicationError) {
            qCDebug(TerrainLog) << "Elevation tile fetching returned communication error. " << reply->errorString();
        } else {
            qCDebug(TerrainLog) << "Elevation tile fetching returned error. " << reply->errorString();
        }
        reply->deleteLater();
        return;
    }
    if (!reply->isFinished()) {
        qCDebug(TerrainLog) << "Error in fetching elevation tile. Not finished. " << reply->errorString();
        reply->deleteLater();
        return;
    }

    // parse received data and insert into hash table
    QByteArray responseBytes = reply->mapImageData();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(TerrainLog) << "Could not parse terrain tile " << parseError.errorString();
        qCDebug(TerrainLog) << responseBytes;
        reply->deleteLater();
        return;
    }

    TerrainTile* terrainTile = new TerrainTile(responseJson);
    if (terrainTile->isValid()) {
        _tilesMutex.lock();
        if (!_tiles.contains(hash)) {
            _tiles.insert(hash), *terrainTile);
        } else {
            delete terrainTile;
        }
        _tilesMutex.unlock();
    } else {
        qCDebug(TerrainLog) << "Received invalid tile";
    }
    reply->deleteLater();

    // now try to query the data again
    queryTerrainData(_coordinates);
}

QString ElevationProvider::_getTileHash(const QGeoCoordinate& coordinate)
{
    QString ret = QGCMapEngine::getTileHash(UrlFactory::AirmapElevation, QGCMapEngine::long2elevationTileX(coordinate.longitude(), 1), QGCMapEngine::lat2elevationTileY(coordinate.latitude(), 1), 1);
    qCDebug(TerrainLog) << "Computing unique tile hash for " << coordinate << ret;

    return ret;
}
