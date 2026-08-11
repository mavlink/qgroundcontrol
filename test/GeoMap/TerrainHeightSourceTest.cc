#include "TerrainHeightSourceTest.h"

#include <QtTest/QSignalSpy>

#include "TerrainHeightSource.h"
#include "TileMath.h"

// These tests run the full production terrain pipeline: TerrainHeightSource routes
// through TerrainTileManager, whose elevation tile cache misses are served synthetic
// tiles built from the UnitTestTerrainData regions (see UnitTestTileGenerator). No
// network access ever occurs.

namespace {
constexpr int kGridSize = 4;
constexpr int kExpectedCount = (kGridSize + 1) * (kGridSize + 1);
constexpr int kFineZoom = 14;  ///< patch span ~2.4km, well inside the 11km test regions

TileMath::TileKey keyOver(const QGeoCoordinate& coord, int zoom)
{
    return TileMath::tileForWorld(TileMath::geoToWorld(coord), zoom);
}
}  // namespace

void TerrainHeightSourceTest::_deliversFlatRegionHeights()
{
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), kGridSize);
    QCOMPARE_GT(requestId, 0);
    QVERIFY(readySpy.isEmpty());  // delivery is async, never re-entrant

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    QVERIFY(failedSpy.isEmpty());

    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation));
    }
}

void TerrainHeightSourceTest::_deliversSlopeRegionHeights()
{
    TerrainHeightSource source;
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
    }
}

void TerrainHeightSourceTest::_interpolatesAcrossCellSteps()
{
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    // Vertices a few meters apart on the slope: the raw 1-arc-second grid steps
    // ~2m per ~20m cell, so without bilinear interpolation adjacent vertices
    // would show whole-cell jumps instead of a smooth ~0.3m-per-vertex rise
    constexpr int closeZoom = 19;  // patch ~50m ground span at this latitude
    constexpr int fineGrid = 16;
    source.requestPatchHeights(keyOver(linearSlopeRegion().center(), closeZoom), fineGrid);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());

    const auto heights = readySpy.first().at(1).value<QList<float>>();
    const int verticesPerEdge = fineGrid + 1;
    QCOMPARE(heights.count(), verticesPerEdge * verticesPerEdge);
    for (int row = 0; row < verticesPerEdge; row++) {
        for (int col = 0; col < fineGrid; col++) {
            const float west = heights.at((row * verticesPerEdge) + col);
            const float east = heights.at((row * verticesPerEdge) + col + 1);
            QCOMPARE_LT(qAbs(east - west), 1.0f);
        }
        // Interpolation must not flatten the slope away
        const float rise = heights.at((row * verticesPerEdge) + fineGrid) - heights.at(row * verticesPerEdge);
        QCOMPARE_GT(rise, 3.0f);
    }
}

void TerrainHeightSourceTest::_sparseSamplingUpsamplesCoarsePatch()
{
    // Pins the sparse-sampling path deliberately: a zoom-13 patch spans
    // ~0.044 degrees, past the 0.04-degree sparse threshold, so a 16-cell
    // grid samples only 4x4 vertices and bilinearly upsamples to 17x17.
    // If the sparse threshold constant changes, revisit the zoom here so
    // this test keeps exercising the upsample.
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    constexpr int sparseZoom = TerrainHeightSource::kMinTerrainZoom + 1;  // zoom 13: patch fits inside the 11km region
    constexpr int fineGrid = 16;
    source.requestPatchHeights(keyOver(linearSlopeRegion().center(), sparseZoom), fineGrid);
    // Sparse fetch actually occurred: 4 cell samples per vertex over 4x4 sampled
    // vertices, not the 17x17 delivery grid (which would be 1156 coordinates)
    QCOMPARE(source.lastQueryCoordinateCount(), 4 * 4 * 4);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(failedSpy.isEmpty());

    // Full-size delivery despite the sparse fetch
    const auto heights = readySpy.first().at(1).value<QList<float>>();
    const int verticesPerEdge = fineGrid + 1;
    QCOMPARE(heights.count(), verticesPerEdge * verticesPerEdge);

    // The slope rises 100m/km west to east: every row must preserve a
    // substantial monotonic rise through the upsample (a row/column
    // transpose would flatten rows), with smooth interpolated steps
    for (int row = 0; row < verticesPerEdge; row++) {
        const float west = heights.at(row * verticesPerEdge);
        const float east = heights.at((row * verticesPerEdge) + fineGrid);
        QCOMPARE_GT(east - west, 100.0f);
        for (int col = 0; col < fineGrid; col++) {
            const float step =
                heights.at((row * verticesPerEdge) + col + 1) - heights.at((row * verticesPerEdge) + col);
            QCOMPARE_GE(step, -1.0f);  // monotonic modulo data noise
            QCOMPARE_LT(step, 50.0f);  // no upsample discontinuities
        }
    }
}

void TerrainHeightSourceTest::_blendBandScalesHeights()
{
    // Zooms in the blend band deliver fractionally scaled heights so the
    // flat-zero far field is approached in steps instead of one cliff
    for (int zoom = TerrainHeightSource::kMinTerrainZoom; zoom <= TerrainHeightSource::kFullTerrainZoom; zoom++) {
        TerrainHeightSource source;
        QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

        source.requestPatchHeights(keyOver(flat10Region().center(), zoom), kGridSize);
        QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());

        const double scale = TerrainHeightSource::heightScaleForZoom(zoom);
        QCOMPARE_GT(scale, 0.0);
        const auto heights = readySpy.first().at(1).value<QList<float>>();
        QCOMPARE(heights.count(), kExpectedCount);
        // Coarse patches extend past the 11km test region, so only the center
        // vertex is guaranteed to sample the flat region's elevation
        const int center = (kGridSize / 2 * (kGridSize + 1)) + (kGridSize / 2);
        QCOMPARE(heights.at(center), static_cast<float>(UnitTestTerrainData::Flat10Region::amslElevation * scale));
    }
    QCOMPARE(TerrainHeightSource::heightScaleForZoom(TerrainHeightSource::kFullTerrainZoom), 1.0);
    QCOMPARE(TerrainHeightSource::heightScaleForZoom(TerrainHeightSource::kMinTerrainZoom - 1), 0.0);
}

void TerrainHeightSourceTest::_coarseZoomDeliversFlatZeros()
{
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    // Below the terrain zoom threshold the source answers locally with zeros,
    // even over a region that has real (10m) elevation data
    const int requestId = source.requestPatchHeights(
        keyOver(flat10Region().center(), TerrainHeightSource::kMinTerrainZoom - 1), kGridSize);
    QVERIFY(readySpy.wait());
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);

    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, 0.0f);
    }
}

void TerrainHeightSourceTest::_cancelPreventsDelivery()
{
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const TileMath::TileKey key = keyOver(flat10Region().center(), kFineZoom);
    const int cancelledId = source.requestPatchHeights(key, kGridSize);
    source.cancelRequest(cancelledId);
    const int liveId = source.requestPatchHeights(key, kGridSize);

    // Identical requests resolve through the same tile path in order: once the
    // live request delivers, the cancelled one would already have been delivered
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(readySpy.first().at(0).toInt(), liveId);
    QVERIFY(failedSpy.isEmpty());
}

void TerrainHeightSourceTest::_invalidGridSizeFails()
{
    TerrainHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(keyOver(flat10Region().center(), kFineZoom), 0);
    QVERIFY(failedSpy.wait());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

UT_REGISTER_TEST(TerrainHeightSourceTest, TestLabel::Integration, TestLabel::Terrain)
