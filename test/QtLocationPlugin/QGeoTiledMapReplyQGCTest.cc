#include "QGeoTiledMapReplyQGCTest.h"

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <QtCore/QSharedPointer>

#include "QGCCacheTile.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTileFetcherQGC.h"
#include "TileFetchMetrics.h"

void QGeoTiledMapReplyQGCTest::_testCacheReplyMarksCachedAndFinished()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 12, 34, 5);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/tile.png"))), spec);

    auto tile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("test_hash"), QByteArrayLiteral("tile_bytes"), QStringLiteral("png"),
                                  UrlFactory::getProviderTypeFromQtMapId(mapId));
    // Fresh so the tile is served directly regardless of network availability;
    // a default (expired, no-validator) tile would trigger a refetch when online.
    tile->expiresAt = 4102444800;  // 2100-01-01

    const bool invoked =
        QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, tile));
    QVERIFY(invoked);

    QVERIFY(reply.isFinished());
    QVERIFY(reply.isCached());
    QCOMPARE(reply.mapImageData(), QByteArrayLiteral("tile_bytes"));
    QCOMPARE(reply.mapImageFormat(), QStringLiteral("png"));
}

void QGeoTiledMapReplyQGCTest::_testCacheReplyNullTileSetsError()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 12, 34, 5);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/tile.png"))), spec);

    const bool invoked =
        QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, QSharedPointer<QGCCacheTile>()));
    QVERIFY(invoked);

    QCOMPARE(reply.error(), QGeoTiledMapReply::UnknownError);
}

void QGeoTiledMapReplyQGCTest::_testNonExpiredCacheServedWithoutNetworkManager()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 12, 34, 5);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/tile.png"))), spec);

    auto cachedTile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("cached_hash"), QByteArrayLiteral("cached_bytes"),
                                        QStringLiteral("png"), UrlFactory::getProviderTypeFromQtMapId(mapId));
    cachedTile->expiresAt = 4102444800;  // 2100-01-01: fresh, so served without a network manager
    QVERIFY(QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, cachedTile)));

    QVERIFY(reply.isFinished());
    QVERIFY(reply.isCached());
    QCOMPARE(reply.mapImageData(), QByteArrayLiteral("cached_bytes"));
}

void QGeoTiledMapReplyQGCTest::_testHandleNetworkResult304WithoutCacheSetsError()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 5, 6, 4);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/t.png"))), spec);

    QGeoTileFetcherQGC::FetchResult result;
    result.statusCode = 304;
    result.isOpen = true;

    QVERIFY(QMetaObject::invokeMethod(&reply, "_handleNetworkResult", Qt::DirectConnection,
                                      Q_ARG(QGeoTileFetcherQGC::FetchResult, result)));

    QCOMPARE(reply.error(), QGeoTiledMapReply::UnknownError);
}

void QGeoTiledMapReplyQGCTest::_testHandleNetworkResultSuccessCachesAndFinishes()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 5, 6, 4);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/t.png"))), spec);

    const TileFetchStats before = TileFetchMetrics::instance().snapshot();

    QGeoTileFetcherQGC::FetchResult result;
    result.statusCode = 200;
    result.isOpen = true;
    result.body = QByteArray::fromHex("89504e470d0a1a0a") + QByteArrayLiteral("fakecontent");

    QVERIFY(QMetaObject::invokeMethod(&reply, "_handleNetworkResult", Qt::DirectConnection,
                                      Q_ARG(QGeoTileFetcherQGC::FetchResult, result)));

    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), QGeoTiledMapReply::NoError);

    const TileFetchStats after = TileFetchMetrics::instance().snapshot();
    QCOMPARE(after.networkSuccess, before.networkSuccess + 1);
    QVERIFY(after.bytesDownloaded >= before.bytesDownloaded + static_cast<quint64>(result.body.size()));
}

void QGeoTiledMapReplyQGCTest::_testHandleNetworkResultHttpErrorSetsError()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 5, 6, 4);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/t.png"))), spec);

    QGeoTileFetcherQGC::FetchResult result;
    result.statusCode = 404;
    result.isOpen = true;
    result.reasonPhrase = QStringLiteral("Not Found");

    QVERIFY(QMetaObject::invokeMethod(&reply, "_handleNetworkResult", Qt::DirectConnection,
                                      Q_ARG(QGeoTileFetcherQGC::FetchResult, result)));

    QCOMPARE(reply.error(), QGeoTiledMapReply::CommunicationError);
}

void QGeoTiledMapReplyQGCTest::_testFreshCacheServedWithoutRevalidation()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 5, 6, 4);
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/t.png"))), spec);

    const TileFetchStats before = TileFetchMetrics::instance().snapshot();

    // expiresAt far in the future => fresh, served directly even though validators exist.
    auto tile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("fresh_hash"), QByteArrayLiteral("fresh_bytes"), QStringLiteral("png"),
                                  UrlFactory::getProviderTypeFromQtMapId(mapId));
    tile->etag = QByteArrayLiteral("\"etag\"");
    tile->expiresAt = 4102444800;  // 2100-01-01

    QVERIFY(QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, tile)));

    QVERIFY(reply.isFinished());
    QVERIFY(reply.isCached());
    QCOMPARE(reply.mapImageData(), QByteArrayLiteral("fresh_bytes"));

    const TileFetchStats after = TileFetchMetrics::instance().snapshot();
    QCOMPARE(after.cacheHits, before.cacheHits + 1);
}

void QGeoTiledMapReplyQGCTest::_testMustRevalidateCacheTileNotServedDirectly()
{
    if (!QGCNetworkHelper::isInternetAvailable()) {
        QSKIP("Revalidation path requires network availability");
    }

    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 7, 8, 4);
    QNetworkAccessManager nam;
    QGeoTiledMapReplyQGC reply(&nam, QNetworkRequest(QUrl(QStringLiteral("http://127.0.0.1:9/t.png"))), spec);

    const TileFetchStats before = TileFetchMetrics::instance().snapshot();

    // Fresh expiry but must-revalidate set => treated as needing revalidation,
    // so it must NOT be served as a direct cache hit.
    auto tile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("mr_hash"), QByteArrayLiteral("mr_bytes"), QStringLiteral("png"),
                                  UrlFactory::getProviderTypeFromQtMapId(mapId));
    tile->etag = QByteArrayLiteral("\"etag\"");
    tile->expiresAt = 4102444800;  // 2100-01-01
    tile->mustRevalidate = true;

    QVERIFY(QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, tile)));

    const TileFetchStats after = TileFetchMetrics::instance().snapshot();
    QCOMPARE(after.cacheHits, before.cacheHits);
    QVERIFY(!reply.isCached());
}

void QGeoTiledMapReplyQGCTest::_testExpiredRevalidatableTileServedStale()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QGeoTileSpec spec = tileSpec(mapId, 9, 10, 4);
    // No fetcher and no QNAM => the background revalidator no-ops, isolating the
    // stale-serve decision. EXPIRED + validators + NOT must-revalidate must still be
    // served from cache (stale-while-revalidate offline, or direct serve when no net).
    QGeoTiledMapReplyQGC reply(nullptr, QNetworkRequest(QUrl(QStringLiteral("https://example.com/t.png"))), spec);

    const TileFetchStats before = TileFetchMetrics::instance().snapshot();

    auto tile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("swr_hash"), QByteArrayLiteral("swr_bytes"), QStringLiteral("png"),
                                  UrlFactory::getProviderTypeFromQtMapId(mapId));
    tile->etag = QByteArrayLiteral("\"etag\"");
    tile->expiresAt = 1000;        // 1970 => expired
    tile->mustRevalidate = false;  // plain expiry => stale-while-revalidate eligible

    QVERIFY(QMetaObject::invokeMethod(&reply, "_cacheReply", Qt::DirectConnection, Q_ARG(QSharedPointer<QGCCacheTile>, tile)));

    QVERIFY(reply.isFinished());
    QVERIFY(reply.isCached());
    QCOMPARE(reply.mapImageData(), QByteArrayLiteral("swr_bytes"));

    const TileFetchStats after = TileFetchMetrics::instance().snapshot();
    QCOMPARE(after.cacheHits, before.cacheHits + 1);
}

UT_REGISTER_TEST(QGeoTiledMapReplyQGCTest, TestLabel::Unit)
