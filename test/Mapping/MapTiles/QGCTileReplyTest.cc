#include "QGCTileReplyTest.h"

#include <memory>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QUrl>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGCTileReply.h"
#include "SettingsManager.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

/// Any map provider works: under unit tests a cache miss is served by UnitTestTileGenerator unless a miss is forced
int tileReplyMapId()
{
    return UrlFactory::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType());
}

constexpr int kTileX = 4302;
constexpr int kTileY = 2867;
constexpr int kTileZoom = 13;

/// The provider that fetches from the URL template the user sets
const QString kTileReplyCustomUrlType = QStringLiteral("CustomURL Custom");
const QString kTileReplySourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
const QString kTileReplySourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

/// What a captive portal answers in place of a tile, with the HTTP status of a tile
const QByteArray kTileReplyPageBody = QByteArrayLiteral("<html><body>Sign in to this network</body></html>");

enum class TileReplyCacheAnswer
{
    None,
    Stored,
    Missing,
};

/// Whether the tile cache has stored the tile \a hash. Forces the miss that UnitTestTileGenerator would otherwise
/// answer with a tile of its own, and leaves no forced miss behind.
TileReplyCacheAnswer tileReplyCacheAnswer(const QString& hash)
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReplyCacheAnswer answer = TileReplyCacheAnswer::None;
    // Declared after what its signals write, so that a late signal is dropped
    const QObject context;
    QGCFetchTileTask* const task = new QGCFetchTileTask(hash);
    (void) QObject::connect(task, &QGCFetchTileTask::tileFetched, &context,
                            [&answer] { answer = TileReplyCacheAnswer::Stored; });
    (void) QObject::connect(task, &QGCMapTask::error, &context, [&answer] { answer = TileReplyCacheAnswer::Missing; });
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        return TileReplyCacheAnswer::None;
    }
    (void) QTest::qWaitFor([&answer] { return answer != TileReplyCacheAnswer::None; }, TestTimeout::mediumMs());
    return answer;
}

}  // namespace

void QGCTileReplyTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void QGCTileReplyTest::_cachedTileServedWithoutNetwork()
{
    TileReplyMockNam nam;
    QGCTileReply reply(&nam, tileReplyMapId(), kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);

    reply.start();
    QVERIFY(!reply.isFinished());  // never finishes from within start()

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
    QVERIFY(reply.isCached());
    QVERIFY(!reply.data().isEmpty());
    QCOMPARE(reply.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 0);
}

void QGCTileReplyTest::_serverTileStoredInCache_data()
{
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("format");

    // A map tile is told by the signature of its picture format. Only the PNG is a picture beyond it.
    QTest::newRow("PNG") << (kTileX + 1) << tileReplyPngBody() << QStringLiteral("png");
    QTest::newRow("JPEG") << (kTileX + 70) << (QByteArray("\xFF\xD8\xFF\xE0", 4) + QByteArray(64, 'j'))
                          << QStringLiteral("jpg");
    QTest::newRow("GIF") << (kTileX + 71) << (QByteArrayLiteral("GIF89a") + QByteArray(64, 'g'))
                         << QStringLiteral("gif");
    QTest::newRow("WebP") << (kTileX + 72) << (QByteArray("RIFF\x48\x00\x00\x00WEBP", 12) + QByteArray(64, 'w'))
                          << QStringLiteral("webp");
}

void QGCTileReplyTest::_serverTileStoredInCache()
{
    QFETCH(int, tileX);
    QFETCH(QByteArray, body);
    QFETCH(QString, format);

    TileReplyMockNam nam;
    nam.body = body;
    QVERIFY(!nam.body.isEmpty());

    // The second forced miss stays unconsumed when the stored tile answers the second lookup
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    QGCTileReply fetched(&nam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &QGCTileReply::finished);
    fetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!fetched.isCached());
    QCOMPARE(fetched.data(), nam.body);
    QCOMPARE(fetched.format(), format);
    QCOMPARE(nam.requestCount, 1);

    QGCTileReply cached(&nam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
    QCOMPARE(cached.format(), format);
    QCOMPARE(nam.requestCount, 1);
}

void QGCTileReplyTest::_serverFailure_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QGCTileReply::Error>("expectedError");

    const QString mapType = UrlFactory::getProviderTypeFromMapId(tileReplyMapId());
    QTest::newRow("network error") << mapType << (kTileX + 10) << QByteArray() << QNetworkReply::TimeoutError << 0
                                   << QGCTileReply::Error::NetworkError;
    QTest::newRow("http error status") << mapType << (kTileX + 11) << tileReplyPngBody() << QNetworkReply::NoError
                                       << 500 << QGCTileReply::Error::NetworkError;
    QTest::newRow("empty body") << mapType << (kTileX + 12) << QByteArray() << QNetworkReply::NoError << 200
                                << QGCTileReply::Error::DataError;
    QTest::newRow("Bing placeholder") << QStringLiteral("Bing Hybrid") << (kTileX + 13) << tileReplyBingPlaceholder()
                                      << QNetworkReply::NoError << 200 << QGCTileReply::Error::NoTileError;
    // A server answers 404, which Qt reports as this error, where it has no tile
    QTest::newRow("http 404") << mapType << (kTileX + 14) << QByteArray() << QNetworkReply::ContentNotFoundError << 404
                              << QGCTileReply::Error::NoTileError;
    // A URL that the user typed wrong is answered the same way: that must not pass for a place without a tile
    QTest::newRow("http 404 of the Custom URL map")
        << kTileReplyCustomUrlType << (kTileX + 15) << QByteArray() << QNetworkReply::ContentNotFoundError << 404
        << QGCTileReply::Error::NetworkError;
    QTest::newRow("page in place of a picture") << mapType << (kTileX + 16) << kTileReplyPageBody
                                                << QNetworkReply::NoError << 200 << QGCTileReply::Error::DataError;
}

void QGCTileReplyTest::_serverFailure()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(QByteArray, body);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QGCTileReply::Error, expectedError);

    const int mapId = UrlFactory::getMapIdFromProviderType(providerType);

    // The Custom URL map fetches from the URL that the user set
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, kTileReplySourceA);

    TileReplyMockNam nam;
    nam.body = body;
    nam.error = networkError;
    nam.httpStatus = httpStatus;

    UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    QGCTileReply failed(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy failedSpy(&failed, &QGCTileReply::finished);
    failed.start();
    QVERIFY_SIGNAL_COUNT_WAIT(failedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(failed.error(), expectedError);
    QVERIFY(!failed.errorString().isEmpty());
    QVERIFY(failed.data().isEmpty());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(tileReplyCacheAnswer(failed.hash()), TileReplyCacheAnswer::Missing);

    // The next request asks the server again
    forcedMisses.setCount(1);
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    QGCTileReply retried(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy retriedSpy(&retried, &QGCTileReply::finished);
    retried.start();
    QVERIFY_SIGNAL_COUNT_WAIT(retriedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(retried.error(), QGCTileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 2);
}

// A request that cannot be made is answered as every other request is: by finished(), after start() returned
void QGCTileReplyTest::_unknownMapIdFinishesWithError()
{
    TileReplyMockNam nam;
    QGCTileReply reply(&nam, -1, kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);

    reply.start();
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), QGCTileReply::Error::RequestError);
    QVERIFY(reply.errorString().contains(QStringLiteral("-1")));
    QVERIFY(reply.data().isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

void QGCTileReplyTest::_secondStartDoesNothing_data()
{
    QTest::addColumn<int>("mapId");

    QTest::newRow("request that is made") << tileReplyMapId();
    QTest::newRow("request that cannot be made") << -1;
}

void QGCTileReplyTest::_secondStartDoesNothing()
{
    QFETCH(int, mapId);

    TileReplyMockNam nam;
    QGCTileReply reply(&nam, mapId, kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);

    reply.start();
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    reply.start();

    // What a further start had begun would be answered before this lookup is
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(finishedSpy.count(), 1);
}

void QGCTileReplyTest::_validatorRejectsCachedAndServerTiles()
{
    const QByteArray png = tileReplyPngBody();
    const auto acceptPng = [png](const QByteArray& data) { return data == png; };

    TileReplyMockNam nam;
    nam.body = png;

    // The tile UnitTestTileGenerator serves as the cached one is not the accepted tile: the server is asked
    QGCTileReply refetched(&nam, tileReplyMapId(), kTileX + 20, kTileY, kTileZoom);
    refetched.setValidator(acceptPng);
    QSignalSpy refetchedSpy(&refetched, &QGCTileReply::finished);
    refetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(refetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!refetched.isCached());
    QCOMPARE(refetched.data(), png);
    QCOMPARE(nam.requestCount, 1);

    // A rejected server tile fails and is not stored: a request accepting any tile still has to ask the server
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);
    nam.body = tileReplyPngBody(Qt::red);
    QVERIFY(!nam.body.isEmpty() && (nam.body != png));
    QGCTileReply rejected(&nam, tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    rejected.setValidator(acceptPng);
    QSignalSpy rejectedSpy(&rejected, &QGCTileReply::finished);
    rejected.start();
    QVERIFY_SIGNAL_COUNT_WAIT(rejectedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(rejected.error(), QGCTileReply::Error::DataError);
    QCOMPARE(nam.requestCount, 2);

    QGCTileReply unvalidated(&nam, tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    QSignalSpy unvalidatedSpy(&unvalidated, &QGCTileReply::finished);
    unvalidated.start();
    QVERIFY_SIGNAL_COUNT_WAIT(unvalidatedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unvalidated.error(), QGCTileReply::Error::NoError);
    QVERIFY(!unvalidated.isCached());
    QCOMPARE(nam.requestCount, 3);
}

void QGCTileReplyTest::_unusableCachedTileIsReplaced_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("unusable");
    QTest::addColumn<bool>("rejectedByValidator");

    const QString mapType = UrlFactory::getProviderTypeFromMapId(tileReplyMapId());
    QTest::newRow("tile that the validator rejects") << mapType << (kTileX + 40) << tileReplyPngBody(Qt::red) << true;
    // The picture that Bing sends for a place without imagery, which the cache of an older version can hold
    QTest::newRow("no-tile picture of the provider")
        << QStringLiteral("Bing Hybrid") << (kTileX + 41) << tileReplyBingPlaceholder() << false;
    QTest::newRow("page in place of a picture") << mapType << (kTileX + 42) << kTileReplyPageBody << false;
}

// A tile in the cache that cannot be used, as a notice of refusal that was cached as a tile is, gives way to the
// tile that is fetched in its place: it is not fetched again and again
void QGCTileReplyTest::_unusableCachedTileIsReplaced()
{
    QFETCH(QString, type);
    QFETCH(int, tileX);
    QFETCH(QByteArray, unusable);
    QFETCH(bool, rejectedByValidator);

    const QByteArray usable = tileReplyPngBody();
    QVERIFY(!unusable.isEmpty() && (unusable != usable));
    QGCTileReply::Validator rejectUnusable;
    if (rejectedByValidator) {
        rejectUnusable = [unusable](const QByteArray& data) { return data != unusable; };
    }

    const int mapId = UrlFactory::getMapIdFromProviderType(type);
    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();
    engine->cacheTile(type, UrlFactory::getTileHash(type, tileX, kTileY, kTileZoom), unusable, QStringLiteral("png"));

    TileReplyMockNam nam;
    nam.body = usable;

    QGCTileReply refetched(&nam, mapId, tileX, kTileY, kTileZoom);
    refetched.setValidator(rejectUnusable);
    QSignalSpy refetchedSpy(&refetched, &QGCTileReply::finished);
    refetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(refetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!refetched.isCached());
    QCOMPARE(refetched.data(), usable);
    QCOMPARE(nam.requestCount, 1);

    QGCTileReply cached(&nam, mapId, tileX, kTileY, kTileZoom);
    cached.setValidator(rejectUnusable);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), usable);
    QCOMPARE(nam.requestCount, 1);
}

void QGCTileReplyTest::_abortedReplyStaysSilent()
{
    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    QGCTileReply reply(&nam, tileReplyMapId(), kTileX + 2, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    reply.start();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    const QPointer<TileReplyCannedReply> networkReply(nam.replies.at(0));
    reply.abort();
    networkReply->finishNow();  // a late answer of the aborted fetch
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());

    // The fetch is released without waiting for it to finish
    QTRY_VERIFY_WITH_TIMEOUT(networkReply.isNull(), TestTimeout::mediumMs());
}

void QGCTileReplyTest::_abortedBeforeQueuedStepStaysSilent_data()
{
    QTest::addColumn<int>("mapId");
    QTest::addColumn<bool>("forTileSet");

    QTest::newRow("request that cannot be made") << -1 << false;
    QTest::newRow("tile set request") << tileReplyMapId() << true;
}

// start() leaves to the event loop what would finish the request from within it
void QGCTileReplyTest::_abortedBeforeQueuedStepStaysSilent()
{
    QFETCH(int, mapId);
    QFETCH(bool, forTileSet);

    TileReplyMockNam nam;
    QGCTileReply reply(&nam, mapId, kTileX + 7, kTileY, kTileZoom);
    if (forTileSet) {
        reply.setTileSet(1);
    }
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    reply.start();
    reply.abort();

    // The step that start() queued is delivered before this lookup is answered
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());
    QCOMPARE(nam.requestCount, 0);
}

void QGCTileReplyTest::_deletedDuringCacheLookup()
{
    TileReplyMockNam nam;
    {
        QGCTileReply deleted(&nam, tileReplyMapId(), kTileX + 3, kTileY, kTileZoom);
        deleted.start();
    }

    // The cache worker runs lookups in order: once this one is answered, the deleted reply's result has arrived too
    QGCTileReply sentinel(&nam, tileReplyMapId(), kTileX + 4, kTileY, kTileZoom);
    QSignalSpy sentinelSpy(&sentinel, &QGCTileReply::finished);
    sentinel.start();
    QVERIFY_SIGNAL_COUNT_WAIT(sentinelSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sentinel.error(), QGCTileReply::Error::NoError);
}

void QGCTileReplyTest::_networkManagerDestroyedFirst()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    // Gone before the cache miss: there is nothing to ask the server with
    auto nam = std::make_unique<TileReplyMockNam>();
    QGCTileReply unfetched(nam.get(), tileReplyMapId(), kTileX + 5, kTileY, kTileZoom);
    QSignalSpy unfetchedSpy(&unfetched, &QGCTileReply::finished);
    unfetched.start();
    nam.reset();
    QVERIFY_SIGNAL_COUNT_WAIT(unfetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unfetched.error(), QGCTileReply::Error::NetworkError);

    // Gone during the fetch: the manager deletes the network reply it owns, which the tile reply must not touch
    nam = std::make_unique<TileReplyMockNam>();
    nam->holdReplies = true;
    auto fetching = std::make_unique<QGCTileReply>(nam.get(), tileReplyMapId(), kTileX + 6, kTileY, kTileZoom);
    fetching->start();
    QTRY_COMPARE_WITH_TIMEOUT(nam->requestCount, 1, TestTimeout::mediumMs());
    nam.reset();
    fetching.reset();
}

// Two maps that show the same place ask for the same tiles at the same moment: the server is asked once
void QGCTileReplyTest::_concurrentRequestsShareOneFetch()
{
    const QByteArray png = tileReplyPngBody();

    // Every consumer of tiles has a network manager of its own
    TileReplyMockNam fetchingNam;
    fetchingNam.body = png;
    fetchingNam.holdReplies = true;
    TileReplyMockNam waitingNam;
    waitingNam.body = png;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(4);

    const int mapId = tileReplyMapId();
    const int tileX = kTileX + 50;
    QGCTileReply fetching(&fetchingNam, mapId, tileX, kTileY, kTileZoom);
    QGCTileReply waiting(&waitingNam, mapId, tileX, kTileY, kTileZoom);
    QGCTileReply rejecting(&waitingNam, mapId, tileX, kTileY, kTileZoom);
    rejecting.setValidator([](const QByteArray&) { return false; });
    QGCTileReply withdrawn(&waitingNam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy fetchingSpy(&fetching, &QGCTileReply::finished);
    QSignalSpy waitingSpy(&waiting, &QGCTileReply::finished);
    QSignalSpy rejectingSpy(&rejecting, &QGCTileReply::finished);
    QSignalSpy withdrawnSpy(&withdrawn, &QGCTileReply::finished);
    fetching.start();
    waiting.start();
    rejecting.start();
    withdrawn.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(fetchingNam.requestCount, 1);
    QCOMPARE(waitingNam.requestCount, 0);

    // A request that stops waiting leaves the fetch to the others
    withdrawn.abort();
    fetchingNam.replies.at(0)->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(waitingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetchingSpy.count(), 1);
    for (const QGCTileReply* const reply : {&fetching, &waiting}) {
        QCOMPARE(reply->error(), QGCTileReply::Error::NoError);
        QVERIFY(!reply->isCached());
        QCOMPARE(reply->data(), png);
        QCOMPARE(reply->format(), QStringLiteral("png"));
    }
    // Every request checks the tile for itself
    QVERIFY_SIGNAL_COUNT_WAIT(rejectingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(rejecting.error(), QGCTileReply::Error::DataError);
    QVERIFY(withdrawnSpy.isEmpty());
    QCOMPARE(waitingNam.requestCount, 0);
}

void QGCTileReplyTest::_waitingRequestFetchesAfterFetchWithoutTile_data()
{
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QString>("ending");

    QTest::newRow("aborted") << (kTileX + 51) << QStringLiteral("aborted");
    QTest::newRow("deleted") << (kTileX + 52) << QStringLiteral("deleted");
}

// A request must not wait for ever, or fail, because of what became of another consumer's request
void QGCTileReplyTest::_waitingRequestFetchesAfterFetchWithoutTile()
{
    QFETCH(int, tileX);
    QFETCH(QString, ending);

    const QByteArray png = tileReplyPngBody();
    TileReplyMockNam fetchingNam;
    fetchingNam.body = png;
    fetchingNam.holdReplies = true;
    TileReplyMockNam waitingNam;
    waitingNam.body = png;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    auto fetching = std::make_unique<QGCTileReply>(&fetchingNam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    QGCTileReply waiting(&waitingNam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy waitingSpy(&waiting, &QGCTileReply::finished);
    fetching->start();
    waiting.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(fetchingNam.requestCount, 1);
    QCOMPARE(waitingNam.requestCount, 0);

    if (ending == QStringLiteral("aborted")) {
        fetching->abort();
    } else {
        fetching.reset();
    }

    QVERIFY_SIGNAL_COUNT_WAIT(waitingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(waiting.error(), QGCTileReply::Error::NoError);
    QCOMPARE(waiting.data(), png);
    QCOMPARE(waitingNam.requestCount, 1);
}

void QGCTileReplyTest::_waitingRequestsGetErrorOfFetch_data()
{
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QGCTileReply::Error>("expectedError");

    QTest::newRow("server not reached") << (kTileX + 55) << QNetworkReply::TimeoutError << 0
                                        << QGCTileReply::Error::NetworkError;
    QTest::newRow("provider has no tile")
        << (kTileX + 56) << QNetworkReply::ContentNotFoundError << 404 << QGCTileReply::Error::NoTileError;
}

// The server would answer the waiting requests as it answered the fetch: they do not ask it in turn, each after the
// transfer timeout of the one before
void QGCTileReplyTest::_waitingRequestsGetErrorOfFetch()
{
    QFETCH(int, tileX);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QGCTileReply::Error, expectedError);

    TileReplyMockNam fetchingNam;
    fetchingNam.error = networkError;
    fetchingNam.httpStatus = httpStatus;
    fetchingNam.holdReplies = true;
    // A request that asked the server for itself would get a tile
    TileReplyMockNam waitingNam;
    waitingNam.body = tileReplyPngBody();

    constexpr int kWaitingCount = 3;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1 + kWaitingCount);

    const int mapId = tileReplyMapId();
    QGCTileReply fetching(&fetchingNam, mapId, tileX, kTileY, kTileZoom);
    fetching.start();
    QObject waitingOwner;
    QList<QGCTileReply*> waiting;
    for (int i = 0; i < kWaitingCount; i++) {
        waiting.append(new QGCTileReply(&waitingNam, mapId, tileX, kTileY, kTileZoom, &waitingOwner));
        waiting.constLast()->start();
    }
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(fetchingNam.requestCount, 1);

    fetchingNam.replies.at(0)->finishNow();
    QCOMPARE(fetching.error(), expectedError);
    for (const QGCTileReply* const reply : std::as_const(waiting)) {
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), TestTimeout::mediumMs());
        QCOMPARE(reply->error(), expectedError);
        QCOMPARE(reply->errorString(), fetching.errorString());
        QVERIFY(reply->data().isEmpty());
    }
    QCOMPARE(fetchingNam.requestCount, 1);
    QCOMPARE(waitingNam.requestCount, 0);
}

// The request that fetches a tile for an offline set stores it in that set: the tile another request fetches is
// not in the set
void QGCTileReplyTest::_tileSetRequestFetchesForItself()
{
    TileReplyMockNam mapNam;
    mapNam.holdReplies = true;
    TileReplyMockNam setNam;
    setNam.holdReplies = true;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    const int tileX = kTileX + 54;
    QGCTileReply forMap(&mapNam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    forMap.start();
    QTRY_COMPARE_WITH_TIMEOUT(mapNam.requestCount, 1, TestTimeout::mediumMs());

    QGCTileReply forSet(&setNam, tileReplyMapId(), tileX, kTileY, kTileZoom);
    forSet.setTileSet(1);
    forSet.start();
    QTRY_COMPARE_WITH_TIMEOUT(setNam.requestCount, 1, TestTimeout::mediumMs());
}

void QGCTileReplyTest::_tileOfOtherSourceNotServed()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    const int mapId = UrlFactory::getMapIdFromProviderType(kTileReplyCustomUrlType);

    // Each source misses the cache once; when it is configured again, its stored tile answers
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    struct Request
    {
        QString source;
        QByteArray tile;
        bool cached;
    };

    const QByteArray tileA = tileReplyPngBody();
    const QByteArray tileB = tileA + QByteArrayLiteral("of the other source");
    const QList<Request> requests = {
        {kTileReplySourceA, tileA, false},
        {kTileReplySourceB, tileB, false},
        {kTileReplySourceA, tileA, true},
        {kTileReplySourceB, tileB, true},
    };

    TileReplyMockNam nam;
    int serverRequests = 0;
    for (const Request& request : requests) {
        saved.setFactValue(customUrl, request.source);
        nam.body = request.tile;
        if (!request.cached) {
            serverRequests++;
        }

        QGCTileReply reply(&nam, mapId, kTileX + 30, kTileY, kTileZoom);
        QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
        reply.start();
        QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
        QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
        QCOMPARE(reply.isCached(), request.cached);
        QCOMPARE(reply.data(), request.tile);
        QCOMPARE(nam.requestCount, serverRequests);
    }
}

void QGCTileReplyTest::_sourceChangedDuringRequest()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    const int mapId = UrlFactory::getMapIdFromProviderType(kTileReplyCustomUrlType);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();

    // The source changes while the cache is asked for the tile of source A: the tile is fetched from source B
    saved.setFactValue(customUrl, kTileReplySourceA);
    QGCTileReply fetched(&nam, mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &QGCTileReply::finished);
    fetched.start();
    customUrl->setRawValue(kTileReplySourceB);
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), QGCTileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().url().host(), QUrl(kTileReplySourceB).host());

    // It is stored as a tile of source B
    QGCTileReply cached(&nam, mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
    QCOMPARE(nam.requestCount, 1);
}

void QGCTileReplyTest::_requestIdentity_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<bool>("namesApplication");

    // OpenStreetMap answers with a picture of a refusal when a request poses as a browser's or as one of its own
    // web site: its tile usage policy asks for the application's name, and for HTTPS
    QTest::newRow("OpenStreetMap") << QStringLiteral("Street Map") << (kTileX + 30) << true;
    QTest::newRow("Bing") << QStringLiteral("Bing Road") << (kTileX + 31) << false;
}

void QGCTileReplyTest::_requestIdentity()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(bool, namesApplication);

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    QGCTileReply reply(&nam, UrlFactory::getMapIdFromProviderType(providerType), tileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.count(), 1);

    const QNetworkRequest request = nam.requests.constFirst();
    const QString userAgent = request.header(QNetworkRequest::UserAgentHeader).toString();
    QCOMPARE(userAgent == QGCNetworkHelper::defaultUserAgent(), namesApplication);
    QCOMPARE(request.hasRawHeader("Referer"), !namesApplication);
    if (namesApplication) {
        QCOMPARE(request.url().scheme(), QStringLiteral("https"));
    }
}

// A cache without a database takes no lookup. The server needs no cache: the tile is fetched all the same.
void QGCTileReplyTest::_tileFetchedWhenCacheRefusesLookup()
{
    QGCMapEngine* const engine = getQGCMapEngine();
    const auto restoreCache = qScopeGuard([engine] {
        engine->shutdown();
        UnitTestTileGenerator::initMapEngine();
    });

    // The cache worker ends when it cannot open its database, and refuses every task from then on
    engine->shutdown();
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Could not find suitable cache directory")));
    expectLogMessage("MapTiles.QGCTileCacheWorker", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed To Init Database")));
    engine->init(QString());
    // The worker owns a task it takes, and deletes one it refuses
    const auto lookupRefused = [engine] { return !engine->addTask(new QGCFetchTileTask(QStringLiteral("probe"))); };
    QVERIFY(QTest::qWaitFor(lookupRefused, TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    QGCTileReply reply(&nam, tileReplyMapId(), kTileX + 60, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    reply.start();
    QVERIFY(!reply.isFinished());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
    QVERIFY(!reply.isCached());
    QCOMPARE(reply.data(), nam.body);
    QCOMPARE(nam.requestCount, 1);
}

UT_REGISTER_TEST(QGCTileReplyTest, TestLabel::Unit)
