#include "DecodedTileFetcher.h"

#include <memory>
#include <utility>

#include <QtCore/QLoggingCategory>

#include "QGCTileReply.h"

DecodedTileFetcher::DecodedTileFetcher(QNetworkAccessManager* networkManager, int mapId, Decoder decoder,
                                       LoggingCategory category, QObject* parent)
    : QObject(parent)
    , _networkManager(networkManager)
    , _mapId(mapId)
    , _decoder(std::move(decoder))
    , _category(category)
{}

int DecodedTileFetcher::fetch(const TileMath::TileKey& key)
{
    const int fetchId = ++_fetchIdCounter;

    QGCTileReply* const reply = new QGCTileReply(_networkManager, _mapId, key.x, key.y, key.zoom, this);
    // Data that does not decode (a corrupt cache entry, an HTTP-200 error page) must be neither delivered nor
    // cached. The validator keeps the image it decoded for the delivery.
    const auto image = std::make_shared<QImage>();
    reply->setValidator([decoder = _decoder, image](const QByteArray& data) {
        *image = decoder(data);
        return !image->isNull();
    });
    connect(reply, &QGCTileReply::finished, this, [this, fetchId, key, reply, image] {
        reply->deleteLater();
        _replies.remove(fetchId);
        switch (reply->error()) {
            case QGCTileReply::Error::NoError:
                emit fetched(fetchId, key, *image);
                break;
            case QGCTileReply::Error::NoTileError:
                qCDebug(_category) << "the provider has no tile" << key;
                emit noTile(fetchId, key);
                break;
            case QGCTileReply::Error::RequestError:
                // The map id names no provider, which the lookup of the id reported: no warning for every tile
                qCDebug(_category) << "tile" << key << "failed:" << reply->errorString();
                emit failed(fetchId, key);
                break;
            default:
                _warnFailure(key, reply->errorString());
                emit failed(fetchId, key);
                break;
        }
    });

    _replies.insert(fetchId, reply);
    reply->start();
    return fetchId;
}

void DecodedTileFetcher::cancel(int fetchId)
{
    QGCTileReply* const reply = _replies.take(fetchId);
    if (reply) {
        reply->abort();  // an aborted reply does not finish
        reply->deleteLater();
    }
}

void DecodedTileFetcher::_warnFailure(const TileMath::TileKey& key, const QString& reason)
{
    // Fetch failures must be visible without logging configuration (matches the map tile / terrain query
    // convention), but repeats are throttled so an outage doesn't flood the log
    if (!_failureWarnTimer.isValid() || (_failureWarnTimer.elapsed() >= kFailureWarnIntervalMs)) {
        _failureWarnTimer.restart();
        qCWarning(_category) << "tile" << key << "failed:" << reason;
    } else {
        qCDebug(_category) << "tile" << key << "failed:" << reason << "(warning suppressed)";
    }
}
