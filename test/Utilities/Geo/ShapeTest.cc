#include "ShapeTest.h"
#include "CSVHelper.h"
#include "GeoFormatRegistry.h"
#include "GeoJsonHelper.h"
#include "GPXHelper.h"
#include "KMLHelper.h"
#include "OpenAirParser.h"
#include "SHPHelper.h"
#include "KMLDomDocument.h"
#include "KMLSchemaValidator.h"
#include "WKTHelper.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtTest/QTest>
#include <shapefil.h>

#include <cmath>

QString ShapeTest::_copyRes(const QTemporaryDir &tmpDir, const QString &name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void) QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    if (!QFile(resPath).copy(dstPath)) {
        qFatal("Failed to copy test resource %s to %s", qPrintable(resPath), qPrintable(dstPath));
    }
    return dstPath;
}

bool ShapeTest::_copyShapefileSet(const QTemporaryDir &tmpDir, const QString &baseName, QString &shpPath, bool copyPrj)
{
    shpPath = _copyRes(tmpDir, QStringLiteral("%1.shp").arg(baseName));
    if (shpPath.isEmpty()) {
        return false;
    }

    QStringList sidecarExts { QStringLiteral("dbf"), QStringLiteral("shx") };
    if (copyPrj) {
        sidecarExts.append(QStringLiteral("prj"));
    }

    for (const QString &ext : std::as_const(sidecarExts)) {
        if (_copyRes(tmpDir, QStringLiteral("%1.%2").arg(baseName, ext)).isEmpty()) {
            return false;
        }
    }

    return true;
}

bool ShapeTest::_writeTextFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    return stream.status() == QTextStream::Ok;
}

bool ShapeTest::_writePrjFile(const QString &path, const QString &content)
{
    return _writeTextFile(path, content);
}

QString ShapeTest::_writeKmlFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content)
{
    const QString path = tmpDir.filePath(name);
    if (!_writeTextFile(path, content)) {
        qFatal("Failed to write KML test file: %s", qPrintable(path));
    }
    return path;
}

void ShapeTest::_testLoadPolylineFromSHP()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "pline", shpFile));
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(GeoFormatRegistry::loadPolyline(shpFile, rgCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() > 0);
}

void ShapeTest::_testLoadPolylineFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polyline.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(GeoFormatRegistry::loadPolyline(shpFile, rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() > 0);
}

void ShapeTest::_testLoadPolygonFromSHP()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile));
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(GeoFormatRegistry::loadPolygon(shpFile, rgCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testLoadPolygonFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(shpFile, rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testLoadPointsFromSHP()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "point", shpFile));
    QString errorString;
    QList<QGeoCoordinate> points;
    QVERIFY(GeoFormatRegistry::loadPoints(shpFile, points, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(points.count(), 3);

    // Verify first point coordinates
    QCOMPARE(points[0].longitude(), -122.0);
    QCOMPARE(points[0].latitude(), 37.0);
    QCOMPARE(points[0].altitude(), 100.0);

    // Verify last point coordinates
    QCOMPARE(points[2].longitude(), -121.0);
    QCOMPARE(points[2].latitude(), 38.0);
    QCOMPARE(points[2].altitude(), 200.0);
}

void ShapeTest::_testLoadPointsFromSHPMultiPoint()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = tmpDir.filePath("multipoint.shp");
    const QString prjFile = tmpDir.filePath("multipoint.prj");
    QVERIFY(_writePrjFile(prjFile, QStringLiteral("EPSG:4326")));

    SHPHandle shpHandle = SHPCreate(shpFile.toUtf8().constData(), SHPT_MULTIPOINTZ);
    QVERIFY(shpHandle != nullptr);

    const int nVertices = 3;
    double x[nVertices] = { -122.0, -121.5, -121.0 };
    double y[nVertices] = { 37.0, 37.5, 38.0 };
    double z[nVertices] = { 100.0, 150.0, 200.0 };

    SHPObject *shpObject = SHPCreateObject(SHPT_MULTIPOINTZ,
                                           -1,         // iShape: -1 for new shape
                                           0,          // nParts
                                           nullptr,    // panPartStart
                                           nullptr,    // panPartType
                                           nVertices,
                                           x, y, z,
                                           nullptr);   // padfM
    QVERIFY(shpObject != nullptr);

    const int shapeIndex = SHPWriteObject(shpHandle, -1, shpObject);
    SHPDestroyObject(shpObject);
    SHPClose(shpHandle);
    QVERIFY(shapeIndex >= 0);

    QString errorString;
    QCOMPARE(SHPHelper::determineShapeType(shpFile, errorString), GeoFormatRegistry::ShapeType::Point);
    QVERIFY(errorString.isEmpty());

    QList<QGeoCoordinate> points;
    QVERIFY(SHPHelper::loadPointsFromFile(shpFile, points, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(points.count(), nVertices);
    QVERIFY(std::abs(points[0].longitude() - x[0]) < 1e-9);
    QVERIFY(std::abs(points[0].latitude() - y[0]) < 1e-9);
    QVERIFY(std::abs(points[0].altitude() - z[0]) < 1e-9);
    QVERIFY(std::abs(points[2].longitude() - x[2]) < 1e-9);
    QVERIFY(std::abs(points[2].latitude() - y[2]) < 1e-9);
    QVERIFY(std::abs(points[2].altitude() - z[2]) < 1e-9);

    points.clear();
    QVERIFY(GeoFormatRegistry::loadPoints(shpFile, points, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(points.count(), nVertices);
}

void ShapeTest::_testLoadPointsFromKML()
{
    const QTemporaryDir tmpDir;
    const QString kmlFile = _copyRes(tmpDir, "point.kml");
    QString errorString;
    QList<QGeoCoordinate> points;
    QVERIFY(GeoFormatRegistry::loadPoints(kmlFile, points, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(points.count(), 3);

    // Verify first point coordinates
    QCOMPARE(points[0].longitude(), -122.0);
    QCOMPARE(points[0].latitude(), 37.0);
    QCOMPARE(points[0].altitude(), 100.0);

    // Verify last point coordinates
    QCOMPARE(points[2].longitude(), -121.0);
    QCOMPARE(points[2].latitude(), 38.0);
    QCOMPARE(points[2].altitude(), 200.0);
}

void ShapeTest::_testLoadPolygonsFromSHP()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile));
    QString errorString;
    QList<QList<QGeoCoordinate>> polygons;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(GeoFormatRegistry::loadPolygons(shpFile, polygons, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polygons.count() >= 1);
    QVERIFY(polygons.first().count() >= 3);
}

void ShapeTest::_testLoadPolylinesFromSHP()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "pline", shpFile));
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(GeoFormatRegistry::loadPolylines(shpFile, polylines, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polylines.count() >= 1);
    QVERIFY(polylines.first().count() >= 2);
}

void ShapeTest::_testLoadPolygonsFromKML()
{
    const QTemporaryDir tmpDir;
    const QString kmlFile = _copyRes(tmpDir, "polygon.kml");
    QString errorString;
    QList<QList<QGeoCoordinate>> polygons;
    QVERIFY(GeoFormatRegistry::loadPolygons(kmlFile, polygons, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polygons.count() >= 1);
    QVERIFY(polygons.first().count() >= 3);
}

void ShapeTest::_testLoadPolylinesFromKML()
{
    const QTemporaryDir tmpDir;
    const QString kmlFile = _copyRes(tmpDir, "polyline.kml");
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    QVERIFY(GeoFormatRegistry::loadPolylines(kmlFile, polylines, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polylines.count() >= 1);
    QVERIFY(polylines.first().count() >= 2);
}

void ShapeTest::_testGetEntityCount()
{
    const QTemporaryDir tmpDir;

    // Test SHP entity count
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile));
    QString errorString;
    const int shpCount = GeoFormatRegistry::getEntityCount(shpFile, errorString);
    QVERIFY(errorString.isEmpty());
    QVERIFY(shpCount >= 1);

    // Test KML polygon entity count
    const QString kmlPolygonFile = _copyRes(tmpDir, "polygon.kml");
    const int kmlPolygonCount = GeoFormatRegistry::getEntityCount(kmlPolygonFile, errorString);
    QVERIFY(errorString.isEmpty());
    QCOMPARE(kmlPolygonCount, 1);

    // Test KML polyline entity count
    const QString kmlPolylineFile = _copyRes(tmpDir, "polyline.kml");
    const int kmlPolylineCount = GeoFormatRegistry::getEntityCount(kmlPolylineFile, errorString);
    QVERIFY(errorString.isEmpty());
    QCOMPARE(kmlPolylineCount, 1);
}

void ShapeTest::_testDetermineShapeType()
{
    const QTemporaryDir tmpDir;

    // Test polygon type detection
    QString polygonFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", polygonFile));
    QString errorString;
    QCOMPARE(GeoFormatRegistry::determineShapeType(polygonFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());

    // Test polyline type detection
    QString polylineFile;
    QVERIFY(_copyShapefileSet(tmpDir, "pline", polylineFile));
    QCOMPARE(GeoFormatRegistry::determineShapeType(polylineFile, errorString), GeoFormatRegistry::ShapeType::Polyline);
    QVERIFY(errorString.isEmpty());

    // Test KML type detection
    const QString kmlFile = _copyRes(tmpDir, "polygon.kml");
    QCOMPARE(GeoFormatRegistry::determineShapeType(kmlFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());

    // Test GPX track type detection
    const QString gpxTrackContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <trk>
    <name>Track for Shape Detection</name>
    <trkseg>
      <trkpt lat="37.0" lon="-122.0"></trkpt>
      <trkpt lat="37.1" lon="-122.1"></trkpt>
    </trkseg>
  </trk>
</gpx>)";
    const QString gpxTrackFile = _writeGpxFile(tmpDir, "shape_type_track.gpx", gpxTrackContent);
    QCOMPARE(GeoFormatRegistry::determineShapeType(gpxTrackFile, errorString), GeoFormatRegistry::ShapeType::Track);
    QVERIFY(errorString.isEmpty());

    // Test OpenAir type detection
    const QString openAirContent = QStringLiteral(
        "AC R\n"
        "AN Shape Type Airspace\n"
        "AH 5000FT MSL\n"
        "AL GND\n"
        "DP 47:00:00N 008:00:00E\n"
        "DP 47:00:00N 009:00:00E\n"
        "DP 46:00:00N 009:00:00E\n"
        "DP 46:00:00N 008:00:00E\n");
    const QString openAirFile = tmpDir.filePath("shape_type.air");
    QVERIFY(_writeTextFile(openAirFile, openAirContent));
    QCOMPARE(GeoFormatRegistry::determineShapeType(openAirFile, errorString), GeoFormatRegistry::ShapeType::Airspace);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testProjProjectionSupport()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile, false));

    // Use a non-WGS84/non-UTM CRS so SHP loading must use PROJ transformation path.
    const QString prjPath = tmpDir.filePath("polygon.prj");
    QVERIFY(_writePrjFile(prjPath, "EPSG:3857"));

    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(shpFile, rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testProjInvalidCrsError()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile, false));

    const QString prjPath = tmpDir.filePath("polygon.prj");
    QVERIFY(_writePrjFile(prjPath, "EPSG:999999"));

    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(!GeoFormatRegistry::loadPolygon(shpFile, rgCoords, errorString));
    QVERIFY(!errorString.isEmpty());
    QVERIFY(errorString.contains("Unsupported projection", Qt::CaseInsensitive));
}

void ShapeTest::_testLoadFromQtResource()
{
    // Test loading directly from Qt resource path (tests QFile hooks)
    // Note: This requires the test resources to include all required files (.shp, .shx, .prj)
    QString errorString;
    QList<QGeoCoordinate> rgCoords;

    // Test KML from resource (KML already uses QFile internally)
    QVERIFY(GeoFormatRegistry::loadPolygon(":/unittest/polygon.kml", rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testVertexFiltering()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile));

    QString errorString;
    QList<QGeoCoordinate> unfilteredCoords;
    QList<QGeoCoordinate> filteredCoords;

    // Load without filtering
    QVERIFY(GeoFormatRegistry::loadPolygon(shpFile, unfilteredCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());

    // Load with filtering (default 5m)
    QVERIFY(GeoFormatRegistry::loadPolygon(shpFile, filteredCoords, errorString));
    QVERIFY(errorString.isEmpty());

    // Filtered should have same or fewer vertices than unfiltered
    QVERIFY(filteredCoords.count() <= unfilteredCoords.count());

    // Both should have at least minimum valid polygon (3 vertices)
    // Note: If the test data has vertices very close together, filtered may have fewer
    QVERIFY(unfilteredCoords.count() >= 3);
}

void ShapeTest::_testKMLVertexFiltering()
{
    const QTemporaryDir tmpDir;

    // Create KML with closely spaced vertices (< 5m apart at equator)
    // 0.00001 degrees ≈ 1.1m at equator
    const QString kmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Placemark>
    <Polygon>
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>
            0.0,0.0,0 0.00001,0.0,0 0.00002,0.0,0 0.00003,0.0,0 0.001,0.001,0 0.0,0.001,0 0.0,0.0,0
          </coordinates>
        </LinearRing>
      </outerBoundaryIs>
    </Polygon>
  </Placemark>
</kml>)";

    const QString kmlFile = _writeKmlFile(tmpDir, "dense_polygon.kml", kmlContent);

    QString errorString;
    QList<QGeoCoordinate> unfilteredCoords;
    QList<QGeoCoordinate> filteredCoords;

    // Load without filtering
    QVERIFY(GeoFormatRegistry::loadPolygon(kmlFile, unfilteredCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(unfilteredCoords.count(), 6);  // 7 coords - 1 duplicate closing = 6

    // Load with filtering (5m threshold)
    QVERIFY(GeoFormatRegistry::loadPolygon(kmlFile, filteredCoords, errorString, GeoFormatRegistry::kDefaultVertexFilterMeters));
    QVERIFY(errorString.isEmpty());

    // Filtered should have fewer vertices (the closely spaced ones should be filtered)
    QVERIFY(filteredCoords.count() < unfilteredCoords.count());
    QVERIFY(filteredCoords.count() >= 3);  // Minimum valid polygon
}

void ShapeTest::_testKMLAltitudeParsing()
{
    const QTemporaryDir tmpDir;

    // Create KML with altitude values
    const QString kmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Placemark>
    <Polygon>
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>
            -122.0,37.0,100.5 -122.0,38.0,200.0 -121.0,38.0,150.0 -121.0,37.0,175.5 -122.0,37.0,100.5
          </coordinates>
        </LinearRing>
      </outerBoundaryIs>
    </Polygon>
  </Placemark>
</kml>)";

    const QString kmlFile = _writeKmlFile(tmpDir, "altitude_polygon.kml", kmlContent);

    QString errorString;
    QList<QGeoCoordinate> coords;
    QVERIFY(GeoFormatRegistry::loadPolygon(kmlFile, coords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(coords.count(), 4);  // 5 coords - 1 duplicate closing = 4

    // Verify altitudes were parsed correctly
    QCOMPARE(coords[0].altitude(), 100.5);
    QCOMPARE(coords[1].altitude(), 200.0);
    QCOMPARE(coords[2].altitude(), 150.0);
    QCOMPARE(coords[3].altitude(), 175.5);

    // Verify lat/lon as well
    QCOMPARE(coords[0].latitude(), 37.0);
    QCOMPARE(coords[0].longitude(), -122.0);
}

void ShapeTest::_testKMLCoordinateValidation()
{
    const QTemporaryDir tmpDir;

    // Create KML with some invalid coordinates mixed with valid ones
    // Invalid: lat=91 (out of range), lon=200 (out of range), swapped lat/lon
    const QString kmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Placemark>
    <Polygon>
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>
            -122.0,37.0,0 -122.0,91.0,0 200.0,38.0,0 -121.0,38.0,0 -121.0,37.0,0 -122.0,37.0,0
          </coordinates>
        </LinearRing>
      </outerBoundaryIs>
    </Polygon>
  </Placemark>
</kml>)";

    const QString kmlFile = _writeKmlFile(tmpDir, "invalid_coords.kml", kmlContent);

    // Expect warnings for invalid coordinates that will be normalized
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Latitude out of range.*91"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Longitude out of range.*200"));

    QString errorString;
    QList<QGeoCoordinate> coords;
    // Should succeed and normalize invalid coordinates (clamp lat, wrap lon)
    QVERIFY(GeoFormatRegistry::loadPolygon(kmlFile, coords, errorString, 0));
    QVERIFY(errorString.isEmpty());

    // Should have 5 coordinates (all normalized, duplicate closing removed, then reversed for clockwise winding)
    // Original KML: -122,37 | -122,91 | 200,38 | -121,38 | -121,37 | -122,37 (closing)
    // After normalize: -122,37 | -122,90 | -160,38 | -121,38 | -121,37 (closing removed)
    // After clockwise winding check (reversed): -121,37 | -121,38 | -160,38 | -122,90 | -122,37
    QCOMPARE(coords.count(), 5);

    // Verify the coordinates are present, normalized, and in clockwise order
    QCOMPARE(coords[0].latitude(), 37.0);
    QCOMPARE(coords[0].longitude(), -121.0);
    QCOMPARE(coords[1].latitude(), 38.0);
    QCOMPARE(coords[1].longitude(), -121.0);
    QCOMPARE(coords[2].latitude(), 38.0);
    QCOMPARE(coords[2].longitude(), -160.0);  // Wrapped from 200
    QCOMPARE(coords[3].latitude(), 90.0);     // Clamped from 91
    QCOMPARE(coords[3].longitude(), -122.0);
    QCOMPARE(coords[4].latitude(), 37.0);
    QCOMPARE(coords[4].longitude(), -122.0);
}

void ShapeTest::_testKMLExportSchemaValidation()
{
    // Test that KMLSchemaValidator is properly loaded and functional
    const auto *validator = KMLSchemaValidator::instance();

    // Skip test if schema resource isn't available
    if (!validator->isLoaded()) {
        QSKIP("KML schema resource not available - skipping schema validation test");
    }

    // Verify schema was loaded and enum types were extracted
    const QStringList altitudeModes = validator->enumValues("altitudeModeEnumType");
    QVERIFY(!altitudeModes.isEmpty());
    QVERIFY(altitudeModes.contains("absolute"));
    QVERIFY(altitudeModes.contains("clampToGround"));
    QVERIFY(altitudeModes.contains("relativeToGround"));

    // Verify element validation works
    QVERIFY(validator->isValidElement("Polygon"));
    QVERIFY(validator->isValidElement("LineString"));
    QVERIFY(validator->isValidElement("Point"));
    QVERIFY(validator->isValidElement("coordinates"));
    QVERIFY(validator->isValidElement("altitudeMode"));

    // Create a KML document and validate it
    KMLDomDocument doc("Test Export");
    QDomElement placemark = doc.addPlacemark("Test Point", true);
    QDomElement point = doc.createElement("Point");
    doc.addTextElement(point, "altitudeMode", "absolute");
    doc.addTextElement(point, "coordinates", doc.kmlCoordString(QGeoCoordinate(37.0, -122.0, 100.0)));
    placemark.appendChild(point);

    // Validate the document
    const auto result = validator->validate(doc);
    QVERIFY2(result.isValid, qPrintable(QStringLiteral("KML validation errors: %1").arg(result.errors.join(QStringLiteral("; ")))));
    QVERIFY(result.errors.isEmpty());

    // Test validation catches invalid altitudeMode
    const QString invalidKml = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <Placemark>
      <Point>
        <altitudeMode>invalidMode</altitudeMode>
        <coordinates>-122.0,37.0,0</coordinates>
      </Point>
    </Placemark>
  </Document>
</kml>)";

    const QTemporaryDir tmpDir;
    const QString invalidKmlPath = _writeKmlFile(tmpDir, "invalid.kml", invalidKml);
    const auto invalidResult = validator->validateFile(invalidKmlPath);
    QVERIFY(!invalidResult.isValid);
    QVERIFY(invalidResult.errors.first().contains("invalidMode"));

    // Test validation catches out-of-range coordinates
    const QString badCoordsKml = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <Placemark>
      <Point>
        <coordinates>200.0,95.0,0</coordinates>
      </Point>
    </Placemark>
  </Document>
</kml>)";

    const QString badCoordsPath = _writeKmlFile(tmpDir, "bad_coords.kml", badCoordsKml);
    const auto badCoordsResult = validator->validateFile(badCoordsPath);
    QVERIFY(!badCoordsResult.isValid);
    QVERIFY(badCoordsResult.errors.size() >= 2);  // lat and lon both out of range
}

// ============================================================================
// GeoJSON Tests
// ============================================================================

QString ShapeTest::_writeGeoJsonFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content)
{
    const QString path = tmpDir.filePath(name);
    if (!_writeTextFile(path, content)) {
        qFatal("Failed to write GeoJSON test file: %s", qPrintable(path));
    }
    return path;
}

void ShapeTest::_testLoadPolygonFromGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polygon
    QList<QGeoCoordinate> originalVertices;
    originalVertices.append(QGeoCoordinate(37.0, -122.0));
    originalVertices.append(QGeoCoordinate(38.0, -122.0));
    originalVertices.append(QGeoCoordinate(38.0, -121.0));
    originalVertices.append(QGeoCoordinate(37.0, -121.0));

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(geoJsonFile, originalVertices, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back via GeoFormatRegistry
    QList<QGeoCoordinate> loadedCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(geoJsonFile, loadedCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedCoords.count(), 4);

    // Verify coordinates
    QCOMPARE(loadedCoords[0].latitude(), 37.0);
    QCOMPARE(loadedCoords[0].longitude(), -122.0);
}

void ShapeTest::_testLoadPolylineFromGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polyline
    QList<QGeoCoordinate> originalCoords;
    originalCoords.append(QGeoCoordinate(37.0, -122.0));
    originalCoords.append(QGeoCoordinate(37.5, -122.5));
    originalCoords.append(QGeoCoordinate(38.0, -123.0));

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("polyline.geojson");
    QVERIFY(GeoJsonHelper::savePolylineToFile(geoJsonFile, originalCoords, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back via GeoFormatRegistry
    QList<QGeoCoordinate> loadedCoords;
    QVERIFY(GeoFormatRegistry::loadPolyline(geoJsonFile, loadedCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedCoords.count(), 3);

    QCOMPARE(loadedCoords[0].latitude(), 37.0);
    QCOMPARE(loadedCoords[0].longitude(), -122.0);
    QCOMPARE(loadedCoords[2].latitude(), 38.0);
    QCOMPARE(loadedCoords[2].longitude(), -123.0);
}

void ShapeTest::_testLoadPolygonsFromGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create two polygons
    QList<QGeoCoordinate> poly1;
    poly1.append(QGeoCoordinate(37.0, -122.0));
    poly1.append(QGeoCoordinate(38.0, -122.0));
    poly1.append(QGeoCoordinate(38.0, -121.0));
    poly1.append(QGeoCoordinate(37.0, -121.0));

    QList<QGeoCoordinate> poly2;
    poly2.append(QGeoCoordinate(35.0, -120.0));
    poly2.append(QGeoCoordinate(36.0, -120.0));
    poly2.append(QGeoCoordinate(36.0, -119.0));
    poly2.append(QGeoCoordinate(35.0, -119.0));

    QList<QList<QGeoCoordinate>> originalPolygons;
    originalPolygons.append(poly1);
    originalPolygons.append(poly2);

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("polygons.geojson");
    QVERIFY(GeoJsonHelper::savePolygonsToFile(geoJsonFile, originalPolygons, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back
    QList<QList<QGeoCoordinate>> loadedPolygons;
    QVERIFY(GeoFormatRegistry::loadPolygons(geoJsonFile, loadedPolygons, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPolygons.count(), 2);
    QCOMPARE(loadedPolygons[0].count(), 4);
    QCOMPARE(loadedPolygons[1].count(), 4);

    // Verify first polygon
    QCOMPARE(loadedPolygons[0][0].latitude(), 37.0);
    QCOMPARE(loadedPolygons[0][0].longitude(), -122.0);

    // Verify second polygon
    QCOMPARE(loadedPolygons[1][0].latitude(), 35.0);
    QCOMPARE(loadedPolygons[1][0].longitude(), -120.0);
}

void ShapeTest::_testLoadPolylinesFromGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create two polylines
    QList<QGeoCoordinate> line1;
    line1.append(QGeoCoordinate(37.0, -122.0));
    line1.append(QGeoCoordinate(37.5, -122.5));

    QList<QGeoCoordinate> line2;
    line2.append(QGeoCoordinate(35.0, -120.0));
    line2.append(QGeoCoordinate(35.5, -120.5));
    line2.append(QGeoCoordinate(36.0, -121.0));

    QList<QList<QGeoCoordinate>> originalPolylines;
    originalPolylines.append(line1);
    originalPolylines.append(line2);

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("polylines.geojson");
    QVERIFY(GeoJsonHelper::savePolylinesToFile(geoJsonFile, originalPolylines, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back
    QList<QList<QGeoCoordinate>> loadedPolylines;
    QVERIFY(GeoFormatRegistry::loadPolylines(geoJsonFile, loadedPolylines, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPolylines.count(), 2);
    QCOMPARE(loadedPolylines[0].count(), 2);
    QCOMPARE(loadedPolylines[1].count(), 3);
}

void ShapeTest::_testLoadPointsFromGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create points
    QList<QGeoCoordinate> originalPoints;
    originalPoints.append(QGeoCoordinate(37.0, -122.0));
    originalPoints.append(QGeoCoordinate(37.5, -121.5, 100.0));

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("points.geojson");
    QVERIFY(GeoJsonHelper::savePointsToFile(geoJsonFile, originalPoints, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(GeoFormatRegistry::loadPoints(geoJsonFile, loadedPoints, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPoints.count(), 2);

    QCOMPARE(loadedPoints[0].latitude(), 37.0);
    QCOMPARE(loadedPoints[0].longitude(), -122.0);

    QCOMPARE(loadedPoints[1].latitude(), 37.5);
    QCOMPARE(loadedPoints[1].longitude(), -121.5);
    QCOMPARE(loadedPoints[1].altitude(), 100.0);
}

void ShapeTest::_testGeoJSONVertexFiltering()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create polygon with closely spaced vertices (< 5m apart at equator)
    // 0.00001 degrees ≈ 1.1m at equator
    QList<QGeoCoordinate> denseVertices;
    denseVertices.append(QGeoCoordinate(0.0, 0.0));
    denseVertices.append(QGeoCoordinate(0.0, 0.00001));
    denseVertices.append(QGeoCoordinate(0.0, 0.00002));
    denseVertices.append(QGeoCoordinate(0.0, 0.00003));
    denseVertices.append(QGeoCoordinate(0.001, 0.001));
    denseVertices.append(QGeoCoordinate(0.001, 0.0));

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("dense_polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(geoJsonFile, denseVertices, errorString));
    QVERIFY(errorString.isEmpty());

    // Load without filtering
    QList<QGeoCoordinate> unfilteredCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(geoJsonFile, unfilteredCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(unfilteredCoords.count(), 6);

    // Load with filtering (5m threshold)
    QList<QGeoCoordinate> filteredCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(geoJsonFile, filteredCoords, errorString, GeoFormatRegistry::kDefaultVertexFilterMeters));
    QVERIFY(errorString.isEmpty());

    // Filtered should have fewer vertices
    QVERIFY(filteredCoords.count() < unfilteredCoords.count());
    QVERIFY(filteredCoords.count() >= 3);  // Minimum valid polygon
}

void ShapeTest::_testGeoJSONAltitudeParsing()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create polygon with altitude values
    QList<QGeoCoordinate> originalVertices;
    originalVertices.append(QGeoCoordinate(37.0, -122.0, 100.5));
    originalVertices.append(QGeoCoordinate(38.0, -122.0, 200.0));
    originalVertices.append(QGeoCoordinate(38.0, -121.0, 150.0));
    originalVertices.append(QGeoCoordinate(37.0, -121.0, 175.5));

    // Save to file
    const QString geoJsonFile = tmpDir.filePath("altitude_polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(geoJsonFile, originalVertices, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back
    QList<QGeoCoordinate> loadedCoords;
    QVERIFY(GeoFormatRegistry::loadPolygon(geoJsonFile, loadedCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedCoords.count(), 4);

    // Verify altitudes were preserved
    QCOMPARE(loadedCoords[0].altitude(), 100.5);
    QCOMPARE(loadedCoords[1].altitude(), 200.0);
    QCOMPARE(loadedCoords[2].altitude(), 150.0);
    QCOMPARE(loadedCoords[3].altitude(), 175.5);

    // Verify lat/lon
    QCOMPARE(loadedCoords[0].latitude(), 37.0);
    QCOMPARE(loadedCoords[0].longitude(), -122.0);
}

void ShapeTest::_testGeoJSONSaveFunctions()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Test save polygon
    QList<QGeoCoordinate> polygonVertices;
    polygonVertices.append(QGeoCoordinate(37.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -121.0));
    polygonVertices.append(QGeoCoordinate(37.0, -121.0));

    const QString savedPolygonFile = tmpDir.filePath("saved_polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(savedPolygonFile, polygonVertices, errorString));
    QVERIFY(errorString.isEmpty());

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolygon;
    QVERIFY(GeoFormatRegistry::loadPolygon(savedPolygonFile, loadedPolygon, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPolygon.count(), polygonVertices.count());

    // Test save polyline
    QList<QGeoCoordinate> polylineCoords;
    polylineCoords.append(QGeoCoordinate(37.0, -122.0));
    polylineCoords.append(QGeoCoordinate(37.5, -122.5));
    polylineCoords.append(QGeoCoordinate(38.0, -123.0));

    const QString savedPolylineFile = tmpDir.filePath("saved_polyline.geojson");
    QVERIFY(GeoJsonHelper::savePolylineToFile(savedPolylineFile, polylineCoords, errorString));
    QVERIFY(errorString.isEmpty());

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(GeoFormatRegistry::loadPolyline(savedPolylineFile, loadedPolyline, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPolyline.count(), polylineCoords.count());

    // Test save points
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(37.0, -122.0, 100.0));
    points.append(QGeoCoordinate(38.0, -121.0, 200.0));

    const QString savedPointsFile = tmpDir.filePath("saved_points.geojson");
    QVERIFY(GeoJsonHelper::savePointsToFile(savedPointsFile, points, errorString));
    QVERIFY(errorString.isEmpty());

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(GeoFormatRegistry::loadPoints(savedPointsFile, loadedPoints, errorString));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(loadedPoints.count(), points.count());
    QCOMPARE(loadedPoints[0].altitude(), 100.0);
}

void ShapeTest::_testGeoJSONDetermineShapeType()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Test polygon detection
    QList<QGeoCoordinate> polygonVertices;
    polygonVertices.append(QGeoCoordinate(37.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -121.0));
    polygonVertices.append(QGeoCoordinate(37.0, -121.0));

    const QString polygonFile = tmpDir.filePath("polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(polygonFile, polygonVertices, errorString));
    QCOMPARE(GeoFormatRegistry::determineShapeType(polygonFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());

    // Test polyline detection
    QList<QGeoCoordinate> polylineCoords;
    polylineCoords.append(QGeoCoordinate(37.0, -122.0));
    polylineCoords.append(QGeoCoordinate(38.0, -123.0));

    const QString polylineFile = tmpDir.filePath("polyline.geojson");
    QVERIFY(GeoJsonHelper::savePolylineToFile(polylineFile, polylineCoords, errorString));
    QCOMPARE(GeoFormatRegistry::determineShapeType(polylineFile, errorString), GeoFormatRegistry::ShapeType::Polyline);
    QVERIFY(errorString.isEmpty());

    // Test point detection
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(37.0, -122.0));

    const QString pointFile = tmpDir.filePath("point.geojson");
    QVERIFY(GeoJsonHelper::savePointsToFile(pointFile, points, errorString));
    QCOMPARE(GeoFormatRegistry::determineShapeType(pointFile, errorString), GeoFormatRegistry::ShapeType::Point);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testSHPSaveFunctions()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // ========================================================================
    // Test save/load polygon round-trip
    // ========================================================================
    QList<QGeoCoordinate> polygonVertices;
    polygonVertices.append(QGeoCoordinate(37.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -121.0));
    polygonVertices.append(QGeoCoordinate(37.0, -121.0));

    const QString savedPolygonFile = tmpDir.filePath("saved_polygon.shp");
    QVERIFY(SHPHelper::savePolygonToFile(savedPolygonFile, polygonVertices, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify PRJ file was created
    const QString prjFile = tmpDir.filePath("saved_polygon.prj");
    QVERIFY(QFile::exists(prjFile));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolygon;
    QVERIFY(SHPHelper::loadPolygonFromFile(savedPolygonFile, loadedPolygon, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolygon.count(), polygonVertices.count());

    // Verify coordinates
    for (int i = 0; i < polygonVertices.count(); i++) {
        QVERIFY(qAbs(loadedPolygon[i].latitude() - polygonVertices[i].latitude()) < 0.0001);
        QVERIFY(qAbs(loadedPolygon[i].longitude() - polygonVertices[i].longitude()) < 0.0001);
    }

    // ========================================================================
    // Test save/load polyline round-trip
    // ========================================================================
    QList<QGeoCoordinate> polylineCoords;
    polylineCoords.append(QGeoCoordinate(37.0, -122.0));
    polylineCoords.append(QGeoCoordinate(37.5, -122.5));
    polylineCoords.append(QGeoCoordinate(38.0, -123.0));

    const QString savedPolylineFile = tmpDir.filePath("saved_polyline.shp");
    QVERIFY(SHPHelper::savePolylineToFile(savedPolylineFile, polylineCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(SHPHelper::loadPolylineFromFile(savedPolylineFile, loadedPolyline, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolyline.count(), polylineCoords.count());

    // ========================================================================
    // Test save/load points round-trip
    // ========================================================================
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(37.0, -122.0));
    points.append(QGeoCoordinate(38.0, -121.0));
    points.append(QGeoCoordinate(39.0, -120.0));

    const QString savedPointsFile = tmpDir.filePath("saved_points.shp");
    QVERIFY(SHPHelper::savePointsToFile(savedPointsFile, points, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(SHPHelper::loadPointsFromFile(savedPointsFile, loadedPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPoints.count(), points.count());

    // ========================================================================
    // Test save/load with altitude (Z values)
    // ========================================================================
    QList<QGeoCoordinate> polygonWithAlt;
    polygonWithAlt.append(QGeoCoordinate(37.0, -122.0, 100.0));
    polygonWithAlt.append(QGeoCoordinate(38.0, -122.0, 200.0));
    polygonWithAlt.append(QGeoCoordinate(38.0, -121.0, 150.0));
    polygonWithAlt.append(QGeoCoordinate(37.0, -121.0, 175.0));

    const QString savedPolygonZFile = tmpDir.filePath("saved_polygonz.shp");
    QVERIFY(SHPHelper::savePolygonToFile(savedPolygonZFile, polygonWithAlt, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back - altitudes should be preserved
    QList<QGeoCoordinate> loadedPolygonZ;
    QVERIFY(SHPHelper::loadPolygonFromFile(savedPolygonZFile, loadedPolygonZ, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolygonZ.count(), polygonWithAlt.count());

    // Verify altitudes
    QCOMPARE(loadedPolygonZ[0].altitude(), 100.0);
    QCOMPARE(loadedPolygonZ[1].altitude(), 200.0);
    QCOMPARE(loadedPolygonZ[2].altitude(), 150.0);
    QCOMPARE(loadedPolygonZ[3].altitude(), 175.0);

    // ========================================================================
    // Test multiple polygons
    // ========================================================================
    QList<QList<QGeoCoordinate>> multiPolygons;
    QList<QGeoCoordinate> poly1;
    poly1.append(QGeoCoordinate(37.0, -122.0));
    poly1.append(QGeoCoordinate(37.5, -122.0));
    poly1.append(QGeoCoordinate(37.5, -121.5));
    multiPolygons.append(poly1);

    QList<QGeoCoordinate> poly2;
    poly2.append(QGeoCoordinate(38.0, -123.0));
    poly2.append(QGeoCoordinate(38.5, -123.0));
    poly2.append(QGeoCoordinate(38.5, -122.5));
    multiPolygons.append(poly2);

    const QString savedMultiPolyFile = tmpDir.filePath("saved_multi_polygon.shp");
    QVERIFY(SHPHelper::savePolygonsToFile(savedMultiPolyFile, multiPolygons, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QList<QGeoCoordinate>> loadedMultiPolygons;
    QVERIFY(SHPHelper::loadPolygonsFromFile(savedMultiPolyFile, loadedMultiPolygons, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedMultiPolygons.count(), 2);

    // ========================================================================
    // Test determineShapeType on saved files
    // ========================================================================
    QCOMPARE(SHPHelper::determineShapeType(savedPolygonFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QCOMPARE(SHPHelper::determineShapeType(savedPolylineFile, errorString), GeoFormatRegistry::ShapeType::Polyline);
    QCOMPARE(SHPHelper::determineShapeType(savedPointsFile, errorString), GeoFormatRegistry::ShapeType::Point);
}

void ShapeTest::_testSHPMissingPrjFile()
{
    const QTemporaryDir tmpDir;
    QString shpFile;
    QVERIFY(_copyShapefileSet(tmpDir, "polygon", shpFile, false));
    // Intentionally do not copy polygon.prj

    QString errorString;
    QList<QGeoCoordinate> coords;
    QVERIFY(!GeoFormatRegistry::loadPolygon(shpFile, coords, errorString, 0));
    QVERIFY(errorString.contains("File not found", Qt::CaseInsensitive));
    QVERIFY(errorString.contains(".prj", Qt::CaseInsensitive));
}

void ShapeTest::_testSHPMissingShpFile()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = tmpDir.filePath("missing.shp");
    const QString prjFile = tmpDir.filePath("missing.prj");
    QVERIFY(QFile(QStringLiteral(":/unittest/polygon.prj")).copy(prjFile));

    QString errorString;
    QCOMPARE(GeoFormatRegistry::determineShapeType(shpFile, errorString), GeoFormatRegistry::ShapeType::Error);
    QVERIFY(!errorString.isEmpty());

    errorString.clear();
    QCOMPARE(GeoFormatRegistry::getEntityCount(shpFile, errorString), 0);
    QVERIFY(!errorString.isEmpty());
}

void ShapeTest::_testSHPSavePolylinesValidationErrorContext()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = tmpDir.filePath("invalid_polylines.shp");

    QList<QList<QGeoCoordinate>> polylines;
    QList<QGeoCoordinate> polyline;
    polyline.append(QGeoCoordinate(95.0, -122.0));  // Invalid latitude
    polyline.append(QGeoCoordinate(38.0, -121.0));
    polylines.append(polyline);

    QString errorString;
    QVERIFY(!SHPHelper::savePolylinesToFile(shpFile, polylines, errorString));
    QVERIFY(errorString.contains("Polyline 1", Qt::CaseInsensitive));
}

void ShapeTest::_testKMLSaveFunctions()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // ========================================================================
    // Test save/load polygon round-trip
    // ========================================================================
    QList<QGeoCoordinate> polygonVertices;
    polygonVertices.append(QGeoCoordinate(37.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -121.0));
    polygonVertices.append(QGeoCoordinate(37.0, -121.0));

    const QString savedPolygonFile = tmpDir.filePath("saved_polygon.kml");
    QVERIFY(KMLHelper::savePolygonToFile(savedPolygonFile, polygonVertices, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolygon;
    QVERIFY(KMLHelper::loadPolygonFromFile(savedPolygonFile, loadedPolygon, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolygon.count(), polygonVertices.count());

    // ========================================================================
    // Test save/load polyline round-trip
    // ========================================================================
    QList<QGeoCoordinate> polylineCoords;
    polylineCoords.append(QGeoCoordinate(37.0, -122.0));
    polylineCoords.append(QGeoCoordinate(37.5, -122.5));
    polylineCoords.append(QGeoCoordinate(38.0, -123.0));

    const QString savedPolylineFile = tmpDir.filePath("saved_polyline.kml");
    QVERIFY(KMLHelper::savePolylineToFile(savedPolylineFile, polylineCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(KMLHelper::loadPolylineFromFile(savedPolylineFile, loadedPolyline, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolyline.count(), polylineCoords.count());

    // ========================================================================
    // Test save/load points round-trip
    // ========================================================================
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(37.0, -122.0));
    points.append(QGeoCoordinate(38.0, -121.0));
    points.append(QGeoCoordinate(39.0, -120.0));

    const QString savedPointsFile = tmpDir.filePath("saved_points.kml");
    QVERIFY(KMLHelper::savePointsToFile(savedPointsFile, points, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(KMLHelper::loadPointsFromFile(savedPointsFile, loadedPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPoints.count(), points.count());

    // ========================================================================
    // Test save/load with altitude
    // ========================================================================
    QList<QGeoCoordinate> polygonWithAlt;
    polygonWithAlt.append(QGeoCoordinate(37.0, -122.0, 100.0));
    polygonWithAlt.append(QGeoCoordinate(38.0, -122.0, 200.0));
    polygonWithAlt.append(QGeoCoordinate(38.0, -121.0, 150.0));
    polygonWithAlt.append(QGeoCoordinate(37.0, -121.0, 175.0));

    const QString savedPolygonAltFile = tmpDir.filePath("saved_polygon_alt.kml");
    QVERIFY(KMLHelper::savePolygonToFile(savedPolygonAltFile, polygonWithAlt, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back - altitudes should be preserved
    QList<QGeoCoordinate> loadedPolygonAlt;
    QVERIFY(KMLHelper::loadPolygonFromFile(savedPolygonAltFile, loadedPolygonAlt, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolygonAlt.count(), polygonWithAlt.count());

    // Verify altitudes
    QCOMPARE(loadedPolygonAlt[0].altitude(), 100.0);
    QCOMPARE(loadedPolygonAlt[1].altitude(), 200.0);
    QCOMPARE(loadedPolygonAlt[2].altitude(), 150.0);
    QCOMPARE(loadedPolygonAlt[3].altitude(), 175.0);

    // ========================================================================
    // Test multiple polygons
    // ========================================================================
    QList<QList<QGeoCoordinate>> multiPolygons;
    QList<QGeoCoordinate> poly1;
    poly1.append(QGeoCoordinate(37.0, -122.0));
    poly1.append(QGeoCoordinate(37.5, -122.0));
    poly1.append(QGeoCoordinate(37.5, -121.5));
    multiPolygons.append(poly1);

    QList<QGeoCoordinate> poly2;
    poly2.append(QGeoCoordinate(38.0, -123.0));
    poly2.append(QGeoCoordinate(38.5, -123.0));
    poly2.append(QGeoCoordinate(38.5, -122.5));
    multiPolygons.append(poly2);

    const QString savedMultiPolyFile = tmpDir.filePath("saved_multi_polygon.kml");
    QVERIFY(KMLHelper::savePolygonsToFile(savedMultiPolyFile, multiPolygons, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QList<QGeoCoordinate>> loadedMultiPolygons;
    QVERIFY(KMLHelper::loadPolygonsFromFile(savedMultiPolyFile, loadedMultiPolygons, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedMultiPolygons.count(), 2);

    // ========================================================================
    // Test determineShapeType on saved files
    // ========================================================================
    QCOMPARE(KMLHelper::determineShapeType(savedPolygonFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QCOMPARE(KMLHelper::determineShapeType(savedPolylineFile, errorString), GeoFormatRegistry::ShapeType::Polyline);
    QCOMPARE(KMLHelper::determineShapeType(savedPointsFile, errorString), GeoFormatRegistry::ShapeType::Point);
}

// ============================================================================
// GPX Tests
// ============================================================================

QString ShapeTest::_writeGpxFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content)
{
    const QString path = tmpDir.filePath(name);
    if (!_writeTextFile(path, content)) {
        qFatal("Failed to write GPX test file: %s", qPrintable(path));
    }
    return path;
}

void ShapeTest::_testLoadPolylineFromGPX()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with a route
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <rte>
    <name>Test Route</name>
    <rtept lat="37.0" lon="-122.0"><ele>100</ele></rtept>
    <rtept lat="37.5" lon="-122.5"><ele>150</ele></rtept>
    <rtept lat="38.0" lon="-123.0"><ele>200</ele></rtept>
  </rte>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "route.gpx", gpxContent);

    QList<QGeoCoordinate> coords;
    QVERIFY(GeoFormatRegistry::loadPolyline(gpxFile, coords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(coords.count(), 3);
    QCOMPARE(coords[0].latitude(), 37.0);
    QCOMPARE(coords[0].longitude(), -122.0);
    QCOMPARE(coords[2].latitude(), 38.0);
}

void ShapeTest::_testLoadPolylinesFromGPX()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with multiple routes and a track
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <rte>
    <name>Route 1</name>
    <rtept lat="37.0" lon="-122.0"></rtept>
    <rtept lat="37.5" lon="-122.5"></rtept>
  </rte>
  <rte>
    <name>Route 2</name>
    <rtept lat="38.0" lon="-123.0"></rtept>
    <rtept lat="38.5" lon="-123.5"></rtept>
  </rte>
  <trk>
    <name>Track 1</name>
    <trkseg>
      <trkpt lat="39.0" lon="-124.0"></trkpt>
      <trkpt lat="39.5" lon="-124.5"></trkpt>
    </trkseg>
  </trk>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "multi.gpx", gpxContent);

    QList<QList<QGeoCoordinate>> polylines;
    QVERIFY(GeoFormatRegistry::loadPolylines(gpxFile, polylines, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(polylines.count(), 3);  // 2 routes + 1 track segment
}

void ShapeTest::_testLoadPolygonFromGPX()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with a closed route (polygon)
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <rte>
    <name>Closed Route</name>
    <rtept lat="37.0" lon="-122.0"></rtept>
    <rtept lat="38.0" lon="-122.0"></rtept>
    <rtept lat="38.0" lon="-121.0"></rtept>
    <rtept lat="37.0" lon="-121.0"></rtept>
    <rtept lat="37.0" lon="-122.0"></rtept>
  </rte>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "polygon.gpx", gpxContent);

    QList<QGeoCoordinate> vertices;
    QVERIFY(GeoFormatRegistry::loadPolygon(gpxFile, vertices, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(vertices.count(), 4);  // Last point (duplicate of first) is removed
}

void ShapeTest::_testLoadPointsFromGPX()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with waypoints
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <wpt lat="37.0" lon="-122.0">
    <ele>100</ele>
    <name>WPT001</name>
  </wpt>
  <wpt lat="38.0" lon="-121.0">
    <ele>200</ele>
    <name>WPT002</name>
  </wpt>
  <wpt lat="39.0" lon="-120.0">
    <name>WPT003</name>
  </wpt>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "waypoints.gpx", gpxContent);

    QList<QGeoCoordinate> points;
    QVERIFY(GeoFormatRegistry::loadPoints(gpxFile, points, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(points.count(), 3);
    QCOMPARE(points[0].latitude(), 37.0);
    QCOMPARE(points[0].altitude(), 100.0);
    QCOMPARE(points[1].altitude(), 200.0);
    QVERIFY(std::isnan(points[2].altitude()));  // No elevation specified
}

void ShapeTest::_testGPXSaveFunctions()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // ========================================================================
    // Test save/load polygon round-trip
    // ========================================================================
    QList<QGeoCoordinate> polygonVertices;
    polygonVertices.append(QGeoCoordinate(37.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -122.0));
    polygonVertices.append(QGeoCoordinate(38.0, -121.0));
    polygonVertices.append(QGeoCoordinate(37.0, -121.0));

    const QString savedPolygonFile = tmpDir.filePath("saved_polygon.gpx");
    QVERIFY(GPXHelper::savePolygonToFile(savedPolygonFile, polygonVertices, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolygon;
    QVERIFY(GeoFormatRegistry::loadPolygon(savedPolygonFile, loadedPolygon, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolygon.count(), polygonVertices.count());

    // ========================================================================
    // Test save/load polyline round-trip
    // ========================================================================
    QList<QGeoCoordinate> polylineCoords;
    polylineCoords.append(QGeoCoordinate(37.0, -122.0));
    polylineCoords.append(QGeoCoordinate(37.5, -122.5));
    polylineCoords.append(QGeoCoordinate(38.0, -123.0));

    const QString savedPolylineFile = tmpDir.filePath("saved_polyline.gpx");
    QVERIFY(GPXHelper::savePolylineToFile(savedPolylineFile, polylineCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(GeoFormatRegistry::loadPolyline(savedPolylineFile, loadedPolyline, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPolyline.count(), polylineCoords.count());

    // ========================================================================
    // Test save/load points round-trip
    // ========================================================================
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(37.0, -122.0, 100.0));
    points.append(QGeoCoordinate(38.0, -121.0, 200.0));

    const QString savedPointsFile = tmpDir.filePath("saved_points.gpx");
    QVERIFY(GPXHelper::savePointsToFile(savedPointsFile, points, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(GeoFormatRegistry::loadPoints(savedPointsFile, loadedPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedPoints.count(), points.count());

    // Verify altitudes
    QCOMPARE(loadedPoints[0].altitude(), 100.0);
    QCOMPARE(loadedPoints[1].altitude(), 200.0);

    // ========================================================================
    // Test save track
    // ========================================================================
    QList<QGeoCoordinate> trackCoords;
    trackCoords.append(QGeoCoordinate(37.0, -122.0, 50.0));
    trackCoords.append(QGeoCoordinate(37.1, -122.1, 55.0));
    trackCoords.append(QGeoCoordinate(37.2, -122.2, 60.0));

    const QString savedTrackFile = tmpDir.filePath("saved_track.gpx");
    QVERIFY(GPXHelper::saveTrackToFile(savedTrackFile, trackCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify by loading it back as polyline
    QList<QGeoCoordinate> loadedTrack;
    QVERIFY(GeoFormatRegistry::loadPolyline(savedTrackFile, loadedTrack, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(loadedTrack.count(), trackCoords.count());

    // ========================================================================
    // Test determineShapeType on saved files
    // ========================================================================
    QCOMPARE(GPXHelper::determineShapeType(savedPolygonFile, errorString), GeoFormatRegistry::ShapeType::Polygon);
    QCOMPARE(GPXHelper::determineShapeType(savedPolylineFile, errorString), GeoFormatRegistry::ShapeType::Polyline);
    QCOMPARE(GPXHelper::determineShapeType(savedPointsFile, errorString), GeoFormatRegistry::ShapeType::Point);
    QCOMPARE(GPXHelper::determineShapeType(savedTrackFile, errorString), GeoFormatRegistry::ShapeType::Track);
}

void ShapeTest::_testGPXTrackParsing()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with multiple track segments
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <trk>
    <name>Multi-segment Track</name>
    <trkseg>
      <trkpt lat="37.0" lon="-122.0"></trkpt>
      <trkpt lat="37.1" lon="-122.1"></trkpt>
    </trkseg>
    <trkseg>
      <trkpt lat="38.0" lon="-123.0"></trkpt>
      <trkpt lat="38.1" lon="-123.1"></trkpt>
    </trkseg>
  </trk>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "multiseg.gpx", gpxContent);

    QList<QList<QGeoCoordinate>> polylines;
    QVERIFY(GPXHelper::loadPolylinesFromFile(gpxFile, polylines, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(polylines.count(), 2);  // Each track segment becomes a polyline
    QCOMPARE(polylines[0].count(), 2);
    QCOMPARE(polylines[1].count(), 2);

    QCOMPARE(GPXHelper::determineShapeType(gpxFile, errorString), GeoFormatRegistry::ShapeType::Track);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testGPXAltitudeParsing()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // GPX with various altitude scenarios
    const QString gpxContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="QGC Test">
  <wpt lat="37.0" lon="-122.0">
    <ele>100.5</ele>
  </wpt>
  <wpt lat="38.0" lon="-121.0">
    <ele>-50.25</ele>
  </wpt>
  <wpt lat="39.0" lon="-120.0">
  </wpt>
  <rte>
    <rtept lat="40.0" lon="-119.0"><ele>500</ele></rtept>
    <rtept lat="41.0" lon="-118.0"></rtept>
  </rte>
</gpx>)";

    const QString gpxFile = _writeGpxFile(tmpDir, "altitude.gpx", gpxContent);

    QList<QGeoCoordinate> points;
    QVERIFY(GPXHelper::loadPointsFromFile(gpxFile, points, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(points.count(), 3);
    QCOMPARE(points[0].altitude(), 100.5);
    QCOMPARE(points[1].altitude(), -50.25);  // Negative altitude (below sea level)
    QVERIFY(std::isnan(points[2].altitude()));  // No altitude

    // Test route with mixed altitudes
    QList<QGeoCoordinate> routeCoords;
    QVERIFY(GPXHelper::loadPolylineFromFile(gpxFile, routeCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(routeCoords.count(), 2);
    QCOMPARE(routeCoords[0].altitude(), 500.0);
    QVERIFY(std::isnan(routeCoords[1].altitude()));
}

// ============================================================================
// Round-trip tests (save then load to verify data integrity)
// ============================================================================

void ShapeTest::_testRoundTripPolygonKMLToSHP()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polygon coordinates
    QList<QGeoCoordinate> originalCoords;
    originalCoords.append(QGeoCoordinate(37.0, -122.0, 100.0));
    originalCoords.append(QGeoCoordinate(37.1, -122.0, 110.0));
    originalCoords.append(QGeoCoordinate(37.1, -122.1, 120.0));
    originalCoords.append(QGeoCoordinate(37.0, -122.1, 130.0));
    originalCoords.append(QGeoCoordinate(37.0, -122.0, 100.0));  // Closed polygon

    // Save to KML
    const QString kmlFile = tmpDir.filePath("polygon.kml");
    QVERIFY(KMLHelper::savePolygonToFile(kmlFile, originalCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from KML
    QList<QGeoCoordinate> kmlCoords;
    QVERIFY(KMLHelper::loadPolygonFromFile(kmlFile, kmlCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Save to SHP
    const QString shpFile = tmpDir.filePath("polygon.shp");
    QVERIFY(SHPHelper::savePolygonToFile(shpFile, kmlCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from SHP
    QList<QGeoCoordinate> shpCoords;
    QVERIFY(SHPHelper::loadPolygonFromFile(shpFile, shpCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify coordinates match (within tolerance for floating point)
    // Note: SHP may not preserve altitude, so only compare lat/lon
    QCOMPARE(shpCoords.count(), kmlCoords.count());
    for (int i = 0; i < shpCoords.count(); ++i) {
        QVERIFY2(qAbs(shpCoords[i].latitude() - kmlCoords[i].latitude()) < 0.0001,
                 qPrintable(QString("Latitude mismatch at index %1: %2 vs %3")
                            .arg(i).arg(shpCoords[i].latitude()).arg(kmlCoords[i].latitude())));
        QVERIFY2(qAbs(shpCoords[i].longitude() - kmlCoords[i].longitude()) < 0.0001,
                 qPrintable(QString("Longitude mismatch at index %1: %2 vs %3")
                            .arg(i).arg(shpCoords[i].longitude()).arg(kmlCoords[i].longitude())));
    }
}

void ShapeTest::_testRoundTripPolygonKMLToGeoJSON()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polygon coordinates
    QList<QGeoCoordinate> originalCoords;
    originalCoords.append(QGeoCoordinate(47.0, -122.5, 50.0));
    originalCoords.append(QGeoCoordinate(47.2, -122.5, 55.0));
    originalCoords.append(QGeoCoordinate(47.2, -122.8, 60.0));
    originalCoords.append(QGeoCoordinate(47.0, -122.8, 65.0));
    originalCoords.append(QGeoCoordinate(47.0, -122.5, 50.0));  // Closed polygon

    // Save to KML
    const QString kmlFile = tmpDir.filePath("polygon.kml");
    QVERIFY(KMLHelper::savePolygonToFile(kmlFile, originalCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from KML
    QList<QGeoCoordinate> kmlCoords;
    QVERIFY(KMLHelper::loadPolygonFromFile(kmlFile, kmlCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Save to GeoJSON
    const QString geoJsonFile = tmpDir.filePath("polygon.geojson");
    QVERIFY(GeoJsonHelper::savePolygonToFile(geoJsonFile, kmlCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from GeoJSON
    QList<QGeoCoordinate> geoJsonCoords;
    QVERIFY(GeoJsonHelper::loadPolygonFromFile(geoJsonFile, geoJsonCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify coordinates match (GeoJSON preserves altitude)
    QCOMPARE(geoJsonCoords.count(), kmlCoords.count());
    for (int i = 0; i < geoJsonCoords.count(); ++i) {
        QVERIFY2(qAbs(geoJsonCoords[i].latitude() - kmlCoords[i].latitude()) < 0.0001,
                 qPrintable(QString("Latitude mismatch at index %1").arg(i)));
        QVERIFY2(qAbs(geoJsonCoords[i].longitude() - kmlCoords[i].longitude()) < 0.0001,
                 qPrintable(QString("Longitude mismatch at index %1").arg(i)));
        if (!std::isnan(kmlCoords[i].altitude())) {
            QVERIFY2(qAbs(geoJsonCoords[i].altitude() - kmlCoords[i].altitude()) < 0.01,
                     qPrintable(QString("Altitude mismatch at index %1").arg(i)));
        }
    }
}

void ShapeTest::_testRoundTripPolygonKMLToGPX()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polygon coordinates
    QList<QGeoCoordinate> originalCoords;
    originalCoords.append(QGeoCoordinate(40.0, -105.0, 1600.0));
    originalCoords.append(QGeoCoordinate(40.1, -105.0, 1650.0));
    originalCoords.append(QGeoCoordinate(40.1, -105.1, 1700.0));
    originalCoords.append(QGeoCoordinate(40.0, -105.1, 1750.0));
    originalCoords.append(QGeoCoordinate(40.0, -105.0, 1600.0));  // Closed polygon

    // Save to KML
    const QString kmlFile = tmpDir.filePath("polygon.kml");
    QVERIFY(KMLHelper::savePolygonToFile(kmlFile, originalCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from KML
    QList<QGeoCoordinate> kmlCoords;
    QVERIFY(KMLHelper::loadPolygonFromFile(kmlFile, kmlCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Save to GPX
    const QString gpxFile = tmpDir.filePath("polygon.gpx");
    QVERIFY(GPXHelper::savePolygonToFile(gpxFile, kmlCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Load from GPX
    QList<QGeoCoordinate> gpxCoords;
    QVERIFY(GPXHelper::loadPolygonFromFile(gpxFile, gpxCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify coordinates match (GPX preserves altitude)
    QCOMPARE(gpxCoords.count(), kmlCoords.count());
    for (int i = 0; i < gpxCoords.count(); ++i) {
        QVERIFY2(qAbs(gpxCoords[i].latitude() - kmlCoords[i].latitude()) < 0.0001,
                 qPrintable(QString("Latitude mismatch at index %1").arg(i)));
        QVERIFY2(qAbs(gpxCoords[i].longitude() - kmlCoords[i].longitude()) < 0.0001,
                 qPrintable(QString("Longitude mismatch at index %1").arg(i)));
        if (!std::isnan(kmlCoords[i].altitude())) {
            QVERIFY2(qAbs(gpxCoords[i].altitude() - kmlCoords[i].altitude()) < 0.01,
                     qPrintable(QString("Altitude mismatch at index %1").arg(i)));
        }
    }
}

void ShapeTest::_testRoundTripPolylineAllFormats()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test polyline coordinates (flight path)
    QList<QGeoCoordinate> originalCoords;
    originalCoords.append(QGeoCoordinate(33.0, -117.0, 500.0));
    originalCoords.append(QGeoCoordinate(33.1, -117.1, 550.0));
    originalCoords.append(QGeoCoordinate(33.2, -117.0, 600.0));
    originalCoords.append(QGeoCoordinate(33.3, -116.9, 650.0));

    // Test KML -> GeoJSON -> GPX -> KML round-trip
    const QString kmlFile1 = tmpDir.filePath("polyline1.kml");
    QVERIFY(KMLHelper::savePolylineToFile(kmlFile1, originalCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> kmlCoords;
    QVERIFY(KMLHelper::loadPolylineFromFile(kmlFile1, kmlCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString geoJsonFile = tmpDir.filePath("polyline.geojson");
    QVERIFY(GeoJsonHelper::savePolylineToFile(geoJsonFile, kmlCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> geoJsonCoords;
    QVERIFY(GeoJsonHelper::loadPolylineFromFile(geoJsonFile, geoJsonCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString gpxFile = tmpDir.filePath("polyline.gpx");
    QVERIFY(GPXHelper::savePolylineToFile(gpxFile, geoJsonCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> gpxCoords;
    QVERIFY(GPXHelper::loadPolylineFromFile(gpxFile, gpxCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString kmlFile2 = tmpDir.filePath("polyline2.kml");
    QVERIFY(KMLHelper::savePolylineToFile(kmlFile2, gpxCoords, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> finalCoords;
    QVERIFY(KMLHelper::loadPolylineFromFile(kmlFile2, finalCoords, errorString, 0));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify final coords match original
    QCOMPARE(finalCoords.count(), originalCoords.count());
    for (int i = 0; i < finalCoords.count(); ++i) {
        QVERIFY2(qAbs(finalCoords[i].latitude() - originalCoords[i].latitude()) < 0.0001,
                 qPrintable(QString("Latitude mismatch at index %1: expected %2, got %3")
                            .arg(i).arg(originalCoords[i].latitude()).arg(finalCoords[i].latitude())));
        QVERIFY2(qAbs(finalCoords[i].longitude() - originalCoords[i].longitude()) < 0.0001,
                 qPrintable(QString("Longitude mismatch at index %1: expected %2, got %3")
                            .arg(i).arg(originalCoords[i].longitude()).arg(finalCoords[i].longitude())));
        if (!std::isnan(originalCoords[i].altitude())) {
            QVERIFY2(qAbs(finalCoords[i].altitude() - originalCoords[i].altitude()) < 0.01,
                     qPrintable(QString("Altitude mismatch at index %1: expected %2, got %3")
                                .arg(i).arg(originalCoords[i].altitude()).arg(finalCoords[i].altitude())));
        }
    }
}

void ShapeTest::_testRoundTripPointsAllFormats()
{
    const QTemporaryDir tmpDir;
    QString errorString;

    // Create test waypoints
    QList<QGeoCoordinate> originalPoints;
    originalPoints.append(QGeoCoordinate(45.0, -110.0, 1000.0));
    originalPoints.append(QGeoCoordinate(45.5, -110.5, 1100.0));
    originalPoints.append(QGeoCoordinate(46.0, -111.0, 1200.0));

    // Test KML -> GeoJSON -> GPX -> KML round-trip
    const QString kmlFile1 = tmpDir.filePath("points1.kml");
    QVERIFY(KMLHelper::savePointsToFile(kmlFile1, originalPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> kmlPoints;
    QVERIFY(KMLHelper::loadPointsFromFile(kmlFile1, kmlPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString geoJsonFile = tmpDir.filePath("points.geojson");
    QVERIFY(GeoJsonHelper::savePointsToFile(geoJsonFile, kmlPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> geoJsonPoints;
    QVERIFY(GeoJsonHelper::loadPointsFromFile(geoJsonFile, geoJsonPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString gpxFile = tmpDir.filePath("points.gpx");
    QVERIFY(GPXHelper::savePointsToFile(gpxFile, geoJsonPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> gpxPoints;
    QVERIFY(GPXHelper::loadPointsFromFile(gpxFile, gpxPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    const QString kmlFile2 = tmpDir.filePath("points2.kml");
    QVERIFY(KMLHelper::savePointsToFile(kmlFile2, gpxPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    QList<QGeoCoordinate> finalPoints;
    QVERIFY(KMLHelper::loadPointsFromFile(kmlFile2, finalPoints, errorString));
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    // Verify final points match original
    QCOMPARE(finalPoints.count(), originalPoints.count());
    for (int i = 0; i < finalPoints.count(); ++i) {
        QVERIFY2(qAbs(finalPoints[i].latitude() - originalPoints[i].latitude()) < 0.0001,
                 qPrintable(QString("Latitude mismatch at point %1: expected %2, got %3")
                            .arg(i).arg(originalPoints[i].latitude()).arg(finalPoints[i].latitude())));
        QVERIFY2(qAbs(finalPoints[i].longitude() - originalPoints[i].longitude()) < 0.0001,
                 qPrintable(QString("Longitude mismatch at point %1: expected %2, got %3")
                            .arg(i).arg(originalPoints[i].longitude()).arg(finalPoints[i].longitude())));
        if (!std::isnan(originalPoints[i].altitude())) {
            QVERIFY2(qAbs(finalPoints[i].altitude() - originalPoints[i].altitude()) < 0.01,
                     qPrintable(QString("Altitude mismatch at point %1: expected %2, got %3")
                                .arg(i).arg(originalPoints[i].altitude()).arg(finalPoints[i].altitude())));
        }
    }
}

void ShapeTest::_testSupportsFeatureAPI()
{
    using Capability = GeoFormatRegistry::Capability;

    // KML supports most features
    QVERIFY(GeoFormatRegistry::hasCapability("kml", Capability::CanReadPolygons));
    QVERIFY(GeoFormatRegistry::hasCapability("kml", Capability::CanReadPolygonsWithHoles));
    QVERIFY(GeoFormatRegistry::hasCapability("kml", Capability::CanReadPolylines));
    QVERIFY(GeoFormatRegistry::hasCapability("kml", Capability::CanReadPoints));
    QVERIFY(!GeoFormatRegistry::hasCapability("kml", Capability::CanReadTracks));  // KML doesn't have GPX-style tracks

    // GPX supports tracks but not polygons with holes
    QVERIFY(GeoFormatRegistry::hasCapability("gpx", Capability::CanReadPolygons));
    QVERIFY(!GeoFormatRegistry::hasCapability("gpx", Capability::CanReadPolygonsWithHoles));  // GPX doesn't support holes
    QVERIFY(GeoFormatRegistry::hasCapability("gpx", Capability::CanReadPolylines));
    QVERIFY(GeoFormatRegistry::hasCapability("gpx", Capability::CanReadPoints));
    QVERIFY(GeoFormatRegistry::hasCapability("gpx", Capability::CanReadTracks));

    // SHP supports all basic features
    QVERIFY(GeoFormatRegistry::hasCapability("shp", Capability::CanReadPolygons));
    QVERIFY(GeoFormatRegistry::hasCapability("shp", Capability::CanReadPolygonsWithHoles));
    QVERIFY(GeoFormatRegistry::hasCapability("shp", Capability::CanReadPolylines));
    QVERIFY(GeoFormatRegistry::hasCapability("shp", Capability::CanReadPoints));
    QVERIFY(!GeoFormatRegistry::hasCapability("shp", Capability::CanReadTracks));

    // GeoJSON supports all basic features
    QVERIFY(GeoFormatRegistry::hasCapability("geojson", Capability::CanReadPolygons));
    QVERIFY(GeoFormatRegistry::hasCapability("geojson", Capability::CanReadPolygonsWithHoles));
    QVERIFY(GeoFormatRegistry::hasCapability("geojson", Capability::CanReadPolylines));
    QVERIFY(GeoFormatRegistry::hasCapability("geojson", Capability::CanReadPoints));

    // Unknown extension should return false for all features
    QVERIFY(!GeoFormatRegistry::hasCapability("xyz", Capability::CanReadPolygons));
    QVERIFY(!GeoFormatRegistry::hasCapability("xyz", Capability::CanReadTracks));

    // Also test with file path
    QVERIFY(GeoFormatRegistry::fileHasCapability("/some/path/file.kml", Capability::CanReadPolygons));
    QVERIFY(GeoFormatRegistry::fileHasCapability("/some/path/file.gpx", Capability::CanReadTracks));
    QVERIFY(!GeoFormatRegistry::fileHasCapability("/some/path/file.kml", Capability::CanReadTracks));
}

void ShapeTest::_testSaveInvalidCoordinates()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create coordinates with an invalid value (latitude > 90)
    QList<QGeoCoordinate> invalidPolygon;
    invalidPolygon << QGeoCoordinate(100.0, 0.0);  // Invalid latitude
    invalidPolygon << QGeoCoordinate(0.0, 1.0);
    invalidPolygon << QGeoCoordinate(0.0, -1.0);

    GeoFormatRegistry::SaveResult result;

    // Test that save functions reject invalid coordinates for all formats
    const QString kmlFile = tmpDir.filePath("invalid.kml");
    result = GeoFormatRegistry::savePolygon(kmlFile, invalidPolygon);
    QVERIFY2(!result.success, "KML save should reject invalid latitude");
    QVERIFY2(!result.errorString.isEmpty(), "KML rejection should provide an error message");

    const QString geoJsonFile = tmpDir.filePath("invalid.geojson");
    result = GeoFormatRegistry::savePolygon(geoJsonFile, invalidPolygon);
    QVERIFY2(!result.success, "GeoJSON save should reject invalid latitude");
    QVERIFY2(!result.errorString.isEmpty(), "GeoJSON rejection should provide an error message");

    const QString gpxFile = tmpDir.filePath("invalid.gpx");
    result = GeoFormatRegistry::savePolygon(gpxFile, invalidPolygon);
    QVERIFY2(!result.success, "GPX save should reject invalid latitude");
    QVERIFY2(!result.errorString.isEmpty(), "GPX rejection should provide an error message");

    const QString shpFile = tmpDir.filePath("invalid.shp");
    result = GeoFormatRegistry::savePolygon(shpFile, invalidPolygon);
    QVERIFY2(!result.success, "SHP save should reject invalid latitude");
    QVERIFY2(!result.errorString.isEmpty(), "SHP rejection should provide an error message");

    // Test with invalid longitude
    QList<QGeoCoordinate> invalidLongitude;
    invalidLongitude << QGeoCoordinate(0.0, 200.0);  // Invalid longitude
    invalidLongitude << QGeoCoordinate(1.0, 0.0);
    invalidLongitude << QGeoCoordinate(-1.0, 0.0);

    result = GeoFormatRegistry::savePolygon(kmlFile, invalidLongitude);
    QVERIFY2(!result.success, "KML save should reject invalid longitude");
    QVERIFY2(!result.errorString.isEmpty(), "Invalid longitude rejection should provide an error message");
}

// ============================================================================
// OpenAir Parser Tests
// ============================================================================

void ShapeTest::_testOpenAirParsePolygon()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create a simple OpenAir file with a polygon
    const QString content = R"(
AC R
AN Test Restricted Area
AH 5000FT MSL
AL GND
DP 47:00:00N 008:00:00E
DP 47:00:00N 009:00:00E
DP 46:00:00N 009:00:00E
DP 46:00:00N 008:00:00E
)";

    const QString filePath = tmpDir.filePath("test.txt");
    QVERIFY(_writeTextFile(filePath, content));

    OpenAirParser::ParseResult result = OpenAirParser::parseFile(filePath);
    QVERIFY(result.success);
    QCOMPARE(result.airspaces.count(), 1);
    QCOMPARE(result.airspaces[0].name, QStringLiteral("Test Restricted Area"));
    QVERIFY(result.airspaces[0].boundary.count() >= 4);
}

void ShapeTest::_testOpenAirParseCircle()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create an OpenAir file with a circle
    const QString content = R"(
AC D
AN Test Circle
AH FL100
AL 2000FT MSL
V X=47:30:00N 008:30:00E
DC 5
)";

    const QString filePath = tmpDir.filePath("circle.txt");
    QVERIFY(_writeTextFile(filePath, content));

    OpenAirParser::ParseResult result = OpenAirParser::parseFile(filePath);
    QVERIFY(result.success);
    QCOMPARE(result.airspaces.count(), 1);
    // Circle should generate multiple points (72 by default)
    QVERIFY(result.airspaces[0].boundary.count() >= 36);
}

void ShapeTest::_testOpenAirParseArc()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create an OpenAir file with an arc
    const QString content = R"(
AC C
AN Test Arc Area
AH 3000FT MSL
AL GND
DP 47:00:00N 008:00:00E
V X=47:00:00N 008:30:00E
V D=+
DA 10,0,90
DP 47:00:00N 009:00:00E
)";

    const QString filePath = tmpDir.filePath("arc.txt");
    QVERIFY(_writeTextFile(filePath, content));

    OpenAirParser::ParseResult result = OpenAirParser::parseFile(filePath);
    QVERIFY(result.success);
    QCOMPARE(result.airspaces.count(), 1);
    QVERIFY(result.airspaces[0].boundary.count() >= 3);
}

void ShapeTest::_testOpenAirParseCoordinate()
{
    QGeoCoordinate coord;
    QString errorString;

    // Test DMS format
    QVERIFY(OpenAirParser::parseCoordinate(QStringLiteral("47:30:00N 008:30:00E"), coord));
    QCOMPARE(qRound(coord.latitude() * 100), 4750);
    QCOMPARE(qRound(coord.longitude() * 100), 850);

    // Test decimal format
    QVERIFY(OpenAirParser::parseCoordinate(QStringLiteral("47.5N 8.5E"), coord));
    QCOMPARE(qRound(coord.latitude() * 10), 475);
    QCOMPARE(qRound(coord.longitude() * 10), 85);

    // Test southern/western coordinates
    QVERIFY(OpenAirParser::parseCoordinate(QStringLiteral("33:45:00S 070:40:00W"), coord));
    QVERIFY(coord.latitude() < 0);
    QVERIFY(coord.longitude() < 0);
}

void ShapeTest::_testOpenAirParseAltitude()
{
    // Test various altitude formats
    OpenAirParser::Altitude alt;

    alt = OpenAirParser::parseAltitude(QStringLiteral("5000FT MSL"));
    QCOMPARE(alt.reference, OpenAirParser::AltitudeReference::MSL);
    QCOMPARE(qRound(alt.value), 5000);

    alt = OpenAirParser::parseAltitude(QStringLiteral("FL100"));
    QCOMPARE(alt.reference, OpenAirParser::AltitudeReference::FL);
    QCOMPARE(qRound(alt.value), 100);

    alt = OpenAirParser::parseAltitude(QStringLiteral("GND"));
    QCOMPARE(alt.reference, OpenAirParser::AltitudeReference::SFC);
    QCOMPARE(qRound(alt.value), 0);

    alt = OpenAirParser::parseAltitude(QStringLiteral("2500FT AGL"));
    QCOMPARE(alt.reference, OpenAirParser::AltitudeReference::AGL);
    QCOMPARE(qRound(alt.value), 2500);
}

void ShapeTest::_testOpenAirRoundTrip()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create test airspaces
    QList<OpenAirParser::Airspace> originalAirspaces;

    // Airspace 1: Simple polygon
    OpenAirParser::Airspace airspace1;
    airspace1.name = QStringLiteral("Test Restricted Area");
    airspace1.airspaceClass = OpenAirParser::AirspaceClass::Restricted;
    airspace1.floor.value = 0;
    airspace1.floor.reference = OpenAirParser::AltitudeReference::SFC;
    airspace1.ceiling.value = 5000;
    airspace1.ceiling.reference = OpenAirParser::AltitudeReference::MSL;
    airspace1.boundary.append(QGeoCoordinate(47.0, 8.0));
    airspace1.boundary.append(QGeoCoordinate(47.0, 9.0));
    airspace1.boundary.append(QGeoCoordinate(46.0, 9.0));
    airspace1.boundary.append(QGeoCoordinate(46.0, 8.0));
    originalAirspaces.append(airspace1);

    // Airspace 2: Another polygon with different class
    OpenAirParser::Airspace airspace2;
    airspace2.name = QStringLiteral("Control Zone");
    airspace2.airspaceClass = OpenAirParser::AirspaceClass::D;
    airspace2.floor.value = 0;
    airspace2.floor.reference = OpenAirParser::AltitudeReference::SFC;
    airspace2.ceiling.value = 3500;
    airspace2.ceiling.reference = OpenAirParser::AltitudeReference::MSL;
    airspace2.boundary.append(QGeoCoordinate(48.0, 10.0));
    airspace2.boundary.append(QGeoCoordinate(48.5, 10.0));
    airspace2.boundary.append(QGeoCoordinate(48.5, 10.5));
    airspace2.boundary.append(QGeoCoordinate(48.0, 10.5));
    originalAirspaces.append(airspace2);

    // Save to file
    const QString filePath = tmpDir.filePath("roundtrip.txt");
    QString errorString;
    QVERIFY(OpenAirParser::saveFile(filePath, originalAirspaces, errorString));
    QVERIFY(errorString.isEmpty());

    // Load back from file
    OpenAirParser::ParseResult result = OpenAirParser::parseFile(filePath);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE(result.airspaces.count(), originalAirspaces.count());

    // Verify airspace 1
    QCOMPARE(result.airspaces[0].name, originalAirspaces[0].name);
    QCOMPARE(result.airspaces[0].airspaceClass, originalAirspaces[0].airspaceClass);
    QCOMPARE(result.airspaces[0].boundary.count(), originalAirspaces[0].boundary.count());

    // Verify coordinates are close (may have some precision loss)
    for (int i = 0; i < result.airspaces[0].boundary.count(); i++) {
        QVERIFY(result.airspaces[0].boundary[i].distanceTo(originalAirspaces[0].boundary[i]) < 100);  // Within 100m
    }

    // Verify airspace 2
    QCOMPARE(result.airspaces[1].name, originalAirspaces[1].name);
    QCOMPARE(result.airspaces[1].airspaceClass, originalAirspaces[1].airspaceClass);
}

// ============================================================================
// GeoFormatRegistry Tests
// ============================================================================

void ShapeTest::_testGeoFormatRegistrySupportedFormats()
{
    QList<GeoFormatRegistry::FormatInfo> formats = GeoFormatRegistry::supportedFormats();
    QVERIFY(formats.count() >= 7); // At least KML, KMZ, GeoJSON, GPX, SHP, WKT, OpenAir

    // Verify format info structure
    for (const auto& format : formats) {
        QVERIFY(!format.name.isEmpty());
        QVERIFY(!format.extensions.isEmpty());
        QVERIFY(format.isNative);
    }
}

void ShapeTest::_testGeoFormatRegistryLoadFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Create a test KML file
    const QString kmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
<Document>
<Placemark>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
8,47,0 9,47,0 9,48,0 8,48,0 8,47,0
</coordinates></LinearRing></outerBoundaryIs></Polygon>
</Placemark>
</Document>
</kml>)";

    const QString kmlFile = tmpDir.filePath("test.kml");
    QVERIFY(_writeTextFile(kmlFile, kmlContent));

    // Load using registry
    GeoFormatRegistry::LoadResult result = GeoFormatRegistry::loadFile(kmlFile);
    QVERIFY(result.success);
    QCOMPARE(result.formatUsed, QStringLiteral("KML"));
    QVERIFY(result.totalFeatures() > 0);
}

void ShapeTest::_testGeoFormatRegistryExtensionDispatch()
{
    const QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QList<QGeoCoordinate> polygon = {
        QGeoCoordinate(47.0, 8.0),
        QGeoCoordinate(47.0, 9.0),
        QGeoCoordinate(48.0, 9.0)
    };

    const QString upperKmlFile = tmpDir.filePath("dispatch_test.KML");
    const GeoFormatRegistry::SaveResult kmlResult = GeoFormatRegistry::savePolygon(upperKmlFile, polygon);
    QVERIFY2(kmlResult.success, qPrintable(kmlResult.errorString));
    QCOMPARE(kmlResult.formatUsed, QStringLiteral("KML"));

    const QString upperJsonFile = tmpDir.filePath("dispatch_test.JSON");
    const GeoFormatRegistry::SaveResult jsonResult = GeoFormatRegistry::savePolygon(upperJsonFile, polygon);
    QVERIFY2(jsonResult.success, qPrintable(jsonResult.errorString));
    QCOMPARE(jsonResult.formatUsed, QStringLiteral("GeoJSON"));

    const QString openAirContent = QStringLiteral(
        "AC R\n"
        "AN Dispatch Test\n"
        "AH 5000FT MSL\n"
        "AL GND\n"
        "DP 47:00:00N 008:00:00E\n"
        "DP 47:00:00N 009:00:00E\n"
        "DP 46:00:00N 009:00:00E\n"
        "DP 46:00:00N 008:00:00E\n");
    const QString upperTxtFile = tmpDir.filePath("dispatch_test.TXT");
    QVERIFY(_writeTextFile(upperTxtFile, openAirContent));

    const GeoFormatRegistry::LoadResult openAirResult = GeoFormatRegistry::loadFile(upperTxtFile);
    QVERIFY(openAirResult.success);
    QCOMPARE(openAirResult.formatUsed, QStringLiteral("OpenAir"));
    QVERIFY(!openAirResult.polygons.isEmpty());

    const QString unknownFile = tmpDir.filePath("dispatch_test.xyz");
    QVERIFY(_writeTextFile(unknownFile, QStringLiteral("not a supported geo format")));
    const GeoFormatRegistry::LoadResult unknownResult = GeoFormatRegistry::loadFile(unknownFile);
    QVERIFY(!unknownResult.success);
    QVERIFY(unknownResult.errorString.contains(QStringLiteral("Unsupported format"), Qt::CaseInsensitive));
}

void ShapeTest::_testGeoFormatRegistryFileFilters()
{
    QString readFilter = GeoFormatRegistry::readFileFilter();
    QVERIFY(!readFilter.isEmpty());
    QVERIFY(readFilter.contains(QStringLiteral("*.kml")));
    QVERIFY(readFilter.contains(QStringLiteral("*.geojson")));
    QVERIFY(readFilter.contains(QStringLiteral("*.gpx")));
    QVERIFY(readFilter.contains(QStringLiteral("*.shp")));

    QString writeFilter = GeoFormatRegistry::writeFileFilter();
    QVERIFY(!writeFilter.isEmpty());
}

void ShapeTest::_testGeoFormatRegistrySaveFunctions()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Test data
    QList<QGeoCoordinate> polygon;
    polygon << QGeoCoordinate(47.0, 8.0);
    polygon << QGeoCoordinate(47.0, 9.0);
    polygon << QGeoCoordinate(48.0, 9.0);
    polygon << QGeoCoordinate(48.0, 8.0);

    QList<QGeoCoordinate> polyline;
    polyline << QGeoCoordinate(47.0, 8.0);
    polyline << QGeoCoordinate(47.5, 8.5);
    polyline << QGeoCoordinate(48.0, 9.0);

    QList<QGeoCoordinate> points;
    points << QGeoCoordinate(47.1, 8.1);
    points << QGeoCoordinate(47.2, 8.2);

    // Test save to KML
    const QString kmlFile = tmpDir.filePath("registry_test.kml");
    GeoFormatRegistry::SaveResult kmlResult = GeoFormatRegistry::savePolygon(kmlFile, polygon);
    QVERIFY2(kmlResult.success, qPrintable(kmlResult.errorString));
    QVERIFY(QFile::exists(kmlFile));

    // Test save to GeoJSON
    const QString geojsonFile = tmpDir.filePath("registry_test.geojson");
    GeoFormatRegistry::SaveResult geojsonResult = GeoFormatRegistry::savePolygon(geojsonFile, polygon);
    QVERIFY2(geojsonResult.success, qPrintable(geojsonResult.errorString));
    QVERIFY(QFile::exists(geojsonFile));

    // Test save to GPX (polyline)
    const QString gpxFile = tmpDir.filePath("registry_test.gpx");
    GeoFormatRegistry::SaveResult gpxResult = GeoFormatRegistry::savePolyline(gpxFile, polyline);
    QVERIFY2(gpxResult.success, qPrintable(gpxResult.errorString));
    QVERIFY(QFile::exists(gpxFile));

    // Test save to WKT
    const QString wktFile = tmpDir.filePath("registry_test.wkt");
    GeoFormatRegistry::SaveResult wktResult = GeoFormatRegistry::savePolygon(wktFile, polygon);
    QVERIFY2(wktResult.success, qPrintable(wktResult.errorString));
    QVERIFY(QFile::exists(wktFile));

    // Test save points to WKT
    const QString wktPointsFile = tmpDir.filePath("registry_points.wkt");
    GeoFormatRegistry::SaveResult wktPointsResult = GeoFormatRegistry::savePoints(wktPointsFile, points);
    QVERIFY2(wktPointsResult.success, qPrintable(wktPointsResult.errorString));
    QVERIFY(QFile::exists(wktPointsFile));

    // Test save multiple polygons
    QList<QList<QGeoCoordinate>> polygons;
    polygons.append(polygon);
    const QString multiFile = tmpDir.filePath("registry_multi.geojson");
    GeoFormatRegistry::SaveResult multiResult = GeoFormatRegistry::savePolygons(multiFile, polygons);
    QVERIFY2(multiResult.success, qPrintable(multiResult.errorString));
}

void ShapeTest::_testGeoFormatRegistryCapabilityValidation()
{
    // Test that all registered formats have valid capability declarations
    QList<GeoFormatRegistry::ValidationResult> results = GeoFormatRegistry::validateCapabilities();

    // Verify validation returns results for all formats
    QVERIFY(results.count() > 0);

    // Check that all native formats are valid (no capability mismatches)
    bool allValid = GeoFormatRegistry::allCapabilitiesValid();
    QStringList invalidFormats;
    for (const auto &result : results) {
        if (!result.valid) {
            invalidFormats.append(QStringLiteral("%1: %2").arg(result.formatName, result.issues.join(QStringLiteral(", "))));
        }
    }

    const QByteArray failureMessage = invalidFormats.isEmpty()
        ? QByteArray("Some formats have capability mismatches")
        : QStringLiteral("Some formats have capability mismatches: %1").arg(invalidFormats.join(QStringLiteral(" | "))).toUtf8();
    QVERIFY2(allValid, failureMessage.constData());
}

// ============================================================================
// WKT (Well-Known Text) Tests
// ============================================================================

QString ShapeTest::_writeWktFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content)
{
    const QString path = tmpDir.filePath(name);
    if (!_writeTextFile(path, content)) {
        qFatal("Failed to write WKT test file: %s", qPrintable(path));
    }
    return path;
}

void ShapeTest::_testWKTParsePoint()
{
    QGeoCoordinate coord;
    QString errorString;

    // Test basic POINT
    QVERIFY(WKTHelper::parsePoint(QStringLiteral("POINT (8.5 47.5)"), coord, errorString));
    QCOMPARE(qRound(coord.longitude() * 10), 85);
    QCOMPARE(qRound(coord.latitude() * 10), 475);

    // Test POINT Z (with altitude)
    QVERIFY(WKTHelper::parsePoint(QStringLiteral("POINT Z (8.5 47.5 100)"), coord, errorString));
    QCOMPARE(qRound(coord.longitude() * 10), 85);
    QCOMPARE(qRound(coord.latitude() * 10), 475);
    QCOMPARE(qRound(coord.altitude()), 100);

    // Test invalid point
    QVERIFY(!WKTHelper::parsePoint(QStringLiteral("POINT ()"), coord, errorString));
}

void ShapeTest::_testWKTParseLineString()
{
    QList<QGeoCoordinate> coords;
    QString errorString;

    // Test basic LINESTRING
    QVERIFY(WKTHelper::parseLineString(
        QStringLiteral("LINESTRING (8 47, 8.5 47.5, 9 48)"), coords, errorString));
    QCOMPARE(coords.count(), 3);
    QCOMPARE(qRound(coords[0].longitude()), 8);
    QCOMPARE(qRound(coords[0].latitude()), 47);
    QCOMPARE(qRound(coords[2].longitude()), 9);
    QCOMPARE(qRound(coords[2].latitude()), 48);

    // Test LINESTRING Z
    coords.clear();
    QVERIFY(WKTHelper::parseLineString(
        QStringLiteral("LINESTRING Z (8 47 100, 9 48 200)"), coords, errorString));
    QCOMPARE(coords.count(), 2);
    QCOMPARE(qRound(coords[0].altitude()), 100);
    QCOMPARE(qRound(coords[1].altitude()), 200);
}

void ShapeTest::_testWKTParsePolygon()
{
    QList<QGeoCoordinate> vertices;
    QString errorString;

    // Test basic POLYGON (ring is auto-closed in WKT)
    QVERIFY(WKTHelper::parsePolygon(
        QStringLiteral("POLYGON ((8 47, 9 47, 9 48, 8 48, 8 47))"), vertices, errorString));
    QVERIFY(vertices.count() >= 4);

    // Test POLYGON with holes (outer ring should be extracted)
    vertices.clear();
    QVERIFY(WKTHelper::parsePolygon(
        QStringLiteral("POLYGON ((8 47, 9 47, 9 48, 8 48, 8 47), (8.2 47.2, 8.8 47.2, 8.8 47.8, 8.2 47.8, 8.2 47.2))"),
        vertices, errorString));
    QVERIFY(vertices.count() >= 4);
}

void ShapeTest::_testWKTGenerate()
{
    // Test POINT generation
    QGeoCoordinate point(47.5, 8.5, 100.0);
    QString pointWkt = WKTHelper::toWKTPoint(point, false);
    QVERIFY(pointWkt.startsWith(QStringLiteral("POINT")));
    QVERIFY(pointWkt.contains(QStringLiteral("8.5")));
    QVERIFY(pointWkt.contains(QStringLiteral("47.5")));

    // Test POINT Z generation
    QString pointZWkt = WKTHelper::toWKTPoint(point, true);
    QVERIFY(pointZWkt.startsWith(QStringLiteral("POINT Z")));
    QVERIFY(pointZWkt.contains(QStringLiteral("100")));

    // Test LINESTRING generation
    QList<QGeoCoordinate> line;
    line << QGeoCoordinate(47.0, 8.0);
    line << QGeoCoordinate(47.5, 8.5);
    line << QGeoCoordinate(48.0, 9.0);
    QString lineWkt = WKTHelper::toWKTLineString(line, false);
    QVERIFY(lineWkt.startsWith(QStringLiteral("LINESTRING")));

    // Test POLYGON generation
    QList<QGeoCoordinate> polygon;
    polygon << QGeoCoordinate(47.0, 8.0);
    polygon << QGeoCoordinate(47.0, 9.0);
    polygon << QGeoCoordinate(48.0, 9.0);
    polygon << QGeoCoordinate(48.0, 8.0);
    QString polygonWkt = WKTHelper::toWKTPolygon(polygon, false);
    QVERIFY(polygonWkt.startsWith(QStringLiteral("POLYGON")));
}

void ShapeTest::_testWKTRoundTrip()
{
    QString errorString;

    // Point round-trip
    QGeoCoordinate originalPoint(47.5, 8.5, 100.0);
    QString pointWkt = WKTHelper::toWKTPoint(originalPoint, true);
    QGeoCoordinate parsedPoint;
    QVERIFY(WKTHelper::parsePoint(pointWkt, parsedPoint, errorString));
    QCOMPARE(qRound(parsedPoint.latitude() * 100), qRound(originalPoint.latitude() * 100));
    QCOMPARE(qRound(parsedPoint.longitude() * 100), qRound(originalPoint.longitude() * 100));

    // LineString round-trip
    QList<QGeoCoordinate> originalLine;
    originalLine << QGeoCoordinate(47.0, 8.0);
    originalLine << QGeoCoordinate(47.5, 8.5);
    originalLine << QGeoCoordinate(48.0, 9.0);
    QString lineWkt = WKTHelper::toWKTLineString(originalLine, false);
    QList<QGeoCoordinate> parsedLine;
    QVERIFY(WKTHelper::parseLineString(lineWkt, parsedLine, errorString));
    QCOMPARE(parsedLine.count(), originalLine.count());

    // Polygon round-trip
    QList<QGeoCoordinate> originalPolygon;
    originalPolygon << QGeoCoordinate(47.0, 8.0);
    originalPolygon << QGeoCoordinate(47.0, 9.0);
    originalPolygon << QGeoCoordinate(48.0, 9.0);
    originalPolygon << QGeoCoordinate(48.0, 8.0);
    QString polygonWkt = WKTHelper::toWKTPolygon(originalPolygon, false);
    QList<QGeoCoordinate> parsedPolygon;
    QVERIFY(WKTHelper::parsePolygon(polygonWkt, parsedPolygon, errorString));
    QCOMPARE(parsedPolygon.count(), originalPolygon.count());
}

void ShapeTest::_testWKTLoadFromFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test loading point from file
    const QString pointWkt = _writeWktFile(tmpDir, "point.wkt", "POINT (8.5 47.5)");
    QGeoCoordinate point;
    QVERIFY(WKTHelper::loadPointFromFile(pointWkt, point, errorString));
    QCOMPARE(qRound(point.longitude() * 10), 85);
    QCOMPARE(qRound(point.latitude() * 10), 475);

    // Test loading points from MULTIPOINT file
    const QString multiPointWkt = _writeWktFile(tmpDir, "multipoint.wkt",
        "MULTIPOINT ((8 47), (8.5 47.5), (9 48))");
    QList<QGeoCoordinate> points;
    QVERIFY2(WKTHelper::loadPointsFromFile(multiPointWkt, points, errorString),
             qPrintable(QStringLiteral("loadPointsFromFile failed: %1").arg(errorString)));
    QCOMPARE(points.count(), 3);

    // Test loading polyline from file
    const QString lineWkt = _writeWktFile(tmpDir, "line.wkt",
        "LINESTRING (8 47, 8.5 47.5, 9 48)");
    QList<QGeoCoordinate> polyline;
    QVERIFY(WKTHelper::loadPolylineFromFile(lineWkt, polyline, errorString));
    QCOMPARE(polyline.count(), 3);

    // Test loading polygon from file
    const QString polygonWkt = _writeWktFile(tmpDir, "polygon.wkt",
        "POLYGON ((8 47, 9 47, 9 48, 8 48, 8 47))");
    QList<QGeoCoordinate> polygon;
    QVERIFY(WKTHelper::loadPolygonFromFile(polygonWkt, polygon, errorString));
    QVERIFY(polygon.count() >= 4);

    // Test loading multiple polygons from MULTIPOLYGON file
    const QString multiPolygonWkt = _writeWktFile(tmpDir, "multipolygon.wkt",
        "MULTIPOLYGON (((8 47, 9 47, 9 48, 8 48, 8 47)), ((10 50, 11 50, 11 51, 10 51, 10 50)))");
    QList<QList<QGeoCoordinate>> polygons;
    QVERIFY(WKTHelper::loadPolygonsFromFile(multiPolygonWkt, polygons, errorString));
    QCOMPARE(polygons.count(), 2);
}

void ShapeTest::_testWKTSaveToFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test saving point to file
    QGeoCoordinate point(47.5, 8.5, 100.0);
    const QString pointFile = tmpDir.filePath("save_point.wkt");
    QVERIFY(WKTHelper::savePointToFile(pointFile, point, errorString));
    QVERIFY(QFile::exists(pointFile));

    // Verify by loading back
    QGeoCoordinate loadedPoint;
    QVERIFY(WKTHelper::loadPointFromFile(pointFile, loadedPoint, errorString));
    QCOMPARE(qRound(loadedPoint.latitude() * 10), qRound(point.latitude() * 10));

    // Test saving points to file
    QList<QGeoCoordinate> points;
    points << QGeoCoordinate(47.1, 8.1);
    points << QGeoCoordinate(47.2, 8.2);
    const QString pointsFile = tmpDir.filePath("save_points.wkt");
    QVERIFY(WKTHelper::savePointsToFile(pointsFile, points, errorString));
    QVERIFY(QFile::exists(pointsFile));

    // Test saving polyline to file
    QList<QGeoCoordinate> polyline;
    polyline << QGeoCoordinate(47.0, 8.0);
    polyline << QGeoCoordinate(47.5, 8.5);
    polyline << QGeoCoordinate(48.0, 9.0);
    const QString polylineFile = tmpDir.filePath("save_polyline.wkt");
    QVERIFY(WKTHelper::savePolylineToFile(polylineFile, polyline, errorString));
    QVERIFY(QFile::exists(polylineFile));

    // Verify by loading back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(WKTHelper::loadPolylineFromFile(polylineFile, loadedPolyline, errorString));
    QCOMPARE(loadedPolyline.count(), polyline.count());

    // Test saving polygon to file
    QList<QGeoCoordinate> polygon;
    polygon << QGeoCoordinate(47.0, 8.0);
    polygon << QGeoCoordinate(47.0, 9.0);
    polygon << QGeoCoordinate(48.0, 9.0);
    polygon << QGeoCoordinate(48.0, 8.0);
    const QString polygonFile = tmpDir.filePath("save_polygon.wkt");
    QVERIFY(WKTHelper::savePolygonToFile(polygonFile, polygon, errorString));
    QVERIFY(QFile::exists(polygonFile));

    // Verify by loading back
    QList<QGeoCoordinate> loadedPolygon;
    QVERIFY(WKTHelper::loadPolygonFromFile(polygonFile, loadedPolygon, errorString));
    QCOMPARE(loadedPolygon.count(), polygon.count());

    // Test saving multiple polygons to file
    QList<QList<QGeoCoordinate>> polygons;
    polygons.append(polygon);
    const QString polygonsFile = tmpDir.filePath("save_polygons.wkt");
    QVERIFY(WKTHelper::savePolygonsToFile(polygonsFile, polygons, errorString));
    QVERIFY(QFile::exists(polygonsFile));

    // Verify by loading back
    QList<QList<QGeoCoordinate>> loadedPolygons;
    QVERIFY(WKTHelper::loadPolygonsFromFile(polygonsFile, loadedPolygons, errorString));
    QCOMPARE(loadedPolygons.count(), polygons.count());
}

// ============================================================================
// CSV Helper Functions
// ============================================================================

QString ShapeTest::_writeCsvFile(const QTemporaryDir &tmpDir, const QString &name, const QString &content)
{
    const QString path = tmpDir.filePath(name);
    if (!_writeTextFile(path, content)) {
        qFatal("Failed to write CSV test file: %s", qPrintable(path));
    }
    return path;
}

// ============================================================================
// CSV Tests
// ============================================================================

void ShapeTest::_testCSVLoadPoints()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test loading standard CSV with header
    const QString csv1 = _writeCsvFile(tmpDir, "points.csv",
        "lat,lon,alt\n"
        "47.6062,-122.3321,100\n"
        "37.7749,-122.4194,50\n"
        "34.0522,-118.2437,71\n");

    QList<QGeoCoordinate> points;
    QVERIFY2(CSVHelper::loadPointsFromFile(csv1, points, errorString),
             qPrintable(QStringLiteral("loadPointsFromFile failed: %1").arg(errorString)));
    QCOMPARE(points.count(), 3);
    QCOMPARE(qRound(points[0].latitude() * 10000), 476062);
    QCOMPARE(qRound(points[0].longitude() * 10000), -1223321);
    QCOMPARE(qRound(points[0].altitude()), 100);

    // Test loading CSV with different column names
    const QString csv2 = _writeCsvFile(tmpDir, "points2.csv",
        "latitude,longitude,elevation\n"
        "47.0,8.0,500\n"
        "48.0,9.0,600\n");

    points.clear();
    QVERIFY(CSVHelper::loadPointsFromFile(csv2, points, errorString));
    QCOMPARE(points.count(), 2);
    QCOMPARE(qRound(points[0].altitude()), 500);

    // Test loading CSV with x,y column names
    const QString csv3 = _writeCsvFile(tmpDir, "points3.csv",
        "y,x\n"
        "47.5,8.5\n");

    points.clear();
    QVERIFY(CSVHelper::loadPointsFromFile(csv3, points, errorString));
    QCOMPARE(points.count(), 1);
    QCOMPARE(qRound(points[0].latitude() * 10), 475);
    QCOMPARE(qRound(points[0].longitude() * 10), 85);

    // Test loading CSV without header (auto-detect lat,lon order)
    const QString csv4 = _writeCsvFile(tmpDir, "noheader.csv",
        "47.0,8.0\n"
        "48.0,9.0\n");

    CSVHelper::ParseOptions opts;
    opts.hasHeader = false;
    CSVHelper::LoadResult result = CSVHelper::loadPointsFromFile(csv4, opts);
    QVERIFY(result.success);
    QCOMPARE(result.points.count(), 2);

    // Test empty file error handling
    const QString emptyFile = _writeCsvFile(tmpDir, "empty.csv", "");
    points.clear();
    QVERIFY(!CSVHelper::loadPointsFromFile(emptyFile, points, errorString));
    QVERIFY(!errorString.isEmpty());

    // Test invalid coordinates are skipped
    const QString csvInvalid = _writeCsvFile(tmpDir, "invalid.csv",
        "lat,lon\n"
        "47.0,8.0\n"
        "999.0,8.0\n"  // Invalid latitude
        "48.0,9.0\n");

    points.clear();
    QVERIFY(CSVHelper::loadPointsFromFile(csvInvalid, points, errorString));
    QCOMPARE(points.count(), 2);  // Invalid row skipped
}

void ShapeTest::_testCSVLoadPointsWithNames()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Test loading CSV with names
    const QString csv = _writeCsvFile(tmpDir, "named_points.csv",
        "name,lat,lon\n"
        "Seattle,47.6062,-122.3321\n"
        "San Francisco,37.7749,-122.4194\n"
        "Los Angeles,34.0522,-118.2437\n");

    CSVHelper::LoadResult result = CSVHelper::loadPointsFromFile(csv);
    QVERIFY(result.success);
    QCOMPARE(result.points.count(), 3);
    QCOMPARE(result.names.count(), 3);
    QCOMPARE(result.names[0], QStringLiteral("Seattle"));
    QCOMPARE(result.names[1], QStringLiteral("San Francisco"));
    QCOMPARE(result.names[2], QStringLiteral("Los Angeles"));

    // Test CSV with names containing commas (quoted)
    const QString csvQuoted = _writeCsvFile(tmpDir, "quoted.csv",
        "name,lat,lon\n"
        "\"City, State\",47.0,8.0\n");

    result = CSVHelper::loadPointsFromFile(csvQuoted);
    QVERIFY(result.success);
    QCOMPARE(result.names[0], QStringLiteral("City, State"));
}

void ShapeTest::_testCSVSavePoints()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Create test points
    QList<QGeoCoordinate> points;
    points << QGeoCoordinate(47.6062, -122.3321, 100);
    points << QGeoCoordinate(37.7749, -122.4194, 50);

    // Save without names
    const QString file1 = tmpDir.filePath("save_points.csv");
    QVERIFY(CSVHelper::savePointsToFile(file1, points, errorString));
    QVERIFY(QFile::exists(file1));

    // Verify by loading back
    QList<QGeoCoordinate> loadedPoints;
    QVERIFY(CSVHelper::loadPointsFromFile(file1, loadedPoints, errorString));
    QCOMPARE(loadedPoints.count(), points.count());
    QCOMPARE(qRound(loadedPoints[0].latitude() * 10000), qRound(points[0].latitude() * 10000));
    QCOMPARE(qRound(loadedPoints[0].altitude()), qRound(points[0].altitude()));

    // Save with names
    QStringList names;
    names << "Seattle" << "San Francisco";
    const QString file2 = tmpDir.filePath("save_named_points.csv");
    QVERIFY(CSVHelper::savePointsToFile(file2, points, names, errorString));
    QVERIFY(QFile::exists(file2));

    // Verify names
    CSVHelper::LoadResult result = CSVHelper::loadPointsFromFile(file2);
    QVERIFY(result.success);
    QCOMPARE(result.names[0], QStringLiteral("Seattle"));
}

void ShapeTest::_testCSVDelimiterDetection()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test semicolon delimiter
    const QString csvSemicolon = _writeCsvFile(tmpDir, "semicolon.csv",
        "lat;lon;alt\n"
        "47.0;8.0;100\n"
        "48.0;9.0;200\n");

    QList<QGeoCoordinate> points;
    QVERIFY(CSVHelper::loadPointsFromFile(csvSemicolon, points, errorString));
    QCOMPARE(points.count(), 2);

    // Test tab delimiter
    const QString csvTab = _writeCsvFile(tmpDir, "tab.csv",
        "lat\tlon\talt\n"
        "47.0\t8.0\t100\n"
        "48.0\t9.0\t200\n");

    points.clear();
    QVERIFY(CSVHelper::loadPointsFromFile(csvTab, points, errorString));
    QCOMPARE(points.count(), 2);

    // Test detectDelimiter function directly
    QCOMPARE(CSVHelper::detectDelimiter("lat,lon,alt"), QChar(','));
    QCOMPARE(CSVHelper::detectDelimiter("lat;lon;alt"), QChar(';'));
    QCOMPARE(CSVHelper::detectDelimiter("lat\tlon\talt"), QChar('\t'));
}

void ShapeTest::_testCSVColumnDetection()
{
    CSVHelper::ParseOptions opts;

    // Test standard column names
    QStringList headers1 = {"lat", "lon", "alt"};
    CSVHelper::detectColumns(headers1, opts);
    QCOMPARE(opts.latColumn, 0);
    QCOMPARE(opts.lonColumn, 1);
    QCOMPARE(opts.altColumn, 2);

    // Test alternative column names
    opts = CSVHelper::ParseOptions();
    QStringList headers2 = {"longitude", "latitude", "elevation"};
    CSVHelper::detectColumns(headers2, opts);
    QCOMPARE(opts.latColumn, 1);
    QCOMPARE(opts.lonColumn, 0);
    QCOMPARE(opts.altColumn, 2);

    // Test x,y,z names
    opts = CSVHelper::ParseOptions();
    QStringList headers3 = {"x", "y", "z", "name"};
    CSVHelper::detectColumns(headers3, opts);
    QCOMPARE(opts.latColumn, 1);  // y = lat
    QCOMPARE(opts.lonColumn, 0);  // x = lon
    QCOMPARE(opts.altColumn, 2);  // z = alt
    QCOMPARE(opts.nameColumn, 3);

    // Test polyline column names
    opts = CSVHelper::ParseOptions();
    QStringList headers4 = {"lat", "lon", "sequence", "line_id"};
    CSVHelper::detectColumns(headers4, opts);
    QCOMPARE(opts.sequenceColumn, 2);
    QCOMPARE(opts.lineIdColumn, 3);
}

void ShapeTest::_testCSVLoadPolylines()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test loading single polyline (ordered by sequence)
    const QString csv1 = _writeCsvFile(tmpDir, "polyline.csv",
        "lat,lon,sequence\n"
        "47.0,8.0,0\n"
        "47.5,8.5,1\n"
        "48.0,9.0,2\n");

    QList<QGeoCoordinate> polyline;
    QVERIFY2(CSVHelper::loadPolylineFromFile(csv1, polyline, errorString),
             qPrintable(QStringLiteral("loadPolylineFromFile failed: %1").arg(errorString)));
    QCOMPARE(polyline.count(), 3);
    QCOMPARE(qRound(polyline[0].latitude() * 10), 470);
    QCOMPARE(qRound(polyline[2].latitude() * 10), 480);

    // Test loading multiple polylines (grouped by line_id)
    const QString csv2 = _writeCsvFile(tmpDir, "polylines.csv",
        "lat,lon,sequence,line_id\n"
        "47.0,8.0,0,A\n"
        "47.5,8.5,1,A\n"
        "48.0,9.0,2,A\n"
        "50.0,10.0,0,B\n"
        "50.5,10.5,1,B\n");

    QList<QList<QGeoCoordinate>> polylines;
    QVERIFY(CSVHelper::loadPolylinesFromFile(csv2, polylines, errorString));
    QCOMPARE(polylines.count(), 2);
    QCOMPARE(polylines[0].count(), 3);  // Line A
    QCOMPARE(polylines[1].count(), 2);  // Line B

    // Test out-of-order sequence is sorted
    const QString csv3 = _writeCsvFile(tmpDir, "unordered.csv",
        "lat,lon,sequence\n"
        "48.0,9.0,2\n"
        "47.0,8.0,0\n"
        "47.5,8.5,1\n");

    polyline.clear();
    QVERIFY(CSVHelper::loadPolylineFromFile(csv3, polyline, errorString));
    QCOMPARE(polyline.count(), 3);
    // Should be sorted by sequence
    QCOMPARE(qRound(polyline[0].latitude() * 10), 470);  // seq 0
    QCOMPARE(qRound(polyline[1].latitude() * 10), 475);  // seq 1
    QCOMPARE(qRound(polyline[2].latitude() * 10), 480);  // seq 2
}

void ShapeTest::_testCSVSavePolylines()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Create test polyline
    QList<QGeoCoordinate> polyline;
    polyline << QGeoCoordinate(47.0, 8.0);
    polyline << QGeoCoordinate(47.5, 8.5);
    polyline << QGeoCoordinate(48.0, 9.0);

    // Save single polyline
    const QString file1 = tmpDir.filePath("save_polyline.csv");
    QVERIFY(CSVHelper::savePolylineToFile(file1, polyline, errorString));
    QVERIFY(QFile::exists(file1));

    // Verify by loading back
    QList<QGeoCoordinate> loadedPolyline;
    QVERIFY(CSVHelper::loadPolylineFromFile(file1, loadedPolyline, errorString));
    QCOMPARE(loadedPolyline.count(), polyline.count());

    // Create multiple polylines
    QList<QList<QGeoCoordinate>> polylines;
    polylines.append(polyline);
    QList<QGeoCoordinate> polyline2;
    polyline2 << QGeoCoordinate(50.0, 10.0);
    polyline2 << QGeoCoordinate(50.5, 10.5);
    polylines.append(polyline2);

    // Save multiple polylines
    const QString file2 = tmpDir.filePath("save_polylines.csv");
    QVERIFY(CSVHelper::savePolylinesToFile(file2, polylines, errorString));
    QVERIFY(QFile::exists(file2));

    // Verify by loading back
    QList<QList<QGeoCoordinate>> loadedPolylines;
    QVERIFY(CSVHelper::loadPolylinesFromFile(file2, loadedPolylines, errorString));
    QCOMPARE(loadedPolylines.count(), 2);
}

void ShapeTest::_testCSVRoundTrip()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString errorString;

    // Test points round-trip with altitude and names
    QList<QGeoCoordinate> origPoints;
    origPoints << QGeoCoordinate(47.6062, -122.3321, 100.5);
    origPoints << QGeoCoordinate(37.7749, -122.4194, 50.25);
    origPoints << QGeoCoordinate(34.0522, -118.2437);  // No altitude
    QStringList origNames = {"Seattle", "San Francisco", "Los Angeles"};

    const QString pointsFile = tmpDir.filePath("roundtrip_points.csv");
    QVERIFY(CSVHelper::savePointsToFile(pointsFile, origPoints, origNames, errorString));

    CSVHelper::LoadResult result = CSVHelper::loadPointsFromFile(pointsFile);
    QVERIFY(result.success);
    QCOMPARE(result.points.count(), origPoints.count());
    QCOMPARE(result.names.count(), origNames.count());

    // Verify coordinate precision is maintained
    for (int i = 0; i < origPoints.count(); ++i) {
        QCOMPARE(qRound(result.points[i].latitude() * 10000),
                 qRound(origPoints[i].latitude() * 10000));
        QCOMPARE(qRound(result.points[i].longitude() * 10000),
                 qRound(origPoints[i].longitude() * 10000));
        if (!qIsNaN(origPoints[i].altitude())) {
            QCOMPARE(qRound(result.points[i].altitude() * 100),
                     qRound(origPoints[i].altitude() * 100));
        }
    }

    // Verify names preserved
    for (int i = 0; i < origNames.count(); ++i) {
        QCOMPARE(result.names[i], origNames[i]);
    }

    // Test polylines round-trip
    QList<QList<QGeoCoordinate>> origPolylines;
    QList<QGeoCoordinate> line1;
    line1 << QGeoCoordinate(47.0, 8.0, 100);
    line1 << QGeoCoordinate(47.5, 8.5, 150);
    line1 << QGeoCoordinate(48.0, 9.0, 200);
    origPolylines.append(line1);

    QList<QGeoCoordinate> line2;
    line2 << QGeoCoordinate(50.0, 10.0);
    line2 << QGeoCoordinate(51.0, 11.0);
    origPolylines.append(line2);

    const QString polylinesFile = tmpDir.filePath("roundtrip_polylines.csv");
    QVERIFY(CSVHelper::savePolylinesToFile(polylinesFile, origPolylines, errorString));

    QList<QList<QGeoCoordinate>> loadedPolylines;
    QVERIFY(CSVHelper::loadPolylinesFromFile(polylinesFile, loadedPolylines, errorString));
    QCOMPARE(loadedPolylines.count(), origPolylines.count());

    for (int i = 0; i < origPolylines.count(); ++i) {
        QCOMPARE(loadedPolylines[i].count(), origPolylines[i].count());
    }
}

UT_REGISTER_TEST(ShapeTest, TestLabel::Unit, TestLabel::Utilities)
