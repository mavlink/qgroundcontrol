#pragma once

#include <memory>

#include "DownloadTile.h"
#include "UnitTest.h"

class TileCacheDatabase;

class TileCacheDatabaseTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitWithValidPath();
    void _testInitWithEmptyPath();
    void _testInitWithPathThatCannotBeOpened();
    void _testConnectDisconnect();
    void _testDefaultTileSetCreated();
    void _testSaveTileAndGetTile();
    void _testFindTile();
    void _testGetTileSetsMultiple();
    void _testRenameTileSet();
    void _testFindTileSetID();
    void _testDeleteTileSet();
    void _testResetDatabase();
    void _testComputeTotalsOfDefaultSet_data();
    void _testComputeTotalsOfDefaultSet();
    void _testComputeSetTotalsDefault();
    void _testDeleteCachedTilesKeepsTilesOfSets();
    void _testPruneCache();
    void _testPruneCacheKeepsTilesUsedLately();
    void _testMarkTileUsedWritesOnceADay();
    void _testUpdateTileDownloadState();
    void _testUpdateTileDownloadStatesIsAllOrNone();
    void _testExportImportReplace();
    void _testImportReplaceOfReadOnlyFileLeavesCacheWritable();
    void _testGetTileDownloadList();
    void _testGetTileDownloadListFailureIsNone();
    void _testImportSetsMerge();
    void _testComputeSetTotalsNonDefault();
    void _testSaveDuplicateTile();
    void _testOperationsAfterDisconnect();
    void _testDeleteTileSetWithMoreTilesThanSqlParameters();
    void _testImportSetsMergeDeduplicatesName();
    void _testExportAndMergeSetOfSeveralChunks();
    void _testImportMBTiles();
    void _testImportMBTilesIgnoresBoundsThatAreNoArea_data();
    void _testImportMBTilesIgnoresBoundsThatAreNoArea();
    void _testImportMBTilesReplacesTilesImportedBefore();
    void _testImportMBTilesRefusesFileWithoutPictures_data();
    void _testImportMBTilesRefusesFileWithoutPictures();
    void _testImportMBTilesInPlaceOfCache_data();
    void _testImportMBTilesInPlaceOfCache();
    void _testImportMBTilesOfSeveralChunksKeepsLogSmall();
    void _testImportMBTilesThatFailsLeavesNoSet();
    void _testImportMBTilesThatCannotBeReadLeavesNoSet();
    void _testImportSetsMergeThatFailsLeavesNoSet();
    void _testExportThatFailsLeavesNoFile();
    void _testSaveTileLinksToDifferentSet();
    void _testExportImportNoLingeringConnections();
    void _testCreateTileSet();
    void _testDeleteDefaultSetInvalidatesCache();
    void _testPruneCacheMultipleBatches();
    void _testSchemaVersionResetsLegacyDB();
    void _testMigrateVersion1RenumbersProviders();
    void _testMigrateVersion1KeepsPermanentIds();
    void _testMigrateFailureResetsCache_data();
    void _testMigrateFailureResetsCache();
    void _testResetFailureFailsInit();
    void _testDefaultSetLookupFailureRemovesCache();
    void _testImportVersion1_data();
    void _testImportVersion1();
    void _testImportRefusesVersion_data();
    void _testImportRefusesVersion();
    void _testImportMigrateFailureKeepsCache_data();
    void _testImportMigrateFailureKeepsCache();
    void _testImportReplaceThatCannotSwapKeepsCache_data();
    void _testImportReplaceThatCannotSwapKeepsCache();
    void _testImportReplaceOfMissingFileIntoMissingCacheSaysFileCannotBeOpened();
    void _testImportRefusesWorkingFileOfCache_data();
    void _testImportRefusesWorkingFileOfCache();
    void _testImportAndExportRefuseTheCacheItself();
    void _testImportSetsMergeOfNothingNewFails();
    void _testVersion2TilesGetCacheVariant_data();
    void _testVersion2TilesGetCacheVariant();
    void _testVersion1TilesGetCacheVariantOfPermanentId();
    void _testCreateTileSetKeysTilesByCacheVariant();
    void _testCreateTileSetOfMoreTilesThanALookup();
    void _testCreateTileSetAcrossAntimeridian();
    void _testFreePagesReleasedInBatches_data();
    void _testFreePagesReleasedInBatches();
    void _testFreePagesKeptWithoutAutoVacuum();
    void _testResetGivesSpaceBack();
    void _testResetGivesOldFileAutoVacuum();
    void _testNewFileHasSchema_data();
    void _testNewFileHasSchema();
    void _testInitDropsIndexThatAnotherCovers();

private:
    std::unique_ptr<TileCacheDatabase> _createInitializedDB(QTemporaryDir& tempDir);
    void _insertTileSet(TileCacheDatabase* db, const QString& name, quint64& outSetID);
    void _insertDownloadRecord(TileCacheDatabase* db, quint64 setID, const QString& hash,
                               DownloadTile::TileState state = DownloadTile::TileState::Pending);
    void _linkTileToSet(TileCacheDatabase* db, quint64 tileID, quint64 setID);
};
