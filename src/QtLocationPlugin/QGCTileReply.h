#pragma once

#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class MapProvider;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
struct QGCCacheTile;

/// A request for one map or elevation tile. The tile comes from the shared tile cache or, on a miss, from the
/// provider's server; a tile fetched from the server is stored in the cache.
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
    };
    Q_ENUM(Error)

    QGCTileReply(QNetworkAccessManager* networkManager, int mapId, int x, int y, int zoom, QObject* parent = nullptr);
    ~QGCTileReply() override;

    /// Asks the server only while the system reports internet access. Off by default: the server is always tried.
    void setRequiresInternet(bool requiresInternet) { _requiresInternet = requiresInternet; }

    /// Returns false when the request could not be started; finished() is then not emitted.
    [[nodiscard]] bool start();

    /// Stops the request. finished() is not emitted.
    void abort();

    bool isFinished() const { return _state == State::Finished; }

    Error error() const { return _error; }

    QString errorString() const { return _errorString; }

    /// The tile as the cache stores it: an elevation tile is in its provider's serialized form
    QByteArray data() const { return _data; }

    QString format() const { return _format; }

    bool isCached() const { return _cached; }

    /// Key of the tile in the cache; empty until start()
    QString hash() const { return _hash; }

    /// The request for a tile on its provider's server. Its URL is empty when \a mapId names no provider.
    static QNetworkRequest networkRequest(int mapId, int x, int y, int zoom);

signals:
    /// Emitted once, after start() returned
    void finished();

private:
    enum class State
    {
        Idle,
        Starting,
        CacheLookup,
        Network,
        Finished,
        Aborted,
    };

    void _cacheLookupFinished(const QGCCacheTile* tile);
    void _fetchFromServer();
    void _networkReplyFinished();
    void _finish(Error error, const QString& errorString = QString());
    bool _isUsable(const QByteArray& data) const;
    bool _isNoTilePlaceholder(const QByteArray& data) const;

    QPointer<QNetworkAccessManager> _networkManager;
    const int _mapId;
    const int _x;
    const int _y;
    const int _zoom;
    bool _requiresInternet = false;

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
