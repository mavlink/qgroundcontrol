#include "TerrariumHeightSourceTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>
#include <algorithm>

#include "TerrariumHeightSource.h"
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
    bool holdReplies = false;      ///< replies stay open until finished manually
    QList<CannedReply*> replies;   ///< creation order, only tracked while holding

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

void TerrariumHeightSourceTest::_deliversFlatRegionHeights()
{
    TerrariumHeightSource source;
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

void TerrariumHeightSourceTest::_deliversSlopeRegionHeights()
{
    TerrariumHeightSource source;
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

void TerrariumHeightSourceTest::_coarseZoomDeliversRealTerrain()
{
    // Terrarium serves one tile per patch at any zoom, so coarse patches get real
    // terrain — no flat floor or blend band (the old TerrainHeightSource fetch-storm hack)
    TerrariumHeightSource source;
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

void TerrariumHeightSourceTest::_deepZoomSamplesAncestorTile()
{
    // The dataset tops out at z15: deeper patches must fetch the z15 ancestor and
    // sample its sub-window instead of requesting tiles that don't exist
    TerrariumHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    constexpr int deepZoom = 18;
    const TileMath::TileKey key = keyOver(flat10Region().center(), deepZoom);
    source.requestPatchHeights(key, kGridSize);

    const TileMath::TileKey fetched = source.lastFetchKey();
    QCOMPARE(fetched.zoom, TerrariumHeightSource::kMaxTileZoom);
    QCOMPARE(fetched.x, key.x >> (deepZoom - TerrariumHeightSource::kMaxTileZoom));
    QCOMPARE(fetched.y, key.y >> (deepZoom - TerrariumHeightSource::kMaxTileZoom));

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }
}

void TerrariumHeightSourceTest::_cancelPreventsDelivery()
{
    TerrariumHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), kGridSize);
    source.cancelRequest(requestId);

    QVERIFY_NO_SIGNAL_WAIT(readySpy, TestTimeout::shortMs());
    QVERIFY(failedSpy.isEmpty());
}

void TerrariumHeightSourceTest::_invalidGridSizeFails()
{
    TerrariumHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), 0);
    QCOMPARE_GT(requestId, 0);
    QVERIFY(failedSpy.isEmpty());  // failure is async too

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

void TerrariumHeightSourceTest::_networkFallbackDeliversAndCaches()
{
    // Distinct zoom from other tests: write-back puts this tile in the shared
    // per-process cache DB, which must not shadow other tests' generator path
    constexpr int zoom = 13;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumHeightSource source(nullptr, &nam);
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

void TerrariumHeightSourceTest::_networkErrorFails()
{
    constexpr int zoom = 12;

    MockNam nam;  // empty body: canned network error
    TerrariumHeightSource source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), zoom), kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
}

void TerrariumHeightSourceTest::_networkGarbageBodyFailsAndNotCached()
{
    // An HTTP-200 non-PNG body (e.g. an error page) must fail the request and
    // must NOT be written back to the cache
    constexpr int zoom = 15;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = QByteArrayLiteral("this is not a png");
    TerrariumHeightSource source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

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

void TerrariumHeightSourceTest::_networkWrongSizeImageFailsAndNotCached()
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

    TerrariumHeightSource source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

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

void TerrariumHeightSourceTest::_duplicateRequestsShareOneFetch()
{
    constexpr int zoom = 10;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumHeightSource source(nullptr, &nam);
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

void TerrariumHeightSourceTest::_cancelOneWaiterKeepsSharedFetchAlive()
{
    constexpr int zoom = 9;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    TerrariumHeightSource source(nullptr, &nam);
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

void TerrariumHeightSourceTest::_staleCancelledReplyDoesNotFailNewRequest()
{
    // A cancelled reply whose finished emission lands late must not touch the
    // state of a newer fetch for the same tile installed in the meantime
    constexpr int zoom = 7;
    const TileMath::TileKey key = keyOver(flat10Region().center(), zoom);

    MockNam nam;
    nam.body = UnitTestTileGenerator::syntheticTerrariumTileData(key.x, key.y, key.zoom);
    nam.holdReplies = true;
    TerrariumHeightSource source(nullptr, &nam);
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId1 = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());
    source.cancelRequest(requestId1);

    const int requestId2 = source.requestPatchHeights(key, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());

    // The stale reply's abort error arrives only now, after the new fetch owns
    // the tile: it must be ignored, and the new reply must deliver normally
    nam.replies.at(0)->finishAsAborted();
    nam.replies.at(1)->finishNormally();

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId2);
    QVERIFY(failedSpy.isEmpty());
}

UT_REGISTER_TEST(TerrariumHeightSourceTest, TestLabel::Integration, TestLabel::Terrain)
