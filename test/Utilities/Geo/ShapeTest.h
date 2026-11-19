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
    UT_PARAMETERIZED_TEST(_testLoadShapes);
    UT_PARAMETERIZED_TEST(_testLoadFailures);
    void _testLoadFromQtResource();
    UT_PARAMETERIZED_TEST(_testSkippedVertices);
    void _testKMLExportSchemaValidation();
};
