#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class MapProvider;
class TileServerPauses;
class QNetworkReply;
struct CacheTile;

/// A request for one map or elevation tile. The tile comes from the shared tile cache or, on a miss, from the
/// provider's server; a tile fetched from the server is stored in the cache.
///
/// A map tile is a PNG, JPEG, GIF or WebP picture, told by its signature: any other answer of the server is a DataError
/// and is not stored, and any other entry of the cache is fetched again. An elevation tile is checked by its
/// provider.
///
/// Requests that miss the cache while another request is fetching the same tile wait for that fetch, so the server
/// is asked once: they finish with its tile, which each checks with its own validator, or with its error, the
/// DataError of the fetching request's validator included. One of them fetches the tile itself when the fetching
/// request is aborted or deleted. A request for an offline tile set takes no part in this.
///
/// A request does not ask a server that answered that it was asked too often, or unavailable for a time it named,
/// until the pause of the server ends and its turn comes: it waits after its cache lookup. All requests share the
/// pauses (MapEngine::serverPauses()), whichever part of QGC makes them and whichever map of the service
/// (MapProvider::service()) they are for.
///
/// Independent of QtLocation, so every map engine and the terrain queries fetch tiles the same way.
///
/// For the main thread only: a request reads the settings of its provider, and the requests share their fetches and
/// the network manager of the map engine without a lock.
class TileReply : public QObject
{
    Q_OBJECT

public:
    enum class Error
    {
        NoError,
        NetworkError,  ///< The server could not be reached, or it reported an error: networkFailure() tells which
        NoTileError,   ///< The provider has no tile at this position: asking again gets none
        DataError,     ///< The server answered with something that is not a tile
        RequestError,  ///< The request names no map provider
    };
    Q_ENUM(Error)

    /// What made a request end with a NetworkError, which tells whether asking again later can get the tile
    enum class NetworkFailure
    {
        None,         ///< The request did not end with a NetworkError
        Unreachable,  ///< The server did not answer: no connection to it, or the transfer timed out
        Unavailable,  ///< The server answered that it cannot serve the tile now: HTTP 408 or 5xx
        RateLimited,  ///< The server answered that it was asked too often: HTTP 429
        Rejected,     ///< Any other answer of the server, or a request that could not be made
    };
    Q_ENUM(NetworkFailure)

    /// Returns false for a tile the caller cannot use
    using Validator = std::function<bool(const QByteArray& data)>;

    /// The server is asked through the network manager of the map engine (MapEngine::networkManager()), which all
    /// requests share.
    TileReply(int mapId, int x, int y, int zoom, QObject* parent = nullptr);
    ~TileReply() override;

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

    NetworkFailure networkFailure() const { return _networkFailure; }

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

    /// Tells the requests waiting for this one's fetch how it ended: with the tile \a data, with \a error, or with
    /// neither when this request was aborted or deleted
    void fetchEnded(const QByteArray& data, const QString& format, TileReply::Error error,
                    TileReply::NetworkFailure networkFailure, const QString& errorString, QPrivateSignal);

private:
    enum class State
    {
        Idle,
        Rejected,  ///< Names no map provider: finishes with the error once start() returned
        Starting,
        CacheLookup,
        Network,
        Waiting,  ///< Another request is fetching the tile from the server
        Paused,   ///< Waits for the pause of the server of the provider to end, and its turn after it
        Finished,
        Aborted,
    };

    void _cacheLookupFinished(const CacheTile* tile);
    /// \a released: the server pauses let the request ask now
    void _fetchFromServer(bool released = false);
    void _serverReleased();
    void _networkReplyFinished();
    void _awaitedFetchEnded(const QByteArray& data, const QString& format, Error error, NetworkFailure networkFailure,
                            const QString& errorString);
    void _endFetch();
    void _finish(Error error, const QString& errorString = QString(),
                 NetworkFailure networkFailure = NetworkFailure::None);
    bool _isUsable(const QByteArray& data) const;

    const int _mapId;
    const int _x;
    const int _y;
    const int _zoom;
    Validator _validator;
    quint64 _tileSet = UINT64_MAX;
    QPointer<TileServerPauses> _serverPauses;  ///< Of the map engine, as the request went to ask the server

    State _state = State::Idle;
    std::shared_ptr<const MapProvider> _provider;
    QString _hash;
    QPointer<QNetworkReply> _networkReply;

    Error _error = Error::NoError;
    NetworkFailure _networkFailure = NetworkFailure::None;
    QString _errorString;
    QByteArray _data;
    QString _format;
    bool _cached = false;
};
