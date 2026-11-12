#pragma once

#include "UnitTest.h"

class Viewer3DTileQuery;

class Viewer3DTileQueryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testMaxTileCount();
    void _testTileCoordinateRoundTrip();
    void _testCoarserZoomWhereServerHasNoTile_data();
    void _testCoarserZoomWhereServerHasNoTile();
    void _testLoadOutcome_data();
    void _testLoadOutcome();
    void _testUnansweredTileAskedAgainThenGivenUp();
};
