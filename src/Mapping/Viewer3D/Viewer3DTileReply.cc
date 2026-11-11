#include "Viewer3DTileReply.h"

#include <QtCore/QTimer>

#include "QGCLoggingCategory.h"
#include "QGCTileReply.h"

QGC_LOGGING_CATEGORY(Viewer3DTileReplyLog, "Viewer3d.Viewer3DTileReply")

Viewer3DTileReply::Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId,
                                     QNetworkAccessManager* networkManager, QObject* parent)
    : QObject{parent}
    , _networkManager(networkManager)
    , _timeoutTimer(new QTimer(this))
{
    _tile.x = tileX;
    _tile.y = tileY;
    _tile.zoomLevel = zoomLevel;
    _tile.mapId = mapId;

    connect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::_onTimeout);
    _timeoutTimer->start(kTimeoutMs);
    _startFetch();
}

void Viewer3DTileReply::_startFetch()
{
    _tileReply = new QGCTileReply(_networkManager, _tile.mapId, _tile.x, _tile.y, _tile.zoomLevel, this);
    connect(_tileReply, &QGCTileReply::finished, this, &Viewer3DTileReply::_onFetchFinished);
    if (!_tileReply->start()) {
        // Queued: the signals of this object are connected only after its construction
        QMetaObject::invokeMethod(
            this, [this] { _giveUp(QStringLiteral("could not start the request")); }, Qt::QueuedConnection);
    }
}

void Viewer3DTileReply::_onFetchFinished()
{
    const QGCTileReply::Error error = _tileReply->error();
    if (error == QGCTileReply::Error::NoError) {
        _timeoutTimer->stop();
        _tile.data = _tileReply->data();
        _tileReply->deleteLater();
        _tileReply = nullptr;
        emit tileDone(_tile);
    } else if (error == QGCTileReply::Error::NoTileError) {
        _timeoutTimer->stop();
        _tileReply->deleteLater();
        _tileReply = nullptr;
        emit tileEmpty(_tile);
    } else {
        _giveUp(_tileReply->errorString());
    }
}

void Viewer3DTileReply::_onTimeout()
{
    qCDebug(Viewer3DTileReplyLog) << "Timeout for tile x:" << _tile.x << "y:" << _tile.y << "retry:" << _timeoutCounter
                                  << "/" << kMaxRetries;
    if (_timeoutCounter > kMaxRetries) {
        _giveUp(QStringLiteral("timed out"));
        return;
    }

    _timeoutCounter++;
    _tileReply->abort();
    _tileReply->deleteLater();
    _startFetch();
}

void Viewer3DTileReply::_giveUp(const QString& reason)
{
    qCWarning(Viewer3DTileReplyLog) << "Request error for tile x:" << _tile.x << "y:" << _tile.y
                                    << "zoom:" << _tile.zoomLevel << reason;
    _timeoutTimer->stop();
    _tileReply->abort();
    _tileReply->deleteLater();
    _tileReply = nullptr;
    emit tileGiveUp(_tile);
}
