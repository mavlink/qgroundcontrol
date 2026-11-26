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
    UT_PARAMETERIZED_TEST(_testExportImportReplace);
    UT_PARAMETERIZED_TEST(_testImportReplaceRefused);
    UT_PARAMETERIZED_TEST(_testImportReplaceFailsHalfway);
    void _testImportMergeRefusesNonTileSet();
    UT_PARAMETERIZED_TEST(_testImportFromPickedDirectory);
    UT_PARAMETERIZED_TEST(_testImportRefusesFileInUse);
    UT_PARAMETERIZED_TEST(_testImportReplaceKeepsLeftoverBackup);
    UT_PARAMETERIZED_TEST(_testInitPutsBackupBack);
    UT_PARAMETERIZED_TEST(_testImmutableFileUri);
    UT_PARAMETERIZED_TEST(_testImportFileNameAsUri);
    UT_PARAMETERIZED_TEST(_testImportReadOnlyFile);
    void _testReadOnlyCacheHeals();
    void _testGetTileDownloadList();
    void _testImportSetsMerge();
    void _testComputeSetTotalsNonDefault();
    void _testSaveDuplicateTile();
    void _testOperationsAfterDisconnect();
    void _testPruneCacheCleansSetTiles();
    void _testDeleteTileSetCleansTiles();
    void _testImportSetsMergeDeduplicatesName();
    void _testDeduplicatedNameKeepsMarkers();
    void _testGetTileDownloadListBatch();
    void _testSaveTileLinksToDifferentSet();
    void _testExportImportNoLingeringConnections();
    void _testCreateTileSet();
    void _testDeleteBingNoTileTiles();
    void _testDeleteDefaultSetInvalidatesCache();
    void _testPruneCacheMultipleBatches();
    void _testSchemaVersionSetOnFreshDB();
    void _testSchemaVersionResetsLegacyDB();
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
    std::unique_ptr<QGCTileCacheDatabase> _createInitializedDB(QTemporaryDir &tempDir);
    void _insertTileSet(QGCTileCacheDatabase* db, const QString& name, quint64& outSetID);
    void _insertDownloadRecord(QGCTileCacheDatabase* db, quint64 setID, const QString& hash, int state = 0);
    void _linkTileToSet(QGCTileCacheDatabase* db, quint64 tileID, quint64 setID);
    /// Export the default tile set of a new database that holds one tile, @a hash
    bool _writeTileSetFile(const QString& path, const QString& hash);
    /// The cache of @a db holds the tile @a keptHash and takes a new one
    void _verifyCacheUsable(QGCTileCacheDatabase* db, const QString& keptHash);
};
