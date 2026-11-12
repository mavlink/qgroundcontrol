#include "QGCTileReplyTest.h"

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "MapProvider.h"
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
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            return UrlFactory::getMapIdFromProviderType(type);
        }
    }
    return -1;
}

constexpr int kTileX = 4302;
constexpr int kTileY = 2867;
constexpr int kTileZoom = 13;

/// The provider that fetches from the URL template the user sets
const QString kTileReplyCustomUrlType = QStringLiteral("CustomURL Custom");
const QString kTileReplySourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
const QString kTileReplySourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

}  // namespace

void QGCTileReplyTest::_cachedTileServedWithoutNetwork()
{
    TileReplyMockNam nam;
    QGCTileReply reply(&nam, tileReplyMapId(), kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);

    QVERIFY(reply.start());
    QVERIFY(!reply.isFinished());  // never finishes from within start()
    QVERIFY(!reply.start());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
    QVERIFY(reply.isCached());
    QVERIFY(!reply.data().isEmpty());
    QCOMPARE(reply.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 0);
}

void QGCTileReplyTest::_serverTileStoredInCache()
{
    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    QVERIFY(!nam.body.isEmpty());

    // The second forced miss stays unconsumed when the stored tile answers the second lookup
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply fetched(&nam, tileReplyMapId(), kTileX + 1, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &QGCTileReply::finished);
    QVERIFY(fetched.start());
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!fetched.isCached());
    QCOMPARE(fetched.data(), nam.body);
    QCOMPARE(fetched.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 1);

    QGCTileReply cached(&nam, tileReplyMapId(), kTileX + 1, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    QVERIFY(cached.start());
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
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
    QTest::newRow("network error") << mapType << (kTileX + 10) << QByteArray() << QNetworkReply::ContentNotFoundError
                                   << 404 << QGCTileReply::Error::NetworkError;
    QTest::newRow("http error status") << mapType << (kTileX + 11) << tileReplyPngBody() << QNetworkReply::NoError
                                       << 500 << QGCTileReply::Error::NetworkError;
    QTest::newRow("empty body") << mapType << (kTileX + 12) << QByteArray() << QNetworkReply::NoError << 200
                                << QGCTileReply::Error::DataError;
    QTest::newRow("Bing placeholder") << QStringLiteral("Bing Hybrid") << (kTileX + 13) << tileReplyBingPlaceholder()
                                      << QNetworkReply::NoError << 200 << QGCTileReply::Error::NoTileError;
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

    TileReplyMockNam nam;
    nam.body = body;
    nam.error = networkError;
    nam.httpStatus = httpStatus;

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply failed(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy failedSpy(&failed, &QGCTileReply::finished);
    QVERIFY(failed.start());
    QVERIFY_SIGNAL_COUNT_WAIT(failedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(failed.error(), expectedError);
    QVERIFY(!failed.errorString().isEmpty());
    QVERIFY(failed.data().isEmpty());
    QCOMPARE(nam.requestCount, 1);

    // Nothing was stored: the next request misses the cache again and asks the server
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    QGCTileReply retried(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy retriedSpy(&retried, &QGCTileReply::finished);
    QVERIFY(retried.start());
    QVERIFY_SIGNAL_COUNT_WAIT(retriedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(retried.error(), QGCTileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 2);
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
    QVERIFY(refetched.start());
    QVERIFY_SIGNAL_COUNT_WAIT(refetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(refetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!refetched.isCached());
    QCOMPARE(refetched.data(), png);
    QCOMPARE(nam.requestCount, 1);

    // A rejected server tile fails and is not stored: a request accepting any tile still has to ask the server
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });
    nam.body = QByteArrayLiteral("not the accepted tile");
    QGCTileReply rejected(&nam, tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    rejected.setValidator(acceptPng);
    QSignalSpy rejectedSpy(&rejected, &QGCTileReply::finished);
    QVERIFY(rejected.start());
    QVERIFY_SIGNAL_COUNT_WAIT(rejectedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(rejected.error(), QGCTileReply::Error::DataError);
    QCOMPARE(nam.requestCount, 2);

    QGCTileReply unvalidated(&nam, tileReplyMapId(), kTileX + 21, kTileY, kTileZoom);
    QSignalSpy unvalidatedSpy(&unvalidated, &QGCTileReply::finished);
    QVERIFY(unvalidated.start());
    QVERIFY_SIGNAL_COUNT_WAIT(unvalidatedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unvalidated.error(), QGCTileReply::Error::NoError);
    QVERIFY(!unvalidated.isCached());
    QCOMPARE(nam.requestCount, 3);
}

void QGCTileReplyTest::_abortedReplyStaysSilent()
{
    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply reply(&nam, tileReplyMapId(), kTileX + 2, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    QVERIFY(reply.start());
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    const QPointer<TileReplyCannedReply> networkReply(nam.replies.at(0));
    reply.abort();
    networkReply->finishNow();  // a late answer of the aborted fetch
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());

    // The fetch is released without waiting for it to finish
    QTRY_VERIFY_WITH_TIMEOUT(networkReply.isNull(), TestTimeout::mediumMs());
}

void QGCTileReplyTest::_deletedDuringCacheLookup()
{
    TileReplyMockNam nam;
    {
        QGCTileReply deleted(&nam, tileReplyMapId(), kTileX + 3, kTileY, kTileZoom);
        QVERIFY(deleted.start());
    }

    // The cache worker runs lookups in order: once this one is answered, the deleted reply's result has arrived too
    QGCTileReply sentinel(&nam, tileReplyMapId(), kTileX + 4, kTileY, kTileZoom);
    QSignalSpy sentinelSpy(&sentinel, &QGCTileReply::finished);
    QVERIFY(sentinel.start());
    QVERIFY_SIGNAL_COUNT_WAIT(sentinelSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sentinel.error(), QGCTileReply::Error::NoError);
}

void QGCTileReplyTest::_networkManagerDestroyedFirst()
{
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Gone before the cache miss: there is nothing to ask the server with
    auto* nam = new TileReplyMockNam;
    QGCTileReply unfetched(nam, tileReplyMapId(), kTileX + 5, kTileY, kTileZoom);
    QSignalSpy unfetchedSpy(&unfetched, &QGCTileReply::finished);
    QVERIFY(unfetched.start());
    delete nam;
    QVERIFY_SIGNAL_COUNT_WAIT(unfetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unfetched.error(), QGCTileReply::Error::NetworkError);

    // Gone during the fetch: the manager deletes the network reply it owns, which the tile reply must not touch
    nam = new TileReplyMockNam;
    nam->holdReplies = true;
    auto* const fetching = new QGCTileReply(nam, tileReplyMapId(), kTileX + 6, kTileY, kTileZoom);
    QVERIFY(fetching->start());
    QTRY_COMPARE_WITH_TIMEOUT(nam->requestCount, 1, TestTimeout::mediumMs());
    delete nam;
    delete fetching;
}

void QGCTileReplyTest::_tileOfOtherSourceNotServed()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });
    const int mapId = UrlFactory::getMapIdFromProviderType(kTileReplyCustomUrlType);

    // Each source misses the cache once; when it is configured again, its stored tile answers
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

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
        customUrl->setRawValue(request.source);
        nam.body = request.tile;
        if (!request.cached) {
            serverRequests++;
        }

        QGCTileReply reply(&nam, mapId, kTileX + 30, kTileY, kTileZoom);
        QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
        QVERIFY(reply.start());
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
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });
    const int mapId = UrlFactory::getMapIdFromProviderType(kTileReplyCustomUrlType);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();

    // The source changes while the cache is asked for the tile of source A: the tile is fetched from source B
    customUrl->setRawValue(kTileReplySourceA);
    QGCTileReply fetched(&nam, mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &QGCTileReply::finished);
    QVERIFY(fetched.start());
    customUrl->setRawValue(kTileReplySourceB);
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), QGCTileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 1);

    // It is stored as a tile of source B
    QGCTileReply cached(&nam, mapId, kTileX + 31, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    QVERIFY(cached.start());
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

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply reply(&nam, UrlFactory::getMapIdFromProviderType(providerType), tileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    QVERIFY(reply.start());
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.replies.count(), 1);

    const QNetworkRequest request = nam.replies.constFirst()->request();
    const QString userAgent = request.header(QNetworkRequest::UserAgentHeader).toString();
    QCOMPARE(userAgent == QGCNetworkHelper::defaultUserAgent(), namesApplication);
    QCOMPARE(request.hasRawHeader("Referer"), !namesApplication);
    if (namesApplication) {
        QCOMPARE(request.url().scheme(), QStringLiteral("https"));
    }
}

UT_REGISTER_TEST(QGCTileReplyTest, TestLabel::Unit)
