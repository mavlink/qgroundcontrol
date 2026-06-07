#pragma once

#include "UnitTest.h"

class QGCMapTaskTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testBaseTypeAndError();
    void _testFetchTileSetTask();
    void _testCreateTileSetTaskSavedTransfersOwnership();
    void _testCreateTileSetTaskUnsavedDeletesTileSet();
    void _testFetchTileTask();
    void _testSaveTileTaskDeletesTileOnDestroy();
    void _testGetTileDownloadListTask();
    void _testCommandTask();
    void _testExportTileTask();
    void _testImportTileTask();
};
