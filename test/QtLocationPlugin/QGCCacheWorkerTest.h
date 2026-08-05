#pragma once

#include "UnitTest.h"

class QGCCacheWorker;

class QGCCacheWorkerTest : public UnitTest
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
    void _testResetDatabase();
    void _testStopWhileProcessing();

private:
    bool _startWorker(QGCCacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
};
