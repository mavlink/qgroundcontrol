#include "MapProvider.h"
#include <QGCLoggingCategory.h>
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QLocale>
#include <QtCore/QRandomGenerator>
#include <cmath>

QGC_LOGGING_CATEGORY(MapProviderLog, "QtLocationPlugin.MapProvider")

// QtLocation expects MapIds to start at 1 and be sequential.
int MapProvider::_mapIdIndex = 1;

// MapProviderManager implementation
MapProviderManager::MapProviderManager()
    : _rateLimiter(10)
    , _coordCache(500)
    , _concurrentLimiter(6)
{
    qCDebug(MapProviderLog) << "MapProviderManager initialized";
}

Q_GLOBAL_STATIC(MapProviderManager, s_mapProviderManager)

MapProviderManager* MapProviderManager::instance()
{
    return s_mapProviderManager();
}

MapProvider::MapProvider(
    const QString &mapName,
    const QString &referrer,
    const QString &imageFormat,
    quint32 averageSize,
    QGeoMapType::MapStyle mapStyle)
    : _mapName(mapName)
    , _referrer(referrer)
    , _imageFormat(imageFormat)
    , _averageSize(averageSize)
    , _mapStyle(mapStyle)
    , _language(!QLocale::system().uiLanguages().isEmpty() ? QLocale::system().uiLanguages().constFirst() : "en")
    , _mapId(_mapIdIndex++)
    , _urlCache(QGC_URL_CACHE_SIZE)
{
    qCDebug(MapProviderLog) << "Created provider:" << _mapName << "id:" << _mapId;
}

MapProvider::~MapProvider()
{
    qCDebug(MapProviderLog) << "Destroyed provider:" << _mapName << "id:" << _mapId;
}

QUrl MapProvider::getTileURL(int x, int y, int zoom) const
{
    // Validate zoom level
    if (!isValidZoom(zoom)) {
        qCWarning(MapProviderLog) << "Invalid zoom in getTileURL:" << zoom;
        return QUrl();
    }

    // Check if provider has specific zoom limits
    const ProviderMetadata meta = metadata();
    if ((zoom < meta.minZoom) || (zoom > meta.maxZoom)) {
        qCWarning(MapProviderLog) << "Zoom" << zoom << "outside provider limits [" << meta.minZoom << "," << meta.maxZoom << "]";
        return QUrl();
    }

    // Validate tile coordinates
    if (!isValidTileCoordinate(x, y, zoom)) {
        qCWarning(MapProviderLog) << "Invalid tile coordinate:" << x << "," << y << "at zoom" << zoom;
        return QUrl();
    }

    const QUrl url(_getURL(x, y, zoom));
    return url;
}

QString MapProvider::getImageFormat(QByteArrayView image) const
{
    const QByteArray data = image.toByteArray();
    const QString detected = TileValidator::detectImageFormat(data);
    return detected.isEmpty() ? _imageFormat : detected;
}

QString MapProvider::_tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const
{
    QString quadKey;
    quadKey.reserve(levelOfDetail);
    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        const int mask = 1 << (i - 1);

        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit += 2;
        }

        quadKey.append(digit);
    }

    return quadKey;
}

int MapProvider::_getServerNum(int x, int y, int max) const
{
    const int serverNum = (x + (2 * y)) % max;
    return serverNum;
}

int MapProvider::long2tileX(double lon, int z) const
{
    // Clamp longitude to valid range [-180, 180]
    if (lon < -180.0 || lon > 180.0) {
        qCWarning(MapProviderLog) << "Invalid longitude" << lon << "- clamping to [-180, 180]";
        lon = qBound(-180.0, lon, 180.0);
    }

    // Clamp zoom to valid range [0, MAX_ZOOM]
    if (z < 0 || z > QGC_MAX_MAP_ZOOM) {
        qCWarning(MapProviderLog) << "Invalid zoom" << z << "- clamping to [0," << QGC_MAX_MAP_ZOOM << "]";
        z = qBound(0, z, QGC_MAX_MAP_ZOOM);
    }

    return coordinateCache().getCachedLongToTileX(lon, z, this);
}

int MapProvider::lat2tileY(double lat, int z) const
{
    // Clamp latitude to valid range [-90, 90]
    if (lat < -90.0 || lat > 90.0) {
        qCWarning(MapProviderLog) << "Invalid latitude" << lat << "- clamping to [-90, 90]";
        lat = qBound(-90.0, lat, 90.0);
    }

    // Clamp zoom to valid range [0, MAX_ZOOM]
    if (z < 0 || z > QGC_MAX_MAP_ZOOM) {
        qCWarning(MapProviderLog) << "Invalid zoom" << z << "- clamping to [0," << QGC_MAX_MAP_ZOOM << "]";
        z = qBound(0, z, QGC_MAX_MAP_ZOOM);
    }

    return coordinateCache().getCachedLatToTileY(lat, z, this);
}

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon,
                                     double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(topleftLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(bottomRightLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}

// Resolution math: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Resolution_and_Scale

bool MapProvider::isValidLongitude(double lon)
{
    if ((lon < -180.0) || (lon > 180.0)) {
        qCWarning(MapProviderLog) << "Invalid longitude:" << lon << "(valid range: -180 to 180)";
        return false;
    }
    return true;
}

bool MapProvider::isValidLatitude(double lat)
{
    if ((lat < -90.0) || (lat > 90.0)) {
        qCWarning(MapProviderLog) << "Invalid latitude:" << lat << "(valid range: -90 to 90)";
        return false;
    }
    return true;
}

bool MapProvider::isValidZoom(int zoom)
{
    if ((zoom < 0) || (zoom > QGC_MAX_MAP_ZOOM)) {
        qCWarning(MapProviderLog) << "Invalid zoom level:" << zoom << "(valid range: 0 to" << QGC_MAX_MAP_ZOOM << ")";
        return false;
    }
    return true;
}

QString MapProvider::getSettingsToken(const QString& tokenName, const QString& providerName)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    if (!appSettings) {
        qCWarning(MapProviderLog) << "Failed to get AppSettings for" << providerName;
        return QString();
    }

    Fact* tokenFact = nullptr;

    // Map token names to settings
    if (tokenName == "mapbox") {
        tokenFact = appSettings->mapboxToken();
    } else if (tokenName == "esri") {
        tokenFact = appSettings->esriToken();
    } else if (tokenName == "tianditu") {
        tokenFact = appSettings->tiandituToken();
    } else if (tokenName == "linz") {
        tokenFact = appSettings->linzToken();
    } else if (tokenName == "openaip") {
        tokenFact = appSettings->openaipToken();
    } else if (tokenName == "vworld") {
        tokenFact = appSettings->vworldToken();
    } else {
        qCWarning(MapProviderLog) << "Unknown token name:" << tokenName << "for provider:" << providerName;
        return QString();
    }

    if (!tokenFact) {
        qCWarning(MapProviderLog) << "Failed to get token fact for" << tokenName << "in provider:" << providerName;
        return QString();
    }

    const QString token = tokenFact->rawValue().toString();
    if (token.isEmpty()) {
        qCDebug(MapProviderLog) << "Empty token for" << providerName << "- user may need to configure" << tokenName << "token in settings";
    }

    return token;
}

bool MapProvider::hasCapability(ProviderCapability capability) const
{
    return capabilities().contains(capability);
}

QString MapProvider::getCachedURL(int x, int y, int zoom)
{
    // Validate zoom level
    if (zoom < 0 || zoom > QGC_MAX_MAP_ZOOM) {
        qCWarning(MapProviderLog) << "Invalid zoom" << zoom << "in getCachedURL - clamping";
        zoom = qBound(0, zoom, QGC_MAX_MAP_ZOOM);
    }

    const TileKey key = {x, y, zoom};

    // Thread-safe cache access
    QMutexLocker locker(&_urlCacheMutex);

    // Check cache first
    QString* cachedUrl = _urlCache.object(key);
    if (cachedUrl) {
        qCDebug(MapProviderLog) << "URL cache hit for" << _mapName << "tile" << x << y << zoom;
        return *cachedUrl;
    }

    // Generate URL and cache it (unlock during URL generation for better concurrency)
    locker.unlock();
    const QString url = _getURL(x, y, zoom);
    locker.relock();

    if (!url.isEmpty()) {
        _urlCache.insert(key, new QString(url));
        qCDebug(MapProviderLog) << "URL cache miss for" << _mapName << "tile" << x << y << zoom << "- cached";
    }

    return url;
}

void MapProvider::clearURLCache()
{
    QMutexLocker locker(&_urlCacheMutex);
    _urlCache.clear();
    qCDebug(MapProviderLog) << "URL cache cleared for" << _mapName;
}

bool MapProvider::isWithinBounds(double lat, double lon) const
{
    const ProviderMetadata meta = metadata();
    if (!meta.hasBoundsLimits) {
        return true;  // No bounds limits, everything is valid
    }

    if (!meta.bounds.contains(lat, lon)) {
        qCDebug(MapProviderLog) << "Coordinate" << lat << "," << lon << "outside provider bounds for" << _mapName;
        return false;
    }

    return true;
}

bool MapProvider::isValidTileCoordinate(int x, int y, int zoom) const
{
    if (!isValidZoom(zoom)) {
        return false;
    }

    // Calculate maximum tile coordinates for this zoom level
    const int maxTile = (1 << zoom) - 1;

    if ((x < 0) || (x > maxTile) || (y < 0) || (y > maxTile)) {
        qCDebug(MapProviderLog) << "Tile coordinate" << x << "," << y << "out of range [0," << maxTile << "] at zoom" << zoom;
        return false;
    }

    return true;
}

bool MapProvider::isValidTileData(const QByteArray& data) const
{
    if (!TileValidator::hasValidSize(data)) {
        qCWarning(MapProviderLog) << "Tile data has invalid size:" << data.size() << "bytes";
        return false;
    }

    const QString format = detectTileFormat(data);
    if (format.isEmpty()) {
        qCWarning(MapProviderLog) << "Could not detect valid tile format";
        return false;
    }

    return true;
}

QString MapProvider::detectTileFormat(const QByteArray& data) const
{
    return TileValidator::detectImageFormat(data);
}

ProviderMetadata MapProvider::metadata() const
{
    ProviderMetadata meta;
    meta.name = _mapName;
    meta.description = QString("Map tiles from %1").arg(_mapName);
    meta.attribution = QString();
    meta.termsOfServiceUrl = QString();
    meta.minZoom = 0;
    meta.maxZoom = QGC_MAX_MAP_ZOOM;
    meta.hasBoundsLimits = false;
    return meta;
}

ProviderHealth MapProvider::getHealth() const
{
    QMutexLocker locker(&_healthMutex);
    return _health;
}

void MapProvider::recordSuccess()
{
    QMutexLocker locker(&_healthMutex);
    _health.recordSuccess();
    healthDashboard().updateProviderHealth(_mapName, _health);
}

void MapProvider::recordFailure()
{
    QMutexLocker locker(&_healthMutex);
    _health.recordFailure();
    healthDashboard().updateProviderHealth(_mapName, _health);
}

bool MapProvider::canMakeRequest() const
{
    return MapProviderManager::instance()->rateLimiter().canMakeRequest(_mapName);
}

void MapProvider::recordRequest()
{
    MapProviderManager::instance()->rateLimiter().recordRequest(_mapName);
}

// RateLimiter implementation
RateLimiter::RateLimiter(int maxRequestsPerSecond)
    : _maxRequestsPerSecond(maxRequestsPerSecond)
{
}

bool RateLimiter::canMakeRequest(const QString& providerName)
{
    QMutexLocker locker(&_mutex);

    const QDateTime now = QDateTime::currentDateTime();
    QQueue<QDateTime>& history = _requestHistory[providerName];

    // Remove requests older than 1 second
    while (!history.isEmpty() && history.head().msecsTo(now) > kHistoryWindowMs) {
        history.dequeue();
    }

    const bool canRequest = (history.size() < _maxRequestsPerSecond);
    if (!canRequest) {
        qCDebug(MapProviderLog) << "Rate limit hit for" << providerName
                                << "- requests in last second:" << history.size();
    }
    return canRequest;
}

void RateLimiter::recordRequest(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    _requestHistory[providerName].enqueue(QDateTime::currentDateTime());
}

void RateLimiter::reset(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    _requestHistory.remove(providerName);
}

// TileValidator implementation
bool TileValidator::isValidTile(const QByteArray& data, const QString& expectedFormat)
{
    if (data.isEmpty() || (data.size() < 10)) {
        return false;
    }

    // Check for valid image signatures
    if (expectedFormat == "png") {
        static constexpr QByteArrayView pngSig("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
        return data.startsWith(pngSig);
    } else if (expectedFormat == "jpg" || expectedFormat == "jpeg") {
        static constexpr QByteArrayView jpegSig("\xFF\xD8\xFF");
        return data.startsWith(jpegSig);
    } else if (expectedFormat == "gif") {
        static constexpr QByteArrayView gifSig("\x47\x49\x46\x38");
        return data.startsWith(gifSig);
    }

    // For other formats, just check basic size
    return hasValidSize(data);
}

bool TileValidator::isExpired(const QDateTime& cacheTime, int maxAgeDays)
{
    if (!cacheTime.isValid()) {
        return true;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const qint64 ageInDays = cacheTime.daysTo(now);
    return (ageInDays > maxAgeDays);
}

bool TileValidator::hasValidSize(const QByteArray& data)
{
    static constexpr int kMinTileSize = 100;      // 100 bytes minimum
    static constexpr int kMaxTileSize = 5242880;  // 5MB maximum
    return (data.size() >= kMinTileSize) && (data.size() <= kMaxTileSize);
}

// Enhanced format detection
QString TileValidator::detectImageFormat(const QByteArray& data)
{
    if (data.size() < 8) {
        return QString();
    }

    // Try each format in order of likelihood
    if (isValidPNG(data)) return QStringLiteral("png");
    if (isValidJPEG(data)) return QStringLiteral("jpg");
    if (isValidWebP(data)) return QStringLiteral("webp");
    if (isValidGIF(data)) return QStringLiteral("gif");
    if (isValidAVIF(data)) return QStringLiteral("avif");
    if (isValidBMP(data)) return QStringLiteral("bmp");

    return QString();
}

bool TileValidator::isValidPNG(const QByteArray& data)
{
    if (data.size() < 8) {
        return false;
    }
    static constexpr QByteArrayView pngSig("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
    return data.startsWith(pngSig);
}

bool TileValidator::isValidJPEG(const QByteArray& data)
{
    if (data.size() < 3) {
        return false;
    }
    static constexpr QByteArrayView jpegSig("\xFF\xD8\xFF");
    return data.startsWith(jpegSig);
}

bool TileValidator::isValidGIF(const QByteArray& data)
{
    if (data.size() < 6) {
        return false;
    }
    // GIF87a or GIF89a
    static constexpr QByteArrayView gif87Sig("GIF87a");
    static constexpr QByteArrayView gif89Sig("GIF89a");
    return (data.startsWith(gif87Sig) || data.startsWith(gif89Sig));
}

bool TileValidator::isValidWebP(const QByteArray& data)
{
    if (data.size() < 12) {
        return false;
    }
    // WebP signature: RIFF[4 bytes]WEBP
    static constexpr QByteArrayView riffSig("RIFF");
    static constexpr QByteArrayView webpSig("WEBP");
    return (data.startsWith(riffSig) && QByteArrayView(data.constData() + 8, 4).startsWith(webpSig));
}

bool TileValidator::isValidAVIF(const QByteArray& data)
{
    if (data.size() < 12) {
        return false;
    }
    // AVIF signature: [4 bytes]ftyp with 'avif' or 'avis' brand
    const char* dataPtr = data.constData();
    if ((dataPtr[4] == 'f') && (dataPtr[5] == 't') && (dataPtr[6] == 'y') && (dataPtr[7] == 'p')) {
        // Check for avif or avis brand at offset 8
        return ((dataPtr[8] == 'a') && (dataPtr[9] == 'v') && (dataPtr[10] == 'i') &&
                ((dataPtr[11] == 'f') || (dataPtr[11] == 's')));
    }
    return false;
}

bool TileValidator::isValidBMP(const QByteArray& data)
{
    if (data.size() < 2) {
        return false;
    }
    static constexpr QByteArrayView bmpSig("BM");
    return data.startsWith(bmpSig);
}

// MapProviderFactory implementation
QMap<QString, MapProviderFactory::ProviderCreator>& MapProviderFactory::registry()
{
    static QMap<QString, ProviderCreator> _registry;
    return _registry;
}

MapProvider* MapProviderFactory::create(const QString& providerType)
{
    auto& reg = registry();
    if (reg.contains(providerType)) {
        qCDebug(MapProviderLog) << "Creating provider:" << providerType;
        return reg[providerType]();
    }

    qCWarning(MapProviderLog) << "Unknown provider type:" << providerType;
    return nullptr;
}

QStringList MapProviderFactory::availableProviders()
{
    return registry().keys();
}

bool MapProviderFactory::registerProvider(const QString& type, ProviderCreator creator)
{
    if (registry().contains(type)) {
        qCWarning(MapProviderLog) << "Provider" << type << "already registered";
        return false;
    }

    registry()[type] = creator;
    qCDebug(MapProviderLog) << "Registered provider:" << type;
    return true;
}

// ProviderFallbackChain implementation
void ProviderFallbackChain::setPrimaryProvider(MapProvider* primary)
{
    _chain.clear();
    if (primary) {
        _chain.append(primary);
    }
}

void ProviderFallbackChain::addFallback(MapProvider* fallback)
{
    if (fallback && !_chain.contains(fallback)) {
        _chain.append(fallback);
    }
}

void ProviderFallbackChain::clearFallbacks()
{
    _chain.clear();
}

QString ProviderFallbackChain::getTileURL(int x, int y, int zoom)
{
    for (MapProvider* provider : _chain) {
        if (provider->getHealth().isHealthy() && provider->canMakeRequest()) {
            const QString url = provider->getCachedURL(x, y, zoom);
            if (!url.isEmpty()) {
                provider->recordRequest();
                qCDebug(MapProviderLog) << "Using provider" << provider->getMapName()
                                        << "for tile" << x << y << zoom;
                return url;
            }
        }
    }

    qCWarning(MapProviderLog) << "All providers failed or rate limited for tile" << x << y << zoom;
    return QString();
}

MapProvider* ProviderFallbackChain::getHealthyProvider()
{
    for (MapProvider* provider : _chain) {
        if (provider->getHealth().isHealthy() && provider->canMakeRequest()) {
            return provider;
        }
    }
    return nullptr;
}

// CoordinateTransformCache implementation
CoordinateTransformCache::CoordinateTransformCache(int cacheSize)
    : _lonCache(cacheSize)
    , _latCache(cacheSize)
{
}

int CoordinateTransformCache::getCachedLongToTileX(double lon, int zoom, const MapProvider* provider)
{
    const qint64 key = static_cast<qint64>(lon * 1000000.0);
    const CoordKey cacheKey(key, zoom);

    int* cached = _lonCache.object(cacheKey);
    if (cached) {
        return *cached;
    }

    // Calculate and cache
    const int tileX = static_cast<int>(floor(((lon + 180.0) / 360.0) * (1 << zoom)));
    _lonCache.insert(cacheKey, new int(tileX));
    return tileX;
}

int CoordinateTransformCache::getCachedLatToTileY(double lat, int zoom, const MapProvider* provider)
{
    const qint64 key = static_cast<qint64>(lat * 1000000.0);
    const CoordKey cacheKey(key, zoom);

    int* cached = _latCache.object(cacheKey);
    if (cached) {
        return *cached;
    }

    // Calculate and cache
    const int tileY = static_cast<int>(floor(((1.0 - (log(tan((lat * M_PI) / 180.0) + (1.0 / cos((lat * M_PI) / 180.0))) / M_PI)) / 2.0) * (1 << zoom)));
    _latCache.insert(cacheKey, new int(tileY));
    return tileY;
}

void CoordinateTransformCache::clear()
{
    _lonCache.clear();
    _latCache.clear();
}

// CacheStats implementation
void CacheStats::logStats() const
{
    qCDebug(MapProviderLog) << "Cache Statistics:";
    qCDebug(MapProviderLog) << "  URL Cache:" << urlCacheSize << "entries," << urlCacheMemoryBytes << "bytes";
    qCDebug(MapProviderLog) << "  Coordinate Cache:" << coordinateCacheSize << "entries," << coordinateCacheMemoryBytes << "bytes";
    qCDebug(MapProviderLog) << "  Total Memory:" << totalMemoryUsage() << "bytes";
}

// ErrorTracker implementation
void ErrorTracker::recordError(const ErrorReport& error)
{
    _errors.append(error);

    // Keep only recent errors
    if (_errors.size() > kMaxStoredErrors) {
        _errors.removeFirst();
    }

    qCWarning(MapProviderLog) << "Tile error recorded:"
                              << "Type=" << static_cast<int>(error.type)
                              << "Provider=" << error.providerName
                              << "Details=" << error.details;
}

QList<ErrorReport> ErrorTracker::getRecentErrors(int count) const
{
    const int start = qMax(0, _errors.size() - count);
    return _errors.mid(start, count);
}

QList<ErrorReport> ErrorTracker::getErrorsByType(TileErrorType type) const
{
    QList<ErrorReport> filtered;
    for (const ErrorReport& error : _errors) {
        if (error.type == type) {
            filtered.append(error);
        }
    }
    return filtered;
}

QList<ErrorReport> ErrorTracker::getErrorsByProvider(const QString& providerName) const
{
    QList<ErrorReport> filtered;
    for (const ErrorReport& error : _errors) {
        if (error.providerName == providerName) {
            filtered.append(error);
        }
    }
    return filtered;
}

int ErrorTracker::errorCountByType(TileErrorType type) const
{
    int count = 0;
    for (const ErrorReport& error : _errors) {
        if (error.type == type) {
            count++;
        }
    }
    return count;
}

void ErrorTracker::clear()
{
    _errors.clear();
}

// ProviderHealthDashboard implementation
void ProviderHealthDashboard::updateProviderHealth(const QString& providerName, const ProviderHealth& health)
{
    if (!_providerHealth.contains(providerName)) {
        ProviderHealthInfo info;
        info.providerName = providerName;
        info.health = health;
        info.errorCount = 0;
        info.lastErrorType = TileErrorType::None;
        _providerHealth[providerName] = info;
    } else {
        _providerHealth[providerName].health = health;
    }
}

void ProviderHealthDashboard::recordProviderError(const QString& providerName, TileErrorType errorType)
{
    if (!_providerHealth.contains(providerName)) {
        ProviderHealthInfo info;
        info.providerName = providerName;
        _providerHealth[providerName] = info;
    }

    _providerHealth[providerName].errorCount++;
    _providerHealth[providerName].lastErrorType = errorType;
}

QList<ProviderHealthDashboard::ProviderHealthInfo> ProviderHealthDashboard::getAllProviderHealth() const
{
    return _providerHealth.values();
}

ProviderHealthDashboard::ProviderHealthInfo ProviderHealthDashboard::getWorstProvider() const
{
    ProviderHealthInfo worst;
    double worstRate = 1.0;

    for (const ProviderHealthInfo& info : _providerHealth.values()) {
        const double rate = info.health.successRate();
        if (rate < worstRate) {
            worstRate = rate;
            worst = info;
        }
    }

    return worst;
}

ProviderHealthDashboard::ProviderHealthInfo ProviderHealthDashboard::getBestProvider() const
{
    ProviderHealthInfo best;
    double bestRate = 0.0;

    for (const ProviderHealthInfo& info : _providerHealth.values()) {
        const double rate = info.health.successRate();
        if (rate > bestRate) {
            bestRate = rate;
            best = info;
        }
    }

    return best;
}

QVariantMap ProviderHealthDashboard::healthSummaryForQML() const
{
    QVariantMap summary;
    QVariantList providers;

    for (const ProviderHealthInfo& info : _providerHealth.values()) {
        QVariantMap providerData;
        providerData["name"] = info.providerName;
        providerData["successCount"] = info.health.successCount;
        providerData["failureCount"] = info.health.failureCount;
        providerData["successRate"] = info.health.successRate();
        providerData["isHealthy"] = info.health.isHealthy();
        providerData["errorCount"] = info.errorCount;
        providerData["lastErrorType"] = static_cast<int>(info.lastErrorType);
        providers.append(providerData);
    }

    summary["providers"] = providers;
    summary["totalProviders"] = _providerHealth.size();

    return summary;
}

// MapProvider new methods
CacheStats MapProvider::getCacheStats() const
{
    QMutexLocker locker(&_urlCacheMutex);
    CacheStats stats;
    stats.urlCacheSize = _urlCache.size();

    // Calculate actual URL cache memory usage
    size_t urlMemory = 0;
    QList<TileKey> keys = _urlCache.keys();
    for (const TileKey& key : keys) {
        QString* url = _urlCache.object(key);
        if (url) {
            // TileKey struct + QString object + string data
            urlMemory += sizeof(TileKey) + sizeof(QString) + (url->size() * sizeof(QChar));
        }
    }
    stats.urlCacheMemoryBytes = urlMemory;

    // Coordinate cache size and memory
    stats.coordinateCacheSize = coordinateCache().size();
    // Each entry: QPair<qint64, int> (key) + int (value) + hash table overhead
    stats.coordinateCacheMemoryBytes = coordinateCache().size() * (sizeof(qint64) + sizeof(int) + sizeof(int) + 16);

    return stats;
}

void MapProvider::clearCachesIfOverLimit(size_t maxBytes)
{
    const CacheStats stats = getCacheStats();
    if (stats.isOverLimit(maxBytes)) {
        qCWarning(MapProviderLog) << "Cache over limit" << stats.totalMemoryUsage() << "bytes, clearing...";
        clearURLCache();
        coordinateCache().clear();
    }
}

void MapProvider::recordError(TileErrorType type, const QString& details, int x, int y, int zoom)
{
    ErrorReport error(type, _mapName, details, x, y, zoom);
    errorTracker().recordError(error);
    healthDashboard().recordProviderError(_mapName, type);
}

// Integration methods for complete tile fetching workflow
bool MapProvider::canStartTileRequest()
{
    // Check rate limiting
    if (!canMakeRequest()) {
        qCDebug(MapProviderLog) << "Rate limit prevents tile request for" << _mapName;
        return false;
    }

    // Check concurrent request limiting
    if (!concurrentLimiter().canStartRequest(_mapName)) {
        qCDebug(MapProviderLog) << "Concurrent limit prevents tile request for" << _mapName
                                << "- active:" << concurrentLimiter().activeRequests(_mapName);
        return false;
    }

    // Check provider health
    if (!getHealth().isHealthy()) {
        qCDebug(MapProviderLog) << "Provider" << _mapName << "is unhealthy - success rate:"
                                << getHealth().successRate();
        return false;
    }

    return true;
}

void MapProvider::beginTileRequest(int x, int y, int zoom, bool fromCache)
{
    // Record request metrics
    metricsCollector().recordRequest(_mapName, fromCache);

    if (!fromCache) {
        // Record rate limiter
        recordRequest();

        // Start concurrent request tracking (caller should use RequestGuard for RAII)
        qCDebug(MapProviderLog) << "Beginning tile request for" << _mapName << "tile" << x << y << zoom;
    }
}

void MapProvider::completeTileRequest(int x, int y, int zoom, bool success, quint64 bytes, double responseTimeMs, TileErrorType errorType)
{
    const QString tileKey = QString("%1,%2,%3").arg(x).arg(y).arg(zoom);

    if (success) {
        // Record success metrics
        recordSuccess();
        metricsCollector().recordSuccess(_mapName, bytes, responseTimeMs);
        retryManager().recordSuccess(tileKey);

        qCDebug(MapProviderLog) << "Tile request succeeded for" << _mapName << "tile" << x << y << zoom
                                << "- bytes:" << bytes << "time:" << responseTimeMs << "ms";
    } else {
        // Record failure
        recordFailure();
        metricsCollector().recordFailure(_mapName);
        recordError(errorType, "Tile request failed", x, y, zoom);

        qCWarning(MapProviderLog) << "Tile request failed for" << _mapName << "tile" << x << y << zoom
                                  << "- error:" << static_cast<int>(errorType);
    }
}

bool MapProvider::shouldRetryTile(int x, int y, int zoom, TileErrorType errorType)
{
    const QString tileKey = QString("%1,%2,%3").arg(x).arg(y).arg(zoom);

    if (!RetryPolicy::isRetryable(errorType)) {
        qCDebug(MapProviderLog) << "Error type" << static_cast<int>(errorType) << "is not retryable for tile" << x << y << zoom;
        return false;
    }

    const bool shouldRetry = retryManager().shouldRetry(tileKey, errorType);
    if (shouldRetry) {
        retryManager().recordAttempt(tileKey, errorType);
        metricsCollector().recordRetry(_mapName);
        qCDebug(MapProviderLog) << "Will retry tile" << x << y << zoom << "after delay";
    } else {
        qCDebug(MapProviderLog) << "Max retries exceeded for tile" << x << y << zoom;
    }

    return shouldRetry;
}

int MapProvider::getRetryDelay(int x, int y, int zoom) const
{
    const QString tileKey = QString("%1,%2,%3").arg(x).arg(y).arg(zoom);
    return retryManager().getRetryDelayMs(tileKey);
}


// AttributionManager implementation
void AttributionManager::registerAttribution(const QString& providerName, const QString& attribution, const QString& termsUrl)
{
    if (providerName.isEmpty() || attribution.isEmpty()) {
        qCWarning(MapProviderLog) << "Cannot register empty attribution for" << providerName;
        return;
    }

    AttributionInfo info;
    info.providerName = providerName;
    info.attributionText = attribution;
    info.termsUrl = termsUrl;
    info.isRequired = true;

    _attributions[providerName] = info;
    qCDebug(MapProviderLog) << "Registered attribution for" << providerName << ":" << attribution;
}

void AttributionManager::unregisterAttribution(const QString& providerName)
{
    if (_attributions.remove(providerName) > 0) {
        qCDebug(MapProviderLog) << "Unregistered attribution for" << providerName;
    }
}

QList<AttributionManager::AttributionInfo> AttributionManager::getAllAttributions() const
{
    return _attributions.values();
}

QString AttributionManager::getCombinedAttribution(const QString& separator) const
{
    QStringList attributions;
    for (const AttributionInfo& info : _attributions.values()) {
        if (info.isRequired && !info.attributionText.isEmpty()) {
            attributions.append(info.attributionText);
        }
    }
    return attributions.join(separator);
}

QStringList AttributionManager::getRequiredAttributions() const
{
    QStringList attributions;
    for (const AttributionInfo& info : _attributions.values()) {
        if (info.isRequired && !info.attributionText.isEmpty()) {
            attributions.append(info.attributionText);
        }
    }
    return attributions;
}

bool AttributionManager::hasAttribution(const QString& providerName) const
{
    return _attributions.contains(providerName);
}

// ConcurrentRequestLimiter implementation
ConcurrentRequestLimiter::ConcurrentRequestLimiter(int maxConcurrentRequests)
    : _maxConcurrentRequests(maxConcurrentRequests)
{
    qCDebug(MapProviderLog) << "ConcurrentRequestLimiter initialized with max" << maxConcurrentRequests << "concurrent requests";
}

bool ConcurrentRequestLimiter::canStartRequest(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    const int active = _activeRequests.value(providerName, 0);
    return (active < _maxConcurrentRequests);
}

void ConcurrentRequestLimiter::startRequest(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    _activeRequests[providerName]++;
    qCDebug(MapProviderLog) << "Started request for" << providerName << "- active:" << _activeRequests[providerName];
}

void ConcurrentRequestLimiter::finishRequest(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    if (_activeRequests.contains(providerName) && (_activeRequests[providerName] > 0)) {
        _activeRequests[providerName]--;
        qCDebug(MapProviderLog) << "Finished request for" << providerName << "- active:" << _activeRequests[providerName];
    }
}

int ConcurrentRequestLimiter::activeRequests(const QString& providerName) const
{
    QMutexLocker locker(&_mutex);
    return _activeRequests.value(providerName, 0);
}

void ConcurrentRequestLimiter::reset(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    _activeRequests.remove(providerName);
    qCDebug(MapProviderLog) << "Reset concurrent requests for" << providerName;
}

// RetryPolicy implementation
bool RetryPolicy::isRetryable(TileErrorType errorType)
{
    switch (errorType) {
        case TileErrorType::NetworkTimeout:
        case TileErrorType::RateLimited:
        case TileErrorType::ServerError:
        case TileErrorType::EmptyResponse:
            return true;

        case TileErrorType::InvalidToken:
        case TileErrorType::InvalidTile:
        case TileErrorType::OutOfBounds:
        case TileErrorType::None:
        default:
            return false;
    }
}

int RetryPolicy::getDelayForAttempt(int attemptNumber) const
{
    if (attemptNumber <= 0) {
        return 0;
    }

    // Exponential backoff: initialDelay * (multiplier ^ (attempt - 1))
    double delay = initialDelayMs * std::pow(backoffMultiplier, attemptNumber - 1);
    return std::min(static_cast<int>(delay), maxDelayMs);
}

// RetryState implementation
bool RetryState::shouldRetry(const RetryPolicy& policy) const
{
    return ((attemptCount < policy.maxRetries) && RetryPolicy::isRetryable(lastErrorType));
}

int RetryState::getNextDelayMs(const RetryPolicy& policy) const
{
    return policy.getDelayForAttempt(attemptCount + 1);
}

// RetryManager implementation
RetryManager::RetryManager(const RetryPolicy& policy)
    : _policy(policy)
{
    qCDebug(MapProviderLog) << "RetryManager initialized with max" << policy.maxRetries << "retries";
}

bool RetryManager::shouldRetry(const QString& tileKey, TileErrorType errorType)
{
    if (!RetryPolicy::isRetryable(errorType)) {
        qCDebug(MapProviderLog) << "Error type not retryable for" << tileKey;
        return false;
    }

    if (!_retryStates.contains(tileKey)) {
        // First failure, can retry
        return true;
    }

    const RetryState& state = _retryStates[tileKey];
    return state.shouldRetry(_policy);
}

void RetryManager::recordAttempt(const QString& tileKey, TileErrorType errorType)
{
    RetryState& state = _retryStates[tileKey];
    state.attemptCount++;
    state.lastAttemptTime = QDateTime::currentDateTime();
    state.lastErrorType = errorType;
    state.tileKey = tileKey;

    _totalRetries++;

    qCDebug(MapProviderLog) << "Retry attempt" << state.attemptCount << "for" << tileKey
                            << "- next delay:" << state.getNextDelayMs(_policy) << "ms";

    // Limit tracked tiles to prevent memory growth - remove oldest entry
    if (_retryStates.size() > kMaxTrackedTiles) {
        QDateTime oldestTime = QDateTime::currentDateTime();
        QString oldestKey;

        // Find the entry with the oldest lastAttemptTime
        for (auto it = _retryStates.constBegin(); it != _retryStates.constEnd(); ++it) {
            if (!it->lastAttemptTime.isValid() || it->lastAttemptTime < oldestTime) {
                oldestTime = it->lastAttemptTime;
                oldestKey = it.key();
            }
        }

        if (!oldestKey.isEmpty()) {
            _retryStates.remove(oldestKey);
            qCDebug(MapProviderLog) << "Removed oldest retry state for" << oldestKey;
        }
    }
}

void RetryManager::recordSuccess(const QString& tileKey)
{
    if (_retryStates.contains(tileKey)) {
        const int attempts = _retryStates[tileKey].attemptCount;
        if (attempts > 0) {
            _successfulRetries++;
            qCDebug(MapProviderLog) << "Successful retry for" << tileKey << "after" << attempts << "attempts";
        }
        _retryStates.remove(tileKey);
    }
}

int RetryManager::getRetryDelayMs(const QString& tileKey) const
{
    if (_retryStates.contains(tileKey)) {
        return _retryStates[tileKey].getNextDelayMs(_policy);
    }
    return 0;
}

void RetryManager::clear()
{
    _retryStates.clear();
    _totalRetries = 0;
    _successfulRetries = 0;
    qCDebug(MapProviderLog) << "Retry manager cleared";
}

// MetricsCollector implementation
bool MetricsCollector::canAddProvider(const QString& providerName) const
{
    if (_metrics.contains(providerName)) {
        return true;
    }
    if (_metrics.size() >= kMaxTrackedProviders) {
        qCWarning(MapProviderLog) << "MetricsCollector: max providers reached (" << kMaxTrackedProviders
                                  << "), ignoring new provider:" << providerName;
        return false;
    }
    return true;
}

int MetricsCollector::providerCount() const
{
    QMutexLocker locker(&_mutex);
    return _metrics.size();
}

void MetricsCollector::recordRequest(const QString& providerName, bool fromCache)
{
    QMutexLocker locker(&_mutex);

    if (!canAddProvider(providerName)) {
        return;
    }

    ProviderMetrics& metrics = _metrics[providerName];
    if (metrics.providerName.isEmpty()) {
        metrics.providerName = providerName;
        metrics.firstRequestTime = QDateTime::currentDateTime();
    }

    metrics.totalRequests++;
    if (fromCache) {
        metrics.cachedRequests++;
    }
    metrics.lastRequestTime = QDateTime::currentDateTime();
}

void MetricsCollector::recordSuccess(const QString& providerName, quint64 bytes, double responseTimeMs)
{
    QMutexLocker locker(&_mutex);

    if (!canAddProvider(providerName)) {
        return;
    }

    ProviderMetrics& metrics = _metrics[providerName];
    metrics.successfulRequests++;
    metrics.bytesDownloaded += bytes;

    // Update running average
    const quint64 totalSuccesses = metrics.successfulRequests;
    metrics.averageResponseTimeMs = ((metrics.averageResponseTimeMs * (totalSuccesses - 1)) + responseTimeMs) / totalSuccesses;
}

void MetricsCollector::recordFailure(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    if (!canAddProvider(providerName)) {
        return;
    }
    _metrics[providerName].failedRequests++;
}

void MetricsCollector::recordRetry(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    if (!canAddProvider(providerName)) {
        return;
    }
    _metrics[providerName].retriedRequests++;
}

ProviderMetrics MetricsCollector::getMetrics(const QString& providerName) const
{
    QMutexLocker locker(&_mutex);
    return _metrics.value(providerName);
}

QList<ProviderMetrics> MetricsCollector::getAllMetrics() const
{
    QMutexLocker locker(&_mutex);
    return _metrics.values();
}

QVariantMap MetricsCollector::getMetricsForQML() const
{
    QMutexLocker locker(&_mutex);

    QVariantMap result;
    QVariantList providersList;

    for (const ProviderMetrics& metrics : _metrics.values()) {
        QVariantMap providerData;
        providerData["name"] = metrics.providerName;
        providerData["totalRequests"] = static_cast<qulonglong>(metrics.totalRequests);
        providerData["successfulRequests"] = static_cast<qulonglong>(metrics.successfulRequests);
        providerData["failedRequests"] = static_cast<qulonglong>(metrics.failedRequests);
        providerData["retriedRequests"] = static_cast<qulonglong>(metrics.retriedRequests);
        providerData["cachedRequests"] = static_cast<qulonglong>(metrics.cachedRequests);
        providerData["bytesDownloaded"] = static_cast<qulonglong>(metrics.bytesDownloaded);
        providerData["averageResponseTimeMs"] = metrics.averageResponseTimeMs;
        providerData["successRate"] = metrics.successRate();
        providerData["cacheHitRate"] = metrics.cacheHitRate();

        providersList.append(providerData);
    }

    result["providers"] = providersList;
    result["totalProviders"] = _metrics.size();

    return result;
}

QString MetricsCollector::getMetricsSummary() const
{
    QMutexLocker locker(&_mutex);

    QString summary;
    summary += QString("=== Map Provider Metrics ===\n");
    summary += QString("Total Providers: %1\n\n").arg(_metrics.size());

    for (const ProviderMetrics& metrics : _metrics.values()) {
        summary += QString("Provider: %1\n").arg(metrics.providerName);
        summary += QString("  Total Requests: %1\n").arg(metrics.totalRequests);
        summary += QString("  Successful: %1 (%.1f%%)\n")
                       .arg(metrics.successfulRequests)
                       .arg(metrics.successRate() * 100.0);
        summary += QString("  Failed: %1\n").arg(metrics.failedRequests);
        summary += QString("  Retried: %1\n").arg(metrics.retriedRequests);
        summary += QString("  Cached: %1 (%.1f%% hit rate)\n")
                       .arg(metrics.cachedRequests)
                       .arg(metrics.cacheHitRate() * 100.0);
        summary += QString("  Data Downloaded: %1 MB\n")
                       .arg(metrics.bytesDownloaded / 1048576.0, 0, 'f', 2);
        summary += QString("  Avg Response Time: %.1f ms\n").arg(metrics.averageResponseTimeMs);
        summary += QString("\n");
    }

    return summary;
}

void MetricsCollector::reset()
{
    QMutexLocker locker(&_mutex);
    _metrics.clear();
    qCDebug(MapProviderLog) << "All metrics cleared";
}

void MetricsCollector::reset(const QString& providerName)
{
    QMutexLocker locker(&_mutex);
    _metrics.remove(providerName);
    qCDebug(MapProviderLog) << "Metrics cleared for" << providerName;
}

// ProviderRotationStrategy implementation
ProviderRotationStrategy::ProviderRotationStrategy(Strategy strategy)
    : _strategy(strategy)
    , _lastUsedIndex(-1)
{
    qCDebug(MapProviderLog) << "ProviderRotationStrategy initialized with strategy" << static_cast<int>(strategy);
}

MapProvider* ProviderRotationStrategy::selectProvider(const QList<MapProvider*>& providers)
{
    if (providers.isEmpty()) {
        return nullptr;
    }

    QMutexLocker locker(&_mutex);

    switch (_strategy) {
        case Strategy::RoundRobin:
            return selectRoundRobin(providers);
        case Strategy::WeightedHealth:
            return selectWeightedHealth(providers);
        case Strategy::LeastConnections:
            return selectLeastConnections(providers);
        case Strategy::Random:
            return selectRandom(providers);
        default:
            return selectRoundRobin(providers);
    }
}

MapProvider* ProviderRotationStrategy::selectRoundRobin(const QList<MapProvider*>& providers)
{
    if (providers.isEmpty()) {
        return nullptr;
    }

    // Rotate to next provider
    _lastUsedIndex = (_lastUsedIndex + 1) % providers.size();
    return providers[_lastUsedIndex];
}

MapProvider* ProviderRotationStrategy::selectWeightedHealth(const QList<MapProvider*>& providers)
{
    if (providers.isEmpty()) {
        return nullptr;
    }

    // Calculate total weighted health
    double totalWeight = 0.0;
    QList<double> cumulativeWeights;

    for (MapProvider* provider : providers) {
        const double health = provider->getHealth().successRate();
        totalWeight += health;
        cumulativeWeights.append(totalWeight);
    }

    if (totalWeight <= 0.0) {
        // All providers unhealthy - fall back to round robin
        return selectRoundRobin(providers);
    }

    // Random selection weighted by health
    const double random = QRandomGenerator::global()->bounded(totalWeight);

    for (int i = 0; i < cumulativeWeights.size(); i++) {
        if (random <= cumulativeWeights[i]) {
            return providers[i];
        }
    }

    return providers.last();
}

MapProvider* ProviderRotationStrategy::selectLeastConnections(const QList<MapProvider*>& providers)
{
    if (providers.isEmpty()) {
        return nullptr;
    }

    MapProvider* leastBusy = nullptr;
    int minConnections = INT_MAX;

    for (MapProvider* provider : providers) {
        const int active = MapProvider::concurrentLimiter().activeRequests(provider->getMapName());
        if (active < minConnections) {
            minConnections = active;
            leastBusy = provider;
        }
    }

    return leastBusy;
}

MapProvider* ProviderRotationStrategy::selectRandom(const QList<MapProvider*>& providers)
{
    if (providers.isEmpty()) {
        return nullptr;
    }

    const int index = QRandomGenerator::global()->bounded(providers.size());
    return providers[index];
}

void ProviderRotationStrategy::recordProviderUsage(MapProvider* provider)
{
    if (!provider) {
        return;
    }

    QMutexLocker locker(&_mutex);
    _usageCounts[provider]++;
}

void ProviderRotationStrategy::setStrategy(Strategy strategy)
{
    QMutexLocker locker(&_mutex);
    _strategy = strategy;
    qCDebug(MapProviderLog) << "Changed rotation strategy to" << static_cast<int>(strategy);
}

// LoadBalancer implementation
LoadBalancer::LoadBalancer(double minHealthThreshold, bool autoFailover)
    : _minHealthThreshold(minHealthThreshold)
    , _autoFailover(autoFailover)
{
    qCDebug(MapProviderLog) << "LoadBalancer initialized - minHealth:" << minHealthThreshold
                            << "autoFailover:" << autoFailover;
}

void LoadBalancer::addProvider(MapProvider* provider, int weight)
{
    if (!provider) {
        qCWarning(MapProviderLog) << "Cannot add null provider to LoadBalancer";
        return;
    }

    QMutexLocker locker(&_mutex);

    // Check if provider already exists
    for (const ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            qCWarning(MapProviderLog) << "Provider" << provider->getMapName() << "already in LoadBalancer";
            return;
        }
    }

    _providers.append(ProviderConfig(provider, weight, true));
    qCDebug(MapProviderLog) << "Added provider" << provider->getMapName() << "to LoadBalancer with weight" << weight;
}

void LoadBalancer::removeProvider(MapProvider* provider)
{
    if (!provider) {
        return;
    }

    QMutexLocker locker(&_mutex);

    for (int i = 0; i < _providers.size(); i++) {
        if (_providers[i].provider == provider) {
            _providers.removeAt(i);
            qCDebug(MapProviderLog) << "Removed provider" << provider->getMapName() << "from LoadBalancer";
            return;
        }
    }
}

void LoadBalancer::enableProvider(MapProvider* provider, bool enable)
{
    if (!provider) {
        return;
    }

    QMutexLocker locker(&_mutex);

    for (ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            config.enabled = enable;
            qCDebug(MapProviderLog) << "Provider" << provider->getMapName()
                                    << (enable ? "enabled" : "disabled") << "in LoadBalancer";
            return;
        }
    }
}

void LoadBalancer::setProviderWeight(MapProvider* provider, int weight)
{
    if (!provider || weight < 1) {
        return;
    }

    QMutexLocker locker(&_mutex);

    for (ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            config.weight = weight;
            qCDebug(MapProviderLog) << "Provider" << provider->getMapName() << "weight set to" << weight;
            return;
        }
    }
}

MapProvider* LoadBalancer::getNextProvider()
{
    QMutexLocker locker(&_mutex);

    // Get list of healthy, enabled providers
    QList<MapProvider*> candidates;
    for (const ProviderConfig& config : _providers) {
        if (!config.enabled) {
            continue;
        }

        if (_autoFailover) {
            // Check health threshold
            const double health = config.provider->getHealth().successRate();
            if (health >= _minHealthThreshold) {
                candidates.append(config.provider);
            }
        } else {
            // Include all enabled providers regardless of health
            candidates.append(config.provider);
        }
    }

    if (candidates.isEmpty()) {
        qCWarning(MapProviderLog) << "No healthy providers available in LoadBalancer";
        return nullptr;
    }

    // Use rotation strategy to select provider
    MapProvider* selected = _rotationStrategy.selectProvider(candidates);
    if (selected) {
        _rotationStrategy.recordProviderUsage(selected);
        qCDebug(MapProviderLog) << "LoadBalancer selected provider" << selected->getMapName();
    }

    return selected;
}

QList<MapProvider*> LoadBalancer::getHealthyProviders() const
{
    QMutexLocker locker(&_mutex);

    QList<MapProvider*> healthy;
    for (const ProviderConfig& config : _providers) {
        if (config.enabled && (config.provider->getHealth().successRate() >= _minHealthThreshold)) {
            healthy.append(config.provider);
        }
    }

    return healthy;
}

QList<MapProvider*> LoadBalancer::getAllProviders() const
{
    QMutexLocker locker(&_mutex);

    QList<MapProvider*> all;
    for (const ProviderConfig& config : _providers) {
        all.append(config.provider);
    }

    return all;
}

void LoadBalancer::setMinHealthThreshold(double threshold)
{
    QMutexLocker locker(&_mutex);
    _minHealthThreshold = qBound(0.0, threshold, 1.0);
    qCDebug(MapProviderLog) << "LoadBalancer health threshold set to" << _minHealthThreshold;
}

void LoadBalancer::enableAutoFailover(bool enable)
{
    QMutexLocker locker(&_mutex);
    _autoFailover = enable;
    qCDebug(MapProviderLog) << "LoadBalancer auto-failover" << (enable ? "enabled" : "disabled");
}

void LoadBalancer::setStrategy(ProviderRotationStrategy::Strategy strategy)
{
    QMutexLocker locker(&_mutex);
    _rotationStrategy.setStrategy(strategy);
}

int LoadBalancer::healthyProviderCount() const
{
    return getHealthyProviders().size();
}

double LoadBalancer::averageHealthScore() const
{
    QMutexLocker locker(&_mutex);

    if (_providers.isEmpty()) {
        return 0.0;
    }

    double totalHealth = 0.0;
    for (const ProviderConfig& config : _providers) {
        totalHealth += config.provider->getHealth().successRate();
    }

    return totalHealth / _providers.size();
}

QVariantMap LoadBalancer::getStatisticsForQML() const
{
    QMutexLocker locker(&_mutex);

    QVariantMap stats;
    stats["totalProviders"] = _providers.size();
    stats["healthyProviders"] = healthyProviderCount();
    stats["averageHealth"] = averageHealthScore();
    stats["minHealthThreshold"] = _minHealthThreshold;
    stats["autoFailover"] = _autoFailover;
    stats["strategy"] = static_cast<int>(_rotationStrategy.getStrategy());

    QVariantList providersList;
    for (const ProviderConfig& config : _providers) {
        QVariantMap providerData;
        providerData["name"] = config.provider->getMapName();
        providerData["weight"] = config.weight;
        providerData["enabled"] = config.enabled;
        providerData["health"] = config.provider->getHealth().successRate();
        providerData["activeRequests"] = MapProvider::concurrentLimiter().activeRequests(config.provider->getMapName());
        providersList.append(providerData);
    }
    stats["providers"] = providersList;

    return stats;
}

bool LoadBalancer::hasProvider(MapProvider* provider) const
{
    if (!provider) {
        return false;
    }

    QMutexLocker locker(&_mutex);

    for (const ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            return true;
        }
    }

    return false;
}

int LoadBalancer::getProviderWeight(MapProvider* provider) const
{
    if (!provider) {
        return 0;
    }

    QMutexLocker locker(&_mutex);

    for (const ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            return config.weight;
        }
    }

    return 0;
}

bool LoadBalancer::isProviderEnabled(MapProvider* provider) const
{
    if (!provider) {
        return false;
    }

    QMutexLocker locker(&_mutex);

    for (const ProviderConfig& config : _providers) {
        if (config.provider == provider) {
            return config.enabled;
        }
    }

    return false;
}
