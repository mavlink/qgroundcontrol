#include "GeoJsonHelperTest.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>

#include "FileFixtures.h"
#include "GeoJsonHelper.h"

namespace {

QString _writeGeoJsonFile(const QString& dirPath, const QString& fileName, const QByteArray& contents)
{
    const QString filePath = QDir(dirPath).filePath(fileName);
    (void) TestFixtures::writeFile(filePath, contents);
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

void GeoJsonHelperTest::_determineShapeType_test_data()
{
    QTest::addColumn<QByteArray>("document");    // null to write no file
    QTest::addColumn<ShapeFileHelper::ShapeType>("expected");
    QTest::addColumn<QString>("errorFragment");  // empty when a type is found

    QTest::newRow("polygon") << kPolygonGeoJson << ShapeFileHelper::ShapeType::Polygon << QString();
    QTest::newRow("polyline") << kPolylineGeoJson << ShapeFileHelper::ShapeType::Polyline << QString();
    QTest::newRow("no shapes") << kNoShapesGeoJson << ShapeFileHelper::ShapeType::Error << "No shapes found";
    QTest::newRow("missing file") << QByteArray() << ShapeFileHelper::ShapeType::Error << "File not found";
    QTest::newRow("unsupported geometry") << kPointGeoJson << ShapeFileHelper::ShapeType::Error << "No supported type";
    QTest::newRow("invalid json") << kInvalidGeoJson << ShapeFileHelper::ShapeType::Error
                                  << "GeoJson file load failed.";
}

void GeoJsonHelperTest::_determineShapeType_test()
{
    QFETCH(QByteArray, document);
    QFETCH(ShapeFileHelper::ShapeType, expected);
    QFETCH(QString, errorFragment);

    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("shape.geojson");
    if (!document.isNull()) {
        QVERIFY(TestFixtures::writeFile(filePath, document));
    }
    QString error;

    QCOMPARE(GeoJsonHelper::determineShapeType(filePath, error), expected);
    QVERIFY2(errorFragment.isEmpty() ? error.isEmpty() : error.contains(errorFragment), qPrintable(error));
}

void GeoJsonHelperTest::_loadShape_test_data()
{
    QTest::addColumn<QByteArray>("document");
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

    // The ring of a feature is counter-clockwise, as GeoJSON prescribes: it comes back clockwise, without its
    // closing vertex.
    QTest::newRow("polygon of a feature")
        << kPolygonGeoJson << filtered << QList<QGeoCoordinate>{{47.1, 8.1}, {47.0, 8.1}, {47.0, 8.0}};
    QTest::newRow("polyline of a feature")
        << kPolylineGeoJson << filtered << QList<QGeoCoordinate>{{47.5, 8.5}, {47.6, 8.6}};
    QTest::newRow("polygon in a geometry collection")
        << kNestedPolygonGeoJson << filtered << QList<QGeoCoordinate>{{47.3, 8.3}, {47.2, 8.3}, {47.2, 8.2}};
    QTest::newRow("polyline in a geometry collection")
        << kNestedPolylineGeoJson << filtered << QList<QGeoCoordinate>{{47.7, 8.7}, {47.8, 8.8}, {47.9, 8.9}};
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

void GeoJsonHelperTest::_loadShape_test()
{
    QFETCH(QByteArray, document);
    QFETCH(double, filterMeters);
    QFETCH(QList<QGeoCoordinate>, expected);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shape.geojson", document);
    QList<QGeoCoordinate> vertices;
    QString error;

    if (document.contains("Polygon")) {
        QVERIFY2(GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error, filterMeters), qPrintable(error));
    } else {
        QVERIFY2(GeoJsonHelper::loadPolylineFromFile(filePath, vertices, error, filterMeters), qPrintable(error));
    }
    QVERIFY(error.isEmpty());
    QCOMPARE(vertices, expected);
}

void GeoJsonHelperTest::_loadFailure_test_data()
{
    QTest::addColumn<QByteArray>("document");
    QTest::addColumn<bool>("polygon");
    QTest::addColumn<QString>("warning");  // pattern of the warning the load logs, empty for none

    const QString tooFewForPolygon = QStringLiteral("fewer than 3 vertices");
    const QString tooFewForPolyline = QStringLiteral("fewer than 2 vertices");

    QTest::newRow("polygon without rings")
        << QByteArray(R"({"type": "Polygon", "coordinates": []})") << true << tooFewForPolygon;
    QTest::newRow("polygon with an empty ring")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[]]})") << true << tooFewForPolygon;
    QTest::newRow("polygon of two vertices")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.001, 47.0]]]})") << true
        << tooFewForPolygon;
    // Qt's importer drops the whole ring when one of its positions is not on the globe.
    QTest::newRow("polygon with a latitude out of range")
        << QByteArray(
               R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.0, 147.0], [8.001, 47.0], [8.0, 47.0]]]})")
        << true << tooFewForPolygon;
    QTest::newRow("polyline without vertices")
        << QByteArray(R"({"type": "LineString", "coordinates": []})") << false << tooFewForPolyline;
    QTest::newRow("polyline of one vertex")
        << QByteArray(R"({"type": "LineString", "coordinates": [[8.0, 47.0]]})") << false << tooFewForPolyline;
    // A shape of the other kind is passed over without a warning.
    QTest::newRow("polyline as a polygon") << kPolylineGeoJson << true << QString();
    QTest::newRow("polygon as a polyline") << kPolygonGeoJson << false << QString();
}

void GeoJsonHelperTest::_loadFailure_test()
{
    QFETCH(QByteArray, document);
    QFETCH(bool, polygon);
    QFETCH(QString, warning);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shape.geojson", document);
    QList<QGeoCoordinate> vertices;
    QString error;

    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.GeoJsonHelper", QtWarningMsg, QRegularExpression(warning));
    }
    if (polygon) {
        QVERIFY(!GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error));
        QCOMPARE(error, QStringLiteral("GeoJson file load failed. No polygon found in GeoJson file."));
    } else {
        QVERIFY(!GeoJsonHelper::loadPolylineFromFile(filePath, vertices, error));
        QCOMPARE(error, QStringLiteral("GeoJson file load failed. No polyline found in GeoJson file."));
    }
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
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
