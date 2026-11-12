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

const QByteArray kTriangleRings = "[[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]]]";
const QByteArray kTriangleGeoJson = R"({"type": "Polygon", "coordinates": )" + kTriangleRings + "}";

QByteArray _nestInGeometryCollections(QByteArray geometry, int collectionCount)
{
    for (int i = 0; i < collectionCount; ++i) {
        geometry = R"({"type": "GeometryCollection", "geometries": [)" + geometry + "]}";
    }
    return geometry;
}

/// What the loaders find in one document. A loader that finds nothing of its kind fails.
struct GeoJsonShapesExpectation
{
    ShapeFileHelper::ShapeType shapeType = ShapeFileHelper::ShapeType::Error;
    QList<QList<QGeoCoordinate>> polygons{};
    QList<QList<QGeoCoordinate>> polylines{};
    QList<QGeoCoordinate> points{};
};

const QString kGeoJsonErrorPrefix = QStringLiteral("GeoJSON file load failed. ");

}  // namespace

void GeoJsonHelperTest::_loadShapes_test_data()
{
    using ShapeType = ShapeFileHelper::ShapeType;

    QTest::addColumn<QByteArray>("geoJson");
    QTest::addColumn<ShapeType>("shapeType");
    QTest::addColumn<QList<QList<QGeoCoordinate>>>("polygons");
    QTest::addColumn<QList<QList<QGeoCoordinate>>>("polylines");
    QTest::addColumn<QList<QGeoCoordinate>>("points");

    const auto addRow = [](const char* tag, const QByteArray& geoJson, const GeoJsonShapesExpectation& expected) {
        QTest::newRow(tag) << geoJson << expected.shapeType << expected.polygons << expected.polylines
                           << expected.points;
    };

    // kTriangleRings is closed and counterclockwise, as RFC 7946 wants it
    const QList<QGeoCoordinate> triangle{{47.1, 8.1}, {47.0, 8.1}, {47.0, 8.0}};
    const QList<QGeoCoordinate> line{{47.5, 8.5}, {47.6, 8.6}};
    const GeoJsonShapesExpectation nothingFound{};
    const GeoJsonShapesExpectation triangleFound{.shapeType = ShapeType::Polygon, .polygons = {triangle}};
    const GeoJsonShapesExpectation lineFound{.shapeType = ShapeType::Polyline, .polylines = {line}};

    addRow("FeatureCollection with a Polygon: closing position dropped, wound clockwise", kPolygonGeoJson,
           triangleFound);
    addRow("FeatureCollection with a LineString", kPolylineGeoJson, lineFound);
    addRow("FeatureCollection with a Point", kPointGeoJson, {.shapeType = ShapeType::Point, .points = {{47.0, 8.0}}});
    addRow("FeatureCollection without features", kNoShapesGeoJson, nothingFound);
    addRow("Polygon in the GeometryCollection of a Feature", kNestedPolygonGeoJson,
           {.shapeType = ShapeType::Polygon, .polygons = {{{47.3, 8.3}, {47.2, 8.3}, {47.2, 8.2}}}});
    addRow("LineString in the GeometryCollection of a Feature", kNestedPolylineGeoJson,
           {.shapeType = ShapeType::Polyline, .polylines = {{{47.7, 8.7}, {47.8, 8.8}, {47.9, 8.9}}}});
    addRow("bare Polygon: holes are dropped",
           R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]],
                                                  [[8.04, 47.02], [8.06, 47.02], [8.06, 47.04], [8.04, 47.02]]]})",
           triangleFound);
    addRow("clockwise Polygon without closing position: loaded as written",
           R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.1], [8.1, 47.0]]]})",
           {.shapeType = ShapeType::Polygon, .polygons = {{{47.0, 8.0}, {47.1, 8.1}, {47.0, 8.1}}}});
    addRow("bare LineString: longitude comes first, altitude is optional",
           R"({"type": "LineString", "coordinates": [[120.5, 30.25, 100.0], [-120.5, -30.25]]})",
           {.shapeType = ShapeType::Polyline, .polylines = {{{30.25, 120.5, 100.0}, {-30.25, -120.5}}}});
    addRow(
        "top-level Feature: Polygon with altitude",
        R"({"type": "Feature", "properties": null, "geometry":
               {"type": "Polygon",
                "coordinates": [[[8.0, 47.0, 500.0], [8.1, 47.0, 510.0], [8.1, 47.1, 520.0], [8.0, 47.0, 500.0]]]}})",
        {.shapeType = ShapeType::Polygon, .polygons = {{{47.1, 8.1, 520.0}, {47.0, 8.1, 510.0}, {47.0, 8.0, 500.0}}}});
    addRow("MultiPolygon: the outer ring of each polygon",
           R"({"type": "MultiPolygon", "coordinates": [
               [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]],
                [[8.04, 47.02], [8.06, 47.02], [8.06, 47.04], [8.04, 47.02]]],
               [[[9.0, 48.0], [9.1, 48.0], [9.1, 48.1], [9.0, 48.0]]]]})",
           {.shapeType = ShapeType::Polygon, .polygons = {triangle, {{48.1, 9.1}, {48.0, 9.1}, {48.0, 9.0}}}});
    addRow("MultiLineString: each line",
           R"({"type": "MultiLineString", "coordinates": [[[8.5, 47.5], [8.6, 47.6]], [[9.5, 48.5], [9.6, 48.6]]]})",
           {.shapeType = ShapeType::Polyline, .polylines = {line, {{48.5, 9.5}, {48.6, 9.6}}}});
    addRow("MultiPoint: each point", R"({"type": "MultiPoint", "coordinates": [[8.0, 47.0], [8.1, 47.1]]})",
           {.shapeType = ShapeType::Point, .points = {{47.0, 8.0}, {47.1, 8.1}}});
    addRow("several shapes: each kind in document order, the type is Polygon before Polyline before Point",
           R"({"type": "FeatureCollection", "features": [
               {"type": "Feature", "geometry": {"type": "Point", "coordinates": [7.0, 46.0]}},
               {"type": "Feature", "geometry": {"type": "LineString", "coordinates": [[8.5, 47.5], [8.6, 47.6]]}},
               {"type": "Feature", "geometry":
                {"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.0], [8.1, 47.1], [8.0, 47.0]]]}},
               {"type": "Feature", "geometry":
                {"type": "Polygon", "coordinates": [[[9.0, 48.0], [9.1, 48.0], [9.1, 48.1], [9.0, 48.0]]]}}]})",
           {.shapeType = ShapeType::Polygon,
            .polygons = {triangle, {{48.1, 9.1}, {48.0, 9.1}, {48.0, 9.0}}},
            .polylines = {line},
            .points = {{46.0, 7.0}}});
    addRow("Feature without geometry", R"({"type": "Feature", "geometry": null, "properties": {}})", nothingFound);
    addRow("unknown type", R"({"type": "Circle", "coordinates": [8.0, 47.0], "radius": 10.0})", nothingFound);
    addRow("top-level array", "[" + kTriangleGeoJson + "]", nothingFound);
    addRow("numbers written as strings are converted",
           R"({"type": "LineString", "coordinates": [["8.5", "47.5"], [8.6, 47.6]]})", lineFound);
    addRow("crs member that names WGS84",
           R"({"type": "LineString", "coordinates": [[8.5, 47.5], [8.6, 47.6]],
               "crs": {"type": "name", "properties": {"name": "urn:ogc:def:crs:OGC:1.3:CRS84"}}})",
           lineFound);
    addRow("vertices less than 5 m after the one before are dropped",
           R"({"type": "LineString", "coordinates": [[8.5, 47.5], [8.50001, 47.5], [8.6, 47.6]]})", lineFound);

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
    QFETCH(QList<QList<QGeoCoordinate>>, polygons);
    QFETCH(QList<QList<QGeoCoordinate>>, polylines);
    QFETCH(QList<QGeoCoordinate>, points);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shapes.geojson", geoJson);
    const auto errorWithout = [](bool found, const char* typeName) {
        return found ? QString() : kGeoJsonErrorPrefix + QStringLiteral("No %1 found in GeoJSON file.").arg(typeName);
    };

    // Stale output must not survive a call
    QString error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::determineShapeType(filePath, error), shapeType);
    QCOMPARE(error, (shapeType == ShapeFileHelper::ShapeType::Error)
                        ? kGeoJsonErrorPrefix + QStringLiteral("No shapes found in GeoJSON file.")
                        : QString());

    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::getEntityCount(filePath, error),
             static_cast<int>(polygons.size() + polylines.size() + points.size()));
    QCOMPARE(error, QString());

    QList<QList<QGeoCoordinate>> loadedPolygons{{QGeoCoordinate(1.0, 2.0)}};
    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::loadPolygonsFromFile(filePath, loadedPolygons, error), !polygons.isEmpty());
    QCOMPARE(error, errorWithout(!polygons.isEmpty(), "Polygon"));
    QCOMPARE(loadedPolygons, polygons);

    QList<QList<QGeoCoordinate>> loadedPolylines{{QGeoCoordinate(1.0, 2.0)}};
    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::loadPolylinesFromFile(filePath, loadedPolylines, error), !polylines.isEmpty());
    QCOMPARE(error, errorWithout(!polylines.isEmpty(), "LineString"));
    QCOMPARE(loadedPolylines, polylines);

    QList<QGeoCoordinate> loadedPoints{QGeoCoordinate(1.0, 2.0)};
    error = QStringLiteral("stale");
    QCOMPARE(GeoJsonHelper::loadPointsFromFile(filePath, loadedPoints, error), !points.isEmpty());
    QCOMPARE(error, errorWithout(!points.isEmpty(), "Point"));
    QCOMPARE(loadedPoints, points);
}

void GeoJsonHelperTest::_loadUnusableShapes_test_data()
{
    QTest::addColumn<QByteArray>("geoJson");
    QTest::addColumn<bool>("loadPolygons");
    QTest::addColumn<QStringList>("problems");
    QTest::addColumn<QList<QList<QGeoCoordinate>>>("usable");

    // What is wrong with each unusable shape of the loaded kind, in document order
    const auto invalidPosition = [](const char* shape, const char* position) {
        return QStringLiteral("%1 has an invalid position: %2. A position is [longitude, latitude] in degrees.")
            .arg(shape, position);
    };
    const QString lineTooShort = QStringLiteral("LineString 1 has fewer than 2 vertices.");
    const QString polygonTooSmall = QStringLiteral("Polygon 1 has fewer than 3 vertices.");
    const QList<QList<QGeoCoordinate>> none;

    QTest::newRow("LineString without positions")
        << QByteArray(R"({"type": "LineString", "coordinates": []})") << false << QStringList{lineTooShort} << none;
    QTest::newRow("LineString with a latitude out of range")
        << QByteArray(R"({"type": "LineString", "coordinates": [[8.5, 47.5], [8.6, 97.6]]})") << false
        << QStringList{invalidPosition("LineString 1", "[8.6,97.6]")} << none;
    QTest::newRow("LineString with an altitude that is no number")
        << QByteArray(R"({"type": "LineString", "coordinates": [[8.5, 47.5, true], [8.6, 47.6]]})") << false
        << QStringList{invalidPosition("LineString 1", "[8.5,47.5,true]")} << none;
    QTest::newRow("LineString with rings for positions: the position is shortened")
        << QByteArray(R"({"type": "LineString", "coordinates": [[[8.25, 47.25], [8.5, 47.25], [8.5, 47.5],
                          [8.75, 47.5], [8.75, 47.75], [8.25, 47.75], [8.25, 47.25]]]})")
        << false
        << QStringList{invalidPosition("LineString 1",
                                       "[[8.25,47.25],[8.5,47.25],[8.5,47.5],[8.75,47.5],[8.75,47.75...")}
        << none;
    QTest::newRow("Polygon without rings")
        << QByteArray(R"({"type": "Polygon", "coordinates": []})") << true << QStringList{polygonTooSmall} << none;
    QTest::newRow("Polygon with two vertices and the closing position")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1, 47.0], [8.0, 47.0]]]})") << true
        << QStringList{polygonTooSmall} << none;
    QTest::newRow("Polygon with a position without latitude")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [8.1], [8.1, 47.1], [8.0, 47.0]]]})") << true
        << QStringList{invalidPosition("Polygon 1", "[8.1]")} << none;
    QTest::newRow("Polygon with a longitude that is no number")
        << QByteArray(R"({"type": "Polygon", "coordinates": [[[8.0, 47.0], [null, 47.5], [8.1, 47.1], [8.0, 47.0]]]})")
        << true << QStringList{invalidPosition("Polygon 1", "[null,47.5]")} << none;
    QTest::newRow("several unusable shapes: the error is about the first")
        << QByteArray(R"({"type": "MultiLineString", "coordinates": [[[8.5, 47.5], [8.6, 97.6]], [[9.5, 48.5]]]})")
        << false
        << QStringList{invalidPosition("LineString 1", "[8.6,97.6]"),
                       QStringLiteral("LineString 2 has fewer than 2 vertices.")}
        << none;
    QTest::newRow("unusable shape before a usable one: skipped")
        << QByteArray(R"({"type": "MultiLineString", "coordinates": [[[8.5, 47.5], [8.6, 97.6]],
                                                                    [[9.5, 48.5], [9.6, 48.6]]]})")
        << false << QStringList{invalidPosition("LineString 1", "[8.6,97.6]")}
        << QList<QList<QGeoCoordinate>>{{{48.5, 9.5}, {48.6, 9.6}}};
}

void GeoJsonHelperTest::_loadUnusableShapes_test()
{
    QFETCH(QByteArray, geoJson);
    QFETCH(bool, loadPolygons);
    QFETCH(QStringList, problems);
    QFETCH(QList<QList<QGeoCoordinate>>, usable);

    QTemporaryDir tempDir;
    const QString filePath = _writeGeoJsonFile(tempDir.path(), "shapes.geojson", geoJson);

    for (const QString& problem : problems) {
        expectLogMessage("Utilities.GeoJsonHelper", QtWarningMsg,
                         QRegularExpression(QRegularExpression::escape(problem)));
    }
    QList<QList<QGeoCoordinate>> loaded{{QGeoCoordinate(1.0, 2.0)}};
    QString error = QStringLiteral("stale");
    const bool success = loadPolygons ? GeoJsonHelper::loadPolygonsFromFile(filePath, loaded, error)
                                      : GeoJsonHelper::loadPolylinesFromFile(filePath, loaded, error);
    for (qsizetype i = 0; i < problems.size(); ++i) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(success, !usable.isEmpty());
    QCOMPARE(error, usable.isEmpty() ? kGeoJsonErrorPrefix + problems.first() : QString());
    QCOMPARE(loaded, usable);
}

void GeoJsonHelperTest::_loadUnreadableFile_test_data()
{
    QTest::addColumn<bool>("fileExists");
    QTest::addColumn<QByteArray>("contents");
    QTest::addColumn<QString>("errorFragment");

    const QByteArray triangleWithCrs = kTriangleGeoJson.chopped(1) + R"(, "crs": {"type": )";
    const QString convertToWgs84 = QStringLiteral(". Convert the file to WGS84 using QGIS or ogr2ogr.");

    QTest::newRow("missing file") << false << QByteArray() << QStringLiteral("File not found: ");
    QTest::newRow("invalid JSON") << true << kInvalidGeoJson << QStringLiteral(" (offset ");
    QTest::newRow("empty file") << true << QByteArray() << QStringLiteral(" (offset ");
    QTest::newRow("crs member that names another system than WGS84")
        << true << QByteArray(triangleWithCrs + R"("name", "properties": {"name": "urn:ogc:def:crs:EPSG::3857"}}})")
        << QString(QStringLiteral("Unsupported coordinate reference system: urn:ogc:def:crs:EPSG::3857") +
                   convertToWgs84);
    QTest::newRow("crs member that links to a system")
        << true << QByteArray(triangleWithCrs + R"("link", "properties": {"href": "crs.wkt"}}})")
        << QString(QStringLiteral("Unsupported coordinate reference system: ") +
                   QStringLiteral(R"({"properties":{"href":"crs.wkt"},"type":"link"})") + convertToWgs84);
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
    QVERIFY2(shapeTypeError.startsWith(kGeoJsonErrorPrefix), qPrintable(shapeTypeError));
    QVERIFY2(shapeTypeError.contains(errorFragment), qPrintable(shapeTypeError));

    QString entityCountError;
    QCOMPARE(GeoJsonHelper::getEntityCount(filePath, entityCountError), 0);
    QCOMPARE(entityCountError, shapeTypeError);

    QList<QList<QGeoCoordinate>> polygons{{QGeoCoordinate(1.0, 2.0)}};
    QString polygonsError;
    QVERIFY(!GeoJsonHelper::loadPolygonsFromFile(filePath, polygons, polygonsError));
    QVERIFY(polygons.isEmpty());
    QCOMPARE(polygonsError, shapeTypeError);

    QList<QList<QGeoCoordinate>> polylines{{QGeoCoordinate(1.0, 2.0)}};
    QString polylinesError;
    QVERIFY(!GeoJsonHelper::loadPolylinesFromFile(filePath, polylines, polylinesError));
    QVERIFY(polylines.isEmpty());
    QCOMPARE(polylinesError, shapeTypeError);

    QList<QGeoCoordinate> points{QGeoCoordinate(1.0, 2.0)};
    QString pointsError;
    QVERIFY(!GeoJsonHelper::loadPointsFromFile(filePath, points, pointsError));
    QVERIFY(points.isEmpty());
    QCOMPARE(pointsError, shapeTypeError);
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
