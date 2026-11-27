#pragma once

#include "UnitTest.h"

class MapEngineManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testTerrainSet_data();
    void _testTerrainSet();
    void _testElevationTypeSet_data();
    void _testElevationTypeSet();
    void _testSetStopsAtDeepestZoomOfProvider_data();
    void _testSetStopsAtDeepestZoomOfProvider();
    void _testTerrainSetServedFromCache();
    void _testNewSetTellsManagerOfItsSelection();
    void _testReloadKeepsSetsAndTheirDownloads();
    void _testReloadDropsSetsThatTheCacheLacks();
    void _testClearCachedTilesKeepsOfflineSets();
    void _testShowsErrorOfDownloadOfSet();
    void _testDeletedSetStopsDownloading_data();
    void _testDeletedSetStopsDownloading();
    void _testFailedDeleteLeavesNothingDeleting_data();
    void _testFailedDeleteLeavesNothingDeleting();
    void _testRenamedSetGetsNameThatNoOtherSetHas();
    void _testExportsSelectedSets();
    void _testFailedImportEndsTheImport();
    void _testImportedMBTilesFileIsAMapOfItsTiles();
};
