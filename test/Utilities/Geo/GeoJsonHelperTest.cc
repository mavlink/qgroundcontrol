#include "GeoJsonHelperTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include "GeoJsonHelper.h"

namespace {

QString _writeGeoJsonFile(QTemporaryDir &tempDir, const QString &fileName, const QByteArray &contents)
{
    const QString filePath = tempDir.filePath(fileName);
    QFile file(filePath);
    const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    Q_ASSERT(opened);
    Q_UNUSED(opened);
    (void) file.write(contents);
    file.close();
    return filePath;
}

const QByteArray kPolygonGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [
            [8.0, 47.0],
            [8.1, 47.0],
            [8.1, 47.1],
            [8.0, 47.0]
          ]
        ]
      }
    }
  ]
}
)JSON";

const QByteArray kPolylineGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "LineString",
        "coordinates": [
          [8.5, 47.5],
          [8.6, 47.6]
        ]
      }
    }
  ]
}
)JSON";

const QByteArray kPointGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [8.0, 47.0]
      }
    }
  ]
}
)JSON";

const QByteArray kNestedPolygonGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "meta": { "source": "nested" }
      },
      "geometry": {
        "type": "GeometryCollection",
        "geometries": [
          {
            "type": "Polygon",
            "coordinates": [
              [
                [8.2, 47.2],
                [8.3, 47.2],
                [8.3, 47.3],
                [8.2, 47.2]
              ]
            ]
          }
        ]
      }
    }
  ]
}
)JSON";

const QByteArray kNestedPolylineGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {
        "meta": { "source": "nested" }
      },
      "geometry": {
        "type": "GeometryCollection",
        "geometries": [
          {
            "type": "LineString",
            "coordinates": [
              [8.7, 47.7],
              [8.8, 47.8],
              [8.9, 47.9]
            ]
          }
        ]
      }
    }
  ]
}
)JSON";

const QByteArray kNoShapesGeoJson = R"JSON(
{
  "type": "FeatureCollection",
  "features": []
}
)JSON";

const QByteArray kInvalidGeoJson = "{not valid json";

} // namespace

void GeoJsonHelperTest::_determineShapeTypePolygon_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "polygon.geojson", kPolygonGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Polygon);
    QVERIFY(error.isEmpty());
}

void GeoJsonHelperTest::_determineShapeTypePolyline_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "line.geojson", kPolylineGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Polyline);
    QVERIFY(error.isEmpty());
}

void GeoJsonHelperTest::_determineShapeTypeNoShapes_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "empty.geojson", kNoShapesGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No shapes found"));
}

void GeoJsonHelperTest::_determineShapeTypeMissingFile_test()
{
    QString error;
    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType("missing-file.geojson", error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("File not found"));
}

void GeoJsonHelperTest::_determineShapeTypeUnsupportedGeometry_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "point.geojson", kPointGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No supported type"));
}

void GeoJsonHelperTest::_determineShapeTypeInvalidJson_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "invalid.geojson", kInvalidGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
}

void GeoJsonHelperTest::_loadPolygonFromFile_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "polygon.geojson", kPolygonGeoJson);
    QList<QGeoCoordinate> vertices;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    QVERIFY(vertices.size() >= 4);
    QCOMPARE(vertices.first().latitude(), 47.0);
    QCOMPARE(vertices.first().longitude(), 8.0);
}

void GeoJsonHelperTest::_loadPolylineFromFile_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "line.geojson", kPolylineGeoJson);
    QList<QGeoCoordinate> coords;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolylineFromFile(filePath, coords, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    QCOMPARE(coords.size(), 2);
    QCOMPARE(coords.at(0).latitude(), 47.5);
    QCOMPARE(coords.at(0).longitude(), 8.5);
    QCOMPARE(coords.at(1).latitude(), 47.6);
    QCOMPARE(coords.at(1).longitude(), 8.6);
}

void GeoJsonHelperTest::_loadPolygonFromNestedGeometryCollection_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "nested-polygon.geojson", kNestedPolygonGeoJson);
    QList<QGeoCoordinate> vertices;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    QVERIFY(vertices.size() >= 4);
    QCOMPARE(vertices.first().latitude(), 47.2);
    QCOMPARE(vertices.first().longitude(), 8.2);
}

void GeoJsonHelperTest::_loadPolylineFromNestedGeometryCollection_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "nested-polyline.geojson", kNestedPolylineGeoJson);
    QList<QGeoCoordinate> coords;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolylineFromFile(filePath, coords, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    QCOMPARE(coords.size(), 3);
    QCOMPARE(coords.at(0).latitude(), 47.7);
    QCOMPARE(coords.at(0).longitude(), 8.7);
    QCOMPARE(coords.at(2).latitude(), 47.9);
    QCOMPARE(coords.at(2).longitude(), 8.9);
}

void GeoJsonHelperTest::_loadPolygonFromPolylineFails_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "line.geojson", kPolylineGeoJson);
    QList<QGeoCoordinate> vertices;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error);
    QVERIFY(!loaded);
    QVERIFY(vertices.isEmpty());
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No polygon"));
}

void GeoJsonHelperTest::_loadPolylineFromPolygonFails_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = _writeGeoJsonFile(tempDir, "polygon.geojson", kPolygonGeoJson);
    QList<QGeoCoordinate> coords;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolylineFromFile(filePath, coords, error);
    QVERIFY(!loaded);
    QVERIFY(coords.isEmpty());
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No polyline"));
}

UT_REGISTER_TEST(GeoJsonHelperTest, TestLabel::Unit, TestLabel::Utilities)
