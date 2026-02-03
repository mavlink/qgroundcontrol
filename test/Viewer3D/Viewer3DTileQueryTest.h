#pragma once

#include "UnitTest.h"

class Viewer3DTileQuery;

class Viewer3DTileQueryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testMaxTileCount();
    void _testTileCoordinateRoundTrip();
};
