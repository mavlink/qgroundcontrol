#pragma once

#include "UnitTest.h"

class TerrainTileManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testUnknownElevationProviderFailsTheQuery();
    void _testFailedTileFailsOnlyQueriesThatNeedIt();
    void _testFailedTileFailsPathAndCarpetQueries();
    void _testAltitudesAreGivenOnceTheirTileIsInMemory();
    void _testUnreadableCachedTileIsFetchedAgain();
};
