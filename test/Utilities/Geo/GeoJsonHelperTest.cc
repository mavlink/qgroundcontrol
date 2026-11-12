#include "GeoJsonHelperTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>

#include "GeoJsonHelper.h"

namespace {

QString _writeGeoJsonFile(const QString& dirPath, const QString& fileName, const QByteArray& contents)
{
    const QString filePath = QDir(dirPath).filePath(fileName);
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
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "polygon.geojson", kPolygonGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Polygon);
    QVERIFY(error.isEmpty());
}

void GeoJsonHelperTest::_determineShapeTypePolyline_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "line.geojson", kPolylineGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Polyline);
    QVERIFY(error.isEmpty());
}

void GeoJsonHelperTest::_determineShapeTypeNoShapes_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "empty.geojson", kNoShapesGeoJson);
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
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "point.geojson", kPointGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No supported type"));
}

void GeoJsonHelperTest::_determineShapeTypeInvalidJson_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "invalid.geojson", kInvalidGeoJson);
    QString error;

    const ShapeFileHelper::ShapeType shapeType = GeoJsonHelper::determineShapeType(filePath, error);
    QCOMPARE(shapeType, ShapeFileHelper::ShapeType::Error);
    QVERIFY(!error.isEmpty());
}

void GeoJsonHelperTest::_loadPolygonFromFile_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "polygon.geojson", kPolygonGeoJson);
    QList<QGeoCoordinate> vertices;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    // The ring is counter-clockwise, as GeoJSON prescribes: it comes back clockwise, without its closing vertex.
    QCOMPARE(vertices, (QList<QGeoCoordinate>{{47.1, 8.1}, {47.0, 8.1}, {47.0, 8.0}}));
}

void GeoJsonHelperTest::_loadPolylineFromFile_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "line.geojson", kPolylineGeoJson);
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
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "nested-polygon.geojson", kNestedPolygonGeoJson);
    QList<QGeoCoordinate> vertices;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error);
    QVERIFY(loaded);
    QVERIFY(error.isEmpty());
    QCOMPARE(vertices, (QList<QGeoCoordinate>{{47.3, 8.3}, {47.2, 8.3}, {47.2, 8.2}}));
}

void GeoJsonHelperTest::_loadPolylineFromNestedGeometryCollection_test()
{
    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "nested-polyline.geojson", kNestedPolylineGeoJson);
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
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "line.geojson", kPolylineGeoJson);
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
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "polygon.geojson", kPolygonGeoJson);
    QList<QGeoCoordinate> coords;
    QString error;

    const bool loaded = GeoJsonHelper::loadPolylineFromFile(filePath, coords, error);
    QVERIFY(!loaded);
    QVERIFY(coords.isEmpty());
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("No polyline"));
}

void GeoJsonHelperTest::_loadShapeCleanUp_test_data()
{
    QTest::addColumn<QByteArray>("geometry");
    QTest::addColumn<double>("filterMeters");
    QTest::addColumn<QList<QGeoCoordinate>>("expected");

    const double filtered = ShapeFileHelper::kDefaultVertexFilterMeters;
    const double unfiltered = 0.0;
    const auto polygon = [](const char* ring) {
        return QByteArray(R"({"type": "Polygon", "coordinates": [[)") + ring + "]]}";
    };
    const auto lineString = [](const char* positions) {
        return QByteArray(R"({"type": "LineString", "coordinates": [)") + positions + "]}";
    };

    // The rings and lines of ShapeTest::_testLoadShapes, which the KML and SHP loaders return the same way.
    const QList<QGeoCoordinate> square{{47.0, 8.0}, {47.001, 8.0}, {47.001, 8.001}, {47.0, 8.001}};
    const char* denseRing =
        "[8, 47], [8, 47.00001], [8, 47.00002], [8, 47.001], [8.001, 47.001], [8.001, 47], "
        "[8.00001, 47], [8, 47]";
    const char* denseLine = "[8, 47], [8, 47.00001], [8, 47.001], [8.001, 47.001]";

    QTest::newRow("polygon, closing vertex")
        << polygon("[8, 47], [8, 47.001], [8.001, 47.001], [8.001, 47], [8, 47]") << filtered << square;
    QTest::newRow("polygon, closed to within a centimetre")
        << polygon("[8, 47], [8, 47.001], [8.001, 47.001], [8.001, 47], [8.00000001, 47]") << filtered << square;
    QTest::newRow("polygon, counter-clockwise")
        << polygon("[8, 47], [8.001, 47], [8.001, 47.001], [8, 47.001], [8, 47]") << filtered
        << QList<QGeoCoordinate>{{47.001, 8.0}, {47.001, 8.001}, {47.0, 8.001}, {47.0, 8.0}};
    QTest::newRow("polygon with altitude")
        << polygon("[8, 47, 10], [8, 47.001, 20], [8.001, 47.001, 30], [8.001, 47, 40], [8, 47, 10]") << filtered
        << QList<QGeoCoordinate>{{47.0, 8.0, 10.0}, {47.001, 8.0, 20.0}, {47.001, 8.001, 30.0}, {47.0, 8.001, 40.0}};
    QTest::newRow("polygon, dense") << polygon(denseRing) << filtered
                                    << (square + QList<QGeoCoordinate>{{47.0, 8.00001}});
    QTest::newRow("polygon, dense, unfiltered")
        << polygon(denseRing) << unfiltered
        << QList<QGeoCoordinate>{{47.0, 8.0},     {47.00001, 8.0}, {47.00002, 8.0}, {47.001, 8.0},
                                 {47.001, 8.001}, {47.0, 8.001},   {47.0, 8.00001}};
    QTest::newRow("polygon, filter keeps three vertices")
        << polygon("[8, 47], [8, 47.00001], [8.001, 47], [8, 47]") << filtered
        << QList<QGeoCoordinate>{{47.0, 8.0}, {47.00001, 8.0}, {47.0, 8.001}};
    QTest::newRow("polyline, dense") << lineString(denseLine) << filtered
                                     << QList<QGeoCoordinate>{{47.0, 8.0}, {47.001, 8.0}, {47.001, 8.001}};
    QTest::newRow("polyline, dense, unfiltered")
        << lineString(denseLine) << unfiltered
        << QList<QGeoCoordinate>{{47.0, 8.0}, {47.00001, 8.0}, {47.001, 8.0}, {47.001, 8.001}};
    QTest::newRow("polyline, filter keeps two vertices")
        << lineString("[8, 47], [8, 47.00001]") << filtered << QList<QGeoCoordinate>{{47.0, 8.0}, {47.00001, 8.0}};
}

void GeoJsonHelperTest::_loadShapeCleanUp_test()
{
    QFETCH(QByteArray, geometry);
    QFETCH(double, filterMeters);
    QFETCH(QList<QGeoCoordinate>, expected);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shape.geojson", geometry);
    QList<QGeoCoordinate> vertices;
    QString error;

    if (geometry.contains("Polygon")) {
        QVERIFY2(GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error, filterMeters), qPrintable(error));
    } else {
        QVERIFY2(GeoJsonHelper::loadPolylineFromFile(filePath, vertices, error, filterMeters), qPrintable(error));
    }
    QVERIFY(error.isEmpty());
    QCOMPARE(vertices, expected);
}

void GeoJsonHelperTest::_loadTooFewVertices_test_data()
{
    QTest::addColumn<QByteArray>("geometry");
    QTest::addColumn<bool>("polygon");

    QTest::newRow("polygon without rings") << QByteArray(R"({"type": "Polygon", "coordinates": []})") << true;
    QTest::newRow("polygon with an empty ring") << QByteArray(R"({"type": "Polygon", "coordinates": [[]]})") << true;
    QTest::newRow("polygon of two vertices")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.001, 47.0]]]})") << true;
    // Qt's importer drops the whole ring when one of its positions is not on the globe.
    QTest::newRow("polygon with a latitude out of range")
        << QByteArray(
               R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.0, 147.0], [8.001, 47.0], [8.0, 47.0]]]})")
        << true;
    QTest::newRow("polyline without vertices") << QByteArray(R"({"type": "LineString", "coordinates": []})") << false;
    QTest::newRow("polyline of one vertex")
        << QByteArray(R"({"type": "LineString", "coordinates": [[8.0, 47.0]]})") << false;
}

void GeoJsonHelperTest::_loadTooFewVertices_test()
{
    QFETCH(QByteArray, geometry);
    QFETCH(bool, polygon);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shape.geojson", geometry);
    QList<QGeoCoordinate> vertices;
    QString error;

    if (polygon) {
        expectLogMessage("Utilities.GeoJsonHelper", QtWarningMsg, QRegularExpression("fewer than 3 vertices"));
        QVERIFY(!GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error));
        QCOMPARE(error, QStringLiteral("GeoJson file load failed. No polygon found in GeoJson file."));
    } else {
        expectLogMessage("Utilities.GeoJsonHelper", QtWarningMsg, QRegularExpression("fewer than 2 vertices"));
        QVERIFY(!GeoJsonHelper::loadPolylineFromFile(filePath, vertices, error));
        QCOMPARE(error, QStringLiteral("GeoJson file load failed. No polyline found in GeoJson file."));
    }
    verifyExpectedLogMessage();
    QVERIFY(vertices.isEmpty());
}

void GeoJsonHelperTest::_loadSaveGeoJsonCoordinate_test()
{
    const QGeoCoordinate original(47.3764, 8.5481);

    QJsonValue jsonValue;
    GeoJsonHelper::saveGeoJsonCoordinate(original, false, jsonValue);
    QVERIFY(jsonValue.isArray());

    // GeoJSON ordering is [lon, lat] (RFC 7946).
    const QJsonArray arr = jsonValue.toArray();
    QCOMPARE(arr.count(), 2);
    QCOMPARE_FUZZY(arr[0].toDouble(), original.longitude(), 1e-7);
    QCOMPARE_FUZZY(arr[1].toDouble(), original.latitude(), 1e-7);

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoJsonCoordinate(jsonValue, false, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
}

void GeoJsonHelperTest::_loadSaveGeoJsonCoordinateWithAltitude_test()
{
    const QGeoCoordinate original(47.3764, 8.5481, 500.0);

    QJsonValue jsonValue;
    GeoJsonHelper::saveGeoJsonCoordinate(original, true, jsonValue);

    QGeoCoordinate loaded;
    QString errorString;
    QVERIFY(GeoJsonHelper::loadGeoJsonCoordinate(jsonValue, true, loaded, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE_FUZZY(loaded.latitude(), original.latitude(), 1e-7);
    QCOMPARE_FUZZY(loaded.longitude(), original.longitude(), 1e-7);
}

UT_REGISTER_TEST(GeoJsonHelperTest, TestLabel::Unit, TestLabel::Utilities)
