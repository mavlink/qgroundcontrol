#include "TileReplyTest.h"

#include <chrono>
#include <memory>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimeZone>
#include <QtCore/QUrl>
#include <QtNetwork/QHttpHeaders>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "CacheTasks.h"
#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "ManualScheduler.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapProviderTable.h"
#include "QGCNetworkHelper.h"
#include "SettingsManager.h"
#include "TileCache.h"
#include "TileReply.h"
#include "TileReplyNetworkMock.h"
#include "TileServerPauses.h"
#include "UnitTestTileGenerator.h"

namespace {

/// Any map provider works: under unit tests a cache miss is served by UnitTestTileGenerator unless a miss is forced
int tileReplyMapId()
{
    return MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType());
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

/// The time at which a server's Retry-After is read
QDateTime tileReplyRetryAfterNow()
{
    return QDateTime(QDate(2026, 11, 18), QTime(10, 0), QTimeZone::UTC);
}

/// \a utc as an HTTP date
QByteArray tileReplyHttpDate(const QDateTime& utc)
{
    return QLocale::c().toString(utc, QStringLiteral("ddd, dd MMM yyyy hh:mm:ss 'GMT'")).toLatin1();
}

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
    CacheTask* const task = CacheTasks::fetchTile(hash, &context, [&answer](const CacheTile* tile) {
        answer = tile ? TileReplyCacheAnswer::Stored : TileReplyCacheAnswer::Missing;
    });
    if (!TileCache::instance()->addTask(task)) {
        return TileReplyCacheAnswer::None;
    }
    (void) QTest::qWaitFor([&answer] { return answer != TileReplyCacheAnswer::None; }, TestTimeout::mediumMs());
    return answer;
}

}  // namespace

void TileReplyTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void TileReplyTest::_cachedTileServedWithoutNetwork()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    TileReply reply(tileReplyMapId(), kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);

    reply.start();
    QVERIFY(!reply.isFinished());  // never finishes from within start()

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), TileReply::Error::NoError);
    QVERIFY(reply.isCached());
    QVERIFY(!reply.data().isEmpty());
    QCOMPARE(reply.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 0);
}

void TileReplyTest::_serverTileStoredInCache_data()
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

void TileReplyTest::_serverTileStoredInCache()
{
    QFETCH(int, tileX);
    QFETCH(QByteArray, body);
    QFETCH(QString, format);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = body;
    QVERIFY(!nam.body.isEmpty());

    // The second forced miss stays unconsumed when the stored tile answers the second lookup
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    TileReply fetched(tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &TileReply::finished);
    fetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), TileReply::Error::NoError);
    QVERIFY(!fetched.isCached());
    QCOMPARE(fetched.data(), nam.body);
    QCOMPARE(fetched.format(), format);
    QCOMPARE(nam.requestCount, 1);

    TileReply cached(tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &TileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
    QCOMPARE(cached.format(), format);
    QCOMPARE(nam.requestCount, 1);
}

void TileReplyTest::_serverFailure_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<TileReply::Error>("expectedError");
    QTest::addColumn<TileReply::NetworkFailure>("expectedFailure");

    using NetworkFailure = TileReply::NetworkFailure;
    const QString mapType = MapProviderRegistry::getProviderTypeFromMapId(tileReplyMapId());
    QTest::newRow("network error") << mapType << (kTileX + 10) << QByteArray() << QNetworkReply::TimeoutError << 0
                                   << TileReply::Error::NetworkError << NetworkFailure::Unreachable;
    QTest::newRow("http error status") << mapType << (kTileX + 11) << tileReplyPngBody() << QNetworkReply::NoError
                                       << 500 << TileReply::Error::NetworkError << NetworkFailure::Unavailable;
    QTest::newRow("empty body") << mapType << (kTileX + 12) << QByteArray() << QNetworkReply::NoError << 200
                                << TileReply::Error::DataError << NetworkFailure::None;
    QTest::newRow("Bing placeholder") << QStringLiteral("Bing Hybrid") << (kTileX + 13) << tileReplyBingPlaceholder()
                                      << QNetworkReply::NoError << 200 << TileReply::Error::NoTileError
                                      << NetworkFailure::None;
    // A server answers 404, which Qt reports as this error, where it has no tile
    QTest::newRow("http 404") << mapType << (kTileX + 14) << QByteArray() << QNetworkReply::ContentNotFoundError << 404
                              << TileReply::Error::NoTileError << NetworkFailure::None;
    // A URL that the user typed wrong is answered the same way: that must not pass for a place without a tile
    QTest::newRow("http 404 of the Custom URL map")
        << kTileReplyCustomUrlType << (kTileX + 15) << QByteArray() << QNetworkReply::ContentNotFoundError << 404
        << TileReply::Error::NetworkError << NetworkFailure::Rejected;
    QTest::newRow("page in place of a picture")
        << mapType << (kTileX + 16) << kTileReplyPageBody << QNetworkReply::NoError << 200
        << TileReply::Error::DataError << NetworkFailure::None;
    // As for a key of the provider that is not valid
    QTest::newRow("http 403") << mapType << (kTileX + 17) << QByteArray() << QNetworkReply::ContentAccessDenied << 403
                              << TileReply::Error::NetworkError << NetworkFailure::Rejected;
    // http 429 pauses the server: _rateLimitHoldsEveryRequestOfProvider()
}

void TileReplyTest::_serverFailure()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(QByteArray, body);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(TileReply::Error, expectedError);
    QFETCH(TileReply::NetworkFailure, expectedFailure);

    const int mapId = MapProviderRegistry::getMapIdFromProviderType(providerType);

    // The Custom URL map fetches from the URL that the user set
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, kTileReplySourceA);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = body;
    nam.error = networkError;
    nam.httpStatus = httpStatus;

    UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReply failed(mapId, tileX, kTileY, kTileZoom);
    QSignalSpy failedSpy(&failed, &TileReply::finished);
    failed.start();
    QVERIFY_SIGNAL_COUNT_WAIT(failedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(failed.error(), expectedError);
    QCOMPARE(failed.networkFailure(), expectedFailure);
    QVERIFY(!failed.errorString().isEmpty());
    QVERIFY(failed.data().isEmpty());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(tileReplyCacheAnswer(failed.hash()), TileReplyCacheAnswer::Missing);

    // The next request asks the server again
    forcedMisses.setCount(1);
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    TileReply retried(mapId, tileX, kTileY, kTileZoom);
    QSignalSpy retriedSpy(&retried, &TileReply::finished);
    retried.start();
    QVERIFY_SIGNAL_COUNT_WAIT(retriedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(retried.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 2);
}

// A request that cannot be made is answered as every other request is: by finished(), after start() returned
void TileReplyTest::_unknownMapIdFinishesWithError()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    TileReply reply(-1, kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);

    reply.start();
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), TileReply::Error::RequestError);
    QVERIFY(reply.errorString().contains(QStringLiteral("-1")));
    QVERIFY(reply.data().isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

// The map of the tiles that the user imported has no server: a tile that the cache lacks is one that the map has not,
// which a view fills from the zoom levels above, and no failed request
void TileReplyTest::_mapWithoutServerHasNoTileBeyondCache()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);
    TileReply reply(MapProviderTable::kImportedMapId, kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);

    reply.start();

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(reply.error(), TileReply::Error::NoTileError);
    QVERIFY(reply.data().isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

void TileReplyTest::_secondStartDoesNothing_data()
{
    QTest::addColumn<int>("mapId");

    QTest::newRow("request that is made") << tileReplyMapId();
    QTest::newRow("request that cannot be made") << -1;
}

void TileReplyTest::_secondStartDoesNothing()
{
    QFETCH(int, mapId);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    TileReply reply(mapId, kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);

    reply.start();
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    reply.start();

    // What a further start had begun would be answered before this lookup is
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(finishedSpy.count(), 1);
}

void TileReplyTest::_validatorRejectsCachedAndServerTiles()
{
    const QByteArray png = tileReplyPngBody();
    const auto acceptPng = [png](const QByteArray& data) { return data == png; };

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = png;

    // The tile UnitTestTileGenerator serves as the cached one is not the accepted tile: the server is asked
    TileReply refetched(tileReplyMapId(), kTileX + 20, kTileY, kTileZoom);
    refetched.setValidator(acceptPng);
    QSignalSpy refetchedSpy(&refetched, &TileReply::finished);
    refetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(refetched.error(), TileReply::Error::NoError);
    QVERIFY(!refetched.isCached());
    QCOMPARE(refetched.data(), png);
    QCOMPARE(nam.requestCount, 1);

    // A rejected server tile fails and is not stored: a request accepting any tile still has to ask the server
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);
    nam.body = tileReplyPngBody(Qt::red);
    QVERIFY(!nam.body.isEmpty() && (nam.body != png));
    TileReply rejected(tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    rejected.setValidator(acceptPng);
    QSignalSpy rejectedSpy(&rejected, &TileReply::finished);
    rejected.start();
    QVERIFY_SIGNAL_COUNT_WAIT(rejectedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(rejected.error(), TileReply::Error::DataError);
    QCOMPARE(nam.requestCount, 2);

    TileReply unvalidated(tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    QSignalSpy unvalidatedSpy(&unvalidated, &TileReply::finished);
    unvalidated.start();
    QVERIFY_SIGNAL_COUNT_WAIT(unvalidatedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unvalidated.error(), TileReply::Error::NoError);
    QVERIFY(!unvalidated.isCached());
    QCOMPARE(nam.requestCount, 3);
}

void TileReplyTest::_unusableCachedTileIsReplaced_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("unusable");
    QTest::addColumn<bool>("rejectedByValidator");

    const QString mapType = MapProviderRegistry::getProviderTypeFromMapId(tileReplyMapId());
    QTest::newRow("tile that the validator rejects") << mapType << (kTileX + 40) << tileReplyPngBody(Qt::red) << true;
    // The picture that Bing sends for a place without imagery, which the cache of an older version can hold
    QTest::newRow("no-tile picture of the provider")
        << QStringLiteral("Bing Hybrid") << (kTileX + 41) << tileReplyBingPlaceholder() << false;
    QTest::newRow("page in place of a picture") << mapType << (kTileX + 42) << kTileReplyPageBody << false;
}

// A tile in the cache that cannot be used, as a notice of refusal that was cached as a tile is, gives way to the
// tile that is fetched in its place: it is not fetched again and again
void TileReplyTest::_unusableCachedTileIsReplaced()
{
    QFETCH(QString, type);
    QFETCH(int, tileX);
    QFETCH(QByteArray, unusable);
    QFETCH(bool, rejectedByValidator);

    const QByteArray usable = tileReplyPngBody();
    QVERIFY(!unusable.isEmpty() && (unusable != usable));
    TileReply::Validator rejectUnusable;
    if (rejectedByValidator) {
        rejectUnusable = [unusable](const QByteArray& data) { return data != unusable; };
    }

    const int mapId = MapProviderRegistry::getMapIdFromProviderType(type);
    TileCache* const cache = TileCache::instance();
    cache->ensureInitialized();
    cache->cacheTile(mapId,
                     MapProviderRegistry::getMapProviderFromProviderType(type)->getTileHash(tileX, kTileY, kTileZoom),
                     unusable, QStringLiteral("png"));

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = usable;

    TileReply refetched(mapId, tileX, kTileY, kTileZoom);
    refetched.setValidator(rejectUnusable);
    QSignalSpy refetchedSpy(&refetched, &TileReply::finished);
    refetched.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(refetched.error(), TileReply::Error::NoError);
    QVERIFY(!refetched.isCached());
    QCOMPARE(refetched.data(), usable);
    QCOMPARE(nam.requestCount, 1);

    TileReply cached(mapId, tileX, kTileY, kTileZoom);
    cached.setValidator(rejectUnusable);
    QSignalSpy cachedSpy(&cached, &TileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), usable);
    QCOMPARE(nam.requestCount, 1);
}

void TileReplyTest::_abortedReplyStaysSilent()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReply reply(tileReplyMapId(), kTileX + 2, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
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

void TileReplyTest::_abortedBeforeQueuedStepStaysSilent_data()
{
    QTest::addColumn<int>("mapId");
    QTest::addColumn<bool>("forTileSet");

    QTest::newRow("request that cannot be made") << -1 << false;
    QTest::newRow("tile set request") << tileReplyMapId() << true;
}

// start() leaves to the event loop what would finish the request from within it
void TileReplyTest::_abortedBeforeQueuedStepStaysSilent()
{
    QFETCH(int, mapId);
    QFETCH(bool, forTileSet);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    TileReply reply(mapId, kTileX + 7, kTileY, kTileZoom);
    if (forTileSet) {
        reply.setTileSet(1);
    }
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    reply.start();
    reply.abort();

    // The step that start() queued is delivered before this lookup is answered
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());
    QCOMPARE(nam.requestCount, 0);
}

void TileReplyTest::_deletedDuringCacheLookup()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    {
        TileReply deleted(tileReplyMapId(), kTileX + 3, kTileY, kTileZoom);
        deleted.start();
    }

    // The cache worker runs lookups in order: once this one is answered, the deleted reply's result has arrived too
    TileReply sentinel(tileReplyMapId(), kTileX + 4, kTileY, kTileZoom);
    QSignalSpy sentinelSpy(&sentinel, &TileReply::finished);
    sentinel.start();
    QVERIFY_SIGNAL_COUNT_WAIT(sentinelSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sentinel.error(), TileReply::Error::NoError);
}

// The network manager of the map engine goes as the application exits, and deletes the network replies it owns: a
// request that is fetching must not touch its reply
void TileReplyTest::_networkManagerDestroyedFirst()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    auto nam = std::make_unique<TileReplyMockNam>();
    nam->holdReplies = true;
    const TileReplyNetworkScope networkScope(nam.get());
    auto fetching = std::make_unique<TileReply>(tileReplyMapId(), kTileX + 6, kTileY, kTileZoom);
    fetching->start();
    QTRY_COMPARE_WITH_TIMEOUT(nam->requestCount, 1, TestTimeout::mediumMs());
    nam.reset();
    fetching.reset();
}

// Two maps that show the same place ask for the same tiles at the same moment: the server is asked once
void TileReplyTest::_concurrentRequestsShareOneFetch()
{
    const QByteArray png = tileReplyPngBody();

    TileReplyMockNam nam;
    nam.body = png;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(4);

    const int mapId = tileReplyMapId();
    const int tileX = kTileX + 50;
    TileReply fetching(mapId, tileX, kTileY, kTileZoom);
    TileReply waiting(mapId, tileX, kTileY, kTileZoom);
    TileReply rejecting(mapId, tileX, kTileY, kTileZoom);
    rejecting.setValidator([](const QByteArray&) { return false; });
    TileReply withdrawn(mapId, tileX, kTileY, kTileZoom);
    QSignalSpy fetchingSpy(&fetching, &TileReply::finished);
    QSignalSpy waitingSpy(&waiting, &TileReply::finished);
    QSignalSpy rejectingSpy(&rejecting, &TileReply::finished);
    QSignalSpy withdrawnSpy(&withdrawn, &TileReply::finished);
    fetching.start();
    waiting.start();
    rejecting.start();
    withdrawn.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().originatingObject(), &fetching);

    // A request that stops waiting leaves the fetch to the others
    withdrawn.abort();
    nam.replies.at(0)->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(waitingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetchingSpy.count(), 1);
    for (const TileReply* const reply : {&fetching, &waiting}) {
        QCOMPARE(reply->error(), TileReply::Error::NoError);
        QVERIFY(!reply->isCached());
        QCOMPARE(reply->data(), png);
        QCOMPARE(reply->format(), QStringLiteral("png"));
    }
    // Every request checks the tile for itself
    QVERIFY_SIGNAL_COUNT_WAIT(rejectingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(rejecting.error(), TileReply::Error::DataError);
    QVERIFY(withdrawnSpy.isEmpty());
    QCOMPARE(nam.requestCount, 1);
}

void TileReplyTest::_waitingRequestFetchesAfterFetchWithoutTile_data()
{
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QString>("ending");

    QTest::newRow("aborted") << (kTileX + 51) << QStringLiteral("aborted");
    QTest::newRow("deleted") << (kTileX + 52) << QStringLiteral("deleted");
}

// A request must not wait for ever, or fail, because of what became of another consumer's request
void TileReplyTest::_waitingRequestFetchesAfterFetchWithoutTile()
{
    QFETCH(int, tileX);
    QFETCH(QString, ending);

    const QByteArray png = tileReplyPngBody();
    TileReplyMockNam nam;
    nam.body = png;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    auto fetching = std::make_unique<TileReply>(tileReplyMapId(), tileX, kTileY, kTileZoom);
    TileReply waiting(tileReplyMapId(), tileX, kTileY, kTileZoom);
    QSignalSpy waitingSpy(&waiting, &TileReply::finished);
    fetching->start();
    waiting.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().originatingObject(), fetching.get());

    // The request that waited asks the server for itself, which answers it at once
    nam.holdReplies = false;
    if (ending == QStringLiteral("aborted")) {
        fetching->abort();
    } else {
        fetching.reset();
    }

    QVERIFY_SIGNAL_COUNT_WAIT(waitingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(waiting.error(), TileReply::Error::NoError);
    QCOMPARE(waiting.data(), png);
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(nam.requests.constLast().originatingObject(), &waiting);
}

void TileReplyTest::_waitingRequestsGetErrorOfFetch_data()
{
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<TileReply::Error>("expectedError");

    QTest::newRow("server not reached") << (kTileX + 55) << QNetworkReply::TimeoutError << 0
                                        << TileReply::Error::NetworkError;
    QTest::newRow("provider has no tile")
        << (kTileX + 56) << QNetworkReply::ContentNotFoundError << 404 << TileReply::Error::NoTileError;
}

// The server would answer the waiting requests as it answered the fetch: they do not ask it in turn, each after the
// transfer timeout of the one before
void TileReplyTest::_waitingRequestsGetErrorOfFetch()
{
    QFETCH(int, tileX);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(TileReply::Error, expectedError);

    TileReplyMockNam nam;
    nam.error = networkError;
    nam.httpStatus = httpStatus;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);

    constexpr int kWaitingCount = 3;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1 + kWaitingCount);

    const int mapId = tileReplyMapId();
    TileReply fetching(mapId, tileX, kTileY, kTileZoom);
    fetching.start();
    QObject waitingOwner;
    QList<TileReply*> waiting;
    for (int i = 0; i < kWaitingCount; i++) {
        waiting.append(new TileReply(mapId, tileX, kTileY, kTileZoom, &waitingOwner));
        waiting.constLast()->start();
    }
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 1);

    nam.replies.at(0)->finishNow();
    QCOMPARE(fetching.error(), expectedError);
    for (const TileReply* const reply : std::as_const(waiting)) {
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), TestTimeout::mediumMs());
        QCOMPARE(reply->error(), expectedError);
        QCOMPARE(reply->networkFailure(), fetching.networkFailure());
        QCOMPARE(reply->errorString(), fetching.errorString());
        QVERIFY(reply->data().isEmpty());
    }
    QCOMPARE(nam.requestCount, 1);
}

// The request that fetches a tile for an offline set stores it in that set: the tile another request fetches is
// not in the set
void TileReplyTest::_tileSetRequestFetchesForItself()
{
    TileReplyMockNam nam;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    const int tileX = kTileX + 54;
    TileReply forMap(tileReplyMapId(), tileX, kTileY, kTileZoom);
    forMap.start();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    TileReply forSet(tileReplyMapId(), tileX, kTileY, kTileZoom);
    forSet.setTileSet(1);
    forSet.start();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.constLast().originatingObject(), &forSet);
}

void TileReplyTest::_tileOfOtherSourceNotServed()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    const int mapId = MapProviderRegistry::getMapIdFromProviderType(kTileReplyCustomUrlType);

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
    const TileReplyNetworkScope networkScope(&nam);
    int serverRequests = 0;
    for (const Request& request : requests) {
        saved.setFactValue(customUrl, request.source);
        nam.body = request.tile;
        if (!request.cached) {
            serverRequests++;
        }

        TileReply reply(mapId, kTileX + 30, kTileY, kTileZoom);
        QSignalSpy finishedSpy(&reply, &TileReply::finished);
        reply.start();
        QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
        QCOMPARE(reply.error(), TileReply::Error::NoError);
        QCOMPARE(reply.isCached(), request.cached);
        QCOMPARE(reply.data(), request.tile);
        QCOMPARE(nam.requestCount, serverRequests);
    }
}

void TileReplyTest::_sourceChangedDuringRequest()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    const int mapId = MapProviderRegistry::getMapIdFromProviderType(kTileReplyCustomUrlType);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();

    // The source changes while the cache is asked for the tile of source A: the tile is fetched from source B
    saved.setFactValue(customUrl, kTileReplySourceA);
    TileReply fetched(mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &TileReply::finished);
    fetched.start();
    customUrl->setRawValue(kTileReplySourceB);
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(nam.requests.constFirst().url().host(), QUrl(kTileReplySourceB).host());

    // It is stored as a tile of source B
    TileReply cached(mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &TileReply::finished);
    cached.start();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
    QCOMPARE(nam.requestCount, 1);
}

void TileReplyTest::_requestIdentity_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<bool>("namesApplication");

    // OpenStreetMap answers with a picture of a refusal when a request poses as a browser's or as one of its own
    // web site: its tile usage policy asks for the application's name, and for HTTPS
    QTest::newRow("OpenStreetMap") << QStringLiteral("Street Map") << (kTileX + 30) << true;
    QTest::newRow("Bing") << QStringLiteral("Bing Road") << (kTileX + 31) << false;
}

void TileReplyTest::_requestIdentity()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(bool, namesApplication);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReply reply(MapProviderRegistry::getMapIdFromProviderType(providerType), tileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.count(), 1);

    const QNetworkRequest request = nam.requests.constFirst();
    const QString userAgent = request.header(QNetworkRequest::UserAgentHeader).toString();
    QCOMPARE(userAgent == QGCNetworkHelper::defaultUserAgent(), namesApplication);
    if (!namesApplication) {
        // A browser's, with one version in both its places
        static const QRegularExpression browser(
            QStringLiteral("^Mozilla/5\\.0 \\([^)]+; rv:([0-9.]+)\\) Gecko/[0-9.]+ Firefox/\\1$"));
        QVERIFY2(browser.match(userAgent).hasMatch(), qPrintable(userAgent));
    }
    QCOMPARE(request.hasRawHeader("Referer"), !namesApplication);
    if (namesApplication) {
        QCOMPARE(request.url().scheme(), QStringLiteral("https"));
    }
}

// Esri takes the token of the user in a header of the request
void TileReplyTest::_esriTokenSentInHeader()
{
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->appSettings()->esriToken(), QStringLiteral("esri-token"));

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReply reply(MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Esri World Street")), kTileX + 33,
                    kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.count(), 1);

    const QNetworkRequest request = nam.requests.constFirst();
    QCOMPARE(request.rawHeader("User-Token"), QByteArrayLiteral("esri-token"));
    QVERIFY(!request.url().toString().contains(QStringLiteral("esri-token")));
}

// The answer of an elevation server is stored in the form that the terrain reads. An answer that has no such form
// is no tile.
void TileReplyTest::_elevationAnswerThatIsNoTerrainTileFails()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = QByteArrayLiteral(R"({"status":"error","data":{}})");
    nam.httpStatus = 200;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    const int mapId =
        MapProviderRegistry::getMapIdFromProviderType(QString::fromLatin1(CopernicusElevationProvider::kProviderKey));
    TileReply reply(mapId, 18854, 13739, 1);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    expectLogMessage("MapTiles.CopernicusTileFormat", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid terrain tile")));
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(reply.error(), TileReply::Error::DataError);
    QVERIFY(reply.data().isEmpty());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(tileReplyCacheAnswer(reply.hash()), TileReplyCacheAnswer::Missing);
}

// A cache without a database takes no lookup. The server needs no cache: the tile is fetched all the same.
void TileReplyTest::_tileFetchedWhenCacheRefusesLookup()
{
    TileCache* const cache = TileCache::instance();
    const auto restoreCache = qScopeGuard([cache] {
        cache->shutdown();
        UnitTestTileGenerator::initTileCache();
    });

    // The cache worker ends when it cannot open its database, and refuses every task from then on
    cache->shutdown();
    expectLogMessage("MapTiles.TileCacheDatabase", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Could not find suitable cache directory")));
    expectLogMessage("MapTiles.CacheWorker", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed To Init Database")));
    expectLogMessage("MapTiles.TileCache", QtWarningMsg,
                     QRegularExpression(QStringLiteral("The tile cache could not be opened")));
    expectAppMessage(QRegularExpression(QStringLiteral("The map tile cache could not be opened")));
    cache->init(QString());
    // The worker owns a task it takes, and deletes one it refuses
    const auto lookupRefused = [cache] {
        return !cache->addTask(CacheTasks::fetchTile(QStringLiteral("probe"), cache, [](auto) {}));
    };
    QVERIFY(QTest::qWaitFor(lookupRefused, TestTimeout::mediumMs()));
    // The cache tells of the failure with the end of its first task, which is queued once the worker has ended
    QCoreApplication::processEvents();
    for (int i = 0; i < 4; i++) {
        verifyExpectedLogMessage();
    }

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();
    TileReply reply(tileReplyMapId(), kTileX + 60, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    reply.start();
    QVERIFY(!reply.isFinished());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(reply.error(), TileReply::Error::NoError);
    QVERIFY(!reply.isCached());
    QCOMPARE(reply.data(), nam.body);
    QCOMPARE(nam.requestCount, 1);
}

// A server that answered 429 is not asked by any request for a tile of its provider until its pause ends: not by the
// maps, and not by an offline tile set, the terrain or the 3D view either, which ask the same server
void TileReplyTest::_rateLimitHoldsEveryRequestOfProvider()
{
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::UnknownContentError;
    nam.httpStatus = 429;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReply refused(tileReplyMapId(), kTileX + 80, kTileY, kTileZoom);
    QSignalSpy refusedSpy(&refused, &TileReply::finished);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, QRegularExpression(QStringLiteral("asked too often")));
    refused.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refusedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(refused.error(), TileReply::Error::NetworkError);
    QCOMPARE(refused.networkFailure(), TileReply::NetworkFailure::RateLimited);
    QVERIFY(!refused.errorString().isEmpty());
    QVERIFY(refused.data().isEmpty());

    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    TileReply forSet(tileReplyMapId(), kTileX + 81, kTileY, kTileZoom);
    forSet.setTileSet(1);
    TileReply other(tileReplyMapId(), kTileX + 82, kTileY, kTileZoom);
    QSignalSpy forSetSpy(&forSet, &TileReply::finished);
    QSignalSpy otherSpy(&other, &TileReply::finished);
    // The request for the set asks no cache: it is held first
    forSet.start();
    other.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause - std::chrono::milliseconds(1)));
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(forSetSpy.isEmpty());
    QVERIFY(otherSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(forSetSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(forSet.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 2);
    QVERIFY(otherSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QVERIFY_SIGNAL_COUNT_WAIT(otherSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(other.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 3);
}

// The maps of a service are served by the same servers: after a 429 for one of them none of them asks, and the map
// of another service does
void TileReplyTest::_rateLimitHoldsEveryMapOfService()
{
    const int roadMapId = MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Bing Road"));
    const int satelliteMapId = MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Bing Satellite"));
    const int otherServiceMapId = MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Street Map"));
    QCOMPARE_GT(roadMapId, 0);
    QCOMPARE_GT(satelliteMapId, 0);
    QCOMPARE_GT(otherServiceMapId, 0);

    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::UnknownContentError;
    nam.httpStatus = 429;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReply refused(roadMapId, kTileX + 100, kTileY, kTileZoom);
    QSignalSpy refusedSpy(&refused, &TileReply::finished);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Bing.*asked too often")));
    refused.start();
    QVERIFY_SIGNAL_COUNT_WAIT(refusedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(refused.networkFailure(), TileReply::NetworkFailure::RateLimited);

    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    TileReply sameService(satelliteMapId, kTileX + 101, kTileY, kTileZoom);
    TileReply otherService(otherServiceMapId, kTileX + 102, kTileY, kTileZoom);
    QSignalSpy sameServiceSpy(&sameService, &TileReply::finished);
    QSignalSpy otherServiceSpy(&otherService, &TileReply::finished);
    sameService.start();
    otherService.start();
    QVERIFY_SIGNAL_COUNT_WAIT(otherServiceSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(otherService.error(), TileReply::Error::NoError);
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(sameServiceSpy.isEmpty());
    QCOMPARE(nam.requestCount, 2);

    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QVERIFY_SIGNAL_COUNT_WAIT(sameServiceSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sameService.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 3);
}

void TileReplyTest::_answerDuringPauseLengthensIt_data()
{
    QTest::addColumn<int>("tileX");  // tiles of its own for every row
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QByteArray>("retryAfter");
    QTest::addColumn<int>("pauseSeconds");

    QTest::newRow("429 that asks for longer")
        << (kTileX + 110) << QNetworkReply::UnknownContentError << 429 << QByteArrayLiteral("120") << 120;
    QTest::newRow("503 that asks for longer")
        << (kTileX + 113) << QNetworkReply::ServiceUnavailableError << 503 << QByteArrayLiteral("120") << 120;
    // The server is not asked sooner than its earlier answer asked
    QTest::newRow("429 that asks for less")
        << (kTileX + 116) << QNetworkReply::UnknownContentError << 429 << QByteArrayLiteral("5")
        << static_cast<int>(TileServerPauses::kDefaultPause.count());
}

// A map asks for the tiles of its view at once: the answers to the requests that were in flight when the server
// refused the first one arrive during the pause
void TileReplyTest::_answerDuringPauseLengthensIt()
{
    QFETCH(int, tileX);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QByteArray, retryAfter);
    QFETCH(int, pauseSeconds);

    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::UnknownContentError;
    nam.httpStatus = 429;
    nam.holdReplies = true;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReply refused(tileReplyMapId(), tileX, kTileY, kTileZoom);
    TileReply inFlight(tileReplyMapId(), tileX + 1, kTileY, kTileZoom);
    refused.start();
    inFlight.start();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    // One line for the pause, and none for the answer that changes its length
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, QRegularExpression(QStringLiteral("asked too often")));
    nam.replies.at(0)->finishNow();
    verifyExpectedLogMessage();
    nam.replies.at(1)->setHeader(QHttpHeaders::WellKnownHeader::RetryAfter, retryAfter);
    nam.replies.at(1)->failNow(networkError, httpStatus);

    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    nam.holdReplies = false;
    TileReply held(tileReplyMapId(), tileX + 2, kTileY, kTileZoom);
    QSignalSpy heldSpy(&held, &TileReply::finished);
    held.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(pauseSeconds) - std::chrono::milliseconds(1)));
    QCOMPARE(nam.requestCount, 2);
    QVERIFY(heldSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(heldSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(held.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 3);
}

void TileReplyTest::_unscheduledWaitReleasesHeldRequests_data()
{
    QTest::addColumn<bool>("refuseTurn");  // false: the lengthening of the pause is refused

    QTest::newRow("pause lengthened") << false;
    QTest::newRow("turn after the pause") << true;
}

// A wait for a server that cannot be scheduled would hold its requests for ever: they ask at once, as without a pause
void TileReplyTest::_unscheduledWaitReleasesHeldRequests()
{
    QFETCH(bool, refuseTurn);

    ManualScheduler manualScheduler;
    TileReplyRefusingScheduler scheduler(&manualScheduler);
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::UnknownContentError;
    nam.httpStatus = 429;
    nam.holdReplies = true;

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReply rateLimited(tileReplyMapId(), kTileX + 90, kTileY, kTileZoom);
    TileReply inFlight(tileReplyMapId(), kTileX + 91, kTileY, kTileZoom);
    rateLimited.start();
    inFlight.start();
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, QRegularExpression(QStringLiteral("asked too often")));
    nam.replies.at(0)->finishNow();
    verifyExpectedLogMessage();

    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    nam.holdReplies = false;
    // Tiles of its own for each row: the cache has those that a row fetched
    const int tileX = kTileX + (refuseTurn ? 94 : 92);
    TileReply first(tileReplyMapId(), tileX, kTileY, kTileZoom);
    TileReply second(tileReplyMapId(), tileX + 1, kTileY, kTileZoom);
    QSignalSpy firstSpy(&first, &TileReply::finished);
    QSignalSpy secondSpy(&second, &TileReply::finished);
    first.start();
    second.start();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(firstSpy.count(), 0);
    QCOMPARE(secondSpy.count(), 0);

    scheduler.refuseTasks = true;
    // At the turn after the pause, the first request asks before the turn of the second one is refused
    expectLogMessage(
        "MapTiles.TileServerPauses", QtWarningMsg,
        QRegularExpression(QStringLiteral("could not be scheduled: the %1 requests").arg(refuseTurn ? 1 : 2)));
    if (refuseTurn) {
        QVERIFY(manualScheduler.advanceBy(TileServerPauses::kDefaultPause));
    } else {
        nam.replies.at(1)->setHeader(QHttpHeaders::WellKnownHeader::RetryAfter, "120");
        nam.replies.at(1)->finishNow();
    }
    verifyExpectedLogMessage();

    QVERIFY_SIGNAL_COUNT_WAIT(firstSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(secondSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(first.error(), TileReply::Error::NoError);
    QCOMPARE(second.error(), TileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 4);
}

void TileReplyTest::_retryAfterRead_data()
{
    QTest::addColumn<QByteArray>("value");     // empty: no header
    QTest::addColumn<int>("expectedSeconds");  // -1: none

    const QDateTime now = tileReplyRetryAfterNow();
    QTest::newRow("delay in seconds") << QByteArrayLiteral("120") << 120;
    QTest::newRow("HTTP date") << tileReplyHttpDate(now.addSecs(90)) << 90;
    QTest::newRow("HTTP date that is past") << tileReplyHttpDate(now.addSecs(-60)) << 0;
    QTest::newRow("negative delay") << QByteArrayLiteral("-5") << -1;
    QTest::newRow("neither") << QByteArrayLiteral("soon") << -1;
    QTest::newRow("no header") << QByteArray() << -1;
}

void TileReplyTest::_retryAfterRead()
{
    QFETCH(QByteArray, value);
    QFETCH(int, expectedSeconds);

    QHttpHeaders headers;
    if (!value.isEmpty()) {
        QVERIFY(headers.append(QHttpHeaders::WellKnownHeader::RetryAfter, value));
    }
    const std::optional<std::chrono::seconds> retryAfter =
        TileServerPauses::retryAfter(headers, tileReplyRetryAfterNow());
    if (expectedSeconds < 0) {
        QVERIFY(!retryAfter.has_value());
    } else {
        QVERIFY(retryAfter.has_value());
        QCOMPARE(retryAfter->count(), expectedSeconds);
    }
}

UT_REGISTER_TEST(TileReplyTest, TestLabel::Unit)
