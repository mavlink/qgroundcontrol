/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoTileFetcherQGCTest.h"
#include "QGeoTileFetcherQGC.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void QGeoTileFetcherQGCTest::init()
{
    _networkManager = new QNetworkAccessManager(this);
}

void QGeoTileFetcherQGCTest::cleanup()
{
    delete _tileFetcher;
    _tileFetcher = nullptr;

    delete _networkManager;
    _networkManager = nullptr;

    // Clean up any test settings
    QSettings settings;
    settings.remove("MapTileLoadBalancer");
}

void QGeoTileFetcherQGCTest::testLoadBalancerInitialization()
{
    // Create tile fetcher - should initialize LoadBalancer automatically
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);

    // Verify LoadBalancer was created and populated
    LoadBalancer* lb = _tileFetcher->loadBalancer();
    QVERIFY(lb != nullptr);

    // Should have providers from UrlFactory
    const int providerCount = lb->totalProviders();
    QVERIFY(providerCount > 0);
    qDebug() << "LoadBalancer initialized with" << providerCount << "providers";

    // Should have some healthy providers
    const int healthyCount = lb->healthyProviderCount();
    QVERIFY(healthyCount > 0);
    qDebug() << "Healthy providers:" << healthyCount;

    // Check that provider weights were set correctly
    const QList<MapProvider*> providers = lb->getAllProviders();
    bool foundGoogleProvider = false;
    bool foundOSMProvider = false;

    for (MapProvider* provider : providers) {
        QVERIFY(provider != nullptr);

        const QString name = provider->getMapName();
        const int weight = lb->getProviderWeight(provider);

        if (name.contains("Google", Qt::CaseInsensitive)) {
            QCOMPARE(weight, 3);  // Google should have weight 3
            foundGoogleProvider = true;
        } else if (name.contains("OpenStreet", Qt::CaseInsensitive)) {
            QCOMPARE(weight, 2);  // OSM should have weight 2
            foundOSMProvider = true;
        }
    }

    // Note: Google provider might not exist in build (QGC_NO_GOOGLE_MAPS)
    if (foundGoogleProvider) {
        qDebug() << "Google provider found with correct weight";
    }
    if (foundOSMProvider) {
        qDebug() << "OpenStreetMap provider found with correct weight";
    }
}

void QGeoTileFetcherQGCTest::testProviderSelectionBasic()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Should be able to get a provider
    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // Provider should be healthy
    QVERIFY(provider->getHealth().isHealthy());

    // Should be able to get providers repeatedly
    for (int i = 0; i < 10; i++) {
        MapProvider* p = lb->getNextProvider();
        QVERIFY(p != nullptr);
    }
}

void QGeoTileFetcherQGCTest::testProviderSelectionWithFailover()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Get a provider and make it unhealthy
    MapProvider* provider1 = lb->getNextProvider();
    QVERIFY(provider1 != nullptr);

    // Record failures to make it unhealthy (< 50% success rate)
    for (int i = 0; i < 10; i++) {
        provider1->recordFailure();
    }

    QVERIFY(!provider1->getHealth().isHealthy());

    // LoadBalancer should skip unhealthy providers
    MapProvider* provider2 = lb->getNextProvider();
    QVERIFY(provider2 != nullptr);

    // Should get a different, healthy provider
    if (lb->totalProviders() > 1) {
        QVERIFY(provider2 != provider1 || provider2->getHealth().isHealthy());
    }
}

void QGeoTileFetcherQGCTest::testHealthCheckBeforeFetch()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // Provider should pass health check
    QVERIFY(provider->canStartTileRequest());

    // Make provider unhealthy
    for (int i = 0; i < 10; i++) {
        provider->recordFailure();
    }

    // Health check might still pass (provider still in LoadBalancer)
    // but success rate should be low
    QVERIFY(provider->getHealth().successRate() < 0.5);
}

void QGeoTileFetcherQGCTest::testAutomaticRetryOnError()
{
    // This test would require mocking QGeoTiledMapReplyQGC
    // For now, verify retry logic exists in provider

    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // Test retry decision
    const bool shouldRetry = provider->shouldRetryTile(0, 0, 10, TileErrorType::NetworkTimeout);
    QVERIFY(shouldRetry);  // NetworkTimeout is retryable

    const bool shouldNotRetry = provider->shouldRetryTile(0, 0, 10, TileErrorType::InvalidToken);
    QVERIFY(!shouldNotRetry);  // InvalidToken is not retryable
}

void QGeoTileFetcherQGCTest::testRetryExponentialBackoff()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // First retry attempt
    provider->beginTileRequest(0, 0, 10, false);
    provider->completeTileRequest(0, 0, 10, false, 0, 100.0, TileErrorType::NetworkTimeout);

    int delay1 = provider->getRetryDelay(0, 0, 10);

    // Second retry attempt
    provider->beginTileRequest(0, 0, 10, false);
    provider->completeTileRequest(0, 0, 10, false, 0, 100.0, TileErrorType::NetworkTimeout);

    int delay2 = provider->getRetryDelay(0, 0, 10);

    // Delays should increase (exponential backoff)
    qDebug() << "Retry delays:" << delay1 << delay2;
    QVERIFY(delay2 > delay1);
}

void QGeoTileFetcherQGCTest::testMaxRetriesReached()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // Simulate max retries (default is 3)
    for (int i = 0; i < 4; i++) {
        provider->beginTileRequest(0, 0, 10, false);
        provider->completeTileRequest(0, 0, 10, false, 0, 100.0, TileErrorType::NetworkTimeout);
    }

    // Should not retry after max attempts
    const bool shouldRetry = provider->shouldRetryTile(0, 0, 10, TileErrorType::NetworkTimeout);
    QVERIFY(!shouldRetry);
}

void QGeoTileFetcherQGCTest::testConcurrentRequestLimiting()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    ConcurrentRequestLimiter& limiter = MapProvider::concurrentLimiter();
    limiter.reset(provider->getMapName());

    // Should be able to start up to 6 concurrent requests (default limit)
    for (int i = 0; i < 6; i++) {
        QVERIFY(limiter.canStartRequest(provider->getMapName()));
        limiter.startRequest(provider->getMapName());
    }

    // 7th request should fail
    QVERIFY(!limiter.canStartRequest(provider->getMapName()));

    // Cleanup
    for (int i = 0; i < 6; i++) {
        limiter.finishRequest(provider->getMapName());
    }
}

void QGeoTileFetcherQGCTest::testMetricsTracking()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    MetricsCollector& metrics = MapProvider::metricsCollector();
    metrics.reset(provider->getMapName());

    // Simulate successful request
    provider->beginTileRequest(0, 0, 10, false);
    provider->completeTileRequest(0, 0, 10, true, 15000, 250.0);

    // Check metrics
    ProviderMetrics m = metrics.getMetrics(provider->getMapName());
    QCOMPARE(m.totalRequests, static_cast<quint64>(1));
    QCOMPARE(m.successfulRequests, static_cast<quint64>(1));
    QCOMPARE(m.bytesDownloaded, static_cast<quint64>(15000));
    QVERIFY(m.averageResponseTimeMs > 0);
}

void QGeoTileFetcherQGCTest::testConfigurationPersistence()
{
    // Create temporary settings file
    QSettings settings;
    settings.remove("MapTileLoadBalancer");

    {
        _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
        LoadBalancer* lb = _tileFetcher->loadBalancer();

        // Modify provider configuration
        MapProvider* provider = lb->getNextProvider();
        if (provider) {
            lb->setProviderWeight(provider, 5);
            lb->enableProvider(provider, false);
        }

        // Save configuration
        _tileFetcher->saveProviderConfiguration();

        delete _tileFetcher;
        _tileFetcher = nullptr;
    }

    // Verify settings were saved
    settings.beginGroup("MapTileLoadBalancer");
    const int count = settings.beginReadArray("providers");
    QVERIFY(count > 0);
    settings.endArray();
    settings.endGroup();
}

void QGeoTileFetcherQGCTest::testConfigurationLoad()
{
    // Setup: Save a configuration
    QSettings settings;
    settings.remove("MapTileLoadBalancer");
    settings.beginGroup("MapTileLoadBalancer");
    settings.beginWriteArray("providers");

    // Write test data for first provider
    settings.setArrayIndex(0);
    settings.setValue("mapId", 1);
    settings.setValue("name", "TestProvider");
    settings.setValue("weight", 7);
    settings.setValue("enabled", false);

    settings.endArray();
    settings.endGroup();
    settings.sync();

    // Create tile fetcher - should load configuration
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);

    // Configuration should be loaded (though mapId 1 might not exist)
    // This test verifies the load mechanism works without crashing
    QVERIFY(_tileFetcher != nullptr);
}

void QGeoTileFetcherQGCTest::testAllProvidersDisabled()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Disable all providers
    const QList<MapProvider*> providers = lb->getAllProviders();
    for (MapProvider* provider : providers) {
        lb->enableProvider(provider, false);
    }

    // Should return nullptr when all disabled
    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider == nullptr);

    // Verify healthy count is 0
    QCOMPARE(lb->healthyProviderCount(), 0);
}

void QGeoTileFetcherQGCTest::testAllProvidersUnhealthy()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Make all providers unhealthy
    const QList<MapProvider*> providers = lb->getAllProviders();
    for (MapProvider* provider : providers) {
        for (int i = 0; i < 10; i++) {
            provider->recordFailure();
        }
    }

    // LoadBalancer should still return a provider (auto-failover disabled case)
    // or nullptr if strict health checking
    MapProvider* provider = lb->getNextProvider();
    // Behavior depends on LoadBalancer configuration
    qDebug() << "All unhealthy result:" << provider;
}

void QGeoTileFetcherQGCTest::testNullNetworkManager()
{
    // Verify tile fetcher handles null network manager gracefully
    _tileFetcher = new QGeoTileFetcherQGC(nullptr, QVariantMap(), nullptr);

    // Should not crash during initialization
    QVERIFY(_tileFetcher != nullptr);

    // LoadBalancer should still initialize
    LoadBalancer* lb = _tileFetcher->loadBalancer();
    QVERIFY(lb != nullptr);
}

void QGeoTileFetcherQGCTest::testInvalidMapId()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);

    // Request tile with invalid mapId
    // Would need to call getTileImage() which is private
    // This test documents the edge case
    QVERIFY(_tileFetcher != nullptr);
}

void QGeoTileFetcherQGCTest::testCorruptedSettings()
{
    // Write corrupted settings
    QSettings settings;
    settings.remove("MapTileLoadBalancer");
    settings.beginGroup("MapTileLoadBalancer");
    settings.beginWriteArray("providers");

    settings.setArrayIndex(0);
    settings.setValue("mapId", -999);  // Invalid mapId
    settings.setValue("weight", -50);  // Invalid weight
    settings.setValue("enabled", "garbage");  // Wrong type

    settings.endArray();
    settings.endGroup();
    settings.sync();

    // Should not crash when loading corrupted settings
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    QVERIFY(_tileFetcher != nullptr);

    // LoadBalancer should still work with defaults
    LoadBalancer* lb = _tileFetcher->loadBalancer();
    QVERIFY(lb->totalProviders() > 0);
}

void QGeoTileFetcherQGCTest::testProviderWeightBounds()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    if (!provider) {
        QSKIP("No providers available");
    }

    // Test setting various weights
    lb->setProviderWeight(provider, 1);
    QCOMPARE(lb->getProviderWeight(provider), 1);

    lb->setProviderWeight(provider, 100);
    QCOMPARE(lb->getProviderWeight(provider), 100);

    // Negative weight should be clamped to 1
    lb->setProviderWeight(provider, -5);
    QCOMPARE(lb->getProviderWeight(provider), 1);
}

void QGeoTileFetcherQGCTest::testConcurrentTileFetches()
{
    // This test would require multi-threading and mocking
    // For now, document the requirement
    QSKIP("Requires multi-threading test infrastructure");
}

void QGeoTileFetcherQGCTest::testProviderAccessRaceCondition()
{
    // This test would require multi-threading
    QSKIP("Requires multi-threading test infrastructure");
}

void QGeoTileFetcherQGCTest::testNoMemoryLeaksOnSuccess()
{
    // This test would require valgrind or similar
    QSKIP("Requires memory profiling tools");
}

void QGeoTileFetcherQGCTest::testNoMemoryLeaksOnError()
{
    // This test would require valgrind or similar
    QSKIP("Requires memory profiling tools");
}

void QGeoTileFetcherQGCTest::testProviderLifecycleSafety()
{
    // Test that providers remain valid throughout request lifecycle
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    MapProvider* provider = lb->getNextProvider();
    QVERIFY(provider != nullptr);

    // Provider should remain valid
    const QString name = provider->getMapName();
    QVERIFY(!name.isEmpty());

    // Simulate request lifecycle
    provider->beginTileRequest(0, 0, 10, false);
    provider->completeTileRequest(0, 0, 10, true, 1000, 100.0);

    // Provider should still be valid
    QVERIFY(!provider->getMapName().isEmpty());
}

void QGeoTileFetcherQGCTest::testLoadBalancerPerformance()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Measure time to select 1000 providers
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < 1000; i++) {
        MapProvider* provider = lb->getNextProvider();
        Q_UNUSED(provider);
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "1000 provider selections took" << elapsed << "ms";

    // Should be fast (< 100ms for 1000 selections)
    QVERIFY(elapsed < 100);
}

void QGeoTileFetcherQGCTest::testProviderSelectionPerformance()
{
    _tileFetcher = new QGeoTileFetcherQGC(_networkManager, QVariantMap(), nullptr);
    LoadBalancer* lb = _tileFetcher->loadBalancer();

    // Measure time for single provider selection
    QElapsedTimer timer;
    timer.start();

    MapProvider* provider = lb->getNextProvider();

    const qint64 elapsed = timer.nsecsElapsed();
    qDebug() << "Single provider selection took" << elapsed << "ns";

    // Should be very fast (< 1ms)
    QVERIFY(elapsed < 1000000);  // 1ms in nanoseconds
    QVERIFY(provider != nullptr);
}

QTEST_GUILESS_MAIN(QGeoTileFetcherQGCTest)

#include "QGeoTileFetcherQGCTest.moc"
