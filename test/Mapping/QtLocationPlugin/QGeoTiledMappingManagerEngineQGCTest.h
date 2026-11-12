#pragma once

#include "UnitTest.h"

class QGeoTiledMappingManagerEngineQGCTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testElevationProvidersAreNotMapTypes();
    void _testMapTypeIdsAreSequential();
    void _testMapIdTranslationWithGaps();
    void _testSourceChangeGivesTilesNewVersion();
    void _testFactoryNeedsQmlEngine();
    void _testCachedTileServedWithoutProviderUrl();
    void _testUncachedTileFailsWithoutProviderUrl();
};
