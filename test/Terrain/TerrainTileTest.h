#pragma once

#include <QtCore/QByteArray>

#include "BaseClasses/TerrainTest.h"
#include "TerrainTile.h"

class TerrainTileTest : public TerrainTest
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
    UT_PARAMETERIZED_TEST(_testCopernicusResponseSize);

private:
    static QByteArray _createValidTileData(double swLat, double swLon, double neLat, double neLon, int16_t minElev,
                                           int16_t maxElev, double avgElev, int16_t gridSizeLat, int16_t gridSizeLon,
                                           int16_t fillElevation);

    /// A response to a request for a Copernicus tile: 37 x 37 elevations of 17 significant digits, one to a line
    static QByteArray _copernicusTileResponse();
};
