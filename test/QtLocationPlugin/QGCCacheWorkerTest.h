#pragma once

#include "BaseClasses/TempDirectoryTest.h"

class QGCCacheWorker;

class QGCCacheWorkerTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _testStartAndStop();
    void _testEnqueueBeforeInit();
    void _testUpdateTotalsOnInit();
    void _testSaveAndFetchTile();
    void _testFetchTileNotFound();
    void _testFetchTileSets();
    void _testCreateAndDeleteTileSet();
    void _testPruneCache();
    void _testResetDatabase();
    void _testStopWhileProcessing();

private:
    bool _startWorker(QGCCacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
};
