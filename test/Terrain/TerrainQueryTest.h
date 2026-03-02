#pragma once

#include "BaseClasses/TerrainTest.h"

class TerrainQueryTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void _testRequestCoordinateHeights();
    void _testRequestPathHeights();
    void _testRequestPathHeightsSpacing();
    void _testRequestCarpetHeights();
    void _testRequestCarpetHeightsStatsOnly();
    void _testRequestCarpetHeightsInvalidBounds();
    void _testPolyPathQueryEmptyPath();
    void _testPolyPathQuerySingleCoord();
    void _testPolyPathQueryFailureClearsAccumulatedSegments();
    void _testTerrainAtCoordinateQuery();
};
