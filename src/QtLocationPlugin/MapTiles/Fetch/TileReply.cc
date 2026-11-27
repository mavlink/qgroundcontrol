#include "TileReply.h"

#include <chrono>

#include <QtCore/QDateTime>
#include <QtCore/QGlobalStatic>
#include <QtCore/QHash>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "CacheTasks.h"
#include "CacheTile.h"
#include "ElevationMapProvider.h"
#include "MapEngine.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "TileCache.h"
#include "TileServerPauses.h"

QGC_LOGGING_CATEGORY(TileReplyLog, "MapTiles.TileReply")

namespace {

// Keep pooled sockets warm across sparse tile/terrain fetches; Qt 6.11 otherwise reaps idle ones after 2 min.
constexpr int kConnectionCacheExpirySecs = 300;
constexpr std::chrono::seconds kTcpKeepAliveIdle{60};
constexpr std::chrono::seconds kTcpKeepAliveInterval{30};
constexpr int kTcpKeepAliveProbeCount = 3;
// A server that stops sending fails the request after this long without data, rather than holding it open
constexpr std::chrono::milliseconds kTileReplyTransferTimeout{10000};

/// What a request names itself to the server of a map that does not ask for the name of the application: Firefox, as
/// its extended support release names itself on the platform, with one version in all its places.
QString tileReplyBrowserUserAgent()
{
    static const QString userAgent = [] {
        const QLatin1StringView version("153.0");
        // On the desktop the token after Gecko is this date; on Android it is the version
        [[maybe_unused]] const QLatin1StringView desktopGecko("20100101");
#if defined(Q_OS_MACOS)
        const QLatin1StringView platform("Macintosh; Intel Mac OS X 10.15");
        const QLatin1StringView gecko = desktopGecko;
#elif defined(Q_OS_WIN)
        const QLatin1StringView platform("Windows NT 10.0; Win64; x64");
        const QLatin1StringView gecko = desktopGecko;
#elif defined(Q_OS_ANDROID)
        const QLatin1StringView platform("Android 10; Mobile");
        const QLatin1StringView gecko = version;
#elif defined(Q_OS_LINUX)
        const QLatin1StringView platform("X11; Linux x86_64");
        const QLatin1StringView gecko = desktopGecko;
#else
        // A platform that Firefox has no release of, as iOS
        const QLatin1StringView platform;
        const QLatin1StringView gecko;
#endif
        if (platform.isEmpty()) {
            return QStringLiteral("Qt Location based application");
        }
        return QStringLiteral("Mozilla/5.0 (%1; rv:%2) Gecko/%3 Firefox/%2").arg(platform, version, gecko);
    }();
    return userAgent;
}

QNetworkRequest tileRequest(const MapProvider& provider, int x, int y, int zoom)
{
    QNetworkRequest request;
    request.setUrl(provider.getTileURL(x, y, zoom));
    request.setTransferTimeout(kTileReplyTransferTimeout);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, provider.requiresApplicationUserAgent()
                                                            ? QGCNetworkHelper::defaultUserAgent()
                                                            : tileReplyBrowserUserAgent());
    const QByteArray referrer = provider.referrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = provider.token();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setAttribute(QNetworkRequest::ConnectionCacheExpiryTimeoutSecondsAttribute, kConnectionCacheExpirySecs);
    request.setTcpKeepAliveIdleTimeBeforeProbes(kTcpKeepAliveIdle);
    request.setTcpKeepAliveIntervalBetweenProbes(kTcpKeepAliveInterval);
    request.setTcpKeepAliveProbeCount(kTcpKeepAliveProbeCount);

    // Qt reads this only for a file or resource URL, which a custom URL can be: the file is then opened off this thread
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);

    return request;
}

/// The request that is fetching a tile from its server, by the hash of the tile. A global static tells when it is
/// gone: a singleton destroyed after it, as the application exits, still destroys its requests.
using TileFetches = QHash<QString, TileReply*>;

Q_GLOBAL_STATIC(TileFetches, tileFetches)

/// What a network reply that failed with \a error and \a httpStatus, 0 for none, tells about asking again
TileReply::NetworkFailure tileReplyNetworkFailure(QNetworkReply::NetworkError error, int httpStatus)
{
    using HttpStatusCode = QGCNetworkHelper::HttpStatusCode;
    if (httpStatus == static_cast<int>(HttpStatusCode::TooManyRequests)) {
        return TileReply::NetworkFailure::RateLimited;
    }
    if ((httpStatus == static_cast<int>(HttpStatusCode::RequestTimeout)) ||
        QGCNetworkHelper::isHttpServerError(httpStatus)) {
        return TileReply::NetworkFailure::Unavailable;
    }
    if ((httpStatus > 0) && !QGCNetworkHelper::isHttpSuccess(httpStatus)) {
        return TileReply::NetworkFailure::Rejected;
    }

    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::OperationCanceledError:  // The transfer timeout of the request
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::UnknownNetworkError:
            return TileReply::NetworkFailure::Unreachable;
        default:
            return TileReply::NetworkFailure::Rejected;
    }
}

}  // namespace

TileReply::TileReply(int mapId, int x, int y, int zoom, QObject* parent)
    : QObject(parent)
    , _mapId(mapId)
    , _x(x)
    , _y(y)
    , _zoom(zoom)
{}

TileReply::~TileReply()
{
    abort();
}

void TileReply::start()
{
    if (_state != State::Idle) {
        return;
    }

    _provider = MapProviderRegistry::getMapProviderFromMapId(_mapId);
    if (!_provider) {
        _state = State::Rejected;
        // Queued: finished() must not be emitted from within start()
        QMetaObject::invokeMethod(
            this,
            [this] {
                if (_state == State::Rejected) {
                    _finish(Error::RequestError, QStringLiteral("no map provider has the id %1").arg(_mapId));
                }
            },
            Qt::QueuedConnection);
        return;
    }
    _hash = _provider->getTileHash(_x, _y, _zoom);

    TileCache* const cache = TileCache::instance();
    // Only a QtLocation map initializes the tile cache on its own
    cache->ensureInitialized();

    if (_tileSet != UINT64_MAX) {
        _state = State::CacheLookup;
        // Queued: finished() must not be emitted from within start()
        QMetaObject::invokeMethod(this, [this] { _cacheLookupFinished(nullptr); }, Qt::QueuedConnection);
        return;
    }

    CacheTask* const task =
        CacheTasks::fetchTile(_hash, this, [this](const CacheTile* tile) { _cacheLookupFinished(tile); });

    // A lookup that cannot be queued ends from within addTask(), which Starting ignores
    _state = State::Starting;
    const bool lookupQueued = cache->addTask(task);
    _state = State::CacheLookup;
    if (!lookupQueued) {
        // The server needs no cache: go on as after a miss. Queued, as for a tile set
        QMetaObject::invokeMethod(this, [this] { _cacheLookupFinished(nullptr); }, Qt::QueuedConnection);
    }
}

void TileReply::abort()
{
    if ((_state == State::Finished) || (_state == State::Aborted)) {
        return;
    }
    if ((_state == State::Paused) && _serverPauses) {
        _serverPauses->withdraw(_provider->service(), this);
    }
    _state = State::Aborted;
    _endFetch();

    if (QNetworkReply* const reply = _networkReply.data()) {
        _networkReply.clear();  // its finished() is ignored from here on
        reply->abort();
        reply->deleteLater();
    }
}

void TileReply::_cacheLookupFinished(const CacheTile* tile)
{
    if (_state != State::CacheLookup) {
        return;
    }

    if (tile && _isUsable(tile->img)) {
        qCDebug(TileReplyLog) << "tile" << _hash << "served from cache";
        _data = tile->img;
        _format = tile->format;
        _cached = true;
        _finish(Error::NoError);
        return;
    }

    qCDebug(TileReplyLog) << "tile" << _hash << (tile ? "cached entry unusable," : "not cached,")
                          << "fetching from network";
    _fetchFromServer();
}

void TileReply::_fetchFromServer(bool released)
{
    if (!_provider->hasServer()) {
        _finish(Error::NoTileError, QStringLiteral("the map has no tile here: none was imported"));
        return;
    }

    // The user can have configured another source for the provider since start(): the tile gets the key of the
    // source it is fetched from
    _hash = _provider->getTileHash(_x, _y, _zoom);
    QNetworkRequest request = tileRequest(*_provider, _x, _y, _zoom);
    if (request.url().isEmpty()) {
        _finish(Error::NetworkError, QStringLiteral("provider has no URL for the tile"), NetworkFailure::Rejected);
        return;
    }

    _serverPauses = MapEngine::instance()->serverPauses();
    if (!released && _serverPauses->hold(_provider->service(), this, [this] { _serverReleased(); })) {
        qCDebug(TileReplyLog) << "tile" << _hash << "waits for its turn to ask its server";
        _state = State::Paused;
        return;
    }

    // A tile of an offline set is stored in the set by the request that fetches it
    TileFetches* const fetches = (_tileSet == UINT64_MAX) ? tileFetches() : nullptr;
    if (fetches) {
        if (TileReply* const fetching = fetches->value(_hash)) {
            qCDebug(TileReplyLog) << "tile" << _hash << "is being fetched for another request, waiting for it";
            _state = State::Waiting;
            (void) connect(fetching, &TileReply::fetchEnded, this, &TileReply::_awaitedFetchEnded,
                           Qt::QueuedConnection);
            return;
        }
        fetches->insert(_hash, this);
    }
    request.setOriginatingObject(this);

    _state = State::Network;
    // The engine is asked for its manager here, not before: a request that the cache answers needs none
    _networkReply = MapEngine::instance()->networkManager()->get(request);
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(_networkReply);
    (void) connect(_networkReply, &QNetworkReply::finished, this, &TileReply::_networkReplyFinished);
}

void TileReply::_serverReleased()
{
    if (_state == State::Paused) {
        _fetchFromServer(true);
    }
}

void TileReply::_networkReplyFinished()
{
    QNetworkReply* const reply = _networkReply.data();
    if (!reply || (_state != State::Network)) {
        return;
    }
    _networkReply.clear();
    reply->deleteLater();

    const QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (_serverPauses && httpStatus.isValid()) {
        const bool tooManyRequests =
            httpStatus.toInt() == static_cast<int>(QGCNetworkHelper::HttpStatusCode::TooManyRequests);
        const bool unavailable =
            httpStatus.toInt() == static_cast<int>(QGCNetworkHelper::HttpStatusCode::ServiceUnavailable);
        const std::optional<std::chrono::seconds> retryAfter =
            (tooManyRequests || unavailable)
                ? TileServerPauses::retryAfter(reply->headers(), QDateTime::currentDateTimeUtc())
                : std::nullopt;
        if (tooManyRequests) {
            _serverPauses->pause(_provider->service(), retryAfter);
        } else if (unavailable && retryAfter) {
            // Without the time, the requests ask again on their own, as after other failures that can clear up
            _serverPauses->pause(_provider->service(), retryAfter, TileServerPauses::Reason::Unavailable);
        } else {
            _serverPauses->answered(_provider->service());
        }
    }
    // A server answers 404 where it has no tile. Where the user sets the source, a wrong URL is the likelier cause:
    // that stays an error.
    const bool notFound = httpStatus.toInt() == static_cast<int>(QGCNetworkHelper::HttpStatusCode::NotFound);
    if (notFound && _provider->sourceSettings().isEmpty()) {
        _finish(Error::NoTileError, QStringLiteral("the server has no tile here: http 404"));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        _finish(Error::NetworkError,
                QStringLiteral("network error: %1 (http %2)").arg(reply->errorString()).arg(httpStatus.toInt()),
                tileReplyNetworkFailure(reply->error(), httpStatus.toInt()));
        return;
    }
    // Only an HTTP reply has a status
    if (httpStatus.isValid() && !QGCNetworkHelper::isHttpSuccess(httpStatus.toInt())) {
        _finish(Error::NetworkError, QStringLiteral("network error: http %1").arg(httpStatus.toInt()),
                tileReplyNetworkFailure(QNetworkReply::NoError, httpStatus.toInt()));
        return;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        _finish(Error::DataError, QStringLiteral("network fetch returned empty body"));
        return;
    }
    if (_provider->isNoTileImage(data)) {
        _finish(Error::NoTileError, QStringLiteral("the server has no tile here: it sent its no-tile picture"));
        return;
    }
    if (_provider->isRefusalImage(data)) {
        _finish(Error::DataError, QStringLiteral("the server refused the request: it sent a notice, not a tile"));
        return;
    }

    if (_provider->isElevationProvider()) {
        data = std::static_pointer_cast<const ElevationProvider>(_provider)->serialize(data);
        if (data.isEmpty()) {
            _finish(Error::DataError, QStringLiteral("network body failed to serialize as a terrain tile"));
            return;
        }
    } else if (MapProvider::imageFormatFromSignature(data).isEmpty()) {
        _finish(Error::DataError, QStringLiteral("network body is not a picture, %1 bytes").arg(data.size()));
        return;
    }

    if (_validator && !_validator(data)) {
        _finish(Error::DataError, QStringLiteral("network body is not a usable tile, %1 bytes").arg(data.size()));
        return;
    }

    const QString format = _provider->getImageFormat(data);
    if (format.isEmpty()) {
        _finish(Error::DataError, QStringLiteral("network body has an unknown tile format"));
        return;
    }

    qCDebug(TileReplyLog) << "tile" << _hash << "fetched from network," << data.size() << "bytes, caching";
    TileCache::instance()->cacheTile(_provider->mapId(), _hash, data, format, _tileSet);
    _data = data;
    _format = format;
    _finish(Error::NoError);
}

void TileReply::_awaitedFetchEnded(const QByteArray& data, const QString& format, Error error,
                                   NetworkFailure networkFailure, const QString& errorString)
{
    if (_state != State::Waiting) {
        return;
    }

    if (error != Error::NoError) {
        // The server would answer this request as it answered the other one
        _finish(error, errorString, networkFailure);
        return;
    }
    if (data.isEmpty()) {
        // The other request was aborted or deleted, which tells nothing about the tile
        _fetchFromServer();
        return;
    }
    if (_validator && !_validator(data)) {
        _finish(Error::DataError, QStringLiteral("network body is not a usable tile, %1 bytes").arg(data.size()));
        return;
    }

    qCDebug(TileReplyLog) << "tile" << _hash << "fetched for another request," << data.size() << "bytes";
    _data = data;
    _format = format;
    _finish(Error::NoError);
}

void TileReply::_endFetch()
{
    TileFetches* const fetches = tileFetches();
    if (!fetches) {
        return;
    }
    const auto fetch = fetches->constFind(_hash);
    if ((fetch == fetches->cend()) || (fetch.value() != this)) {
        return;
    }
    fetches->erase(fetch);

    emit fetchEnded(_data, _format, _error, _networkFailure, _errorString, QPrivateSignal());
}

void TileReply::_finish(Error error, const QString& errorString, NetworkFailure networkFailure)
{
    _state = State::Finished;
    _error = error;
    _networkFailure = (error == Error::NetworkError) ? networkFailure : NetworkFailure::None;
    _errorString = errorString;
    // Before finished(): its receiver is free to delete this request
    _endFetch();
    emit finished();
}

bool TileReply::_isUsable(const QByteArray& data) const
{
    // An elevation tile is no picture: it is in its provider's serialized form
    const bool isPicture = !MapProvider::imageFormatFromSignature(data).isEmpty();
    return !data.isEmpty() && (isPicture || _provider->isElevationProvider()) && !_provider->isNoTileImage(data) &&
           !_provider->isRefusalImage(data) && (!_validator || _validator(data));
}
