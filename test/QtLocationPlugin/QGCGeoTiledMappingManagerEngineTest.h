#pragma once

#include "UnitTest.h"

class QGCGeoTiledMappingManagerEngineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testElevationProvidersAreNotMapTypes();
    void _testNightMapTypeIsFlagged();
    void _testMapTypeIdsAreSequential();
    void _testMapIdTranslationWithGaps();
    void _testSourceChangeGivesTilesNewVersion();
    void _testPluginLoadsThroughQtLocation();
    void _testFactoryNeedsMainThread();
    void _testCachedTileServedWithoutProviderUrl();
    void _testTileThatCannotBeAskedForEndsWithoutTile_data();
    void _testTileThatCannotBeAskedForEndsWithoutTile();
    void _testTileWithoutPictureIsNotAskedForAgain_data();
    void _testTileWithoutPictureIsNotAskedForAgain();
    void _testTileBelowDeepestZoomShownFromAbove();
    void _testMapCreditsProviderOfItsMapType();
    void _testMapCreditsAreaOfTurnedView();
    void _testPrefetchStyleReachesExistingMaps();
    void _testTilesOfViewAskedForNearestCentreFirst();
    void _testTileCacheStaysInMemory();
    void _testTileCacheReportsBadTile();
    void _testTileFromAboveMadeAgainAfterEviction();
};
