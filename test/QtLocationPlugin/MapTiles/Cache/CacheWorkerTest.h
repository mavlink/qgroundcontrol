#pragma once

#include "UnitTest.h"

class CacheWorker;

class CacheWorkerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void _testStartAndStop();
    void _testEnqueueBeforeInit();
    void _testUpdateTotalsOnInit();
    void _testSaveAndFetchTile();
    void _testFetchTileNotFound();
    void _testFetchMapTileMissServesFakeTile();
    void _testFetchElevationTileMissServesSyntheticTerrain();
    void _testFetchTileSets();
    void _testCreateAndDeleteTileSet();
    void _testPruneCache();
    void _testPruneKeepsTileThatWasFetched();
    void _testResetDatabase();
    void _testStopWhileProcessing();
    void _testTaskQueuedAsWorkerQuitsIsRefused();
    void _testCreateTileSetKeepsSourceOfItsCreation();
    void _testImportGivesOldTilesTheConfiguredSource();
    void _testImportNamesSetOfMBTilesFileWithoutName();
    void _testImportOfMBTilesFileReplacesCacheWhenAsked_data();
    void _testImportOfMBTilesFileReplacesCacheWhenAsked();
    void _testFreePagesReleasedWhenIdle_data();
    void _testFreePagesReleasedWhenIdle();
    void _testTileFetchedBeforeFreePagesReleased();

private:
    bool _startWorker(CacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
};
