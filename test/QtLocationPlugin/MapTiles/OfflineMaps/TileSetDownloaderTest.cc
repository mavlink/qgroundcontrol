#include "TileSetDownloaderTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "CachedTileSet.h"
#include "DownloadTile.h"
#include "TileCache.h"
#include "TileCacheTestHelpers.h"
#include "TileReplyNetworkMock.h"
#include "TileSetDownloader.h"
#include "TileSetTestHelpers.h"
#include "UnitTestTileGenerator.h"

namespace {

// The downloader tells the cache what became of its tiles this many at a time
constexpr int kDownloaderTestStateBatchSize = 64;
// With this zoom the region has more tiles than that, and fewer than the database gives a set to download at a time
constexpr int kDownloaderTestBatchesZoom = 21;
// With this zoom the region has more tiles than the database gives a set to download at a time
constexpr int kDownloaderTestSeveralListsZoom = 22;

/// A set that is saved in the tile cache of the process, with what a test reads of the cache about it
class DownloaderTestSet
{
public:
    DownloaderTestSet(const QString& name, double longitude, int maxZoom)
        : _set(tileSetTestSavedSet(name, UnitTestTileGenerator::mapProviderType(), longitude, maxZoom))
    {}

    bool isSaved() const { return _set != nullptr; }

    quint64 id() const { return _set->id(); }

    int tileCount() const { return static_cast<int>(_set->totalTileCount()); }

    /// The tiles that the set is still to download, by their state, once the cache has run the tasks it has.
    /// -1 when they cannot be counted.
    int tilesToDownload(DownloadTile::TileState state) const
    {
        return _count(QStringLiteral("SELECT COUNT(*) FROM TilesDownload WHERE setID = %1 AND state = %2")
                          .arg(id())
                          .arg(static_cast<int>(state)));
    }

    /// The tiles that are stored in the set
    int tilesStored() const
    {
        return _count(QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = %1").arg(id()));
    }

private:
    int _count(const QString& sql) const
    {
        const QObject context;
        const std::optional<QStringList> rows = tileCacheWorkerRows(&context, tileCacheProcessAddTask, sql);
        return (rows && (rows->size() == 1)) ? rows->constFirst().toInt() : -1;
    }

    const std::unique_ptr<CachedTileSet> _set;
};

}  // namespace

void TileSetDownloaderTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

// The states of the last tiles are fewer than the downloader stores at a time: they are stored when it is done
void TileSetDownloaderTest::_testEndOfDownloadStoresEveryState()
{
    const DownloaderTestSet set(QStringLiteral("end of download test"), 20.10, kTileSetTestManyTilesZoom);
    QVERIFY(set.isSaved());
    QCOMPARE_LT(set.tileCount(), kDownloaderTestStateBatchSize);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy savedSpy(&downloader, &TileSetDownloader::tileSaved);
    QSignalSpy failedSpy(&downloader, &TileSetDownloader::tileFailed);
    QSignalSpy finishedSpy(&downloader, &TileSetDownloader::finished);
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());

    QCOMPARE(savedSpy.count(), set.tileCount());
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(nam.requestCount, set.tileCount());
    QCOMPARE(set.tilesStored(), set.tileCount());
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), 0);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Pending), 0);
}

// A task of the cache and a transaction for the state of each tile would be as many as for the tiles themselves
void TileSetDownloaderTest::_testStatesAreStoredInBatches()
{
    const DownloaderTestSet set(QStringLiteral("state batches test"), 20.20, kDownloaderTestBatchesZoom);
    QVERIFY(set.isSaved());
    const int tileCount = set.tileCount();
    QCOMPARE_GT(tileCount, kDownloaderTestStateBatchSize + 1);
    QCOMPARE_LT(tileCount, static_cast<int>(kTileSetTestDownloadListSize));

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy savedSpy(&downloader, &TileSetDownloader::tileSaved);
    downloader.start(set.id());

    // The server answers one request after the other: each answer lets the downloader make its next request
    const auto answerNext = [&nam, &savedSpy] {
        const int answered = savedSpy.count();
        if (!QTest::qWaitFor([&nam, answered] { return nam.replies.size() > answered; }, TestTimeout::mediumMs())) {
            return false;
        }
        nam.replies.at(answered)->finishNow();
        return QTest::qWaitFor([&savedSpy, answered] { return savedSpy.count() == (answered + 1); },
                               TestTimeout::mediumMs());
    };

    for (int i = 0; i < (kDownloaderTestStateBatchSize - 1); i++) {
        QVERIFY(answerNext());
    }
    // Every tile of the set is being downloaded as far as the cache knows
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount);

    QVERIFY(answerNext());
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount - kDownloaderTestStateBatchSize);

    // The next batch has begun, and a cancel stores what there is of it
    QVERIFY(answerNext());
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount - kDownloaderTestStateBatchSize);
    downloader.cancel();
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount - kDownloaderTestStateBatchSize - 1);
    QCOMPARE(set.tilesStored(), kDownloaderTestStateBatchSize + 1);
}

// The cache lists the tiles of a set a list at a time: the downloader asks for the next list as it nears the end of
// the one it has, until a list has no tile left
void TileSetDownloaderTest::_testSetOfSeveralListsIsDownloadedWhole()
{
    const DownloaderTestSet set(QStringLiteral("several lists test"), 20.60, kDownloaderTestSeveralListsZoom);
    QVERIFY(set.isSaved());
    const int tileCount = set.tileCount();
    QCOMPARE_GT(tileCount, static_cast<int>(kTileSetTestDownloadListSize));

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy savedSpy(&downloader, &TileSetDownloader::tileSaved);
    QSignalSpy failedSpy(&downloader, &TileSetDownloader::tileFailed);
    QSignalSpy finishedSpy(&downloader, &TileSetDownloader::finished);
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::longMs());

    // Each tile once
    QCOMPARE(nam.requestCount, tileCount);
    QCOMPARE(savedSpy.count(), tileCount);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(set.tilesStored(), tileCount);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), 0);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Pending), 0);
}

void TileSetDownloaderTest::_testCancelKeepsWhatWasFetchedAndRetryFetchesTheRest()
{
    const DownloaderTestSet set(QStringLiteral("cancel and retry test"), 20.30, kTileSetTestManyTilesZoom);
    QVERIFY(set.isSaved());
    const int tileCount = set.tileCount();
    QCOMPARE_GT(tileCount, kTileSetTestRequestsAtOnce + 2);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy savedSpy(&downloader, &TileSetDownloader::tileSaved);
    QSignalSpy failedSpy(&downloader, &TileSetDownloader::tileFailed);
    QSignalSpy finishedSpy(&downloader, &TileSetDownloader::finished);
    downloader.start(set.id());
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, kTileSetTestRequestsAtOnce, TestTimeout::mediumMs());

    // Two tiles are fetched, and the requests that take their places are in flight with the others
    nam.replies.at(0)->finishNow();
    nam.replies.at(1)->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(savedSpy, 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, kTileSetTestRequestsAtOnce + 2, TestTimeout::mediumMs());
    const int requestsBeforeCancel = nam.requestCount;

    downloader.cancel();
    // The requests that were aborted are neither tiles that failed nor the end of a download
    QCOMPARE(set.tilesStored(), 2);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount - 2);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);

    // Without retry() the cache lists none of the tiles that the cancelled download was given
    nam.holdReplies = false;
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, requestsBeforeCancel);

    downloader.retry(set.id());
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, requestsBeforeCancel + tileCount - 2);
    QCOMPARE(savedSpy.count(), tileCount);
    QCOMPARE(set.tilesStored(), tileCount);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), 0);
}

void TileSetDownloaderTest::_testFailedTilesStayForRetry()
{
    const DownloaderTestSet set(QStringLiteral("failed tiles test"), 20.40, kTileSetTestMinZoom);
    QVERIFY(set.isSaved());
    const int tileCount = set.tileCount();

    TileReplyMockNam nam;
    nam.error = QNetworkReply::TimeoutError;
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy savedSpy(&downloader, &TileSetDownloader::tileSaved);
    QSignalSpy failedSpy(&downloader, &TileSetDownloader::tileFailed);
    QSignalSpy finishedSpy(&downloader, &TileSetDownloader::finished);
    for (int i = 0; i < tileCount; i++) {
        expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Error fetching tile")));
    }
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    for (int i = 0; i < tileCount; i++) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(failedSpy.count(), tileCount);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Error), tileCount);
    QCOMPARE(set.tilesStored(), 0);

    // A download that follows asks for none of them, and ends
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, tileCount);

    downloader.retry(set.id());
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 3, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2 * tileCount);
    QCOMPARE(savedSpy.count(), tileCount);
    QCOMPARE(set.tilesStored(), tileCount);
    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Error), 0);
}

// The cache marks the tiles that it lists as being downloaded. A download that is cancelled before its list arrives
// requests none of them, and they stay for retry().
void TileSetDownloaderTest::_testListOfCancelledDownloadIsNotUsed()
{
    const DownloaderTestSet set(QStringLiteral("cancelled list test"), 20.50, kTileSetTestManyTilesZoom);
    QVERIFY(set.isSaved());
    const int tileCount = set.tileCount();

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    const TileReplyNetworkScope networkScope(&nam);
    TileSetDownloader downloader;
    QSignalSpy finishedSpy(&downloader, &TileSetDownloader::finished);
    downloader.start(set.id());
    downloader.cancel();

    QCOMPARE(set.tilesToDownload(DownloadTile::TileState::Downloading), tileCount);
    // The list has arrived by now: the count above waited for the tasks of the cache, the list among them
    QCoreApplication::processEvents();
    QCOMPARE(nam.requestCount, 0);
    QCOMPARE(finishedSpy.count(), 0);

    downloader.retry(set.id());
    downloader.start(set.id());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, tileCount);
    QCOMPARE(set.tilesStored(), tileCount);
}

UT_REGISTER_TEST(TileSetDownloaderTest, TestLabel::Unit)
