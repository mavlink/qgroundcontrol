#include "QGeoTileFetcherQGC.h"

#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QTimeZone>
#include <QtCore/private/qobject_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilefetcher_p_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkInformation>
#include <QtNetwork/QNetworkRequest>

#include "MapProvider.h"
#include "QGCHostCircuitBreaker.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "QtLocationPlugin.QGeoTileFetcherQGC")

QByteArray QGeoTileFetcherQGC::s_userAgentOverride;
QMutex QGeoTileFetcherQGC::s_templateMutex;
QHash<int, QGeoTileFetcherQGC::CachedRequestTemplate> QGeoTileFetcherQGC::s_templateCache;

void QGeoTileFetcherQGC::setUserAgentOverride(const QByteArray& userAgent)
{
    s_userAgentOverride = userAgent;
}

namespace {
// Raster tile providers commonly omit Expires/Cache-Control; cap how long an
// undated tile is treated as fresh before revalidation. 7 days is conservative
// and within OSM tile-usage etiquette for client-side caching.
constexpr qint64 kDefaultTileExpirySecs = 7 * 24 * 60 * 60;
}  // namespace

uint32_t QGeoTileFetcherQGC::concurrentDownloads(const QString& type)
{
    Q_UNUSED(type);

    const QNetworkInformation* const networkInfo = QNetworkInformation::instance();
    if (!networkInfo) {
        return kDefaultConcurrent;
    }

    switch (networkInfo->transportMedium()) {
        case QNetworkInformation::TransportMedium::Cellular:
        case QNetworkInformation::TransportMedium::Bluetooth:
            return kConstrainedConcurrent;
        default:
            return kDefaultConcurrent;
    }
}

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, const QVariantMap& parameters,
                                       QGeoTiledMappingManagerEngineQGC* parent)
    : QGeoTileFetcher(parent), m_networkManager(networkManager)
{
    if (!networkManager) {
        qCDebug(QGeoTileFetcherQGCLog) << "No network manager injected; using a fetcher-owned shared instance";
    }

    qCDebug(QGeoTileFetcherQGCLog) << this;

    m_concurrencyBudget = concurrentDownloads(QString());
    if (QNetworkInformation* const networkInfo = QNetworkInformation::instance()) {
        (void) connect(networkInfo, &QNetworkInformation::transportMediumChanged, this,
                       [this](QNetworkInformation::TransportMedium) {
                           m_concurrencyBudget = concurrentDownloads(QString());
                           _startNextPending();
                       });
    }

    if (parameters.contains(QStringLiteral("useragent"))) {
        setUserAgentOverride(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    qCDebug(QGeoTileFetcherQGCLog) << this;

    // The concurrency gate is a process-wide singleton: slots held by in-flight
    // leader GETs leak unless released here on mid-flight teardown.
    for (auto it = m_inFlight.constBegin(); it != m_inFlight.constEnd(); ++it) {
        if (it->reply) {
            disconnect(it->reply, nullptr, this, nullptr);
            it->reply->abort();
            HostConcurrencyGate::instance().release(it->host);
        }
    }
}

QNetworkAccessManager* QGeoTileFetcherQGC::_sharedNetworkManager()
{
    if (m_networkManager) {
        return m_networkManager;
    }
    if (!m_ownedNetworkManager) {
        m_ownedNetworkManager = new QNetworkAccessManager(this);
        QGCNetworkHelper::configureProxy(m_ownedNetworkManager);
    }
    return m_ownedNetworkManager;
}

size_t qHash(const QGeoTileFetcherQGC::FetchKey& k, size_t seed) noexcept
{
    return qHashMulti(seed, k.url, k.ifNoneMatch, k.ifModifiedSince);
}

QGeoTileFetcherQGC::FetchKey QGeoTileFetcherQGC::_keyFor(const QNetworkRequest& request)
{
    return FetchKey{
        request.url(),
        request.rawHeader(QByteArrayLiteral("If-None-Match")),
        request.rawHeader(QByteArrayLiteral("If-Modified-Since")),
    };
}

void QGeoTileFetcherQGC::dedupFetch(QGeoTiledMapReplyQGC* requester, const QNetworkRequest& request)
{
    if (!requester) {
        return;
    }

    const FetchKey key = _keyFor(request);
    InFlight& entry = m_inFlight[key];
    entry.followers.append(QPointer<QGeoTiledMapReplyQGC>(requester));

    if (entry.reply) {
        return;
    }

    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(requester->tileSpec().mapId());
    entry.isOSM = provider && provider->isOSMProvider();
    const QString host = request.url().host();

    // Over the concurrency budget (or the shared per-host gate): park this request
    // and let a finishing leader pull it in. Drained in arrival order, which is the
    // viewport-priority order produced by updateTileRequests().
    if (!_canStart(entry.isOSM, host)) {
        m_pendingFetches.append(PendingFetch{key, request, entry.isOSM, host});
        return;
    }

    _startLeaderGet(key, request, entry.isOSM, host);
}

void QGeoTileFetcherQGC::_startLeaderGet(const FetchKey& key, const QNetworkRequest& request, bool isOSM,
                                         const QString& host)
{
    auto it = m_inFlight.find(key);
    if ((it == m_inFlight.end()) || it->reply) {
        return;
    }

    // Shared cross-path gate is the hard cap (A2): another QNAM/path may already
    // hold this host's slots. If so, re-park so a release elsewhere wakes us.
    if (!HostConcurrencyGate::instance().tryAcquire(host, static_cast<int>(_currentBudget()), isOSM)) {
        m_pendingFetches.append(PendingFetch{key, request, isOSM, host});
        return;
    }

    QNetworkAccessManager* nam = _sharedNetworkManager();
    QNetworkRequest req = request;
    // Single timeout authority: the fetcher's leader timeout (chrono overload).
    // getNetworkRequest() no longer sets one, so this always applies.
    req.setTransferTimeout(kLeaderTransferTimeout);
    QNetworkReply* reply = nam->get(req);
    reply->setParent(this);
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
    it->reply = reply;
    it->host = host;

    ++m_activeNetworkFetches;
    if (isOSM) {
        ++m_activeOSMFetches;
    }
    m_peakConcurrentFetches = qMax(m_peakConcurrentFetches, m_activeNetworkFetches);

    // errorOccurred fires before finished for failures, but finished always fires
    // afterwards (incl. timeout/cancel) — so completion is driven solely off
    // finished() and the snapshot carries the error. ssl errors surface as a
    // failing finished() once ignoreSslErrorsIfNeeded has had its say.
    (void) connect(reply, &QNetworkReply::finished, this, [this, key, reply]() { _onLeaderFinished(key, reply); });
}

void QGeoTileFetcherQGC::cancelFetch(QGeoTiledMapReplyQGC* requester, const QNetworkRequest& request)
{
    const FetchKey key = _keyFor(request);
    auto it = m_inFlight.find(key);
    if (it == m_inFlight.end()) {
        return;
    }
    it->followers.removeIf(
        [requester](const QPointer<QGeoTiledMapReplyQGC>& p) { return (p == requester) || p.isNull(); });

    if (it->followers.isEmpty()) {
        // No one left to receive this fetch: tear down the leader reply so it
        // doesn't run on (and fire into a dead lambda) after the last follower left.
        const bool hadReply = (it->reply != nullptr);
        const bool wasOSM = it->isOSM;
        const QString host = it->host;
        _eraseEntry(key, it->reply);
        if (hadReply) {
            _onFetchFinished(wasOSM, host);
        } else {
            // Parked but never started: just drop it from the pending queue.
            m_pendingFetches.removeIf([&key](const PendingFetch& p) { return p.key == key; });
        }
    }
}

void QGeoTileFetcherQGC::_eraseEntry(const FetchKey& key, QNetworkReply* reply)
{
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
    (void) m_inFlight.remove(key);
}

void QGeoTileFetcherQGC::_onLeaderFinished(const FetchKey& key, QNetworkReply* reply)
{
    // Ignore stale or superseded deliveries: the entry may have been erased on
    // cancel, or recreated by a retry with a different reply. In both cases the
    // reply's lifetime is owned by whoever replaced us, so don't touch it here.
    const auto it = m_inFlight.constFind(key);
    if ((it == m_inFlight.constEnd()) || (it->reply != reply)) {
        return;
    }

    const FetchResult result = snapshot(reply);

    // Detach before delivering so a follower re-requesting (e.g. retry) starts fresh.
    const InFlight entry = m_inFlight.take(key);
    disconnect(reply, nullptr, this, nullptr);
    reply->deleteLater();

    _onFetchFinished(entry.isOSM, entry.host);

    for (const QPointer<QGeoTiledMapReplyQGC>& follower : entry.followers) {
        if (follower) {
            follower->_handleNetworkResult(result);
        }
    }
}

void QGeoTileFetcherQGC::_onFetchFinished(bool wasOSM, const QString& host)
{
    if (m_activeNetworkFetches > 0) {
        --m_activeNetworkFetches;
    }
    if (wasOSM && (m_activeOSMFetches > 0)) {
        --m_activeOSMFetches;
    }
    HostConcurrencyGate::instance().release(host);
    _startNextPending();
}

bool QGeoTileFetcherQGC::_canStart(bool isOSM, const QString& host) const
{
    Q_UNUSED(host);
    // Cheap local pre-filter only: the authoritative cross-path cap is claimed
    // (and may still reject) in _startLeaderGet via HostConcurrencyGate. This
    // keeps the in-thread budget/OSM accounting for scheduling fairness.
    if (m_activeNetworkFetches >= static_cast<int>(_currentBudget())) {
        return false;
    }
    if (isOSM && (m_activeOSMFetches >= static_cast<int>(kConstrainedConcurrent))) {
        return false;
    }
    return true;
}

void QGeoTileFetcherQGC::_startNextPending()
{
    // Scan rather than strict takeFirst: an OSM request blocked by its cap must not
    // stall non-OSM (or other) requests queued behind it. Order is otherwise preserved.
    bool started = true;
    while (started && !m_pendingFetches.isEmpty()) {
        started = false;
        for (int i = 0; i < m_pendingFetches.size(); ++i) {
            const PendingFetch& pending = m_pendingFetches.at(i);
            if (!_canStart(pending.isOSM, pending.host)) {
                continue;
            }
            const auto it = m_inFlight.constFind(pending.key);
            if ((it == m_inFlight.constEnd()) || it->reply) {
                // Cancelled/already started while parked: drop and keep scanning.
                m_pendingFetches.removeAt(i);
                started = true;
                break;
            }
            const PendingFetch taken = m_pendingFetches.takeAt(i);
            _startLeaderGet(taken.key, taken.request, taken.isOSM, taken.host);
            started = true;
            break;
        }
    }
}

uint32_t QGeoTileFetcherQGC::_currentBudget() const
{
    return m_concurrencyBudget;
}

qint64 QGeoTileFetcherQGC::_parseExpiry(const QNetworkReply* reply, bool *hasExplicitExpiry)
{
    if (hasExplicitExpiry) {
        *hasExplicitExpiry = true;
    }

    // Cache-Control: max-age wins over Expires per RFC 7234.
    const QByteArray cacheControl = reply->rawHeader(QByteArrayLiteral("Cache-Control"));
    if (!cacheControl.isEmpty()) {
        qint64 sMaxAge = -1;
        for (const QByteArray& directive : cacheControl.split(',')) {
            const QByteArray trimmed = directive.trimmed();
            if (trimmed.startsWith("no-store") || trimmed.startsWith("no-cache")) {
                return 0;
            }
            if (trimmed.startsWith("max-age=")) {
                bool ok = false;
                const qint64 maxAge = trimmed.mid(8).toLongLong(&ok);
                return (ok && (maxAge > 0)) ? (QDateTime::currentSecsSinceEpoch() + maxAge) : 0;
            }
            // s-maxage is a fallback only: max-age above wins for a private cache.
            if (trimmed.startsWith("s-maxage=")) {
                bool ok = false;
                const qint64 value = trimmed.mid(9).toLongLong(&ok);
                if (ok) {
                    sMaxAge = value;
                }
            }
        }
        if (sMaxAge >= 0) {
            return (sMaxAge > 0) ? (QDateTime::currentSecsSinceEpoch() + sMaxAge) : 0;
        }
    }

    // Expires has no KnownHeaders enum; parse the raw HTTP-date with the C locale
    // (RFC 7231 IMF-fixdate), interpreted as UTC. Qt::RFC2822Date won't accept the
    // trailing "GMT", so it can't be used here.
    const QByteArray expires = reply->rawHeader(QByteArrayLiteral("Expires"));
    if (!expires.isEmpty()) {
        QDateTime expiresDt =
            QLocale::c().toDateTime(QString::fromLatin1(expires), QStringLiteral("ddd, dd MMM yyyy HH:mm:ss 'GMT'"));
        if (expiresDt.isValid()) {
            expiresDt.setTimeZone(QTimeZone::UTC);
            return expiresDt.toSecsSinceEpoch();
        }
    }

    // No server-provided freshness lifetime: default to a conservative 7 days so
    // undated tiles still get revalidated rather than cached forever.
    if (hasExplicitExpiry) {
        *hasExplicitExpiry = false;
    }
    return QDateTime::currentSecsSinceEpoch() + kDefaultTileExpirySecs;
}

bool QGeoTileFetcherQGC::_parseMustRevalidate(const QNetworkReply* reply)
{
    // no-cache / must-revalidate => cache but always revalidate before serving.
    // no-store => caller should not persist at all (treated as must-revalidate
    // here so a stored copy is never trusted without a fresh check).
    const QByteArray cacheControl = reply->rawHeader(QByteArrayLiteral("Cache-Control"));
    if (cacheControl.isEmpty()) {
        return false;
    }
    for (const QByteArray& directive : cacheControl.split(',')) {
        const QByteArray trimmed = directive.trimmed();
        if (trimmed.startsWith("no-cache") || trimmed.startsWith("no-store") || trimmed.startsWith("must-revalidate")) {
            return true;
        }
    }
    return false;
}

QGeoTileFetcherQGC::FetchResult QGeoTileFetcherQGC::snapshot(QNetworkReply* reply)
{
    FetchResult r;
    r.error = reply->error();
    r.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    r.errorString = reply->errorString();
    r.isOpen = reply->isOpen();
    r.etag = reply->rawHeader(QByteArrayLiteral("ETag"));
    // Round-trip Last-Modified as the canonical IMF-fixdate string for caching and
    // for re-sending as If-Modified-Since. Qt parses it via the known-header enum.
    const QVariant lastModifiedVar = reply->header(QNetworkRequest::LastModifiedHeader);
    if (lastModifiedVar.isValid()) {
        const QDateTime lm = lastModifiedVar.toDateTime().toUTC();
        r.lastModified = QLocale::c().toString(lm, QStringLiteral("ddd, dd MMM yyyy HH:mm:ss 'GMT'")).toLatin1();
    } else {
        r.lastModified = reply->rawHeader(QByteArrayLiteral("Last-Modified"));
    }
    r.expiresAt = _parseExpiry(reply, &r.hasExplicitExpiry);
    r.mustRevalidate = _parseMustRevalidate(reply);
    r.retryAfter = QGCNetworkHelper::retryAfterFromReply(reply);
    if (r.error == QNetworkReply::NoError) {
        r.body = reply->readAll();
    }
    return r;
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec& spec)
{
    if ((spec.zoom() < 1) || (spec.zoom() > QGC_MAX_MAP_ZOOM)) {
        return nullptr;
    }

    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(spec.mapId());
    if (!provider) {
        return nullptr;
    }

    /*if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        return nullptr;
    }*/

    const QNetworkRequest request = getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom());
    if (request.url().isEmpty()) {
        return nullptr;
    }

    QGeoTiledMapReplyQGC* tileImage = new QGeoTiledMapReplyQGC(_sharedNetworkManager(), request, spec, nullptr, this);
    if (!tileImage->init()) {
        tileImage->deleteLater();
        return nullptr;
    }

    return tileImage;
}

bool QGeoTileFetcherQGC::initialized() const
{
    return true;
}

bool QGeoTileFetcherQGC::fetchingEnabled() const
{
    return initialized();
}

void QGeoTileFetcherQGC::timerEvent(QTimerEvent* event)
{
    QGeoTileFetcher::timerEvent(event);
}

void QGeoTileFetcherQGC::updateTileRequests(const QSet<QGeoTileSpec>& tilesAdded,
                                            const QSet<QGeoTileSpec>& tilesRemoved)
{
    // Reaches into Qt-private QGeoTileFetcherPrivate (queue_/timer_/queueMutex_) because
    // the base queue is plain FIFO with no priority hook. This depends on the private
    // layout of qgeotilefetcher_p_p.h and may need revisiting on Qt minor-version bumps.
    QGeoTileFetcherPrivate* const d = static_cast<QGeoTileFetcherPrivate*>(QObjectPrivate::get(this));

    QMutexLocker ml(&d->queueMutex_);

    // Pan/zoom: drop still-pending (queued, no reply yet) removed tiles. Replies
    // already created for removed tiles are deliberately left to finish into the
    // cache, so we do NOT touch d->invmap_ (that is the base cancelTileRequests
    // path, which aborts in-flight work).
    for (const QGeoTileSpec& spec : tilesRemoved) {
        d->queue_.removeAll(spec);
    }

    // Float newly-visible tiles ahead of any prefetch/low-priority backlog: the
    // base queue is plain FIFO, so prepend added tiles (skipping ones already
    // queued, preserving their existing position).
    int insertAt = 0;
    for (const QGeoTileSpec& spec : tilesAdded) {
        if (!d->queue_.contains(spec)) {
            d->queue_.insert(insertAt, spec);
            ++insertAt;
        }
    }

    if (d->enabled_ && initialized() && !d->queue_.isEmpty() && !d->timer_.isActive()) {
        d->timer_.start(0, this);
    }
}

void QGeoTileFetcherQGC::handleReply(QGeoTiledMapReply* reply, const QGeoTileSpec& spec)
{
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (!initialized()) {
        return;
    }

    if (reply->error() == QGeoTiledMapReply::NoError) {
        emit tileFinished(spec, reply->mapImageData(), reply->mapImageFormat());
    } else {
        const QString error = reply->errorString().isEmpty() ? tr("Unknown tile fetch error") : reply->errorString();
        emit tileError(spec, error);
    }
}

QNetworkRequest QGeoTileFetcherQGC::getNetworkRequest(int mapId, int x, int y, int zoom)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(mapId);
    if (!provider || (zoom < 1) || !provider->isValidTileCoordinate(x, y, zoom)) {
        return QNetworkRequest();
    }
    const CachedRequestTemplate tmpl = _templateForProvider(provider);
    return _requestFromTemplate(tmpl, x, y, zoom);
}

QNetworkRequest QGeoTileFetcherQGC::getNetworkRequest(QStringView providerType, int x, int y, int zoom)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(providerType);
    if (!provider || (zoom < 1) || !provider->isValidTileCoordinate(x, y, zoom)) {
        return QNetworkRequest();
    }
    const CachedRequestTemplate tmpl = _templateForProvider(provider);
    return _requestFromTemplate(tmpl, x, y, zoom);
}

QGeoTileFetcherQGC::CachedRequestTemplate QGeoTileFetcherQGC::_templateForProvider(const SharedMapProvider& provider)
{
    if (!provider) {
        return CachedRequestTemplate{};
    }

    const int mapId = provider->getMapId();
    {
        QMutexLocker lock(&s_templateMutex);
        const auto it = s_templateCache.constFind(mapId);
        if (it != s_templateCache.constEnd()) {
            return *it;
        }
    }

    // Build outside the lock: getTileURL(0,0,0) only seeds the host/scheme so the
    // template carries provider state; per-tile URLs replace it in _requestFromTemplate.
    CachedRequestTemplate tmpl;
    tmpl.provider = provider;
    tmpl.request = _buildTileRequest(provider, 0, 0, 0);

    QMutexLocker lock(&s_templateMutex);
    // First writer wins; a racing builder produced an identical template.
    return *s_templateCache.insert(mapId, tmpl);
}

QNetworkRequest QGeoTileFetcherQGC::_requestFromTemplate(const CachedRequestTemplate& tmpl, int x, int y, int zoom)
{
    if (!tmpl.provider) {
        return QNetworkRequest();
    }
    QNetworkRequest request = tmpl.request;
    request.setUrl(tmpl.provider->getTileURL(x, y, zoom));
    return request;
}

QNetworkRequest QGeoTileFetcherQGC::_buildTileRequest(const SharedMapProvider& mapProvider, int x, int y, int zoom)
{
    if (!mapProvider) {
        return QNetworkRequest();
    }

    QNetworkRequest request;
    request.setUrl(mapProvider->getTileURL(x, y, zoom));
    request.setPriority(QNetworkRequest::NormalPriority);
    // Timeout is owned by the fetcher's leader-GET path (kLeaderTransferTimeout),
    // not set here, so there is a single source of truth.
    // request.setOriginatingObject(this);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    // OSM tile-usage policy requires an app-identifiable User-Agent. Commercial
    // providers keep the browser-style UA for compatibility.
    if (mapProvider->isOSMProvider()) {
        request.setHeader(QNetworkRequest::UserAgentHeader, QGCNetworkHelper::defaultUserAgent());
    } else if (!s_userAgentOverride.isEmpty()) {
        request.setHeader(QNetworkRequest::UserAgentHeader, s_userAgentOverride);
    } else {
        request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(s_userAgent));
    }
    const QByteArray referrer = mapProvider->getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = mapProvider->getToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("keep-alive"));
    // Don't set Accept-Encoding: a manual value disables QNAM's automatic
    // response decompression, so tiles would land in the cache still compressed.
    // Never send Cache-Control: no-cache on the request (OSM etiquette); revalidation
    // is driven by conditional headers in QGeoTiledMapReplyQGC instead.

    // Attributes. No QNetworkDiskCache is attached to the QNAM, so cache-load /
    // cache-save control would be no-ops; cache policy lives in QGeoFileTileCacheQGC.
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    // request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

    return request;
}
