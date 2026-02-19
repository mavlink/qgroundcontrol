#pragma once

#include "TempDirectoryTest.h"

class OsmParserThreadTest : public TempDirectoryTest
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
