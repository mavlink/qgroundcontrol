#include "QGeoTileFetcherQGCTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <memory>
#include <vector>

#include "Fixtures/RAIIFixtures.h"
#include "QGCHostCircuitBreaker.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTileFetcherQGC.h"

void QGeoTileFetcherQGCTest::init()
{
    // The host gate is a process-wide singleton; reset so leader GETs left
    // in-flight by a prior test (unreachable port, never completes) don't carry
    // held slots into the next one.
    HostConcurrencyGate::instance().resetForTest();
}

void QGeoTileFetcherQGCTest::_testConcurrentDownloadsConstant()
{
    QCOMPARE(QGeoTileFetcherQGC::concurrentDownloads(QStringLiteral("AnyProvider")), 6u);
}

void QGeoTileFetcherQGCTest::_testGetNetworkRequestInvalidMapId()
{
    const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(-1, 0, 0, 1);
    QVERIFY(request.url().isEmpty());
}

void QGeoTileFetcherQGCTest::_testGetNetworkRequestValidMapBasics()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, 10, 20, 5);
    QVERIFY(!request.url().isEmpty());
    QVERIFY(request.url().isValid());
    QCOMPARE(request.priority(), QNetworkRequest::NormalPriority);
    QCOMPARE(request.transferTimeout(), 0);  // owned by the fetcher leader-GET path

    QCOMPARE(request.rawHeader(QByteArrayLiteral("Accept")), QByteArrayLiteral("*/*"));
    QCOMPARE(request.rawHeader(QByteArrayLiteral("Connection")), QByteArrayLiteral("keep-alive"));
}

void QGeoTileFetcherQGCTest::_testGetNetworkRequestAttributes()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, 1, 2, 3);

    QCOMPARE(request.attribute(QNetworkRequest::BackgroundRequestAttribute).toBool(), true);
    QCOMPARE(request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),
             static_cast<int>(QNetworkRequest::NoLessSafeRedirectPolicy));
    QCOMPARE(request.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute).toBool(), false);
    // Trimmed: Http2Allowed (default true in Qt 6) and Cache{Load,Save}Control
    // (no QNetworkDiskCache attached, so they were no-ops).
    QVERIFY(!request.attribute(QNetworkRequest::CacheLoadControlAttribute).isValid());
    QVERIFY(!request.attribute(QNetworkRequest::CacheSaveControlAttribute).isValid());
    QVERIFY(!request.attribute(QNetworkRequest::Http2AllowedAttribute).isValid());
}

void QGeoTileFetcherQGCTest::_testDedupCoalescesSameUrl()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    QNetworkAccessManager nam;
    QGeoTileFetcherQGC fetcher(&nam, QVariantMap(), nullptr);

    const QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:9/same_tile.png")));
    QGeoTiledMapReplyQGC a(&nam, request, tileSpec(mapId, 1, 1, 2));
    QGeoTiledMapReplyQGC b(&nam, request, tileSpec(mapId, 1, 1, 2));
    QGeoTiledMapReplyQGC c(&nam, request, tileSpec(mapId, 1, 1, 2));

    fetcher.dedupFetch(&a, request);
    fetcher.dedupFetch(&b, request);
    fetcher.dedupFetch(&c, request);

    QCOMPARE(fetcher.inFlightUrlCount(), 1);
    QCOMPARE(fetcher.followerCount(request.url()), 3);

    fetcher.cancelFetch(&a, request);
    QCOMPARE(fetcher.followerCount(request.url()), 2);
}

void QGeoTileFetcherQGCTest::_testDedupSeparateUrlsDistinct()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    QNetworkAccessManager nam;
    QGeoTileFetcherQGC fetcher(&nam, QVariantMap(), nullptr);

    const QNetworkRequest r1(QUrl(QStringLiteral("http://127.0.0.1:9/a.png")));
    const QNetworkRequest r2(QUrl(QStringLiteral("http://127.0.0.1:9/b.png")));
    QGeoTiledMapReplyQGC a(&nam, r1, tileSpec(mapId, 1, 1, 2));
    QGeoTiledMapReplyQGC b(&nam, r2, tileSpec(mapId, 2, 2, 2));

    fetcher.dedupFetch(&a, r1);
    fetcher.dedupFetch(&b, r2);

    QCOMPARE(fetcher.inFlightUrlCount(), 2);
    QCOMPARE(fetcher.followerCount(r1.url()), 1);
    QCOMPARE(fetcher.followerCount(r2.url()), 1);
}

void QGeoTileFetcherQGCTest::_testParseExpirySMaxAgeFallback()
{
    TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/t.png")));
    reply.setHttpStatus(200);
    reply.setRawHeader(QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("public, s-maxage=3600"));

    const qint64 before = QDateTime::currentSecsSinceEpoch();
    const QGeoTileFetcherQGC::FetchResult result = QGeoTileFetcherQGC::snapshot(&reply);
    const qint64 after = QDateTime::currentSecsSinceEpoch();

    QVERIFY(result.expiresAt >= (before + 3600));
    QVERIFY(result.expiresAt <= (after + 3600));
}

void QGeoTileFetcherQGCTest::_testParseExpiryMaxAgeWinsOverSMaxAge()
{
    TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/t.png")));
    reply.setHttpStatus(200);
    reply.setRawHeader(QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("s-maxage=86400, max-age=60"));

    const qint64 before = QDateTime::currentSecsSinceEpoch();
    const QGeoTileFetcherQGC::FetchResult result = QGeoTileFetcherQGC::snapshot(&reply);
    const qint64 after = QDateTime::currentSecsSinceEpoch();

    QVERIFY(result.expiresAt >= (before + 60));
    QVERIFY(result.expiresAt <= (after + 60));
}

void QGeoTileFetcherQGCTest::_testParseExpirySMaxAgeZeroIsNotCached()
{
    TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/t.png")));
    reply.setHttpStatus(200);
    reply.setRawHeader(QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("s-maxage=0"));

    const QGeoTileFetcherQGC::FetchResult result = QGeoTileFetcherQGC::snapshot(&reply);
    QCOMPARE(result.expiresAt, static_cast<qint64>(0));
}

void QGeoTileFetcherQGCTest::_testConcurrencyGateCapsInFlight()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    QNetworkAccessManager nam;
    QGeoTileFetcherQGC fetcher(&nam, QVariantMap(), nullptr);
    fetcher.setConcurrencyBudgetForTest(2);

    constexpr int kRequests = 6;
    std::vector<std::unique_ptr<QGeoTiledMapReplyQGC>> replies;
    for (int i = 0; i < kRequests; ++i) {
        const QUrl url(QStringLiteral("http://127.0.0.1:9/tile_%1.png").arg(i));
        const QNetworkRequest request(url);
        auto reply = std::make_unique<QGeoTiledMapReplyQGC>(&nam, request, tileSpec(mapId, i, i, 2));
        fetcher.dedupFetch(reply.get(), request);
        replies.push_back(std::move(reply));
        QVERIFY(fetcher.activeNetworkFetches() <= 2);
    }

    QCOMPARE(fetcher.inFlightUrlCount(), kRequests);
    QCOMPARE(fetcher.activeNetworkFetches(), 2);
    QCOMPARE(fetcher.peakConcurrentFetches(), 2);
}

void QGeoTileFetcherQGCTest::_testHostConcurrencyGateSharedCap()
{
    HostConcurrencyGate& gate = HostConcurrencyGate::instance();
    const QString host = QStringLiteral("tile.example.org");

    // OSM hosts are clamped to kOSMHostLimit (2) regardless of a larger budget,
    // so live + bulk paths combined can never exceed the policy cap.
    QVERIFY(gate.tryAcquire(host, 6, true));
    QVERIFY(gate.tryAcquire(host, 6, true));
    QVERIFY(!gate.tryAcquire(host, 6, true));
    QCOMPARE(gate.activeForTest(host), 2);

    gate.release(host);
    QVERIFY(gate.tryAcquire(host, 6, true));
    QCOMPARE(gate.activeForTest(host), 2);

    gate.release(host);
    gate.release(host);
    QCOMPARE(gate.activeForTest(host), 0);

    // Empty host is never gated (unit-test URLs without a real host).
    QVERIFY(gate.tryAcquire(QString(), 1, true));
    QVERIFY(gate.tryAcquire(QString(), 1, true));
}

void QGeoTileFetcherQGCTest::_testSnapshotParsesMustRevalidate()
{
    const auto mustRevalidateFor = [](const QByteArray& cacheControl) {
        TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/t.png")));
        reply.setHttpStatus(200);
        if (!cacheControl.isEmpty()) {
            reply.setRawHeader(QByteArrayLiteral("Cache-Control"), cacheControl);
        }
        return QGeoTileFetcherQGC::snapshot(&reply).mustRevalidate;
    };

    QVERIFY(mustRevalidateFor(QByteArrayLiteral("no-cache")));
    QVERIFY(mustRevalidateFor(QByteArrayLiteral("must-revalidate")));
    QVERIFY(mustRevalidateFor(QByteArrayLiteral("no-store")));
    QVERIFY(mustRevalidateFor(QByteArrayLiteral("public, max-age=60, must-revalidate")));
    QVERIFY(!mustRevalidateFor(QByteArrayLiteral("public, max-age=3600")));
    QVERIFY(!mustRevalidateFor(QByteArray()));
}

UT_REGISTER_TEST(QGeoTileFetcherQGCTest, TestLabel::Unit)
