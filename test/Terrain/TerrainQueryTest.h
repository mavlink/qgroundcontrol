#pragma once

#include "BaseClasses/TerrainTest.h"

class TerrainQueryTest : public TerrainTest
{
    Q_OBJECT

private slots:
    void _testRequestCoordinateHeights();
    void _testRequestPathHeights();
    void _testRequestCarpetHeights();
    void _testRequestCarpetHeightsInvalidBounds();
    void _testPolyPathQueryEmptyPath();
    void _testPolyPathQuerySingleCoord();
    void _testTerrainAtCoordinateQuery();
};
