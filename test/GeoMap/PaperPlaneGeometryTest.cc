#include "PaperPlaneGeometryTest.h"

#include <QtGui/QVector3D>

#include <cmath>

#include "PaperPlaneGeometry.h"

namespace {

// Float offsets within a vertex: position 3, normal 3, color 4
constexpr int kNormalOffset = 3;
constexpr int kColorOffset = 6;
constexpr int kAlphaOffset = 9;

const float* vertexAt(const QByteArray& data, int index)
{
    return reinterpret_cast<const float*>(data.constData()) + (index * PaperPlaneGeometry::kFloatsPerVertex);
}

}  // namespace

void PaperPlaneGeometryTest::_meshLayout()
{
    PaperPlaneGeometry geometry;

    QCOMPARE(geometry.primitiveType(), QQuick3DGeometry::PrimitiveType::Triangles);
    QCOMPARE(geometry.stride(), PaperPlaneGeometry::kFloatsPerVertex * static_cast<int>(sizeof(float)));
    QCOMPARE(geometry.vertexData().size(), PaperPlaneGeometry::kTriangleCount * 3 *
                                               PaperPlaneGeometry::kFloatsPerVertex * static_cast<int>(sizeof(float)));
    QCOMPARE(geometry.attributeCount(), 3);

    const QQuick3DGeometry::Attribute position = geometry.attribute(0);
    QCOMPARE(position.semantic, QQuick3DGeometry::Attribute::PositionSemantic);
    QCOMPARE(position.offset, 0);
    QCOMPARE(position.componentType, QQuick3DGeometry::Attribute::F32Type);

    const QQuick3DGeometry::Attribute normal = geometry.attribute(1);
    QCOMPARE(normal.semantic, QQuick3DGeometry::Attribute::NormalSemantic);
    QCOMPARE(normal.offset, kNormalOffset * static_cast<int>(sizeof(float)));
    QCOMPARE(normal.componentType, QQuick3DGeometry::Attribute::F32Type);

    const QQuick3DGeometry::Attribute color = geometry.attribute(2);
    QCOMPARE(color.semantic, QQuick3DGeometry::Attribute::ColorSemantic);
    QCOMPARE(color.offset, kColorOffset * static_cast<int>(sizeof(float)));
    QCOMPARE(color.componentType, QQuick3DGeometry::Attribute::F32Type);

    // Nose forward (+y), symmetric wingspan, dart proportions
    QCOMPARE(geometry.boundsMax().y(), 50.0f);
    QCOMPARE(geometry.boundsMin().y(), -50.0f);
    QCOMPARE(geometry.boundsMax().x(), -geometry.boundsMin().x());
    QCOMPARE_GT(geometry.boundsMax().z(), 0.0f);
    QCOMPARE_LT(geometry.boundsMin().z(), 0.0f);
}

void PaperPlaneGeometryTest::_normalsAndColors()
{
    PaperPlaneGeometry geometry;
    const QByteArray data = geometry.vertexData();

    for (int triangle = 0; triangle < PaperPlaneGeometry::kTriangleCount; triangle++) {
        const float* const first = vertexAt(data, triangle * 3);
        for (int corner = 0; corner < 3; corner++) {
            const float* const vertex = vertexAt(data, (triangle * 3) + corner);
            const QVector3D normal(vertex[kNormalOffset], vertex[kNormalOffset + 1], vertex[kNormalOffset + 2]);
            QCOMPARE_LT(std::abs(normal.length() - 1.0f), 1e-5f);
            // Flat shading and per-face color: all corners identical
            for (int component = kNormalOffset; component < PaperPlaneGeometry::kFloatsPerVertex; component++) {
                QCOMPARE(vertex[component], first[component]);
            }
            QCOMPARE(vertex[kAlphaOffset], 1.0f);  // opaque
        }
    }

    // Wings face up, keel faces sideways
    QCOMPARE_GT(vertexAt(data, 0)[kNormalOffset + 2], 0.0f);
    QCOMPARE_GT(vertexAt(data, 3)[kNormalOffset + 2], 0.0f);
    const float* const keel = vertexAt(data, 2 * 3);
    QCOMPARE(keel[kNormalOffset + 2], 0.0f);
    QCOMPARE_GT(std::abs(keel[kNormalOffset]), 0.9f);

    // Left wing darker than the right (icon's two red shades); white keel
    // fuselage brighter than both
    QCOMPARE_LT(vertexAt(data, 0)[kColorOffset], vertexAt(data, 3)[kColorOffset]);
    QCOMPARE_GT(keel[kColorOffset + 1], vertexAt(data, 3)[kColorOffset + 1]);
}

UT_REGISTER_TEST_LIGHTWEIGHT(PaperPlaneGeometryTest, TestLabel::Unit)
