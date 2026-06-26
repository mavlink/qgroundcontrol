#pragma once

#include "QtLocationTestBase.h"

class QGeoTiledMapReplyQGCTest : public QtLocationTestBase
{
    Q_OBJECT

private slots:
    void _testCacheReplyMarksCachedAndFinished();
    void _testCacheReplyNullTileSetsError();
    void _testNonExpiredCacheServedWithoutNetworkManager();
    void _testHandleNetworkResultSuccessCachesAndFinishes();
    void _testHandleNetworkResultHttpErrorSetsError();
    void _testHandleNetworkResult304WithoutCacheSetsError();
    void _testFreshCacheServedWithoutRevalidation();
    void _testMustRevalidateCacheTileNotServedDirectly();
    void _testExpiredRevalidatableTileServedStale();
};
