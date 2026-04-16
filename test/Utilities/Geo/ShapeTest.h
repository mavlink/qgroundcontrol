#pragma once

#include "TempDirectoryTest.h"

class ShapeTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _testLoadPolylineFromSHP();
    void _testLoadPolylineFromKML();
    void _testLoadPolygonFromSHP();
    void _testLoadPolygonFromKML();
    void _testLoadPolygonsFromSHP();
    void _testLoadPolylinesFromSHP();
    void _testLoadPolygonsFromKML();
    void _testLoadPolylinesFromKML();
    void _testGetEntityCount();
    void _testDetermineShapeType();
    void _testUnsupportedProjectionError();
    void _testLoadFromQtResource();
    void _testVertexFiltering();
    void _testKMLVertexFiltering();
    void _testKMLAltitudeParsing();
    void _testKMLCoordinateValidation();
    void _testKMLExportSchemaValidation();

private:
    static QString _copyRes(const QString& dirPath, const QString& name);
    static void _writePrjFile(const QString& path, const QString& content);
    static QString _writeKmlFile(const QString& dirPath, const QString& name, const QString& content);
};
