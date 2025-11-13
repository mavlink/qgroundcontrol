/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapProviderTest.h"
#include "MockMapProvider.h"
#include "MapProvider.h"

#include <QtTest/QTest>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QVector>
#include <atomic>

void MapProviderTest::_testCoordinateValidation()
{
    // Test valid coordinates
    QVERIFY(MapProvider::isValidLongitude(0.0));
    QVERIFY(MapProvider::isValidLongitude(180.0));
    QVERIFY(MapProvider::isValidLongitude(-180.0));
    QVERIFY(MapProvider::isValidLatitude(0.0));
    QVERIFY(MapProvider::isValidLatitude(90.0));
    QVERIFY(MapProvider::isValidLatitude(-90.0));
    QVERIFY(MapProvider::isValidZoom(0));
    QVERIFY(MapProvider::isValidZoom(QGC_MAX_MAP_ZOOM));

    // Test invalid coordinates
    QVERIFY(!MapProvider::isValidLongitude(181.0));
    QVERIFY(!MapProvider::isValidLongitude(-181.0));
    QVERIFY(!MapProvider::isValidLatitude(91.0));
    QVERIFY(!MapProvider::isValidLatitude(-91.0));
    QVERIFY(!MapProvider::isValidZoom(-1));
    QVERIFY(!MapProvider::isValidZoom(QGC_MAX_MAP_ZOOM + 1));
}

void MapProviderTest::_testCapabilities()
{
    MockMapProvider provider;

    // Test capability detection
    QVERIFY(provider.hasCapability(ProviderCapability::Street));
    QVERIFY(provider.hasCapability(ProviderCapability::RequiresToken));
    QVERIFY(!provider.hasCapability(ProviderCapability::Elevation));
    QVERIFY(!provider.hasCapability(ProviderCapability::Satellite));

    // Test capabilities() returns correct set
    const QSet<ProviderCapability> caps = provider.capabilities();
    QCOMPARE(caps.size(), 2);
    QVERIFY(caps.contains(ProviderCapability::Street));
    QVERIFY(caps.contains(ProviderCapability::RequiresToken));
}

void MapProviderTest::_testURLCaching()
{
    MockMapProvider provider;

    // First call should cache the URL
    provider.resetCallCount();
    const QString url1 = provider.getCachedURL(10, 20, 5);
    QVERIFY(!url1.isEmpty());
    QCOMPARE(provider.getURLCallCount(), 1);

    // Second call for same tile should use cache
    const QString url2 = provider.getCachedURL(10, 20, 5);
    QCOMPARE(url1, url2);
    QCOMPARE(provider.getURLCallCount(), 1); // Still 1, used cache

    // Different tile should generate new URL
    const QString url3 = provider.getCachedURL(11, 20, 5);
    QVERIFY(!url3.isEmpty());
    QCOMPARE(provider.getURLCallCount(), 2); // Cache miss

    // Clear cache and verify it regenerates
    provider.clearURLCache();
    const QString url4 = provider.getCachedURL(10, 20, 5);
    QCOMPARE(url1, url4);
    QCOMPARE(provider.getURLCallCount(), 3); // Cache was cleared
}

void MapProviderTest::_testHealthTracking()
{
    MockMapProvider provider;

    // Initial state
    const ProviderHealth& health = provider.getHealth();
    QCOMPARE(health.successCount, 0);
    QCOMPARE(health.failureCount, 0);
    QVERIFY(health.isHealthy()); // Should be healthy with no data

    // Record successes
    provider.recordSuccess();
    provider.recordSuccess();
    QCOMPARE(provider.getHealth().successCount, 2);
    QCOMPARE(provider.getHealth().failureCount, 0);
    QVERIFY(provider.getHealth().isHealthy());
    QCOMPARE(provider.getHealth().successRate(), 1.0);

    // Record failures
    provider.recordFailure();
    QCOMPARE(provider.getHealth().successCount, 2);
    QCOMPARE(provider.getHealth().failureCount, 1);
    QVERIFY(provider.getHealth().isHealthy()); // 2/3 = 66%
    QVERIFY(provider.getHealth().successRate() > 0.6);

    // Record more failures to make unhealthy
    provider.recordFailure();
    provider.recordFailure();
    QCOMPARE(provider.getHealth().failureCount, 3);
    QVERIFY(!provider.getHealth().isHealthy()); // 2/5 = 40%
}

void MapProviderTest::_testMetadata()
{
    MockMapProvider provider;

    const ProviderMetadata meta = provider.metadata();
    QVERIFY(!meta.name.isEmpty());
    QVERIFY(!meta.description.isEmpty());
    QCOMPARE(meta.minZoom, 0);
    QCOMPARE(meta.maxZoom, QGC_MAX_MAP_ZOOM);
}

void MapProviderTest::_testRateLimiting()
{
    MockMapProvider provider;

    // Should be able to make initial requests
    for (int i = 0; i < 10; i++) {
        QVERIFY(provider.canMakeRequest());
        provider.recordRequest();
    }

    // Should be rate limited after 10 requests per second
    QVERIFY(!provider.canMakeRequest());

    // Wait a bit and verify rate limit clears
    QTest::qWait(1100); // Wait >1 second
    QVERIFY(provider.canMakeRequest());
}

void MapProviderTest::_testTileValidation()
{
    // Valid PNG
    QByteArray pngData("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
    pngData.append(QString("x").repeated(200)); // Pad to valid size
    QVERIFY(TileValidator::isValidTile(pngData, "png"));

    // Valid JPEG
    QByteArray jpegData("\xFF\xD8\xFF");
    jpegData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidTile(jpegData, "jpg"));

    // Invalid - too small
    QByteArray tooSmall("tiny");
    QVERIFY(!TileValidator::isValidTile(tooSmall, "png"));

    // Invalid - wrong signature
    QByteArray wrongSig("INVALID_DATA");
    wrongSig.append(QString("x").repeated(200));
    QVERIFY(!TileValidator::isValidTile(wrongSig, "png"));

    // Expiration tests
    QDateTime old = QDateTime::currentDateTime().addDays(-31);
    QVERIFY(TileValidator::isExpired(old, 30));

    QDateTime recent = QDateTime::currentDateTime().addDays(-5);
    QVERIFY(!TileValidator::isExpired(recent, 30));
}

void MapProviderTest::_testFactory()
{
    // Register a mock provider
    bool registered = MapProviderFactory::registerProvider("mock", []() -> MapProvider* {
        return new MockMapProvider();
    });
    QVERIFY(registered);

    // Can't register same type twice
    bool duplicate = MapProviderFactory::registerProvider("mock", []() -> MapProvider* {
        return new MockMapProvider();
    });
    QVERIFY(!duplicate);

    // Create provider
    MapProvider* provider = MapProviderFactory::create("mock");
    QVERIFY(provider != nullptr);
    delete provider;

    // Unknown type returns null
    MapProvider* unknown = MapProviderFactory::create("nonexistent");
    QVERIFY(unknown == nullptr);

    // Check available providers
    QStringList available = MapProviderFactory::availableProviders();
    QVERIFY(available.contains("mock"));
}

void MapProviderTest::_testFallbackChain()
{
    MockMapProvider primary;
    MockMapProvider fallback1;
    MockMapProvider fallback2;

    ProviderFallbackChain chain;
    chain.setPrimaryProvider(&primary);
    chain.addFallback(&fallback1);
    chain.addFallback(&fallback2);

    // Should use primary when healthy
    QString url = chain.getTileURL(10, 20, 5);
    QVERIFY(!url.isEmpty());
    QVERIFY(url.contains("mock.test"));

    // Make primary unhealthy
    for (int i = 0; i < 10; i++) {
        primary.recordFailure();
    }
    QVERIFY(!primary.getHealth().isHealthy());

    // Should fallback to fallback1
    url = chain.getTileURL(11, 20, 5);
    QVERIFY(!url.isEmpty());

    // Make all unhealthy
    for (int i = 0; i < 10; i++) {
        fallback1.recordFailure();
        fallback2.recordFailure();
    }

    // Should return empty when all fail
    fallback1.setShouldReturnEmpty(true);
    fallback2.setShouldReturnEmpty(true);
    url = chain.getTileURL(12, 20, 5);
    QVERIFY(url.isEmpty());
}

void MapProviderTest::_testCoordinateCache()
{
    MockMapProvider provider;

    // Access coordinate cache and verify it's working
    CoordinateTransformCache& cache = MapProvider::coordinateCache();

    // Initial state - cache should be empty or small
    const size_t initialSize = cache.size();

    // Convert some coordinates - should populate cache
    const int x1 = provider.long2tileX(-122.4194, 10);
    const int y1 = provider.lat2tileY(37.7749, 10);
    QVERIFY(x1 >= 0);
    QVERIFY(y1 >= 0);

    // Cache should have grown
    QVERIFY(cache.size() > initialSize);

    // Same coordinate should hit cache (size shouldn't change)
    const size_t sizeAfterFirst = cache.size();
    const int x2 = provider.long2tileX(-122.4194, 10);
    const int y2 = provider.lat2tileY(37.7749, 10);
    QCOMPARE(x1, x2);
    QCOMPARE(y1, y2);
    QCOMPARE(cache.size(), sizeAfterFirst);

    // Different coordinate should miss cache
    const int x3 = provider.long2tileX(0.0, 10);
    QVERIFY(cache.size() > sizeAfterFirst);

    // Clear cache
    cache.clear();
    QCOMPARE(cache.size(), 0u);
}

void MapProviderTest::_testMemoryMonitoring()
{
    MockMapProvider provider;

    // Get initial stats
    CacheStats stats = provider.getCacheStats();
    QVERIFY(stats.urlCacheSize >= 0);
    QVERIFY(stats.coordinateCacheSize >= 0);

    // Generate some URLs to populate cache
    for (int i = 0; i < 10; i++) {
        provider.getCachedURL(i, i, 5);
    }

    // Stats should show increased usage
    stats = provider.getCacheStats();
    QVERIFY(stats.urlCacheSize > 0);
    QVERIFY(stats.totalMemoryUsage() > 0);

    // Test over-limit detection
    QVERIFY(!stats.isOverLimit(1000000)); // 1MB limit - should be fine
    QVERIFY(stats.isOverLimit(10));       // 10 byte limit - should be over

    // Test cache clearing on limit
    provider.clearCachesIfOverLimit(10); // Very low limit should trigger clear
    stats = provider.getCacheStats();
    // URL cache should be cleared
    QCOMPARE(stats.urlCacheSize, 0u);
}

void MapProviderTest::_testErrorTracking()
{
    ErrorTracker& tracker = MapProvider::errorTracker();
    tracker.clear();

    // Record various error types
    tracker.recordError(ErrorReport(TileErrorType::NetworkTimeout, "TestProvider", "Timeout occurred", 10, 20, 5));
    tracker.recordError(ErrorReport(TileErrorType::InvalidToken, "TestProvider", "Bad token", 11, 21, 5));
    tracker.recordError(ErrorReport(TileErrorType::RateLimited, "TestProvider", "Too many requests", 12, 22, 5));
    tracker.recordError(ErrorReport(TileErrorType::NetworkTimeout, "TestProvider", "Another timeout", 13, 23, 5));

    // Verify error counts
    QCOMPARE(tracker.errorCountByType(TileErrorType::NetworkTimeout), 2);
    QCOMPARE(tracker.errorCountByType(TileErrorType::InvalidToken), 1);
    QCOMPARE(tracker.errorCountByType(TileErrorType::RateLimited), 1);
    QCOMPARE(tracker.errorCountByType(TileErrorType::ServerError), 0);

    // Test querying by type
    QList<ErrorReport> timeoutErrors = tracker.getErrorsByType(TileErrorType::NetworkTimeout);
    QCOMPARE(timeoutErrors.size(), 2);
    QVERIFY(timeoutErrors[0].providerName == "TestProvider");

    // Test querying by provider
    QList<ErrorReport> providerErrors = tracker.getErrorsByProvider("TestProvider");
    QCOMPARE(providerErrors.size(), 4);

    // Test getting recent errors
    QList<ErrorReport> recent = tracker.getRecentErrors(2);
    QCOMPARE(recent.size(), 2);
    QCOMPARE(recent[0].type, TileErrorType::NetworkTimeout); // Most recent

    // Test clear
    tracker.clear();
    QCOMPARE(tracker.errorCountByType(TileErrorType::NetworkTimeout), 0);
}

void MapProviderTest::_testHealthDashboard()
{
    ProviderHealthDashboard& dashboard = MapProvider::healthDashboard();

    // Create health data for providers
    ProviderHealth health1;
    health1.successCount = 10;
    health1.failureCount = 2;

    ProviderHealth health2;
    health2.successCount = 5;
    health2.failureCount = 10;

    // Update dashboard
    dashboard.updateProviderHealth("Provider1", health1);
    dashboard.updateProviderHealth("Provider2", health2);

    // Record some errors
    dashboard.recordProviderError("Provider1", TileErrorType::NetworkTimeout);
    dashboard.recordProviderError("Provider2", TileErrorType::InvalidToken);
    dashboard.recordProviderError("Provider2", TileErrorType::RateLimited);

    // Get all health info
    QList<ProviderHealthDashboard::ProviderHealthInfo> allHealth = dashboard.getAllProviderHealth();
    QCOMPARE(allHealth.size(), 2);

    // Test best/worst provider
    ProviderHealthDashboard::ProviderHealthInfo best = dashboard.getBestProvider();
    QCOMPARE(best.providerName, QString("Provider1"));
    QVERIFY(best.health.successRate() > 0.8);

    ProviderHealthDashboard::ProviderHealthInfo worst = dashboard.getWorstProvider();
    QCOMPARE(worst.providerName, QString("Provider2"));
    QVERIFY(worst.health.successRate() < 0.5);

    // Test QML export
    QVariantMap summary = dashboard.healthSummaryForQML();
    QVERIFY(summary.contains("providers"));
    QVERIFY(summary.contains("totalProviders"));
    QCOMPARE(summary["totalProviders"].toInt(), 2);

    QVariantList providers = summary["providers"].toList();
    QCOMPARE(providers.size(), 2);

    // Verify provider data structure
    QVariantMap provider1Data = providers[0].toMap();
    QVERIFY(provider1Data.contains("name"));
    QVERIFY(provider1Data.contains("successCount"));
    QVERIFY(provider1Data.contains("failureCount"));
    QVERIFY(provider1Data.contains("successRate"));
    QVERIFY(provider1Data.contains("isHealthy"));
    QVERIFY(provider1Data.contains("errorCount"));
}

void MapProviderTest::_testEnhancedFormatDetection()
{
    // Test PNG detection
    QByteArray pngData("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
    pngData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidPNG(pngData));
    QCOMPARE(TileValidator::detectImageFormat(pngData), QString("png"));

    // Test JPEG detection
    QByteArray jpegData("\xFF\xD8\xFF");
    jpegData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidJPEG(jpegData));
    QCOMPARE(TileValidator::detectImageFormat(jpegData), QString("jpg"));

    // Test GIF detection
    QByteArray gifData("GIF89a");
    gifData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidGIF(gifData));
    QCOMPARE(TileValidator::detectImageFormat(gifData), QString("gif"));

    // Test WebP detection
    QByteArray webpData("RIFF\x00\x00\x00\x00WEBP");
    webpData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidWebP(webpData));
    QCOMPARE(TileValidator::detectImageFormat(webpData), QString("webp"));

    // Test BMP detection
    QByteArray bmpData("BM");
    bmpData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidBMP(bmpData));
    QCOMPARE(TileValidator::detectImageFormat(bmpData), QString("bmp"));

    // Test AVIF detection
    QByteArray avifData("\x00\x00\x00\x20" "ftyp" "avif");
    avifData.append(QString("x").repeated(200));
    QVERIFY(TileValidator::isValidAVIF(avifData));
    QCOMPARE(TileValidator::detectImageFormat(avifData), QString("avif"));

    // Test invalid data
    QByteArray invalidData("INVALID");
    QVERIFY(!TileValidator::isValidPNG(invalidData));
    QVERIFY(!TileValidator::isValidJPEG(invalidData));
    QCOMPARE(TileValidator::detectImageFormat(invalidData), QString());
}

void MapProviderTest::_testMetadataValidation()
{
    // Valid metadata
    ProviderMetadata validMeta("TestProvider", "Test Description", "© Test 2024");
    validMeta.minZoom = 0;
    validMeta.maxZoom = 20;
    QVERIFY(validMeta.isValid());

    // Invalid - empty name
    ProviderMetadata emptyName("", "Description", "Attribution");
    QVERIFY(!emptyName.isValid());

    // Invalid - empty description
    ProviderMetadata emptyDesc("Name", "", "Attribution");
    QVERIFY(!emptyDesc.isValid());

    // Invalid - bad zoom range
    ProviderMetadata badZoom("Name", "Desc", "Attr");
    badZoom.minZoom = 10;
    badZoom.maxZoom = 5;
    QVERIFY(!badZoom.isValid());

    // Invalid - zoom out of bounds
    ProviderMetadata outOfBounds("Name", "Desc", "Attr");
    outOfBounds.maxZoom = 100;
    QVERIFY(!outOfBounds.isValid());

    // Valid with bounds
    ProviderMetadata withBounds("Name", "Desc", "Attr");
    withBounds.hasBoundsLimits = true;
    withBounds.bounds = GeographicBounds(-45.0, -34.0, 166.0, 179.0);
    QVERIFY(withBounds.isValid());

    // Invalid - bad bounds
    ProviderMetadata badBounds("Name", "Desc", "Attr");
    badBounds.hasBoundsLimits = true;
    badBounds.bounds = GeographicBounds(40.0, 30.0, 170.0, 180.0);  // minLat > maxLat
    QVERIFY(!badBounds.isValid());
}

void MapProviderTest::_testAttributionManagement()
{
    AttributionManager& mgr = MapProvider::attributionManager();

    // Clear any existing attributions
    for (const auto& info : mgr.getAllAttributions()) {
        mgr.unregisterAttribution(info.providerName);
    }

    // Register attributions
    mgr.registerAttribution("Provider1", "© Provider 1", "http://provider1.com/tos");
    mgr.registerAttribution("Provider2", "© Provider 2", "http://provider2.com/tos");

    QVERIFY(mgr.hasAttribution("Provider1"));
    QVERIFY(mgr.hasAttribution("Provider2"));
    QVERIFY(!mgr.hasAttribution("Provider3"));

    // Get all attributions
    QList<AttributionManager::AttributionInfo> all = mgr.getAllAttributions();
    QCOMPARE(all.size(), 2);

    // Get combined attribution
    QString combined = mgr.getCombinedAttribution(" | ");
    QVERIFY(combined.contains("Provider 1"));
    QVERIFY(combined.contains("Provider 2"));

    // Get required attributions
    QStringList required = mgr.getRequiredAttributions();
    QCOMPARE(required.size(), 2);

    // Unregister
    mgr.unregisterAttribution("Provider1");
    QVERIFY(!mgr.hasAttribution("Provider1"));
    QCOMPARE(mgr.getAllAttributions().size(), 1);

    // Cleanup
    mgr.unregisterAttribution("Provider2");
}

void MapProviderTest::_testGeographicBounds()
{
    // Test bounds creation and validation
    GeographicBounds validBounds(-45.0, -34.0, 166.0, 179.0);
    QVERIFY(validBounds.isValid());
    QVERIFY(validBounds.contains(-40.0, 170.0));
    QVERIFY(!validBounds.contains(0.0, 0.0));

    // Test invalid bounds (minLat > maxLat)
    GeographicBounds invalidLat(40.0, 30.0, 170.0, 180.0);
    QVERIFY(!invalidLat.isValid());

    // Test invalid bounds (minLon > maxLon)
    GeographicBounds invalidLon(-40.0, -30.0, 180.0, 170.0);
    QVERIFY(!invalidLon.isValid());

    // Test out of range bounds
    GeographicBounds outOfRange(-100.0, 100.0, -200.0, 200.0);
    QVERIFY(!outOfRange.isValid());

    // Test provider bounds checking
    MockMapProvider provider;
    QVERIFY(provider.isWithinBounds(0.0, 0.0));  // No bounds by default
}

void MapProviderTest::_testTileCoordinateValidation()
{
    MockMapProvider provider;

    // Valid coordinates at zoom 10
    QVERIFY(provider.isValidTileCoordinate(0, 0, 10));
    QVERIFY(provider.isValidTileCoordinate(512, 512, 10));
    QVERIFY(provider.isValidTileCoordinate(1023, 1023, 10));

    // Invalid - out of range
    QVERIFY(!provider.isValidTileCoordinate(1024, 0, 10));
    QVERIFY(!provider.isValidTileCoordinate(0, 1024, 10));
    QVERIFY(!provider.isValidTileCoordinate(-1, 0, 10));

    // Invalid zoom
    QVERIFY(!provider.isValidTileCoordinate(0, 0, -1));
    QVERIFY(!provider.isValidTileCoordinate(0, 0, 100));

    // Check zoom limits affect getTileURL
    QUrl url1 = provider.getTileURL(0, 0, 10);
    QVERIFY(!url1.isEmpty());

    QUrl url2 = provider.getTileURL(0, 0, 100);  // Invalid zoom
    QVERIFY(url2.isEmpty());
}

void MapProviderTest::_testTileSizeValidation()
{
    MockMapProvider provider;

    // Valid PNG data
    QByteArray validData("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
    validData.append(QString("x").repeated(200));
    QVERIFY(provider.isValidTileData(validData));

    // Too small
    QByteArray tooSmall("tiny");
    QVERIFY(!provider.isValidTileData(tooSmall));

    // Too large (> 5MB)
    QByteArray tooLarge(6000000, 'x');
    QVERIFY(!provider.isValidTileData(tooLarge));

    // Invalid format
    QByteArray invalidFormat("NOTANIMAGE");
    invalidFormat.append(QString("x").repeated(200));
    QVERIFY(!provider.isValidTileData(invalidFormat));

    // Test size limits
    QVERIFY(TileValidator::hasValidSize(validData));
    QVERIFY(!TileValidator::hasValidSize(tooSmall));
    QVERIFY(!TileValidator::hasValidSize(tooLarge));
}

// Phase 5 & Refactoring tests
void MapProviderTest::_testThreadSafetyURLCache()
{
    MockMapProvider provider;

    // Spawn multiple threads all accessing URL cache
    QVector<QFuture<void>> futures;
    std::atomic<int> successCount(0);

    for (int thread = 0; thread < 10; thread++) {
        futures.append(QtConcurrent::run([&provider, &successCount, thread]() {
            for (int i = 0; i < 100; i++) {
                // Each thread requests different tiles
                const QString url = provider.getCachedURL(thread * 100 + i, i, 5);
                if (!url.isEmpty()) {
                    successCount++;
                }
            }
        }));
    }

    // Wait for all threads
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify all requests succeeded
    QCOMPARE(successCount.load(), 1000);

    // Verify cache didn't crash and has reasonable size
    QVERIFY(provider.getCacheStats().urlCacheSize <= QGC_URL_CACHE_SIZE);
}

void MapProviderTest::_testThreadSafetyHealth()
{
    MockMapProvider provider;

    // Spawn multiple threads recording success/failure
    QVector<QFuture<void>> futures;

    for (int thread = 0; thread < 10; thread++) {
        futures.append(QtConcurrent::run([&provider, thread]() {
            for (int i = 0; i < 100; i++) {
                if ((thread + i) % 2 == 0) {
                    provider.recordSuccess();
                } else {
                    provider.recordFailure();
                }
            }
        }));
    }

    // Wait for all threads
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Verify counts are accurate (1000 total operations, 500 success, 500 failure)
    ProviderHealth health = provider.getHealth();
    QCOMPARE(health.successCount + health.failureCount, 1000);
    QCOMPARE(health.successCount, 500);
    QCOMPARE(health.failureCount, 500);
}

void MapProviderTest::_testThreadSafetyRateLimiter()
{
    RateLimiter limiter(100); // 100 requests per second
    const QString providerName = "TestProvider";

    // Spawn multiple threads making requests
    QVector<QFuture<void>> futures;
    std::atomic<int> allowedCount(0);

    for (int thread = 0; thread < 10; thread++) {
        futures.append(QtConcurrent::run([&limiter, &allowedCount, providerName]() {
            for (int i = 0; i < 50; i++) {
                if (limiter.canMakeRequest(providerName)) {
                    limiter.recordRequest(providerName);
                    allowedCount++;
                }
            }
        }));
    }

    // Wait for all threads
    for (auto& future : futures) {
        future.waitForFinished();
    }

    // Should have limited to 100 requests per second
    QVERIFY(allowedCount.load() <= 100);
    qDebug() << "Rate limiter allowed" << allowedCount.load() << "of 500 requests";
}

void MapProviderTest::_testRequestGuardRAII()
{
    ConcurrentRequestLimiter& limiter = MapProvider::concurrentLimiter();
    const QString providerName = "TestProvider";

    // Reset state
    limiter.reset(providerName);
    QCOMPARE(limiter.activeRequests(providerName), 0);

    {
        // Create guard - should start request
        RequestGuard guard(limiter, providerName);
        QVERIFY(guard.isActive());
        QCOMPARE(limiter.activeRequests(providerName), 1);

        {
            // Nested guard
            RequestGuard guard2(limiter, providerName);
            QVERIFY(guard2.isActive());
            QCOMPARE(limiter.activeRequests(providerName), 2);
        }

        // Guard2 destroyed, count should decrease
        QCOMPARE(limiter.activeRequests(providerName), 1);
    }

    // Both guards destroyed, count should be 0
    QCOMPARE(limiter.activeRequests(providerName), 0);
}

void MapProviderTest::_testConcurrentRequestLimiter()
{
    ConcurrentRequestLimiter limiter(3); // Max 3 concurrent
    const QString providerName = "TestProvider";

    // Start 3 requests - should all succeed
    QVERIFY(limiter.canStartRequest(providerName));
    limiter.startRequest(providerName);

    QVERIFY(limiter.canStartRequest(providerName));
    limiter.startRequest(providerName);

    QVERIFY(limiter.canStartRequest(providerName));
    limiter.startRequest(providerName);

    QCOMPARE(limiter.activeRequests(providerName), 3);

    // 4th request should fail
    QVERIFY(!limiter.canStartRequest(providerName));

    // Finish one request
    limiter.finishRequest(providerName);
    QCOMPARE(limiter.activeRequests(providerName), 2);

    // Now 4th request should succeed
    QVERIFY(limiter.canStartRequest(providerName));
}

void MapProviderTest::_testRetryIntegration()
{
    MockMapProvider provider;
    RetryManager& retryMgr = MapProvider::retryManager();
    retryMgr.clear();

    // Simulate failed tile request
    const int x = 10, y = 20, zoom = 5;

    // First failure - should retry
    QVERIFY(provider.shouldRetryTile(x, y, zoom, TileErrorType::NetworkTimeout));

    // Get retry delay and verify exponential backoff
    int delay1 = provider.getRetryDelay(x, y, zoom);
    QVERIFY(delay1 >= 100); // Initial delay

    // Second failure - should still retry
    QVERIFY(provider.shouldRetryTile(x, y, zoom, TileErrorType::NetworkTimeout));
    int delay2 = provider.getRetryDelay(x, y, zoom);
    QVERIFY(delay2 > delay1); // Exponential backoff

    // Third failure - should still retry
    QVERIFY(provider.shouldRetryTile(x, y, zoom, TileErrorType::NetworkTimeout));

    // Fourth failure - should NOT retry (max 3 attempts)
    QVERIFY(!provider.shouldRetryTile(x, y, zoom, TileErrorType::NetworkTimeout));

    // Non-retryable error - should not retry
    QVERIFY(!provider.shouldRetryTile(15, 25, 5, TileErrorType::InvalidToken));
}

void MapProviderTest::_testMetricsIntegration()
{
    MockMapProvider provider;
    MetricsCollector& metrics = MapProvider::metricsCollector();
    metrics.reset();

    const QString providerName = provider.getMapName();

    // Simulate successful tile request
    provider.beginTileRequest(10, 20, 5, false);
    provider.completeTileRequest(10, 20, 5, true, 15000, 125.5);

    // Simulate cached request
    provider.beginTileRequest(11, 21, 5, true);
    provider.completeTileRequest(11, 21, 5, true, 15000, 0);

    // Simulate failed request
    provider.beginTileRequest(12, 22, 5, false);
    provider.completeTileRequest(12, 22, 5, false, 0, 500.0, TileErrorType::NetworkTimeout);

    // Check metrics
    ProviderMetrics providerMetrics = metrics.getMetrics(providerName);
    QCOMPARE(providerMetrics.totalRequests, static_cast<quint64>(3));
    QCOMPARE(providerMetrics.successfulRequests, static_cast<quint64>(2));
    QCOMPARE(providerMetrics.failedRequests, static_cast<quint64>(1));
    QCOMPARE(providerMetrics.cachedRequests, static_cast<quint64>(1));
    QVERIFY(providerMetrics.bytesDownloaded > 0);
    QVERIFY(providerMetrics.averageResponseTimeMs > 0);

    // Test success rate
    QVERIFY(qAbs(providerMetrics.successRate() - 0.666) < 0.01);

    // Test cache hit rate
    QVERIFY(qAbs(providerMetrics.cacheHitRate() - 0.333) < 0.01);
}

void MapProviderTest::_testIntegrationWorkflow()
{
    MockMapProvider provider;

    // Test canStartTileRequest
    QVERIFY(provider.canStartTileRequest());

    // Use RequestGuard for automatic cleanup
    {
        RequestGuard guard(MapProvider::concurrentLimiter(), provider.getMapName());
        QVERIFY(guard.isActive());

        // Begin request
        provider.beginTileRequest(10, 20, 5, false);

        // Simulate successful fetch
        provider.completeTileRequest(10, 20, 5, true, 12345, 150.0);

        // Verify health improved
        QVERIFY(provider.getHealth().successCount > 0);
    }

    // Verify guard cleaned up
    QCOMPARE(MapProvider::concurrentLimiter().activeRequests(provider.getMapName()), 0);

    // Test failure workflow
    {
        RequestGuard guard(MapProvider::concurrentLimiter(), provider.getMapName());

        provider.beginTileRequest(11, 21, 5, false);
        provider.completeTileRequest(11, 21, 5, false, 0, 1000.0, TileErrorType::ServerError);

        // Check if should retry
        if (provider.shouldRetryTile(11, 21, 5, TileErrorType::ServerError)) {
            int delay = provider.getRetryDelay(11, 21, 5);
            QVERIFY(delay >= 100);
        }
    }
}

void MapProviderTest::_testMapProviderManager()
{
    // Verify singleton works
    MapProviderManager* mgr1 = MapProviderManager::instance();
    MapProviderManager* mgr2 = MapProviderManager::instance();
    QCOMPARE(mgr1, mgr2);

    // Verify all managers are accessible
    QVERIFY(&mgr1->rateLimiter() != nullptr);
    QVERIFY(&mgr1->coordinateCache() != nullptr);
    QVERIFY(&mgr1->errorTracker() != nullptr);
    QVERIFY(&mgr1->healthDashboard() != nullptr);
    QVERIFY(&mgr1->attributionManager() != nullptr);
    QVERIFY(&mgr1->concurrentLimiter() != nullptr);
    QVERIFY(&mgr1->retryManager() != nullptr);
    QVERIFY(&mgr1->metricsCollector() != nullptr);

    // Verify static accessors work
    QCOMPARE(&MapProvider::coordinateCache(), &mgr1->coordinateCache());
    QCOMPARE(&MapProvider::errorTracker(), &mgr1->errorTracker());
    QCOMPARE(&MapProvider::healthDashboard(), &mgr1->healthDashboard());
}

// Phase 7 - Provider Rotation & Load Balancing tests
void MapProviderTest::_testProviderRotationRoundRobin()
{
    // Create test providers
    MockMapProvider provider1, provider2, provider3;
    QList<MapProvider*> providers = {&provider1, &provider2, &provider3};

    // Test round-robin rotation
    ProviderRotationStrategy strategy(ProviderRotationStrategy::Strategy::RoundRobin);

    MapProvider* selected1 = strategy.selectProvider(providers);
    QCOMPARE(selected1, &provider1);

    MapProvider* selected2 = strategy.selectProvider(providers);
    QCOMPARE(selected2, &provider2);

    MapProvider* selected3 = strategy.selectProvider(providers);
    QCOMPARE(selected3, &provider3);

    // Should wrap around to first provider
    MapProvider* selected4 = strategy.selectProvider(providers);
    QCOMPARE(selected4, &provider1);
}

void MapProviderTest::_testProviderRotationWeightedHealth()
{
    // Create providers with different health scores
    MockMapProvider provider1, provider2, provider3;

    // Give provider1 great health
    for (int i = 0; i < 10; i++) provider1.recordSuccess();

    // Give provider2 mediocre health
    for (int i = 0; i < 5; i++) provider2.recordSuccess();
    for (int i = 0; i < 5; i++) provider2.recordFailure();

    // Give provider3 poor health
    for (int i = 0; i < 2; i++) provider3.recordSuccess();
    for (int i = 0; i < 8; i++) provider3.recordFailure();

    QList<MapProvider*> providers = {&provider1, &provider2, &provider3};

    // Test weighted health strategy
    ProviderRotationStrategy strategy(ProviderRotationStrategy::Strategy::WeightedHealth);

    // Run multiple selections - provider1 should be selected most often
    QMap<MapProvider*, int> selectionCounts;
    for (int i = 0; i < 100; i++) {
        MapProvider* selected = strategy.selectProvider(providers);
        selectionCounts[selected]++;
    }

    // Provider1 (100% health) should be selected more than provider3 (20% health)
    QVERIFY(selectionCounts[&provider1] > selectionCounts[&provider3]);
    qDebug() << "Weighted selection counts:" << selectionCounts[&provider1]
             << selectionCounts[&provider2] << selectionCounts[&provider3];
}

void MapProviderTest::_testProviderRotationLeastConnections()
{
    MockMapProvider provider1, provider2, provider3;
    QList<MapProvider*> providers = {&provider1, &provider2, &provider3};

    // Simulate different concurrent request loads
    ConcurrentRequestLimiter& limiter = MapProvider::concurrentLimiter();
    limiter.reset(provider1.getMapName());
    limiter.reset(provider2.getMapName());
    limiter.reset(provider3.getMapName());

    // Provider1: 3 active requests
    limiter.startRequest(provider1.getMapName());
    limiter.startRequest(provider1.getMapName());
    limiter.startRequest(provider1.getMapName());

    // Provider2: 1 active request
    limiter.startRequest(provider2.getMapName());

    // Provider3: 0 active requests

    // Test least connections strategy
    ProviderRotationStrategy strategy(ProviderRotationStrategy::Strategy::LeastConnections);

    // Should select provider3 (0 connections)
    MapProvider* selected1 = strategy.selectProvider(providers);
    QCOMPARE(selected1, &provider3);

    // Add connection to provider3
    limiter.startRequest(provider3.getMapName());

    // Should now select provider2 (1 connection) over provider3 (1 connection, but checked first)
    // or provider1 (3 connections)
    MapProvider* selected2 = strategy.selectProvider(providers);
    QVERIFY(selected2 == &provider2 || selected2 == &provider3);
    QVERIFY(selected2 != &provider1);

    // Cleanup
    limiter.reset(provider1.getMapName());
    limiter.reset(provider2.getMapName());
    limiter.reset(provider3.getMapName());
}

void MapProviderTest::_testProviderRotationRandom()
{
    MockMapProvider provider1, provider2, provider3;
    QList<MapProvider*> providers = {&provider1, &provider2, &provider3};

    // Test random strategy
    ProviderRotationStrategy strategy(ProviderRotationStrategy::Strategy::Random);

    // Run multiple selections and verify distribution
    QMap<MapProvider*, int> selectionCounts;
    for (int i = 0; i < 90; i++) {
        MapProvider* selected = strategy.selectProvider(providers);
        selectionCounts[selected]++;
    }

    // Each provider should be selected at least once (statistically very likely with 90 selections)
    QVERIFY(selectionCounts[&provider1] > 0);
    QVERIFY(selectionCounts[&provider2] > 0);
    QVERIFY(selectionCounts[&provider3] > 0);

    qDebug() << "Random selection counts:" << selectionCounts[&provider1]
             << selectionCounts[&provider2] << selectionCounts[&provider3];
}

void MapProviderTest::_testLoadBalancerBasic()
{
    MockMapProvider provider1, provider2, provider3;

    // Create load balancer
    LoadBalancer balancer(0.5, true);

    // Add providers
    balancer.addProvider(&provider1, 1);
    balancer.addProvider(&provider2, 2);
    balancer.addProvider(&provider3, 1);

    QCOMPARE(balancer.totalProviders(), 3);
    QVERIFY(balancer.hasProvider(&provider1));
    QVERIFY(balancer.hasProvider(&provider2));
    QVERIFY(balancer.hasProvider(&provider3));

    // Check weights
    QCOMPARE(balancer.getProviderWeight(&provider1), 1);
    QCOMPARE(balancer.getProviderWeight(&provider2), 2);

    // All providers start healthy
    QVERIFY(balancer.healthyProviderCount() == 3);

    // Disable a provider
    balancer.enableProvider(&provider3, false);
    QVERIFY(!balancer.isProviderEnabled(&provider3));

    // Remove a provider
    balancer.removeProvider(&provider2);
    QCOMPARE(balancer.totalProviders(), 2);
    QVERIFY(!balancer.hasProvider(&provider2));
}

void MapProviderTest::_testLoadBalancerAutoFailover()
{
    MockMapProvider provider1, provider2, provider3;

    // Give providers different health scores
    for (int i = 0; i < 10; i++) provider1.recordSuccess();  // 100% health
    for (int i = 0; i < 3; i++) provider2.recordSuccess();   // 30% health
    for (int i = 0; i < 7; i++) provider2.recordFailure();
    for (int i = 0; i < 1; i++) provider3.recordSuccess();   // 10% health
    for (int i = 0; i < 9; i++) provider3.recordFailure();

    // Create load balancer with 50% health threshold
    LoadBalancer balancer(0.5, true);
    balancer.addProvider(&provider1);
    balancer.addProvider(&provider2);
    balancer.addProvider(&provider3);

    // With auto-failover enabled, only healthy providers should be returned
    QList<MapProvider*> healthy = balancer.getHealthyProviders();
    QCOMPARE(healthy.size(), 1);
    QVERIFY(healthy.contains(&provider1));

    // Get next provider - should only select provider1
    for (int i = 0; i < 10; i++) {
        MapProvider* selected = balancer.getNextProvider();
        QCOMPARE(selected, &provider1);
    }

    // Disable auto-failover
    balancer.enableAutoFailover(false);

    // Now all providers should be candidates
    MapProvider* selected = balancer.getNextProvider();
    QVERIFY(selected != nullptr);  // Should get one of the providers

    // Change health threshold to 0.2 (20%)
    balancer.enableAutoFailover(true);
    balancer.setMinHealthThreshold(0.2);

    // Now provider1 and provider2 should be healthy
    healthy = balancer.getHealthyProviders();
    QCOMPARE(healthy.size(), 2);
}

void MapProviderTest::_testLoadBalancerStatistics()
{
    MockMapProvider provider1, provider2, provider3;

    // Give different health scores
    for (int i = 0; i < 10; i++) provider1.recordSuccess();
    for (int i = 0; i < 5; i++) provider2.recordSuccess();
    for (int i = 0; i < 5; i++) provider2.recordFailure();
    for (int i = 0; i < 2; i++) provider3.recordSuccess();
    for (int i = 0; i < 8; i++) provider3.recordFailure();

    LoadBalancer balancer(0.5, true);
    balancer.addProvider(&provider1);
    balancer.addProvider(&provider2);
    balancer.addProvider(&provider3);

    // Test statistics
    QCOMPARE(balancer.totalProviders(), 3);
    QCOMPARE(balancer.healthyProviderCount(), 2);  // provider1 (100%) and provider2 (50%)

    double avgHealth = balancer.averageHealthScore();
    QVERIFY(avgHealth > 0.5);  // (1.0 + 0.5 + 0.2) / 3 = 0.566...
    QVERIFY(avgHealth < 0.7);

    // Test QML statistics export
    QVariantMap stats = balancer.getStatisticsForQML();
    QVERIFY(stats.contains("totalProviders"));
    QVERIFY(stats.contains("healthyProviders"));
    QVERIFY(stats.contains("averageHealth"));
    QVERIFY(stats.contains("minHealthThreshold"));
    QVERIFY(stats.contains("autoFailover"));
    QVERIFY(stats.contains("strategy"));
    QVERIFY(stats.contains("providers"));

    QCOMPARE(stats["totalProviders"].toInt(), 3);
    QCOMPARE(stats["healthyProviders"].toInt(), 2);

    QVariantList providersList = stats["providers"].toList();
    QCOMPARE(providersList.size(), 3);

    // Verify provider data structure
    QVariantMap provider1Data = providersList[0].toMap();
    QVERIFY(provider1Data.contains("name"));
    QVERIFY(provider1Data.contains("weight"));
    QVERIFY(provider1Data.contains("enabled"));
    QVERIFY(provider1Data.contains("health"));
    QVERIFY(provider1Data.contains("activeRequests"));
}

QTEST_APPLESS_MAIN(MapProviderTest)
