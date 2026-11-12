/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TileImageSource.h"

#include <memory>

#include <QtNetwork/QNetworkAccessManager>

#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileReply.h"

QGC_LOGGING_CATEGORY(GeoMapTileImageSourceLog, "GeoMap.TileImageSource")
QGC_LOGGING_CATEGORY(GeoMapTileImageSourceVerboseLog, "GeoMap.TileImageSource.Verbose")

TileImageSource::TileImageSource(const QString& mapType, QObject* parent, QNetworkAccessManager* networkManager)
    : QObject(parent)
    , _mapType(mapType)
    , _mapId(UrlFactory::getMapIdFromProviderType(mapType))
    , _networkManager(networkManager ? networkManager : new QNetworkAccessManager(this))
{
    if (_mapId < 0) {
        qCWarning(GeoMapTileImageSourceLog) << "unknown map type" << _mapType << "- all tile requests will fail";
    }
    connect(getQGCMapEngine(), &QGCMapEngine::tileSourceChanged, this, [this](int mapId) {
        if (mapId == _mapId) {
            emit sourceChanged();
        }
    });
}

int TileImageSource::requestTileImage(const TileMath::TileKey& key)
{
    const int requestId = ++_requestIdCounter;

    if (_mapId < 0) {
        _pending.insert(requestId, nullptr);
        _failAsync(requestId);  // unknown provider: don't spam the cache/URL factory per request
        return requestId;
    }
    qCDebug(GeoMapTileImageSourceVerboseLog) << "requestTileImage: key" << key << "requestId" << requestId;

    QGCTileReply* const reply = new QGCTileReply(_networkManager, _mapId, key.x, key.y, key.zoom, this);
    // A body that does not decode (a corrupt cache entry, an HTTP-200 error page) must be neither delivered nor
    // cached. The validator keeps the image it decoded for the delivery.
    const auto image = std::make_shared<QImage>();
    reply->setValidator([image](const QByteArray& data) { return image->loadFromData(data); });
    connect(reply, &QGCTileReply::finished, this, [this, requestId, key, reply, image] {
        reply->deleteLater();
        _pending.remove(requestId);
        if (reply->error() != QGCTileReply::Error::NoError) {
            _warnFailure(key, reply->errorString());
            emit tileImageFailed(requestId);
            return;
        }
        emit tileImageReady(requestId, *image);
    });

    _pending.insert(requestId, reply);
    reply->start();
    return requestId;
}

void TileImageSource::_failAsync(int requestId)
{
    // Deliver asynchronously so callers see one consistent flow
    QMetaObject::invokeMethod(
        this,
        [this, requestId] {
            if (_pending.remove(requestId)) {
                emit tileImageFailed(requestId);
            }
        },
        Qt::QueuedConnection);
}

void TileImageSource::cancelRequest(int requestId)
{
    const auto it = _pending.constFind(requestId);
    if (it == _pending.cend()) {
        return;
    }
    QGCTileReply* const reply = it.value();
    _pending.erase(it);
    if (reply) {
        reply->abort();  // an aborted reply does not finish
        reply->deleteLater();
    }
}

void TileImageSource::_warnFailure(const TileMath::TileKey& key, const QString& reason)
{
    // Fetch failures must be visible without logging configuration (matches
    // the map tile / terrain query convention), but repeats are throttled so
    // an outage doesn't flood the log
    if (!_failureWarnTimer.isValid() || (_failureWarnTimer.elapsed() >= kFailureWarnIntervalMs)) {
        _failureWarnTimer.restart();
        qCWarning(GeoMapTileImageSourceLog) << "tile" << key << "failed:" << reason;
    } else {
        qCDebug(GeoMapTileImageSourceLog) << "tile" << key << "failed:" << reason << "(warning suppressed)";
    }
}
