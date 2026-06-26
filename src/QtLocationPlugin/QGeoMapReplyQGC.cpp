#include "QGeoMapReplyQGC.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QSslError>
#include <algorithm>
#include <chrono>
#include <utility>

#include "BingMapProvider.h"
#include "ElevationMapProvider.h"
#include "MapProvider.h"
#include "QGCCacheTile.h"
#include "QGCHostCircuitBreaker.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGCTileCache.h"
#include "QGeoTileFetcherQGC.h"
#include "TileFetchMetrics.h"

QGC_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog, "QtLocationPlugin.QGeoTiledMapReplyQGC")

namespace {
constexpr int kHttpNotModified = 304;

// Self-owned background conditional GET (R5). Outlives the QGeoTiledMapReplyQGC
// that spawned it (which has already finished with stale bytes), writes the fresh
// tile into the SQLite cache on 200 / refreshes validators on 304, and deletes
// itself. Holds no pointer back to the reply, so it is safe after the reply dies.
class DetachedRevalidator : public QObject
{
public:
    DetachedRevalidator(QNetworkAccessManager* nam, const QNetworkRequest& request, QString providerName, int x, int y,
                        int z, QByteArray etag, QByteArray lastModified, qint64 staleExpiresAt)
        : _providerName(std::move(providerName)),
          _x(x),
          _y(y),
          _z(z),
          _etag(std::move(etag)),
          _lastModified(std::move(lastModified)),
          _staleExpiresAt(staleExpiresAt)
    {
        QNetworkReply* const reply = nam->get(request);
        reply->setParent(this);
        QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() { _finished(reply); });
    }

private:
    void _finished(QNetworkReply* reply)
    {
        const QGeoTileFetcherQGC::FetchResult result = QGeoTileFetcherQGC::snapshot(reply);
        const QString hash = UrlFactory::getTileHash(_providerName, _x, _y, _z);
        const QString host = reply->request().url().host();

        if (result.error == QNetworkReply::NoError) {
            HostCircuitBreaker::instance().recordSuccess(host);
            if (result.statusCode == kHttpNotModified) {
                const QByteArray etag = result.etag.isEmpty() ? _etag : result.etag;
                const QByteArray lm = result.lastModified.isEmpty() ? _lastModified : result.lastModified;
                const qint64 expiresAt = (result.expiresAt == 0) ? _staleExpiresAt : result.expiresAt;
                QGCTileCache::refreshTileValidators(_providerName, hash, etag, lm, expiresAt);
            } else if (QGCNetworkHelper::isHttpSuccess(result.statusCode) && !result.body.isEmpty()) {
                const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(_providerName);
                const QString format = provider ? provider->getImageFormat(result.body) : QString();
                if (!format.isEmpty()) {
                    QGCTileCache::cacheTile(_providerName, hash, result.body, format, UINT64_MAX, result.etag,
                                            result.lastModified, result.expiresAt, result.mustRevalidate);
                }
            }
            // Non-2xx (incl. 404): leave the stale copy in place; do not poison the cache.
        } else if (QGCNetworkHelper::isTransientError(result.error, result.statusCode)) {
            HostCircuitBreaker::instance().recordFailure(host);
        }
        deleteLater();
    }

    const QString _providerName;
    const int _x;
    const int _y;
    const int _z;
    const QByteArray _etag;
    const QByteArray _lastModified;
    const qint64 _staleExpiresAt;
};

}  // namespace

QByteArray QGeoTiledMapReplyQGC::_bingNoTileImage;
QByteArray QGeoTiledMapReplyQGC::_badTile;

QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QNetworkRequest& request,
                                           const QGeoTileSpec& spec, QObject* parent, QGeoTileFetcherQGC* fetcher)
    : QGeoTiledMapReply(spec, parent), _fetcher(fetcher), _networkManager(networkManager), _request(request)
{
    qCDebug(QGeoTiledMapReplyQGCLog) << this;
}

QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    qCDebug(QGeoTiledMapReplyQGCLog) << this;
}

bool QGeoTiledMapReplyQGC::init()
{
    if (m_initialized) {
        return true;
    }

    m_initialized = true;

    _initDataFromResources();

    (void) connect(
        this, &QGeoTiledMapReplyQGC::errorOccurred, this,
        [this](QGeoTiledMapReply::Error error, const QString& errorString) {
            // Single-tile failures are routine; debug-level keeps a network burst from flooding warnings.
            qCDebug(QGeoTiledMapReplyQGCLog) << error << errorString;
            setMapImageData(_badTile);
            setMapImageFormat(QStringLiteral("png"));
            setCached(false);
        },
        Qt::AutoConnection);

    QGCFetchTileTask* task = QGCTileCache::createFetchTileTask(
        UrlFactory::getProviderTypeFromQtMapId(tileSpec().mapId()), tileSpec().x(), tileSpec().y(), tileSpec().zoom());
    if (!task) {
        qCWarning(QGeoTiledMapReplyQGCLog) << "Failed to create fetch tile task";
        m_initialized = false;
        return false;
    }
    (void) connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::_cacheReply);
    (void) connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::_cacheError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        m_initialized = false;
        return false;
    }

    return true;
}

void QGeoTiledMapReplyQGC::_initDataFromResources()
{
    if (_bingNoTileImage.isEmpty()) {
        QFile file(":/res/BingNoTileBytes.dat");
        if (file.open(QFile::ReadOnly)) {
            _bingNoTileImage = file.readAll();
            file.close();
        }
    }

    if (_badTile.isEmpty()) {
        QFile file(":/res/images/notile.png");
        if (file.open(QFile::ReadOnly)) {
            _badTile = file.readAll();
            file.close();
        }
    }
}

void QGeoTiledMapReplyQGC::_networkReplyFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        setError(QGeoTiledMapReply::UnknownError, tr("Unexpected Error"));
        return;
    }
    reply->deleteLater();

    // The error path is owned by _networkReplyError (errorOccurred fires before
    // finished); bail so we don't clobber it.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    _handleNetworkResult(QGeoTileFetcherQGC::snapshot(reply));
}

void QGeoTiledMapReplyQGC::_handleNetworkResult(const QGeoTileFetcherQGC::FetchResult& result)
{
    if (isFinished()) {
        return;
    }

    if (result.error != QNetworkReply::NoError) {
        // Reached only via the coalescing path: the non-coalesced path bails on
        // error in _networkReplyFinished before delivering here. A delivered
        // cancel is therefore a leader timeout/abort (a follower that aborted
        // itself was already removed from the group), so retry rather than
        // finishing with no tile.
        if (_scheduleRetryIfNeeded(result.error, result.statusCode, result.retryAfter)) {
            return;
        }
        TileFetchMetrics::instance().recordNetworkError(_fetchTimer.isValid() ? _fetchTimer.elapsed() : 0);
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, result.errorString);
        return;
    }

    const int statusCode = result.statusCode;

    // 304 Not Modified: the stale cached body is still valid. Refresh its
    // validators/expiry in the cache and serve it without re-downloading.
    if (statusCode == kHttpNotModified) {
        if (_cachedTile) {
            const QByteArray newEtag = result.etag.isEmpty() ? _etag : result.etag;
            const QByteArray newLastModified = result.lastModified.isEmpty() ? _lastModified : result.lastModified;
            // A bare 304 has no freshness headers (_parseExpiry defaults to now+7d); keep the cached expiry instead.
            const qint64 newExpiresAt = result.hasExplicitExpiry ? result.expiresAt : _cachedTile->expiresAt;

            const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(tileSpec().mapId());
            if (provider) {
                QGCTileCache::refreshTileValidators(
                    provider->getMapName(),
                    UrlFactory::getTileHash(provider->getMapName(), tileSpec().x(), tileSpec().y(), tileSpec().zoom()),
                    newEtag, newLastModified, newExpiresAt);
            }
            HostCircuitBreaker::instance().recordSuccess(_request.url().host());
            TileFetchMetrics::instance().recordNotModified(_fetchTimer.isValid() ? _fetchTimer.elapsed() : 0);
            _serveCachedTile();
        } else {
            setError(QGeoTiledMapReply::UnknownError, tr("Unexpected 304 Not Modified"));
        }
        return;
    }

    if (!QGCNetworkHelper::isHttpSuccess(statusCode)) {
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, result.reasonPhrase);
        return;
    }

    QByteArray image = result.body;
    if (image.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, tr("Image is Empty"));
        return;
    }

    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(tileSpec().mapId());
    if (!mapProvider) {
        setError(QGeoTiledMapReply::UnknownError, tr("Invalid Map Provider"));
        return;
    }

    if (std::dynamic_pointer_cast<const BingMapProvider>(mapProvider) && (image == _bingNoTileImage)) {
        setError(QGeoTiledMapReply::CommunicationError, tr("Bing Tile Above Zoom Level"));
        return;
    }

    if (const SharedElevationProvider elevationProvider =
            std::dynamic_pointer_cast<const ElevationProvider>(mapProvider)) {
        image = elevationProvider->serialize(image);
        if (image.isEmpty()) {
            setError(QGeoTiledMapReply::ParseError, tr("Failed to Serialize Terrain Tile"));
            return;
        }
    }
    setMapImageData(image);

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        setError(QGeoTiledMapReply::ParseError, tr("Unknown Format"));
        return;
    }
    setMapImageFormat(format);

    QGCTileCache::cacheTile(mapProvider->getMapName(), tileSpec().x(), tileSpec().y(), tileSpec().zoom(), image, format,
                            UINT64_MAX, result.etag, result.lastModified, result.expiresAt, result.mustRevalidate);

    HostCircuitBreaker::instance().recordSuccess(_request.url().host());
    TileFetchMetrics::instance().recordNetworkSuccess(static_cast<quint64>(result.body.size()),
                                                      _fetchTimer.isValid() ? _fetchTimer.elapsed() : 0);
    setFinished(true);
}

void QGeoTiledMapReplyQGC::_networkReplyError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::OperationCanceledError) {
        setFinished(true);
        return;
    }

    const QNetworkReply* const reply = qobject_cast<const QNetworkReply*>(sender());
    const int statusCode = reply ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 0;

    if (_scheduleRetryIfNeeded(reply, error, statusCode)) {
        return;
    }

    if (!reply) {
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, tr("Invalid Reply"));
    } else {
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, reply->errorString());
    }
}

bool QGeoTiledMapReplyQGC::_scheduleRetryIfNeeded(const QNetworkReply* reply, QNetworkReply::NetworkError error,
                                                  int statusCode)
{
    if (!QGCNetworkHelper::isTransientError(error, statusCode)) {
        return false;
    }
    // Transient failure: feed the per-host breaker regardless of whether retries remain.
    HostCircuitBreaker::instance().recordFailure(_request.url().host());
    if (_retryAttempt >= kMaxRetryAttempts) {
        return false;
    }

    std::chrono::milliseconds delay = QGCNetworkHelper::retryBackoff(_retryAttempt);
    if (statusCode == QGCNetworkHelper::kHttpTooManyRequests) {
        delay = QGCNetworkHelper::retryAfterFromReply(reply).value_or(QGCNetworkHelper::kDefaultRateLimitDelay);
    }

    _retryAttempt++;
    qCDebug(QGeoTiledMapReplyQGCLog) << "Retrying tile fetch, attempt" << _retryAttempt << "in" << delay.count()
                                     << "ms";

    QTimer::singleShot(delay, this, [this]() {
        if (!isFinished()) {
            _startNetworkRequest();
        }
    });

    return true;
}

bool QGeoTiledMapReplyQGC::_scheduleRetryIfNeeded(QNetworkReply::NetworkError error, int statusCode,
                                                  std::optional<std::chrono::milliseconds> retryAfter)
{
    // Coalesced-only overload: a leader transfer timeout surfaces as
    // OperationCanceledError here (per-reply user aborts never reach this path),
    // so treat it as transient alongside the usual retriable errors.
    const bool transient =
        (error == QNetworkReply::OperationCanceledError) || QGCNetworkHelper::isTransientError(error, statusCode);
    if (!transient) {
        return false;
    }
    HostCircuitBreaker::instance().recordFailure(_request.url().host());
    if (_retryAttempt >= kMaxRetryAttempts) {
        return false;
    }

    std::chrono::milliseconds delay = QGCNetworkHelper::retryBackoff(_retryAttempt);
    if (statusCode == QGCNetworkHelper::kHttpTooManyRequests) {
        delay = retryAfter.value_or(QGCNetworkHelper::kDefaultRateLimitDelay);
    }

    _retryAttempt++;
    qCDebug(QGeoTiledMapReplyQGCLog) << "Retrying coalesced tile fetch, attempt" << _retryAttempt << "in"
                                     << delay.count() << "ms";

    QTimer::singleShot(delay, this, [this]() {
        if (!isFinished()) {
            _startNetworkRequest();
        }
    });

    return true;
}

void QGeoTiledMapReplyQGC::_networkReplySslErrors(const QList<QSslError>& errors)
{
    QString errorString;
    for (const QSslError& error : errors) {
        if (!errorString.isEmpty()) {
            (void) errorString.append('\n');
        }
        (void) errorString.append(error.errorString());
    }

    if (!errorString.isEmpty()) {
        setError(QGeoTiledMapReply::CommunicationError, errorString);
    }
}

void QGeoTiledMapReplyQGC::_cacheReply(QSharedPointer<QGCCacheTile> tile)
{
    if (!tile) {
        setError(QGeoTiledMapReply::UnknownError, tr("Invalid Cache Tile"));
        return;
    }

    _cachedTile = tile;

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    // expiresAt == 0 (no-store/no-cache/max-age=0) means immediately stale, not fresh-forever.
    const bool expired = (_cachedTile->expiresAt == 0) || (_cachedTile->expiresAt <= now);
    const bool revalidatable = !_cachedTile->etag.isEmpty() || !_cachedTile->lastModified.isEmpty();
    // A tile stored with Cache-Control no-cache/must-revalidate must never be
    // served without a fresh check; treat it like an expired tile.
    const bool needsRevalidation = expired || _cachedTile->mustRevalidate;

    // Fresh, or no network to revalidate over: serve what we cached.
    if (!needsRevalidation || !QGCNetworkHelper::isInternetAvailable()) {
        TileFetchMetrics::instance().recordCacheHit();
        _serveCachedTile();
        return;
    }

    // must-revalidate (RFC 7234 §5.2.2.1): a stale tile MUST NOT be served without a
    // successful check, so block on the network — a conditional GET when we have
    // validators, else a full refetch. No stale-while-revalidate for these.
    if (_cachedTile->mustRevalidate) {
        if (revalidatable) {
            _etag = _cachedTile->etag;
            _lastModified = _cachedTile->lastModified;
        }
        _startNetworkRequest();
        return;
    }

    // Plain expiry, online, with validators: stale-while-revalidate (R5) — hand the
    // stale bytes to the renderer NOW so the map never blanks, then refresh in the
    // background. QtLocation's QGeoTiledMapReply is single-shot, so the fresh tile
    // lands on the next pass.
    if (revalidatable) {
        _etag = _cachedTile->etag;
        _lastModified = _cachedTile->lastModified;
        qCDebug(QGeoTiledMapReplyQGCLog) << "Serving stale tile while revalidating" << tileSpec().x() << tileSpec().y()
                                         << tileSpec().zoom();
        _backgroundRevalidate();
        TileFetchMetrics::instance().recordCacheHit();
        _serveCachedTile();
        return;
    }

    // No validators: a full refetch so tiles lacking etag/Last-Modified don't stay
    // stale forever. Here the live reply IS the refetch (no stale copy to serve early
    // without validators to confirm it).
    qCDebug(QGeoTiledMapReplyQGCLog) << "Refetching expired tile (no validators)" << tileSpec().x() << tileSpec().y()
                                     << tileSpec().zoom();
    _startNetworkRequest();
}

void QGeoTiledMapReplyQGC::_serveCachedTile()
{
    if (!_cachedTile) {
        setError(QGeoTiledMapReply::UnknownError, tr("Invalid Cache Tile"));
        return;
    }
    setMapImageData(_cachedTile->img);
    setMapImageFormat(_cachedTile->format);
    setCached(true);
    setFinished(true);
    _cachedTile.reset();
}

void QGeoTiledMapReplyQGC::_serveAncestorFallbackOr(QGeoTiledMapReply::Error err, const QString& errStr)
{
    if (isFinished()) {
        return;
    }

    // Attempt the fallback scan at most once; on a second terminal failure raise the
    // original error directly so we never loop enqueuing fallback tasks.
    if (_fallbackAttempted) {
        setError(err, errStr);
        return;
    }
    _fallbackAttempted = true;

    const QString provider = UrlFactory::getProviderTypeFromQtMapId(tileSpec().mapId());
    if (provider.isEmpty()) {
        setError(err, errStr);
        return;
    }

    _fallbackPendingError = err;
    _fallbackPendingErrorString = errStr;

    const int tileSize = UrlFactory::useRetinaTiles() ? 512 : 256;
    auto* task = new QGCFetchFallbackTileTask(provider, tileSpec().x(), tileSpec().y(), tileSpec().zoom(), tileSize);
    (void) connect(task, &QGCFetchFallbackTileTask::fallbackFetched, this, &QGeoTiledMapReplyQGC::_fallbackTileFetched);
    (void) connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::_fallbackTileError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        setError(err, errStr);
    }
}

void QGeoTiledMapReplyQGC::_fallbackTileFetched(const QByteArray& image, const QString& format, int levelDelta)
{
    if (isFinished()) {
        return;
    }
    if (image.isEmpty()) {
        setError(_fallbackPendingError, _fallbackPendingErrorString);
        return;
    }
    qCDebug(QGeoTiledMapReplyQGCLog) << "Serving lower-zoom fallback tile" << tileSpec().x() << tileSpec().y()
                                     << tileSpec().zoom() << "from ancestor" << levelDelta << "levels up";
    setMapImageData(image);
    setMapImageFormat(format);
    setCached(true);
    setFinished(true);
}

void QGeoTiledMapReplyQGC::_fallbackTileError(QGCMapTask::TaskType type, QStringView errorString)
{
    Q_UNUSED(type);
    Q_UNUSED(errorString);
    if (isFinished()) {
        return;
    }
    setError(_fallbackPendingError, _fallbackPendingErrorString);
}

void QGeoTiledMapReplyQGC::_backgroundRevalidate()
{
    // Only spawn a detached GET on the fetcher path, where the QNAM is the engine's
    // long-lived shared manager. Without a fetcher (bare per-reply manager) the
    // manager may not outlive the detached reply, so serve stale only.
    if (!_cachedTile || !_networkManager || !_fetcher) {
        return;
    }
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(tileSpec().mapId());
    if (!provider) {
        return;
    }

    QNetworkRequest req = _request;
    if (!_etag.isEmpty()) {
        req.setRawHeader(QByteArrayLiteral("If-None-Match"), _etag);
    }
    if (!_lastModified.isEmpty()) {
        req.setRawHeader(QByteArrayLiteral("If-Modified-Since"), _lastModified);
    }
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    // Skip the live GET entirely when the breaker is open for this host.
    if (!HostCircuitBreaker::instance().allowRequest(req.url().host())) {
        return;
    }

    // Self-owned: outlives this reply, deletes itself when the GET completes.
    new DetachedRevalidator(_networkManager, req, provider->getMapName(), tileSpec().x(), tileSpec().y(),
                            tileSpec().zoom(), _cachedTile->etag, _cachedTile->lastModified, _cachedTile->expiresAt);
}

void QGeoTiledMapReplyQGC::_cacheError(QGCMapTask::TaskType type, QStringView errorString)
{
    Q_UNUSED(errorString);

    if (type != QGCMapTask::TaskType::taskFetchTile) {
        qCWarning(QGeoTiledMapReplyQGCLog) << "Unexpected cache task type in _cacheError:" << static_cast<int>(type);
        return;
    }

    if (!QGCNetworkHelper::isInternetAvailable()) {
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, tr("Network Not Available"));
        return;
    }

    TileFetchMetrics::instance().recordCacheMiss();
    _startNetworkRequest();
}

void QGeoTiledMapReplyQGC::_applyConditionalHeaders()
{
    // Force revalidation past QNAM's own cache so the conditional request
    // actually reaches the server.
    if (!_etag.isEmpty()) {
        _request.setRawHeader(QByteArrayLiteral("If-None-Match"), _etag);
    }
    if (!_lastModified.isEmpty()) {
        _request.setRawHeader(QByteArrayLiteral("If-Modified-Since"), _lastModified);
    }
    if (!_etag.isEmpty() || !_lastModified.isEmpty()) {
        _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }
}

void QGeoTiledMapReplyQGC::_cancelDedupFetch()
{
    if (_fetcher) {
        _fetcher->cancelFetch(this, _request);
    }
}

void QGeoTiledMapReplyQGC::_startNetworkRequest()
{
    // Circuit breaker (R2): fast-fail when the breaker for this host is OPEN so a
    // dead/erroring endpoint doesn't keep spawning doomed GETs. A HALF-OPEN probe
    // is allowed through to test recovery.
    const QString host = _request.url().host();
    if (!HostCircuitBreaker::instance().allowRequest(host)) {
        qCDebug(QGeoTiledMapReplyQGCLog) << "Circuit breaker open; failing fast for host" << host;
        _serveAncestorFallbackOr(QGeoTiledMapReply::CommunicationError, tr("Host temporarily unavailable"));
        return;
    }

    _request.setOriginatingObject(this);
    _applyConditionalHeaders();
    if (!_fetchTimer.isValid()) {
        _fetchTimer.start();
    }

    if (_fetcher) {
        // Coalesce identical in-flight fetches. The fetcher delivers the result by
        // calling _handleNetworkResult directly, so no signal hookup is needed.
        (void) connect(this, &QGeoTiledMapReplyQGC::aborted, this, &QGeoTiledMapReplyQGC::_cancelDedupFetch,
                       Qt::UniqueConnection);
        _fetcher->dedupFetch(this, _request);
        return;
    }

    if (!_networkManager) {
        setError(QGeoTiledMapReply::CommunicationError, tr("No network manager available"));
        return;
    }

    QNetworkReply* const reply = _networkManager->get(_request);
    reply->setParent(this);
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);

    (void) connect(reply, &QNetworkReply::finished, this, &QGeoTiledMapReplyQGC::_networkReplyFinished);
    (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGeoTiledMapReplyQGC::_networkReplyError);
    (void) connect(reply, &QNetworkReply::sslErrors, this, &QGeoTiledMapReplyQGC::_networkReplySslErrors);
    (void) connect(this, &QGeoTiledMapReplyQGC::aborted, reply, &QNetworkReply::abort);
}

void QGeoTiledMapReplyQGC::abort()
{
    QGeoTiledMapReply::abort();
}
