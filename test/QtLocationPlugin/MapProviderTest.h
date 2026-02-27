#pragma once

#include "UnitTest.h"

class MapProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetImageFormatPng();
    void _testGetImageFormatJpeg();
    void _testGetImageFormatGif();
    void _testGetImageFormatFallback();
    void _testGetImageFormatTooSmall();
    void _testLong2tileXZoom0();
    void _testLong2tileXZoom1();
    void _testLong2tileXNegative();
    void _testLong2tileXBoundary180();
    void _testLat2tileYEquator();
    void _testLat2tileYNorthern();
    void _testLat2tileYPoles();
    void _testGetTileCountSingleTile();
    void _testGetTileCountMultipleTiles();
    void _testGetTileCountSizeMatchesAvg();
    void _testQuadKeyLevel1();
    void _testQuadKeyLevel3();
    void _testQuadKeyLevel0();
    void _testGetServerNumBasic();
    void _testGetServerNumWrap();
    void _testGettersReturnConstructorValues();
};
