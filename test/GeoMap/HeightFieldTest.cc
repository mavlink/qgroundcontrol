#include "HeightFieldTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include <algorithm>
#include <cmath>

#include "Benchmarking.h"
#include "HeightField.h"

using namespace TileMath;

namespace {

/// 4x4 gradient grid h(row, col) = col*10 + row*40: linear in both axes, so
/// bilinear sampling must reproduce the plane exactly (uniform grids couldn't
/// catch sub-window or axis-swap errors)
ElevationTilePyramid::Grid gradientGrid()
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            grid.heights.append(float((col * 10) + (row * 40)));
        }
    }
    return grid;
}

ElevationTilePyramid::Grid uniformGrid(float height)
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    grid.heights = QList<float>(16, height);
    return grid;
}

/// Transposed gradient h(row, col) = col*40 + row*10: deliberately disagrees
/// with gradientGrid() everywhere off the diagonal, so a patch clamping its
/// own tile at a shared edge cannot accidentally match its neighbor
ElevationTilePyramid::Grid transposedGradientGrid()
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            grid.heights.append(float((col * 40) + (row * 10)));
        }
    }
    return grid;
}

/// World position of a patch vertex, row-major from the NW corner
QPointF vertexWorld(const TileKey& key, int gridSize, int row, int col)
{
    const QPointF corner = tileMinCorner(key);
    const double span = tileSpanAtZoom(key.zoom);
    return QPointF(corner.x() + (span * col / gridSize), corner.y() + (span * (gridSize - row) / gridSize));
}

}  // namespace

void HeightFieldTest::_zeroWhenEmpty()
{
    const HeightField field;
    QCOMPARE(field.tileCount(), 0);
    QCOMPARE(field.heightAt(QPointF(0, 0)), 0.0);
    QCOMPARE(field.heightAt(QPointF(worldSize() / 4, -worldSize() / 4)), 0.0);

    // Patches still mesh on an empty field: full-size, all-zero drape
    const QList<float> heights = field.samplePatch(TileKey{5, 6, 3}, 4);
    QCOMPARE(heights.size(), 25);
    for (const float h : heights) {
        QCOMPARE(h, 0.0f);
    }
}

void HeightFieldTest::_invalidRequests()
{
    ignoreLogMessage("GeoMap.HeightField", QtWarningMsg, QRegularExpression("^(insertTile|samplePatch) rejected:"));

    HeightField field;
    QVERIFY(!field.insertTile(TileKey{0, 0, -1}, gradientGrid()));
    QVERIFY(!field.insertTile(TileKey{0, 0, 0}, ElevationTilePyramid::Grid{}));
    QCOMPARE(field.tileCount(), 0);

    QVERIFY(field.samplePatch(TileKey{5, 6, 3}, 0).isEmpty());
    QVERIFY(field.samplePatch(TileKey{5, 6, 3}, -1).isEmpty());
    QVERIFY(field.samplePatch(TileKey{5, 6, 3}, HeightField::kMaxGridSize + 1).isEmpty());
    QVERIFY(field.samplePatch(TileKey{0, 0, -1}, 4).isEmpty());
    QVERIFY(field.samplePatch(TileKey{0, 0, kMaxZoom + 1}, 4).isEmpty());
    QVERIFY(field.samplePatch(TileKey{-3, 0, 3}, 4).isEmpty());
    QVERIFY(field.samplePatch(TileKey{0, 8, 3}, 4).isEmpty());
}

void HeightFieldTest::_exactValuesAtSampleCenters()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));

    // Sample center of grid cell (col 2, row 1) in the z0 tile: u=(2+0.5)/4,
    // v=(1+0.5)/4 from the NW corner
    const double span = worldSize();
    const double half = span / 2.0;
    const QPointF world((0.625 * span) - half, half - (0.375 * span));
    QCOMPARE(field.heightAt(world), 60.0);  // 2*10 + 1*40
}

void HeightFieldTest::_bilinearBetweenCenters()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));

    // Halfway between the (col 1, row 1) and (col 2, row 1) centers:
    // px = 1.5, py = 1.0 on the plane col*10 + row*40
    const double span = worldSize();
    const double half = span / 2.0;
    const QPointF world((0.5 * span) - half, half - (0.375 * span));
    QCOMPARE(field.heightAt(world), 55.0);
}

void HeightFieldTest::_clampAtTileEdges()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));

    // Beyond the outermost sample centers the height clamps to the edge value
    const double half = worldSize() / 2.0;
    QCOMPARE(field.heightAt(QPointF(-half, half)), 0.0);    // NW corner: h(0,0)
    QCOMPARE(field.heightAt(QPointF(half, -half)), 150.0);  // SE corner: h(3,3)
}

void HeightFieldTest::_finestTileWins()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, uniformGrid(100.0f)));
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(200.0f)));

    // Inside the fine tile: fine data; outside it: the z0 estimate
    const QPointF insideFine = vertexWorld(TileKey{5, 6, 3}, 2, 1, 1);
    const QPointF outsideFine = vertexWorld(TileKey{1, 1, 3}, 2, 1, 1);
    QCOMPARE(field.heightAt(insideFine), 200.0);
    QCOMPARE(field.heightAt(outsideFine), 100.0);
}

void HeightFieldTest::_pinnedTilesSurviveInsertPressure()
{
    // Tiles backing rendered patches are pinned: no amount of insert
    // pressure may evict them, or a visible mesh re-samples coarser data
    // next to an intact neighbor — a cliff
    HeightField field;
    const TileKey fineKey{5, 6, 3};
    QVERIFY(field.insertTile(fineKey, uniformGrid(200.0f)));
    field.setPinnedKeys({fineKey});

    for (int i = 0; i < ElevationTilePyramid::kMaxTiles + 8; i++) {
        QVERIFY(field.insertTile(TileKey{i, 100, 8}, uniformGrid(50.0f)));
    }

    QVERIFY(field.hasTile(fineKey));
    QCOMPARE(field.heightAt(vertexWorld(fineKey, 2, 1, 1)), 200.0);
}

void HeightFieldTest::_evictionEmitsRegionChanged()
{
    // Eviction changes what the field answers over the evicted region, so it
    // must notify like any other data change: consumers re-mesh to the
    // coarser estimate instead of rendering stale fine samples next to
    // freshly sampled neighbors.
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, uniformGrid(100.0f)));
    const int fineZoom = 8;
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles - 1; i++) {
        QVERIFY(field.insertTile(TileKey{i, 100, fineZoom}, uniformGrid(200.0f)));
    }
    QCOMPARE(field.tileCount(), ElevationTilePyramid::kMaxTiles);

    // The next insert evicts the least-recently-used tile (the z0 root):
    // its region must be announced alongside the inserted tile's own
    QSignalSpy regionSpy(&field, &HeightField::regionChanged);
    QVERIFY(field.insertTile(TileKey{200, 100, fineZoom}, uniformGrid(200.0f)));
    QCOMPARE(field.tileCount(), ElevationTilePyramid::kMaxTiles);
    bool evictedRegionAnnounced = false;
    const QPointF probe = vertexWorld(TileKey{50, 50, fineZoom}, 2, 1, 1);  // far from any fine tile
    for (const QList<QVariant>& args : regionSpy) {
        if (args.first().toRectF().contains(probe)) {
            evictedRegionAnnounced = true;
        }
    }
    QVERIFY2(evictedRegionAnnounced, "no regionChanged covering the evicted tile's region");
}

void HeightFieldTest::_sharedEdgeIdentity()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));

    // Two patches sharing a vertical edge: east column of A and west column
    // of B sample identical world positions, so heights must match exactly
    const int gridSize = 4;
    const QList<float> a = field.samplePatch(TileKey{5, 6, 3}, gridSize);
    const QList<float> b = field.samplePatch(TileKey{6, 6, 3}, gridSize);
    QCOMPARE(a.size(), 25);
    QCOMPARE(b.size(), 25);
    for (int row = 0; row <= gridSize; row++) {
        QCOMPARE(a[(row * (gridSize + 1)) + gridSize], b[row * (gridSize + 1)]);
    }
}

void HeightFieldTest::_sharedEdgeIdentityAcrossBackingTiles()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, gradientGrid()));
    QVERIFY(field.insertTile(TileKey{6, 6, 3}, transposedGradientGrid()));

    // Adjacent exact tiles with disagreeing data: both patches must resolve
    // the shared edge canonically (east side owns it), or A clamps its own
    // last pixel column while B reads its first — a crack
    const int gridSize = 4;
    const QList<float> a = field.samplePatch(TileKey{5, 6, 3}, gridSize);
    const QList<float> b = field.samplePatch(TileKey{6, 6, 3}, gridSize);
    for (int row = 0; row <= gridSize; row++) {
        QCOMPARE_EQ(a[(row * (gridSize + 1)) + gridSize], b[row * (gridSize + 1)]);
    }
}

void HeightFieldTest::_sharedEdgeIdentityFineNextToAncestorBacked()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));
    QVERIFY(field.insertTile(TileKey{6, 6, 3}, transposedGradientGrid()));

    // Patch A {5,6,3} is ancestor-backed, neighbor B {6,6,3} has its exact
    // tile: their shared edge must still sample one canonical grid
    const int gridSize = 4;
    const QList<float> a = field.samplePatch(TileKey{5, 6, 3}, gridSize);
    const QList<float> b = field.samplePatch(TileKey{6, 6, 3}, gridSize);
    for (int row = 0; row <= gridSize; row++) {
        QCOMPARE_EQ(a[(row * (gridSize + 1)) + gridSize], b[row * (gridSize + 1)]);
    }
}

void HeightFieldTest::_crossZoomVertexIdentity()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, gradientGrid()));

    // Child patch {10,12,4} is the NW quarter of {5,6,3} and its half-density
    // grid lands exactly on parent vertices (r,c) for r,c in 0..2: coincident
    // positions must sample identical heights across the LOD boundary
    const QList<float> parent = field.samplePatch(TileKey{5, 6, 3}, 4);
    const QList<float> child = field.samplePatch(TileKey{10, 12, 4}, 2);

    for (int row = 0; row <= 2; row++) {
        for (int col = 0; col <= 2; col++) {
            QCOMPARE(child[(row * 3) + col], parent[(row * 5) + col]);
        }
    }
}

void HeightFieldTest::_regionChangedOnInsert()
{
    HeightField field;
    QSignalSpy spy(&field, &HeightField::regionChanged);
    QVERIFY(spy.isValid());

    // Fine tile: rect is exactly the tile's world extent
    const TileKey fineKey{5, 6, 3};
    QVERIFY(field.insertTile(fineKey, uniformGrid(10.0f)));
    QCOMPARE(spy.count(), 1);
    const QPointF fineCorner = tileMinCorner(fineKey);
    const double fineSpan = tileSpanAtZoom(fineKey.zoom);
    QCOMPARE(spy[0][0].toRectF(), QRectF(fineCorner.x(), fineCorner.y(), fineSpan, fineSpan));

    // Ancestor tile: notifies its full (here: whole-world) area
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, uniformGrid(20.0f)));
    QCOMPARE(spy.count(), 2);
    const double world = worldSize();
    QCOMPARE(spy[1][0].toRectF(), QRectF(-world / 2.0, -world / 2.0, world, world));

    // Replacing a tile changes heights under the same area: must re-notify
    QVERIFY(field.insertTile(fineKey, uniformGrid(30.0f)));
    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy[2][0].toRectF(), spy[0][0].toRectF());
}

void HeightFieldTest::_noRegionChangedOnRejectedInsert()
{
    ignoreLogMessage("GeoMap.HeightField", QtWarningMsg, QRegularExpression("^insertTile rejected:"));

    HeightField field;
    QSignalSpy spy(&field, &HeightField::regionChanged);
    QVERIFY(spy.isValid());

    QVERIFY(!field.insertTile(TileKey{0, 0, -1}, uniformGrid(10.0f)));
    QVERIFY(!field.insertTile(TileKey{0, 0, 0}, ElevationTilePyramid::Grid{}));
    QCOMPARE(spy.count(), 0);
}

void HeightFieldTest::_heightAtMemoAvoidsRepeatLookups()
{
    HeightField field;
    const TileKey key{5, 6, 3};
    QVERIFY(field.insertTile(key, uniformGrid(50.0f)));

    // Memoization gate: the first heightAt resolves the backing tile and
    // primes the memo; further queries strictly inside the same tile must
    // answer from it without new pyramid lookups
    const int gridSize = 16;
    QCOMPARE(field.heightAt(vertexWorld(key, gridSize, 1, 1)), 50.0);
    const qint64 primed = field.lookupCountForTest();
    for (int row = 1; row < gridSize; row++) {
        for (int col = 1; col < gridSize; col++) {
            QCOMPARE(field.heightAt(vertexWorld(key, gridSize, row, col)), 50.0);
        }
    }
    QCOMPARE(field.lookupCountForTest(), primed);
}

void HeightFieldTest::_samplePatchLookupCountGateMixedZooms()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(50.0f)));

    // A deeper tile far from the sampled patch must not disable memoization:
    // only a descendant of the resolved tile could override its answer
    QVERIFY(field.insertTile(TileKey{0, 0, 5}, uniformGrid(80.0f)));

    const qint64 before = field.lookupCountForTest();
    const int gridSize = 16;
    const QList<float> heights = field.samplePatch(TileKey{5, 6, 3}, gridSize);
    QCOMPARE(heights.size(), (gridSize + 1) * (gridSize + 1));
    QCOMPARE_LE(field.lookupCountForTest() - before, 40);
}

void HeightFieldTest::_samplePatchLookupsBounded()
{
    // Interior vertices resolve the patch's backing view once; boundary
    // vertices resolve canonically by position through a per-tile memo, so
    // lookups are bounded by the few tiles the boundary touches (own plus
    // up to 8 neighbors), never per-vertex
    HeightField field;
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(50.0f)));

    const qint64 before = field.lookupCountForTest();
    const QList<float> heights = field.samplePatch(TileKey{5, 6, 3}, 16);
    QCOMPARE(heights.size(), 17 * 17);
    QCOMPARE_LE(field.lookupCountForTest() - before, 10);
}

void HeightFieldTest::_backingKeyFor()
{
    HeightField field;

    // Empty field: nothing backs any key
    QVERIFY(!TileMath::isValidKey(field.backingKeyFor(TileKey{5, 6, 3})));

    // Only an ancestor stored: it backs the descendant query
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, uniformGrid(1.0f)));
    QCOMPARE(field.backingKeyFor(TileKey{5, 6, 3}), (TileKey{0, 0, 0}));

    // The exact tile wins once stored
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(2.0f)));
    QCOMPARE(field.backingKeyFor(TileKey{5, 6, 3}), (TileKey{5, 6, 3}));

    // Siblings are unaffected: still the ancestor
    QCOMPARE(field.backingKeyFor(TileKey{6, 6, 3}), (TileKey{0, 0, 0}));
}

void HeightFieldTest::_memoInvalidatedOnInsert()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{0, 0, 0}, uniformGrid(100.0f)));

    // First query primes the memoized view on the coarse tile
    const QPointF p = vertexWorld(TileKey{5, 6, 3}, 2, 1, 1);
    QCOMPARE(field.heightAt(p), 100.0);

    // A finer tile over the same position must win immediately: the insert
    // invalidates the memoized coarse view
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(200.0f)));
    QCOMPARE(field.heightAt(p), 200.0);
}

void HeightFieldTest::_samplePatchBenchmark()
{
    HeightField field;
    QVERIFY(field.insertTile(TileKey{5, 6, 3}, uniformGrid(50.0f)));

    const TileKey key{5, 6, 3};
    QBENCHMARK
    {
        const QList<float> heights = field.samplePatch(key, 64);
        QT_BENCHMARK_KEEP(heights);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(HeightFieldTest, TestLabel::Unit)
