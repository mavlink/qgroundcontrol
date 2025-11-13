/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCachedTileSetTest.h"

#include "QGCCachedTileSet.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void QGCCachedTileSetTest::_testDownloadStatsUpdates()
{
    QGCCachedTileSet tileSet(QStringLiteral("Test Set"));
    QSignalSpy spy(&tileSet, &QGCCachedTileSet::downloadStatsChanged);

    QCOMPARE(tileSet.pendingTiles(), 0u);
    QCOMPARE(tileSet.downloadingTiles(), 0u);
    QCOMPARE(tileSet.errorTiles(), 0u);

    tileSet.setDownloadStats(5, 2, 1);
    QCOMPARE(tileSet.pendingTiles(), 5u);
    QCOMPARE(tileSet.downloadingTiles(), 2u);
    QCOMPARE(tileSet.errorTiles(), 1u);
    QCOMPARE(spy.count(), 1);

    // Setting identical values should not emit again
    tileSet.setDownloadStats(5, 2, 1);
    QCOMPARE(spy.count(), 1);

    tileSet.setDownloadStats(0, 0, 0);
    QCOMPARE(tileSet.pendingTiles(), 0u);
    QCOMPARE(tileSet.downloadingTiles(), 0u);
    QCOMPARE(tileSet.errorTiles(), 0u);
    QCOMPARE(spy.count(), 2);
}
