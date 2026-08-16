#include "FlightPathGeometryTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>

#include <cmath>

#include "FlightPathGeometry.h"
#include "GeoScene.h"
#include "TileMath.h"

namespace {

// Float offsets within a vertex: position 3, tangent 3, side flag 2
constexpr int kTangentOffset = 3;
constexpr int kSideOffset = 6;

// Near-equator coordinates: unanchored GeoScene has verticalScale == 1, so z
// offsets equal altitude differences exactly
const QGeoCoordinate kStart(0.001, 0.001, 100.0);

const float* vertexAt(const QByteArray& data, int index)
{
    return reinterpret_cast<const float*>(data.constData()) + (index * FlightPathGeometry::kFloatsPerVertex);
}

QVariantList makePath(const QList<QGeoCoordinate>& coordinates)
{
    QVariantList path;
    for (const QGeoCoordinate& coordinate : coordinates) {
        path.append(QVariant::fromValue(coordinate));
    }
    return path;
}

}  // namespace

void FlightPathGeometryTest::_emptyAndShortPaths()
{
    GeoScene scene;
    FlightPathGeometry geometry;
    geometry.setScene(&scene);

    QCOMPARE(geometry.vertexData().size(), 0);
    QCOMPARE(geometry.pointCount(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());

    // A single point has no ribbon yet but anchors the item
    geometry.setPath(makePath({kStart}));
    QCOMPARE(geometry.vertexData().size(), 0);
    QCOMPARE(geometry.pointCount(), 1);
    QCOMPARE(geometry.anchorCoordinate(), kStart);

    // Invalid coordinates are dropped
    geometry.setPath(makePath({kStart, QGeoCoordinate()}));
    QCOMPARE(geometry.pointCount(), 1);
    QCOMPARE(geometry.vertexData().size(), 0);

    // No scene: no geometry even with enough points
    FlightPathGeometry sceneless;
    sceneless.setPath(makePath({kStart, kStart.atDistanceAndAzimuth(100, 90)}));
    QCOMPARE(sceneless.pointCount(), 2);
    QCOMPARE(sceneless.vertexData().size(), 0);
}

void FlightPathGeometryTest::_ribbonLayout()
{
    GeoScene scene;
    FlightPathGeometry geometry;
    geometry.setScene(&scene);

    const QGeoCoordinate second = kStart.atDistanceAndAzimuth(100, 90);
    QGeoCoordinate third = second.atDistanceAndAzimuth(100, 90);
    third.setAltitude(kStart.altitude() + 25.0);
    geometry.setPath(makePath({kStart, second, third}));

    QCOMPARE(geometry.primitiveType(), QQuick3DGeometry::PrimitiveType::TriangleStrip);
    QCOMPARE(geometry.stride(), FlightPathGeometry::kFloatsPerVertex * static_cast<int>(sizeof(float)));
    QCOMPARE(geometry.vertexData().size(),
             3 * 2 * FlightPathGeometry::kFloatsPerVertex * static_cast<int>(sizeof(float)));
    QCOMPARE(geometry.attributeCount(), 3);

    const QQuick3DGeometry::Attribute position = geometry.attribute(0);
    QCOMPARE(position.semantic, QQuick3DGeometry::Attribute::PositionSemantic);
    QCOMPARE(position.offset, 0);
    QCOMPARE(position.componentType, QQuick3DGeometry::Attribute::F32Type);

    const QQuick3DGeometry::Attribute tangent = geometry.attribute(1);
    QCOMPARE(tangent.semantic, QQuick3DGeometry::Attribute::NormalSemantic);
    QCOMPARE(tangent.offset, kTangentOffset * static_cast<int>(sizeof(float)));
    QCOMPARE(tangent.componentType, QQuick3DGeometry::Attribute::F32Type);

    const QQuick3DGeometry::Attribute side = geometry.attribute(2);
    QCOMPARE(side.semantic, QQuick3DGeometry::Attribute::TexCoord0Semantic);
    QCOMPARE(side.offset, kSideOffset * static_cast<int>(sizeof(float)));
    QCOMPARE(side.componentType, QQuick3DGeometry::Attribute::F32Type);

    const QByteArray data = geometry.vertexData();

    // Anchor-relative: the first pair sits at the origin
    QCOMPARE(vertexAt(data, 0)[0], 0.0f);
    QCOMPARE(vertexAt(data, 0)[1], 0.0f);
    QCOMPARE(vertexAt(data, 0)[2], 0.0f);

    // Both vertices of a pair share the centerline position
    for (int point = 0; point < 3; point++) {
        const float* const left = vertexAt(data, point * 2);
        const float* const right = vertexAt(data, (point * 2) + 1);
        for (int component = 0; component < kSideOffset; component++) {
            QCOMPARE(left[component], right[component]);
        }
    }

    // z offsets are altitude differences (verticalScale == 1 near the equator)
    QCOMPARE(vertexAt(data, 2)[2], 0.0f);
    QCOMPARE(vertexAt(data, 4)[2], 25.0f);
    QCOMPARE(geometry.boundsMax().z(), 25.0f);
    QCOMPARE(geometry.boundsMin().z(), 0.0f);

    // Horizontal positions match the double-precision mercator offsets
    const QPointF expected = TileMath::geoToWorld(second) - TileMath::geoToWorld(kStart);
    QCOMPARE_LT(std::abs(vertexAt(data, 2)[0] - expected.x()), 1e-3);
    QCOMPARE_LT(std::abs(vertexAt(data, 2)[1] - expected.y()), 1e-3);
}

void FlightPathGeometryTest::_directionsAndSides()
{
    GeoScene scene;
    FlightPathGeometry geometry;
    geometry.setScene(&scene);

    // A bend: east then north
    const QGeoCoordinate corner = kStart.atDistanceAndAzimuth(100, 90);
    geometry.setPath(makePath({kStart, corner, corner.atDistanceAndAzimuth(100, 0)}));
    const QByteArray data = geometry.vertexData();

    for (int point = 0; point < 3; point++) {
        const float* const vertex = vertexAt(data, point * 2);
        const QVector3D tangent(vertex[kTangentOffset], vertex[kTangentOffset + 1], vertex[kTangentOffset + 2]);
        QCOMPARE_LT(std::abs(tangent.length() - 1.0f), 1e-5f);

        // Side flags alternate -1 / +1 within each pair
        QCOMPARE(vertexAt(data, point * 2)[kSideOffset], -1.0f);
        QCOMPARE(vertexAt(data, (point * 2) + 1)[kSideOffset], 1.0f);
    }

    // Endpoint tangents follow their single adjacent segment; the corner
    // blends both
    const auto segmentDirection = [&data](int fromPoint, int toPoint) {
        const float* const from = vertexAt(data, fromPoint * 2);
        const float* const to = vertexAt(data, toPoint * 2);
        return QVector3D(to[0] - from[0], to[1] - from[1], to[2] - from[2]).normalized();
    };
    const auto tangentAt = [&data](int point) {
        const float* const vertex = vertexAt(data, point * 2);
        return QVector3D(vertex[kTangentOffset], vertex[kTangentOffset + 1], vertex[kTangentOffset + 2]);
    };
    QCOMPARE_LT((tangentAt(0) - segmentDirection(0, 1)).length(), 1e-5f);
    QCOMPARE_LT((tangentAt(2) - segmentDirection(1, 2)).length(), 1e-5f);
    const QVector3D blended = (segmentDirection(0, 1) + segmentDirection(1, 2)).normalized();
    QCOMPARE_LT((tangentAt(1) - blended).length(), 1e-5f);

    // Straight vertical climb keeps a valid vertical tangent (a horizontal
    // perpendicular here rendered the ribbon edge-on and invisible)
    QGeoCoordinate above = kStart;
    above.setAltitude(kStart.altitude() + 50.0);
    geometry.setPath(makePath({kStart, above}));
    const QByteArray vertical = geometry.vertexData();
    const float* const climb = vertexAt(vertical, 0);
    QCOMPARE(climb[kTangentOffset], 0.0f);
    QCOMPARE(climb[kTangentOffset + 1], 0.0f);
    QCOMPARE(climb[kTangentOffset + 2], 1.0f);
}

void FlightPathGeometryTest::_pathMutations()
{
    GeoScene scene;
    FlightPathGeometry geometry;
    geometry.setScene(&scene);

    geometry.appendPoint(kStart);
    QCOMPARE(geometry.anchorCoordinate(), kStart);
    QCOMPARE(geometry.vertexData().size(), 0);

    geometry.appendPoint(kStart.atDistanceAndAzimuth(100, 90));
    QCOMPARE(geometry.pointCount(), 2);
    QCOMPARE(geometry.vertexData().size(),
             2 * 2 * FlightPathGeometry::kFloatsPerVertex * static_cast<int>(sizeof(float)));

    // Colinear updates rewrite the last pair in place
    QGeoCoordinate moved = kStart.atDistanceAndAzimuth(200, 90);
    moved.setAltitude(kStart.altitude() + 10.0);
    geometry.updateLastPoint(moved);
    QCOMPARE(geometry.pointCount(), 2);
    QCOMPARE(vertexAt(geometry.vertexData(), 2)[2], 10.0f);

    // Invalid input is ignored
    geometry.appendPoint(QGeoCoordinate());
    geometry.updateLastPoint(QGeoCoordinate());
    QCOMPARE(geometry.pointCount(), 2);

    geometry.clearPath();
    QCOMPARE(geometry.pointCount(), 0);
    QCOMPARE(geometry.vertexData().size(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());
}

void FlightPathGeometryTest::_incrementalMatchesRebuild()
{
    GeoScene scene;
    FlightPathGeometry incremental;
    incremental.setScene(&scene);

    // Mirror every mutation into a coordinate list for the reference rebuild
    QList<QGeoCoordinate> coordinates;
    const auto append = [&](const QGeoCoordinate& coordinate) {
        coordinates.append(coordinate);
        incremental.appendPoint(coordinate);
    };
    const auto updateLast = [&](const QGeoCoordinate& coordinate) {
        coordinates.last() = coordinate;
        incremental.updateLastPoint(coordinate);
    };

    append(kStart);
    append(kStart.atDistanceAndAzimuth(100, 90));
    updateLast(kStart.atDistanceAndAzimuth(150, 90));  // colinear extension
    QGeoCoordinate climb = coordinates.last();
    climb.setAltitude(kStart.altitude() + 40.0);
    append(climb);                              // straight vertical climb
    append(climb);                              // repeated point: degenerate segment
    append(climb.atDistanceAndAzimuth(80, 0));  // bend north
    QGeoCoordinate lowered = coordinates.last();
    lowered.setAltitude(kStart.altitude() + 20.0);
    updateLast(lowered);  // re-tilts the last segment and its neighbor's blend

    FlightPathGeometry reference;
    reference.setScene(&scene);
    reference.setPath(makePath(coordinates));

    QCOMPARE(incremental.pointCount(), reference.pointCount());
    QCOMPARE(incremental.vertexData(), reference.vertexData());
    QCOMPARE(incremental.boundsMin(), reference.boundsMin());
    QCOMPARE(incremental.boundsMax(), reference.boundsMax());
}

UT_REGISTER_TEST_LIGHTWEIGHT(FlightPathGeometryTest, TestLabel::Unit)
