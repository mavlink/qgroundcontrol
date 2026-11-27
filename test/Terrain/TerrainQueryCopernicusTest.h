#pragma once

#include "UnitTest.h"

class TerrainQueryCopernicusTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCarpetHeights_data();
    void _testCarpetHeights();
    void _testCarpetAnswerThatIsNoCarpetFails_data();
    void _testCarpetAnswerThatIsNoCarpetFails();
    void _testCarpetRequestThatFailsFails();
    void _testQueriesTheServerLacksFailWithoutRequest();
};
