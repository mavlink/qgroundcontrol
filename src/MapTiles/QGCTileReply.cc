#include "QGCTileReply.h"

#include <chrono>

#include <QtCore/QFile>
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
// TODO: Detect Wayland vs X11
constexpr const char* kUserAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/112.0";
#else
constexpr const char* kUserAgent = "Qt Location based application";
#endif

QNetworkRequest tileRequest(const MapProvider& provider, int x, int y, int zoom)
{
    QNetworkRequest request;
    request.setUrl(provider.getTileURL(x, y, zoom));
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(10000);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
    const QByteArray referrer = provider.getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = provider.getToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("keep-alive"));
    request.setAttribute(QNetworkRequest::ConnectionCacheExpiryTimeoutSecondsAttribute, kConnectionCacheExpirySecs);
    request.setTcpKeepAliveIdleTimeBeforeProbes(kTcpKeepAliveIdle);
    request.setTcpKeepAliveIntervalBetweenProbes(kTcpKeepAliveInterval);
    request.setTcpKeepAliveProbeCount(kTcpKeepAliveProbeCount);

    // Attributes
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);

    return request;
}

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

bool QGCTileReply::start()
{
    if (_state != State::Idle) {
        return false;
    }

    _provider = UrlFactory::getMapProviderFromMapId(_mapId);
    if (!_provider) {
        return false;
    }
    _hash = UrlFactory::getTileHash(_provider->getMapName(), _x, _y, _zoom);

    QGCMapEngine* const engine = getQGCMapEngine();
    // Only a QtLocation map initializes the tile cache on its own
    engine->ensureInitialized();

    if (_tileSet != UINT64_MAX) {
        _state = State::CacheLookup;
        // Queued: finished() must not be emitted from within start()
        QMetaObject::invokeMethod(this, [this] { _cacheLookupFinished(nullptr); }, Qt::QueuedConnection);
        return true;
    }

    QGCFetchTileTask* const task = new QGCFetchTileTask(_hash);
    (void) connect(task, &QGCFetchTileTask::tileFetched, this,
                   [this](const QGCCacheTile& tile) { _cacheLookupFinished(&tile); });
    (void) connect(task, &QGCMapTask::error, this, [this] { _cacheLookupFinished(nullptr); });

    // A lookup that cannot be queued reports its error from within addTask(), which Starting ignores
    _state = State::Starting;
    if (!engine->addTask(task)) {
        task->deleteLater();
        _state = State::Idle;
        return false;
    }
    _state = State::CacheLookup;

    return true;
}

void QGCTileReply::abort()
{
    if ((_state == State::Finished) || (_state == State::Aborted)) {
        return;
    }
    _state = State::Aborted;

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

    QNetworkRequest request = tileRequest(*_provider, _x, _y, _zoom);
    if (request.url().isEmpty()) {
        _finish(Error::NetworkError, QStringLiteral("provider has no URL for the tile"));
        return;
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
    if (_isNoTilePlaceholder(data)) {
        _finish(Error::NoTileError, QStringLiteral("network fetch returned Bing no-tile placeholder"));
        return;
    }

    if (_provider->isElevationProvider()) {
        data = std::static_pointer_cast<const ElevationProvider>(_provider)->serialize(data);
        if (data.isEmpty()) {
            _finish(Error::DataError, QStringLiteral("network body failed to serialize as a terrain tile"));
            return;
        }
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

void QGCTileReply::_finish(Error error, const QString& errorString)
{
    _state = State::Finished;
    _error = error;
    _errorString = errorString;
    emit finished();
}

bool QGCTileReply::_isUsable(const QByteArray& data) const
{
    return !data.isEmpty() && !_isNoTilePlaceholder(data) && (!_validator || _validator(data));
}

bool QGCTileReply::_isNoTilePlaceholder(const QByteArray& data) const
{
    // Bing answers with a placeholder image, not with an HTTP error, where it has no imagery
    static const QByteArray bingNoTileImage = [] {
        QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
        return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
    }();

    return _provider->isBingProvider() && !bingNoTileImage.isEmpty() && (data == bingNoTileImage);
}
