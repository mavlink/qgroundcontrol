#pragma once

#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "TileMath.h"

class QGCTileReply;
class QLoggingCategory;
class QNetworkAccessManager;

/// Fetches the tiles of one map provider as images, for the two tile sources of GeoMap: TileImageSource (imagery)
/// and TerrariumTileFetcher (elevation). A fetch is a QGCTileReply, so the tile comes from the shared tile cache or,
/// on a miss, from the provider's server, and only a tile that the decoder of the source accepts is delivered or
/// cached.
///
/// Every fetch that is not cancelled ends with one of the three signals, after fetch() returned.
class DecodedTileFetcher : public QObject
{
    Q_OBJECT

public:
    /// Returns the image of a tile, or a null image for data the source cannot use
    using Decoder = std::function<QImage(const QByteArray& data)>;
    using LoggingCategory = const QLoggingCategory& (*)();

    /// \a networkManager is not owned and must outlive this object. A fetch that fails is logged in \a category.
    DecodedTileFetcher(QNetworkAccessManager* networkManager, int mapId, Decoder decoder, LoggingCategory category,
                       QObject* parent);

    /// Returns the id of the fetch (> 0)
    int fetch(const TileMath::TileKey& key);

    /// Stops a fetch, aborting its network request. None of the signals is emitted for it.
    void cancel(int fetchId);

    /// Fetches that have not ended or been cancelled
    int count() const { return _replies.count(); }

signals:
    void fetched(int fetchId, const TileMath::TileKey& key, const QImage& image);
    /// The tile could not be fetched: a later fetch can get it
    void failed(int fetchId, const TileMath::TileKey& key);
    /// The provider has no tile at this position: a later fetch gets the same answer
    void noTile(int fetchId, const TileMath::TileKey& key);

private:
    void _warnFailure(const TileMath::TileKey& key, const QString& reason);

    static constexpr int kFailureWarnIntervalMs = 10000;  ///< throttle for fetch-failure warnings

    QNetworkAccessManager* const _networkManager;
    const int _mapId;
    const Decoder _decoder;
    const LoggingCategory _category;
    int _fetchIdCounter = 0;
    QHash<int, QGCTileReply*> _replies;
    QElapsedTimer _failureWarnTimer;  ///< restarted on each emitted failure warning
};
