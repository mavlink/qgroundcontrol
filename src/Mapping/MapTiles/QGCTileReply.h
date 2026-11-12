#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class MapProvider;
class QNetworkAccessManager;
class QNetworkReply;
struct QGCCacheTile;

/// A request for one map or elevation tile. The tile comes from the shared tile cache or, on a miss, from the
/// provider's server; a tile fetched from the server is stored in the cache.
///
/// Requests that miss the cache while another request is fetching the same tile wait for that fetch, so the server
/// is asked once. A request for an offline tile set takes no part in this.
///
/// Independent of QtLocation, so every map engine and the terrain queries fetch tiles the same way.
class QGCTileReply : public QObject
{
    Q_OBJECT

public:
    enum class Error
    {
        NoError,
        NetworkError,  ///< The server could not be reached, or it reported an error
        NoTileError,   ///< The provider has no tile at this position
        DataError,     ///< The server answered with something that is not a tile
        RequestError,  ///< The request names no map provider
    };
    Q_ENUM(Error)

    /// Returns false for a tile the caller cannot use
    using Validator = std::function<bool(const QByteArray& data)>;

    QGCTileReply(QNetworkAccessManager* networkManager, int mapId, int x, int y, int zoom, QObject* parent = nullptr);
    ~QGCTileReply() override;

    /// A cached tile the validator rejects is fetched from the server again. A tile from the server it rejects is a
    /// DataError and is not stored in the cache.
    void setValidator(Validator validator) { _validator = std::move(validator); }

    /// Downloads the tile for an offline tile set: the tile is stored in \a tileSet, and the cache is not asked for
    /// it first, as a set downloads only the tiles the cache lacks.
    void setTileSet(quint64 tileSet) { _tileSet = tileSet; }

    /// Starts the request. finished() is emitted once, after start() returned: a request that cannot be made
    /// finishes with an error. A second call does nothing.
    void start();

    /// Stops the request. finished() is not emitted.
    void abort();

    bool isFinished() const { return _state == State::Finished; }

    Error error() const { return _error; }

    QString errorString() const { return _errorString; }

    /// The tile as the cache stores it: an elevation tile is in its provider's serialized form
    QByteArray data() const { return _data; }

    QString format() const { return _format; }

    bool isCached() const { return _cached; }

    /// Key of the tile in the cache; empty until start(), and for a request that names no map provider
    QString hash() const { return _hash; }

signals:
    /// Emitted once, after start() returned
    void finished();

    /// Tells the requests waiting for this one's fetch that it ended; \a data is empty when it did not get the tile
    void fetchEnded(const QByteArray& data, const QString& format, QPrivateSignal);

private:
    enum class State
    {
        Idle,
        Rejected,  ///< Names no map provider: finishes with the error once start() returned
        Starting,
        CacheLookup,
        Network,
        Waiting,  ///< Another request is fetching the tile from the server
        Finished,
        Aborted,
    };

    void _cacheLookupFinished(const QGCCacheTile* tile);
    void _fetchFromServer();
    void _networkReplyFinished();
    void _awaitedFetchEnded(const QByteArray& data, const QString& format);
    void _endFetch();
    void _finish(Error error, const QString& errorString = QString());
    bool _isUsable(const QByteArray& data) const;
    bool _isNoTilePlaceholder(const QByteArray& data) const;

    QPointer<QNetworkAccessManager> _networkManager;
    const int _mapId;
    const int _x;
    const int _y;
    const int _zoom;
    Validator _validator;
    quint64 _tileSet = UINT64_MAX;

    State _state = State::Idle;
    std::shared_ptr<const MapProvider> _provider;
    QString _hash;
    QPointer<QNetworkReply> _networkReply;

    Error _error = Error::NoError;
    QString _errorString;
    QByteArray _data;
    QString _format;
    bool _cached = false;
};
