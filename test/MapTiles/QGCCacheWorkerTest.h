#pragma once

#include <functional>

#include "UnitTest.h"

class QGCCacheWorker;
class QGCMapTask;

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
    void _testMapEngineCloseStopsWorkerAndRefusesInit();
    void _testMapEngineEnsureInitializedAcceptsTasks();
    void _testCreateTileSetKeepsSourceOfItsCreation();
    void _testImportGivesOldTilesTheConfiguredSource();
    void _testMapEngineGivesOldTilesTheConfiguredSource();
    void _testFreePagesReleasedWhenIdle_data();
    void _testFreePagesReleasedWhenIdle();
    void _testTileFetchedBeforeFreePagesReleased();

private:
    bool _startWorker(QGCCacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
    /// The image the cache has for \a hash, asked through \a addTask of a worker or an engine. Empty without one.
    QByteArray _fetchTileImage(const std::function<bool(QGCMapTask*)>& addTask, const QString& hash);
};
