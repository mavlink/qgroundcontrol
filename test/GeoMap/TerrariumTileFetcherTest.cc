#include "TerrariumTileFetcherTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include <algorithm>
#include <cstring>
#include <limits>

#include "ElevationMapProvider.h"
#include "HeightField.h"
#include "QGeoFileTileCacheQGC.h"
#include "TerrariumTileFetcher.h"
#include "TileMath.h"
#include "UnitTestTileGenerator.h"

// These tests run the production terrarium pipeline: tile cache misses are served
// synthetic terrarium PNGs built from the UnitTestTerrainData regions (see
// UnitTestTileGenerator). No network access ever occurs. Network-fallback tests
// force generator misses and serve canned replies from a mock QNetworkAccessManager.

namespace {
constexpr int kGridSize = 16;
constexpr int kExpectedCount = (kGridSize + 1) * (kGridSize + 1);
constexpr int kFineZoom = 14;  ///< patch span ~1.6km at this latitude, well inside the 11km test regions

TileMath::TileKey keyOver(const QGeoCoordinate& coord, int zoom)
{
    return TileMath::tileForWorld(TileMath::geoToWorld(coord), zoom);
}

/// Serves a canned body (or an error when the body is empty) asynchronously;
/// held replies stay open until finished manually (stale-reply race testing)
class CannedReply : public QNetworkReply
{
public:
    CannedReply(const QNetworkRequest& request, const QByteArray& body, QObject* parent, bool hold = false)
        : QNetworkReply(parent), _body(body)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        (void) open(ReadOnly);
        if (!hold) {
            QTimer::singleShot(0, this, &CannedReply::finishNormally);
        }
    }

    void finishNormally()
    {
        if (_body.isEmpty()) {
            setError(ContentNotFoundError, QStringLiteral("canned error"));
            emit errorOccurred(ContentNotFoundError);
        }
        setFinished(true);
        emit finished();
    }

    /// Late abort-error delivery: models a cancelled reply whose queued
    /// finished emission lands after cancelRequest returned
    void finishAsAborted()
    {
        setError(OperationCanceledError, QStringLiteral("canned abort"));
        emit errorOccurred(OperationCanceledError);
        setFinished(true);
        emit finished();
    }

    void abort() final {}

    qint64 bytesAvailable() const final { return (_body.size() - _offset) + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxSize) final
    {
        const qint64 n = qMin<qint64>(maxSize, _body.size() - _offset);
        if (n <= 0) {
            return (_offset >= _body.size()) ? -1 : 0;
        }
        (void) memcpy(data, _body.constData() + _offset, static_cast<size_t>(n));
        _offset += n;
        return n;
    }

private:
    const QByteArray _body;
    qint64 _offset = 0;
};

/// Counts requests and serves the configured body (empty body = network error)
class MockNam : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QByteArray body;
    int requestCount = 0;
    bool holdReplies = false;     ///< replies stay open until finished manually
    QList<CannedReply*> replies;  ///< creation order, only tracked while holding

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        CannedReply* const reply = new CannedReply(request, body, this, holdReplies);
        if (holdReplies) {
            replies.append(reply);
        }
        return reply;
    }
};
}  // namespace

void TerrariumTileFetcherTest::_deliversFlatRegionHeights()
{
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), kGridSize);
    QCOMPARE_GT(requestId, 0);
    QVERIFY(readySpy.isEmpty());  // delivery is async, never re-entrant

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    QVERIFY(failedSpy.isEmpty());

    // 10m encodes exactly in terrarium's 1/256 m quanta, so bilinear sampling
    // of a flat field must return it exactly
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }
}

void TerrariumTileFetcherTest::_deliversSlopeRegionHeights()
{
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    source.requestPatchHeights(keyOver(linearSlopeRegion().center(), kFineZoom), kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());

    // The slope rises 100m/km west to east: each row must rise substantially
    // across the ~1.6km ground span of a zoom-14 patch at this latitude
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    const int verticesPerEdge = kGridSize + 1;
    for (int row = 0; row < verticesPerEdge; row++) {
        const float west = heights.at(row * verticesPerEdge);
        const float east = heights.at((row * verticesPerEdge) + kGridSize);
        QCOMPARE_GT(east - west, 50.0f);
        // Bilinear sampling keeps adjacent vertices smooth on a linear slope
        for (int col = 0; col < kGridSize; col++) {
            const float a = heights.at((row * verticesPerEdge) + col);
            const float b = heights.at((row * verticesPerEdge) + col + 1);
            QCOMPARE_LT(qAbs(b - a), 20.0f);
        }
    }
}

void TerrariumTileFetcherTest::_patchSamplingMatchesFieldSampling()
{
    // Cross-source vertex identity: heights sampled from the tile image (patch
    // delivery) and from the decoded field grid must be bit-identical, or
    // patches meshed from different sources crack at shared edges
    HeightField field;
    TerrariumTileFetcher source;
    source.setHeightField(&field);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    const TileMath::TileKey key = keyOver(linearSlopeRegion().center(), kFineZoom);
    const int requestId = source.requestPatchHeights(key, kGridSize);
    QVERIFY(source.requestTile(key));

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);

    const auto imageHeights = readySpy.first().at(1).value<QList<float>>();
    const QList<float> fieldHeights = field.samplePatch(key, kGridSize);
    QCOMPARE(imageHeights.count(), kExpectedCount);
    QCOMPARE(fieldHeights, imageHeights);
}

void TerrariumTileFetcherTest::_coarseZoomDeliversRealTerrain()
{
    // Terrarium serves one tile per patch at any zoom, so coarse patches get real
    // terrain — no flat floor or blend band (the old TerrainHeightSource fetch-storm hack)
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    constexpr int coarseZoom = 11;  // below the old TerrainHeightSource's minimum terrain zoom
    source.requestPatchHeights(keyOver(flat10Region().center(), coarseZoom), kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(failedSpy.isEmpty());

    // The ~13km patch overlaps the test regions (flat 10m, plus the adjacent
    // slope region): real unscaled terrain must show at least the flat region's
    // height (flat zeros or a blend-band scale would deliver 0 everywhere)
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    float maxHeight = std::numeric_limits<float>::lowest();
    for (float h : heights) {
        maxHeight = qMax(maxHeight, h);
    }
    QCOMPARE_GE(maxHeight, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
}

void TerrariumTileFetcherTest::_deepZoomSamplesAncestorTile()
{
    // The dataset tops out at z15: deeper patches must fetch the z15 ancestor and
    // sample its sub-window instead of requesting tiles that don't exist
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    constexpr int deepZoom = 18;
    const TileMath::TileKey key = keyOver(flat10Region().center(), deepZoom);
    source.requestPatchHeights(key, kGridSize);

    const TileMath::TileKey fetched = source.lastFetchKey();
    QCOMPARE(fetched.zoom, TerrariumTileFetcher::kMaxTileZoom);
    QCOMPARE(fetched.x, key.x >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));
    QCOMPARE(fetched.y, key.y >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }
}

void TerrariumTileFetcherTest::_cancelPreventsDelivery()
{
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), kGridSize);
    source.cancelRequest(requestId);

    QVERIFY_NO_SIGNAL_WAIT(readySpy, TestTimeout::shortMs());
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumTileFetcherTest::_invalidGridSizeFails()
{
    TerrariumTileFetcher source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^requestPatchHeights rejected:")));
    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), 0);
    verifyExpectedLogMessage();
    QCOMPARE_GT(requestId, 0);
    QVERIFY(failedSpy.isEmpty());  // failure is async too

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

void TerrariumTileFetcherTest::_oversizeGridSizeFails()
{
    // A gridSize beyond the supported maximum is a programming error: warn and
    // fail async like the other invalid inputs
    MockNam nam;
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("gridSize")));
    const int requestId =
        source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), TerrariumTileFetcher::kMaxGridSize + 1);
    verifyExpectedLogMessage();
    QCOMPARE_GT(requestId, 0);

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

void TerrariumTileFetcherTest::_invalidKeyFails()
{
    // An invalid key must fail asynchronously before any shift arithmetic or
    // fetch machinery runs
    MockNam nam;
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^requestPatchHeights rejected:")));
    const int requestId = source.requestPatchHeights(TileMath::TileKey{0, 0, -1}, kGridSize);
    verifyExpectedLogMessage();
    QCOMPARE_GT(requestId, 0);

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
    QCOMPARE(nam.requestCount, 0);
}

void TerrariumTileFetcherTest::_networkFallbackDeliversAndCaches()
{
    // Distinct zoom from other tests: write-back puts this tile in the shared
    // per-process cache DB, which must not shadow other tests' generator path
    constexpr int zoom = 13;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    // Two forced generator misses: the first request must fall back to the
    // network; the second must be served by the write-back cached tile (the
    // DB hit precedes the generator, so the second miss stays unconsumed)
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }

    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);  // cached by write-back: no second network fetch
}

void TerrariumTileFetcherTest::_networkErrorFails()
{
    constexpr int zoom = 12;

    MockNam nam;  // empty body: canned network error
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Fetch failures must be visible without logging configuration (matches
    // the map tile / Copernicus convention); repeats are throttled
    expectLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), zoom), kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
    verifyExpectedLogMessage();
}

void TerrariumTileFetcherTest::_networkGarbageBodyFailsAndNotCached()
{
    // An HTTP-200 non-PNG body (e.g. an error page) must fail the request and
    // must NOT be written back to the cache
    constexpr int zoom = 15;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = QByteArrayLiteral("this is not a png");
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    ignoreLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);

    // Garbage was not cached: the retry misses again and re-fetches, this time
    // getting a valid tile
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
}

void TerrariumTileFetcherTest::_networkWrongSizeImageFailsAndNotCached()
{
    // A decodable body that isn't a 256x256 tile (e.g. a CDN placeholder image)
    // is not elevation data: it must fail delivery, and caching it would poison
    // the shared cache — every retry would hit the same invalid tile
    constexpr int zoom = 8;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    QImage placeholder(100, 100, QImage::Format_RGB32);
    placeholder.fill(Qt::black);
    QBuffer buffer(&nam.body);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(placeholder.save(&buffer, "PNG"));

    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    ignoreLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);

    // The wrong-size image was not cached: the retry misses again and
    // re-fetches, this time getting a valid tile
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
}

void TerrariumTileFetcherTest::_cachedUnusableTileFallsBackToNetwork()
{
    // Pre-seed the shared cache with an undecodable body: the cache-hit path
    // must treat it as a miss and recover via the network — failing would
    // wedge the tile on the same bytes every retry until cache eviction
    constexpr int zoom = 15;  // tile span ~0.8km: the +2 offset stays well inside the 11km flat region
    const TileMath::TileKey centerKey = keyOver(flat10Region().center(), zoom);
    // Offset off other tests' write-back keys: the seeded garbage stays in the
    // shared per-process cache DB
    const TileMath::TileKey key{centerKey.x + 2, centerKey.y, zoom};
    // The cache worker runs tasks in order: the store lands before the lookup
    QGeoFileTileCacheQGC::cacheTile(QString::fromLatin1(TerrariumElevationProvider::kProviderKey), key.x, key.y,
                                    key.zoom, QByteArrayLiteral("garbage"), QStringLiteral("png"));

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);  // cache hit rejected, recovered from the network
    QVERIFY(failedSpy.isEmpty());
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }
}

void TerrariumTileFetcherTest::_duplicateRequestsShareOneFetch()
{
    constexpr int zoom = 10;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Same tile requested twice before either completes: one shared fetch
    // must serve both (only one forced miss gets consumed)
    const int requestId1 = source.requestPatchHeights(key, kGridSize);
    const int requestId2 = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 1);

    QList<int> readyIds{readySpy.at(0).at(0).toInt(), readySpy.at(1).at(0).toInt()};
    std::sort(readyIds.begin(), readyIds.end());
    const QList<int> expectedIds{qMin(requestId1, requestId2), qMax(requestId1, requestId2)};
    QCOMPARE(readyIds, expectedIds);
}

void TerrariumTileFetcherTest::_cancelOneWaiterKeepsSharedFetchAlive()
{
    constexpr int zoom = 9;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId1 = source.requestPatchHeights(key, kGridSize);
    const int requestId2 = source.requestPatchHeights(key, kGridSize);
    source.cancelRequest(requestId1);

    // Cancelling one waiter must not abort the shared fetch for the other
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId2);
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumTileFetcherTest::_cancelDestroysAbortedReply()
{
    // Cleanup must not rely on abort emitting finished (real QNetworkReply does,
    // but it isn't guaranteed here — CannedReply::abort is a no-op)
    constexpr int zoom = 1;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    TerrariumTileFetcher source(nullptr, &nam);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    const QPointer<CannedReply> reply(nam.replies.at(0));
    source.cancelRequest(requestId);
    QTRY_VERIFY_WITH_TIMEOUT(reply.isNull(), TestTimeout::mediumMs());
}

void TerrariumTileFetcherTest::_staleCancelledReplyDoesNotFailNewRequest()
{
    // A cancelled reply whose finished emission lands late must not touch the
    // state of a newer fetch for the same tile installed in the meantime
    constexpr int zoom = 7;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    TerrariumTileFetcher source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId1 = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());
    source.cancelRequest(requestId1);

    // Real QNetworkReply::abort() emits finished synchronously inside
    // cancelRequest; deliver it now, while the reply is still alive
    // (cancelRequest schedules deleteLater on it). The stale guard must
    // ignore it and a new fetch for the same tile must deliver normally
    nam.replies.at(0)->finishAsAborted();

    const int requestId2 = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    nam.replies.at(1)->finishNormally();

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId2);
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumTileFetcherTest::_tileRequestPopulatesField()
{
    HeightField field;
    TerrariumTileFetcher source;
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);
    QVERIFY(regionSpy.isValid());

    const TileMath::TileKey key = keyOver(flat10Region().center(), kFineZoom);
    QVERIFY(source.requestTile(key));
    QCOMPARE(field.tileCount(), 0);  // delivery is async, never re-entrant

    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(field.tileCount(), 1);

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
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    constexpr int deepZoom = 18;
    const TileMath::TileKey key = keyOver(flat10Region().center(), deepZoom);
    QVERIFY(source.requestTile(key));

    const TileMath::TileKey fetched = source.lastFetchKey();
    QCOMPARE(fetched.zoom, TerrariumTileFetcher::kMaxTileZoom);
    QCOMPARE(fetched.x, key.x >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));
    QCOMPARE(fetched.y, key.y >> (deepZoom - TerrariumTileFetcher::kMaxTileZoom));

    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(field.tileCount(), 1);
    QCOMPARE(regionSpy[0][0].toRectF().width(), TileMath::tileSpanAtZoom(TerrariumTileFetcher::kMaxTileZoom));

    const double height = field.heightAt(TileMath::geoToWorld(flat10Region().center()));
    QCOMPARE_LT(qAbs(height - UnitTestTerrainData::Flat10Region::amslElevation), 0.01);
}

void TerrariumTileFetcherTest::_duplicateTileRequestsShareOneFetch()
{
    constexpr int zoom = 6;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
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

    MockNam nam;
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
    QVERIFY_NO_SIGNAL_WAIT(regionSpy, TestTimeout::shortMs());
    QCOMPARE(nam.requestCount, 1);
}

void TerrariumTileFetcherTest::_tileFetchFailureLeavesFieldIntact()
{
    constexpr int zoom = 4;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;  // empty body: canned network error
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    ignoreLogMessage("GeoMap.TerrariumTileFetcher", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());
    QVERIFY_NO_SIGNAL_WAIT(regionSpy, TestTimeout::shortMs());
    QCOMPARE(field.tileCount(), 0);

    // Failure clears the in-flight key: a retry fetches again and succeeds
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    QVERIFY(source.requestTile(key));
    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(field.tileCount(), 1);
}

void TerrariumTileFetcherTest::_fieldRequestKeepsFetchAliveAfterCancel()
{
    constexpr int zoom = 2;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    HeightField field;
    TerrariumTileFetcher source(nullptr, &nam);
    source.setHeightField(&field);
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    // The field piggybacks on the patch fetch already in flight
    QVERIFY(source.requestTile(key));
    QCOMPARE(nam.requestCount, 1);

    // Cancelling the last patch waiter must not abort a fetch the field wants
    source.cancelRequest(requestId);
    nam.replies.at(0)->finishNormally();

    QTRY_COMPARE_WITH_TIMEOUT(regionSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(field.tileCount(), 1);
    QVERIFY(readySpy.isEmpty());  // the cancelled patch request stays silent
}

void TerrariumTileFetcherTest::_tileRequestGuards()
{
    MockNam nam;
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

UT_REGISTER_TEST(TerrariumTileFetcherTest, TestLabel::Integration, TestLabel::Terrain)
