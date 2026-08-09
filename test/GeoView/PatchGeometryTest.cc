#include "PatchGeometryTest.h"

#include <cmath>

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

UT_REGISTER_TEST(PatchGeometryTest, TestLabel::Unit)
