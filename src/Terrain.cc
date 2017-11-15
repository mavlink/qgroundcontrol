/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Terrain.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(TerrainLog, "TerrainLog")

QMutex                          ElevationProvider::_tilesMutex;
QHash<QString, TerrainTile>     ElevationProvider::_tiles;
QStringList                     ElevationProvider::_downloadQueue;

ElevationProvider::ElevationProvider(QObject* parent)
    : QObject(parent)
{

}

bool ElevationProvider::queryTerrainData(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return false;
    }

    bool fallBackToOnline = false;
    QList<float> altitudes;
    foreach (QGeoCoordinate coordinate, coordinates) {
        QString uniqueTileId = _uniqueTileId(coordinate);
        _tilesMutex.lock();
        if (!_tiles.contains(uniqueTileId)) {
            _tilesMutex.unlock();
            fallBackToOnline = true;
            break;
        } else {
            if (_tiles[uniqueTileId].isIn(coordinate)) {
                altitudes.push_back(_tiles[uniqueTileId].elevation(coordinate));
            } else {
                qCDebug(TerrainLog) << "Error: coordinate not in tile region";
                altitudes.push_back(-1.0);
            }
        }
        _tilesMutex.unlock();
    }

    if (!fallBackToOnline) {
        qCDebug(TerrainLog) << "All altitudes taken from cached data";
        emit terrainData(true, altitudes);
        return true;
    }

    if (_state != State::Idle) {
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

bool ElevationProvider::cacheTerrainTiles(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return false;
    }

    for (const auto& coordinate : coordinates) {
        QString uniqueTileId = _uniqueTileId(coordinate);
        _tilesMutex.lock();
        if (_downloadQueue.contains(uniqueTileId) || _tiles.contains(uniqueTileId)) {
            continue;
        }
        _downloadQueue.append(uniqueTileId.replace("-", ","));
        _tilesMutex.unlock();
        qCDebug(TerrainLog) << "Adding tile to download queue: " << uniqueTileId;
    }

    _downloadTiles();
    return true;
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

void ElevationProvider::_requestFinishedTile()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    _state = State::Idle;

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseBytes = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
        qCDebug(TerrainLog) << "ERROR: Received " << responseJson;
        // TODO: Handle error in downloading data
        return;
    }

    QByteArray responseBytes = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        // TODO: Handle error in downloading data
        return;
    }

    TerrainTile* tile = new TerrainTile(responseJson);
    if (tile->isValid()) {
        _tilesMutex.lock();
        if (!_tiles.contains(_uniqueTileId(tile->centerCoordinate()))) {
            _tiles.insert(_uniqueTileId(tile->centerCoordinate()), *tile);
        } else {
            delete tile;
        }
        _tilesMutex.lock();
    }

    _downloadTiles();
}

void ElevationProvider::_downloadTiles(void)
{
    if (_state == State::Idle && _downloadQueue.count() > 0) {
        QUrlQuery query;
        _tilesMutex.lock();
        qCDebug(TerrainLog) << "Starting download for " << _downloadQueue.first();
        query.addQueryItem(QStringLiteral("points"), _downloadQueue.first());
        _downloadQueue.pop_front();
        _tilesMutex.unlock();
        QUrl url(QStringLiteral("https://api.airmap.com/elevation/stage/srtm1/carpet"));
        url.setQuery(query);

        QNetworkRequest request(url);

        QNetworkProxy tProxy;
        tProxy.setType(QNetworkProxy::DefaultProxy);
        _networkManager.setProxy(tProxy);

        QNetworkReply* networkReply = _networkManager.get(request);
        if (!networkReply) {
            return;
        }

        connect(networkReply, &QNetworkReply::finished, this, &ElevationProvider::_requestFinishedTile);

        _state = State::Downloading;
    }
}

QString ElevationProvider::_uniqueTileId(const QGeoCoordinate& coordinate)
{
    QGeoCoordinate southEast;
    southEast.setLatitude(floor(coordinate.latitude()*40.0)/40.0);
    southEast.setLongitude(floor(coordinate.longitude()*40.0)/40.0);
    QGeoCoordinate northEast;
    northEast.setLatitude(ceil(coordinate.latitude()*40.0)/40.0);
    northEast.setLongitude(ceil(coordinate.longitude()*40.0)/40.0);

    QString ret = QString::number(southEast.latitude(), 'f', 6) + "-" + QString::number(southEast.longitude(), 'f', 6) + "-" +
                  QString::number(northEast.latitude(), 'f', 6) + "-" + QString::number(northEast.longitude(), 'f', 6);
    qCDebug(TerrainLog) << "Computing unique tile id for " << coordinate << ret;

    return ret;
}
