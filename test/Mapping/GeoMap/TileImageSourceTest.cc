#include "TileImageSourceTest.h"

#include <QtCore/QScopeGuard>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "TileImageSource.h"
#include "TileMath.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

const TileMath::TileKey kKey{4302, 2867, 13};  // Zurich area

}  // namespace

void TileImageSourceTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void TileImageSourceTest::_tileDelivered()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(kKey);
    QCOMPARE_GT(requestId, 0);
    QCOMPARE(source.pendingCount(), 1);

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(source.pendingCount(), 0);

    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    const QImage image = readySpy.first().at(1).value<QImage>();
    QVERIFY(!image.isNull());
    QCOMPARE_GT(image.width(), 0);
}

void TileImageSourceTest::_cancelledRequestSilent()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int cancelledId = source.requestTileImage(kKey);
    source.cancelRequest(cancelledId);
    QCOMPARE(source.pendingCount(), 0);

    // The cache worker runs tasks in order: once this later request delivers,
    // the cancelled one has already run its course without signalling
    const int sentinelId = source.requestTileImage(TileMath::TileKey{kKey.x + 1, kKey.y, kKey.zoom});
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(readySpy.first().at(0).toInt(), sentinelId);
    QCOMPARE(failedSpy.count(), 0);
}

void TileImageSourceTest::_multipleRequestsIndependent()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);

    const int first = source.requestTileImage(kKey);
    const int second = source.requestTileImage(TileMath::TileKey{kKey.x + 1, kKey.y, kKey.zoom});
    QCOMPARE_NE(first, second);

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);

    QSet<int> deliveredIds;
    for (const QList<QVariant>& args : readySpy) {
        deliveredIds.insert(args.at(0).toInt());
    }
    QCOMPARE(deliveredIds, (QSet<int>{first, second}));
}

void TileImageSourceTest::_unknownProviderFails()
{
    // Both the provider lookup and the source itself deliberately warn once at construction
    expectLogMessage("MapTiles.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("type not found: \"No Such Provider\"")));
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("unknown map type")));
    TileImageSource source(QStringLiteral("No Such Provider"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(kKey);
    QCOMPARE_GT(requestId, 0);
    QCOMPARE(source.pendingCount(), 1);  // failure is still delivered asynchronously

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(source.pendingCount(), 0);
}

void TileImageSourceTest::_networkFallbackDeliversAndCaches()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 10, kKey.y, kKey.zoom};  // unique: write-back caches this tile

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    QVERIFY(!nam.body.isEmpty());
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    // Two forced generator misses: the first request must fall back to the
    // network; the second must be served by the write-back cached tile (the
    // DB hit precedes the generator, so the second miss stays unconsumed)
    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(!readySpy.first().at(1).value<QImage>().isNull());

    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);
    QCOMPARE(nam.requestCount, 1);  // cached by write-back: no second network fetch
    QVERIFY(failedSpy.isEmpty());
}

void TileImageSourceTest::_networkErrorFails()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 11, kKey.y, kKey.zoom};

    TileReplyMockNam nam;
    nam.error = QNetworkReply::ConnectionRefusedError;
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    // Fetch failures must be visible without logging configuration
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("network error")));
    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
}

void TileImageSourceTest::_networkEmptyBodyFails()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 12, kKey.y, kKey.zoom};

    TileReplyMockNam nam;  // NoError with an empty body: HTTP success carrying nothing
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("empty body")));
    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

void TileImageSourceTest::_networkGarbageBodyFailsAndNotCached()
{
    // An HTTP-200 non-image body (e.g. an error page) must fail the request
    // and must NOT be written back to the cache — a cached invalid tile would
    // fail every retry from then on
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 13, kKey.y, kKey.zoom};

    TileReplyMockNam nam;
    nam.body = QByteArrayLiteral("this is not an image");
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(2);

    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("network body is not")));
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(nam.requestCount, 1);

    // Garbage was not cached: the retry misses again and re-fetches, this
    // time getting a valid image
    nam.body = tileReplyPngBody();
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(nam.requestCount, 2);
}

void TileImageSourceTest::_missingTileReported_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<int>("tileX");

    // Bing serves a placeholder image instead of an HTTP error where it has no imagery: it decodes fine, but
    // delivering or caching it would drape it onto patches
    QTest::newRow("Bing placeholder") << tileReplyBingPlaceholder() << QNetworkReply::NoError << 0 << (kKey.x + 14);
    QTest::newRow("HTTP 404") << QByteArray() << QNetworkReply::ContentNotFoundError << 404 << (kKey.x + 17);
}

void TileImageSourceTest::_missingTileReported()
{
    QFETCH(QByteArray, body);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(int, tileX);
    QVERIFY2(!body.isEmpty() || (httpStatus > 0), "the placeholder image is not in the resources");

    TileReplyMockNam nam;
    nam.body = body;
    nam.error = networkError;
    nam.httpStatus = httpStatus;
    TileImageSource source(QStringLiteral("Bing Hybrid"), nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);
    QSignalSpy missingSpy(&source, &TileImageSource::tileImageMissing);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    // No tile is no failure: nothing is logged, which the log check of the test would report
    const int requestId = source.requestTileImage(TileMath::TileKey{tileX, kKey.y, kKey.zoom});
    QTRY_COMPARE_WITH_TIMEOUT(missingSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(missingSpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
    QVERIFY(failedSpy.isEmpty());
    QCOMPARE(source.pendingCount(), 0);
}

void TileImageSourceTest::_cachedUnusableTileFallsBackToNetwork()
{
    // Pre-seed the shared cache with an undecodable body: the cache-hit path
    // must treat it as a miss and recover via the network — failing would
    // wedge the tile on the same bytes every paced retry
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 15, kKey.y, kKey.zoom};
    // The cache worker runs tasks in order: the store lands before the lookup
    getQGCMapEngine()->cacheTile(type, UrlFactory::getTileHash(type, key.x, key.y, key.zoom),
                                 QByteArrayLiteral("garbage"), QStringLiteral("png"));

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    QVERIFY(!nam.body.isEmpty());
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);  // cache hit rejected, recovered from the network
    QVERIFY(failedSpy.isEmpty());

    // The fetched tile took the place of the bad entry: a repeat request is served from the cache
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(failedSpy.isEmpty());
}

void TileImageSourceTest::_cancelAbortsNetworkFetch()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 16, kKey.y, kKey.zoom};

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, 5000);

    source.cancelRequest(requestId);
    QCOMPARE(source.pendingCount(), 0);
    source.cancelRequest(requestId);  // double cancel is a no-op

    // The aborted reply's late finished emission must stay silent; the source handles it synchronously
    nam.replies.at(0)->finishNow();
    QVERIFY(readySpy.isEmpty());
    QVERIFY(failedSpy.isEmpty());
}

UT_REGISTER_TEST(TileImageSourceTest, TestLabel::Integration)
