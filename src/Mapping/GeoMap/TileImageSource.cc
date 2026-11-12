/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TileImageSource.h"

#include <QtNetwork/QNetworkAccessManager>

#include "DecodedTileFetcher.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"

QGC_LOGGING_CATEGORY(GeoMapTileImageSourceLog, "GeoMap.TileImageSource")
QGC_LOGGING_CATEGORY(GeoMapTileImageSourceVerboseLog, "GeoMap.TileImageSource.Verbose")

TileImageSource::TileImageSource(const QString& mapType, QObject* parent, QNetworkAccessManager* networkManager)
    : QObject(parent)
    , _mapType(mapType)
    , _mapId(UrlFactory::getMapIdFromProviderType(mapType))
    , _fetcher(new DecodedTileFetcher(
          networkManager ? networkManager : new QNetworkAccessManager(this), _mapId,
          [](const QByteArray& data) { return QImage::fromData(data); }, &GeoMapTileImageSourceLog, this))
{
    if (_mapId < 0) {
        qCWarning(GeoMapTileImageSourceLog) << "unknown map type" << _mapType << "- all tile requests will fail";
    }
    connect(_fetcher, &DecodedTileFetcher::fetched, this,
            [this](int requestId, const TileMath::TileKey&, const QImage& image) {
                emit tileImageReady(requestId, image);
            });
    connect(_fetcher, &DecodedTileFetcher::failed, this, &TileImageSource::tileImageFailed);
    connect(_fetcher, &DecodedTileFetcher::noTile, this, &TileImageSource::tileImageMissing);
    connect(getQGCMapEngine(), &QGCMapEngine::tileSourceChanged, this, [this](int mapId) {
        if (mapId == _mapId) {
            emit sourceChanged();
        }
    });
}

int TileImageSource::requestTileImage(const TileMath::TileKey& key)
{
    const int requestId = _fetcher->fetch(key);
    qCDebug(GeoMapTileImageSourceVerboseLog) << "requestTileImage: key" << key << "requestId" << requestId;
    return requestId;
}

void TileImageSource::cancelRequest(int requestId)
{
    _fetcher->cancel(requestId);
}

int TileImageSource::pendingCount() const
{
    return _fetcher->count();
}
