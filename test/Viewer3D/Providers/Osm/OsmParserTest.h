#pragma once

#include "UnitTest.h"

class OsmParser;

class OsmParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testBuildingToMeshEmpty();
    void _testTriangulateWalls();
    void _testTriangulateWallsInverse();
    void _testTriangulateRectangle();
    void _testTriangulateRectangleInverted();
    void _testSetBuildingLevelHeight();
    void _testGpsRefSetReset();
};
