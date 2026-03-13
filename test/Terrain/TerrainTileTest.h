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
    void _testArduPilotParseFileName();
    void _testArduPilotDetectDimension();
    void _testArduPilotParseCoordinateData();
    void _testArduPilotSerializeSRTM1();
    void _testArduPilotSerializeSRTM3();
    void _testArduPilotSerializeInvalidSize();
    void _testArduPilotSerializeInvalidFilename();

private:
    static QByteArray _createValidTileData(double swLat, double swLon, double neLat, double neLon, int16_t minElev,
                                           int16_t maxElev, double avgElev, int16_t gridSizeLat, int16_t gridSizeLon,
                                           int16_t fillElevation);
};
