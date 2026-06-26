#pragma once

#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <memory>

#include <QtCore/QSharedPointer>

#include <QtCore/QElapsedTimer>

#include "QGCMapTaskBase.h"
#include "QGeoTileFetcherQGC.h"

struct QGCCacheTile;
class QGeoTileFetcherQGC;
class QNetworkAccessManager;
class QSslError;
class QUrl;

class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT

    // Delivers coalesced fetch results straight into the reply.
    friend class QGeoTileFetcherQGC;

public:
    explicit QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QNetworkRequest& request,
                                  const QGeoTileSpec& spec, QObject* parent = nullptr, QGeoTileFetcherQGC* fetcher = nullptr);
    ~QGeoTiledMapReplyQGC();

    bool init();
    void abort() final;

private slots:
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _networkReplySslErrors(const QList<QSslError>& errors);
    void _cacheReply(QSharedPointer<QGCCacheTile> tile);
    void _cacheError(QGCMapTask::TaskType type, QStringView errorString);
    void _handleNetworkResult(const QGeoTileFetcherQGC::FetchResult& result);
    void _fallbackTileFetched(const QByteArray& image, const QString& format, int levelDelta);
    void _fallbackTileError(QGCMapTask::TaskType type, QStringView errorString);

private:
    static void _initDataFromResources();
    void _startNetworkRequest();
    // Qt::UniqueConnection needs a pointer-to-member slot, not a lambda.
    void _cancelDedupFetch();
    bool _scheduleRetryIfNeeded(const QNetworkReply* reply, QNetworkReply::NetworkError error, int statusCode);
    bool _scheduleRetryIfNeeded(QNetworkReply::NetworkError error, int statusCode,
                                std::optional<std::chrono::milliseconds> retryAfter = std::nullopt);

    static constexpr int kMaxRetryAttempts = 3;

    void _applyConditionalHeaders();
    void _serveCachedTile();
    // R6 lower-zoom fallback: on a terminal network/offline failure, enqueue a
    // worker task that scans cached ancestor tiles. On a hit the scaled ancestor is
    // served as a placeholder; on a miss the original error is raised. Attempted at
    // most once per reply to avoid loops.
    void _serveAncestorFallbackOr(QGeoTiledMapReply::Error err, const QString& errStr);
    // Stale-while-revalidate (R5): fire a detached conditional GET that updates the
    // SQLite cache in the background. The reply is single-shot (already finished with
    // stale bytes), so the fresh tile is picked up on the next viewport pass.
    void _backgroundRevalidate();

    QGeoTileFetcherQGC* _fetcher = nullptr;
    QNetworkAccessManager* _networkManager = nullptr;
    QNetworkRequest _request;
    bool m_initialized = false;
    bool _fallbackAttempted = false;
    int _retryAttempt = 0;

    // Original terminal error, raised if the ancestor-fallback lookup misses.
    QGeoTiledMapReply::Error _fallbackPendingError = QGeoTiledMapReply::NoError;
    QString _fallbackPendingErrorString;

    // Validation metadata for the stale cached tile we are revalidating. When
    // _cachedTile is non-null the body is reused on a 304 Not Modified and the
    // etag/lastModified are sent as conditional request headers.
    QSharedPointer<QGCCacheTile> _cachedTile;
    QByteArray _etag;
    QByteArray _lastModified;
    QElapsedTimer _fetchTimer;

    static QByteArray _bingNoTileImage;
    static QByteArray _badTile;
};
