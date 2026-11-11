#include "GeoJsonHelperTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtPositioning/QGeoCoordinate>

#include "GeoJsonHelper.h"
#include <QtCore/QTemporaryDir>

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

const QByteArray kTriangleRings = "[[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]]]";
const QByteArray kTriangleGeoJson = R"({"type": "Polygon", "coordinates": )" + kTriangleRings + "}";

QByteArray _nestInGeometryCollections(QByteArray geometry, int collectionCount)
{
    for (int i = 0; i < collectionCount; ++i) {
        geometry = R"({"type": "GeometryCollection", "geometries": [)" + geometry + "]}";
    }
    return geometry;
}

/// What determineShapeType(), loadPolygonFromFile() and loadPolylineFromFile() report for one document.
/// An empty error means the call succeeds.
struct GeoJsonShapesExpectation
{
    ShapeFileHelper::ShapeType shapeType = ShapeFileHelper::ShapeType::Error;
    QString shapeTypeError{};
    QList<QGeoCoordinate> polygon{};
    QString polygonError{};
    QList<QGeoCoordinate> polyline{};
    QString polylineError{};
};

}  // namespace

void GeoJsonHelperTest::_loadShapes_test_data()
{
    using ShapeType = ShapeFileHelper::ShapeType;

    QTest::addColumn<QByteArray>("geoJson");
    QTest::addColumn<ShapeType>("shapeType");
    QTest::addColumn<QString>("shapeTypeError");
    QTest::addColumn<QList<QGeoCoordinate>>("polygon");
    QTest::addColumn<QString>("polygonError");
    QTest::addColumn<QList<QGeoCoordinate>>("polyline");
    QTest::addColumn<QString>("polylineError");

    const auto addRow = [](const char* tag, const QByteArray& geoJson, const GeoJsonShapesExpectation& expected) {
        QTest::newRow(tag) << geoJson << expected.shapeType << expected.shapeTypeError << expected.polygon
                           << expected.polygonError << expected.polyline << expected.polylineError;
    };

    const QString noPolygon = QStringLiteral("GeoJson file load failed. No polygon found in GeoJson file.");
    const QString noPolyline = QStringLiteral("GeoJson file load failed. No polyline found in GeoJson file.");
    const GeoJsonShapesExpectation nothingFound{
        .shapeTypeError = QStringLiteral("GeoJson file load failed. No shapes found in GeoJson file."),
        .polygonError = QStringLiteral("GeoJson file load failed. No polygon data found in GeoJson file."),
        .polylineError = QStringLiteral("GeoJson file load failed. No polyline data found in GeoJson file."),
    };
    const GeoJsonShapesExpectation onlyPointsFound{
        .shapeTypeError = QStringLiteral("GeoJson file load failed. No supported type found in GeoJson file."),
        .polygonError = noPolygon,
        .polylineError = noPolyline,
    };
    const GeoJsonShapesExpectation triangleFound{
        .shapeType = ShapeType::Polygon,
        .polygon = {{47.0, 8.0}, {47.0, 8.1}, {47.1, 8.1}, {47.0, 8.0}},
        .polylineError = noPolyline,
    };
    const GeoJsonShapesExpectation emptyPolylineLoaded{.shapeType = ShapeType::Polyline, .polygonError = noPolygon};
    const GeoJsonShapesExpectation emptyPolygonLoaded{.shapeType = ShapeType::Polygon, .polylineError = noPolyline};

    addRow("FeatureCollection with a Polygon: the closing vertex is kept", kPolygonGeoJson, triangleFound);
    addRow("FeatureCollection with a LineString", kPolylineGeoJson,
           {.shapeType = ShapeType::Polyline, .polygonError = noPolygon, .polyline = {{47.5, 8.5}, {47.6, 8.6}}});
    addRow("FeatureCollection with a Point", kPointGeoJson, onlyPointsFound);
    addRow("FeatureCollection without features", kNoShapesGeoJson, nothingFound);
    addRow("Polygon in the GeometryCollection of a Feature", kNestedPolygonGeoJson,
           {.shapeType = ShapeType::Polygon,
            .polygon = {{47.2, 8.2}, {47.2, 8.3}, {47.3, 8.3}, {47.2, 8.2}},
            .polylineError = noPolyline});
    addRow("LineString in the GeometryCollection of a Feature", kNestedPolylineGeoJson,
           {.shapeType = ShapeType::Polyline,
            .polygonError = noPolygon,
            .polyline = {{47.7, 8.7}, {47.8, 8.8}, {47.9, 8.9}}});
    addRow("bare Polygon: holes are dropped",
           R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]],
                                                  [[8.04, 47.02], [8.06, 47.02], [8.06, 47.04], [8.04, 47.02]]]})",
           triangleFound);
    addRow("bare LineString: longitude comes first, altitude is optional",
           R"({"type": "LineString", "coordinates": [[120.5, 30.25, 100.0], [-120.5, -30.25]]})",
           {.shapeType = ShapeType::Polyline,
            .polygonError = noPolygon,
            .polyline = {{30.25, 120.5, 100.0}, {-30.25, -120.5}}});
    addRow("top-level Feature: Polygon with altitude",
           R"({"type": "Feature", "properties": null, "geometry":
               {"type": "Polygon",
                "coordinates": [[[8.0, 47.0, 500.0], [8.1, 47.0, 510.0], [8.1, 47.1, 520.0], [8.0, 47.0, 500.0]]]}})",
           {.shapeType = ShapeType::Polygon,
            .polygon = {{47.0, 8.0, 500.0}, {47.0, 8.1, 510.0}, {47.1, 8.1, 520.0}, {47.0, 8.0, 500.0}},
            .polylineError = noPolyline});
    addRow("MultiPolygon: only the outer ring of the first polygon",
           R"({"type": "MultiPolygon", "coordinates": [
               [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]],
                [[8.04, 47.02], [8.06, 47.02], [8.06, 47.04], [8.04, 47.02]]],
               [[[9.0, 48.0], [9.1, 48.0], [9.1, 48.1], [9.0, 48.0]]]]})",
           triangleFound);
    addRow("MultiLineString: only the first line",
           R"({"type": "MultiLineString", "coordinates": [[[8.5, 47.5], [8.6, 47.6]], [[9.5, 48.5], [9.6, 48.6]]]})",
           {.shapeType = ShapeType::Polyline, .polygonError = noPolygon, .polyline = {{47.5, 8.5}, {47.6, 8.6}}});
    addRow("MultiPoint", R"({"type": "MultiPoint", "coordinates": [[8.0, 47.0], [8.1, 47.1]]})", onlyPointsFound);
    addRow("several shapes: the first of each kind in document order, points skipped",
           R"({"type": "FeatureCollection", "features": [
               {"type": "Feature", "geometry": {"type": "Point", "coordinates": [7.0, 46.0]}},
               {"type": "Feature", "geometry": {"type": "LineString", "coordinates": [[8.5, 47.5], [8.6, 47.6]]}},
               {"type": "Feature", "geometry":
                {"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]]]}},
               {"type": "Feature", "geometry":
                {"type": "Polygon", "coordinates": [[[9.0, 48.0], [9.1, 48.0], [9.1, 48.1], [9.0, 48.0]]]}}]})",
           {.shapeType = ShapeType::Polyline,
            .polygon = {{47.0, 8.0}, {47.0, 8.1}, {47.1, 8.1}, {47.0, 8.0}},
            .polyline = {{47.5, 8.5}, {47.6, 8.6}}});
    addRow("Feature without geometry", R"({"type": "Feature", "geometry": null, "properties": {}})", nothingFound);
    addRow("unknown type", R"({"type": "Circle", "coordinates": [8.0, 47.0], "radius": 10.0})", nothingFound);
    addRow("top-level array", "[" + kTriangleGeoJson + "]", nothingFound);

    // Surprising: these load successfully, with no coordinates
    addRow("LineString without positions", R"({"type": "LineString", "coordinates": []})", emptyPolylineLoaded);
    addRow("LineString with a position out of range",
           R"({"type": "LineString", "coordinates": [[8.5, 47.5], [8.6, 97.6]]})", emptyPolylineLoaded);
    addRow("Polygon with a position without latitude",
           R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1], [8.1, 47.1], [8.0, 47.0]]]})",
           emptyPolygonLoaded);

    addRow("numbers written as strings are converted",
           R"({"type": "LineString", "coordinates": [["8.5", "47.5"], [8.6, 47.6]]})",
           {.shapeType = ShapeType::Polyline, .polygonError = noPolygon, .polyline = {{47.5, 8.5}, {47.6, 8.6}}});

    // Shapes nested in more than 15 collections are ignored; a FeatureCollection or Multi* geometry is one too
    addRow("15 nested GeometryCollections", _nestInGeometryCollections(kTriangleGeoJson, 15), triangleFound);
    addRow("16 nested GeometryCollections", _nestInGeometryCollections(kTriangleGeoJson, 16), nothingFound);
    addRow("FeatureCollection and 15 nested GeometryCollections",
           R"({"type": "FeatureCollection", "features": [{"type": "Feature", "geometry": )" +
               _nestInGeometryCollections(kTriangleGeoJson, 15) + "}]}",
           nothingFound);
    addRow("MultiPolygon in 15 nested GeometryCollections",
           _nestInGeometryCollections(R"({"type": "MultiPolygon", "coordinates": [)" + kTriangleRings + "]}", 15),
           nothingFound);
}

void GeoJsonHelperTest::_loadShapes_test()
{
    QFETCH(QByteArray, geoJson);
    QFETCH(ShapeFileHelper::ShapeType, shapeType);
    QFETCH(QString, shapeTypeError);
    QFETCH(QList<QGeoCoordinate>, polygon);
    QFETCH(QString, polygonError);
    QFETCH(QList<QGeoCoordinate>, polyline);
    QFETCH(QString, polylineError);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shapes.geojson", geoJson);

    // Stale output must not survive a call
    QString error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::determineShapeType(filePath, error), shapeType);
    QCOMPARE(error, shapeTypeError);

    QList<QGeoCoordinate> vertices{QGeoCoordinate(1.0, 2.0)};
    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::loadPolygonFromFile(filePath, vertices, error), polygonError.isEmpty());
    QCOMPARE(error, polygonError);
    QCOMPARE(vertices, polygon);

    QList<QGeoCoordinate> coords{QGeoCoordinate(1.0, 2.0)};
    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::loadPolylineFromFile(filePath, coords, error), polylineError.isEmpty());
    QCOMPARE(error, polylineError);
    QCOMPARE(coords, polyline);
}

void GeoJsonHelperTest::_loadUnreadableFile_test_data()
{
    QTest::addColumn<bool>("fileExists");
    QTest::addColumn<QByteArray>("contents");
    QTest::addColumn<QString>("errorFragment");

    QTest::newRow("missing file") << false << QByteArray() << QStringLiteral("File not found: ");
    QTest::newRow("invalid JSON") << true << kInvalidGeoJson << QStringLiteral(" (offset ");
    QTest::newRow("empty file") << true << QByteArray() << QStringLiteral(" (offset ");
}

void GeoJsonHelperTest::_loadUnreadableFile_test()
{
    QFETCH(bool, fileExists);
    QFETCH(QByteArray, contents);
    QFETCH(QString, errorFragment);

    QTemporaryDir tempDir;
    const QString filePath = fileExists ? _writeGeoJsonFile(tempDir.path(), "unreadable.geojson", contents)
                                        : QDir(tempDir.path()).filePath("missing.geojson");

    QString shapeTypeError;
    QCOMPARE(GeoJsonHelper::determineShapeType(filePath, shapeTypeError), ShapeFileHelper::ShapeType::Error);
    QVERIFY2(shapeTypeError.startsWith(QStringLiteral("GeoJson file load failed. ")), qPrintable(shapeTypeError));
    QVERIFY2(shapeTypeError.contains(errorFragment), qPrintable(shapeTypeError));

    QList<QGeoCoordinate> vertices{QGeoCoordinate(1.0, 2.0)};
    QString polygonError;
    QVERIFY(!GeoJsonHelper::loadPolygonFromFile(filePath, vertices, polygonError));
    QVERIFY(vertices.isEmpty());
    QCOMPARE(polygonError, shapeTypeError);

    QList<QGeoCoordinate> coords{QGeoCoordinate(1.0, 2.0)};
    QString polylineError;
    QVERIFY(!GeoJsonHelper::loadPolylineFromFile(filePath, coords, polylineError));
    QVERIFY(coords.isEmpty());
    QCOMPARE(polylineError, shapeTypeError);
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
