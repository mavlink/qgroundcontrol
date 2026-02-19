#include "OsmParserTest.h"

#include <QtTest/QSignalSpy>

#include "OsmParser.h"
#include "OsmParserThread.h"

void OsmParserTest::_testBuildingToMeshEmpty()
{
    OsmParser parser;
    QByteArray mesh = parser.buildingToMesh();
    QVERIFY(mesh.isEmpty());
}

void OsmParserTest::_testTriangulateWalls()
{
    OsmParser parser;
    std::vector<QVector3D> mesh;
    std::vector<QVector2D> square = {QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), QVector2D(0, 1)};
    float height = 5.0f;

    parser._triangulateWallsExtrudedPolygon(mesh, square, height, false);

    // 4 walls × 1 rectangle each × 6 verts per rectangle = 24
    // Plus 1 extra inverted rectangle at the end = 24 + 6 = 30
    QCOMPARE(static_cast<int>(mesh.size()), 30);
}

void OsmParserTest::_testTriangulateWallsInverse()
{
    OsmParser parser;
    std::vector<QVector3D> meshNormal;
    std::vector<QVector3D> meshInverse;
    std::vector<QVector2D> square = {QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), QVector2D(0, 1)};
    float height = 5.0f;

    parser._triangulateWallsExtrudedPolygon(meshNormal, square, height, false);
    parser._triangulateWallsExtrudedPolygon(meshInverse, square, height, true);

    QCOMPARE(meshNormal.size(), meshInverse.size());

    // Winding order should differ — check that at least one vertex differs
    bool anyDifferent = false;
    for (size_t i = 0; i < meshNormal.size(); ++i) {
        if (meshNormal[i] != meshInverse[i]) {
            anyDifferent = true;
            break;
        }
    }
    QVERIFY(anyDifferent);
}

void OsmParserTest::_testTriangulateRectangle()
{
    OsmParser parser;
    std::vector<QVector3D> mesh;
    std::vector<QVector3D> rect = {QVector3D(0, 0, 0), QVector3D(1, 0, 0), QVector3D(1, 1, 0), QVector3D(0, 1, 0)};

    parser._triangulateRectangle(mesh, rect, false);
    QCOMPARE(static_cast<int>(mesh.size()), 6);
}

void OsmParserTest::_testTriangulateRectangleInverted()
{
    OsmParser parser;
    std::vector<QVector3D> meshNormal;
    std::vector<QVector3D> meshInverted;
    std::vector<QVector3D> rect = {QVector3D(0, 0, 0), QVector3D(1, 0, 0), QVector3D(1, 1, 0), QVector3D(0, 1, 0)};

    parser._triangulateRectangle(meshNormal, rect, false);
    parser._triangulateRectangle(meshInverted, rect, true);

    QCOMPARE(meshNormal.size(), meshInverted.size());

    // Inverted normal reverses winding — vertices should differ in order
    bool anyDifferent = false;
    for (size_t i = 0; i < meshNormal.size(); ++i) {
        if (meshNormal[i] != meshInverted[i]) {
            anyDifferent = true;
            break;
        }
    }
    QVERIFY(anyDifferent);
}

void OsmParserTest::_testSetBuildingLevelHeight()
{
    OsmParser parser;
    QSignalSpy spy(&parser, &OsmParser::buildingLevelHeightChanged);
    QVERIFY(spy.isValid());

    parser._setBuildingLevelHeight(QVariant(4.5f));
    QCOMPARE(spy.count(), 1);
    QCOMPARE_FUZZY(parser.buildingLevelHeight(), 4.5f, 0.001f);

    parser._setBuildingLevelHeight(QVariant(3.0f));
    QCOMPARE(spy.count(), 2);
    QCOMPARE_FUZZY(parser.buildingLevelHeight(), 3.0f, 0.001f);
}

void OsmParserTest::_testGpsRefSetReset()
{
    OsmParser parser;
    QSignalSpy spy(&parser, &OsmParser::gpsRefChanged);
    QVERIFY(spy.isValid());

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    parser.setGpsRef(ref);
    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QGeoCoordinate emittedCoord = args.at(0).value<QGeoCoordinate>();
    bool isRefSet = args.at(1).toBool();
    QCOMPARE(emittedCoord, ref);
    QVERIFY(isRefSet);

    parser.resetGpsRef();
    QCOMPARE(spy.count(), 1);
    args = spy.takeFirst();
    isRefSet = args.at(1).toBool();
    QVERIFY(!isRefSet);
}

UT_REGISTER_TEST(OsmParserTest, TestLabel::Unit)
