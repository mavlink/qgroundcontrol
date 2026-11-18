#pragma once

#include "UnitTest.h"

class ShapeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLoadPolygonsFromSHP();
    void _testLoadPolylinesFromSHP();
    void _testLoadPolygonsFromKML();
    void _testLoadPolylinesFromKML();
    void _testLoadShapes_data();
    void _testLoadShapes();
    void _testLoadFailures_data();
    void _testLoadFailures();
    void _testLoadFromQtResource();
    void _testSkippedVertices_data();
    void _testSkippedVertices();
    void _testKMLExportSchemaValidation();
};
