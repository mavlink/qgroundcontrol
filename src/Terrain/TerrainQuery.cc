/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQuery.h"
#include "TerrainTileManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(TerrainQueryLog, "qgc.terrain.terrainquery")
QGC_LOGGING_CATEGORY(TerrainQueryVerboseLog, "qgc.terrain.terrainquery.verbose")

Q_GLOBAL_STATIC(TerrainAtCoordinateBatchManager, s_TerrainAtCoordinateBatchManager)

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

TerrainAtCoordinateQuery::TerrainAtCoordinateQuery(bool autoDelete)
    : _autoDelete(autoDelete)
{

}
void TerrainAtCoordinateQuery::requestData(const QList<QGeoCoordinate>& coordinates)
{
    if (coordinates.length() == 0) {
        return;
    }

    s_TerrainAtCoordinateBatchManager->addQuery(this, coordinates);
}

bool TerrainAtCoordinateQuery::getAltitudesForCoordinates(const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error)
{
    return TerrainTileManager::instance()->getAltitudesForCoordinates(coordinates, altitudes, error);
}

void TerrainAtCoordinateQuery::_signalTerrainData(bool success, QList<double>& heights)
{
    emit terrainDataReceived(success, heights);
    if (_autoDelete) {
        deleteLater();
    }
}

TerrainPathQuery::TerrainPathQuery(bool autoDelete)
   : _autoDelete   (autoDelete)
{
    qRegisterMetaType<PathHeightInfo_t>();
    connect(&_terrainQuery, &TerrainQueryInterface::pathHeightsReceived, this, &TerrainPathQuery::_pathHeights);
}

void TerrainPathQuery::requestData(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    _terrainQuery.requestPathHeights(fromCoord, toCoord);
}

void TerrainPathQuery::_pathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double>& heights)
{
    PathHeightInfo_t pathHeightInfo;
    pathHeightInfo.distanceBetween =        distanceBetween;
    pathHeightInfo.finalDistanceBetween =   finalDistanceBetween;
    pathHeightInfo.heights =                heights;
    emit terrainDataReceived(success, pathHeightInfo);
    if (_autoDelete) {
        deleteLater();
    }
}

TerrainPolyPathQuery::TerrainPolyPathQuery(bool autoDelete)
    : _autoDelete   (autoDelete)
    , _pathQuery    (false /* autoDelete */)
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
        if (_autoDelete) {
            deleteLater();
        }
    } else {
        _pathQuery.requestData(_rgCoords[_curIndex], _rgCoords[_curIndex+1]);
    }
}
