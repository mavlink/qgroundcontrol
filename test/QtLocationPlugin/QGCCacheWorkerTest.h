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
    void _testEnqueueAfterStop();
    void _testUpdateTotalsOnInit();
    void _testGetTotalsTask();
    void _testSaveAndFetchTile();
    void _testFetchTileNotFound();
    void _testFetchTileSets();
    void _testFetchTileSetsMainThreadAffinity();
    void _testCreateAndDeleteTileSet();
    void _testPruneCache();
    void _testResetDatabase();
    void _testStopWhileProcessing();
    void _testCacheTileBatchCoalesce();

private:
    bool _startWorker(QGCCacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
};
