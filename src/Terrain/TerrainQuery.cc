/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQuery.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(TerrainQueryLog, "qgc.terrain.terrainquery")
QGC_LOGGING_CATEGORY(TerrainQueryVerboseLog, "qgc.terrain.terrainquery.verbose")

Q_GLOBAL_STATIC(TerrainAtCoordinateBatchManager, _terrainAtCoordinateBatchManager)

TerrainAtCoordinateBatchManager::TerrainAtCoordinateBatchManager(QObject *parent)
    : QObject(parent)
    , _batchTimer(new QTimer(this))
    , _terrainQuery(new TerrainOfflineQuery(this))
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;

    _batchTimer->setSingleShot(true);
    _batchTimer->setInterval(_batchTimeout);

    (void) connect(_batchTimer, &QTimer::timeout, this, &TerrainAtCoordinateBatchManager::_sendNextBatch);
    (void) connect(_terrainQuery, &TerrainQueryInterface::coordinateHeightsReceived, this, &TerrainAtCoordinateBatchManager::_coordinateHeights);
}

TerrainAtCoordinateBatchManager::~TerrainAtCoordinateBatchManager()
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;
}

TerrainAtCoordinateBatchManager *TerrainAtCoordinateBatchManager::instance()
{
    return _terrainAtCoordinateBatchManager();
}

void TerrainAtCoordinateBatchManager::addQuery(TerrainAtCoordinateQuery *terrainAtCoordinateQuery, const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    (void) connect(terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
    const QueuedRequestInfo_t queuedRequestInfo = {
        terrainAtCoordinateQuery,
        coordinates
    };
    _requestQueue.enqueue(queuedRequestInfo);

    if (!_batchTimer->isActive()) {
        _batchTimer->start();
    }
}

void TerrainAtCoordinateBatchManager::_sendNextBatch()
{
    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "_state:_requestQueue.count:_sentRequests.count" << _stateToString(_state) << _requestQueue.count() << _sentRequests.count();

    if (_state != TerrainQuery::State::Idle) {
        // Waiting for last download the complete, wait some more
        qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "waiting for current batch, restarting timer";
        _batchTimer->start();
        return;
    }

    if (_requestQueue.isEmpty()) {
        return;
    }

    _sentRequests.clear();

    // Convert coordinates to point strings for json query
    QList<QGeoCoordinate> coords;
    int requestQueueAdded = 0;
    while (!_requestQueue.isEmpty()) {
        const QueuedRequestInfo_t requestInfo = _requestQueue.dequeue();
        const SentRequestInfo_t sentRequestInfo = {
            requestInfo.terrainAtCoordinateQuery,
            false,
            requestInfo.coordinates.count()
        };
        (void) _sentRequests.append(sentRequestInfo);
        coords += requestInfo.coordinates;
        requestQueueAdded++;
        if (coords.count() > 50) {
            break;
        }
    }

    (void) _requestQueue.append(_requestQueue.mid(requestQueueAdded));
    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "requesting next batch _state:_requestQueue.count:_sentRequests.count" << _stateToString(_state) << _requestQueue.count() << _sentRequests.count();

    _state = TerrainQuery::State::Downloading;
    _terrainQuery->requestCoordinateHeights(coords);
}

void TerrainAtCoordinateBatchManager::_batchFailed()
{
    const QList<double> noHeights;

    for (const SentRequestInfo_t &sentRequestInfo: _sentRequests) {
        if (!sentRequestInfo.queryObjectDestroyed) {
            (void) disconnect(sentRequestInfo.terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
            sentRequestInfo.terrainAtCoordinateQuery->signalTerrainData(false, noHeights);
        }
    }

    _sentRequests.clear();
}

void TerrainAtCoordinateBatchManager::_queryObjectDestroyed(QObject *terrainAtCoordinateQuery)
{
    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "TerrainAtCoordinateQuery" << terrainAtCoordinateQuery;

    int i = 0;
    while (i < _requestQueue.count()) {
        const QueuedRequestInfo_t &requestInfo = _requestQueue[i];
        if (requestInfo.terrainAtCoordinateQuery == terrainAtCoordinateQuery) {
            qCDebug(TerrainQueryLog) << "Removing deleted provider from _requestQueue index:terrainAtCoordinateQuery" << i << requestInfo.terrainAtCoordinateQuery;
            (void) _requestQueue.removeAt(i);
        } else {
            i++;
        }
    }

    for (SentRequestInfo_t &sentRequestInfo : _sentRequests) {
        if (sentRequestInfo.terrainAtCoordinateQuery == terrainAtCoordinateQuery) {
            qCDebug(TerrainQueryLog) << "Zombieing deleted provider from _sentRequests index:terrainAtCoordinateQuery" << sentRequestInfo.terrainAtCoordinateQuery;
            sentRequestInfo.queryObjectDestroyed = true;
        }
    }
}

QString TerrainAtCoordinateBatchManager::_stateToString(TerrainQuery::State state)
{
    switch (state) {
    case TerrainQuery::State::Idle:
        return QStringLiteral("Idle");
    case TerrainQuery::State::Downloading:
        return QStringLiteral("Downloading");
    default:
        break;
    }

    return QStringLiteral("State unknown");
}

void TerrainAtCoordinateBatchManager::_coordinateHeights(bool success, const QList<double> &heights)
{
    _state = TerrainQuery::State::Idle;

    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "signalled success:count" << success << heights.count();

    if (!success) {
        _batchFailed();
        return;
    }

    int currentIndex = 0;
    for (const SentRequestInfo_t &sentRequestInfo: _sentRequests) {
        if (sentRequestInfo.queryObjectDestroyed) {
            continue;
        }

        qCDebug(TerrainQueryVerboseLog) << Q_FUNC_INFO << "returned TerrainCoordinateQuery:count" << sentRequestInfo.terrainAtCoordinateQuery << sentRequestInfo.cCoord;
        (void) disconnect(sentRequestInfo.terrainAtCoordinateQuery, &TerrainAtCoordinateQuery::destroyed, this, &TerrainAtCoordinateBatchManager::_queryObjectDestroyed);
        const QList<double> requestAltitudes = heights.mid(currentIndex, sentRequestInfo.cCoord);
        sentRequestInfo.terrainAtCoordinateQuery->signalTerrainData(true, requestAltitudes);
        currentIndex += sentRequestInfo.cCoord;
    }
    _sentRequests.clear();

    if (!_requestQueue.isEmpty()) {
        _batchTimer->start();
    }
}

/*===========================================================================*/

TerrainAtCoordinateQuery::TerrainAtCoordinateQuery(bool autoDelete, QObject *parent)
    : QObject(parent)
    , _autoDelete(autoDelete)
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;
}

TerrainAtCoordinateQuery::~TerrainAtCoordinateQuery()
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;
}

void TerrainAtCoordinateQuery::requestData(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    TerrainAtCoordinateBatchManager::instance()->addQuery(this, coordinates);
}

bool TerrainAtCoordinateQuery::getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error)
{
    return TerrainTileManager::instance()->getAltitudesForCoordinates(coordinates, altitudes, error);
}

void TerrainAtCoordinateQuery::signalTerrainData(bool success, const QList<double> &heights)
{
    emit terrainDataReceived(success, heights);
    if (_autoDelete) {
        deleteLater();
    }
}

/*===========================================================================*/

TerrainPathQuery::TerrainPathQuery(bool autoDelete, QObject *parent)
   : QObject(parent)
   , _autoDelete(autoDelete)
   , _terrainQuery(new TerrainOfflineQuery(this))
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;

    (void) connect(_terrainQuery, &TerrainQueryInterface::pathHeightsReceived, this, &TerrainPathQuery::_pathHeights);
}

TerrainPathQuery::~TerrainPathQuery()
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;
}

void TerrainPathQuery::requestData(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    _terrainQuery->requestPathHeights(fromCoord, toCoord);
}

void TerrainPathQuery::_pathHeights(bool success, double distanceBetween, double finalDistanceBetween, const QList<double> &heights)
{
    PathHeightInfo_t pathHeightInfo;
    pathHeightInfo.distanceBetween = distanceBetween;
    pathHeightInfo.finalDistanceBetween = finalDistanceBetween;
    pathHeightInfo.heights = heights;
    emit terrainDataReceived(success, pathHeightInfo);
    if (_autoDelete) {
        deleteLater();
    }
}

/*===========================================================================*/

TerrainPolyPathQuery::TerrainPolyPathQuery(bool autoDelete, QObject *parent)
    : QObject(parent)
    , _autoDelete(autoDelete)
    , _pathQuery(new TerrainPathQuery(false, this))
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;

    (void) connect(_pathQuery, &TerrainPathQuery::terrainDataReceived, this, &TerrainPolyPathQuery::_terrainDataReceived);
}

TerrainPolyPathQuery::~TerrainPolyPathQuery()
{
    // qCDebug(TerrainQueryLog) << Q_FUNC_INFO << this;
}

void TerrainPolyPathQuery::requestData(const QVariantList &polyPath)
{
    QList<QGeoCoordinate> path;

    for (const QVariant &geoVar: polyPath) {
        (void) path.append(geoVar.value<QGeoCoordinate>());
    }

    requestData(path);
}

void TerrainPolyPathQuery::requestData(const QList<QGeoCoordinate> &polyPath)
{
    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "count" << polyPath.count();

    _rgCoords = polyPath;
    _curIndex = 0;
    _pathQuery->requestData(_rgCoords[0], _rgCoords[1]);
}

void TerrainPolyPathQuery::_terrainDataReceived(bool success, const TerrainPathQuery::PathHeightInfo_t &pathHeightInfo)
{
    qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "success:_curIndex" << success << _curIndex;

    if (!success) {
        _rgPathHeightInfo.clear();
        emit terrainDataReceived(false, _rgPathHeightInfo);
        return;
    }

    (void) _rgPathHeightInfo.append(pathHeightInfo);

    if (++_curIndex >= (_rgCoords.count() - 1)) {
        qCDebug(TerrainQueryLog) << Q_FUNC_INFO << "complete";
        emit terrainDataReceived(true, _rgPathHeightInfo);
        if (_autoDelete) {
            deleteLater();
        }
    } else {
        _pathQuery->requestData(_rgCoords[_curIndex], _rgCoords[_curIndex + 1]);
    }
}
