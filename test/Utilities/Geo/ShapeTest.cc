#include "ShapeTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>

#include "KMLDomDocument.h"
#include "KMLSchemaValidator.h"
#include "ShapeFileHelper.h"

QString ShapeTest::_copyRes(const QTemporaryDir& tmpDir, const QString& name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void)QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void)QFile(resPath).copy(dstPath);
    return dstPath;
}

void ShapeTest::_writePrjFile(const QString& path, const QString& content)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << content;
    }
}

QString ShapeTest::_writeKmlFile(const QTemporaryDir& tmpDir, const QString& name, const QString& content)
{
    const QString path = tmpDir.filePath(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << content;
    }
    return path;
}

void ShapeTest::_testLoadPolylineFromSHP()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "pline.shp");
    (void)_copyRes(tmpDir, "pline.dbf");
    (void)_copyRes(tmpDir, "pline.shx");
    (void)_copyRes(tmpDir, "pline.prj");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(ShapeFileHelper::loadPolylineFromFile(shpFile, rgCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() > 0);
}

void ShapeTest::_testLoadPolylineFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polyline.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(ShapeFileHelper::loadPolylineFromFile(shpFile, rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() > 0);
}

void ShapeTest::_testLoadPolygonFromSHP()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    (void)_copyRes(tmpDir, "polygon.prj");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, rgCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testLoadPolygonFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testLoadPolygonsFromSHP()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    (void)_copyRes(tmpDir, "polygon.prj");
    QString errorString;
    QList<QList<QGeoCoordinate>> polygons;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(ShapeFileHelper::loadPolygonsFromFile(shpFile, polygons, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polygons.count() >= 1);
    QVERIFY(polygons.first().count() >= 3);
}

void ShapeTest::_testLoadPolylinesFromSHP()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "pline.shp");
    (void)_copyRes(tmpDir, "pline.dbf");
    (void)_copyRes(tmpDir, "pline.shx");
    (void)_copyRes(tmpDir, "pline.prj");
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(ShapeFileHelper::loadPolylinesFromFile(shpFile, polylines, errorString, 0));
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
    QVERIFY(ShapeFileHelper::loadPolygonsFromFile(kmlFile, polygons, errorString, 0));
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
    QVERIFY(ShapeFileHelper::loadPolylinesFromFile(kmlFile, polylines, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polylines.count() >= 1);
    QVERIFY(polylines.first().count() >= 2);
}

void ShapeTest::_testGetEntityCount()
{
    const QTemporaryDir tmpDir;
    // Test SHP entity count
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    (void)_copyRes(tmpDir, "polygon.prj");
    QString errorString;
    const int shpCount = ShapeFileHelper::getEntityCount(shpFile, errorString);
    QVERIFY(errorString.isEmpty());
    QVERIFY(shpCount >= 1);
    // Test KML polygon entity count
    const QString kmlPolygonFile = _copyRes(tmpDir, "polygon.kml");
    const int kmlPolygonCount = ShapeFileHelper::getEntityCount(kmlPolygonFile, errorString);
    QVERIFY(errorString.isEmpty());
    QCOMPARE(kmlPolygonCount, 1);
    // Test KML polyline entity count
    const QString kmlPolylineFile = _copyRes(tmpDir, "polyline.kml");
    const int kmlPolylineCount = ShapeFileHelper::getEntityCount(kmlPolylineFile, errorString);
    QVERIFY(errorString.isEmpty());
    QCOMPARE(kmlPolylineCount, 1);
}

void ShapeTest::_testDetermineShapeType()
{
    const QTemporaryDir tmpDir;
    // Test polygon type detection
    const QString polygonFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    (void)_copyRes(tmpDir, "polygon.prj");
    QString errorString;
    QCOMPARE(ShapeFileHelper::determineShapeType(polygonFile, errorString), ShapeFileHelper::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());
    // Test polyline type detection
    const QString polylineFile = _copyRes(tmpDir, "pline.shp");
    (void)_copyRes(tmpDir, "pline.dbf");
    (void)_copyRes(tmpDir, "pline.shx");
    (void)_copyRes(tmpDir, "pline.prj");
    QCOMPARE(ShapeFileHelper::determineShapeType(polylineFile, errorString), ShapeFileHelper::ShapeType::Polyline);
    QVERIFY(errorString.isEmpty());
    // Test KML type detection
    const QString kmlFile = _copyRes(tmpDir, "polygon.kml");
    QCOMPARE(ShapeFileHelper::determineShapeType(kmlFile, errorString), ShapeFileHelper::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());
}

void ShapeTest::_testUnsupportedProjectionError()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    // Write an unsupported projection PRJ file
    const QString prjPath = tmpDir.filePath("polygon.prj");
    _writePrjFile(prjPath, "PROJCS[\"NAD_1983_StatePlane_California_III\",GEOGCS[\"GCS_North_American_1983\"]]");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(!ShapeFileHelper::loadPolygonFromFile(shpFile, rgCoords, errorString));
    QVERIFY(errorString.contains("NAD_1983_StatePlane_California_III"));
    QVERIFY(errorString.contains("WGS84"));
}

void ShapeTest::_testLoadFromQtResource()
{
    // Test loading directly from Qt resource path (tests QFile hooks)
    // Note: This requires the test resources to include all required files (.shp, .shx, .prj)
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    // Test KML from resource (KML already uses QFile internally)
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(":/unittest/polygon.kml", rgCoords, errorString));
    QVERIFY(errorString.isEmpty());
    QVERIFY(rgCoords.count() >= 3);
}

void ShapeTest::_testVertexFiltering()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void)_copyRes(tmpDir, "polygon.dbf");
    (void)_copyRes(tmpDir, "polygon.shx");
    (void)_copyRes(tmpDir, "polygon.prj");
    QString errorString;
    QList<QGeoCoordinate> unfilteredCoords;
    QList<QGeoCoordinate> filteredCoords;
    // Load without filtering
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, unfilteredCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    // Load with filtering (default 5m)
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, filteredCoords, errorString));
    QVERIFY(errorString.isEmpty());
    // Filtered should have same or fewer vertices than unfiltered
    QVERIFY(filteredCoords.count() <= unfilteredCoords.count());
    // Both should have at least minimum valid polygon (3 vertices)
    QVERIFY(unfilteredCoords.count() >= 3);
    QVERIFY(filteredCoords.count() >= 3);
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
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(kmlFile, unfilteredCoords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QCOMPARE(unfilteredCoords.count(), 6);  // 7 coords - 1 duplicate closing = 6
    // Load with filtering (default 5m)
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(kmlFile, filteredCoords, errorString));
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
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(kmlFile, coords, errorString, 0));
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
    // Expect warnings for invalid coordinates
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Latitude out of range.*91"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Longitude out of range.*200"));
    QString errorString;
    QList<QGeoCoordinate> coords;
    // Should succeed but skip invalid coordinates
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(kmlFile, coords, errorString, 0));
    QVERIFY(errorString.isEmpty());
    // Should have 3 valid coordinates (the two invalid ones skipped, plus duplicate closing removed)
    // Valid: -122,37 | -121,38 | -121,37
    // Invalid: -122,91 (lat>90) | 200,38 (lon>180)
    QCOMPARE(coords.count(), 3);
    // Verify the valid coordinates are present
    QCOMPARE(coords[0].latitude(), 37.0);
    QCOMPARE(coords[0].longitude(), -122.0);
    QCOMPARE(coords[1].latitude(), 38.0);
    QCOMPARE(coords[1].longitude(), -121.0);
    QCOMPARE(coords[2].latitude(), 37.0);
    QCOMPARE(coords[2].longitude(), -121.0);
}

void ShapeTest::_testKMLExportSchemaValidation()
{
    // Test that KMLSchemaValidator is properly loaded and functional
    const auto* validator = KMLSchemaValidator::instance();
    // Verify schema was loaded and enum types were extracted
    const QStringList altitudeModes = validator->validEnumValues("altitudeModeEnumType");
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
    if (!result.isValid) {
        qWarning() << "KML validation errors:" << result.errors;
    }
    QVERIFY(result.isValid);
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

UT_REGISTER_TEST(ShapeTest, TestLabel::Unit, TestLabel::Utilities)
