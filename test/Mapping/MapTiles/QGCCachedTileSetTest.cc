#include "QGCCachedTileSetTest.h"

#include <algorithm>
#include <memory>

#include <QtCore/QPointer>
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

namespace {

// The region of a download test lies within one tile at this zoom
constexpr int kTileSetTestMinZoom = 14;
constexpr double kTileSetTestTopLat = 47.3905;
constexpr double kTileSetTestBottomLat = 47.3895;
constexpr double kTileSetTestHalfLonSpan = 0.0005;
// With this zoom the region has more tiles than a set requests at a time, and fewer than the database gives a set
// to download at a time
constexpr int kTileSetTestManyTilesZoom = 19;
constexpr int kTileSetTestRequestsAtOnce = 6;
constexpr quint32 kTileSetTestDownloadListSize = 256;

/// A set of \a providerType that is saved in the tile cache database and has downloaded nothing. Its region lies
/// around \a longitude, and its zooms are kTileSetTestMinZoom to \a maxZoom. Null when it could not be saved.
std::unique_ptr<QGCCachedTileSet> tileSetTestSavedSet(const QString& name, const QString& providerType,
                                                      double longitude, int maxZoom)
{
    const double leftLon = longitude - kTileSetTestHalfLonSpan;
    const double rightLon = longitude + kTileSetTestHalfLonSpan;
    quint32 tileCount = 0;
    for (int zoom = kTileSetTestMinZoom; zoom <= maxZoom; zoom++) {
        const QGCTileSet tiles =
            UrlFactory::getTileCount(zoom, leftLon, kTileSetTestTopLat, rightLon, kTileSetTestBottomLat, providerType);
        tileCount += tiles.tileCount;
    }

    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();

    QGCCachedTileSet* const newSet = new QGCCachedTileSet(name);
    newSet->setMapTypeStr(providerType);
    newSet->setType(providerType);
    newSet->setTopleftLat(kTileSetTestTopLat);
    newSet->setTopleftLon(leftLon);
    newSet->setBottomRightLat(kTileSetTestBottomLat);
    newSet->setBottomRightLon(rightLon);
    newSet->setMinZoom(kTileSetTestMinZoom);
    newSet->setMaxZoom(maxZoom);
    newSet->setTotalTileCount(tileCount);

    bool saved = false;
    // Declared after what its signal writes, so that a late signal is dropped
    const QObject savedContext;
    // The task owns the set until it is saved
    QGCCreateTileSetTask* const createTask = new QGCCreateTileSetTask(newSet);
    (void) QObject::connect(createTask, &QGCCreateTileSetTask::tileSetSaved, &savedContext, [&saved] { saved = true; });
    if (!engine->addTask(createTask)) {
        createTask->deleteLater();
        return nullptr;
    }
    if (!QTest::qWaitFor([&saved] { return saved; }, TestTimeout::mediumMs())) {
        return nullptr;
    }
    return std::unique_ptr<QGCCachedTileSet>(newSet);
}

}  // namespace

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

void QGCCachedTileSetTest::_testDownload()
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
    const std::unique_ptr<QGCCachedTileSet> set = tileSetTestSavedSet(name, providerType, longitude, maxZoom);
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
    // default set, and it is complete unless a tile is left to download.
    QList<QGCCachedTileSet*> databaseSets;
    const auto deleteSets = qScopeGuard([&databaseSets] { qDeleteAll(databaseSets); });
    // The context of a task's signal is declared after what the signal writes, so that a late signal is dropped
    const QObject fetchSetsContext;
    QGCFetchTileSetTask* const fetchSetsTask = new QGCFetchTileSetTask();
    connect(fetchSetsTask, &QGCFetchTileSetTask::tileSetFetched, &fetchSetsContext,
            [&databaseSets](QGCCachedTileSet* databaseSet) { databaseSets.append(databaseSet); });
    QVERIFY(getQGCMapEngine()->addTask(fetchSetsTask));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    const auto databaseSet =
        std::find_if(databaseSets.cbegin(), databaseSets.cend(),
                     [&name](const QGCCachedTileSet* candidate) { return candidate->name() == name; });
    QVERIFY(databaseSet != databaseSets.cend());
    QCOMPARE((*databaseSet)->savedTileCount(), storedCount);
    QCOMPARE((*databaseSet)->complete(), failedCount == 0U);
}

void QGCCachedTileSetTest::_testCancelDownload_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("cancelInSlot");

    QTest::newRow("while its requests are in flight") << 8.90 << false;
    // The set was about to go on with the download when it emitted the signal
    QTest::newRow("in a slot of a signal of the set") << 9.00 << true;
}

// A cancel stops the download at once: also when the set has been given all its tiles to download, which a set
// with fewer of them than the database gives it at a time has from the start
void QGCCachedTileSetTest::_testCancelDownload()
{
    QFETCH(double, longitude);
    QFETCH(bool, cancelInSlot);

    const QString name = QStringLiteral("cancel test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<QGCCachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestManyTilesZoom);
    QVERIFY(set);
    QCOMPARE_GT(set->totalTileCount(), static_cast<quint32>(kTileSetTestRequestsAtOnce));
    QCOMPARE_LT(set->totalTileCount(), kTileSetTestDownloadListSize);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    set->setNetworkManager(&nam);
    set->createDownloadTask();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, kTileSetTestRequestsAtOnce, TestTimeout::mediumMs());

    quint32 savedCount = 0;
    if (cancelInSlot) {
        (void) connect(set.get(), &QGCCachedTileSet::savedTileCountChanged, set.get(),
                       &QGCCachedTileSet::cancelDownloadTask);
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

void QGCCachedTileSetTest::_testResumeAfterCancel_data()
{
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<bool>("requestsInFlight");

    QTest::newRow("cancelled with requests in flight") << 9.10 << true;
    // The list is of the download that was cancelled: the resumed one asks for a list of its own
    QTest::newRow("cancelled before its download list arrived") << 9.20 << false;
}

// A resume downloads every tile that the cancelled download left, once
void QGCCachedTileSetTest::_testResumeAfterCancel()
{
    QFETCH(double, longitude);
    QFETCH(bool, requestsInFlight);

    const QString name = QStringLiteral("resume test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const std::unique_ptr<QGCCachedTileSet> set =
        tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, kTileSetTestManyTilesZoom);
    QVERIFY(set);
    const quint32 tileCount = set->totalTileCount();
    QCOMPARE_GT(tileCount, static_cast<quint32>(kTileSetTestRequestsAtOnce));
    QCOMPARE_LT(tileCount, kTileSetTestDownloadListSize);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    set->setNetworkManager(&nam);
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

UT_REGISTER_TEST(QGCCachedTileSetTest, TestLabel::Unit)
