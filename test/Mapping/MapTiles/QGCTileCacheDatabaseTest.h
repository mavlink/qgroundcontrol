#pragma once

#include <memory>

#include "UnitTest.h"

class QGCTileCacheDatabase;

class QGCTileCacheDatabaseTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitWithValidPath();
    void _testInitWithEmptyPath();
    void _testConnectDisconnect();
    void _testDefaultTileSetCreated();
    void _testSaveTileAndGetTile();
    void _testGetTileNotFound();
    void _testFindTile();
    void _testGetTileSetsMultiple();
    void _testRenameTileSet();
    void _testFindTileSetID();
    void _testDeleteTileSet();
    void _testResetDatabase();
    void _testComputeTotals();
    void _testComputeSetTotalsDefault();
    void _testPruneCache();
    void _testUpdateTileDownloadState();
    void _testExportImportReplace();
    void _testGetTileDownloadList();
    void _testImportSetsMerge();
    void _testComputeSetTotalsNonDefault();
    void _testSaveDuplicateTile();
    void _testOperationsAfterDisconnect();
    void _testPruneCacheCleansSetTiles();
    void _testDeleteTileSetCleansTiles();
    void _testImportSetsMergeDeduplicatesName();
    void _testGetTileDownloadListBatch();
    void _testSaveTileLinksToDifferentSet();
    void _testExportImportNoLingeringConnections();
    void _testCreateTileSet();
    void _testDeleteBingNoTileTiles();
    void _testDeleteDefaultSetInvalidatesCache();
    void _testPruneCacheMultipleBatches();
    void _testSchemaVersionSetOnFreshDB();
    void _testSchemaVersionResetsLegacyDB();
    void _testMigrateVersion1RenumbersProviders();
    void _testMigrateVersion1KeepsPermanentIds();
    void _testMigrateFailureResetsCache();
    void _testResetFailureFailsInit();
    void _testImportVersion1_data();
    void _testImportVersion1();
    void _testImportRefusesVersion_data();
    void _testImportRefusesVersion();
    void _testImportMigrateFailureKeepsCache_data();
    void _testImportMigrateFailureKeepsCache();
    void _testImportReplaceCopyFailureKeepsCache();
    void _testVersion2TilesGetCacheVariant_data();
    void _testVersion2TilesGetCacheVariant();
    void _testVersion1TilesGetCacheVariantOfPermanentId();
    void _testCreateTileSetKeysTilesByCacheVariant();
    void _testNewFileHasIncrementalAutoVacuum();
    void _testFreePagesReleasedInBatches_data();
    void _testFreePagesReleasedInBatches();
    void _testFreePagesKeptWithoutAutoVacuum();
    void _testResetGivesSpaceBack();
    void _testResetGivesOldFileAutoVacuum();
    void _testSaveTileTypeStoredAsInteger();
    void _testCreateTileSetTypeStoredAsInteger();
    void _testTablesExist();
    void _testTilesTableColumns();
    void _testTileSetsTableColumns();
    void _testSetTilesTableColumns();
    void _testTilesDownloadTableColumns();
    void _testIndexesExist();
    void _testForeignKeyCascadeDelete();

private:
    std::unique_ptr<QGCTileCacheDatabase> _createInitializedDB(QTemporaryDir& tempDir);
    void _insertTileSet(QGCTileCacheDatabase* db, const QString& name, quint64& outSetID);
    void _insertDownloadRecord(QGCTileCacheDatabase* db, quint64 setID, const QString& hash, int state = 0);
    void _linkTileToSet(QGCTileCacheDatabase* db, quint64 tileID, quint64 setID);
};
