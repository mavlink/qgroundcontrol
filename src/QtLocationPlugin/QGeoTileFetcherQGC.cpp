#include "QGeoTileFetcherQGC.h"

#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSettings>
#include <QtCore/QMutexLocker>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkRequest>

#include "MapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "TileMetricsManager.h"
#include "QGCTileLogger.h"
#include "TileQualitySettings.h"
#include "CircuitBreaker.h"

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "QtLocationPlugin.QGeoTileFetcherQGC")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager *networkManager, const QVariantMap &parameters, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
    , _loadBalancer(0.5, true)  // 50% health threshold, auto-failover enabled
{
    if (!networkManager) {
        qCCritical(QGeoTileFetcherQGCLog) << "Network manager is null";
    }

    // Initialize new components
    _bandwidthThrottler = new BandwidthThrottler(this);
    _geoRouter = new GeoRouter(this);
    _metricsManager = TileMetricsManager::instance();
    _logger = QGCTileLogger::instance();
    _qualitySettings = TileQualitySettings::instance();

    // Connect bandwidth throttler to metrics manager
    connect(_bandwidthThrottler, &BandwidthThrottler::throttlingStateChanged,
            this, [this](bool throttling) {
        _metricsManager->setThrottlingActive(throttling);
    });

    initializeLoadBalancer();

    qCDebug(QGeoTileFetcherQGCLog) << this << "Initialized with" << _loadBalancer.totalProviders() << "providers"
                                    << "Platform:" << (PlatformDetector::isMobile() ? "Mobile" : "Desktop");
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    // Save provider configuration on shutdown
    saveProviderConfiguration();

    qCDebug(QGeoTileFetcherQGCLog) << this;
}

void QGeoTileFetcherQGC::initializeLoadBalancer()
{
    // Get all available providers from UrlFactory
    const QList<SharedMapProvider>& providers = UrlFactory::getProviders();

    // Initialize LoadBalancer with providers and weights
    // Higher weight = higher priority for selection
    for (const SharedMapProvider& provider : providers) {
        if (!provider) {
            continue;
        }

        // P1-5: const_cast is safe here - MapProvider internal state (health, metrics)
        // is mutable and designed to be modified. The const in SharedMapProvider
        // refers to the provider's identity/configuration, not its runtime state.
        // All state-modifying methods use mutable members and are thread-safe.
        MapProvider* nonConstProvider = const_cast<MapProvider*>(provider.get());

        // Set weights based on provider type (can be customized based on preferences)
        int weight = 1;  // Default weight

        // Prioritize popular/reliable providers
        const QString& providerName = provider->getMapName();
        if (providerName.contains("Google", Qt::CaseInsensitive)) {
            weight = 3;  // Google maps get higher priority
        } else if (providerName.contains("OpenStreet", Qt::CaseInsensitive)) {
            weight = 2;  // OSM gets medium priority
        } else if (providerName.contains("Bing", Qt::CaseInsensitive)) {
            weight = 2;  // Bing gets medium priority
        }

        // Thread-safe: _providersByMapId will be protected by mutex during access
        QMutexLocker locker(&_providersMutex);
        _loadBalancer.addProvider(nonConstProvider, weight);
        _providersByMapId.insert(provider->getMapId(), nonConstProvider);

        // Create circuit breaker for this provider
        CircuitBreaker* breaker = new CircuitBreaker(this);
        breaker->setFailureThreshold(5);
        breaker->setTimeout(60000);  // 1 minute
        _circuitBreakers.insert(provider->getMapId(), breaker);

        locker.unlock();

        qCDebug(QGeoTileFetcherQGCLog) << "Added provider:" << providerName
                                       << "weight:" << weight
                                       << "mapId:" << provider->getMapId();
    }

    // Set load balancing strategy - WeightedHealth balances between provider health and weights
    _loadBalancer.setStrategy(ProviderRotationStrategy::Strategy::WeightedHealth);

    qCDebug(QGeoTileFetcherQGCLog) << "LoadBalancer initialized with"
                                   << _loadBalancer.totalProviders() << "providers,"
                                   << _loadBalancer.healthyProviderCount() << "healthy";

    // Load saved configuration
    loadProviderConfiguration();
}

void QGeoTileFetcherQGC::saveProviderConfiguration()
{
    QSettings settings;
    settings.beginGroup("MapTileLoadBalancer");

    // Save provider weights and enabled state
    settings.beginWriteArray("providers");
    int index = 0;
    for (auto it = _providersByMapId.constBegin(); it != _providersByMapId.constEnd(); ++it) {
        MapProvider* provider = it.value();
        if (!provider) {
            continue;
        }

        settings.setArrayIndex(index++);
        settings.setValue("mapId", it.key());
        settings.setValue("name", provider->getMapName());
        settings.setValue("weight", _loadBalancer.getProviderWeight(provider));
        settings.setValue("enabled", _loadBalancer.isProviderEnabled(provider));
    }
    settings.endArray();

    settings.endGroup();

    qCDebug(QGeoTileFetcherQGCLog) << "Saved configuration for" << index << "providers";
}

void QGeoTileFetcherQGC::loadProviderConfiguration()
{
    QSettings settings;
    settings.beginGroup("MapTileLoadBalancer");

    // Load provider weights and enabled state
    const int providersCount = settings.beginReadArray("providers");
    int loadedCount = 0;
    int skippedCount = 0;

    for (int i = 0; i < providersCount; ++i) {
        settings.setArrayIndex(i);

        // P1-7: Validate settings data to handle corruption
        const int mapId = settings.value("mapId", -1).toInt();
        if (mapId < 0) {
            qCWarning(QGeoTileFetcherQGCLog) << "Skipping invalid mapId in settings:" << mapId;
            skippedCount++;
            continue;
        }

        // Clamp weight to reasonable range (1-100)
        int weight = settings.value("weight", 1).toInt();
        if (weight < 1) {
            qCWarning(QGeoTileFetcherQGCLog) << "Invalid weight in settings:" << weight << "clamping to 1";
            weight = 1;
        } else if (weight > 100) {
            qCWarning(QGeoTileFetcherQGCLog) << "Invalid weight in settings:" << weight << "clamping to 100";
            weight = 100;
        }

        const bool enabled = settings.value("enabled", true).toBool();

        // Thread-safe provider lookup
        MapProvider* provider = nullptr;
        {
            QMutexLocker locker(&_providersMutex);
            provider = _providersByMapId.value(mapId, nullptr);
        }

        if (provider) {
            _loadBalancer.setProviderWeight(provider, weight);
            _loadBalancer.enableProvider(provider, enabled);
            loadedCount++;

            qCDebug(QGeoTileFetcherQGCLog) << "Loaded config for provider:"
                                           << provider->getMapName()
                                           << "weight:" << weight
                                           << "enabled:" << enabled;
        } else {
            qCWarning(QGeoTileFetcherQGCLog) << "Provider not found for mapId:" << mapId;
            skippedCount++;
        }
    }
    settings.endArray();

    settings.endGroup();

    if (loadedCount > 0) {
        qCDebug(QGeoTileFetcherQGCLog) << "Loaded configuration for" << loadedCount << "providers"
                                       << "(" << skippedCount << "skipped)";
    } else if (skippedCount > 0) {
        qCWarning(QGeoTileFetcherQGCLog) << "Failed to load any provider configuration,"
                                         << skippedCount << "entries skipped";
    }
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    // Thread-safe provider lookup
    MapProvider* provider = nullptr;
    {
        QMutexLocker locker(&_providersMutex);
        provider = _providersByMapId.value(spec.mapId(), nullptr);

        if (!provider) {
            // If specific provider not found, try to get from load balancer
            // LoadBalancer is internally thread-safe
            provider = _loadBalancer.getNextProvider();
        }
    } // Release mutex before expensive operations

    if (!provider) {
        const QString error = tr("No available map provider for mapId %1").arg(spec.mapId());
        qCWarning(QGeoTileFetcherQGCLog) << error;
        emit tileError(spec, error);
        // P1-6: Returning nullptr is acceptable - QGeoTileFetcher handles it appropriately
        return nullptr;
    }

    // Check if provider can start this tile request (rate limiting, concurrent limiting, health)
    if (!provider->canStartTileRequest()) {
        qCDebug(QGeoTileFetcherQGCLog) << "Provider" << provider->getMapName()
                                       << "cannot start request for tile"
                                       << spec.x() << spec.y() << spec.zoom()
                                       << "(rate limited, concurrent limit, or unhealthy)";

        // Try to get another provider from load balancer (thread-safe internally)
        provider = _loadBalancer.getNextProvider();
        if (!provider || !provider->canStartTileRequest()) {
            const QString error = tr("All map providers are unavailable");
            qCWarning(QGeoTileFetcherQGCLog) << error;
            emit tileError(spec, error);
            // P1-6: Returning nullptr is acceptable - QGeoTileFetcher handles it
            return nullptr;
        }
    }

    // TODO: Re-enable zoom level check once all providers have correct min/max zoom levels set
    /*if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        emit tileError(spec, "Zoom level out of range");
        return nullptr;
    }*/

    // Store mapId for later lookup (P0-1 fix: avoid storing raw pointer)
    const int providerMapId = provider->getMapId();

    // Use RAII RequestGuard for automatic concurrent request tracking (P0-4 fix)
    // Note: RequestGuard automatically calls finishRequest() in destructor
    RequestGuard guard(MapProvider::concurrentLimiter(), provider->getMapName());
    if (!guard.isActive()) {
        const QString error = tr("Concurrent request limit reached for provider %1").arg(provider->getMapName());
        qCWarning(QGeoTileFetcherQGCLog) << error;
        emit tileError(spec, error);
        return nullptr;
    }

    // Begin tile request (records metrics, starts timer)
    const bool fromCache = false;  // Will be determined by QGeoTiledMapReplyQGC
    provider->beginTileRequest(spec.x(), spec.y(), spec.zoom(), fromCache);

    // Build network request
    const QNetworkRequest request = getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom(), provider);
    if (!request.url().isValid() || request.url().isEmpty()) {
        const QString error = tr("Map provider returned an invalid URL");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "provider:" << provider->getMapName()
                                         << "url:" << request.url();
        provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, 0, 0.0, TileErrorType::InvalidTile);
        emit tileError(spec, error);
        return nullptr;
    }

    // Create tile reply with provider context for completion tracking
    QGeoTiledMapReplyQGC *tileReply = new QGeoTiledMapReplyQGC(m_networkManager, request, spec);

    // Store mapId instead of raw pointer (P0-1 fix: safer lifecycle management)
    tileReply->setProperty("providerMapId", providerMapId);
    tileReply->setProperty("requestStartTime", QDateTime::currentMSecsSinceEpoch());

    if (!tileReply->init()) {
        const QString error = tr("Failed to start tile request");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "provider:" << provider->getMapName()
                                         << "x:" << spec.x() << "y:" << spec.y()
                                         << "zoom:" << spec.zoom();
        provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, 0, 0.0, TileErrorType::NetworkTimeout);
        tileReply->deleteLater();
        emit tileError(spec, error);
        return nullptr;
    }

    qCDebug(QGeoTileFetcherQGCLog) << "Fetching tile via" << provider->getMapName()
                                   << "x:" << spec.x() << "y:" << spec.y() << "zoom:" << spec.zoom();

    return tileReply;
}

bool QGeoTileFetcherQGC::initialized() const
{
    return (m_networkManager != nullptr);
}

bool QGeoTileFetcherQGC::fetchingEnabled() const
{
    return initialized();
}

void QGeoTileFetcherQGC::timerEvent(QTimerEvent *event)
{
    QGeoTileFetcher::timerEvent(event);
}

void QGeoTileFetcherQGC::handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec)
{
    if (!reply) {
        const QString error = tr("Invalid tile reply");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "x:" << spec.x() << "y:" << spec.y()
                                         << "zoom:" << spec.zoom();
        emit tileError(spec, error);
        return;
    }

    // Extract provider by mapId (P0-1 fix: safe lookup instead of raw pointer)
    const int providerMapId = reply->property("providerMapId").toInt();
    const qint64 startTime = reply->property("requestStartTime").toLongLong();
    const double responseTimeMs = startTime > 0 ? (QDateTime::currentMSecsSinceEpoch() - startTime) : 0.0;

    // Thread-safe provider lookup
    MapProvider* provider = nullptr;
    {
        QMutexLocker locker(&_providersMutex);
        provider = _providersByMapId.value(providerMapId, nullptr);
    }

    if (!provider) {
        qCWarning(QGeoTileFetcherQGCLog) << "Provider no longer available for mapId:" << providerMapId
                                         << "tile:" << spec.x() << spec.y() << spec.zoom();
        reply->deleteLater();
        // Don't emit error - provider was removed, request is no longer valid
        return;
    }

    reply->deleteLater();

    if (!initialized()) {
        const QString error = tr("Tile fetcher is not initialized");
        qCWarning(QGeoTileFetcherQGCLog) << error;
        if (provider) {
            provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, 0, responseTimeMs, TileErrorType::ServerError);
        }
        emit tileError(spec, error);
        return;
    }

    if (reply->error() == QGeoTiledMapReply::NoError) {
        const QByteArray bytes = reply->mapImageData();
        const QString format = reply->mapImageFormat();

        // Complete tile request successfully
        if (provider) {
            provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), true, bytes.size(), responseTimeMs);
            qCDebug(QGeoTileFetcherQGCLog) << "Tile success via" << provider->getMapName()
                                           << "x:" << spec.x() << "y:" << spec.y() << "zoom:" << spec.zoom()
                                           << "size:" << bytes.size() << "bytes"
                                           << "time:" << responseTimeMs << "ms";
        }

        emit tileFinished(spec, bytes, format);
    } else {
        const QString errorString = reply->errorString().isEmpty()
            ? tr("Unknown tile fetch error")
            : reply->errorString();

        // Determine error type based on reply error
        TileErrorType errorType = TileErrorType::ServerError;
        switch (reply->error()) {
            case QGeoTiledMapReply::CommunicationError:
                errorType = TileErrorType::NetworkTimeout;
                break;
            case QGeoTiledMapReply::UnknownError:
            default:
                errorType = TileErrorType::ServerError;
                break;
        }

        // Complete tile request with error
        if (provider) {
            const quint64 bytes = reply->mapImageData().size();
            provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, bytes, responseTimeMs, errorType);

            qCDebug(QGeoTileFetcherQGCLog) << "Tile error via" << provider->getMapName()
                                           << "x:" << spec.x() << "y:" << spec.y() << "zoom:" << spec.zoom()
                                           << "error:" << errorString
                                           << "time:" << responseTimeMs << "ms";

            // Check if we should retry
            if (provider->shouldRetryTile(spec.x(), spec.y(), spec.zoom(), errorType)) {
                const int delayMs = provider->getRetryDelay(spec.x(), spec.y(), spec.zoom());
                qCDebug(QGeoTileFetcherQGCLog) << "Scheduling retry for tile"
                                               << spec.x() << spec.y() << spec.zoom()
                                               << "in" << delayMs << "ms";

                // Capture mapId instead of raw pointer (P0-3 fix: avoid dangling pointer)
                const int providerMapId = provider->getMapId();
                QTimer::singleShot(delayMs, this, [this, providerMapId, spec]() {
                    // Look up provider safely
                    MapProvider* retryProvider = nullptr;
                    {
                        QMutexLocker locker(&_providersMutex);
                        retryProvider = _providersByMapId.value(providerMapId, nullptr);
                    }

                    if (!retryProvider) {
                        qCWarning(QGeoTileFetcherQGCLog) << "Retry failed: provider removed for mapId:" << providerMapId;
                        emit tileError(spec, tr("Retry failed: provider unavailable"));
                        return;
                    }

                    retryTileFetch(retryProvider, spec);
                });
                return;  // Don't emit error yet, we're retrying
            }
        }

        emit tileError(spec, errorString);
    }
}

void QGeoTileFetcherQGC::retryTileFetch(MapProvider* provider, const QGeoTileSpec &spec)
{
    if (!provider) {
        qCWarning(QGeoTileFetcherQGCLog) << "Cannot retry tile: provider is null";
        emit tileError(spec, tr("Retry failed: provider is null"));
        return;
    }

    qCDebug(QGeoTileFetcherQGCLog) << "Retrying tile fetch via" << provider->getMapName()
                                   << "x:" << spec.x() << "y:" << spec.y() << "zoom:" << spec.zoom();

    // Check if provider can still make requests
    if (!provider->canStartTileRequest()) {
        qCWarning(QGeoTileFetcherQGCLog) << "Cannot retry: provider" << provider->getMapName()
                                         << "cannot start request";
        const QString error = tr("Retry failed: provider unavailable");
        emit tileError(spec, error);
        return;
    }

    // Store mapId for completion tracking
    const int providerMapId = provider->getMapId();

    // Use RAII RequestGuard (P0-4)
    RequestGuard guard(MapProvider::concurrentLimiter(), provider->getMapName());
    if (!guard.isActive()) {
        const QString error = tr("Retry failed: concurrent limit reached");
        qCWarning(QGeoTileFetcherQGCLog) << error;
        emit tileError(spec, error);
        return;
    }

    // Begin new request
    provider->beginTileRequest(spec.x(), spec.y(), spec.zoom(), false);

    // Build network request
    const QNetworkRequest request = getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom(), provider);
    if (!request.url().isValid() || request.url().isEmpty()) {
        const QString error = tr("Retry failed: invalid URL");
        qCWarning(QGeoTileFetcherQGCLog) << error;
        provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, 0, 0.0, TileErrorType::InvalidTile);
        emit tileError(spec, error);
        return;
    }

    // Create new tile reply
    QGeoTiledMapReplyQGC *tileReply = new QGeoTiledMapReplyQGC(m_networkManager, request, spec);

    // Store mapId instead of raw pointer (P0-1 fix)
    tileReply->setProperty("providerMapId", providerMapId);
    tileReply->setProperty("requestStartTime", QDateTime::currentMSecsSinceEpoch());

    if (!tileReply->init()) {
        const QString error = tr("Retry failed: could not initialize request");
        qCWarning(QGeoTileFetcherQGCLog) << error;
        provider->completeTileRequest(spec.x(), spec.y(), spec.zoom(), false, 0, 0.0, TileErrorType::NetworkTimeout);
        tileReply->deleteLater();
        emit tileError(spec, error);
        return;
    }

    // P0-3 FIX: Connect signals so reply is properly handled
    // This is critical - without this connection, the reply leaks memory
    // and handleReply() is never called
    connect(tileReply, &QGeoTiledMapReply::finished,
            this, [this, tileReply, spec]() {
        handleReply(tileReply, spec);
    }, Qt::QueuedConnection);

    qCDebug(QGeoTileFetcherQGCLog) << "Retry request initiated for tile"
                                   << spec.x() << spec.y() << spec.zoom();
}

QNetworkRequest QGeoTileFetcherQGC::getNetworkRequest(int mapId, int x, int y, int zoom, MapProvider* provider)
{
    // Use provided provider or look up from mapId
    const SharedMapProvider mapProvider = provider
        ? UrlFactory::getMapProviderFromQtMapId(provider->getMapId())
        : UrlFactory::getMapProviderFromQtMapId(mapId);

    if (!mapProvider) {
        return QNetworkRequest();
    }

    QNetworkRequest request;
    request.setUrl(mapProvider->getTileURL(x, y, zoom));
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(kNetworkRequestTimeoutMs);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, s_userAgent);
    const QByteArray referrer = mapProvider->getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = mapProvider->getToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("keep-alive"));
    request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate, br"));

    // Attributes
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

    return request;
}
