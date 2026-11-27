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

#include "MapEngine.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "TileReply.h"

QGC_LOGGING_CATEGORY(GeoMapTileImageSourceLog, "GeoMap.TileImageSource")
QGC_LOGGING_CATEGORY(GeoMapTileImageSourceVerboseLog, "GeoMap.TileImageSource.Verbose")

TileImageSource::TileImageSource(const QString& mapType, QObject* parent)
    : QObject(parent)
    , _mapType(mapType)
    , _mapId(MapProviderRegistry::getMapIdFromProviderType(mapType))
{
    if (_mapId < 0) {
        qCWarning(GeoMapTileImageSourceLog) << "unknown map type" << _mapType << "- all tile requests will fail";
    }
    connect(MapEngine::instance(), &MapEngine::tileSourceChanged, this, [this](int mapId) {
        if (mapId == _mapId) {
            emit sourceChanged();
        }
    });
}

int TileImageSource::requestTileImage(const TileMath::TileKey& key)
{
    const int requestId = ++_requestIdCounter;
    qCDebug(GeoMapTileImageSourceVerboseLog) << "requestTileImage: key" << key << "requestId" << requestId;

    TileReply* const reply = new TileReply(_mapId, key.x, key.y, key.zoom, this);
    // A body that does not decode (a corrupt cache entry, an HTTP-200 error page) must be neither delivered nor
    // cached. The validator keeps the image it decoded for the delivery.
    const auto image = std::make_shared<QImage>();
    reply->setValidator([image](const QByteArray& data) { return image->loadFromData(data); });
    connect(reply, &TileReply::finished, this, [this, requestId, key, reply, image] {
        reply->deleteLater();
        _pending.remove(requestId);
        if (reply->error() != TileReply::Error::NoError) {
            // Every request of a map type that is unknown fails so: the constructor warned once
            if (reply->error() != TileReply::Error::RequestError) {
                _warnFailure(key, reply->errorString());
            }
            emit tileImageFailed(requestId);
            return;
        }
        emit tileImageReady(requestId, *image);
    });

    _pending.insert(requestId, reply);
    reply->start();
    return requestId;
}

void TileImageSource::cancelRequest(int requestId)
{
    const auto it = _pending.constFind(requestId);
    if (it == _pending.cend()) {
        return;
    }
    TileReply* const reply = it.value();
    _pending.erase(it);
    reply->abort();  // an aborted reply does not finish
    reply->deleteLater();
}

void TileImageSource::_warnFailure(const TileMath::TileKey& key, const QString& reason)
{
    if (_failureWarnings.allow()) {
        qCWarning(GeoMapTileImageSourceLog) << "tile" << key << "failed:" << reason;
    } else {
        qCDebug(GeoMapTileImageSourceLog) << "tile" << key << "failed:" << reason << "(warning suppressed)";
    }
}
