#pragma once

#include "UnitTest.h"

class ElevationProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSRTM1ProviderConstants();
    void _testSRTM3ProviderConstants();
    void _testOpenElevationProviderConstants();
    void _testSRTM1TileCoordinates();
    void _testSRTM3TileCoordinates();
    void _testOpenElevationTileNoOp();
    void _testSRTM1TileCount();
    void _testSRTM3TileCount();
    void _testOpenElevationTileCountEmpty();
    void _testSRTM1SerializeFromResource();
};
