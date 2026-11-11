#include "ElevationTilePyramidTest.h"

#include <QtCore/QSet>

#include "ElevationTilePyramid.h"

namespace {

/// Uniform-height grid, big enough to be a plausible tile
ElevationTilePyramid::Grid makeGrid(float height, int size = 4)
{
    ElevationTilePyramid::Grid grid;
    grid.width = size;
    grid.height = size;
    grid.heights = QList<float>(qsizetype(size) * size, height);
    return grid;
}

}  // namespace

void ElevationTilePyramidTest::_emptyPyramidHasNoView()
{
    const ElevationTilePyramid pyramid;
    QCOMPARE(pyramid.tileCount(), 0);
    QVERIFY(!pyramid.hasTile(TileMath::TileKey{0, 0, 0}));
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{5, 6, 3}).isValid());
}

void ElevationTilePyramidTest::_insertRejectsInvalidGrid()
{
    ElevationTilePyramid pyramid;
    const TileMath::TileKey key{1, 1, 1};

    QVERIFY(!pyramid.insertTile(key, ElevationTilePyramid::Grid{}));

    // Sample count inconsistent with dimensions
    ElevationTilePyramid::Grid bad = makeGrid(10.0f);
    bad.heights.removeLast();
    QVERIFY(!pyramid.insertTile(key, bad));

    // Zero rows with nonzero width
    ElevationTilePyramid::Grid zeroRows = makeGrid(10.0f);
    zeroRows.height = 0;
    QVERIFY(!pyramid.insertTile(key, zeroRows));

    QCOMPARE(pyramid.tileCount(), 0);
    QVERIFY(!pyramid.hasTile(key));
}

void ElevationTilePyramidTest::_rejectsInvalidKey()
{
    ElevationTilePyramid pyramid;
    QVERIFY(!pyramid.insertTile(TileMath::TileKey{0, 0, TileMath::kMaxZoom + 1}, makeGrid(1.0f)));
    QVERIFY(!pyramid.insertTile(TileMath::TileKey{0, 0, -1}, makeGrid(1.0f)));

    // x/y outside the slippy range for the zoom
    QVERIFY(!pyramid.insertTile(TileMath::TileKey{-1, 0, 3}, makeGrid(1.0f)));
    QVERIFY(!pyramid.insertTile(TileMath::TileKey{0, -1, 3}, makeGrid(1.0f)));
    QVERIFY(!pyramid.insertTile(TileMath::TileKey{0, 8, 3}, makeGrid(1.0f)));
    QCOMPARE(pyramid.tileCount(), 0);

    // Absurd zoom must fail cleanly, not walk the shift into UB
    QVERIFY(pyramid.insertTile(TileMath::TileKey{0, 0, 0}, makeGrid(1.0f)));
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{0, 0, TileMath::kMaxZoom + 1}).isValid());
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{0, 0, 500}).isValid());
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{0, 0, -1}).isValid());
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{-1, 0, 3}).isValid());
    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{8, 0, 3}).isValid());
}

void ElevationTilePyramidTest::_selfTileWins()
{
    ElevationTilePyramid pyramid;
    const TileMath::TileKey key{5, 6, 3};
    QVERIFY(pyramid.insertTile(TileMath::TileKey{1, 1, 1}, makeGrid(100.0f)));
    QVERIFY(pyramid.insertTile(key, makeGrid(200.0f)));
    QVERIFY(pyramid.hasTile(key));

    const ElevationTilePyramid::View view = pyramid.bestTileFor(key);
    QVERIFY(view.isValid());
    QCOMPARE(view.key, key);
    QCOMPARE(view.subWindow, QRectF(0, 0, 1, 1));
    QCOMPARE(view.grid->heights.first(), 200.0f);
}

void ElevationTilePyramidTest::_nearestAncestorWins()
{
    ElevationTilePyramid pyramid;
    // Ancestors of (5,6,3): (2,3,2), (1,1,1), (0,0,0)
    QVERIFY(pyramid.insertTile(TileMath::TileKey{0, 0, 0}, makeGrid(1.0f)));
    QVERIFY(pyramid.insertTile(TileMath::TileKey{1, 1, 1}, makeGrid(2.0f)));
    QVERIFY(pyramid.insertTile(TileMath::TileKey{2, 3, 2}, makeGrid(3.0f)));

    const ElevationTilePyramid::View view = pyramid.bestTileFor(TileMath::TileKey{5, 6, 3});
    QVERIFY(view.isValid());
    QCOMPARE(view.key, (TileMath::TileKey{2, 3, 2}));
    QCOMPARE(view.grid->heights.first(), 3.0f);
}

void ElevationTilePyramidTest::_noDescendantFallback()
{
    ElevationTilePyramid pyramid;
    // Child and unrelated sibling of the query — neither covers it
    QVERIFY(pyramid.insertTile(TileMath::TileKey{10, 12, 4}, makeGrid(1.0f)));
    QVERIFY(pyramid.insertTile(TileMath::TileKey{4, 6, 3}, makeGrid(2.0f)));

    QVERIFY(!pyramid.bestTileFor(TileMath::TileKey{5, 6, 3}).isValid());
}

void ElevationTilePyramidTest::_subWindowMath()
{
    ElevationTilePyramid pyramid;
    QVERIFY(pyramid.insertTile(TileMath::TileKey{1, 1, 1}, makeGrid(5.0f)));

    // (5,6,3) within ancestor (1,1,1): shift 2, quarter-tile window
    const ElevationTilePyramid::View view = pyramid.bestTileFor(TileMath::TileKey{5, 6, 3});
    QVERIFY(view.isValid());
    QCOMPARE(view.key, (TileMath::TileKey{1, 1, 1}));
    QCOMPARE(view.subWindow, QRectF(0.25, 0.5, 0.25, 0.25));
}

void ElevationTilePyramidTest::_subWindowDeepZoom()
{
    ElevationTilePyramid pyramid;
    const TileMath::TileKey ancestor{7, 2, 3};
    QVERIFY(pyramid.insertTile(ancestor, makeGrid(5.0f)));

    // 10 zoom levels deeper: NW-most descendant maps to a tiny window at the
    // ancestor's NW corner
    const int shift = 10;
    const double scale = 1.0 / (1 << shift);
    const TileMath::TileKey nwChild{7 << shift, 2 << shift, 3 + shift};
    const ElevationTilePyramid::View nwView = pyramid.bestTileFor(nwChild);
    QVERIFY(nwView.isValid());
    QCOMPARE(nwView.subWindow, QRectF(0, 0, scale, scale));

    // SE-most descendant maps to the far corner
    const TileMath::TileKey seChild{(8 << shift) - 1, (3 << shift) - 1, 3 + shift};
    const ElevationTilePyramid::View seView = pyramid.bestTileFor(seChild);
    QVERIFY(seView.isValid());
    QCOMPARE(seView.subWindow, QRectF(1.0 - scale, 1.0 - scale, scale, scale));
}

void ElevationTilePyramidTest::_insertReplacesTile()
{
    ElevationTilePyramid pyramid;
    const TileMath::TileKey key{5, 6, 3};
    QVERIFY(pyramid.insertTile(key, makeGrid(1.0f)));
    QVERIFY(pyramid.insertTile(key, makeGrid(2.0f)));

    QCOMPARE(pyramid.tileCount(), 1);
    QCOMPARE(pyramid.bestTileFor(key).grid->heights.first(), 2.0f);
}

void ElevationTilePyramidTest::_lruEvictionAtCap()
{
    ElevationTilePyramid pyramid;
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        QVERIFY(pyramid.insertTile(TileMath::TileKey{i, 0, 8}, makeGrid(float(i))));
    }
    QCOMPARE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles);

    // Touch the oldest tile so it is no longer least-recently-used
    QVERIFY(pyramid.bestTileFor(TileMath::TileKey{0, 0, 8}).isValid());

    // Inserting past the cap evicts the least-recently-used tile ({1,0,8})
    QVERIFY(pyramid.insertTile(TileMath::TileKey{200, 0, 8}, makeGrid(1.0f)));
    QCOMPARE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles);
    QVERIFY(pyramid.hasTile(TileMath::TileKey{0, 0, 8}));
    QVERIFY(!pyramid.hasTile(TileMath::TileKey{1, 0, 8}));
    QVERIFY(pyramid.hasTile(TileMath::TileKey{200, 0, 8}));

    // Replacing an existing key stays at the cap without evicting others
    QVERIFY(pyramid.insertTile(TileMath::TileKey{200, 0, 8}, makeGrid(2.0f)));
    QCOMPARE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles);
    QVERIFY(pyramid.hasTile(TileMath::TileKey{2, 0, 8}));
}

void ElevationTilePyramidTest::_pinnedTilesSurviveEviction()
{
    // Pinned tiles back rendered patches (and the ancestors resolving them):
    // evicting them yanks data out from under a visible mesh, so LRU pressure
    // must fall on unpinned tiles only — regardless of recency
    ElevationTilePyramid pyramid;
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        QVERIFY(pyramid.insertTile(TileMath::TileKey{i, 0, 8}, makeGrid(float(i))));
    }

    // Pin the two least-recently-used tiles
    pyramid.setPinnedKeys({TileMath::TileKey{0, 0, 8}, TileMath::TileKey{1, 0, 8}});

    QVERIFY(pyramid.insertTile(TileMath::TileKey{200, 0, 8}, makeGrid(1.0f)));
    QVERIFY(pyramid.insertTile(TileMath::TileKey{201, 0, 8}, makeGrid(1.0f)));
    QCOMPARE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles);
    QVERIFY(pyramid.hasTile(TileMath::TileKey{0, 0, 8}));
    QVERIFY(pyramid.hasTile(TileMath::TileKey{1, 0, 8}));
    QVERIFY(!pyramid.hasTile(TileMath::TileKey{2, 0, 8}));
    QVERIFY(!pyramid.hasTile(TileMath::TileKey{3, 0, 8}));

    // Unpinning makes them ordinary LRU victims again
    pyramid.setPinnedKeys({});
    QVERIFY(pyramid.insertTile(TileMath::TileKey{202, 0, 8}, makeGrid(1.0f)));
    QVERIFY(!pyramid.hasTile(TileMath::TileKey{0, 0, 8}));
}

void ElevationTilePyramidTest::_allPinnedGrowsPastCap()
{
    // kMaxTiles is a soft cap: when every resident tile is pinned, inserts
    // must still succeed (grow past the cap) rather than break a rendered
    // patch — the alternative is a cliff
    ElevationTilePyramid pyramid;
    QSet<TileMath::TileKey> pinned;
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        const TileMath::TileKey key{i, 0, 8};
        QVERIFY(pyramid.insertTile(key, makeGrid(1.0f)));
        pinned.insert(key);
    }
    pyramid.setPinnedKeys(pinned);

    QVERIFY(pyramid.insertTile(TileMath::TileKey{200, 0, 8}, makeGrid(1.0f)));
    QCOMPARE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles + 1);
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        QVERIFY(pyramid.hasTile(TileMath::TileKey{i, 0, 8}));
    }

    // Pressure releases once pins clear: the next insert trims back via LRU
    pyramid.setPinnedKeys({});
    QVERIFY(pyramid.insertTile(TileMath::TileKey{201, 0, 8}, makeGrid(1.0f)));
    QCOMPARE_LE(pyramid.tileCount(), ElevationTilePyramid::kMaxTiles + 1);
}

void ElevationTilePyramidTest::_descendantTracking()
{
    ElevationTilePyramid pyramid;

    // {20,24,5} sits under {5,6,3} via {10,12,4}: every ancestor sees it
    QVERIFY(pyramid.insertTile(TileMath::TileKey{20, 24, 5}, makeGrid(1.0f)));
    QVERIFY(pyramid.hasDescendant(TileMath::TileKey{10, 12, 4}));
    QVERIFY(pyramid.hasDescendant(TileMath::TileKey{5, 6, 3}));
    QVERIFY(pyramid.hasDescendant(TileMath::TileKey{0, 0, 0}));
    QVERIFY(!pyramid.hasDescendant(TileMath::TileKey{6, 6, 3}));    // sibling subtree
    QVERIFY(!pyramid.hasDescendant(TileMath::TileKey{20, 24, 5}));  // strictly deeper only
    QVERIFY(!pyramid.hasDescendant(TileMath::TileKey{40, 48, 6}));  // child of stored tile

    // Replacing a stored tile leaves the counts unchanged
    QVERIFY(pyramid.insertTile(TileMath::TileKey{20, 24, 5}, makeGrid(2.0f)));
    QVERIFY(pyramid.hasDescendant(TileMath::TileKey{5, 6, 3}));

    // Evicting the whole {0,0,1} subtree clears its descendant marks: fill
    // the cap under {0,0,1}, then displace it all with the {1,1,1} subtree
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        QVERIFY(pyramid.insertTile(TileMath::TileKey{i, 0, 8}, makeGrid(1.0f)));
    }
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles; i++) {
        QVERIFY(pyramid.insertTile(TileMath::TileKey{128 + i, 128, 8}, makeGrid(1.0f)));
    }
    QVERIFY(!pyramid.hasDescendant(TileMath::TileKey{0, 0, 1}));
    QVERIFY(pyramid.hasDescendant(TileMath::TileKey{1, 1, 1}));
}

UT_REGISTER_TEST_LIGHTWEIGHT(ElevationTilePyramidTest, TestLabel::Unit)
