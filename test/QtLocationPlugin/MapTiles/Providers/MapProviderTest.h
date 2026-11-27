#pragma once

#include "UnitTest.h"

class MapProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetImageFormat_data();
    void _testGetImageFormat();
    void _testLong2tileX_data();
    void _testLong2tileX();
    void _testLat2tileY_data();
    void _testLat2tileY();
    void _testGetTileCount();
    void _testGetTileCountAcrossAntimeridian();
    void _testGetTileCountBeyondMap_data();
    void _testGetTileCountBeyondMap();
    void _testGettersReturnConstructorValues();
    void _testFixedSourcesHaveAttribution();
    void _testTileIsNoRefusalImage();
    void _testTileOfNoTileImageSizeIsTile();
    void _testImageToldBySizeAndSha256();
};
