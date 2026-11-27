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
    void _testTileOutcome_data();
    void _testTileOutcome();
    void _testNewLoadAbortsEarlierLoad();
    void _testTileThatFailsForNowIsAskedForAgain_data();
    void _testTileThatFailsForNowIsAskedForAgain();
    void _testTileThatKeepsFailingIsLeftOut();
    void _testNewLoadDropsRetryOfEarlierLoad();
};
