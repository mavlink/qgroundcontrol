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
#include <QTimer>

QGC_LOGGING_CATEGORY(ElevationProviderLog, "ElevationProviderLog")

Q_GLOBAL_STATIC(TerrainBatchManager, _terrainBatchManager)

TerrainBatchManager::TerrainBatchManager(void)
{
    _batchTimer.setSingleShot(true);
    _batchTimer.setInterval(_batchTimeout);
    connect(&_batchTimer, &QTimer::timeout, this, &TerrainBatchManager::_sendNextBatch);
}

void TerrainBatchManager::addQuery(ElevationProvider* elevationProvider, const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() > 0) {
        qCDebug(ElevationProviderLog) << "addQuery: elevationProvider:coordinates.count" << elevationProvider << coordinates.count();
        connect(elevationProvider, &ElevationProvider::destroyed, this, &TerrainBatchManager::_elevationProviderDestroyed);
        QueuedRequestInfo_t queuedRequestInfo = { elevationProvider, coordinates };
        _requestQueue.append(queuedRequestInfo);
        if (!_batchTimer.isActive()) {
            _batchTimer.start();
        }
    }
}

void TerrainBatchManager::_sendNextBatch(void)
{
    qCDebug(ElevationProviderLog) << "_sendNextBatch _state:_requestQueue.count:_sentRequests.count" << _stateToString(_state) << _requestQueue.count() << _sentRequests.count();

    if (_state != State::Idle) {
        // Waiting for last download the complete, wait some more
        _batchTimer.start();
        return;
    }

    if (_requestQueue.count() == 0) {
        return;
    }

    _sentRequests.clear();

    // Convert coordinates to point strings for json query
    QString points;
    foreach (const QueuedRequestInfo_t& requestInfo, _requestQueue) {
        SentRequestInfo_t sentRequestInfo = { requestInfo.elevationProvider, false, requestInfo.coordinates.count() };
        qCDebug(ElevationProviderLog) << "Building request: coordinate count" << requestInfo.coordinates.count();
        _sentRequests.append(sentRequestInfo);

        foreach (const QGeoCoordinate& coord, requestInfo.coordinates) {
            points += QString::number(coord.latitude(), 'f', 10) + ","
                    + QString::number(coord.longitude(), 'f', 10) + ",";
        }

    }
    points = points.mid(0, points.length() - 1); // remove the last ',' from string
    _requestQueue.clear();

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("points"), points);
    QUrl url(QStringLiteral("https://api.airmap.com/elevation/stage/srtm1/ele"));
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager.setProxy(tProxy);

    QNetworkReply* networkReply = _networkManager.get(request);
    if (!networkReply) {
        _batchFailed();
        return;
    }

    connect(networkReply, &QNetworkReply::finished, this, &TerrainBatchManager::_requestFinished);

    _state = State::Downloading;
}

void TerrainBatchManager::_batchFailed(void)
{
    QList<float>    noAltitudes;

    foreach (const SentRequestInfo_t& sentRequestInfo, _sentRequests) {
        if (!sentRequestInfo.providerDestroyed) {
            disconnect(sentRequestInfo.elevationProvider, &ElevationProvider::destroyed, this, &TerrainBatchManager::_elevationProviderDestroyed);
            sentRequestInfo.elevationProvider->_signalTerrainData(false, noAltitudes);
        }
    }
    _sentRequests.clear();
}

void TerrainBatchManager::_requestFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

    _state = State::Idle;

    // When an error occurs we still end up here
    if (reply->error() != QNetworkReply::NoError) {
        qCDebug(ElevationProviderLog) << "_requestFinished error:" << reply->error();
        _batchFailed();
        reply->deleteLater();
        return;
    }

    QByteArray responseBytes = reply->readAll();

    QJsonParseError parseError;
    QJsonDocument responseJson = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(ElevationProviderLog) << "_requestFinished unable to parse json:" << parseError.errorString();
        _batchFailed();
        reply->deleteLater();
        return;
    }

    QJsonObject rootObject = responseJson.object();
    QString status = rootObject["status"].toString();
    if (status != "success") {
        qCDebug(ElevationProviderLog) << "_requestFinished status != success:" << status;
        _batchFailed();
        reply->deleteLater();
        return;
    }

    QList<float> altitudes;
    const QJsonArray& dataArray = rootObject["data"].toArray();
    for (int i = 0; i < dataArray.count(); i++) {
        altitudes.push_back(dataArray[i].toDouble());
    }

    int currentIndex = 0;
    foreach (const SentRequestInfo_t& sentRequestInfo, _sentRequests) {
        if (!sentRequestInfo.providerDestroyed) {
            disconnect(sentRequestInfo.elevationProvider, &ElevationProvider::destroyed, this, &TerrainBatchManager::_elevationProviderDestroyed);
            QList<float> requestAltitudes = altitudes.mid(currentIndex, sentRequestInfo.cCoord);
            sentRequestInfo.elevationProvider->_signalTerrainData(true, requestAltitudes);
            currentIndex += sentRequestInfo.cCoord;
        }
    }
    _sentRequests.clear();

    reply->deleteLater();
}

void TerrainBatchManager::_elevationProviderDestroyed(QObject* elevationProvider)
{
    // Remove/Mark deleted objects queries from queues

    qCDebug(ElevationProviderLog) << "_elevationProviderDestroyed elevationProvider" << elevationProvider;

    int i = 0;
    while (i < _requestQueue.count()) {
        const QueuedRequestInfo_t& requestInfo = _requestQueue[i];
        if (requestInfo.elevationProvider == elevationProvider) {
            qCDebug(ElevationProviderLog) << "Removing deleted provider from _requestQueue index:elevationProvider" << i << requestInfo.elevationProvider;
            _requestQueue.removeAt(i);
        } else {
            i++;
        }
    }

    for (int i=0; i<_sentRequests.count(); i++) {
        SentRequestInfo_t& sentRequestInfo = _sentRequests[i];
        if (sentRequestInfo.elevationProvider == elevationProvider) {
            qCDebug(ElevationProviderLog) << "Zombieing deleted provider from _sentRequests index:elevatationProvider" << sentRequestInfo.elevationProvider;
            sentRequestInfo.providerDestroyed = true;
        }
    }
}

QString TerrainBatchManager::_stateToString(State state)
{
    switch (state) {
    case State::Idle:
        return QStringLiteral("Idle");
    case State::Downloading:
        return QStringLiteral("Downloading");
    }

    return QStringLiteral("State unknown");
}

ElevationProvider::ElevationProvider(QObject* parent)
    : QObject(parent)
{

}
void ElevationProvider::queryTerrainData(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return;
    }

    _terrainBatchManager->addQuery(this, coordinates);
}

void ElevationProvider::_signalTerrainData(bool success, QList<float>& altitudes)
{
    emit terrainData(success, altitudes);
}
