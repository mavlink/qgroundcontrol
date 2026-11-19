#pragma once

#include <functional>
#include <optional>

#include <QtCore/QString>

#include "QGCCacheTile.h"
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
    void _testMapEnginePrunesAgainAfterFailedPrune();
    void _testCreateTileSetKeepsSourceOfItsCreation();
    void _testImportGivesOldTilesTheConfiguredSource();
    void _testMapEngineGivesOldTilesTheConfiguredSource();
    void _testMapEngineReportsSourceChange_data();
    void _testMapEngineReportsSourceChange();
    void _testFreePagesReleasedWhenIdle_data();
    void _testFreePagesReleasedWhenIdle();
    void _testTileFetchedBeforeFreePagesReleased();

private:
    using AddTask = std::function<bool(QGCMapTask*)>;

    bool _startWorker(QGCCacheWorker& worker, int timeoutMs = TestTimeout::mediumMs());
    /// The tile the cache has for \a hash, asked through \a addTask of a worker or an engine. None for a tile the
    /// cache lacks, and for a question that got no answer.
    std::optional<QGCCacheTile> _fetchTile(const AddTask& addTask, const QString& hash);
    /// Queues \a task through \a addTask and waits for its end. The error of the task, or none for a task that got
    /// no end.
    std::optional<QString> _runTask(const AddTask& addTask, QGCMapTask* task);
};
