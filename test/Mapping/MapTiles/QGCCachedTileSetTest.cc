#include "QGCCachedTileSetTest.h"

#include <algorithm>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "PropertyTestHelper.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

void QGCCachedTileSetTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

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
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::totalTileCountChanged, &QGCCachedTileSet::setTotalTileCount,
                         &QGCCachedTileSet::totalTileCount, 100u);
}

void QGCCachedTileSetTest::_testSetTotalTileSizeEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::totalTilesSizeChanged, &QGCCachedTileSet::setTotalTileSize,
                         &QGCCachedTileSet::totalTilesSize, quint64(1024 * 1024));
}

void QGCCachedTileSetTest::_testSetSavedCountAndSizeEmitSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::savedTileCountChanged, &QGCCachedTileSet::setSavedTileCount,
                         &QGCCachedTileSet::savedTileCount, 50u);
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::savedTileSizeChanged, &QGCCachedTileSet::setSavedTileSize,
                         &QGCCachedTileSet::savedTileSize, quint64(2048));
}

void QGCCachedTileSetTest::_testSetUniqueCountAndSizeEmitSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::uniqueTileCountChanged, &QGCCachedTileSet::setUniqueTileCount,
                         &QGCCachedTileSet::uniqueTileCount, 25u);
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::uniqueTileSizeChanged, &QGCCachedTileSet::setUniqueTileSize,
                         &QGCCachedTileSet::uniqueTileSize, quint64(4096));
}

void QGCCachedTileSetTest::_testSetNameEmitsSignal()
{
    QGCCachedTileSet ts(QStringLiteral("original"));
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::nameChanged, &QGCCachedTileSet::setName, &QGCCachedTileSet::name,
                         QStringLiteral("renamed"));
}

void QGCCachedTileSetTest::_testSetStateEmitsSignals()
{
    QGCCachedTileSet ts(QStringLiteral("test"));
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::deletingChanged, &QGCCachedTileSet::setDeleting,
                         &QGCCachedTileSet::deleting, true);
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::downloadingChanged, &QGCCachedTileSet::setDownloading,
                         &QGCCachedTileSet::downloading, true);
    QVERIFY_PROPERTY_SET(&ts, &QGCCachedTileSet::errorCountChanged, &QGCCachedTileSet::setErrorCount,
                         &QGCCachedTileSet::errorCount, 3u);
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

void QGCCachedTileSetTest::_testDownload_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<int>("maxZoom");
    QTest::addColumn<QByteArray>("serverBody");
    QTest::addColumn<bool>("stored");
    QTest::addColumn<bool>("failed");
    QTest::addColumn<bool>("lastTileFails");
    QTest::addColumn<QString>("expectedWarning");

    QString mapType;
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            mapType = type;
            break;
        }
    }

    QTest::newRow("tiles stored in the set")
        << mapType << 8.50 << 16 << tileReplyPngBody() << true << false << false << QString();
    // The two rows below download one tile, for the one warning each.
    // A bad answer must count as an error and must not stall the download.
    QTest::newRow("server answers without a tile")
        << mapType << 8.60 << 14 << QByteArray() << false << true << false << QStringLiteral("Error fetching tile");
    // The set is done, without an error, when its provider has no tile to give
    QTest::newRow("provider has no tile") << QStringLiteral("Bing Hybrid") << 8.70 << 14 << tileReplyBingPlaceholder()
                                          << false << false << false << QStringLiteral("_savedTileCount=0");
    // The set is not done while a tile is being fetched: that tile can fail. Few enough tiles to be fetched at once.
    QTest::newRow("last tile fails after the others are stored")
        << mapType << 8.80 << 15 << tileReplyPngBody() << true << false << true
        << QStringLiteral("Error fetching tile");
}

void QGCCachedTileSetTest::_testDownload()
{
    QFETCH(QString, providerType);
    QFETCH(double, longitude);
    QFETCH(int, maxZoom);
    QFETCH(QByteArray, serverBody);
    QFETCH(bool, stored);
    QFETCH(bool, failed);
    QFETCH(bool, lastTileFails);
    QFETCH(QString, expectedWarning);

    QVERIFY(!providerType.isEmpty());

    // The region lies within one tile at zoom 14
    constexpr int kMinZoom = 14;
    constexpr double kTopLat = 47.3905;
    constexpr double kBottomLat = 47.3895;
    const double leftLon = longitude - 0.0005;
    const double rightLon = longitude + 0.0005;
    quint32 tileCount = 0;
    for (int zoom = kMinZoom; zoom <= maxZoom; zoom++) {
        tileCount += UrlFactory::getTileCount(zoom, leftLon, kTopLat, rightLon, kBottomLat, providerType).tileCount;
    }
    QCOMPARE_GE(tileCount, static_cast<quint32>(maxZoom - kMinZoom + 1));
    const quint32 failedCount = lastTileFails ? 1U : (failed ? tileCount : 0U);
    const quint32 storedCount = stored ? (tileCount - failedCount) : 0U;

    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();

    const QString name = QStringLiteral("download test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    QGCCachedTileSet* const newSet = new QGCCachedTileSet(name);
    newSet->setMapTypeStr(providerType);
    newSet->setType(providerType);
    newSet->setTopleftLat(kTopLat);
    newSet->setTopleftLon(leftLon);
    newSet->setBottomRightLat(kBottomLat);
    newSet->setBottomRightLon(rightLon);
    newSet->setMinZoom(kMinZoom);
    newSet->setMaxZoom(maxZoom);
    newSet->setTotalTileCount(tileCount);

    QGCCreateTileSetTask* const createTask = new QGCCreateTileSetTask(newSet);
    QSignalSpy savedSpy(createTask, &QGCCreateTileSetTask::tileSetSaved);
    QVERIFY(engine->addTask(createTask));
    QVERIFY_SIGNAL_COUNT_WAIT(savedSpy, 1, TestTimeout::mediumMs());
    const std::unique_ptr<QGCCachedTileSet> set(newSet);  // a saved set no longer belongs to its task

    TileReplyMockNam nam;
    nam.body = serverBody;
    nam.holdReplies = lastTileFails;
    set->setNetworkManager(&nam);
    // What a view bound to downloadStatus shows: the text it read when the property last notified
    QString shownStatus;
    (void) connect(set.get(), &QGCCachedTileSet::downloadStatusChanged, this,
                   [&shownStatus, &set] { shownStatus = set->downloadStatus(); });
    if (!expectedWarning.isEmpty()) {
        expectLogMessage("MapTiles.QGCCachedTileSet", QtWarningMsg, QRegularExpression(expectedWarning));
    }
    set->createDownloadTask();
    QVERIFY(set->downloading());

    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, static_cast<int>(tileCount), TestTimeout::mediumMs());
    if (lastTileFails) {
        for (qsizetype i = 0; i < (nam.replies.size() - 1); i++) {
            nam.replies.at(i)->finishNow();
        }
        QTRY_COMPARE_WITH_TIMEOUT(set->savedTileCount(), storedCount, TestTimeout::mediumMs());
        QVERIFY(set->downloading());
        nam.replies.constLast()->failNow(QNetworkReply::TimeoutError);
    }
    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(set->savedTileCount(), storedCount, TestTimeout::mediumMs());
    QCOMPARE(set->errorCount(), failedCount);
    QCOMPARE(set->complete(), failedCount == 0U);
    QCOMPARE(shownStatus, set->downloadStatus());
    if (!expectedWarning.isEmpty()) {
        verifyExpectedLogMessage();
    }

    // The set as the database has it, which is what a restart loads: its tiles are stored in it, not only in the
    // default set, and it is complete unless a tile is left to download. The database worker runs tasks in order,
    // so the sets are all reported once the task queued after them is.
    QList<QGCCachedTileSet*> databaseSets;
    const auto deleteSets = qScopeGuard([&databaseSets] { qDeleteAll(databaseSets); });
    QGCFetchTileSetTask* const fetchSetsTask = new QGCFetchTileSetTask();
    connect(fetchSetsTask, &QGCFetchTileSetTask::tileSetFetched, this,
            [&databaseSets](QGCCachedTileSet* databaseSet) { databaseSets.append(databaseSet); });
    QVERIFY(engine->addTask(fetchSetsTask));
    bool setsReported = false;
    QGCFetchTileTask* const sentinelTask = new QGCFetchTileTask(QStringLiteral("no such tile"));
    connect(sentinelTask, &QGCMapTask::error, this, [&setsReported] { setsReported = true; });
    QVERIFY(engine->addTask(sentinelTask));
    QTRY_VERIFY_WITH_TIMEOUT(setsReported, TestTimeout::mediumMs());

    const auto databaseSet =
        std::find_if(databaseSets.cbegin(), databaseSets.cend(),
                     [&name](const QGCCachedTileSet* candidate) { return candidate->name() == name; });
    QVERIFY(databaseSet != databaseSets.cend());
    QCOMPARE((*databaseSet)->savedTileCount(), storedCount);
    QCOMPARE((*databaseSet)->complete(), failedCount == 0U);
}

UT_REGISTER_TEST(QGCCachedTileSetTest, TestLabel::Unit)
