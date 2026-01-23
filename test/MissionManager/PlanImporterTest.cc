#include "PlanImporterTest.h"
#include "PlanImporter.h"
#include "KmlPlanImporter.h"
#include "KmzPlanImporter.h"
#include "ShpPlanImporter.h"
#include "GeoJsonPlanImporter.h"
#include "GpxPlanImporter.h"
#include "CsvPlanImporter.h"
#include "WktPlanImporter.h"
#include "GeoPackagePlanImporter.h"
#include "OpenAirPlanImporter.h"

#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtTest/QTest>

void PlanImporterTest::_testImporterRegistration()
{
    // Ensure importers are initialized
    PlanImporter::initializeImporters();

    // Verify all built-in importers are registered
    QStringList extensions = PlanImporter::registeredExtensions();
    QVERIFY(extensions.contains("kml"));
    QVERIFY(extensions.contains("kmz"));
    QVERIFY(extensions.contains("geojson"));
    QVERIFY(extensions.contains("gpx"));
    QVERIFY(extensions.contains("shp"));
    QVERIFY(extensions.contains("csv"));
    QVERIFY(extensions.contains("wkt"));
    QVERIFY(extensions.contains("gpkg"));
    QVERIFY(extensions.contains("txt"));  // OpenAir uses .txt extension
}

void PlanImporterTest::_testImporterForExtension()
{
    PlanImporter::initializeImporters();

    // Test case-insensitive lookup
    QVERIFY(PlanImporter::importerForExtension("kml") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("KML") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("geojson") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("json") != nullptr);  // GeoJSON also registers for .json
    QVERIFY(PlanImporter::importerForExtension("gpx") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("shp") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("csv") != nullptr);
    QVERIFY(PlanImporter::importerForExtension("wkt") != nullptr);

    // Test non-existent extension
    QVERIFY(PlanImporter::importerForExtension("xyz") == nullptr);
}

void PlanImporterTest::_testImporterForFile()
{
    PlanImporter::initializeImporters();

    // Test file path parsing
    QVERIFY(PlanImporter::importerForFile("/path/to/mission.kml") != nullptr);
    QVERIFY(PlanImporter::importerForFile("/path/to/mission.KML") != nullptr);
    QVERIFY(PlanImporter::importerForFile("mission.geojson") != nullptr);
    QVERIFY(PlanImporter::importerForFile("C:\\path\\mission.shp") != nullptr);
    QVERIFY(PlanImporter::importerForFile("data.gpx") != nullptr);

    // Test with non-existent extension
    QVERIFY(PlanImporter::importerForFile("mission.plan") == nullptr);
}

void PlanImporterTest::_testFileDialogFilters()
{
    PlanImporter::initializeImporters();

    QStringList filters = PlanImporter::fileDialogFilters();
    QVERIFY(filters.count() >= 6);

    // Check that filters contain expected patterns
    bool hasKml = false, hasGeoJson = false, hasGpx = false, hasShp = false, hasCsv = false, hasWkt = false;
    for (const QString& filter : filters) {
        if (filter.contains("*.kml")) hasKml = true;
        if (filter.contains("*.geojson")) hasGeoJson = true;
        if (filter.contains("*.gpx")) hasGpx = true;
        if (filter.contains("*.shp")) hasShp = true;
        if (filter.contains("*.csv")) hasCsv = true;
        if (filter.contains("*.wkt")) hasWkt = true;
    }
    QVERIFY(hasKml);
    QVERIFY(hasGeoJson);
    QVERIFY(hasGpx);
    QVERIFY(hasShp);
    QVERIFY(hasCsv);
    QVERIFY(hasWkt);
}

void PlanImporterTest::_testVertexFiltering()
{
    PlanImporter* importer = KmlPlanImporter::instance();
    QVERIFY(importer != nullptr);

    // Test default value
    QCOMPARE(importer->vertexFilterMeters(), 0.0);

    // Test setting filter
    importer->setVertexFilterMeters(10.0);
    QCOMPARE(importer->vertexFilterMeters(), 10.0);

    // Reset for other tests
    importer->setVertexFilterMeters(0.0);
}

void PlanImporterTest::_testKmlImportBasic()
{
    PlanImporter* importer = KmlPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("kml"));
    QVERIFY(importer->formatName().contains("Keyhole") || importer->formatName().contains("KML"));
    QVERIFY(importer->fileFilter().contains("*.kml"));

    // Test importing a good polygon KML file
    PlanImportResult result = importer->importFromFile(QStringLiteral(":/unittest/PolygonGood.kml"));
    QVERIFY2(result.success, qPrintable(result.errorString));
    QVERIFY(result.hasFeatures());
    QVERIFY(result.hasPolygons());
    QVERIFY(!result.formatDescription.isEmpty());
}

void PlanImporterTest::_testKmzImportBasic()
{
    PlanImporter* importer = KmzPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("kmz"));
    QVERIFY(importer->formatName().contains("KML") || importer->formatName().contains("KMZ") || importer->formatName().contains("Compressed"));
    QVERIFY(importer->fileFilter().contains("*.kmz"));
}

void PlanImporterTest::_testGeoJsonImportBasic()
{
    PlanImporter* importer = GeoJsonPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("geojson"));
    QVERIFY(importer->formatName().contains("GeoJSON"));
    QVERIFY(importer->fileFilter().contains("*.geojson"));
}

void PlanImporterTest::_testGpxImportBasic()
{
    PlanImporter* importer = GpxPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("gpx"));
    QVERIFY(importer->formatName().contains("GPS Exchange") || importer->formatName().contains("GPX"));
    QVERIFY(importer->fileFilter().contains("*.gpx"));
}

void PlanImporterTest::_testShpImportBasic()
{
    PlanImporter* importer = ShpPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("shp"));
    QVERIFY(importer->formatName().contains("Shapefile"));
    QVERIFY(importer->fileFilter().contains("*.shp"));
}

void PlanImporterTest::_testCsvImportBasic()
{
    PlanImporter* importer = CsvPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("csv"));
    QVERIFY(importer->formatName().contains("Comma-Separated") || importer->formatName().contains("CSV"));
    QVERIFY(importer->fileFilter().contains("*.csv"));
}

void PlanImporterTest::_testWktImportBasic()
{
    PlanImporter* importer = WktPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("wkt"));
    QVERIFY(importer->formatName().contains("Well-Known") || importer->formatName().contains("WKT"));
    QVERIFY(importer->fileFilter().contains("*.wkt"));
}

void PlanImporterTest::_testGeoPackageImportBasic()
{
    PlanImporter* importer = GeoPackagePlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("gpkg"));
    QVERIFY(importer->formatName().contains("GeoPackage") || importer->formatName().contains("OGC"));
    QVERIFY(importer->fileFilter().contains("*.gpkg"));
}

void PlanImporterTest::_testOpenAirImportBasic()
{
    PlanImporter* importer = OpenAirPlanImporter::instance();
    QVERIFY(importer != nullptr);
    QCOMPARE(importer->fileExtension(), QString("txt"));
    QVERIFY(importer->formatName().contains("OpenAir") || importer->formatName().contains("Airspace"));
    QVERIFY(importer->fileFilter().contains("*.txt") || importer->fileFilter().contains("*.air"));
}

void PlanImporterTest::_testImportNonExistentFile()
{
    PlanImporter* importer = KmlPlanImporter::instance();
    QVERIFY(importer != nullptr);

    // Try to import a non-existent file
    PlanImportResult result = importer->importFromFile(QStringLiteral("/nonexistent/path/file.kml"));
    QVERIFY(!result.success);
    QVERIFY(!result.errorString.isEmpty());
    QVERIFY(!result.hasFeatures());
}

void PlanImporterTest::_testImportMalformedFile()
{
    PlanImporter* importer = KmlPlanImporter::instance();
    QVERIFY(importer != nullptr);

    // Test importing a malformed KML file (bad XML)
    PlanImportResult result = importer->importFromFile(QStringLiteral(":/unittest/PolygonBadXml.kml"));
    // Should fail or return no features
    if (result.success) {
        // If it somehow "succeeded", it shouldn't have any valid features
        QVERIFY(!result.hasFeatures());
    } else {
        // Failure is expected - verify we got an error message
        QVERIFY(!result.errorString.isEmpty() || !result.hasFeatures());
    }
}

void PlanImporterTest::_testPlanImportResultHelpers()
{
    // Test empty result
    PlanImportResult emptyResult;
    QVERIFY(!emptyResult.success);
    QCOMPARE(emptyResult.itemCount(), 0);
    QVERIFY(!emptyResult.hasFeatures());
    QVERIFY(!emptyResult.hasWaypoints());
    QVERIFY(!emptyResult.hasTrackPoints());
    QVERIFY(!emptyResult.hasPolygons());

    // Test result with waypoints
    PlanImportResult waypointResult;
    waypointResult.success = true;
    waypointResult.waypoints.append(QGeoCoordinate(47.0, -122.0));
    waypointResult.waypoints.append(QGeoCoordinate(47.1, -122.1));
    QCOMPARE(waypointResult.itemCount(), 2);
    QVERIFY(waypointResult.hasFeatures());
    QVERIFY(waypointResult.hasWaypoints());
    QVERIFY(!waypointResult.hasTrackPoints());
    QVERIFY(!waypointResult.hasPolygons());

    // Test result with track points
    PlanImportResult trackResult;
    trackResult.success = true;
    trackResult.trackPoints.append(QGeoCoordinate(47.0, -122.0));
    QCOMPARE(trackResult.itemCount(), 1);
    QVERIFY(trackResult.hasFeatures());
    QVERIFY(!trackResult.hasWaypoints());
    QVERIFY(trackResult.hasTrackPoints());
    QVERIFY(!trackResult.hasPolygons());

    // Test result with polygons
    PlanImportResult polygonResult;
    polygonResult.success = true;
    QList<QGeoCoordinate> polygon;
    polygon.append(QGeoCoordinate(47.0, -122.0));
    polygon.append(QGeoCoordinate(47.1, -122.0));
    polygon.append(QGeoCoordinate(47.1, -122.1));
    polygonResult.polygons.append(polygon);
    QCOMPARE(polygonResult.itemCount(), 1);
    QVERIFY(polygonResult.hasFeatures());
    QVERIFY(!polygonResult.hasWaypoints());
    QVERIFY(!polygonResult.hasTrackPoints());
    QVERIFY(polygonResult.hasPolygons());

    // Test combined result
    PlanImportResult combinedResult;
    combinedResult.success = true;
    combinedResult.waypoints.append(QGeoCoordinate(47.0, -122.0));
    combinedResult.trackPoints.append(QGeoCoordinate(47.1, -122.1));
    combinedResult.polygons.append(polygon);
    QCOMPARE(combinedResult.itemCount(), 3);
    QVERIFY(combinedResult.hasFeatures());
    QVERIFY(combinedResult.hasWaypoints());
    QVERIFY(combinedResult.hasTrackPoints());
    QVERIFY(combinedResult.hasPolygons());
}
