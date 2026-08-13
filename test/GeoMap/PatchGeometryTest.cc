#include "PatchGeometryTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include <memory>

#include "HeightField.h"
#include "PatchGeometry.h"

namespace {

constexpr int kGrid = 4;
constexpr float kSpan = 1000.0f;
constexpr int kFloatsPerVertex = 8;  // position 3, normal 3, uv 2

int gridVertexCount(int gridSize)
{
    return (gridSize + 1) * (gridSize + 1);
}

int skirtVertexCount(int gridSize)
{
    return 4 * (gridSize + 1);
}

const float* vertexAt(const QByteArray& data, int index)
{
    return reinterpret_cast<const float*>(data.constData()) + (index * kFloatsPerVertex);
}

QList<float> rampHeights(int gridSize)
{
    // Height = 10 * row: north edge at 0, south edge highest
    QList<float> heights;
    for (int row = 0; row <= gridSize; row++) {
        for (int col = 0; col <= gridSize; col++) {
            heights.append(10.0f * row);
        }
    }
    return heights;
}

ElevationTilePyramid::Grid uniformGrid(float height)
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    grid.heights = QList<float>(16, height);
    return grid;
}

/// Linear in both axes so bilinear sampling reproduces the plane and edge
/// vertices carry distinct values (uniform grids couldn't catch edge bugs)
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

/// Quadratic along y: fine-LOD edge samples deviate from the coarse edge's
/// linear segments, so unstitched edges produce detectable T-junction gaps
/// (a linear field would make stitched and unstitched edges identical)
ElevationTilePyramid::Grid quadraticGrid()
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            grid.heights.append(float(row * row * 40));
        }
    }
    return grid;
}

/// Quadratic along x, linear along y: samples vary along a north/south edge
/// (catches along-axis mistakes) and rows differ (catches edge-side flips)
ElevationTilePyramid::Grid quadraticGridX()
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            grid.heights.append(float((col * col * 40) + (row * 100)));
        }
    }
    return grid;
}

}  // namespace

void PatchGeometryTest::_flatMeshLayout()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);

    const QByteArray vertexData = geometry.vertexData();
    const int expectedVertices = gridVertexCount(kGrid) + skirtVertexCount(kGrid);
    QCOMPARE(vertexData.size(), expectedVertices * kFloatsPerVertex * static_cast<int>(sizeof(float)));

    const QByteArray indexData = geometry.indexData();
    const int expectedIndices = (kGrid * kGrid * 6) + (4 * kGrid * 6);
    QCOMPARE(indexData.size(), expectedIndices * static_cast<int>(sizeof(quint32)));

    // All indices reference valid vertices
    const quint32* indices = reinterpret_cast<const quint32*>(indexData.constData());
    for (int i = 0; i < expectedIndices; i++) {
        QCOMPARE_LT(indices[i], static_cast<quint32>(expectedVertices));
    }
}

void PatchGeometryTest::_cornerPositionsAndUVs()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    const QByteArray data = geometry.vertexData();

    // Vertex 0 is the north-west corner: (-span/2, +span/2), uv (0,0)
    const float* nw = vertexAt(data, 0);
    QCOMPARE(nw[0], -kSpan / 2);
    QCOMPARE(nw[1], kSpan / 2);
    QCOMPARE(nw[2], 0.0f);
    QCOMPARE(nw[6], 0.0f);
    QCOMPARE(nw[7], 0.0f);

    // Last grid vertex is the south-east corner: (+span/2, -span/2), uv (1,1)
    const float* se = vertexAt(data, gridVertexCount(kGrid) - 1);
    QCOMPARE(se[0], kSpan / 2);
    QCOMPARE(se[1], -kSpan / 2);
    QCOMPARE(se[6], 1.0f);
    QCOMPARE(se[7], 1.0f);
}

void PatchGeometryTest::_heightDisplacement()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeights(rampHeights(kGrid));
    const QByteArray data = geometry.vertexData();

    // Row-major from NW: vertex (row, col) has z = 10 * row
    for (int row = 0; row <= kGrid; row++) {
        for (int col = 0; col <= kGrid; col++) {
            const float* vertex = vertexAt(data, (row * (kGrid + 1)) + col);
            QCOMPARE(vertex[2], 10.0f * row);
        }
    }
}

void PatchGeometryTest::_mismatchedHeightsRenderFlat()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setHeights(QList<float>{1.0f, 2.0f, 3.0f});  // wrong count
    const QByteArray data = geometry.vertexData();

    for (int i = 0; i < gridVertexCount(kGrid); i++) {
        QCOMPARE(vertexAt(data, i)[2], 0.0f);
    }
}

void PatchGeometryTest::_skirtVerticesBelowSurface()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeights(rampHeights(kGrid));
    const QByteArray data = geometry.vertexData();

    const float skirtDepth = kSpan * static_cast<float>(PatchGeometry::kSkirtDepthFraction);
    // First skirt vertex duplicates the NW corner, dropped by skirtDepth
    const float* nwSkirt = vertexAt(data, gridVertexCount(kGrid));
    const float* nw = vertexAt(data, 0);
    QCOMPARE(nwSkirt[0], nw[0]);
    QCOMPARE(nwSkirt[1], nw[1]);
    QCOMPARE(nwSkirt[2], nw[2] - skirtDepth);
    // Same uv/normal as the edge vertex (no seams)
    QCOMPARE(nwSkirt[6], nw[6]);
    QCOMPARE(nwSkirt[7], nw[7]);
}

void PatchGeometryTest::_flatNormalsPointUp()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    const QByteArray data = geometry.vertexData();

    for (int i = 0; i < gridVertexCount(kGrid); i++) {
        const float* vertex = vertexAt(data, i);
        QCOMPARE(vertex[3], 0.0f);
        QCOMPARE(vertex[4], 0.0f);
        QCOMPARE(vertex[5], 1.0f);
    }
}

void PatchGeometryTest::_boundsIncludeSkirt()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeights(rampHeights(kGrid));

    const float skirtDepth = kSpan * static_cast<float>(PatchGeometry::kSkirtDepthFraction);
    QCOMPARE(geometry.boundsMin().x(), -kSpan / 2);
    QCOMPARE(geometry.boundsMax().x(), kSpan / 2);
    QCOMPARE(geometry.boundsMin().z(), 0.0f - skirtDepth);
    QCOMPARE(geometry.boundsMax().z(), 10.0f * kGrid);
}

void PatchGeometryTest::_gridSizeClamped()
{
    PatchGeometry geometry;
    geometry.setGridSize(0);
    QCOMPARE(geometry.gridSize(), PatchGeometry::kMinGridSize);
    geometry.setGridSize(100000);
    QCOMPARE(geometry.gridSize(), PatchGeometry::kMaxGridSize);
}

void PatchGeometryTest::_sampleFromFieldDisplacesVertices()
{
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(100.0f)));

    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeightField(&field);
    QVERIFY(geometry.sampleFromField(TileMath::TileKey{5, 6, 3}));

    const QByteArray data = geometry.vertexData();
    for (int i = 0; i < gridVertexCount(kGrid); i++) {
        QCOMPARE(vertexAt(data, i)[2], 100.0f);
    }
}

void PatchGeometryTest::_sampleFromFieldWithoutFieldWarns()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);

    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^sampleFromField rejected:"));
    QVERIFY(!geometry.sampleFromField(TileMath::TileKey{5, 6, 3}));
    verifyExpectedLogMessage();

    // With a field set, an invalid key is rejected by both layers
    HeightField field;
    geometry.setHeightField(&field);
    expectLogMessage("GeoMap.HeightField", QtWarningMsg, QRegularExpression("^samplePatch rejected:"));
    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^sampleFromField rejected:"));
    QVERIFY(!geometry.sampleFromField(TileMath::TileKey{0, 0, -1}));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void PatchGeometryTest::_adjacentPatchesShareEdgeHeights()
{
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, gradientGrid()));

    PatchGeometry west;
    west.setGridSize(kGrid);
    west.setSpan(kSpan);
    west.setHeightField(&field);
    QVERIFY(west.sampleFromField(TileMath::TileKey{5, 6, 3}));

    PatchGeometry east;
    east.setGridSize(kGrid);
    east.setSpan(kSpan);
    east.setHeightField(&field);
    QVERIFY(east.sampleFromField(TileMath::TileKey{6, 6, 3}));

    // Core drape invariant: the shared edge samples identical world positions
    // in the same field, so the meshed heights must match bit-for-bit
    const QByteArray westData = west.vertexData();
    const QByteArray eastData = east.vertexData();
    for (int row = 0; row <= kGrid; row++) {
        const float westEdgeZ = vertexAt(westData, (row * (kGrid + 1)) + kGrid)[2];
        const float eastEdgeZ = vertexAt(eastData, row * (kGrid + 1))[2];
        QCOMPARE(westEdgeZ, eastEdgeZ);
    }

    // Sanity: the gradient varies along the edge, so the comparison is real
    QCOMPARE_NE(vertexAt(westData, kGrid)[2], vertexAt(westData, (kGrid * (kGrid + 1)) + kGrid)[2]);
}

void PatchGeometryTest::_resampleAfterFieldGainsData()
{
    HeightField field;
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeightField(&field);
    QSignalSpy spy(&geometry, &PatchGeometry::heightsChanged);
    QVERIFY(spy.isValid());

    // Empty field: sampling succeeds with the zero best-estimate everywhere
    const TileMath::TileKey key{5, 6, 3};
    QVERIFY(geometry.sampleFromField(key));
    const QByteArray flatData = geometry.vertexData();
    for (int i = 0; i < gridVertexCount(kGrid); i++) {
        QCOMPARE(vertexAt(flatData, i)[2], 0.0f);
    }
    const int emptySampleSignals = spy.count();

    // Field gains data: resampling picks it up and notifies
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(100.0f)));
    QVERIFY(geometry.sampleFromField(key));
    const QByteArray sampledData = geometry.vertexData();
    for (int i = 0; i < gridVertexCount(kGrid); i++) {
        QCOMPARE(vertexAt(sampledData, i)[2], 100.0f);
    }
    QCOMPARE_GT(spy.count(), emptySampleSignals);
}

void PatchGeometryTest::_stitchedEdgeLiesOnCoarseSegments()
{
    // Fine patch {11,12,4} is the NE quarter of {5,6,3}: its east edge is the
    // upper half of coarse neighbor {6,6,3}'s west edge. The z3 tile under
    // the coarse patch is quadratic in y, so the fine edge bows away from the
    // coarse edge's linear segments unless stitched.
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));
    QVERIFY(field.insertTile(TileMath::TileKey{6, 6, 3}, quadraticGrid()));

    PatchGeometry coarse;
    coarse.setGridSize(kGrid);
    coarse.setSpan(kSpan);
    coarse.setHeightField(&field);
    QVERIFY(coarse.sampleFromField(TileMath::TileKey{6, 6, 3}));

    PatchGeometry fine;
    fine.setGridSize(kGrid);
    fine.setSpan(kSpan);
    fine.setHeightField(&field);
    fine.setEdgeLodDeltas(0, 0, 0, 1);  // east neighbor renders one level coarser
    QVERIFY(fine.sampleFromField(TileMath::TileKey{11, 12, 4}));

    const QByteArray fineData = fine.vertexData();
    const QByteArray coarseData = coarse.vertexData();
    const auto fineEastZ = [&](int row) { return vertexAt(fineData, (row * (kGrid + 1)) + kGrid)[2]; };
    const auto coarseWestZ = [&](int row) { return vertexAt(coarseData, row * (kGrid + 1))[2]; };

    // Fine row r maps to coarse row r/2 (fine edge covers the coarse edge's
    // north half). Even rows coincide with coarse vertices (corners included);
    // odd rows must lie exactly on the coarse segment between them.
    for (int row = 0; row <= kGrid; row += 2) {
        QCOMPARE(fineEastZ(row), coarseWestZ(row / 2));
    }
    for (int row = 1; row <= kGrid; row += 2) {
        const float a = coarseWestZ((row - 1) / 2);
        const float b = coarseWestZ((row + 1) / 2);
        QCOMPARE(fineEastZ(row), a + ((b - a) * 0.5f));
    }

    // Sanity: the quadratic field actually bows, so stitching changed values
    PatchGeometry unstitched;
    unstitched.setGridSize(kGrid);
    unstitched.setSpan(kSpan);
    unstitched.setHeightField(&field);
    QVERIFY(unstitched.sampleFromField(TileMath::TileKey{11, 12, 4}));
    const QByteArray unstitchedData = unstitched.vertexData();
    QCOMPARE_NE(vertexAt(unstitchedData, (1 * (kGrid + 1)) + kGrid)[2], fineEastZ(1));
}

void PatchGeometryTest::_stitchedNorthEdgeLiesOnCoarseRenderedRow()
{
    // Fine patch {11,12,4} sits in the top row of parent {5,6,3}: its north
    // neighbor {11,11,4} renders one level coarser as {5,5,3}, whose rendered
    // SOUTH row canonically resolves to {5,6,3}'s north row (the south side
    // of the shared line wins) — the same backing the fine patch's own north
    // samples come from. That backing is quadratic along x, so unstitched
    // odd columns bow away from the coarse row's linear segments.
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));
    QVERIFY(field.insertTile(TileMath::TileKey{5, 6, 3}, quadraticGridX()));

    PatchGeometry coarse;
    coarse.setGridSize(kGrid);
    coarse.setSpan(kSpan);
    coarse.setHeightField(&field);
    QVERIFY(coarse.sampleFromField(TileMath::TileKey{5, 5, 3}));

    PatchGeometry fine;
    fine.setGridSize(kGrid);
    fine.setSpan(kSpan);
    fine.setHeightField(&field);
    fine.setEdgeLodDeltas(1, 0, 0, 0);  // north neighbor renders one level coarser
    QVERIFY(fine.sampleFromField(TileMath::TileKey{11, 12, 4}));

    const QByteArray fineData = fine.vertexData();
    const QByteArray coarseData = coarse.vertexData();
    const auto fineNorthZ = [&](int col) { return vertexAt(fineData, col)[2]; };
    const auto coarseSouthZ = [&](int col) { return vertexAt(coarseData, (kGrid * (kGrid + 1)) + col)[2]; };

    // Fine col c maps to coarse col 2 + c/2; even cols coincide with coarse
    // vertices, odd cols must lie exactly on the coarse segment between them
    for (int col = 0; col <= kGrid; col += 2) {
        QCOMPARE(fineNorthZ(col), coarseSouthZ(2 + (col / 2)));
    }
    for (int col = 1; col <= kGrid; col += 2) {
        const float a = coarseSouthZ(2 + ((col - 1) / 2));
        const float b = coarseSouthZ(2 + ((col + 1) / 2));
        QCOMPARE(fineNorthZ(col), a + ((b - a) * 0.5f));
    }

    // Sanity: the shared line carries real data, not the flat root
    QCOMPARE_NE(fineNorthZ(0), 0.0f);
}

void PatchGeometryTest::_stitchAppliesToAllFourEdges()
{
    // Heights quadratic in both directions so every edge bows away from its
    // linear segments; all four deltas active at once
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    QList<float> heights;
    for (int row = 0; row <= kGrid; row++) {
        for (int col = 0; col <= kGrid; col++) {
            heights.append(((row * row) + (col * col)) * 10.0f);
        }
    }
    geometry.setHeights(heights);
    geometry.setEdgeLodDeltas(1, 1, 1, 1);

    const QByteArray data = geometry.vertexData();
    const auto z = [&](int row, int col) { return vertexAt(data, (row * (kGrid + 1)) + col)[2]; };
    const auto raw = [&](int row, int col) { return heights.at((row * (kGrid + 1)) + col); };
    const auto lerpMid = [&](float a, float b) { return a + ((b - a) * 0.5f); };

    for (int i = 1; i < kGrid; i += 2) {
        QCOMPARE(z(0, i), lerpMid(raw(0, i - 1), raw(0, i + 1)));              // north
        QCOMPARE(z(kGrid, i), lerpMid(raw(kGrid, i - 1), raw(kGrid, i + 1)));  // south
        QCOMPARE(z(i, 0), lerpMid(raw(i - 1, 0), raw(i + 1, 0)));              // west
        QCOMPARE(z(i, kGrid), lerpMid(raw(i - 1, kGrid), raw(i + 1, kGrid)));  // east
        QCOMPARE_NE(z(0, i), raw(0, i));                                       // sanity: stitching actually moved it
    }

    // Interior vertices are untouched
    QCOMPARE(z(1, 1), raw(1, 1));
    QCOMPARE(z(2, 3), raw(2, 3));
}

void PatchGeometryTest::_stitchInvalidDeltaWarns()
{
    PatchGeometry geometry;
    geometry.setGridSize(kGrid);

    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^setEdgeLodDeltas rejected:"));
    geometry.setEdgeLodDeltas(-1, 0, 0, 0);  // negative delta
    verifyExpectedLogMessage();

    // 2^3 = 8 does not divide gridSize 4: coincident vertices would not exist
    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^setEdgeLodDeltas rejected:"));
    geometry.setEdgeLodDeltas(0, 0, 0, 3);
    verifyExpectedLogMessage();

    // Rejected calls leave the mesh unconstrained: ramp heights pass through
    geometry.setHeights(rampHeights(kGrid));
    const QByteArray data = geometry.vertexData();
    for (int row = 0; row <= kGrid; row++) {
        QCOMPARE(vertexAt(data, (row * (kGrid + 1)) + kGrid)[2], 10.0f * row);
    }
}

void PatchGeometryTest::_declarativeHeightsStitchLikeSampleFromField()
{
    // The QML render path binds heights (from the patch model) and edge
    // deltas as independent declarative properties — it never calls
    // sampleFromField. The resulting mesh must match the imperative
    // sampleFromField path bit for bit: the model heights already carry
    // canonical edge values, so no field or key context is needed.
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));
    QVERIFY(field.insertTile(TileMath::TileKey{6, 6, 3}, quadraticGrid()));

    PatchGeometry reference;
    reference.setGridSize(kGrid);
    reference.setSpan(kSpan);
    reference.setHeightField(&field);
    reference.setEdgeLodDeltas(0, 0, 0, 1);
    QVERIFY(reference.sampleFromField(TileMath::TileKey{11, 12, 4}));

    PatchGeometry declarative;
    declarative.setGridSize(kGrid);
    declarative.setSpan(kSpan);
    declarative.setHeights(field.samplePatch(TileMath::TileKey{11, 12, 4}, kGrid));
    declarative.setEdgeLodDeltas(0, 0, 0, 1);
    QCOMPARE(declarative.vertexData(), reference.vertexData());
}

void PatchGeometryTest::_gridSizeChangeResetsInvalidDeltas()
{
    PatchGeometry geometry;
    geometry.setGridSize(8);
    geometry.setEdgeLodDeltas(0, 0, 0, 3);  // valid: 2^3 divides 8

    // Shrinking the grid strands the delta: it must be reset with a warning
    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^setGridSize reset edge LOD deltas"));
    geometry.setGridSize(kGrid);
    verifyExpectedLogMessage();

    // A still-valid delta survives a grid size change
    geometry.setEdgeLodDeltas(0, 0, 0, 1);
    geometry.setGridSize(8);  // 2^1 divides 8: no reset, no warning
    QList<float> heights;
    for (int row = 0; row <= 8; row++) {
        for (int col = 0; col <= 8; col++) {
            heights.append(float(row * row));
        }
    }
    geometry.setHeights(heights);
    const QByteArray data = geometry.vertexData();
    const auto eastZ = [&](int row) { return vertexAt(data, (row * 9) + 8)[2]; };
    QCOMPARE(eastZ(1), eastZ(0) + ((eastZ(2) - eastZ(0)) * 0.5f));  // stitched, not raw 1.0f
}

void PatchGeometryTest::_edgeLodDeltasListProperty()
{
    PatchGeometry viaList;
    viaList.setGridSize(8);
    viaList.setHeights(rampHeights(8));
    QSignalSpy changeSpy(&viaList, &PatchGeometry::edgeLodDeltasChanged);

    // The QML-bindable list form matches the 4-arg setter exactly
    viaList.setEdgeLodDeltas(QList<int>{0, 1, 0, 1});
    QCOMPARE(viaList.edgeLodDeltas(), (QList<int>{0, 1, 0, 1}));
    QCOMPARE(changeSpy.count(), 1);

    PatchGeometry viaArgs;
    viaArgs.setGridSize(8);
    viaArgs.setHeights(rampHeights(8));
    viaArgs.setEdgeLodDeltas(0, 1, 0, 1);
    QCOMPARE(viaList.vertexData(), viaArgs.vertexData());

    // Wrong-size list is rejected with a warning and changes nothing
    expectLogMessage("GeoMap.PatchGeometry", QtWarningMsg, QRegularExpression("^setEdgeLodDeltas rejected:"));
    viaList.setEdgeLodDeltas(QList<int>{1, 1, 1});
    verifyExpectedLogMessage();
    QCOMPARE(viaList.edgeLodDeltas(), (QList<int>{0, 1, 0, 1}));
    QCOMPARE(changeSpy.count(), 1);
}

void PatchGeometryTest::_replacingLiveFieldDisconnectsOld()
{
    // Switching from one live field to another must disconnect the old one,
    // including its destroyed handler — otherwise deleting the abandoned
    // field would null out the geometry's now-current field.
    auto oldField = std::make_unique<HeightField>();
    QVERIFY(oldField->insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));
    HeightField newField;
    QVERIFY(newField.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));

    PatchGeometry geometry;
    geometry.setGridSize(kGrid);
    geometry.setSpan(kSpan);
    geometry.setHeightField(oldField.get());
    geometry.setHeightField(&newField);
    QCOMPARE(geometry.heightField(), &newField);

    // Deleting the abandoned field must leave the current one in place
    oldField.reset();
    QCOMPARE(geometry.heightField(), &newField);

    // The current field is still usable for sampling
    QVERIFY(newField.insertTile(TileMath::TileKey{6, 6, 3}, quadraticGrid()));
    geometry.setEdgeLodDeltas(0, 0, 0, 1);
    QVERIFY(geometry.sampleFromField(TileMath::TileKey{11, 12, 4}));
}

void PatchGeometryTest::_fieldSetterIgnoresSameValue()
{
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, uniformGrid(0.0f)));

    PatchGeometry geometry;
    geometry.setHeightField(&field);

    QSignalSpy fieldSpy(&geometry, &PatchGeometry::heightFieldChanged);
    QVERIFY(fieldSpy.isValid());

    // Re-setting the current value is a no-op: no notify signal
    geometry.setHeightField(&field);
    QCOMPARE(fieldSpy.count(), 0);

    // A real change still fires
    geometry.setHeightField(nullptr);
    QCOMPARE(fieldSpy.count(), 1);
}

void PatchGeometryTest::_skirtDepthScalesWithCoarsestNeighbor()
{
    // Skirts hide the seam against a coarser LOD neighbor. That seam grows
    // with the neighbor's cell size, so the skirt must deepen with the
    // coarsest constraining delta — doubling per level, tracking the coarser
    // level's geometric error. An unconstrained patch
    // keeps the base depth (span x kSkirtDepthFraction).
    const auto skirtDepthOf = [](const PatchGeometry& g) {
        // NW corner (index 0) sits at z=0 for a flat patch; the first skirt
        // vertex duplicates it dropped by the skirt depth
        const QByteArray data = g.vertexData();
        return vertexAt(data, 0)[2] - vertexAt(data, gridVertexCount(kGrid))[2];
    };

    const float base = kSpan * static_cast<float>(PatchGeometry::kSkirtDepthFraction);

    PatchGeometry unconstrained;
    unconstrained.setGridSize(kGrid);
    unconstrained.setSpan(kSpan);
    QCOMPARE(skirtDepthOf(unconstrained), base);

    PatchGeometry oneCoarser;
    oneCoarser.setGridSize(kGrid);
    oneCoarser.setSpan(kSpan);
    oneCoarser.setEdgeLodDeltas(1, 0, 0, 0);  // north neighbor one level coarser
    QCOMPARE(skirtDepthOf(oneCoarser), base * 2.0f);

    PatchGeometry twoCoarser;
    twoCoarser.setGridSize(kGrid);
    twoCoarser.setSpan(kSpan);
    twoCoarser.setEdgeLodDeltas(0, 0, 0, 2);  // east neighbor two levels coarser (2^2 | 4)
    QCOMPARE(skirtDepthOf(twoCoarser), base * 4.0f);

    // Multiple non-zero edges: the *max* delta drives scaling, not sum or
    // first-non-zero (delta 2 needs 2^2 | kGrid, so max delta is 2 here)
    PatchGeometry mixed;
    mixed.setGridSize(kGrid);
    mixed.setSpan(kSpan);
    mixed.setEdgeLodDeltas(1, 2, 0, 0);  // max 2 -> base*4 (sum 3 would be base*8, first-nonzero base*2)
    QCOMPARE(skirtDepthOf(mixed), base * 4.0f);
}

UT_REGISTER_TEST(PatchGeometryTest, TestLabel::Unit)
