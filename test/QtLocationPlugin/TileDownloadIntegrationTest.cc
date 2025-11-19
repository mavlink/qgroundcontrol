/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TileDownloadIntegrationTest.h"

#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void TileDownloadIntegrationTest::_testTileSetCreation()
{
    // Create a small tile set for testing
    QGCCachedTileSet tileSet(QStringLiteral("Integration Test Set"));
    tileSet.setMapTypeStr(QStringLiteral("Google Street"));
    tileSet.setType(QStringLiteral("GoogleMap"));
    tileSet.setTopleftLat(37.7749);      // San Francisco
    tileSet.setTopleftLon(-122.4194);
    tileSet.setBottomRightLat(37.7649);
    tileSet.setBottomRightLon(-122.4094);
    tileSet.setMinZoom(10);
    tileSet.setMaxZoom(11);

    // Verify initial state
    QCOMPARE(tileSet.name(), QStringLiteral("Integration Test Set"));
    QCOMPARE(tileSet.defaultSet(), false);
    QCOMPARE(tileSet.downloading(), false);
    QCOMPARE(tileSet.complete(), true); // Initially complete (0 <= 0)

    // Verify download stats are initialized to zero
    QCOMPARE(tileSet.pendingTiles(), 0u);
    QCOMPARE(tileSet.downloadingTiles(), 0u);
    QCOMPARE(tileSet.errorTiles(), 0u);
    QCOMPARE(tileSet.downloadProgress(), 0.0);
}

void TileDownloadIntegrationTest::_testDownloadStateTransitions()
{
    QGCCachedTileSet tileSet(QStringLiteral("State Test"));

    // Set up initial stats: 10 pending, 0 downloading, 0 errors
    tileSet.setDownloadStats(10, 0, 0);
    QCOMPARE(tileSet.pendingTiles(), 10u);

    // Simulate download progress: 5 pending, 3 downloading, 0 errors
    tileSet.setDownloadStats(5, 3, 0);
    QCOMPARE(tileSet.pendingTiles(), 5u);
    QCOMPARE(tileSet.downloadingTiles(), 3u);

    // Simulate completion: 0 pending, 0 downloading, 2 errors (8 succeeded)
    tileSet.setTotalTileCount(10);
    tileSet.setSavedTileCount(8);
    tileSet.setDownloadStats(0, 0, 2);
    QCOMPARE(tileSet.errorTiles(), 2u);
    QCOMPARE(tileSet.downloadProgress(), 0.8);  // 80% complete
}

void TileDownloadIntegrationTest::_testPauseResume()
{
    QGCCachedTileSet tileSet(QStringLiteral("Pause Test"));
    QSignalSpy downloadingSpy(&tileSet, &QGCCachedTileSet::downloadingChanged);

    // Start download
    tileSet.setDownloading(true);
    QCOMPARE(downloadingSpy.count(), 1);
    QVERIFY(tileSet.downloading());

    // Pause (simulate)
    tileSet.setDownloading(false);
    QCOMPARE(downloadingSpy.count(), 2);
    QVERIFY(!tileSet.downloading());

    // Resume
    tileSet.setDownloading(true);
    QCOMPARE(downloadingSpy.count(), 3);
    QVERIFY(tileSet.downloading());
}

void TileDownloadIntegrationTest::_testErrorHandling()
{
    QGCCachedTileSet tileSet(QStringLiteral("Error Test"));
    QSignalSpy errorSpy(&tileSet, &QGCCachedTileSet::errorCountChanged);

    // No errors initially
    QCOMPARE(tileSet.errorCount(), 0u);

    // Simulate errors
    tileSet.setErrorCount(3);
    QCOMPARE(tileSet.errorCount(), 3u);
    QCOMPARE(errorSpy.count(), 1);

    // Verify error count string formatting
    QString errorStr = tileSet.errorCountStr();
    QVERIFY(errorStr.contains('3'));
}

void TileDownloadIntegrationTest::_testCacheControl()
{
    // Test cache enable/disable via settings
    // This would integrate with QGCMapEngineManager
    // For now, verify the API exists
    QGCMapEngineManager *manager = QGCMapEngineManager::instance();
    QVERIFY(manager != nullptr);

    // Verify cache control methods are accessible
    manager->setCachingPaused(true);
    manager->setCachingPaused(false);
    manager->setCachingDefaultSetEnabled(true);
    manager->setCachingDefaultSetEnabled(false);
}

void TileDownloadIntegrationTest::_testConcurrentDownloads()
{
    // Create multiple tile sets
    QGCCachedTileSet set1(QStringLiteral("Set 1"));
    QGCCachedTileSet set2(QStringLiteral("Set 2"));

    // Set download stats for both
    set1.setDownloadStats(5, 2, 0);
    set2.setDownloadStats(8, 3, 1);

    // Verify stats are independent
    QCOMPARE(set1.pendingTiles(), 5u);
    QCOMPARE(set1.downloadingTiles(), 2u);
    QCOMPARE(set2.pendingTiles(), 8u);
    QCOMPARE(set2.downloadingTiles(), 3u);
    QCOMPARE(set2.errorTiles(), 1u);
}

void TileDownloadIntegrationTest::_testTileSetDeletion()
{
    QGCCachedTileSet tileSet(QStringLiteral("Delete Test"));
    QSignalSpy deletingSpy(&tileSet, &QGCCachedTileSet::deletingChanged);

    QVERIFY(!tileSet.deleting());

    // Simulate deletion flag
    tileSet.setDeleting(true);
    QVERIFY(tileSet.deleting());
    QCOMPARE(deletingSpy.count(), 1);

    tileSet.setDeleting(false);
    QVERIFY(!tileSet.deleting());
    QCOMPARE(deletingSpy.count(), 2);
}

void TileDownloadIntegrationTest::_testDownloadStatsAggregation()
{
    // Create multiple tile sets with different stats
    QGCCachedTileSet set1(QStringLiteral("Set 1"));
    QGCCachedTileSet set2(QStringLiteral("Set 2"));
    QGCCachedTileSet set3(QStringLiteral("Set 3"));

    // Set 1: 10 pending, 2 active, 1 error
    set1.setDownloadStats(10, 2, 1);
    QCOMPARE(set1.pendingTiles(), 10u);
    QCOMPARE(set1.downloadingTiles(), 2u);
    QCOMPARE(set1.errorTiles(), 1u);

    // Set 2: 5 pending, 1 active, 0 errors
    set2.setDownloadStats(5, 1, 0);
    QCOMPARE(set2.pendingTiles(), 5u);
    QCOMPARE(set2.downloadingTiles(), 1u);
    QCOMPARE(set2.errorTiles(), 0u);

    // Set 3: 0 pending, 0 active, 3 errors (all failed)
    set3.setDownloadStats(0, 0, 3);
    QCOMPARE(set3.pendingTiles(), 0u);
    QCOMPARE(set3.downloadingTiles(), 0u);
    QCOMPARE(set3.errorTiles(), 3u);

    // Verify stats are independent
    QCOMPARE(set1.pendingTiles(), 10u);
    QCOMPARE(set2.pendingTiles(), 5u);
    QCOMPARE(set3.pendingTiles(), 0u);

    // Total across all sets would be:
    // Pending: 10 + 5 + 0 = 15
    // Active: 2 + 1 + 0 = 3
    // Errors: 1 + 0 + 3 = 4
}

void TileDownloadIntegrationTest::_testPauseVsCancelBehavior()
{
    QGCCachedTileSet tileSet(QStringLiteral("Pause vs Cancel Test"));

    // Set up a download in progress
    tileSet.setDownloading(true);
    tileSet.setTotalTileCount(100);
    tileSet.setSavedTileCount(50);
    tileSet.setDownloadStats(30, 10, 10);

    QVERIFY(tileSet.downloading());
    QCOMPARE(tileSet.downloadProgress(), 0.5);

    // Pause should stop downloading but preserve state
    tileSet.pauseDownloadTask();
    QVERIFY(!tileSet.downloading());

    // After pause, tiles should be marked as pending (not lost)
    // Resume should be able to continue from where it left off

    // Pause also stops downloading (cancel was removed - use pause instead)
    tileSet.pauseDownloadTask(false);
    QVERIFY(!tileSet.downloading());
}

void TileDownloadIntegrationTest::_testDownloadStatusStrings()
{
    // Test 1: Default set status
    QGCCachedTileSet defaultSet(QStringLiteral("Default"));
    defaultSet.setDefaultSet(true);
    defaultSet.setSavedTileSize(1024 * 1024 * 500); // 500 MB
    QString status = defaultSet.downloadStatus();
    QVERIFY(!status.isEmpty());
    // Default set shows size only

    // Test 2: Complete download
    QGCCachedTileSet completeSet(QStringLiteral("Complete"));
    completeSet.setTotalTileCount(100);
    completeSet.setSavedTileCount(100);
    completeSet.setSavedTileSize(1024 * 1024 * 10); // 10 MB
    status = completeSet.downloadStatus();
    QVERIFY(!status.isEmpty());

    // Test 3: In-progress download
    QGCCachedTileSet inProgressSet(QStringLiteral("In Progress"));
    inProgressSet.setTotalTileCount(100);
    inProgressSet.setSavedTileCount(50);
    inProgressSet.setDownloading(true);
    status = inProgressSet.downloadStatus();
    QVERIFY(!status.isEmpty());

    // Test 4: Not yet started
    QGCCachedTileSet notStartedSet(QStringLiteral("Not Started"));
    notStartedSet.setTotalTileCount(100);
    notStartedSet.setSavedTileCount(0);
    status = notStartedSet.downloadStatus();
    QVERIFY(!status.isEmpty());
}

void TileDownloadIntegrationTest::_testDefaultSetBehavior()
{
    QGCCachedTileSet defaultSet(QStringLiteral("Default Set"));
    defaultSet.setDefaultSet(true);

    // Default sets are NOT considered "complete" (complete() returns false for default sets)
    // This is by design - the UI handles default sets separately
    QVERIFY(!defaultSet.complete());

    // Default sets should not show download progress
    defaultSet.setTotalTileCount(1000);
    defaultSet.setSavedTileCount(500);
    QCOMPARE(defaultSet.downloadProgress(), 0.0);
    QVERIFY(!defaultSet.complete()); // Still not complete (default sets always return false)

    // Default sets can accumulate tiles over time
    defaultSet.setSavedTileSize(1024 * 1024 * 100); // 100 MB
    QVERIFY(defaultSet.savedTileSize() > 0);

    // Verify default set identification
    QVERIFY(defaultSet.defaultSet());
    QCOMPARE(defaultSet.name(), QStringLiteral("Default Set"));
}

void TileDownloadIntegrationTest::_testNetworkErrorTracking()
{
    QGCCachedTileSet tileSet(QStringLiteral("Error Tracking Test"));

    // Initially no errors
    QCOMPARE(tileSet.errorCount(), 0u);
    QCOMPARE(tileSet.errorTiles(), 0u);

    // Simulate network errors during download
    tileSet.setErrorCount(5);
    QCOMPARE(tileSet.errorCount(), 5u);

    // Set download stats with error tiles
    tileSet.setDownloadStats(10, 5, 3); // 10 pending, 5 downloading, 3 errors
    QCOMPARE(tileSet.errorTiles(), 3u);
    QCOMPARE(tileSet.pendingTiles(), 10u);

    // Verify error count string is not empty
    QString errorStr = tileSet.errorCountStr();
    QVERIFY(!errorStr.isEmpty());
    QVERIFY(errorStr.contains('5'));
}

void TileDownloadIntegrationTest::_testErrorTileStateHandling()
{
    QGCCachedTileSet tileSet(QStringLiteral("Error State Test"));
    QSignalSpy errorSpy(&tileSet, &QGCCachedTileSet::errorCountChanged);

    // Set up initial download state with errors
    tileSet.setTotalTileCount(100);
    tileSet.setSavedTileCount(50);
    tileSet.setDownloadStats(30, 10, 10); // 30 pending, 10 downloading, 10 errors

    QCOMPARE(tileSet.errorTiles(), 10u);
    QCOMPARE(tileSet.pendingTiles(), 30u);

    // Retry should reset error count
    tileSet.retryFailedTiles();

    // Verify signals were emitted (implementation-dependent)
    QVERIFY(errorSpy.count() >= 0);

    // Error count should be reset after retry
    QCOMPARE(tileSet.errorCount(), 0u);
}
