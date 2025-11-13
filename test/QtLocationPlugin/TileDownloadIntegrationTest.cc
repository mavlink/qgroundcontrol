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
    QCOMPARE(tileSet.complete(), false);

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
