#include "Viewer3DTerrainGeometryTest.h"

#include <QtTest/QSignalSpy>

#include "Viewer3DTerrainGeometry.h"

void Viewer3DTerrainGeometryTest::_testComputeFaceNormal()
{
    Viewer3DTerrainGeometry geo;

    QVector3D x1(0, 0, 0);
    QVector3D x2(1, 0, 0);
    QVector3D x3(0, 1, 0);

    QVector3D normal = geo._computeFaceNormal(x1, x2, x3);

    QCOMPARE_FUZZY(normal.x(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.y(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.z(), 1.0f, 0.0001f);
}

void Viewer3DTerrainGeometryTest::_testComputeFaceNormalDegenerate()
{
    Viewer3DTerrainGeometry geo;

    // Collinear points — degenerate triangle
    QVector3D x1(0, 0, 0);
    QVector3D x2(1, 0, 0);
    QVector3D x3(2, 0, 0);

    QVector3D normal = geo._computeFaceNormal(x1, x2, x3);

    QCOMPARE_FUZZY(normal.x(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.y(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.z(), 0.0f, 0.0001f);
}

void Viewer3DTerrainGeometryTest::_testBuildTerrainZeroCounts()
{
    Viewer3DTerrainGeometry geo;

    geo.setSectorCount(0);
    geo.setStackCount(0);

    QGeoCoordinate roiMin(47.0, 8.0, 0);
    QGeoCoordinate roiMax(47.1, 8.1, 0);
    QGeoCoordinate ref(47.05, 8.05, 0);

    bool result = geo._buildTerrain(roiMin, roiMax, ref, true);
    QVERIFY(!result);
}

void Viewer3DTerrainGeometryTest::_testBuildTerrainValidRegion()
{
    Viewer3DTerrainGeometry geo;

    geo.setSectorCount(2);
    geo.setStackCount(2);

    QGeoCoordinate roiMin(47.0, 8.0, 0);
    QGeoCoordinate roiMax(47.01, 8.01, 0);
    QGeoCoordinate ref(47.005, 8.005, 0);

    bool result = geo._buildTerrain(roiMin, roiMax, ref, true);
    QVERIFY(result);

    QVERIFY(!geo._vertices.empty());
    QVERIFY(!geo._normals.empty());
    QVERIFY(!geo._texCoords.empty());
    QCOMPARE(geo._vertices.size(), geo._normals.size());
    QCOMPARE(geo._vertices.size(), geo._texCoords.size());
}

void Viewer3DTerrainGeometryTest::_testClearScene()
{
    Viewer3DTerrainGeometry geo;

    geo.setSectorCount(2);
    geo.setStackCount(2);

    QGeoCoordinate roiMin(47.0, 8.0, 0);
    QGeoCoordinate roiMax(47.01, 8.01, 0);
    QGeoCoordinate ref(47.005, 8.005, 0);

    geo._buildTerrain(roiMin, roiMax, ref, true);
    QVERIFY(!geo._vertices.empty());

    geo._clearScene();

    QVERIFY(geo._vertices.empty());
    QVERIFY(geo._normals.empty());
    QVERIFY(geo._texCoords.empty());
    QCOMPARE(geo.sectorCount(), 0);
    QCOMPARE(geo.stackCount(), 0);
}

void Viewer3DTerrainGeometryTest::_testComputeFaceNormalNegativeZ()
{
    Viewer3DTerrainGeometry geo;

    // Reversed winding: normal should point in -Z
    QVector3D x1(0, 0, 0);
    QVector3D x2(0, 1, 0);
    QVector3D x3(1, 0, 0);

    QVector3D normal = geo._computeFaceNormal(x1, x2, x3);

    QCOMPARE_FUZZY(normal.x(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.y(), 0.0f, 0.0001f);
    QCOMPARE_FUZZY(normal.z(), -1.0f, 0.0001f);
}

void Viewer3DTerrainGeometryTest::_testBuildTerrainVertexCountScaling()
{
    Viewer3DTerrainGeometry geo;

    QGeoCoordinate roiMin(47.0, 8.0, 0);
    QGeoCoordinate roiMax(47.01, 8.01, 0);
    QGeoCoordinate ref(47.005, 8.005, 0);

    // 2x2 grid
    geo.setSectorCount(2);
    geo.setStackCount(2);
    geo._buildTerrain(roiMin, roiMax, ref, true);
    const size_t count2x2 = geo._vertices.size();

    // 4x4 grid should produce more vertices
    geo._vertices.clear();
    geo._normals.clear();
    geo._texCoords.clear();
    geo.setSectorCount(4);
    geo.setStackCount(4);
    geo._buildTerrain(roiMin, roiMax, ref, true);
    const size_t count4x4 = geo._vertices.size();

    QVERIFY(count4x4 > count2x2);
}

void Viewer3DTerrainGeometryTest::_testRoiSetters()
{
    Viewer3DTerrainGeometry geo;

    {
        QSignalSpy spy(&geo, &Viewer3DTerrainGeometry::roiMinChanged);
        QGeoCoordinate min(47.0, 8.0, 0);
        geo.setRoiMin(min);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(geo.roiMin(), min);

        // Same value: no signal
        geo.setRoiMin(min);
        QCOMPARE(spy.count(), 1);
    }

    {
        QSignalSpy spy(&geo, &Viewer3DTerrainGeometry::roiMaxChanged);
        QGeoCoordinate max(47.1, 8.1, 0);
        geo.setRoiMax(max);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(geo.roiMax(), max);

        geo.setRoiMax(max);
        QCOMPARE(spy.count(), 1);
    }

    {
        QSignalSpy spy(&geo, &Viewer3DTerrainGeometry::refCoordinateChanged);
        QGeoCoordinate ref(47.05, 8.05, 0);
        geo.setRefCoordinate(ref);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(geo.refCoordinate(), ref);

        geo.setRefCoordinate(ref);
        QCOMPARE(spy.count(), 1);
    }
}

void Viewer3DTerrainGeometryTest::_testPropertySetters()
{
    Viewer3DTerrainGeometry geo;

    {
        QSignalSpy spy(&geo, &Viewer3DTerrainGeometry::sectorCountChanged);
        geo.setSectorCount(10);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(geo.sectorCount(), 10);

        geo.setSectorCount(10);
        QCOMPARE(spy.count(), 1);
    }

    {
        QSignalSpy spy(&geo, &Viewer3DTerrainGeometry::stackCountChanged);
        geo.setStackCount(20);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(geo.stackCount(), 20);

        geo.setStackCount(20);
        QCOMPARE(spy.count(), 1);
    }
}

UT_REGISTER_TEST(Viewer3DTerrainGeometryTest, TestLabel::Unit)
