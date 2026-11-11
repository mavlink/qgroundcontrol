#pragma once

#include "UnitTest.h"

class OsmParserThreadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testParseValidOsmFile();
    void _testParseInvalidFile();
    void _testParseEmptyFile();
    void _testBuildingTypeAppend();
    void _testBuildingTypeBoundingBox();
    void _testParseMultipleBuildings();
    void _testParseMultipolygonRelation();
    void _benchmarkParseOsmFile();
};
