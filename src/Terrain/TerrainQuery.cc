/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQuery.h"
#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGCApplication.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QtLocation/private/qgeotilespec_p.h>

#include <cmath>

QGC_LOGGING_CATEGORY(TerrainQueryLog, "TerrainQueryLog")
QGC_LOGGING_CATEGORY(TerrainQueryVerboseLog, "TerrainQueryVerboseLog")

Q_GLOBAL_STATIC(TerrainAtCoordinateBatchManager, _TerrainAtCoordinateBatchManager)
Q_GLOBAL_STATIC(TerrainTileManager, _terrainTileManager)

TerrainAirMapQuery::TerrainAirMapQuery(QObject* parent)
    : TerrainQueryInterface(parent)
{
    qCDebug(TerrainQueryVerboseLog) << "supportsSsl" << QSslSocket::supportsSsl() << "sslLibraryBuildVersionString" << QSslSocket::sslLibraryBuildVersionString();
}

void TerrainAirMapQuery::requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    if (qgcApp()->runningUnitTests()) {
        emit coordinateHeightsReceived(false, QList<double>());
        return;
    }

    QString points;
    for (const QGeoCoordinate& coord: coordinates) {
            points += QString::number(coord.latitude(), 'f', 10) + ","
                    + QString::number(coord.longitude(), 'f', 10) + ",";
    }
    points = points.mid(0, points.length() - 1); // remove the last ',' from string

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = QueryModeCoordinates;
    _sendQuery(QString() /* path */, query);
}

void TerrainAirMapQuery::requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    if (qgcApp()->runningUnitTests()) {
        emit pathHeightsReceived(false, qQNaN(), qQNaN(), QList<double>());
        return;
    }

    QString points;
    points += QString::number(fromCoord.latitude(), 'f', 10) + ","
            + QString::number(fromCoord.longitude(), 'f', 10) + ",";
    points += QString::number(toCoord.latitude(), 'f', 10) + ","
            + QString::number(toCoord.longitude(), 'f', 10);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = QueryModePath;
    _sendQuery(QStringLiteral("/path"), query);
}

void TerrainAirMapQuery::requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly)
{
    if (qgcApp()->runningUnitTests()) {
        emit carpetHeightsReceived(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    QString points;
    points += QString::number(swCoord.latitude(), 'f', 10) + ","
            + QString::number(swCoord.longitude(), 'f', 10) + ",";
    points += QString::number(neCoord.latitude(), 'f', 10) + ","
            + QString::number(neCoord.longitude(), 'f', 10);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);

    _queryMode = QueryModeCarpet;
    _carpetStatsOnly = statsOnly;

    _sendQuery(QStringLiteral("/carpet"), query);
}

void TerrainAirMapQuery::_sendQuery(const QString& path, const QUrlQuery& urlQuery)
{
    QUrl url(QStringLiteral("https://api.airmap.com/elevation/v1/ele") + path);
    qCDebug(TerrainQueryLog) << "_sendQuery" << url;
    url.setQuery(urlQuery);

    QNetworkRequest request(url);

    QSslConfiguration sslConf = request.sslConfiguration();
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConf);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.get(request);
    if (!networkReply) {
        qCWarning(TerrainQueryLog) << "QNetworkManager::Get did not return QNetworkReply";
        _requestFailed();
        return;
    }
    networkReply->ignoreSslErrors();

    connect(networkReply, &QNetworkReply::finished, this, &TerrainAirMapQuery::_requestFinished);
    connect(networkReply, &QNetworkReply::sslErrors, this, &TerrainAirMapQuery::_sslErrors);
    connect(networkReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &TerrainAirMapQuery::_requestError);
}

void TerrainAirMapQuery::_requestError(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    if (code != QNetworkReply::NoError) {
        qCWarning(TerrainQueryLog) << "_requestError error:url:data" << reply->error() << reply->url() << reply->readAll();
        return;
    }
}

void TerrainAirMapQuery::_sslErrors(const QList<QSslError> &errors)
{
    for (const auto &error : errors) {
        qCWarning(TerrainQueryLog) << "SSL error: " << error.errorString();

        const auto &certificate = error.certificate();
        if (!certificate.isNull()) {
            qCWarning(TerrainQueryLog) << "SSL Certificate problem: " << certificate.toText();
        }
    }
}

void TerrainAirMapQuery::_requestFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(TerrainQueryLog) << "_requestFinished error:url:data" << reply->error() << reply->url() << reply->readAll();
        reply->deleteLater();
        _requestFailed();
        return;
    }

    QByteArray responseBytes = reply->readAll();
    reply->deleteLater();

    // Convert the response to Json
    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(TerrainQueryLog) << "_requestFinished unable to parse json:" << parseError.errorString();
        _requestFailed();
        return;
    }

    // Check airmap reponse status
    QJsonObject rootObject = responseJson.object();
    QString status = rootObject["status"].toString();
    if (status != "success") {
        qCWarning(TerrainQueryLog) << "_requestFinished status != success:" << status;
        _requestFailed();
        return;
    }

    // Send back data
    const QJsonValue& jsonData = rootObject["data"];
    qCDebug(TerrainQueryLog) << "_requestFinished success";
    switch (_queryMode) {
    case QueryModeCoordinates:
        emit _parseCoordinateData(jsonData);
        break;
    case QueryModePath:
        emit _parsePathData(jsonData);
        break;
    case QueryModeCarpet:
        emit _parseCarpetData(jsonData);
        break;
    }
}

void TerrainAirMapQuery::_requestFailed(void)
{
    switch (_queryMode) {
    case QueryModeCoordinates:
        emit coordinateHeightsReceived(false /* success */, QList<double>() /* heights */);
        break;
    case QueryModePath:
        emit pathHeightsReceived(false /* success */, qQNaN() /* latStep */, qQNaN() /* lonStep */, QList<double>() /* heights */);
        break;
    case QueryModeCarpet:
        emit carpetHeightsReceived(false /* success */, qQNaN() /* minHeight */, qQNaN() /* maxHeight */, QList<QList<double>>() /* carpet */);
        break;
    }
}

void TerrainAirMapQuery::_parseCoordinateData(const QJsonValue& coordinateJson)
{
    QList<double> heights;
    const QJsonArray& dataArray = coordinateJson.toArray();
    for (int i = 0; i < dataArray.count(); i++) {
        heights.append(dataArray[i].toDouble());
    }

    emit coordinateHeightsReceived(true /* success */, heights);
}

void TerrainAirMapQuery::_parsePathData(const QJsonValue& pathJson)
{
    QJsonObject jsonObject =    pathJson.toArray()[0].toObject();
    QJsonArray stepArray =      jsonObject["step"].toArray();
    QJsonArray profileArray =   jsonObject["profile"].toArray();

    double latStep = stepArray[0].toDouble();
    double lonStep = stepArray[1].toDouble();

    QList<double> heights;
    for (const QJsonValue& profileValue: profileArray) {
        heights.append(profileValue.toDouble());
    }

    emit pathHeightsReceived(true /* success */, latStep, lonStep, heights);
}

void TerrainAirMapQuery::_parseCarpetData(const QJsonValue& carpetJson)
{
    QJsonObject jsonObject =    carpetJson.toArray()[0].toObject();

    QJsonObject statsObject =   jsonObject["stats"].toObject();
    double      minHeight =     statsObject["min"].toDouble();
    double      maxHeight =     statsObject["min"].toDouble();

    QList<QList<double>> carpet;
    if (!_carpetStatsOnly) {
        QJsonArray carpetArray =   jsonObject["carpet"].toArray();

        for (int i=0; i<carpetArray.count(); i++) {
            QJsonArray rowArray = carpetArray[i].toArray();
            carpet.append(QList<double>());

            for (int j=0; j<rowArray.count(); j++) {
                double height = rowArray[j].toDouble();
                carpet.last().append(height);
            }
        }
    }

    emit carpetHeightsReceived(true /*success*/, minHeight, maxHeight, carpet);
}

TerrainOfflineAirMapQuery::TerrainOfflineAirMapQuery(QObject* parent)
    : TerrainQueryInterface(parent)
{
    qCDebug(TerrainQueryVerboseLog) << "supportsSsl" << QSslSocket::supportsSsl() << "sslLibraryBuildVersionString" << QSslSocket::sslLibraryBuildVersionString();
}

void TerrainOfflineAirMapQuery::requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    if (qgcApp()->runningUnitTests()) {
        emit coordinateHeightsReceived(false, QList<double>());
        return;
    }

    if (coordinates.length() == 0) {
        return;
    }

    _terrainTileManager->addCoordinateQuery(this, coordinates);
}

void TerrainOfflineAirMapQuery::requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    if (qgcApp()->runningUnitTests()) {
        emit pathHeightsReceived(false, qQNaN(), qQNaN(), QList<double>());
        return;
    }

    _terrainTileManager->addPathQuery(this, fromCoord, toCoord);
}

void TerrainOfflineAirMapQuery::requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly)
{
    if (qgcApp()->runningUnitTests()) {
        emit carpetHeightsReceived(false, qQNaN(), qQNaN(), QList<QList<double>>());
        return;
    }

    // TODO
    Q_UNUSED(swCoord);
    Q_UNUSED(neCoord);
    Q_UNUSED(statsOnly);
    qWarning() << "Carpet queries are currently not supported from offline air map data";
}

void TerrainOfflineAirMapQuery::_signalCoordinateHeights(bool success, QList<double> heights)
{
    emit coordinateHeightsReceived(success, heights);
}

void TerrainOfflineAirMapQuery::_signalPathHeights(bool success, double latStep, double lonStep, const QList<double>& heights)
{
    emit pathHeightsReceived(success, latStep, lonStep, heights);
}

void TerrainOfflineAirMapQuery::_signalCarpetHeights(bool success, double minHeight, double maxHeight, const QList<QList<double>>& carpet)
{
    emit carpetHeightsReceived(success, minHeight, maxHeight, carpet);
}

TerrainTileManager::TerrainTileManager(void)
{

}

void TerrainTileManager::addCoordinateQuery(TerrainOfflineAirMapQuery* terrainQueryInterface, const QList<QGeoCoordinate>& coordinates)
{
    qCDebug(TerrainQueryLog) << "TerrainTileManager::addCoordinateQuery count" << coordinates.count();

    if (coordinates.length() > 0) {
        bool error;
        QList<double> altitudes;

        if (!_getAltitudesForCoordinates(coordinates, altitudes, error)) {
            qCDebug(TerrainQueryLog) << "TerrainTileManager::addPathQuery queue count" << _requestQueue.count();
            QueuedRequestInfo_t queuedRequestInfo = { terrainQueryInterface, QueryMode::QueryModeCoordinates, 0, 0, coordinates };
            _requestQueue.append(queuedRequestInfo);
            return;
        }

        if (error) {
            QList<double> noAltitudes;
            qCWarning(TerrainQueryLog) << "addCoordinateQuery: signalling failure due to internal error";
            terrainQueryInterface->_signalCoordinateHeights(false, noAltitudes);
        } else {
            qCDebug(TerrainQueryLog) << "addCoordinateQuery: All altitudes taken from cached data";
            terrainQueryInterface->_signalCoordinateHeights(coordinates.count() == altitudes.count(), altitudes);
        }
    }
}

void TerrainTileManager::addPathQuery(TerrainOfflineAirMapQuery* terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint)
{
    // Convert to individual coordinate queries
    QList<QGeoCoordinate> coordinates;
    double lat = startPoint.latitude();
    double lon = startPoint.longitude();
    double steps = ceil(endPoint.distanceTo(startPoint) / TerrainTile::terrainAltitudeSpacing);
    double latDiff = endPoint.latitude() - lat;
    double lonDiff = endPoint.longitude() - lon;
    for (double i = 0.0; i <= steps; i = i + 1) {
        coordinates.append(QGeoCoordinate(lat + latDiff * i / steps, lon + lonDiff * i / steps));
    }
    // We always have one too many and we always want the last one to be the endpoint
    coordinates.last() = endPoint;
    double latStep = coordinates[1].latitude() - coordinates[0].latitude();
    double lonStep = coordinates[1].longitude() - coordinates[0].longitude();

    qCDebug(TerrainQueryLog) << "TerrainTileManager::addPathQuery start:end:coordCount" << startPoint << endPoint << coordinates.count();

    bool error;
    QList<double> altitudes;
    if (!_getAltitudesForCoordinates(coordinates, altitudes, error)) {
        qCDebug(TerrainQueryLog) << "TerrainTileManager::addPathQuery queue count" << _requestQueue.count();
        QueuedRequestInfo_t queuedRequestInfo = { terrainQueryInterface, QueryMode::QueryModePath, latStep, lonStep, coordinates };
        _requestQueue.append(queuedRequestInfo);
        return;
    }

    if (error) {
        QList<double> noAltitudes;
        qCWarning(TerrainQueryLog) << "addPathQuery: signalling failure due to internal error";
        terrainQueryInterface->_signalPathHeights(false, latStep, lonStep, noAltitudes);
    } else {
        qCDebug(TerrainQueryLog) << "addPathQuery: All altitudes taken from cached data";
        terrainQueryInterface->_signalPathHeights(coordinates.count() == altitudes.count(), latStep, lonStep, altitudes);
    }
}

/// Either returns altitudes from cache or queues database request
///     @param[out] error true: altitude not returned due to error, false: altitudes returned
/// @return true: altitude returned (check error as well), false: database query queued (altitudes not returned)
bool TerrainTileManager::_getAltitudesForCoordinates(const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error)
{
    error = false;

    for (const QGeoCoordinate& coordinate: coordinates) {
        QString tileHash = _getTileHash(coordinate);
        qCDebug(TerrainQueryLog) << "TerrainTileManager::_getAltitudesForCoordinates hash:coordinate" << tileHash << coordinate;

        _tilesMutex.lock();
        if (_tiles.contains(tileHash)) {
            if (_tiles[tileHash].isIn(coordinate)) {
                double elevation = _tiles[tileHash].elevation(coordinate);
                if (qIsNaN(elevation)) {
                    error = true;
                    qCWarning(TerrainQueryLog) << "TerrainTileManager::_getAltitudesForCoordinates Internal Error: negative elevation in tile cache";
                } else {
                    qCDebug(TerrainQueryLog) << "TerrainTileManager::_getAltitudesForCoordinates returning elevation from tile cache" << elevation;
                }
                altitudes.push_back(elevation);
            } else {
                qCWarning(TerrainQueryLog) << "TerrainTileManager::_getAltitudesForCoordinates Internal Error: coordinate not in tile region";
                altitudes.push_back(qQNaN());
                error = true;
            }
        } else {
            if (_state != State::Downloading) {
                QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL("Airmap Elevation", getQGCMapEngine()->urlFactory()->long2tileX("Airmap Elevation",coordinate.longitude(), 1), getQGCMapEngine()->urlFactory()->lat2tileY("Airmap Elevation", coordinate.latitude(), 1), 1, &_networkManager);
                qCDebug(TerrainQueryLog) << "TerrainTileManager::_getAltitudesForCoordinates query from database" << request.url();
                QGeoTileSpec spec;
                spec.setX(getQGCMapEngine()->urlFactory()->long2tileX("Airmap Elevation", coordinate.longitude(), 1));
                spec.setY(getQGCMapEngine()->urlFactory()->lat2tileY("Airmap Elevation", coordinate.latitude(), 1));
                spec.setZoom(1);
                spec.setMapId(getQGCMapEngine()->urlFactory()->getIdFromType("Airmap Elevation"));
                QGeoTiledMapReplyQGC* reply = new QGeoTiledMapReplyQGC(&_networkManager, request, spec);
                connect(reply, &QGeoTiledMapReplyQGC::terrainDone, this, &TerrainTileManager::_terrainDone);
                _state = State::Downloading;
            }
            _tilesMutex.unlock();

            return false;
        }
        _tilesMutex.unlock();
    }

    return true;
}

void TerrainTileManager::_tileFailed(void)
{
    QList<double>    noAltitudes;

    for (const QueuedRequestInfo_t& requestInfo: _requestQueue) {
        if (requestInfo.queryMode == QueryMode::QueryModeCoordinates) {
            requestInfo.terrainQueryInterface->_signalCoordinateHeights(false, noAltitudes);
        } else if (requestInfo.queryMode == QueryMode::QueryModePath) {
            requestInfo.terrainQueryInterface->_signalPathHeights(false, requestInfo.latStep, requestInfo.lonStep, noAltitudes);
        }
    }
    _requestQueue.clear();
}

void TerrainTileManager::_terrainDone(QByteArray responseBytes, QNetworkReply::NetworkError error)
{
    QGeoTiledMapReplyQGC* reply = qobject_cast<QGeoTiledMapReplyQGC*>(QObject::sender());
    _state = State::Idle;

    if (!reply) {
        qCWarning(TerrainQueryLog) << "Elevation tile fetched but invalid reply data type.";
        return;
    }

    // remove from download queue
    QGeoTileSpec spec = reply->tileSpec();
    QString hash = QGCMapEngine::getTileHash("Airmap Elevation", spec.x(), spec.y(), spec.zoom());

    // handle potential errors
    if (error != QNetworkReply::NoError) {
        qCWarning(TerrainQueryLog) << "Elevation tile fetching returned error (" << error << ")";
        _tileFailed();
        reply->deleteLater();
        return;
    }
    if (responseBytes.isEmpty()) {
        qCWarning(TerrainQueryLog) << "Error in fetching elevation tile. Empty response.";
        _tileFailed();
        reply->deleteLater();
        return;
    }

    qCDebug(TerrainQueryLog) << "Received some bytes of terrain data: " << responseBytes.size();

    TerrainTile* terrainTile = new TerrainTile(responseBytes);
    if (terrainTile->isValid()) {
        _tilesMutex.lock();
        if (!_tiles.contains(hash)) {
            _tiles.insert(hash, *terrainTile);
        } else {
            delete terrainTile;
        }
        _tilesMutex.unlock();
    } else {
        delete terrainTile;
        qCWarning(TerrainQueryLog) << "Received invalid tile";
    }
    reply->deleteLater();

    // now try to query the data again
    for (int i = _requestQueue.count() - 1; i >= 0; i--) {
        bool error;
        QList<double> altitudes;
        QueuedRequestInfo_t& requestInfo = _requestQueue[i];

        if (_getAltitudesForCoordinates(requestInfo.coordinates, altitudes, error)) {
            if (requestInfo.queryMode == QueryMode::QueryModeCoordinates) {
                if (error) {
                    QList<double> noAltitudes;
                    qCWarning(TerrainQueryLog) << "_terrainDone(coordinateQuery): signalling failure due to internal error";
                    requestInfo.terrainQueryInterface->_signalCoordinateHeights(false, noAltitudes);
                } else {
                    qCDebug(TerrainQueryLog) << "_terrainDone(coordinateQuery): All altitudes taken from cached data";
                    requestInfo.terrainQueryInterface->_signalCoordinateHeights(requestInfo.coordinates.count() == altitudes.count(), altitudes);
                }
            } else if (requestInfo.queryMode == QueryMode::QueryModePath) {
                if (error) {
                    QList<double> noAltitudes;
                    qCWarning(TerrainQueryLog) << "_terrainDone(coordinateQuery): signalling failure due to internal error";
                    requestInfo.terrainQueryInterface->_signalPathHeights(false, requestInfo.latStep, requestInfo.lonStep, noAltitudes);
                } else {
                    qCDebug(TerrainQueryLog) << "_terrainDone(coordinateQuery): All altitudes taken from cached data";
                    requestInfo.terrainQueryInterface->_signalPathHeights(requestInfo.coordinates.count() == altitudes.count(), requestInfo.latStep, requestInfo.lonStep, altitudes);
                }
            }
            _requestQueue.removeAt(i);
        }
    }
}

QString TerrainTileManager::_getTileHash(const QGeoCoordinate& coordinate)
{
    QString ret = QGCMapEngine::getTileHash(
        "Airmap Elevation",
        getQGCMapEngine()->urlFactory()->long2tileX("Airmap Elevation", coordinate.longitude(), 1),
        getQGCMapEngine()->urlFactory()->lat2tileY("Airmap Elevation", coordinate.latitude(), 1),
        1);
    qCDebug(TerrainQueryVerboseLog) << "Computing unique tile hash for " << coordinate << ret;

    return ret;
}

TerrainAtCoordinateBatchManager::TerrainAtCoordinateBatchManager(void)
{
    _batchTimer.setSingleShot(true);
    _batchTimer.setInterval(_batchTimeout);
    connect(&_batchTimer, &QTimer::timeout, this, &TerrainAtCoordinateBatchManager::_sendNextBatch);
    connect(&_terrainQuery, &TerrainQueryInterface::coordinateHeightsReceived, this, &TerrainAtCoordinateBatchManager::_coordinateHeights);
}

void TerrainAtCoordinateBatchManager::addQuery(TerrainAtCoordinateQuery* terrainAtCoordinateQuery, const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() > 0) {
        connect(terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
        QueuedRequestInfo_t queuedRequestInfo = { terrainAtCoordinateQuery, coordinates };
        _requestQueue.append(queuedRequestInfo);
        if (!_batchTimer.isActive()) {
            _batchTimer.start();
        }
    }
}

void TerrainAtCoordinateBatchManager::_sendNextBatch(void)
{
    qCDebug(TerrainQueryLog) << "TerrainAtCoordinateBatchManager::_sendNextBatch _state:_requestQueue.count:_sentRequests.count" << _stateToString(_state) << _requestQueue.count() << _sentRequests.count();

    if (_state != State::Idle) {
        // Waiting for last download the complete, wait some more
        qCDebug(TerrainQueryLog) << "TerrainAtCoordinateBatchManager::_sendNextBatch waiting for current batch, restarting timer";
        _batchTimer.start();
        return;
    }

    if (_requestQueue.count() == 0) {
        return;
    }

    _sentRequests.clear();

    // Convert coordinates to point strings for json query
    QList<QGeoCoordinate> coords;
    int requestQueueAdded = 0;
    for (const QueuedRequestInfo_t& requestInfo: _requestQueue) {
        SentRequestInfo_t sentRequestInfo = { requestInfo.terrainAtCoordinateQuery, false, requestInfo.coordinates.count() };
        _sentRequests.append(sentRequestInfo);
        coords += requestInfo.coordinates;
        requestQueueAdded++;
        if (coords.count() > 50) {
            break;
        }
    }
    _requestQueue = _requestQueue.mid(requestQueueAdded);
    qCDebug(TerrainQueryLog) << "TerrainAtCoordinateBatchManager::_sendNextBatch requesting next batch _state:_requestQueue.count:_sentRequests.count" << _stateToString(_state) << _requestQueue.count() << _sentRequests.count();

    _state = State::Downloading;
    _terrainQuery.requestCoordinateHeights(coords);
}

void TerrainAtCoordinateBatchManager::_batchFailed(void)
{
    QList<double> noHeights;

    for (const SentRequestInfo_t& sentRequestInfo: _sentRequests) {
        if (!sentRequestInfo.queryObjectDestroyed) {
            disconnect(sentRequestInfo.terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
            sentRequestInfo.terrainAtCoordinateQuery->_signalTerrainData(false, noHeights);
        }
    }
    _sentRequests.clear();
}

void TerrainAtCoordinateBatchManager::_queryObjectDestroyed(QObject* terrainAtCoordinateQuery)
{
    // Remove/Mark deleted objects queries from queues

    qCDebug(TerrainQueryLog) << "_TerrainAtCoordinateQueryDestroyed TerrainAtCoordinateQuery" << terrainAtCoordinateQuery;

    int i = 0;
    while (i < _requestQueue.count()) {
        const QueuedRequestInfo_t& requestInfo = _requestQueue[i];
        if (requestInfo.terrainAtCoordinateQuery == terrainAtCoordinateQuery) {
            qCDebug(TerrainQueryLog) << "Removing deleted provider from _requestQueue index:terrainAtCoordinateQuery" << i << requestInfo.terrainAtCoordinateQuery;
            _requestQueue.removeAt(i);
        } else {
            i++;
        }
    }

    for (int i=0; i<_sentRequests.count(); i++) {
        SentRequestInfo_t& sentRequestInfo = _sentRequests[i];
        if (sentRequestInfo.terrainAtCoordinateQuery == terrainAtCoordinateQuery) {
            qCDebug(TerrainQueryLog) << "Zombieing deleted provider from _sentRequests index:terrainAtCoordinateQuery" << sentRequestInfo.terrainAtCoordinateQuery;
            sentRequestInfo.queryObjectDestroyed = true;
        }
    }
}

QString TerrainAtCoordinateBatchManager::_stateToString(State state)
{
    switch (state) {
    case State::Idle:
        return QStringLiteral("Idle");
    case State::Downloading:
        return QStringLiteral("Downloading");
    }

    return QStringLiteral("State unknown");
}

void TerrainAtCoordinateBatchManager::_coordinateHeights(bool success, QList<double> heights)
{
    _state = State::Idle;

    qCDebug(TerrainQueryLog) << "TerrainAtCoordinateBatchManager::_coordinateHeights signalled success:count" << success << heights.count();

    if (!success) {
        _batchFailed();
        return;
    }

    int currentIndex = 0;
    for (const SentRequestInfo_t& sentRequestInfo: _sentRequests) {
        if (!sentRequestInfo.queryObjectDestroyed) {
            qCDebug(TerrainQueryVerboseLog) << "TerrainAtCoordinateBatchManager::_coordinateHeights returned TerrainCoordinateQuery:count" <<  sentRequestInfo.terrainAtCoordinateQuery << sentRequestInfo.cCoord;
            disconnect(sentRequestInfo.terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
            QList<double> requestAltitudes = heights.mid(currentIndex, sentRequestInfo.cCoord);
            sentRequestInfo.terrainAtCoordinateQuery->_signalTerrainData(true, requestAltitudes);
            currentIndex += sentRequestInfo.cCoord;
        }
    }
    _sentRequests.clear();

    if (_requestQueue.count()) {
        _batchTimer.start();
    }
}

TerrainAtCoordinateQuery::TerrainAtCoordinateQuery(QObject* parent)
    : QObject(parent)
{

}
void TerrainAtCoordinateQuery::requestData(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return;
    }

    _TerrainAtCoordinateBatchManager->addQuery(this, coordinates);
}

void TerrainAtCoordinateQuery::_signalTerrainData(bool success, QList<double>& heights)
{
    emit terrainDataReceived(success, heights);
}

TerrainPathQuery::TerrainPathQuery(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<PathHeightInfo_t>();
    connect(&_terrainQuery, &TerrainQueryInterface::pathHeightsReceived, this, &TerrainPathQuery::_pathHeights);
}

void TerrainPathQuery::requestData(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    _terrainQuery.requestPathHeights(fromCoord, toCoord);
}

void TerrainPathQuery::_pathHeights(bool success, double latStep, double lonStep, const QList<double>& heights)
{
    PathHeightInfo_t pathHeightInfo;
    pathHeightInfo.latStep = latStep;
    pathHeightInfo.lonStep = lonStep;
    pathHeightInfo.heights = heights;
    emit terrainDataReceived(success, pathHeightInfo);
}

TerrainPolyPathQuery::TerrainPolyPathQuery(QObject* parent)
    : QObject   (parent)
    , _curIndex (0)
{
    connect(&_pathQuery, &TerrainPathQuery::terrainDataReceived, this, &TerrainPolyPathQuery::_terrainDataReceived);
}

void TerrainPolyPathQuery::requestData(const QVariantList& polyPath)
{
    QList<QGeoCoordinate> path;

    for (const QVariant& geoVar: polyPath) {
        path.append(geoVar.value<QGeoCoordinate>());
    }

    requestData(path);
}

void TerrainPolyPathQuery::requestData(const QList<QGeoCoordinate>& polyPath)
{
    qCDebug(TerrainQueryLog) << "TerrainPolyPathQuery::requestData count" << polyPath.count();

    // Kick off first request
    _rgCoords = polyPath;
    _curIndex = 0;
    _pathQuery.requestData(_rgCoords[0], _rgCoords[1]);
}

void TerrainPolyPathQuery::_terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo)
{
    qCDebug(TerrainQueryLog) << "TerrainPolyPathQuery::_terrainDataReceived success:_curIndex" << success << _curIndex;

    if (!success) {
        _rgPathHeightInfo.clear();
        emit terrainDataReceived(false /* success */, _rgPathHeightInfo);
        return;
    }

    _rgPathHeightInfo.append(pathHeightInfo);

    if (++_curIndex >= _rgCoords.count() - 1) {
        // We've finished all requests
        qCDebug(TerrainQueryLog) << "TerrainPolyPathQuery::_terrainDataReceived complete";
        emit terrainDataReceived(true /* success */, _rgPathHeightInfo);
    } else {
        _pathQuery.requestData(_rgCoords[_curIndex], _rgCoords[_curIndex+1]);
    }
}

TerrainCarpetQuery::TerrainCarpetQuery(QObject* parent)
    : QObject(parent)
{
    connect(&_terrainQuery, &TerrainQueryInterface::carpetHeightsReceived, this, &TerrainCarpetQuery::terrainDataReceived);
}

void TerrainCarpetQuery::requestData(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool statsOnly)
{
    _terrainQuery.requestCarpetHeights(swCoord, neCoord, statsOnly);
}
