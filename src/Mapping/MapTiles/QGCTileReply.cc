#include "QGCTileReply.h"

#include <chrono>

#include <QtCore/QGlobalStatic>
#include <QtCore/QHash>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "ElevationMapProvider.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(QGCTileReplyLog, "MapTiles.QGCTileReply")

namespace {

// Keep pooled sockets warm across sparse tile/terrain fetches; Qt 6.11 otherwise reaps idle ones after 2 min.
constexpr int kConnectionCacheExpirySecs = 300;
constexpr std::chrono::seconds kTcpKeepAliveIdle{60};
constexpr std::chrono::seconds kTcpKeepAliveInterval{30};
constexpr int kTcpKeepAliveProbeCount = 3;

#if defined Q_OS_MACOS
constexpr const char* kUserAgent =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 14.5; rv:125.0) Gecko/20100101 Firefox/125.0";
#elif defined Q_OS_WIN
constexpr const char* kUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:100.0) Gecko/20100101 Firefox/112.0";
#elif defined Q_OS_ANDROID
constexpr const char* kUserAgent = "Mozilla/5.0 (Android 13; Tablet; rv:68.0) Gecko/68.0 Firefox/112.0";
#elif defined Q_OS_LINUX
constexpr const char* kUserAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/112.0";
#else
constexpr const char* kUserAgent = "Qt Location based application";
#endif

QNetworkRequest tileRequest(const MapProvider& provider, int x, int y, int zoom)
{
    QNetworkRequest request;
    request.setUrl(provider.getTileURL(x, y, zoom));
    request.setTransferTimeout(10000);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, provider.requiresApplicationUserAgent()
                                                            ? QGCNetworkHelper::defaultUserAgent()
                                                            : QString::fromLatin1(kUserAgent));
    const QByteArray referrer = provider.getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = provider.getToken();
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
using TileFetches = QHash<QString, QGCTileReply*>;

Q_GLOBAL_STATIC(TileFetches, tileFetches)

}  // namespace

QGCTileReply::QGCTileReply(QNetworkAccessManager* networkManager, int mapId, int x, int y, int zoom, QObject* parent)
    : QObject(parent)
    , _networkManager(networkManager)
    , _mapId(mapId)
    , _x(x)
    , _y(y)
    , _zoom(zoom)
{}

QGCTileReply::~QGCTileReply()
{
    abort();
}

void QGCTileReply::start()
{
    if (_state != State::Idle) {
        return;
    }

    _provider = UrlFactory::getMapProviderFromMapId(_mapId);
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
    _hash = UrlFactory::getTileHash(_provider->getMapName(), _x, _y, _zoom);

    QGCMapEngine* const engine = getQGCMapEngine();
    // Only a QtLocation map initializes the tile cache on its own
    engine->ensureInitialized();

    if (_tileSet != UINT64_MAX) {
        _state = State::CacheLookup;
        // Queued: finished() must not be emitted from within start()
        QMetaObject::invokeMethod(this, [this] { _cacheLookupFinished(nullptr); }, Qt::QueuedConnection);
        return;
    }

    QGCFetchTileTask* const task = new QGCFetchTileTask(_hash);
    (void) connect(task, &QGCFetchTileTask::tileFetched, this,
                   [this](const QGCCacheTile& tile) { _cacheLookupFinished(&tile); });
    (void) connect(task, &QGCMapTask::error, this, [this] { _cacheLookupFinished(nullptr); });

    // A lookup that cannot be queued reports its error from within addTask(), which Starting ignores
    _state = State::Starting;
    const bool lookupQueued = engine->addTask(task);
    _state = State::CacheLookup;
    if (!lookupQueued) {
        task->deleteLater();
        // The server needs no cache: go on as after a miss. Queued, as for a tile set
        QMetaObject::invokeMethod(this, [this] { _cacheLookupFinished(nullptr); }, Qt::QueuedConnection);
    }
}

void QGCTileReply::abort()
{
    if ((_state == State::Finished) || (_state == State::Aborted)) {
        return;
    }
    _state = State::Aborted;
    _endFetch();

    if (QNetworkReply* const reply = _networkReply.data()) {
        _networkReply.clear();  // its finished() is ignored from here on
        reply->abort();
        reply->deleteLater();
    }
}

void QGCTileReply::_cacheLookupFinished(const QGCCacheTile* tile)
{
    if (_state != State::CacheLookup) {
        return;
    }

    if (tile && _isUsable(tile->img)) {
        qCDebug(QGCTileReplyLog) << "tile" << _hash << "served from cache";
        _data = tile->img;
        _format = tile->format;
        _cached = true;
        _finish(Error::NoError);
        return;
    }

    qCDebug(QGCTileReplyLog) << "tile" << _hash << (tile ? "cached entry unusable," : "not cached,")
                             << "fetching from network";
    _fetchFromServer();
}

void QGCTileReply::_fetchFromServer()
{
    if (!_networkManager) {
        _finish(Error::NetworkError, QStringLiteral("network manager not available"));
        return;
    }

    // The user can have configured another source for the provider since start(): the tile gets the key of the
    // source it is fetched from
    _hash = UrlFactory::getTileHash(_provider->getMapName(), _x, _y, _zoom);
    QNetworkRequest request = tileRequest(*_provider, _x, _y, _zoom);
    if (request.url().isEmpty()) {
        _finish(Error::NetworkError, QStringLiteral("provider has no URL for the tile"));
        return;
    }

    // A tile of an offline set is stored in the set by the request that fetches it
    TileFetches* const fetches = (_tileSet == UINT64_MAX) ? tileFetches() : nullptr;
    if (fetches) {
        if (QGCTileReply* const fetching = fetches->value(_hash)) {
            qCDebug(QGCTileReplyLog) << "tile" << _hash << "is being fetched for another request, waiting for it";
            _state = State::Waiting;
            (void) connect(fetching, &QGCTileReply::fetchEnded, this, &QGCTileReply::_awaitedFetchEnded,
                           Qt::QueuedConnection);
            return;
        }
        fetches->insert(_hash, this);
    }
    request.setOriginatingObject(this);

    _state = State::Network;
    _networkReply = _networkManager->get(request);
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(_networkReply);
    (void) connect(_networkReply, &QNetworkReply::finished, this, &QGCTileReply::_networkReplyFinished);
}

void QGCTileReply::_networkReplyFinished()
{
    QNetworkReply* const reply = _networkReply.data();
    if (!reply || (_state != State::Network)) {
        return;
    }
    _networkReply.clear();
    reply->deleteLater();

    const QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    // A server answers 404 where it has no tile. Where the user sets the source, a wrong URL is the likelier cause:
    // that stays an error.
    const bool notFound = httpStatus.toInt() == static_cast<int>(QGCNetworkHelper::HttpStatusCode::NotFound);
    if (notFound && _provider->sourceSettings().isEmpty()) {
        _finish(Error::NoTileError, QStringLiteral("the server has no tile here: http 404"));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        _finish(Error::NetworkError,
                QStringLiteral("network error: %1 (http %2)").arg(reply->errorString()).arg(httpStatus.toInt()));
        return;
    }
    // Only an HTTP reply has a status
    if (httpStatus.isValid() && !QGCNetworkHelper::isHttpSuccess(httpStatus.toInt())) {
        _finish(Error::NetworkError, QStringLiteral("network error: http %1").arg(httpStatus.toInt()));
        return;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        _finish(Error::DataError, QStringLiteral("network fetch returned empty body"));
        return;
    }
    if (_provider->isNoTileImage(data)) {
        _finish(Error::NoTileError, QStringLiteral("network fetch returned Bing no-tile placeholder"));
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

    qCDebug(QGCTileReplyLog) << "tile" << _hash << "fetched from network," << data.size() << "bytes, caching";
    getQGCMapEngine()->cacheTile(_provider->getMapName(), _hash, data, format, _tileSet);
    _data = data;
    _format = format;
    _finish(Error::NoError);
}

void QGCTileReply::_awaitedFetchEnded(const QByteArray& data, const QString& format, Error error,
                                      const QString& errorString)
{
    if (_state != State::Waiting) {
        return;
    }

    if (error != Error::NoError) {
        // The server would answer this request as it answered the other one
        _finish(error, errorString);
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

    qCDebug(QGCTileReplyLog) << "tile" << _hash << "fetched for another request," << data.size() << "bytes";
    _data = data;
    _format = format;
    _finish(Error::NoError);
}

void QGCTileReply::_endFetch()
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

    emit fetchEnded(_data, _format, _error, _errorString, QPrivateSignal());
}

void QGCTileReply::_finish(Error error, const QString& errorString)
{
    _state = State::Finished;
    _error = error;
    _errorString = errorString;
    // Before finished(): its receiver is free to delete this request
    _endFetch();
    emit finished();
}

bool QGCTileReply::_isUsable(const QByteArray& data) const
{
    // An elevation tile is no picture: it is in its provider's serialized form
    const bool isPicture = !MapProvider::imageFormatFromSignature(data).isEmpty();
    return !data.isEmpty() && (isPicture || _provider->isElevationProvider()) && !_provider->isNoTileImage(data) &&
           !_provider->isRefusalImage(data) && (!_validator || _validator(data));
}
