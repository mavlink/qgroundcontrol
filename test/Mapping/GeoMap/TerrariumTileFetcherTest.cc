#include "TerrariumTileFetcherTest.h"

#include <algorithm>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "GeoMapCamera.h"
#include "HeightField.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "SurfaceModel.h"
#include "TerrariumTileFetcher.h"
#include "TileMath.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

// These tests run the production terrarium pipeline: tile cache misses are served
// synthetic terrarium PNGs built from the UnitTestTerrainData regions (see
// UnitTestTileGenerator). No network access ever occurs. Network-fallback tests
// force generator misses and serve canned replies from a mock QNetworkAccessManager.

namespace {
constexpr int kFineZoom = 14;     ///< tile span ~1.6km at this latitude, well inside the 11km test regions
constexpr int kRetryBaseMs = 50;  ///< first delay of the paced retry: the production one is seconds

TileMath::TileKey keyOver(const QGeoCoordinate& coord, int zoom)
{
    return TileMath::tileForWorld(TileMath::geoToWorld(coord), zoom);
}

QString tilePath(const TileMath::TileKey& key)
{
    return QStringLiteral("/%1/%2/%3.png").arg(key.zoom).arg(key.x).arg(key.y);
}
}  // namespace

void TerrariumTileFetcherTest::initTestCase()
{
    TerrainTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void TerrariumTileFetcherTest::_tileRequestPopulatesField()
{
    HeightField field;
    TerrariumTileFetcher source;
    source.setHeightField(&field);

    const TileMath::TileKey key = keyOver(flat10Region().center(), kFineZoom);
    QVERIFY(source.requestTile(key));
    QCOMPARE(field.tileCount(), 0);  // delivery is async, never re-entrant

    // Inserted only after its anchors
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE_GE(field.tileCount(), 2);

    // 10m encodes exactly in terrarium quanta; bilinear blend of equal corners
    // may differ by rounding only
    const double height = field.heightAt(TileMath::geoToWorld(flat10Region().center()));
    QCOMPARE_LT(qAbs(height - UnitTestTerrainData::Flat10Region::amslElevation), 0.01);
}

void TerrariumTileFetcherTest::_deepZoomTileRequestFetchesAncestor()
{
    // The dataset tops out at z15: a deeper tile request must fetch and store
    // the z15 ancestor (the pyramid sub-windows it at sample time)
    HeightField field;
    TerrariumTileFetcher source;
    source.setHeightField(&field);

    constexpr int deepZoom = 18;
    const TileMath::TileKey key = keyOver(flat10Region().center(), deepZoom);
    QVERIFY(source.requestTile(key));

    const TileMath::TileKey fetched = source.lastFetchKey();
    QCOMPARE(fetched.zoom, TerrariumTileFetcher::kMaxTileZoom);
    QCOMPARE(fetched.x, key.x >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));
    QCOMPARE(fetched.y, key.y >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));

    // Inserted only after its anchors
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(fetched), TestTimeout::mediumMs());
    QCOMPARE_GE(field.tileCount(), 2);

    const double height = field.heightAt(TileMath::geoToWorld(flat10Region().center()));
    QCOMPARE_LT(qAbs(height - UnitTestTerrainData::Flat10Region::amslElevation), 0.01);
}

void TerrariumTileFetcherTest::_slopeTileRisesWestToEast()
{
    HeightField field;
    TerrariumTileFetcher source;
    source.setHeightField(&field);

    const TileMath::TileKey key = keyOver(linearSlopeRegion().center(), kFineZoom);
    QVERIFY(source.requestTile(key));
    // Inserted only after its anchors
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());

    // The slope rises 1100 m over the 0.1 degrees of its region, from west to east only: about 120 m over half of
    // this tile. A decode that swaps rows and columns would show that rise from north to south.
    const QPointF minCorner = TileMath::tileMinCorner(key);
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const auto heightAt = [&field, &minCorner, span](double east, double north) {
        return field.heightAt(minCorner + QPointF(span * east, span * north));
    };
    QCOMPARE_GT(heightAt(0.75, 0.5) - heightAt(0.25, 0.5), 100.0);
    QCOMPARE_LT(qAbs(heightAt(0.5, 0.75) - heightAt(0.5, 0.25)), 1.0);
}

void TerrariumTileFetcherTest::_networkFallbackDeliversAndCaches()
{
    // A zoom of its own: write-back puts this tile in the shared per-process cache DB, which must not shadow
    // other tests' generator path. No finer than the anchors, so that this tile is the only one fetched.
    constexpr int zoom = 9;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);

    // Two forced generator misses: the first request must fall back to the
    // network; the second must be served by the write-back cached tile (the
    // DB hit precedes the generator, so the second miss stays unconsumed)
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QVERIFY(source.requestTile(key));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);
    const double height = field.heightAt(TileMath::geoToWorld(flat10Region().center()));
    QCOMPARE_LT(qAbs(height - UnitTestTerrainData::Flat10Region::amslElevation), 0.01);

    // Another fetcher has the tile neither in its field nor in flight
    HeightField secondField;
    TerrariumTileFetcher secondSource(nullptr, &nam);
    secondSource.setHeightField(&secondField);
    QVERIFY(secondSource.requestTile(key));
    QTRY_VERIFY_WITH_TIMEOUT(secondField.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);  // cached by write-back: no second network fetch
}

void TerrariumTileFetcherTest::_networkBadBodyFailsAndNotCached_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("zoom");

    // Zooms of their own, as the valid tile of the second fetch is written back to the shared cache.
    // An HTTP-200 body that is no PNG, such as an error page
    QTest::newRow("garbage") << QByteArrayLiteral("this is not a png") << 7;
    // An image that is no 256x256 tile, such as a CDN placeholder: it decodes, but is not elevation data
    QTest::newRow("wrongSizeImage") << tileReplyPngBody(Qt::black, 100) << 8;
}

void TerrariumTileFetcherTest::_networkBadBodyFailsAndNotCached()
{
    QFETCH(QByteArray, body);
    QFETCH(int, zoom);

    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    TileReplyMockNam nam;
    nam.body = body;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(field.tileCount(), 0);

    // Caching the body would poison the shared cache: every retry would hit the same invalid tile. It was not
    // cached: the retry misses again and re-fetches, this time getting a valid tile
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    QVERIFY(source.requestTile(key));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
}

void TerrariumTileFetcherTest::_cachedUnusableTileFallsBackToNetwork()
{
    // Pre-seed the shared cache with an undecodable body: the cache-hit path
    // must treat it as a miss and recover via the network — failing would
    // wedge the tile on the same bytes every retry until cache eviction
    const TileMath::TileKey centerKey = keyOver(flat10Region().center(), 9);
    // Offset off other tests' write-back keys: the seeded garbage stays in the
    // shared per-process cache DB
    const TileMath::TileKey key{centerKey.x + 2, centerKey.y, centerKey.zoom};
    // The cache worker runs tasks in order: the store lands before the lookup
    const QString type = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
    getQGCMapEngine()->cacheTile(type, UrlFactory::getTileHash(type, key.x, key.y, key.zoom),
                                 QByteArrayLiteral("garbage"), QStringLiteral("png"));

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    QVERIFY(source.requestTile(key));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);  // cache hit rejected, recovered from the network
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumTileFetcherTest::_duplicateTileRequestsShareOneFetch()
{
    constexpr int zoom = 6;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Same tile requested twice before the fetch completes: one shared fetch,
    // one insert
    QVERIFY(source.requestTile(key));
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);
    QCOMPARE(field.tileCount(), 1);
}

void TerrariumTileFetcherTest::_heldTileNotRefetched()
{
    constexpr int zoom = 5;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    UnitTestTileGenerator::setForcedMissCount(3);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);

    // The field already holds this tile: a new request is a satisfied no-op
    QVERIFY(source.requestTile(key));
    // Negative window: a refetch would come from the local tile cache well within 250 ms
    QVERIFY_NO_SIGNAL_WAIT(regionSpy, 250);
    QCOMPARE(nam.requestCount, 1);
}

void TerrariumTileFetcherTest::_tileFetchFailureLeavesFieldIntact()
{
    constexpr int zoom = 4;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    TileReplyMockNam nam;
    nam.error = QNetworkReply::ConnectionRefusedError;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Fetch failures must be visible without logging configuration (matches
    // the map tile / Copernicus convention); repeats are throttled
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).value<TileMath::TileKey>(), key);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(regionSpy.isEmpty());
    QCOMPARE(field.tileCount(), 0);

    // Failure clears the in-flight key: a retry fetches again and succeeds
    nam.error = QNetworkReply::NoError;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(field.tileCount(), 1);
}

void TerrariumTileFetcherTest::_tileRequestGuards()
{
    TileReplyMockNam nam;
    TerrariumTileFetcher source(nullptr, &nam);

    // No field attached: rejected before any fetch machinery runs
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^requestTile rejected:")));
    QVERIFY(!source.requestTile(keyOver(flat10Region().center(), 3)));
    verifyExpectedLogMessage();

    HeightField field;
    source.setHeightField(&field);
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^requestTile rejected:")));
    QVERIFY(!source.requestTile(TileMath::TileKey{0, 0, -1}));
    verifyExpectedLogMessage();

    QCOMPARE(nam.requestCount, 0);
    QCOMPARE(field.tileCount(), 0);
}

void TerrariumTileFetcherTest::_fineTileHeldUntilAnchorsArrive_data()
{
    QTest::addColumn<double>("longitudeOffset");
    QTest::addColumn<int>("fineX");
    QTest::addColumn<int>("fineY");
    QTest::addColumn<int>("anchorCount");

    // Longitude offsets keep each row's anchors apart and off the test regions:
    // no other test writes tiles back to the shared cache there
    QTest::newRow("inside its anchor") << 1.0 << 8 << 8 << 1;
    QTest::newRow("on its anchor's west edge") << 2.0 << 0 << 8 << 2;
    QTest::newRow("in its anchor's NW corner") << 3.0 << 0 << 0 << 4;
}

void TerrariumTileFetcherTest::_fineTileHeldUntilAnchorsArrive()
{
    QFETCH(double, longitudeOffset);
    QFETCH(int, fineX);
    QFETCH(int, fineY);
    QFETCH(int, anchorCount);

    const QGeoCoordinate coord(flat10Region().center().latitude(),
                               flat10Region().center().longitude() + longitudeOffset);
    const TileMath::TileKey anchor = keyOver(coord, TerrariumTileFetcher::kAnchorZoom);
    const int shift = kFineZoom - TerrariumTileFetcher::kAnchorZoom;
    const TileMath::TileKey key{(anchor.x << shift) + fineX, (anchor.y << shift) + fineY, kFineZoom};

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    UnitTestTileGenerator::setForcedMissCount(anchorCount + 1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // A fine tile request also fetches the anchor of every tile it touches:
    // its own, plus the neighbors' across an anchor boundary
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, anchorCount + 1, TestTimeout::mediumMs());
    QPointer<TileReplyCannedReply> fineReply;
    QList<QPointer<TileReplyCannedReply>> anchorReplies;
    for (const QPointer<TileReplyCannedReply>& reply : std::as_const(nam.replies)) {
        if (reply->request().url().path().endsWith(tilePath(key))) {
            fineReply = reply;
        } else {
            anchorReplies.append(reply);
        }
    }
    QVERIFY(fineReply);
    QCOMPARE(anchorReplies.count(), anchorCount);

    // Fine tile first: inserting it now would leave neighbors without any data
    // rendering 0 m next to real terrain
    fineReply->finishNow();  // the fetcher handles the reply synchronously
    QVERIFY(regionSpy.isEmpty());
    QCOMPARE(field.tileCount(), 0);

    for (int i = 0; i < (anchorCount - 1); i++) {
        anchorReplies.at(i)->finishNow();
        QTRY_COMPARE_WITH_TIMEOUT(field.tileCount(), i + 1, TestTimeout::mediumMs());
        QVERIFY(!field.hasTile(key));
    }
    anchorReplies.last()->finishNow();
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QCOMPARE(field.tileCount(), anchorCount + 1);
    QVERIFY(field.hasTile(anchor));
    QCOMPARE(regionSpy.count(), anchorCount + 1);
    QCOMPARE(regionSpy.last()[0].toRectF().width(), TileMath::tileSpanAtZoom(kFineZoom));
}

void TerrariumTileFetcherTest::_failedAnchorFetchedAgain()
{
    // Its own longitude offset, see _fineTileHeldUntilAnchorsArrive
    const QGeoCoordinate coord(flat10Region().center().latitude(), flat10Region().center().longitude() + 5.0);
    const TileMath::TileKey anchor = keyOver(coord, TerrariumTileFetcher::kAnchorZoom);
    const int shift = kFineZoom - TerrariumTileFetcher::kAnchorZoom;
    const TileMath::TileKey key{(anchor.x << shift) + 8, (anchor.y << shift) + 8, kFineZoom};  // inside its anchor

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    const bool fineFirst = nam.replies.at(0)->request().url().path().endsWith(tilePath(key));
    const QPointer<TileReplyCannedReply> fineReply = nam.replies.at(fineFirst ? 0 : 1);
    const QPointer<TileReplyCannedReply> anchorReply = nam.replies.at(fineFirst ? 1 : 0);

    // The anchor fails: that is reported, and the fine tile is not held back
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    anchorReply->failNow(QNetworkReply::ConnectionRefusedError);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).value<TileMath::TileKey>(), anchor);
    fineReply->finishNow();
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    QVERIFY(!field.hasTile(anchor));

    // Asking again for the tile the field already holds fetches its anchor again (the cache serves it now)
    QVERIFY(source.requestTile(key));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(anchor), TestTimeout::mediumMs());
    // The late anchor backs the rest of its region and leaves the finer data in place
    QCOMPARE(field.backingKeyFor(key), key);
    QCOMPARE(field.backingKeyFor(TileMath::TileKey{key.x + 1, key.y, key.zoom}), anchor);

    // With the anchor in the field the same request fetches nothing. The cache answers in request order,
    // so a fetch started by it would land before the barrier tile.
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);
    QVERIFY(source.requestTile(key));
    const TileMath::TileKey barrier{anchor.x + 2, anchor.y, anchor.zoom};
    QVERIFY(source.requestTile(barrier));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(barrier), TestTimeout::mediumMs());
    QCOMPARE(regionSpy.count(), 1);
}

void TerrariumTileFetcherTest::_residentPatchTileArrivesAfterOutage()
{
    // Its own longitude offset: no other test leaves tiles in the shared cache there
    const QGeoCoordinate coord(flat10Region().center().latitude(), flat10Region().center().longitude() + 7.0);

    TileReplyMockNam nam;
    nam.error = QNetworkReply::ConnectionRefusedError;
    GeoMapCamera camera;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    SurfaceModel model(&camera, &source, &field);
    model.setTileRetryBaseIntervalMs(kRetryBaseMs);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    // Outage: the cache misses and the server cannot be reached
    UnitTestTileGenerator::setForcedMissCount(10000);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    camera.setViewportSize(QSizeF(800, 600));
    camera.lookAt(coord, 0, 0, 2000);
    model.drainUpdates();

    // Tile of the patch under the camera
    const QPointF world = TileMath::geoToWorld(coord);
    int zoom = -1;
    for (const SurfaceModel::Patch& patch : model.patches()) {
        if (patch.key == TileMath::tileForWorld(world, patch.key.zoom)) {
            zoom = qMax(zoom, patch.key.zoom);
        }
    }
    QCOMPARE_GE(zoom, 0);
    const TileMath::TileKey key = keyOver(coord, qMin(zoom, TerrariumTileFetcher::kMaxTileZoom));
    const auto failed = [&failedSpy, &key] {
        return std::any_of(failedSpy.cbegin(), failedSpy.cend(), [&key](const QList<QVariant>& arguments) {
            return arguments.at(0).value<TileMath::TileKey>() == key;
        });
    };
    QTRY_VERIFY_WITH_TIMEOUT(failed(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!field.hasTile(key));

    // The outage ends with the camera at rest: nothing but the paced retry asks for the tile again
    UnitTestTileGenerator::setForcedMissCount(0);
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(key), TestTimeout::mediumMs());
    // The rounds of the outage put each other off for longer. The tiles that arrive end that.
    QTRY_COMPARE_WITH_TIMEOUT(model.tileRetryIntervalMs(), kRetryBaseMs, TestTimeout::mediumMs());
}

void TerrariumTileFetcherTest::_missingTileNotFetchedAgain()
{
    // Zooms of their own, and no finer than the anchors: each request fetches one tile
    const TileMath::TileKey key = keyOver(flat10Region().center(), 2);
    const TileMath::TileKey barrier = keyOver(flat10Region().center(), 1);

    TileReplyMockNam nam;  // the provider has no tile
    nam.error = QNetworkReply::ContentNotFoundError;
    nam.httpStatus = 404;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);
    QSignalSpy replyFinishedSpy(&nam, &QNetworkAccessManager::finished);

    // Every lookup misses, not a count of them: lookups that an earlier test left with the cache take from a count
    UnitTestTileGenerator::setForcedMissCount(10000);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // No tile is no failure: none is reported, and none is logged, which the log check of the test would tell
    QVERIFY(source.requestTile(key));
    // The fetcher handles the reply within the same finished emission
    QTRY_COMPARE_WITH_TIMEOUT(replyFinishedSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(failedSpy.isEmpty());
    QCOMPARE(field.tileCount(), 0);

    // Asking again fetches nothing. The cache answers in request order, so a fetch of the tile would reach the
    // network before the fetch of the barrier tile.
    QVERIFY(source.requestTile(key));
    QVERIFY(source.requestTile(barrier));
    QTRY_COMPARE_WITH_TIMEOUT(replyFinishedSpy.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
    QVERIFY(nam.requests.last().url().path().endsWith(tilePath(barrier)));
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumTileFetcherTest::_missingAnchorDoesNotHoldFineTile()
{
    // Its own longitude offset, see _fineTileHeldUntilAnchorsArrive
    const QGeoCoordinate coord(flat10Region().center().latitude(), flat10Region().center().longitude() + 9.0);
    const TileMath::TileKey anchor = keyOver(coord, TerrariumTileFetcher::kAnchorZoom);
    const int shift = kFineZoom - TerrariumTileFetcher::kAnchorZoom;
    const TileMath::TileKey key{(anchor.x << shift) + 8, (anchor.y << shift) + 8, kFineZoom};  // inside its anchor

    TileReplyMockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy failedSpy(&source, &HeightSource::tileFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    const bool fineFirst = nam.replies.at(0)->request().url().path().endsWith(tilePath(key));
    const QPointer<TileReplyCannedReply> fineReply = nam.replies.at(fineFirst ? 0 : 1);
    const QPointer<TileReplyCannedReply> anchorReply = nam.replies.at(fineFirst ? 1 : 0);

    fineReply->finishNow();  // the fetcher handles a reply synchronously
    QVERIFY(!field.hasTile(key));

    // The provider has no anchor tile: the fine tile waits no longer, and nothing has failed
    anchorReply->failNow(QNetworkReply::ContentNotFoundError, 404);
    QVERIFY(field.hasTile(key));
    QVERIFY(!field.hasTile(anchor));
    QVERIFY(failedSpy.isEmpty());
}

UT_REGISTER_TEST(TerrariumTileFetcherTest, TestLabel::Integration, TestLabel::Terrain)
