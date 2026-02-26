#include "Viewer3DTileReply.h"

#include "MapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTileFetcherQGC.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <mutex>

QGC_LOGGING_CATEGORY(Viewer3DTileReplyLog, "Viewer3d.Viewer3DTileReply")

QByteArray Viewer3DTileReply::_bingNoTileImage;
static std::once_flag s_bingNoTileInitFlag;

Viewer3DTileReply::Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, const QString &mapType, QNetworkAccessManager *networkManager, QObject *parent)
    : QObject{parent}
    , _networkManager(networkManager)
{
    std::call_once(s_bingNoTileInitFlag, []() {
        QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
        if (file.open(QFile::ReadOnly)) {
            _bingNoTileImage = file.readAll();
        } else {
            qWarning() << "Error opening file" << file.fileName();
        }
    });

    _timeoutTimer = new QTimer(this);

    _tile.x = tileX;
    _tile.y = tileY;
    _tile.zoomLevel = zoomLevel;
    _tile.mapId = mapId;

    QGCFetchTileTask *task = QGeoFileTileCacheQGC::createFetchTileTask(mapType, tileX, tileY, zoomLevel);
    connect(task, &QGCFetchTileTask::tileFetched, this, &Viewer3DTileReply::_onCacheHit);
    connect(task, &QGCMapTask::error, this, [this](QGCMapTask::TaskType, const QString &) { _onCacheMiss(); });
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        _onCacheMiss();
    }
}

Viewer3DTileReply::~Viewer3DTileReply()
{
    if (_reply) {
        _disconnectReply();
        _reply->abort();
        delete _reply;
        _reply = nullptr;
    }
}

void Viewer3DTileReply::_prepareDownload()
{
    const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(_tile.mapId, _tile.x, _tile.y, _tile.zoomLevel);
    _reply = _networkManager->get(request);
    connect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::_onRequestFinished);
    connect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::_onRequestError);
}

void Viewer3DTileReply::_onCacheHit(QGCCacheTile *tile)
{
    if (!tile) {
        _onCacheMiss();
        return;
    }

    _tile.data = tile->img;
    delete tile;

    if (_isBingEmptyTile()) {
        _tile.data.clear();
        emit tileEmpty(_tile);
        return;
    }

    emit tileDone(_tile);
}

void Viewer3DTileReply::_onCacheMiss()
{
    _prepareDownload();
    _timeoutTimer->start(kTimeoutMs);
    connect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::_onTimeout);
}

void Viewer3DTileReply::_onRequestFinished()
{
    _tile.data = _reply->readAll();
    _timeoutTimer->stop();
    _disconnectReply();
    disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::_onTimeout);

    if (_isBingEmptyTile()) {
        _tile.data.clear();
        emit tileEmpty(_tile);
        return;
    }

    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(_tile.mapId);
    if (mapProvider && !_tile.data.isEmpty()) {
        const QString format = mapProvider->getImageFormat(_tile.data);
        QGeoFileTileCacheQGC::cacheTile(mapProvider->getMapName(), _tile.x, _tile.y, _tile.zoomLevel, _tile.data, format);
    }

    emit tileDone(_tile);
}

void Viewer3DTileReply::_onRequestError()
{
    qCWarning(Viewer3DTileReplyLog) << "Request error for tile x:" << _tile.x << "y:" << _tile.y << "zoom:" << _tile.zoomLevel;
    _timeoutTimer->stop();
    disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::_onTimeout);
    _disconnectReply();
    emit tileGiveUp(_tile);
}

void Viewer3DTileReply::_onTimeout()
{
    qCDebug(Viewer3DTileReplyLog) << "Timeout for tile x:" << _tile.x << "y:" << _tile.y << "retry:" << _timeoutCounter << "/" << kMaxRetries;
    if (_timeoutCounter > kMaxRetries) {
        _disconnectReply();
        disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::_onTimeout);
        emit tileGiveUp(_tile);
        _timeoutTimer->stop();
    } else if (_tile.data.isEmpty()) {
        if (_reply) {
            _disconnectReply();
            _reply->abort();
            delete _reply;
            _reply = nullptr;
        }
        _prepareDownload();
        _timeoutCounter++;
    }
}

void Viewer3DTileReply::_disconnectReply()
{
    disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::_onRequestFinished);
    disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::_onRequestError);
}

bool Viewer3DTileReply::_isBingEmptyTile() const
{
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(_tile.mapId);
    return mapProvider && mapProvider->isBingProvider() && !_tile.data.isEmpty() && _tile.data == _bingNoTileImage;
}
