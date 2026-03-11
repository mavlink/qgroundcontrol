#include "QGCCachedTileSetTest.h"

#include <QtTest/QSignalSpy>

#include "QGCCachedTileSet.h"

void QGCCachedTileSetTest::_testConstructorSetsName()
{
    QGCCachedTileSet ts(QStringLiteral("MySet"));
    QCOMPARE(ts.name(), QStringLiteral("MySet"));
}

void QGCCachedTileSetTest::_testDefaultPropertyValues()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    QCOMPARE(ts.totalTileCount(), 0u);
    QCOMPARE(ts.totalTilesSize(), quint64(0));
    QCOMPARE(ts.uniqueTileCount(), 0u);
    QCOMPARE(ts.uniqueTileSize(), quint64(0));
    QCOMPARE(ts.savedTileCount(), 0u);
    QCOMPARE(ts.savedTileSize(), quint64(0));
    QCOMPARE(ts.errorCount(), 0u);

    QCOMPARE(ts.topleftLat(), 0.);
    QCOMPARE(ts.topleftLon(), 0.);
    QCOMPARE(ts.bottomRightLat(), 0.);
    QCOMPARE(ts.bottomRightLon(), 0.);

    QCOMPARE(ts.minZoom(), 3);
    QCOMPARE(ts.maxZoom(), 3);
    QCOMPARE(ts.id(), quint64(0));

    QCOMPARE(ts.type(), QStringLiteral("Invalid"));
    QVERIFY(ts.mapTypeStr().isEmpty());

    QVERIFY(!ts.defaultSet());
    QVERIFY(!ts.deleting());
    QVERIFY(!ts.downloading());
    QVERIFY(!ts.selected());
    QVERIFY(!ts.creationDate().isValid());

    QVERIFY(ts.complete());
}

void QGCCachedTileSetTest::_testSetMapTypeStr()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    ts.setMapTypeStr(QStringLiteral("Google Satellite"));
    QCOMPARE(ts.mapTypeStr(), QStringLiteral("Google Satellite"));
}

void QGCCachedTileSetTest::_testSetCoordinates()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    ts.setTopleftLat(47.5);
    ts.setTopleftLon(-122.3);
    ts.setBottomRightLat(47.4);
    ts.setBottomRightLon(-122.2);

    QCOMPARE(ts.topleftLat(), 47.5);
    QCOMPARE(ts.topleftLon(), -122.3);
    QCOMPARE(ts.bottomRightLat(), 47.4);
    QCOMPARE(ts.bottomRightLon(), -122.2);
}

void QGCCachedTileSetTest::_testSetZoomAndMetadata()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    ts.setMinZoom(5);
    ts.setMaxZoom(15);
    QCOMPARE(ts.minZoom(), 5);
    QCOMPARE(ts.maxZoom(), 15);

    ts.setId(42);
    QCOMPARE(ts.id(), quint64(42));

    ts.setType(QStringLiteral("Satellite"));
    QCOMPARE(ts.type(), QStringLiteral("Satellite"));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    ts.setCreationDate(now);
    QCOMPARE(ts.creationDate(), now);

    ts.setDefaultSet(true);
    QVERIFY(ts.defaultSet());
}

void QGCCachedTileSetTest::_testSetTotalTileCountEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QSignalSpy spy(&ts, &QGCCachedTileSet::totalTileCountChanged);

    ts.setTotalTileCount(100);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(ts.totalTileCount(), 100u);
}

void QGCCachedTileSetTest::_testSetTotalTileSizeEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QSignalSpy spy(&ts, &QGCCachedTileSet::totalTilesSizeChanged);

    ts.setTotalTileSize(1024 * 1024);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(ts.totalTilesSize(), quint64(1024 * 1024));
}

void QGCCachedTileSetTest::_testSetSavedCountAndSizeEmitSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QSignalSpy countSpy(&ts, &QGCCachedTileSet::savedTileCountChanged);
    QSignalSpy sizeSpy(&ts, &QGCCachedTileSet::savedTileSizeChanged);

    ts.setSavedTileCount(50);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(ts.savedTileCount(), 50u);

    ts.setSavedTileSize(2048);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(ts.savedTileSize(), quint64(2048));
}

void QGCCachedTileSetTest::_testSetUniqueCountAndSizeEmitSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QSignalSpy countSpy(&ts, &QGCCachedTileSet::uniqueTileCountChanged);
    QSignalSpy sizeSpy(&ts, &QGCCachedTileSet::uniqueTileSizeChanged);

    ts.setUniqueTileCount(25);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(ts.uniqueTileCount(), 25u);

    ts.setUniqueTileSize(4096);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(ts.uniqueTileSize(), quint64(4096));
}

void QGCCachedTileSetTest::_testSetNameEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("original"));
    QSignalSpy spy(&ts, &QGCCachedTileSet::nameChanged);

    ts.setName(QStringLiteral("renamed"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(ts.name(), QStringLiteral("renamed"));
}

void QGCCachedTileSetTest::_testSetStateEmitsSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    QSignalSpy deletingSpy(&ts, &QGCCachedTileSet::deletingChanged);
    ts.setDeleting(true);
    QCOMPARE(deletingSpy.count(), 1);
    QVERIFY(ts.deleting());

    QSignalSpy downloadingSpy(&ts, &QGCCachedTileSet::downloadingChanged);
    ts.setDownloading(true);
    QCOMPARE(downloadingSpy.count(), 1);
    QVERIFY(ts.downloading());

    QSignalSpy errorSpy(&ts, &QGCCachedTileSet::errorCountChanged);
    ts.setErrorCount(3);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(ts.errorCount(), 3u);
}

void QGCCachedTileSetTest::_testSetterNoSignalOnSameValue()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    ts.setTotalTileCount(10);
    ts.setSavedTileCount(5);
    ts.setTotalTileSize(1000);
    ts.setSavedTileSize(500);
    ts.setUniqueTileCount(8);
    ts.setUniqueTileSize(800);
    ts.setDeleting(true);
    ts.setDownloading(true);
    ts.setErrorCount(2);
    ts.setName(QStringLiteral("other"));
    ts.setSelected(true);

    QSignalSpy totalCountSpy(&ts, &QGCCachedTileSet::totalTileCountChanged);
    QSignalSpy savedCountSpy(&ts, &QGCCachedTileSet::savedTileCountChanged);
    QSignalSpy totalSizeSpy(&ts, &QGCCachedTileSet::totalTilesSizeChanged);
    QSignalSpy savedSizeSpy(&ts, &QGCCachedTileSet::savedTileSizeChanged);
    QSignalSpy uniqueCountSpy(&ts, &QGCCachedTileSet::uniqueTileCountChanged);
    QSignalSpy uniqueSizeSpy(&ts, &QGCCachedTileSet::uniqueTileSizeChanged);
    QSignalSpy deletingSpy(&ts, &QGCCachedTileSet::deletingChanged);
    QSignalSpy downloadingSpy(&ts, &QGCCachedTileSet::downloadingChanged);
    QSignalSpy errorSpy(&ts, &QGCCachedTileSet::errorCountChanged);
    QSignalSpy nameSpy(&ts, &QGCCachedTileSet::nameChanged);
    QSignalSpy selectedSpy(&ts, &QGCCachedTileSet::selectedChanged);

    ts.setTotalTileCount(10);
    ts.setSavedTileCount(5);
    ts.setTotalTileSize(1000);
    ts.setSavedTileSize(500);
    ts.setUniqueTileCount(8);
    ts.setUniqueTileSize(800);
    ts.setDeleting(true);
    ts.setDownloading(true);
    ts.setErrorCount(2);
    ts.setName(QStringLiteral("other"));
    ts.setSelected(true);

    QCOMPARE(totalCountSpy.count(), 0);
    QCOMPARE(savedCountSpy.count(), 0);
    QCOMPARE(totalSizeSpy.count(), 0);
    QCOMPARE(savedSizeSpy.count(), 0);
    QCOMPARE(uniqueCountSpy.count(), 0);
    QCOMPARE(uniqueSizeSpy.count(), 0);
    QCOMPARE(deletingSpy.count(), 0);
    QCOMPARE(downloadingSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(nameSpy.count(), 0);
    QCOMPARE(selectedSpy.count(), 0);
}

void QGCCachedTileSetTest::_testCompleteWhenDefaultSet()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    ts.setTotalTileCount(100);
    ts.setSavedTileCount(0);
    QVERIFY(!ts.complete());

    ts.setDefaultSet(true);
    QVERIFY(ts.complete());
}

void QGCCachedTileSetTest::_testCompleteWhenAllSaved()
{
    QGCCachedTileSet ts(QStringLiteral("test"));

    ts.setTotalTileCount(100);
    ts.setSavedTileCount(99);
    QVERIFY(!ts.complete());

    ts.setSavedTileCount(100);
    QVERIFY(ts.complete());

    ts.setSavedTileCount(101);
    QVERIFY(ts.complete());
}

void QGCCachedTileSetTest::_testSetSelectedEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QSignalSpy spy(&ts, &QGCCachedTileSet::selectedChanged);

    ts.setSelected(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(ts.selected());

    ts.setSelected(true);
    QCOMPARE(spy.count(), 1);

    ts.setSelected(false);
    QCOMPARE(spy.count(), 2);
    QVERIFY(!ts.selected());
}

UT_REGISTER_TEST(QGCCachedTileSetTest, TestLabel::Unit)
