#pragma once

#include "UnitTest.h"
#include "TerrainTile.h"

#include <QtCore/QByteArray>

class TerrainTileTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testValidTile();
    void _testEmptyData();
    void _testDataTooSmallForHeader();
    void _testZeroGridDimensions();
    void _testNegativeGridDimensions();
    void _testExcessiveGridDimensions();
    void _testInfeasibleTileExtent();
    void _testDataTooSmallForElevation();
    void _testElevationOutsideBounds();
    void _testInvalidTileElevation();

private:
    static QByteArray _createValidTileData(
        double swLat, double swLon, double neLat, double neLon,
        int16_t minElev, int16_t maxElev, double avgElev,
        int16_t gridSizeLat, int16_t gridSizeLon,
        int16_t fillElevation);
};
