#pragma once

#include <QtLocation/private/qgeomaptype_p.h>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSet>
#include <QtCore/QCache>
#include <QtCore/QDateTime>
#include <QtCore/QQueue>
#include <QtCore/QMutex>

#include <functional>

#include "QGCTileSet.h"

Q_DECLARE_LOGGING_CATEGORY(MapProviderLog)

// Provider capabilities for feature detection
enum class ProviderCapability {
    Elevation,          // Provides elevation/terrain data
    Satellite,          // Provides satellite imagery
    Street,             // Provides street maps
    Hybrid,             // Provides hybrid (satellite + labels)
    RequiresToken,      // Requires API token/key
    SupportsHiDPI,      // Supports high-DPI tiles
    HasZoomLimits,      // Has specific zoom level restrictions
    HasBoundsLimits     // Has geographic boundary restrictions
};

// qgeomaptype_p.h
/*enum MapStyle {
    NoMap = 0,
    StreetMap,
    SatelliteMapDay,
    SatelliteMapNight,
    TerrainMap,
    HybridMap,
    TransitMap,
    GrayStreetMap,
    PedestrianMap,
    CarNavigationMap,
    CycleMap,
    CustomMap = 100
};*/

static constexpr int QGC_MAX_MAP_ZOOM = 23;
static constexpr quint32 QGC_AVERAGE_TILE_SIZE = 13652;
static constexpr int QGC_URL_CACHE_SIZE = 1000; // Number of URLs to cache

// Tile key for URL caching
struct TileKey {
    int x;
    int y;
    int zoom;

    bool operator==(const TileKey& other) const {
        return (x == other.x) && (y == other.y) && (zoom == other.zoom);
    }
};

// Hash function for TileKey - uses Qt's qHashMulti for better distribution
inline size_t qHash(const TileKey& key, size_t seed = 0) {
    return qHashMulti(seed, key.x, key.y, key.zoom);
}

// Provider health tracking
struct ProviderHealth {
    int successCount = 0;
    int failureCount = 0;
    QDateTime lastSuccess;
    QDateTime lastFailure;

    double successRate() const {
        const int total = successCount + failureCount;
        return (total > 0) ? (static_cast<double>(successCount) / total) : 1.0;
    }

    bool isHealthy() const {
        return (successRate() >= 0.5);
    }

    void recordSuccess() {
        successCount++;
        lastSuccess = QDateTime::currentDateTime();
    }

    void recordFailure() {
        failureCount++;
        lastFailure = QDateTime::currentDateTime();
    }
};

// Geographic bounds for regional providers
struct GeographicBounds {
    double minLat = -90.0;
    double maxLat = 90.0;
    double minLon = -180.0;
    double maxLon = 180.0;

    GeographicBounds() = default;

    GeographicBounds(double minLatitude, double maxLatitude, double minLongitude, double maxLongitude)
        : minLat(minLatitude), maxLat(maxLatitude), minLon(minLongitude), maxLon(maxLongitude) {}

    bool contains(double lat, double lon) const {
        return ((lat >= minLat) && (lat <= maxLat) && (lon >= minLon) && (lon <= maxLon));
    }

    bool isValid() const {
        return ((minLat >= -90.0) && (maxLat <= 90.0) && (minLon >= -180.0) && (maxLon <= 180.0) &&
                (minLat < maxLat) && (minLon < maxLon));
    }
};

// Provider metadata for UI and legal compliance
struct ProviderMetadata {
    QString name;
    QString description;
    QString attribution;        // Copyright/attribution text (e.g., "© Google 2024")
    QString termsOfServiceUrl;  // Link to provider's ToS
    int minZoom = 0;
    int maxZoom = QGC_MAX_MAP_ZOOM;
    bool hasBoundsLimits = false;
    GeographicBounds bounds;    // Geographic limits (if hasBoundsLimits = true)

    ProviderMetadata() = default;

    ProviderMetadata(const QString& n, const QString& d, const QString& a, const QString& tos = QString())
        : name(n), description(d), attribution(a), termsOfServiceUrl(tos) {}

    // Validate metadata at construction
    bool isValid() const {
        if (name.isEmpty() || description.isEmpty()) {
            return false;
        }
        if ((minZoom < 0) || (maxZoom > QGC_MAX_MAP_ZOOM) || (minZoom > maxZoom)) {
            return false;
        }
        if (hasBoundsLimits && !bounds.isValid()) {
            return false;
        }
        return true;
    }
};

// Rate limiting per provider (thread-safe)
class RateLimiter {
public:
    RateLimiter(int maxRequestsPerSecond = 10);

    bool canMakeRequest(const QString& providerName);
    void recordRequest(const QString& providerName);
    void reset(const QString& providerName);

private:
    const int _maxRequestsPerSecond;
    QMap<QString, QQueue<QDateTime>> _requestHistory;
    mutable QMutex _mutex;  // Thread-safe access
    static constexpr int kHistoryWindowMs = 1000; // 1 second window
};

// Concurrent request limiting per provider
class ConcurrentRequestLimiter {
public:
    ConcurrentRequestLimiter(int maxConcurrentRequests = 6);

    bool canStartRequest(const QString& providerName);
    void startRequest(const QString& providerName);
    void finishRequest(const QString& providerName);
    int activeRequests(const QString& providerName) const;
    void reset(const QString& providerName);

private:
    const int _maxConcurrentRequests;
    QMap<QString, int> _activeRequests;
    mutable QMutex _mutex;  // Thread-safe access
};

// Error categorization (needed by retry logic)
enum class TileErrorType {
    None,
    NetworkTimeout,
    InvalidToken,
    RateLimited,
    InvalidTile,
    ServerError,
    OutOfBounds,
    EmptyResponse
};

// Retry logic with exponential backoff
struct RetryPolicy {
    int maxRetries = 3;
    int initialDelayMs = 100;
    double backoffMultiplier = 5.0;  // 100ms → 500ms → 2500ms
    int maxDelayMs = 5000;

    // Determine if error type is retryable
    static bool isRetryable(TileErrorType errorType);

    // Calculate delay for retry attempt
    int getDelayForAttempt(int attemptNumber) const;
};

struct RetryState {
    int attemptCount = 0;
    QDateTime lastAttemptTime;
    TileErrorType lastErrorType = TileErrorType::None;
    QString tileKey;  // "x,y,zoom"

    RetryState() = default;
    RetryState(int x, int y, int zoom)
        : tileKey(QString("%1,%2,%3").arg(x).arg(y).arg(zoom)) {}

    bool shouldRetry(const RetryPolicy& policy) const;
    int getNextDelayMs(const RetryPolicy& policy) const;
};

class RetryManager {
public:
    RetryManager(const RetryPolicy& policy = RetryPolicy());

    bool shouldRetry(const QString& tileKey, TileErrorType errorType);
    void recordAttempt(const QString& tileKey, TileErrorType errorType);
    void recordSuccess(const QString& tileKey);
    int getRetryDelayMs(const QString& tileKey) const;
    void clear();

    // Statistics
    int totalRetries() const { return _totalRetries; }
    int successfulRetries() const { return _successfulRetries; }

private:
    RetryPolicy _policy;
    QMap<QString, RetryState> _retryStates;
    int _totalRetries = 0;
    int _successfulRetries = 0;
    static constexpr int kMaxTrackedTiles = 1000;
};

// Telemetry and metrics export
struct ProviderMetrics {
    QString providerName;
    quint64 totalRequests = 0;
    quint64 successfulRequests = 0;
    quint64 failedRequests = 0;
    quint64 retriedRequests = 0;
    quint64 cachedRequests = 0;
    quint64 bytesDownloaded = 0;
    double averageResponseTimeMs = 0.0;
    QDateTime firstRequestTime;
    QDateTime lastRequestTime;

    double successRate() const {
        return (totalRequests > 0) ? (static_cast<double>(successfulRequests) / totalRequests) : 0.0;
    }

    double cacheHitRate() const {
        return (totalRequests > 0) ? (static_cast<double>(cachedRequests) / totalRequests) : 0.0;
    }
};

class MetricsCollector {
public:
    void recordRequest(const QString& providerName, bool fromCache = false);
    void recordSuccess(const QString& providerName, quint64 bytes, double responseTimeMs);
    void recordFailure(const QString& providerName);
    void recordRetry(const QString& providerName);

    ProviderMetrics getMetrics(const QString& providerName) const;
    QList<ProviderMetrics> getAllMetrics() const;
    QVariantMap getMetricsForQML() const;
    QString getMetricsSummary() const;

    void reset();
    void reset(const QString& providerName);

    int providerCount() const;

private:
    bool canAddProvider(const QString& providerName) const;

    QMap<QString, ProviderMetrics> _metrics;
    mutable QMutex _mutex;

    static constexpr int kMaxTrackedProviders = 50;
};

// Tile validation for cache integrity
class TileValidator {
public:
    static bool isValidTile(const QByteArray& data, const QString& expectedFormat);
    static bool isExpired(const QDateTime& cacheTime, int maxAgeDays = 30);
    static bool hasValidSize(const QByteArray& data);

    // Enhanced format detection
    static QString detectImageFormat(const QByteArray& data);
    static bool isValidPNG(const QByteArray& data);
    static bool isValidJPEG(const QByteArray& data);
    static bool isValidGIF(const QByteArray& data);
    static bool isValidWebP(const QByteArray& data);
    static bool isValidAVIF(const QByteArray& data);
    static bool isValidBMP(const QByteArray& data);

private:
    static constexpr int kMinTileSize = 100;        // 100 bytes minimum
    static constexpr int kMaxTileSize = 5242880;    // 5 MB maximum
    static constexpr int kTypicalTileSize = 15000;  // ~15 KB typical
};

// Forward declaration for CoordinateTransformCache
class MapProvider;

// Coordinate transform cache for performance
class CoordinateTransformCache {
public:
    CoordinateTransformCache(int cacheSize = 500);

    int getCachedLongToTileX(double lon, int zoom, const MapProvider* provider);
    int getCachedLatToTileY(double lat, int zoom, const MapProvider* provider);
    void clear();

    size_t size() const { return _lonCache.size() + _latCache.size(); }

private:
    using CoordKey = QPair<qint64, int>; // (coord * 1000000, zoom)
    QCache<CoordKey, int> _lonCache;
    QCache<CoordKey, int> _latCache;
};

// Memory usage monitoring
struct CacheStats {
    size_t urlCacheSize = 0;
    size_t urlCacheMemoryBytes = 0;
    size_t coordinateCacheSize = 0;
    size_t coordinateCacheMemoryBytes = 0;

    size_t totalMemoryUsage() const {
        return urlCacheMemoryBytes + coordinateCacheMemoryBytes;
    }

    void logStats() const;
    bool isOverLimit(size_t maxBytes) const {
        return totalMemoryUsage() > maxBytes;
    }
};

// Error tracking and categorization
struct ErrorReport {
    TileErrorType type = TileErrorType::None;
    QString providerName;
    QDateTime timestamp;
    QString details;
    int x = 0;
    int y = 0;
    int zoom = 0;

    ErrorReport() = default;

    ErrorReport(TileErrorType t, const QString& provider, const QString& desc, int tileX = 0, int tileY = 0, int z = 0)
        : type(t), providerName(provider), timestamp(QDateTime::currentDateTime()),
          details(desc), x(tileX), y(tileY), zoom(z) {}
};

class ErrorTracker {
public:
    void recordError(const ErrorReport& error);
    QList<ErrorReport> getRecentErrors(int count = 100) const;
    QList<ErrorReport> getErrorsByType(TileErrorType type) const;
    QList<ErrorReport> getErrorsByProvider(const QString& providerName) const;

    int errorCountByType(TileErrorType type) const;
    void clear();

private:
    QList<ErrorReport> _errors;
    static constexpr int kMaxStoredErrors = 1000;
};

// Provider health dashboard
class ProviderHealthDashboard {
public:
    struct ProviderHealthInfo {
        QString providerName;
        ProviderHealth health;
        int errorCount;
        TileErrorType lastErrorType;
    };

    void updateProviderHealth(const QString& providerName, const ProviderHealth& health);
    void recordProviderError(const QString& providerName, TileErrorType errorType);

    QList<ProviderHealthInfo> getAllProviderHealth() const;
    ProviderHealthInfo getWorstProvider() const;
    ProviderHealthInfo getBestProvider() const;

    QVariantMap healthSummaryForQML() const;

private:
    QMap<QString, ProviderHealthInfo> _providerHealth;
};

// Attribution management for legal compliance
class AttributionManager {
public:
    struct AttributionInfo {
        QString providerName;
        QString attributionText;
        QString termsUrl;
        bool isRequired = true;
    };

    void registerAttribution(const QString& providerName, const QString& attribution, const QString& termsUrl = QString());
    void unregisterAttribution(const QString& providerName);

    QList<AttributionInfo> getAllAttributions() const;
    QString getCombinedAttribution(const QString& separator = " | ") const;
    QStringList getRequiredAttributions() const;

    bool hasAttribution(const QString& providerName) const;

private:
    QMap<QString, AttributionInfo> _attributions;
};

// Forward declarations
class MapProviderManager;

// RAII guard for automatic request tracking cleanup
class RequestGuard {
public:
    RequestGuard(ConcurrentRequestLimiter& limiter, const QString& providerName)
        : _limiter(limiter), _providerName(providerName), _started(false)
    {
        if (_limiter.canStartRequest(_providerName)) {
            _limiter.startRequest(_providerName);
            _started = true;
        }
    }

    ~RequestGuard() {
        if (_started) {
            _limiter.finishRequest(_providerName);
        }
    }

    bool isActive() const { return _started; }

    Q_DISABLE_COPY(RequestGuard)

private:
    ConcurrentRequestLimiter& _limiter;
    QString _providerName;
    bool _started;
};

// Manager class to hold all shared systems (eliminates static singleton anti-pattern)
class MapProviderManager {
public:
    MapProviderManager();
    ~MapProviderManager() = default;

    // Accessor methods for all managers
    RateLimiter& rateLimiter() { return _rateLimiter; }
    CoordinateTransformCache& coordinateCache() { return _coordCache; }
    ErrorTracker& errorTracker() { return _errorTracker; }
    ProviderHealthDashboard& healthDashboard() { return _healthDashboard; }
    AttributionManager& attributionManager() { return _attributionManager; }
    ConcurrentRequestLimiter& concurrentLimiter() { return _concurrentLimiter; }
    RetryManager& retryManager() { return _retryManager; }
    MetricsCollector& metricsCollector() { return _metricsCollector; }

    // Global singleton access (for backwards compatibility)
    static MapProviderManager* instance();

    Q_DISABLE_COPY(MapProviderManager)

private:
    RateLimiter _rateLimiter;
    CoordinateTransformCache _coordCache;
    ErrorTracker _errorTracker;
    ProviderHealthDashboard _healthDashboard;
    AttributionManager _attributionManager;
    ConcurrentRequestLimiter _concurrentLimiter;
    RetryManager _retryManager;
    MetricsCollector _metricsCollector;
};

// TODO: Inherit from QGeoMapType
class MapProvider
{
public:
    MapProvider(const QString &mapName, const QString &referrer, const QString &imageFormat, quint32 averageSize = QGC_AVERAGE_TILE_SIZE,
                QGeoMapType::MapStyle mapStyle = QGeoMapType::CustomMap);
    virtual ~MapProvider();

    QUrl getTileURL(int x, int y, int zoom) const;

    QString getImageFormat(QByteArrayView image) const;

    // TODO: Download Random Tile And Use That Size Instead?
    quint32 getAverageSize() const { return _averageSize; }

    QGeoMapType::MapStyle getMapStyle() const { return _mapStyle; }
    const QString& getMapName() const { return _mapName; }
    int getMapId() const { return _mapId; }
    const QString& getReferrer() const { return _referrer; }
    virtual QByteArray getToken() const { return QByteArray(); }

    virtual int long2tileX(double lon, int z) const;
    virtual int lat2tileY(double lat, int z) const;

    virtual bool isElevationProvider() const { return false; }
    virtual bool isBingProvider() const { return false; }

    // Capability system
    virtual QSet<ProviderCapability> capabilities() const { return QSet<ProviderCapability>(); }
    bool hasCapability(ProviderCapability capability) const;

    // Metadata system
    virtual ProviderMetadata metadata() const;

    // Rate limiting
    bool canMakeRequest() const;
    void recordRequest();

    virtual QGCTileSet getTileCount(int zoom, double topleftLon,
                                    double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

    // Coordinate validation
    static bool isValidLongitude(double lon);
    static bool isValidLatitude(double lat);
    static bool isValidZoom(int zoom);

    // Geographic bounds validation
    bool isWithinBounds(double lat, double lon) const;
    bool isValidTileCoordinate(int x, int y, int zoom) const;

    // Tile validation
    bool isValidTileData(const QByteArray& data) const;
    QString detectTileFormat(const QByteArray& data) const;

    // Token management helpers
    static QString getSettingsToken(const QString& tokenName, const QString& providerName);

    // URL caching
    QString getCachedURL(int x, int y, int zoom);
    void clearURLCache();

    // Health monitoring (thread-safe)
    ProviderHealth getHealth() const;
    void recordSuccess();
    void recordFailure();

    // Memory monitoring
    CacheStats getCacheStats() const;
    void clearCachesIfOverLimit(size_t maxBytes);

    // Error tracking
    void recordError(TileErrorType type, const QString& details, int x = 0, int y = 0, int zoom = 0);

    /**
     * Integration methods for complete tile fetching workflow with all Phase 5 features.
     *
     * Example usage in tile fetching code:
     *
     *   // Check if we can start a request
     *   if (!provider->canStartTileRequest()) {
     *       return; // Rate limited, concurrent limit reached, or provider unhealthy
     *   }
     *
     *   // Use RAII guard for automatic concurrent request tracking
     *   RequestGuard guard(MapProvider::concurrentLimiter(), provider->getMapName());
     *   if (!guard.isActive()) {
     *       return; // Could not start request
     *   }
     *
     *   // Begin request (records metrics)
     *   provider->beginTileRequest(x, y, zoom, fromCache);
     *
     *   // Fetch tile...
     *   QElapsedTimer timer;
     *   timer.start();
     *   QByteArray tileData = fetchTileFromNetwork(...);
     *   double responseTimeMs = timer.elapsed();
     *
     *   // Complete request (records metrics, health, handles retries)
     *   if (tileData.isEmpty()) {
     *       TileErrorType errorType = TileErrorType::EmptyResponse;
     *       provider->completeTileRequest(x, y, zoom, false, 0, responseTimeMs, errorType);
     *
     *       // Check if we should retry
     *       if (provider->shouldRetryTile(x, y, zoom, errorType)) {
     *           int delayMs = provider->getRetryDelay(x, y, zoom);
     *           QTimer::singleShot(delayMs, [=]() {
     *               retryTileFetch(provider, x, y, zoom);
     *           });
     *       }
     *   } else {
     *       provider->completeTileRequest(x, y, zoom, true, tileData.size(), responseTimeMs);
     *   }
     *   // guard automatically calls finishRequest() on destruction
     */
    bool canStartTileRequest();
    void beginTileRequest(int x, int y, int zoom, bool fromCache = false);
    void completeTileRequest(int x, int y, int zoom, bool success, quint64 bytes = 0, double responseTimeMs = 0.0, TileErrorType errorType = TileErrorType::None);
    bool shouldRetryTile(int x, int y, int zoom, TileErrorType errorType);
    int getRetryDelay(int x, int y, int zoom) const;

    // Global access to shared systems (via manager)
    static CoordinateTransformCache& coordinateCache() { return MapProviderManager::instance()->coordinateCache(); }
    static ErrorTracker& errorTracker() { return MapProviderManager::instance()->errorTracker(); }
    static ProviderHealthDashboard& healthDashboard() { return MapProviderManager::instance()->healthDashboard(); }
    static AttributionManager& attributionManager() { return MapProviderManager::instance()->attributionManager(); }
    static ConcurrentRequestLimiter& concurrentLimiter() { return MapProviderManager::instance()->concurrentLimiter(); }
    static RetryManager& retryManager() { return MapProviderManager::instance()->retryManager(); }
    static MetricsCollector& metricsCollector() { return MapProviderManager::instance()->metricsCollector(); }

protected:
    QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const;
    int _getServerNum(int x, int y, int max) const;

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const QGeoMapType::MapStyle _mapStyle;
    const QString _language;
    const int _mapId;

    // URL cache for performance (thread-safe with mutex)
    mutable QCache<TileKey, QString> _urlCache;
    mutable QMutex _urlCacheMutex;

    // Health tracking (thread-safe with mutex)
    mutable ProviderHealth _health;
    mutable QMutex _healthMutex;

private:
    static int _mapIdIndex;
};

// Provider factory for dynamic provider creation
class MapProviderFactory {
public:
    using ProviderCreator = std::function<MapProvider*()>;

    static MapProvider* create(const QString& providerType);
    static QStringList availableProviders();
    static bool registerProvider(const QString& type, ProviderCreator creator);

private:
    static QMap<QString, ProviderCreator>& registry();
};

// Helper macro for auto-registration
#define REGISTER_MAP_PROVIDER(type, className) \
    namespace { \
        static bool _registered_##className = \
            MapProviderFactory::registerProvider(type, []() -> MapProvider* { \
                return new className(); \
            }); \
    }

// Graceful degradation with fallback chain
class ProviderFallbackChain {
public:
    void setPrimaryProvider(MapProvider* primary);
    void addFallback(MapProvider* fallback);
    void clearFallbacks();

    QString getTileURL(int x, int y, int zoom);
    MapProvider* getHealthyProvider();

    const QList<MapProvider*>& providers() const { return _chain; }

private:
    QList<MapProvider*> _chain;
};

// Provider rotation strategies for load balancing
class ProviderRotationStrategy {
public:
    enum class Strategy {
        RoundRobin,         // Rotate through healthy providers evenly
        WeightedHealth,     // Prefer providers with better health scores
        LeastConnections,   // Use provider with fewest active connections
        Random              // Random selection from healthy providers
    };

    ProviderRotationStrategy(Strategy strategy = Strategy::RoundRobin);

    // Select next provider based on strategy
    MapProvider* selectProvider(const QList<MapProvider*>& providers);

    // Record provider usage (for round-robin tracking)
    void recordProviderUsage(MapProvider* provider);

    // Change strategy at runtime
    void setStrategy(Strategy strategy);
    Strategy getStrategy() const { return _strategy; }

private:
    MapProvider* selectRoundRobin(const QList<MapProvider*>& providers);
    MapProvider* selectWeightedHealth(const QList<MapProvider*>& providers);
    MapProvider* selectLeastConnections(const QList<MapProvider*>& providers);
    MapProvider* selectRandom(const QList<MapProvider*>& providers);

    Strategy _strategy;
    int _lastUsedIndex;
    QMap<MapProvider*, int> _usageCounts;
    mutable QMutex _mutex;
};

// Load balancer for automatic failover and distribution
class LoadBalancer {
public:
    struct ProviderConfig {
        MapProvider* provider;
        int weight;              // Relative weight for weighted strategies (default: 1)
        bool enabled;            // Can be disabled without removing

        ProviderConfig(MapProvider* p = nullptr, int w = 1, bool e = true)
            : provider(p), weight(w), enabled(e) {}
    };

    LoadBalancer(double minHealthThreshold = 0.5, bool autoFailover = true);

    // Provider management
    void addProvider(MapProvider* provider, int weight = 1);
    void removeProvider(MapProvider* provider);
    void enableProvider(MapProvider* provider, bool enable = true);
    void setProviderWeight(MapProvider* provider, int weight);

    // Provider selection
    MapProvider* getNextProvider();                    // Uses current strategy
    QList<MapProvider*> getHealthyProviders() const;   // All healthy & enabled providers
    QList<MapProvider*> getAllProviders() const;       // All providers (even unhealthy)

    // Configuration
    void setMinHealthThreshold(double threshold);
    void enableAutoFailover(bool enable);
    void setStrategy(ProviderRotationStrategy::Strategy strategy);

    // Statistics
    int totalProviders() const { return _providers.size(); }
    int healthyProviderCount() const;
    double averageHealthScore() const;
    QVariantMap getStatisticsForQML() const;

    // Provider info
    bool hasProvider(MapProvider* provider) const;
    int getProviderWeight(MapProvider* provider) const;
    bool isProviderEnabled(MapProvider* provider) const;

private:
    QList<ProviderConfig> _providers;
    ProviderRotationStrategy _rotationStrategy;
    double _minHealthThreshold;
    bool _autoFailover;
    mutable QMutex _mutex;
};
