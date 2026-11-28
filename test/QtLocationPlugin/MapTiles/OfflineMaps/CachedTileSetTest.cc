#include "CachedTileSetTest.h"

#include <algorithm>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "CachedTileSet.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "PropertyTestHelper.h"
#include "TileCache.h"
#include "TileCacheTestHelpers.h"
#include "TileReplyNetworkMock.h"
#include "TileSetTasks.h"
#include "TileSetTestHelpers.h"
#include "UnitTestTileGenerator.h"

namespace {

/// A tile set that covers nothing
TileSetRecord tileSetTestRecord(const QString& name, bool defaultSet = false)
{
    TileSetRecord record;
    record.name = name;
    record.defaultSet = defaultSet;
    return record;
}

/// Runs \a sql on the database of the tile cache of the process
bool tileSetTestCacheExec(const QObject* context, const QString& sql)
{
    return tileCacheWorkerRows(context, tileCacheProcessAddTask, sql).has_value();
}

}  // namespace

void CachedTileSetTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void CachedTileSetTest::_testRecordTellsWhatTheSetCovers()
{
    const TileSetRecord record{.setID = 42,
                               .name = QStringLiteral("MySet"),
                               .mapName = QStringLiteral("Google Satellite"),
                               .topleftLat = 47.5,
                               .topleftLon = -122.3,
                               .bottomRightLat = 47.4,
                               .bottomRightLon = -122.2,
                               .minZoom = 5,
                               .maxZoom = 15,
                               .mapId = 2,
                               .numTiles = 100};
    CachedTileSet ts(record);

    QCOMPARE(ts.id(), quint64(42));
    QCOMPARE(ts.name(), QStringLiteral("MySet"));
    QCOMPARE(ts.mapTypeStr(), QStringLiteral("Google Satellite"));
    QCOMPARE(ts.topleftLat(), 47.5);
    QCOMPARE(ts.topleftLon(), -122.3);
    QCOMPARE(ts.bottomRightLat(), 47.4);
    QCOMPARE(ts.bottomRightLon(), -122.2);
    QCOMPARE(ts.minZoom(), 5);
    QCOMPARE(ts.maxZoom(), 15);
    QVERIFY(!ts.defaultSet());
    // The tiles of the record are the total of the set, of which none is saved
    QCOMPARE(ts.totalTileCount(), 100u);
    QCOMPARE(ts.savedTileCount(), 0u);
    QVERIFY(!ts.complete());

    // The record follows the id, the name and the total that the set gets later
    ts.setId(43);
    ts.setName(QStringLiteral("Renamed"));
    ts.setTotals(TileSetTotals{.totalTileCount = 90});
    const TileSetRecord changed = ts.record();
    QCOMPARE(changed.setID, quint64(43));
    QCOMPARE(changed.name, QStringLiteral("Renamed"));
    QCOMPARE(changed.numTiles, 90u);
    QCOMPARE(changed.mapId, 2);
}

void CachedTileSetTest::_testSetTotalsEmitsSignalsOfChangedValues()
{
    CachedTileSet ts(tileSetTestRecord(QStringLiteral("test")));
    QSignalSpy totalCountSpy(&ts, &CachedTileSet::totalTileCountChanged);
    QSignalSpy totalSizeSpy(&ts, &CachedTileSet::totalTilesSizeChanged);
    QSignalSpy savedCountSpy(&ts, &CachedTileSet::savedTileCountChanged);
    QSignalSpy savedSizeSpy(&ts, &CachedTileSet::savedTileSizeChanged);
    QSignalSpy uniqueCountSpy(&ts, &CachedTileSet::uniqueTileCountChanged);
    QSignalSpy uniqueSizeSpy(&ts, &CachedTileSet::uniqueTileSizeChanged);
    const auto signalCounts = [&] {
        return QList<qsizetype>{totalCountSpy.count(), totalSizeSpy.count(),   savedCountSpy.count(),
                                savedSizeSpy.count(),  uniqueCountSpy.count(), uniqueSizeSpy.count()};
    };

    TileSetTotals totals{.totalTileCount = 10,
                         .savedTileCount = 5,
                         .savedTileSize = 500,
                         .totalTileSize = 1000,
                         .uniqueTileCount = 8,
                         .uniqueTileSize = 800};
    ts.setTotals(totals);
    QCOMPARE(signalCounts(), (QList<qsizetype>{1, 1, 1, 1, 1, 1}));
    QCOMPARE(ts.totalTileCount(), 10u);
    QCOMPARE(ts.totalTilesSize(), quint64(1000));
    QCOMPARE(ts.savedTileCount(), 5u);
    QCOMPARE(ts.savedTileSize(), quint64(500));
    QCOMPARE(ts.uniqueTileCount(), 8u);
    QCOMPARE(ts.uniqueTileSize(), quint64(800));

    // No signal for a value that stays
    ts.setTotals(totals);
    QCOMPARE(signalCounts(), (QList<qsizetype>{1, 1, 1, 1, 1, 1}));
    totals.savedTileSize = 600;
    ts.setTotals(totals);
    QCOMPARE(signalCounts(), (QList<qsizetype>{1, 1, 1, 2, 1, 1}));
}

void CachedTileSetTest::_testSettersEmitSignalsOfChangedValues()
{
    CachedTileSet ts(tileSetTestRecord(QStringLiteral("original")));
    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::nameChanged, &CachedTileSet::setName, &CachedTileSet::name,
                         QStringLiteral("renamed"));
    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::deletingChanged, &CachedTileSet::setDeleting, &CachedTileSet::deleting,
                         true);
    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::downloadingChanged, &CachedTileSet::setDownloading,
                         &CachedTileSet::downloading, true);
    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::errorCountChanged, &CachedTileSet::setErrorCount,
                         &CachedTileSet::errorCount, 3u);
    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::selectedChanged, &CachedTileSet::setSelected, &CachedTileSet::selected,
                         true);

    // No signal for a value that stays
    QVERIFY_PROPERTY_SET_N(&ts, &CachedTileSet::nameChanged, &CachedTileSet::setName, &CachedTileSet::name,
                           QStringLiteral("renamed"), 0);
    QVERIFY_PROPERTY_SET_N(&ts, &CachedTileSet::deletingChanged, &CachedTileSet::setDeleting, &CachedTileSet::deleting,
                           true, 0);
    QVERIFY_PROPERTY_SET_N(&ts, &CachedTileSet::downloadingChanged, &CachedTileSet::setDownloading,
                           &CachedTileSet::downloading, true, 0);
    QVERIFY_PROPERTY_SET_N(&ts, &CachedTileSet::errorCountChanged, &CachedTileSet::setErrorCount,
                           &CachedTileSet::errorCount, 3u, 0);
    QVERIFY_PROPERTY_SET_N(&ts, &CachedTileSet::selectedChanged, &CachedTileSet::setSelected, &CachedTileSet::selected,
                           true, 0);

    QVERIFY_PROPERTY_SET(&ts, &CachedTileSet::selectedChanged, &CachedTileSet::setSelected, &CachedTileSet::selected,
                         false);
}

void CachedTileSetTest::_testCompleteWhenDefaultSet()
{
    const TileSetTotals totals{.totalTileCount = 100};
    CachedTileSet ts(tileSetTestRecord(QStringLiteral("test")));
    ts.setTotals(totals);
    QVERIFY(!ts.complete());

    CachedTileSet defaultSet(tileSetTestRecord(QStringLiteral("default"), true));
    defaultSet.setTotals(totals);
    QVERIFY(defaultSet.complete());
}

void CachedTileSetTest::_testCompleteWhenAllSaved()
{
    CachedTileSet ts(tileSetTestRecord(QStringLiteral("test")));

    ts.setTotals(TileSetTotals{.totalTileCount = 100, .savedTileCount = 99});
    QVERIFY(!ts.complete());
    QSignalSpy completeSpy(&ts, &CachedTileSet::completeChanged);

    ts.setTotals(TileSetTotals{.totalTileCount = 100, .savedTileCount = 100});
    QVERIFY(ts.complete());
    QCOMPARE(completeSpy.count(), 1);

    ts.setTotals(TileSetTotals{.totalTileCount = 100, .savedTileCount = 101});
    QVERIFY(ts.complete());
    QCOMPARE(completeSpy.count(), 1);
}

void CachedTileSetTest::_testDownload_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<int>("maxZoom");
    QTest::addColumn<QByteArray>("serverBody");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<bool>("stored");
    QTest::addColumn<bool>("failed");
    QTest::addColumn<bool>("lastTileFails");
    QTest::addColumn<QString>("expectedWarning");

    const QString mapType = UnitTestTileGenerator::mapProviderType();

    QTest::newRow("tiles stored in the set")
        << mapType << 8.50 << 16 << tileReplyPngBody() << 200 << true << false << false << QString();
    // The three rows below download one tile, for the one warning each.
    // A bad answer must count as an error and must not stall the download.
    QTest::newRow("server answers without a tile") << mapType << 8.60 << 14 << QByteArray() << 200 << false << true
                                                   << false << QStringLiteral("Error fetching tile");
    // The set is done, without an error, when its provider has no tile to give
    QTest::newRow("provider has no tile") << QStringLiteral("Bing Hybrid") << 8.70 << 14 << tileReplyBingPlaceholder()
                                          << 200 << false << false << false << QStringLiteral("_savedTileCount=0");
    QTest::newRow("server has no tile: http 404") << mapType << 9.30 << 14 << QByteArray() << 404 << false << false
                                                  << false << QStringLiteral("_savedTileCount=0");
    // The set is not done while a tile is being fetched: that tile can fail. Few enough tiles to be fetched at once.
    QTest::newRow("last tile fails after the others are stored")
        << mapType << 8.80 << 15 << tileReplyPngBody() << 200 << true << false << true
        << QStringLiteral("Error fetching tile");
}

void CachedTileSetTest::_testDownload()
{
    QFETCH(QString, providerType);
    QFETCH(double, longitude);
    QFETCH(int, maxZoom);
    QFETCH(QByteArray, serverBody);
    QFETCH(int, httpStatus);
    QFETCH(bool, stored);
    QFETCH(bool, failed);
    QFETCH(bool, lastTileFails);
    QFETCH(QString, expectedWarning);

    QVERIFY(!providerType.isEmpty());

    const QString name = QStringLiteral("download test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<CachedTileSet> set = tileSetTestSavedSet(name, providerType, longitude, maxZoom);
    QVERIFY(set);
    const quint32 tileCount = set->totalTileCount();
    QCOMPARE_GE(tileCount, static_cast<quint32>(maxZoom - kTileSetTestMinZoom + 1));
    const quint32 failedCount = lastTileFails ? 1U : (failed ? tileCount : 0U);
    const quint32 storedCount = stored ? (tileCount - failedCount) : 0U;

    TileReplyMockNam nam;
    nam.body = serverBody;
    nam.httpStatus = httpStatus;
    if (httpStatus == 404) {
        nam.error = QNetworkReply::ContentNotFoundError;  // as Qt reports that status
    }
    nam.holdReplies = lastTileFails;
    const TileReplyNetworkScope networkScope(&nam);
    // What a view bound to downloadStatus shows: the text it read when it was bound, or when the property last
    // notified
    QString shownStatus = set->downloadStatus();
    (void) connect(set.get(), &CachedTileSet::downloadStatusChanged, this,
                   [&shownStatus, &set] { shownStatus = set->downloadStatus(); });
    if (!expectedWarning.isEmpty()) {
        // The downloader warns of a tile that it cannot fetch, and the set of a download that saved none
        const char* const category = expectedWarning.startsWith(QStringLiteral("Error fetching"))
                                         ? "MapTiles.TileSetDownloader"
                                         : "MapTiles.CachedTileSet";
        expectLogMessage(category, QtWarningMsg, QRegularExpression(expectedWarning));
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
    // default set, and it is complete unless a tile is left to download.
    QList<CachedTileSet*> databaseSets;
    const auto deleteSets = qScopeGuard([&databaseSets] { qDeleteAll(databaseSets); });
    // The context of a task's signal is declared after what the signal writes, so that a late signal is dropped
    const QObject fetchSetsContext;
    QVERIFY(TileCache::instance()->addTask(TileSetTasks::fetchTileSets(
        &fetchSetsContext, [&databaseSets](const QList<CachedTileSet*>& tileSets) { databaseSets = tileSets; })));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    const auto databaseSet =
        std::find_if(databaseSets.cbegin(), databaseSets.cend(),
                     [&name](const CachedTileSet* candidate) { return candidate->name() == name; });
    QVERIFY(databaseSet != databaseSets.cend());
    QCOMPARE((*databaseSet)->savedTileCount(), storedCount);
    QCOMPARE((*databaseSet)->complete(), failedCount == 0U);
}

void CachedTileSetTest::_testDownloadEndsWhenTilesCannotBeListed_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("databaseFails");
    QTest::addColumn<bool>("resume");

    QTest::newRow("cache that is shut down") << 9.40 << false << false;
    QTest::newRow("cache that cannot open its database") << 9.50 << true << false;
    // The failed tiles are not listed again either: the download must not end as one that has every tile
    QTest::newRow("resumed on a cache that is shut down") << 9.60 << false << true;
}

// A set whose tiles the cache cannot list has nothing to wait for. Its download ends with an error, which leaves it
// to be resumed: it would otherwise show a download without end.
void CachedTileSetTest::_testDownloadEndsWhenTilesCannotBeListed()
{
    QFETCH(double, longitude);
    QFETCH(bool, databaseFails);
    QFETCH(bool, resume);

    const QString name = QStringLiteral("list failure test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<CachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestMinZoom);
    QVERIFY(set);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);

    TileCache* const cache = TileCache::instance();
    const auto restoreCache = qScopeGuard([cache] {
        cache->shutdown();
        UnitTestTileGenerator::initTileCache();
    });
    cache->shutdown();
    if (databaseFails) {
        // The cache worker ends when it cannot open its database, and ends every task with an error from then on
        expectLogMessage("MapTiles.TileCacheDatabase", QtCriticalMsg,
                         QRegularExpression(QStringLiteral("Could not find suitable cache directory")));
        expectLogMessage("MapTiles.CacheWorker", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed To Init Database")));
        expectLogMessage("MapTiles.TileCache", QtWarningMsg,
                         QRegularExpression(QStringLiteral("The tile cache could not be opened")));
        expectAppMessage(QRegularExpression(QStringLiteral("The map tile cache could not be opened")));
        cache->init(QString());
        const auto taskRefused = [cache] {
            return !cache->addTask(TileSetTasks::renameTileSet(0, QStringLiteral("probe")));
        };
        QVERIFY(QTest::qWaitFor(taskRefused, TestTimeout::mediumMs()));
        // The cache tells of the failure with the end of its first task, which is queued once the worker has ended
        QCoreApplication::processEvents();
        for (int i = 0; i < 4; i++) {
            verifyExpectedLogMessage();
        }
    }

    if (resume) {
        expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                         QRegularExpression(QStringLiteral("could not be reset to download them again")));
    }
    expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No list of the tiles to download")));
    if (resume) {
        set->resumeDownloadTask();
    } else {
        set->createDownloadTask();
    }
    QVERIFY(set->downloading());

    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    if (resume) {
        verifyExpectedLogMessage();
    }
    verifyExpectedLogMessage();
    QCOMPARE(set->errorCount(), 1U);
    QVERIFY(!set->complete());
    QCOMPARE(nam.requestCount, 0);
}

void CachedTileSetTest::_testDownloadEndsWithErrorWhenDatabaseFails_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("resume");
    QTest::addColumn<int>("refusedState");
    QTest::addColumn<QString>("databaseWarning");

    // The tiles to download are marked as being downloaded when they are listed
    QTest::newRow("tiles cannot be listed") << 9.70 << false << static_cast<int>(DownloadTile::TileState::Downloading)
                                            << QStringLiteral("batch set TilesDownload state");
    // A resumed download has the tiles that failed marked as pending again
    QTest::newRow("failed tiles cannot be listed again")
        << 9.80 << true << static_cast<int>(DownloadTile::TileState::Pending)
        << QStringLiteral("set TilesDownload states of set");
}

// A database that fails tells of no tile to download, as one does whose set has every tile: the set must not be
// taken for complete then
void CachedTileSetTest::_testDownloadEndsWithErrorWhenDatabaseFails()
{
    QFETCH(double, longitude);
    QFETCH(bool, resume);
    QFETCH(int, refusedState);
    QFETCH(QString, databaseWarning);

    const QString name = QStringLiteral("database failure test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<CachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestMinZoom);
    QVERIFY(set);
    const int tileCount = static_cast<int>(set->totalTileCount());

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    if (resume) {
        // Every tile of the set fails, which leaves none that is pending
        for (int i = 0; i < tileCount; i++) {
            expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                             QRegularExpression(QStringLiteral("Error fetching tile")));
        }
        set->createDownloadTask();
        QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
        for (int i = 0; i < tileCount; i++) {
            verifyExpectedLogMessage();
        }
        QCOMPARE(set->errorCount(), static_cast<quint32>(tileCount));
        // The states of the tiles are stored
        QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    }
    nam.body = tileReplyPngBody();

    const auto dropTrigger =
        qScopeGuard([this] { (void) tileSetTestCacheExec(this, QStringLiteral("DROP TRIGGER refuse_state")); });
    QVERIFY(tileSetTestCacheExec(this, QStringLiteral("CREATE TRIGGER refuse_state BEFORE UPDATE ON TilesDownload "
                                                      "WHEN NEW.state = %1 BEGIN SELECT RAISE(ABORT, 'refused'); END")
                                           .arg(refusedState)));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg, QRegularExpression(databaseWarning));
    if (resume) {
        expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                         QRegularExpression(QStringLiteral("could not be reset to download them again")));
    }
    expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No list of the tiles to download")));
    if (resume) {
        set->resumeDownloadTask();
    } else {
        set->createDownloadTask();
    }
    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    for (int i = 0; i < (resume ? 3 : 2); i++) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(set->errorCount(), 1U);
    QVERIFY(!set->complete());
    QCOMPARE(nam.requestCount, resume ? tileCount : 0);
}

void CachedTileSetTest::_testCancelDownload_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("cancelInSlot");

    QTest::newRow("while its requests are in flight") << 8.90 << false;
    // The set was about to go on with the download when it emitted the signal
    QTest::newRow("in a slot of a signal of the set") << 9.00 << true;
}

// A cancel stops the download at once: also when the set has been given all its tiles to download, which a set
// with fewer of them than the database gives it at a time has from the start
void CachedTileSetTest::_testCancelDownload()
{
    QFETCH(double, longitude);
    QFETCH(bool, cancelInSlot);

    const QString name = QStringLiteral("cancel test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<CachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestManyTilesZoom);
    QVERIFY(set);
    QCOMPARE_GT(set->totalTileCount(), static_cast<quint32>(kTileSetTestRequestsAtOnce));
    QCOMPARE_LT(set->totalTileCount(), kTileSetTestDownloadListSize);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    set->createDownloadTask();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, kTileSetTestRequestsAtOnce, TestTimeout::mediumMs());

    quint32 savedCount = 0;
    if (cancelInSlot) {
        (void) connect(set.get(), &CachedTileSet::savedTileCountChanged, set.get(), &CachedTileSet::cancelDownloadTask);
        nam.replies.constFirst()->finishNow();
        savedCount = 1;
    } else {
        set->cancelDownloadTask();
    }
    QVERIFY(!set->downloading());
    QVERIFY(!set->complete());
    QCOMPARE(set->savedTileCount(), savedCount);

    // The requests in flight are aborted, which deletes their replies. They are no errors of the set, and no tile is
    // requested in their place.
    const auto repliesDeleted = [&nam] {
        return std::all_of(nam.replies.cbegin(), nam.replies.cend(),
                           [](const QPointer<TileReplyCannedReply>& reply) { return reply.isNull(); });
    };
    QTRY_VERIFY_WITH_TIMEOUT(repliesDeleted(), TestTimeout::mediumMs());
    QCOMPARE(set->errorCount(), 0U);
    QCOMPARE(nam.requestCount, kTileSetTestRequestsAtOnce);
    QVERIFY(!set->downloading());
}

void CachedTileSetTest::_testResumeAfterCancel_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("requestsInFlight");

    QTest::newRow("cancelled with requests in flight") << 9.10 << true;
    // The list is of the download that was cancelled: the resumed one asks for a list of its own
    QTest::newRow("cancelled before its download list arrived") << 9.20 << false;
}

// A resume downloads every tile that the cancelled download left, once
void CachedTileSetTest::_testResumeAfterCancel()
{
    QFETCH(double, longitude);
    QFETCH(bool, requestsInFlight);

    const QString name = QStringLiteral("resume test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<CachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestManyTilesZoom);
    QVERIFY(set);
    const quint32 tileCount = set->totalTileCount();
    QCOMPARE_GT(tileCount, static_cast<quint32>(kTileSetTestRequestsAtOnce));
    QCOMPARE_LT(tileCount, kTileSetTestDownloadListSize);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    set->createDownloadTask();
    if (requestsInFlight) {
        QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, kTileSetTestRequestsAtOnce, TestTimeout::mediumMs());
    }
    const int abortedCount = nam.requestCount;
    set->cancelDownloadTask();

    set->resumeDownloadTask();
    QVERIFY(set->downloading());
    // No request is answered before the set has all its download lists: one list too many would add its tiles to
    // those that the set downloads
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, abortedCount + kTileSetTestRequestsAtOnce, TestTimeout::mediumMs());
    nam.holdReplies = false;
    const QList<QPointer<TileReplyCannedReply>> heldReplies = nam.replies;
    for (const QPointer<TileReplyCannedReply>& reply : heldReplies) {
        if (reply) {
            reply->finishNow();
        }
    }

    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    QCOMPARE(set->savedTileCount(), tileCount);
    QVERIFY(set->complete());
    QCOMPARE(set->errorCount(), 0U);
    QCOMPARE(nam.requestCount, abortedCount + static_cast<int>(tileCount));
}

UT_REGISTER_TEST(CachedTileSetTest, TestLabel::Unit)
