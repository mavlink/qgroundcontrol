#pragma once

#include "UnitTest.h"

class TileCacheTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCloseStopsWorkerAndRefusesInit();
    void _testEnsureInitializedAcceptsTasks();
    void _testTellsUserOfCacheThatCannotBeOpened();
    void _testPrunesAgainAfterFailedPrune();
    void _testPrunesAgainAfterShutdownDroppedPrune();
    void _testGivesOldTilesTheConfiguredSource();
};
