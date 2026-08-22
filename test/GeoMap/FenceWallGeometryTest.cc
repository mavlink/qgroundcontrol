#include "FenceWallGeometryTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include <array>
#include <cmath>
#include <cstring>

#include "FenceWallGeometry.h"
#include "GeoScene.h"
#include "TileMath.h"

namespace {

constexpr int kFloatsPerVertex = 3;
constexpr qreal kWallHeight = 121.92;

// Near-equator: unanchored GeoScene has verticalScale == 1, so z offsets are
// meters
const QGeoCoordinate kOrigin(0.001, 0.001);

std::array<float, kFloatsPerVertex> vertexAt(const QByteArray& data, int index)
{
    std::array<float, kFloatsPerVertex> vertex{};
    std::memcpy(vertex.data(), data.constData() + (static_cast<qsizetype>(index) * sizeof(vertex)), sizeof(vertex));
    return vertex;
}

QVariantList makeRing(const QList<QGeoCoordinate>& coordinates)
{
    QVariantList ring;
    for (const QGeoCoordinate& coordinate : coordinates) {
        ring.append(QVariant::fromValue(coordinate));
    }
    return ring;
}

QList<QGeoCoordinate> squareRing(double sideMeters)
{
    const QGeoCoordinate second = kOrigin.atDistanceAndAzimuth(sideMeters, 90);
    return {kOrigin, second, second.atDistanceAndAzimuth(sideMeters, 0), kOrigin.atDistanceAndAzimuth(sideMeters, 0)};
}

// Rebuilds are queued to coalesce burst triggers
void processRebuild()
{
    QCoreApplication::processEvents();
}

}  // namespace

void FenceWallGeometryTest::_emptyAndDisabledStates()
{
    GeoScene scene;
    FenceWallGeometry geometry;
    geometry.setScene(&scene);
    geometry.setHeightAglMeters(kWallHeight);
    processRebuild();
    QCOMPARE(geometry.vertexData().size(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());

    // Invalid coordinates are dropped; fewer than 3 valid vertices is no ring
    geometry.setCoordinates(makeRing({kOrigin, kOrigin.atDistanceAndAzimuth(80, 90), QGeoCoordinate()}));
    processRebuild();
    QCOMPARE(geometry.vertexData().size(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());

    // A valid ring with zero height builds nothing
    geometry.setCoordinates(makeRing(squareRing(80)));
    geometry.setHeightAglMeters(0.0);
    processRebuild();
    QCOMPARE(geometry.vertexData().size(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());

    // No scene: no geometry even with a valid ring and height
    FenceWallGeometry sceneless;
    sceneless.setHeightAglMeters(kWallHeight);
    sceneless.setCoordinates(makeRing(squareRing(80)));
    processRebuild();
    QCOMPARE(sceneless.vertexData().size(), 0);
    QVERIFY(!sceneless.anchorCoordinate().isValid());
}

void FenceWallGeometryTest::_wallLayout()
{
    GeoScene scene;
    FenceWallGeometry geometry;
    geometry.setScene(&scene);
    geometry.setHeightAglMeters(kWallHeight);
    geometry.setCoordinates(makeRing(squareRing(80)));
    processRebuild();

    QCOMPARE(geometry.primitiveType(), QQuick3DGeometry::PrimitiveType::TriangleStrip);
    QCOMPARE(geometry.stride(), kFloatsPerVertex * static_cast<int>(sizeof(float)));
    QCOMPARE(geometry.attributeCount(), 1);
    const QQuick3DGeometry::Attribute position = geometry.attribute(0);
    QCOMPARE(position.semantic, QQuick3DGeometry::Attribute::PositionSemantic);
    QCOMPARE(position.offset, 0);
    QCOMPARE(position.componentType, QQuick3DGeometry::Attribute::F32Type);

    // 80 m edges subdivide into 2 steps: 4 edges * 2 points, plus the closing
    // point, 2 vertices (bottom/top) each
    const QByteArray data = geometry.vertexData();
    QCOMPARE(data.size(), 9 * 2 * kFloatsPerVertex * static_cast<int>(sizeof(float)));

    // Anchor is the first ring vertex at terrain altitude (0 without a
    // surface model)
    QCOMPARE(geometry.anchorCoordinate(), QGeoCoordinate(kOrigin.latitude(), kOrigin.longitude(), 0.0));

    // First pair sits at the origin, bottom then top
    QCOMPARE(vertexAt(data, 0)[0], 0.0f);
    QCOMPARE(vertexAt(data, 0)[1], 0.0f);
    QCOMPARE(vertexAt(data, 0)[2], 0.0f);
    QCOMPARE(vertexAt(data, 1)[0], 0.0f);
    QCOMPARE(vertexAt(data, 1)[1], 0.0f);
    QCOMPARE(vertexAt(data, 1)[2], static_cast<float>(kWallHeight));

    // Strip closes back onto the first pair
    for (int component = 0; component < kFloatsPerVertex; component++) {
        QCOMPARE(vertexAt(data, 16)[component], vertexAt(data, 0)[component]);
        QCOMPARE(vertexAt(data, 17)[component], vertexAt(data, 1)[component]);
    }

    // Flat terrain: z bounds span exactly the wall height
    QCOMPARE(geometry.boundsMin().z(), 0.0f);
    QCOMPARE(geometry.boundsMax().z(), static_cast<float>(kWallHeight));

    // Horizontal positions match the double-precision mercator offsets: point
    // 1 is the 40 m subdivision midpoint of the first edge
    const QPointF expected = TileMath::geoToWorld(kOrigin.atDistanceAndAzimuth(40, 90)) - TileMath::geoToWorld(kOrigin);
    QCOMPARE_LT(std::abs(vertexAt(data, 2)[0] - expected.x()), 1e-3);
    QCOMPARE_LT(std::abs(vertexAt(data, 2)[1] - expected.y()), 1e-3);
}

void FenceWallGeometryTest::_ringNormalization()
{
    GeoScene scene;
    const QList<QGeoCoordinate> ring = squareRing(80);

    FenceWallGeometry open;
    open.setScene(&scene);
    open.setHeightAglMeters(kWallHeight);
    open.setCoordinates(makeRing(ring));

    // A pre-closed ring (circleCoordinates output) builds identical geometry
    FenceWallGeometry closed;
    closed.setScene(&scene);
    closed.setHeightAglMeters(kWallHeight);
    QList<QGeoCoordinate> closedRing = ring;
    closedRing.append(ring.first());
    closed.setCoordinates(makeRing(closedRing));

    // Interleaved invalid coordinates are filtered out
    FenceWallGeometry filtered;
    filtered.setScene(&scene);
    filtered.setHeightAglMeters(kWallHeight);
    QList<QGeoCoordinate> dirtyRing = ring;
    dirtyRing.insert(2, QGeoCoordinate());
    filtered.setCoordinates(makeRing(dirtyRing));

    processRebuild();
    QCOMPARE_GT(open.vertexData().size(), 0);
    QCOMPARE(closed.vertexData(), open.vertexData());
    QCOMPARE(closed.anchorCoordinate(), open.anchorCoordinate());
    QCOMPARE(filtered.vertexData(), open.vertexData());
}

void FenceWallGeometryTest::_subdivisionCap()
{
    GeoScene scene;
    FenceWallGeometry geometry;
    geometry.setScene(&scene);
    geometry.setHeightAglMeters(kWallHeight);

    // 5 km edges all exceed the cap: 64 points per edge, plus the closing
    // point
    const QGeoCoordinate second = kOrigin.atDistanceAndAzimuth(5000, 90);
    geometry.setCoordinates(makeRing({kOrigin, second, second.atDistanceAndAzimuth(5000, 0)}));
    processRebuild();
    QCOMPARE(geometry.vertexData().size(), ((3 * 64) + 1) * 2 * kFloatsPerVertex * static_cast<int>(sizeof(float)));
}

void FenceWallGeometryTest::_deferredCoalescedRebuild()
{
    GeoScene scene;
    FenceWallGeometry geometry;
    geometry.setScene(&scene);
    processRebuild();

    // Property changes don't rebuild until the event loop turns
    geometry.setHeightAglMeters(kWallHeight);
    geometry.setCoordinates(makeRing(squareRing(80)));
    QCOMPARE(geometry.vertexData().size(), 0);
    QVERIFY(!geometry.anchorCoordinate().isValid());
    processRebuild();
    QCOMPARE_GT(geometry.vertexData().size(), 0);
    QVERIFY(geometry.anchorCoordinate().isValid());

    // Multiple triggers in one turn coalesce into a single rebuild: only the
    // final ring's anchor is ever published
    QSignalSpy anchorSpy(&geometry, &FenceWallGeometry::anchorCoordinateChanged);
    const QGeoCoordinate transient = kOrigin.atDistanceAndAzimuth(1000, 45);
    const QGeoCoordinate settled = kOrigin.atDistanceAndAzimuth(2000, 180);
    geometry.setCoordinates(
        makeRing({transient, transient.atDistanceAndAzimuth(80, 90), transient.atDistanceAndAzimuth(80, 0)}));
    geometry.setHeightAglMeters(kWallHeight * 2);
    geometry.setCoordinates(
        makeRing({settled, settled.atDistanceAndAzimuth(80, 90), settled.atDistanceAndAzimuth(80, 0)}));
    processRebuild();
    QCOMPARE(anchorSpy.count(), 1);
    QCOMPARE(geometry.anchorCoordinate(), QGeoCoordinate(settled.latitude(), settled.longitude(), 0.0));
}

UT_REGISTER_TEST_LIGHTWEIGHT(FenceWallGeometryTest, TestLabel::Unit)
