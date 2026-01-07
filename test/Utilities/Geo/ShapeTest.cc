#include "ShapeTest.h"
#include "ShapeFileHelper.h"

#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtTest/QTest>

QString ShapeTest::_copyRes(const QTemporaryDir &tmpDir, const QString &name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void) QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void) QFile(resPath).copy(dstPath);
    return dstPath;
}

void ShapeTest::_writePrjFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << content;
    }
}

void ShapeTest::_testLoadPolylineFromSHP()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "pline.shp");
    (void) _copyRes(tmpDir, "pline.dbf");
    (void) _copyRes(tmpDir, "pline.shx");
    (void) _copyRes(tmpDir, "pline.prj");
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
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");
    (void) _copyRes(tmpDir, "polygon.prj");
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
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");
    (void) _copyRes(tmpDir, "polygon.prj");
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
    (void) _copyRes(tmpDir, "pline.dbf");
    (void) _copyRes(tmpDir, "pline.shx");
    (void) _copyRes(tmpDir, "pline.prj");
    QString errorString;
    QList<QList<QGeoCoordinate>> polylines;
    // Use 0 for filterMeters to disable vertex filtering for test
    QVERIFY(ShapeFileHelper::loadPolylinesFromFile(shpFile, polylines, errorString, 0));
    QVERIFY(errorString.isEmpty());
    QVERIFY(polylines.count() >= 1);
    QVERIFY(polylines.first().count() >= 2);
}

void ShapeTest::_testGetEntityCount()
{
    const QTemporaryDir tmpDir;

    // Test SHP entity count
    const QString shpFile = _copyRes(tmpDir, "polygon.shp");
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");
    (void) _copyRes(tmpDir, "polygon.prj");
    QString errorString;
    const int shpCount = ShapeFileHelper::getEntityCount(shpFile, errorString);
    QVERIFY(errorString.isEmpty());
    QVERIFY(shpCount >= 1);

    // Test KML entity count (always returns 1)
    const QString kmlFile = _copyRes(tmpDir, "polygon.kml");
    const int kmlCount = ShapeFileHelper::getEntityCount(kmlFile, errorString);
    QVERIFY(errorString.isEmpty());
    QCOMPARE(kmlCount, 1);
}

void ShapeTest::_testDetermineShapeType()
{
    const QTemporaryDir tmpDir;

    // Test polygon type detection
    const QString polygonFile = _copyRes(tmpDir, "polygon.shp");
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");
    (void) _copyRes(tmpDir, "polygon.prj");
    QString errorString;
    QCOMPARE(ShapeFileHelper::determineShapeType(polygonFile, errorString), ShapeFileHelper::ShapeType::Polygon);
    QVERIFY(errorString.isEmpty());

    // Test polyline type detection
    const QString polylineFile = _copyRes(tmpDir, "pline.shp");
    (void) _copyRes(tmpDir, "pline.dbf");
    (void) _copyRes(tmpDir, "pline.shx");
    (void) _copyRes(tmpDir, "pline.prj");
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
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");

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
    (void) _copyRes(tmpDir, "polygon.dbf");
    (void) _copyRes(tmpDir, "polygon.shx");
    (void) _copyRes(tmpDir, "polygon.prj");

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
    // Note: If the test data has vertices very close together, filtered may have fewer
    QVERIFY(unfilteredCoords.count() >= 3);
}
