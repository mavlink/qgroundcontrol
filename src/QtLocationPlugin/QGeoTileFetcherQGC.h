#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringView>
#include <QtCore/QUrl>
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <chrono>
#include <memory>
#include <optional>

class MapProvider;
class QGeoTiledMappingManagerEngineQGC;
class QGeoTiledMapReplyQGC;
class QGeoTileSpec;
class QNetworkAccessManager;

class QGeoTileFetcherQGC : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, const QVariantMap& parameters,
                                QGeoTiledMappingManagerEngineQGC* parent = nullptr);
    ~QGeoTileFetcherQGC();

    static QNetworkRequest getNetworkRequest(int mapId, int x, int y, int zoom);
    // Overload for the bulk-download path, which already holds the stable provider
    // name; avoids the name -> mapId -> name registry round-trip of the int overload.
    static QNetworkRequest getNetworkRequest(QStringView providerType, int x, int y, int zoom);
    // QNAM runs at most 6 HTTP requests in parallel per host:port; scale down on
    // metered/high-latency transports (cellular, bluetooth). Falls back to the
    // default when no QNetworkInformation backend is loaded (e.g. unit tests).
    static uint32_t concurrentDownloads(const QString& type);

    static constexpr uint32_t kDefaultConcurrent = 6;
    static constexpr uint32_t kConstrainedConcurrent = 2;

    // Optional per-provider UA override (from the "useragent" plugin parameter).
    // Applied only to non-OSM providers; OSM always uses the policy-compliant UA.
    static void setUserAgentOverride(const QByteArray& userAgent);

    // Completed-fetch snapshot shared with every reply coalesced onto a single
    // network request. Copied out of the leader's QNetworkReply so followers,
    // which never see the live reply, can run their own post-processing.
    struct FetchResult
    {
        QNetworkReply::NetworkError error = QNetworkReply::NoError;
        int statusCode = 0;
        QString reasonPhrase;
        QString errorString;
        QByteArray body;
        QByteArray etag;
        QByteArray lastModified;
        qint64 expiresAt = 0;
        // expiresAt came from a server freshness header, not the 7-day fallback; lets a bare 304 keep its cached expiry.
        bool hasExplicitExpiry = false;
        // Response carried Cache-Control no-cache/no-store/must-revalidate:
        // a stored copy must be revalidated before it is served.
        bool mustRevalidate = false;
        bool isOpen = false;
        // Parsed Retry-After of the leader reply, so coalesced followers (which
        // never see the live reply) can still honour rate-limit backoff.
        std::optional<std::chrono::milliseconds> retryAfter;
    };

    // Coalesce identical in-flight fetches. The fetcher owns a single QNetworkReply
    // per {url + conditional headers}; the first caller triggers the GET, later
    // header-identical callers attach. Requests with differing If-None-Match /
    // If-Modified-Since are NOT merged (a follower without a cached body could not
    // service a 304). Each follower is delivered its result individually. Must run
    // on the fetcher's thread. requester must be a QGeoTiledMapReplyQGC.
    void dedupFetch(QGeoTiledMapReplyQGC* requester, const QNetworkRequest& request);
    // Drop a still-pending follower (e.g. on abort). If it was the last follower,
    // the leader reply is aborted and the in-flight entry erased.
    void cancelFetch(QGeoTiledMapReplyQGC* requester, const QNetworkRequest& request);

#ifdef QGC_UNITTEST_BUILD
    // Test-only: number of distinct in-flight (coalesced) fetch entries.
    int inFlightUrlCount() const { return m_inFlight.size(); }

    // Followers across all entries whose key URL matches (ignores headers).
    int followerCount(const QUrl& url) const
    {
        int n = 0;
        for (auto it = m_inFlight.constBegin(); it != m_inFlight.constEnd(); ++it) {
            if (it.key().url == url) {
                n += it->followers.size();
            }
        }
        return n;
    }
#endif

    // Capture a completed reply into a thread-detached snapshot (also used by the
    // non-coalesced reply path).
    static FetchResult snapshot(QNetworkReply* reply);

#ifdef QGC_UNITTEST_BUILD
    // Test-only: leader GETs currently outstanding against the network.
    int activeNetworkFetches() const { return m_activeNetworkFetches; }

    // Test-only: high-water mark of concurrent leader GETs.
    int peakConcurrentFetches() const { return m_peakConcurrentFetches; }

    void setConcurrencyBudgetForTest(uint32_t budget) { m_concurrencyBudget = budget; }
#endif

public slots:
    // Re-ordered drain over the base FIFO: drop still-pending removed tiles and
    // float newly-visible added tiles ahead of the prefetch backlog (the base
    // queue is plain FIFO, so the priority split lives here). In-flight replies
    // for removed tiles are left to finish into the cache.
    void updateTileRequests(const QSet<QGeoTileSpec>& tilesAdded, const QSet<QGeoTileSpec>& tilesRemoved);

private:
    static QNetworkRequest _buildTileRequest(const std::shared_ptr<const MapProvider>& mapProvider, int x, int y,
                                             int zoom);

    // P2: provider lookup + static header set are identical for every tile of a
    // given mapId, so resolve them once and cache a header-applied request
    // template; per tile only the URL is swapped. getNetworkRequest() runs on
    // both the live (engine) and bulk (main) threads, so the cache is guarded.
    struct CachedRequestTemplate
    {
        std::shared_ptr<const MapProvider> provider;
        QNetworkRequest request;
    };

    static QNetworkRequest _requestFromTemplate(const CachedRequestTemplate& tmpl, int x, int y, int zoom);
    // Resolve + memoize the template for a provider (keyed by its stable mapId).
    // Returns a copy taken under the cache lock so callers never touch a shared
    // entry that another thread could mutate.
    static CachedRequestTemplate _templateForProvider(const std::shared_ptr<const MapProvider>& provider);

    QGeoTiledMapReply* getTileImage(const QGeoTileSpec& spec) final;
    bool initialized() const final;
    bool fetchingEnabled() const final;
    void timerEvent(QTimerEvent* event) final;
    void handleReply(QGeoTiledMapReply* reply, const QGeoTileSpec& spec) final;

    // Single QNAM shared by every reply this fetcher spawns. Lives on the
    // fetcher's thread, so replies (created on the same thread) reuse it without
    // any cross-thread access. Falls back to a fetcher-owned instance when no
    // manager was injected.
    QNetworkAccessManager* _sharedNetworkManager();

    // In-flight requests merge only when url AND both conditional headers match.
    struct FetchKey
    {
        QUrl url;
        QByteArray ifNoneMatch;
        QByteArray ifModifiedSince;

        bool operator==(const FetchKey& o) const
        {
            return (url == o.url) && (ifNoneMatch == o.ifNoneMatch) && (ifModifiedSince == o.ifModifiedSince);
        }
    };

    friend size_t qHash(const QGeoTileFetcherQGC::FetchKey& k, size_t seed) noexcept;

    static FetchKey _keyFor(const QNetworkRequest& request);
    void _onLeaderFinished(const FetchKey& key, QNetworkReply* reply);
    void _eraseEntry(const FetchKey& key, QNetworkReply* reply);
    // Concurrency gate: start a leader GET now if under budget, else park the
    // request so a finishing leader can pull it in (_startNextPending).
    void _startLeaderGet(const FetchKey& key, const QNetworkRequest& request, bool isOSM, const QString& host);
    // OSM tile-usage policy asks clients to stay at/under 2 concurrent connections;
    // cap OSM leaders independently of (and below) the transport budget.
    bool _canStart(bool isOSM, const QString& host) const;
    void _onFetchFinished(bool wasOSM, const QString& host);
    void _startNextPending();
    uint32_t _currentBudget() const;
    static qint64 _parseExpiry(const QNetworkReply* reply, bool *hasExplicitExpiry = nullptr);
    static bool _parseMustRevalidate(const QNetworkReply* reply);

    struct InFlight
    {
        QNetworkReply* reply = nullptr;
        QList<QPointer<QGeoTiledMapReplyQGC>> followers;
        bool isOSM = false;
        // Host the shared concurrency slot was claimed against; released on
        // completion. Empty until a leader GET actually starts.
        QString host;
    };

    QNetworkAccessManager* m_networkManager = nullptr;
    QNetworkAccessManager* m_ownedNetworkManager = nullptr;
    QHash<FetchKey, InFlight> m_inFlight;

    // Leader GETs currently outstanding; capped at m_concurrencyBudget. Followers
    // coalesced onto a leader do not count. Requests over budget wait in
    // m_pendingFetches (FIFO, fed in drain order so viewport priority carries).
    int m_activeNetworkFetches = 0;
    int m_activeOSMFetches = 0;
    int m_peakConcurrentFetches = 0;
    uint32_t m_concurrencyBudget = kDefaultConcurrent;

    struct PendingFetch
    {
        FetchKey key;
        QNetworkRequest request;
        bool isOSM = false;
        QString host;
    };

    QList<PendingFetch> m_pendingFetches;

    // Single timeout authority for tile fetches (see getNetworkRequest comment).
    // Shortened under unit-test builds so UI tests fail fast against CI's dead network
    // rather than stacking 20s timeouts past the CTest watchdog.
#ifdef QGC_UNITTEST_BUILD
    static constexpr std::chrono::milliseconds kLeaderTransferTimeout = std::chrono::seconds(2);
#else
    static constexpr std::chrono::milliseconds kLeaderTransferTimeout = std::chrono::seconds(20);
#endif

#if defined Q_OS_MACOS
    static constexpr const char* s_userAgent =
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 14.5; rv:125.0) Gecko/20100101 Firefox/125.0";
#elif defined Q_OS_WIN
    static constexpr const char* s_userAgent =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:100.0) Gecko/20100101 Firefox/112.0";
#elif defined Q_OS_ANDROID
    static constexpr const char* s_userAgent = "Mozilla/5.0 (Android 13; Tablet; rv:68.0) Gecko/68.0 Firefox/112.0";
#elif defined Q_OS_LINUX
    // TODO: Detect Wayland vs X11
    static constexpr const char* s_userAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/112.0";
#else
    static constexpr const char* s_userAgent = "Qt Location based application";
#endif

    static QByteArray s_userAgentOverride;

    // mapId -> resolved provider + prebuilt header template (P2). Guarded by
    // s_templateMutex because getNetworkRequest is static and reached from the
    // live-fetch (engine) and bulk-download (main) threads.
    static QMutex s_templateMutex;
    static QHash<int, CachedRequestTemplate> s_templateCache;
};

Q_DECLARE_METATYPE(QGeoTileFetcherQGC::FetchResult)
