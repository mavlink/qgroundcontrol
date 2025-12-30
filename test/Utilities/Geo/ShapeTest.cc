#include "ShapeTest.h"
#include "ShapeFileHelper.h"

#include <QtTest/QTest>

QString ShapeTest::_copyRes(const QTemporaryDir &tmpDir, const QString &name)
{
    const QString dstPath = tmpDir.filePath(name);
    (void) QFile::remove(dstPath);
    const QString resPath = QStringLiteral(":/unittest/%1").arg(name);
    (void) QFile(resPath).copy(dstPath);
    return dstPath;
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
    QVERIFY(ShapeFileHelper::loadPolylineFromFile(shpFile, rgCoords, errorString));
}

void ShapeTest::_testLoadPolylineFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polyline.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(ShapeFileHelper::loadPolylineFromFile(shpFile, rgCoords, errorString));
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
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, rgCoords, errorString));
}

void ShapeTest::_testLoadPolygonFromKML()
{
    const QTemporaryDir tmpDir;
    const QString shpFile = _copyRes(tmpDir, "polygon.kml");
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    QVERIFY(ShapeFileHelper::loadPolygonFromFile(shpFile, rgCoords, errorString));
}
