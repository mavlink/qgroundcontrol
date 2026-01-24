#pragma once

#include "TestFixtures.h"

/// Unit test for shape file loading.
/// Uses TempDirTest for automatic temp directory management.
class ShapeTest : public TempDirTest
{
    Q_OBJECT

public:
    ShapeTest() = default;

private slots:

    // SHP Loading Tests
    void _testLoadPolylineFromSHP();
    void _testLoadPolygonFromSHP();
    void _testLoadPolygonsFromSHP();
    void _testLoadPolylinesFromSHP();

    // KML Loading Tests
    void _testLoadPolylineFromKML();
    void _testLoadPolygonFromKML();
    void _testLoadPolygonsFromKML();
    void _testLoadPolylinesFromKML();

    // Utility Function Tests
    void _testGetEntityCount();
    void _testDetermineShapeType();
    void _testUnsupportedProjectionError();
    void _testLoadFromQtResource();

    // Filtering and Parsing Tests
    void _testVertexFiltering();
    void _testKMLVertexFiltering();
    void _testKMLAltitudeParsing();
    void _testKMLCoordinateValidation();

    // Schema Validation Tests
    void _testKMLExportSchemaValidation();

private:
    /// Copy a resource file to the temp directory
    QString _copyRes(const QString &name);

    /// Write a PRJ file with custom content
    void _writePrjFile(const QString &path, const QString &content);

    /// Write a KML file with custom content
    QString _writeKmlFile(const QString &name, const QString &content);
};
