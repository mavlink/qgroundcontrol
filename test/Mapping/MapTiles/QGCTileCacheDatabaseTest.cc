#include "QGCTileCacheDatabaseTest.h"

#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

#include "QGCCacheTile.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTile.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileSet.h"

static const QString kFixedProviderType = QStringLiteral("Bing Road");

namespace {

QString tileCacheHash(int mapId, int x, int y, int z)
{
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, z);
}

/// The rows of a query, the values of a row joined with '|'
QStringList tileCacheRows(const QString& path, const QString& sql)
{
    QStringList rows;
    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/true, QStringLiteral("TileCacheTestRows"));
    QSqlQuery query(connection.database());
    if (!query.exec(sql)) {
        return {query.lastError().text()};
    }
    while (query.next()) {
        QStringList values;
        for (int i = 0; i < query.record().count(); i++) {
            values.append(query.value(i).toString());
        }
        rows.append(values.join(QLatin1Char('|')));
    }
    return rows;
}

const QString kTileCacheTilesSql = QStringLiteral("SELECT tileID, hash, type, hex(tile) FROM Tiles ORDER BY tileID");
const QString kTileCacheSetsSql = QStringLiteral("SELECT setID, name, type FROM TileSets ORDER BY setID");
const QString kTileCacheSetTilesSql = QStringLiteral("SELECT setID, tileID FROM SetTiles ORDER BY setID, tileID");
const QString kTileCacheDownloadsSql =
    QStringLiteral("SELECT setID, hash, type, x, y, z, state FROM TilesDownload ORDER BY rowid");
const QString kTileCacheVersionSql = QStringLiteral("PRAGMA user_version");
const QString kTileCacheAutoVacuumSql = QStringLiteral("PRAGMA auto_vacuum");

/// Creates a cache file as schema version 1 wrote it, with the default set, and runs \a rowStatements on it.
/// Returns the error, if any.
QString createVersion1TileCache(const QString& path, const QStringList& rowStatements)
{
    QStringList statements = {
        QStringLiteral("CREATE TABLE Tiles (tileID INTEGER PRIMARY KEY NOT NULL, hash TEXT NOT NULL UNIQUE, "
                       "format TEXT NOT NULL, tile BLOB NULL, size INTEGER, type INTEGER, date INTEGER DEFAULT 0)"),
        QStringLiteral("CREATE TABLE TileSets (setID INTEGER PRIMARY KEY NOT NULL, name TEXT NOT NULL UNIQUE, "
                       "typeStr TEXT, topleftLat REAL DEFAULT 0.0, topleftLon REAL DEFAULT 0.0, "
                       "bottomRightLat REAL DEFAULT 0.0, bottomRightLon REAL DEFAULT 0.0, minZoom INTEGER DEFAULT 3, "
                       "maxZoom INTEGER DEFAULT 3, type INTEGER DEFAULT -1, numTiles INTEGER DEFAULT 0, "
                       "defaultSet INTEGER DEFAULT 0, date INTEGER DEFAULT 0)"),
        QStringLiteral("CREATE TABLE SetTiles (setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                       "tileID INTEGER NOT NULL REFERENCES Tiles(tileID) ON DELETE CASCADE)"),
        QStringLiteral("CREATE TABLE TilesDownload (setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE "
                       "CASCADE, hash TEXT NOT NULL, type INTEGER, x INTEGER, y INTEGER, z INTEGER, "
                       "state INTEGER DEFAULT 0)"),
        QStringLiteral("CREATE UNIQUE INDEX idx_settiles_unique ON SetTiles(tileID, setID)"),
        QStringLiteral("CREATE INDEX idx_settiles_setid ON SetTiles(setID)"),
        QStringLiteral("CREATE INDEX idx_settiles_tileid ON SetTiles(tileID)"),
        QStringLiteral("CREATE UNIQUE INDEX idx_tilesdownload_setid_hash ON TilesDownload(setID, hash)"),
        QStringLiteral("CREATE INDEX idx_tilesdownload_setid_state ON TilesDownload(setID, state)"),
        QStringLiteral("CREATE INDEX idx_tiles_date ON Tiles(date)"),
        QStringLiteral("INSERT INTO TileSets(setID, name, defaultSet) VALUES(1, 'Default Tile Set', 1)"),
        QStringLiteral("PRAGMA user_version = 1"),
    };
    statements.append(rowStatements);

    const QGCSqlHelper::ScopedConnection connection(path, /*readOnly=*/false, QStringLiteral("TileCacheTestCreate"));
    QSqlQuery query(connection.database());
    for (const QString& statement : std::as_const(statements)) {
        if (!query.exec(statement)) {
            return statement + QStringLiteral(": ") + query.lastError().text();
        }
    }
    return QString();
}

/// Tiles, sets and downloads of the providers 1, 6 and 36. Tiles 1 and 2 are the same x/y/z of the providers 1 and
/// 6, as are the two downloads of set 2: renumbered with shiftedLegacyMapIds(), the row of provider 1 has the key
/// the row of provider 6 had.
QStringList version1Rows()
{
    const QString tile = QStringLiteral(
        "INSERT INTO Tiles(tileID, hash, format, tile, size, type) "
        "VALUES(%1, '%2', 'png', X'%3', 1, %4)");
    const QString download = QStringLiteral(
        "INSERT INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(2, '%1', %2, 11, 21, 5, %3)");
    return {
        tile.arg(1).arg(tileCacheHash(1, 10, 20, 5), QStringLiteral("A1")).arg(1),
        tile.arg(2).arg(tileCacheHash(6, 10, 20, 5), QStringLiteral("B6")).arg(6),
        tile.arg(3).arg(tileCacheHash(36, 3, 4, 2), QStringLiteral("C3")).arg(36),
        QStringLiteral("INSERT INTO TileSets(setID, name, type) VALUES(2, 'Set of 1', 1)"),
        QStringLiteral("INSERT INTO TileSets(setID, name, type) VALUES(3, 'Set of 6', 6)"),
        QStringLiteral("INSERT INTO SetTiles(setID, tileID) VALUES(1, 1), (1, 2), (1, 3), (2, 1), (3, 2)"),
        download.arg(tileCacheHash(1, 11, 21, 5)).arg(1).arg(0),
        download.arg(tileCacheHash(6, 11, 21, 5)).arg(6).arg(2),
    };
}

/// The provider ids of a version-1 cache from a build whose provider list lacked the first 5 providers
QMap<int, int> shiftedLegacyMapIds()
{
    QMap<int, int> ids;
    for (int legacyId = 1; legacyId <= 36; legacyId++) {
        ids.insert(legacyId, legacyId + 5);
    }
    return ids;
}

qint64 tileCacheFreePages(const QString& path)
{
    return tileCacheRows(path, QStringLiteral("PRAGMA freelist_count")).value(0).toLongLong();
}

constexpr int kLargeTileCount = 20;
constexpr int kLargeTileSize = 64 * 1024;
constexpr qint64 kLargeTilesSize = static_cast<qint64>(kLargeTileCount) * kLargeTileSize;

/// Saves tiles that take hundreds of pages together, in the default set when \a setID is not a set
bool saveLargeTiles(QGCTileCacheDatabase& db, quint64 setID)
{
    const QByteArray image(kLargeTileSize, 'T');
    for (int i = 0; i < kLargeTileCount; i++) {
        if (!db.saveTile(QStringLiteral("large_%1").arg(i), QStringLiteral("png"), image, kFixedProviderType, setID)) {
            return false;
        }
    }
    return true;
}

/// The size of the file once all that was written is in the file itself, and not in its write-ahead log
qint64 checkpointedFileSize(const QGCTileCacheDatabase& db, const QString& path)
{
    QSqlQuery query(db.database());
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"))) {
        return -1;
    }
    query.finish();
    return QFileInfo(path).size();
}

/// The tiles of version1Rows() once renumbered with shiftedLegacyMapIds()
QStringList renumberedTileRows()
{
    return {
        QStringLiteral("1|%1|6|A1").arg(tileCacheHash(6, 10, 20, 5)),
        QStringLiteral("2|%1|11|B6").arg(tileCacheHash(11, 10, 20, 5)),
        QStringLiteral("3|%1|41|C3").arg(tileCacheHash(41, 3, 4, 2)),
    };
}

}  // namespace

std::unique_ptr<QGCTileCacheDatabase> QGCTileCacheDatabaseTest::_createInitializedDB(QTemporaryDir& tempDir)
{
    auto db = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("tiles.db"));
    if (!db->init() || !db->connectDB()) {
        return nullptr;
    }
    return db;
}

void QGCTileCacheDatabaseTest::_insertTileSet(QGCTileCacheDatabase* db, const QString& name, quint64& outSetID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT INTO TileSets(name, date) VALUES(?, ?)"));
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    QVERIFY(query.exec());
    outSetID = query.lastInsertId().toULongLong();
}

void QGCTileCacheDatabaseTest::_insertDownloadRecord(QGCTileCacheDatabase* db, quint64 setID, const QString& hash,
                                                     int state)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare(
        "INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(setID);
    query.addBindValue(hash);
    query.addBindValue(0);
    query.addBindValue(0);
    query.addBindValue(0);
    query.addBindValue(1);
    query.addBindValue(state);
    QVERIFY(query.exec());
}

void QGCTileCacheDatabaseTest::_testInitWithValidPath()
{
    QTemporaryDir tempDir;
    QGCTileCacheDatabase db(tempDir.filePath("tiles.db"));
    QVERIFY(!db.isValid());
    QVERIFY(!db.hasFailed());

    QVERIFY(db.init());
    QVERIFY(db.isValid());
    QVERIFY(!db.hasFailed());
}

void QGCTileCacheDatabaseTest::_testInitWithEmptyPath()
{
    // Expected: critical about missing cache directory
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtCriticalMsg,
                     QRegularExpression("Could not find suitable cache directory"));

    const QString emptyPath;
    QGCTileCacheDatabase db(emptyPath);
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    QVERIFY(db.hasFailed());
    QVERIFY(!db.isValid());
}

void QGCTileCacheDatabaseTest::_testConnectDisconnect()
{
    QTemporaryDir tempDir;
    QGCTileCacheDatabase db(tempDir.filePath("tiles.db"));
    QVERIFY(db.init());

    QVERIFY(db.connectDB());
    QVERIFY(db.isValid());

    db.disconnectDB();

    QVERIFY(db.connectDB());
    QVERIFY(db.isValid());

    db.disconnectDB();
}

void QGCTileCacheDatabaseTest::_testDefaultTileSetCreated()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
    QCOMPARE(sets[0].name, QStringLiteral("Default Tile Set"));
}

void QGCTileCacheDatabaseTest::_testSaveTileAndGetTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString hash = QStringLiteral("test_hash_1234");
    const QString format = QStringLiteral("png");
    const QByteArray img("fake_tile_data_bytes");
    QVERIFY(UrlFactory::getMapIdFromProviderType(kFixedProviderType) != -1);

    QVERIFY(db->saveTile(hash, format, img, kFixedProviderType, QGCTileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(hash);
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->hash, hash);
    QCOMPARE(tile->format, format);
    QCOMPARE(tile->img, img);
    QCOMPARE(tile->type, kFixedProviderType);
}

void QGCTileCacheDatabaseTest::_testGetTileNotFound()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    auto tile = db->getTile(QStringLiteral("nonexistent_hash"));
    QVERIFY(tile == nullptr);
}

void QGCTileCacheDatabaseTest::_testFindTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString hash = QStringLiteral("findme_hash");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const auto id = db->findTile(hash);
    QVERIFY(id.has_value());
    QVERIFY(id.value() != 0);

    const auto missing = db->findTile(QStringLiteral("no_such_hash"));
    QVERIFY(!missing.has_value());
}

void QGCTileCacheDatabaseTest::_testGetTileSetsMultiple()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 customSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("My Custom Set"), customSetID);
    QVERIFY(customSetID != 0);

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 2);

    bool foundDefault = false;
    bool foundCustom = false;
    for (const auto& rec : sets) {
        if (rec.defaultSet)
            foundDefault = true;
        if (rec.name == QStringLiteral("My Custom Set"))
            foundCustom = true;
    }
    QVERIFY(foundDefault);
    QVERIFY(foundCustom);
}

void QGCTileCacheDatabaseTest::_testRenameTileSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Original Name"), setID);

    QVERIFY(db->renameTileSet(setID, QStringLiteral("New Name")));

    const auto foundID = db->findTileSetID(QStringLiteral("New Name"));
    QVERIFY(foundID.has_value());
    QCOMPARE(foundID.value(), setID);
}

void QGCTileCacheDatabaseTest::_testFindTileSetID()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto foundID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(foundID.has_value());
    QVERIFY(foundID.value() != 0);

    const auto missingID = db->findTileSetID(QStringLiteral("No Such Set"));
    QVERIFY(!missingID.has_value());
}

void QGCTileCacheDatabaseTest::_testDeleteTileSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("ToDelete"), setID);
    QCOMPARE(db->getTileSets().size(), 2);

    QVERIFY(db->deleteTileSet(setID));

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
}

void QGCTileCacheDatabaseTest::_testResetDatabase()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    QVERIFY(db->saveTile(QStringLiteral("h1"), QStringLiteral("png"), QByteArray("d1"), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->findTile(QStringLiteral("h1")).has_value());

    QVERIFY(db->resetDatabase());
    QVERIFY(db->isValid());

    QVERIFY(!db->findTile(QStringLiteral("h1")).has_value());

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
}

void QGCTileCacheDatabaseTest::_testComputeTotals()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data10(10, 'A');
    const QByteArray data20(20, 'B');
    QVERIFY(db->saveTile(QStringLiteral("ct1"), QStringLiteral("png"), data10, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("ct2"), QStringLiteral("png"), data20, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const TotalsResult totals = db->computeTotals();
    QCOMPARE(totals.totalCount, static_cast<quint32>(2));
    QCOMPARE(totals.totalSize, static_cast<quint64>(30));
    QCOMPARE(totals.defaultCount, static_cast<quint32>(2));
    QCOMPARE(totals.defaultSize, static_cast<quint64>(30));
}

void QGCTileCacheDatabaseTest::_testComputeSetTotalsDefault()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(15, 'X');
    QVERIFY(db->saveTile(QStringLiteral("sd1"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    const SetTotalsResult result = db->computeSetTotals(defaultSetID.value(), true, 0, kFixedProviderType);
    QCOMPARE(result.savedTileCount, static_cast<quint32>(1));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(15));
}

void QGCTileCacheDatabaseTest::_testPruneCache()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(100, 'Z');
    QVERIFY(db->saveTile(QStringLiteral("pr1"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("pr2"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    QVERIFY(db->findTile(QStringLiteral("pr1")).has_value());

    QVERIFY(db->pruneCache(200));

    QVERIFY(!db->findTile(QStringLiteral("pr1")).has_value());
    QVERIFY(!db->findTile(QStringLiteral("pr2")).has_value());
}

void QGCTileCacheDatabaseTest::_testUpdateTileDownloadState()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_hash_1"), QGCTile::StatePending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_hash_2"), QGCTile::StatePending);

    QVERIFY(db->updateTileDownloadState(defaultSetID.value(), QGCTile::StateDownloading, QStringLiteral("dl_hash_1")));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT state FROM TilesDownload WHERE hash = ?")));
        query.addBindValue(QStringLiteral("dl_hash_1"));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), static_cast<int>(QGCTile::StateDownloading));
    }

    QVERIFY(db->updateTileDownloadState(defaultSetID.value(), QGCTile::StateComplete, QStringLiteral("dl_hash_1")));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT COUNT(*) FROM TilesDownload WHERE hash = ?")));
        query.addBindValue(QStringLiteral("dl_hash_1"));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }

    QVERIFY(db->updateAllTileDownloadStates(defaultSetID.value(), QGCTile::StateError));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT state FROM TilesDownload WHERE hash = ?")));
        query.addBindValue(QStringLiteral("dl_hash_2"));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), static_cast<int>(QGCTile::StateError));
    }
}

void QGCTileCacheDatabaseTest::_testExportImportReplace()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(50, 'E');
    QVERIFY(db->saveTile(QStringLiteral("exp1"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("exp2"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const auto sets = db->getTileSets();
    QVERIFY(!sets.isEmpty());

    TileSetRecord defaultRec = sets[0];
    defaultRec.numTiles = 2;

    const QString exportPath = tempDir.filePath("export.db");
    int lastProgress = 0;
    auto progressCb = [&lastProgress](int p) { lastProgress = p; };

    const DatabaseResult exportResult = db->exportSets({defaultRec}, exportPath, progressCb);
    QVERIFY(exportResult.success);
    QVERIFY(lastProgress > 0);
    QCOMPARE(tileCacheRows(exportPath, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});

    db.reset();

    auto db2 = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("tiles2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());

    QVERIFY(!db2->findTile(QStringLiteral("exp1")).has_value());

    const DatabaseResult importResult = db2->importSetsReplace(exportPath, nullptr);
    QVERIFY(importResult.success);
    QVERIFY(db2->isValid());

    QVERIFY(db2->findTile(QStringLiteral("exp1")).has_value());
    QVERIFY(db2->findTile(QStringLiteral("exp2")).has_value());
}

void QGCTileCacheDatabaseTest::_linkTileToSet(QGCTileCacheDatabase* db, quint64 tileID, quint64 setID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"));
    query.addBindValue(tileID);
    query.addBindValue(setID);
    QVERIFY(query.exec());
}

void QGCTileCacheDatabaseTest::_testGetTileDownloadList()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_a"), QGCTile::StatePending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_b"), QGCTile::StatePending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_c"), QGCTile::StatePending);

    const QList<QGCTile> tiles = db->getTileDownloadList(defaultSetID.value(), 2);
    QCOMPARE(tiles.size(), 2);

    QStringList hashes;
    for (const auto& t : tiles) {
        hashes.append(t.hash);
    }
    QVERIFY(hashes.contains(QStringLiteral("dl_a")));
    QVERIFY(hashes.contains(QStringLiteral("dl_b")));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("SELECT state FROM TilesDownload WHERE hash = 'dl_a'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), static_cast<int>(QGCTile::StateDownloading));
    }
    {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("SELECT state FROM TilesDownload WHERE hash = 'dl_b'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), static_cast<int>(QGCTile::StateDownloading));
    }
    {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("SELECT state FROM TilesDownload WHERE hash = 'dl_c'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), static_cast<int>(QGCTile::StatePending));
    }
}

void QGCTileCacheDatabaseTest::_testImportSetsMerge()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);

    const QByteArray data(40, 'M');
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m1"), QStringLiteral("png"), data, kFixedProviderType,
                            QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m2"), QStringLiteral("png"), data, kFixedProviderType,
                            QGCTileCacheDatabase::kInvalidTileSet));

    const auto srcSets = dbSrc->getTileSets();
    QVERIFY(!srcSets.isEmpty());
    TileSetRecord srcRec = srcSets[0];
    srcRec.numTiles = 2;

    const QString exportPath = tempDir.filePath("merge_export.db");
    const DatabaseResult exportResult = dbSrc->exportSets({srcRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("merge_target.db"));
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());

    QVERIFY(dbTgt->saveTile(QStringLiteral("trg_m1"), QStringLiteral("png"), QByteArray(25, 'N'), kFixedProviderType,
                            QGCTileCacheDatabase::kInvalidTileSet));

    int lastProgress = 0;
    auto progressCb = [&lastProgress](int p) { lastProgress = p; };

    const DatabaseResult importResult = dbTgt->importSetsMerge(exportPath, progressCb);
    QVERIFY(importResult.success);
    QVERIFY(lastProgress > 0);

    QVERIFY(dbTgt->findTile(QStringLiteral("trg_m1")).has_value());
    QVERIFY(dbTgt->findTile(QStringLiteral("exp_m1")).has_value());
    QVERIFY(dbTgt->findTile(QStringLiteral("exp_m2")).has_value());
}

void QGCTileCacheDatabaseTest::_testComputeSetTotalsNonDefault()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 customSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("Stats Set"), customSetID);
    QVERIFY(customSetID != 0);

    const QByteArray data20(20, 'A');
    const QByteArray data30(30, 'B');
    QVERIFY(db->saveTile(QStringLiteral("st1"), QStringLiteral("png"), data20, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("st2"), QStringLiteral("png"), data30, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const auto tile1 = db->findTile(QStringLiteral("st1"));
    const auto tile2 = db->findTile(QStringLiteral("st2"));
    QVERIFY(tile1.has_value());
    QVERIFY(tile2.has_value());

    _linkTileToSet(db.get(), tile1.value(), customSetID);
    _linkTileToSet(db.get(), tile2.value(), customSetID);

    // With a tile left to download, the set still counts the tiles it was created for
    _insertDownloadRecord(db.get(), customSetID, QStringLiteral("st3"), QGCTile::StateError);
    const SetTotalsResult result = db->computeSetTotals(customSetID, false, 5, kFixedProviderType);
    QCOMPARE(result.totalTileCount, static_cast<quint32>(5));
    QCOMPARE(result.savedTileCount, static_cast<quint32>(2));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(50));

    // With nothing left to download, it is complete with the tiles it has
    QVERIFY(db->updateTileDownloadState(customSetID, QGCTile::StateComplete, QStringLiteral("st3")));
    const SetTotalsResult doneResult = db->computeSetTotals(customSetID, false, 5, kFixedProviderType);
    QCOMPARE(doneResult.totalTileCount, static_cast<quint32>(2));
    QCOMPARE(doneResult.savedTileCount, static_cast<quint32>(2));
    QCOMPARE(doneResult.totalTileSize, static_cast<quint64>(50));
}

void QGCTileCacheDatabaseTest::_testSaveDuplicateTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data1(10, 'X');
    const QByteArray data2(20, 'Y');

    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("png"), data1, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    // The tile saved last takes the place of the one with its key: it was fetched because that one could not be used
    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("jpg"), data2, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(QStringLiteral("dup_hash"));
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->img, data2);
    QCOMPARE(tile->format, QStringLiteral("jpg"));
    const TotalsResult totals = db->computeTotals();
    QCOMPARE(totals.totalCount, static_cast<quint32>(1));
    QCOMPARE(totals.totalSize, static_cast<quint64>(data2.size()));
}

void QGCTileCacheDatabaseTest::_testOperationsAfterDisconnect()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    db->disconnectDB();

    const char* categoryPattern = "MapTiles.QGCTileCacheDatabase";
    const QRegularExpression disconnectedPattern(QStringLiteral("Database not connected"));

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(!db->saveTile(QStringLiteral("x"), QStringLiteral("png"), QByteArray("d"), kFixedProviderType,
                          QGCTileCacheDatabase::kInvalidTileSet));
    verifyExpectedLogMessage();

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(db->getTile(QStringLiteral("x")) == nullptr);
    verifyExpectedLogMessage();

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(!db->findTile(QStringLiteral("x")).has_value());
    verifyExpectedLogMessage();

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(db->getTileSets().isEmpty());
    verifyExpectedLogMessage();

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    const TotalsResult totals = db->computeTotals();
    verifyExpectedLogMessage();
    QCOMPARE(totals.totalCount, static_cast<quint32>(0));
    QCOMPARE(totals.totalSize, static_cast<quint64>(0));

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(!db->pruneCache(100));
    verifyExpectedLogMessage();

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(!db->resetDatabase());
    verifyExpectedLogMessage();
}

void QGCTileCacheDatabaseTest::_testPruneCacheCleansSetTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(100, 'P');
    QVERIFY(db->saveTile(QStringLiteral("ps1"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("ps2"), QStringLiteral("png"), data, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM SetTiles")));
        QVERIFY(query.next());
        QVERIFY(query.value(0).toInt() >= 2);
    }

    QVERIFY(db->pruneCache(200));

    {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM SetTiles")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }

    QVERIFY(!db->findTile(QStringLiteral("ps1")).has_value());
    QVERIFY(!db->findTile(QStringLiteral("ps2")).has_value());
}

void QGCTileCacheDatabaseTest::_testDeleteTileSetCleansTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("CleanupSet"), setID);
    QVERIFY(setID != 0);

    QVERIFY(db->saveTile(QStringLiteral("cleanup1"), QStringLiteral("png"), QByteArray(50, 'C'), kFixedProviderType,
                         setID));

    QVERIFY(db->findTile(QStringLiteral("cleanup1")).has_value());

    QVERIFY(db->deleteTileSet(setID));

    QVERIFY(!db->findTile(QStringLiteral("cleanup1")).has_value());

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = ?")));
        query.addBindValue(setID);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }
}

void QGCTileCacheDatabaseTest::_testImportSetsMergeDeduplicatesName()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);

    quint64 customSetID = 0;
    _insertTileSet(dbSrc.get(), QStringLiteral("SharedName"), customSetID);
    QVERIFY(customSetID != 0);

    QVERIFY(dbSrc->saveTile(QStringLiteral("dedup_tile1"), QStringLiteral("png"), QByteArray(30, 'D'),
                            kFixedProviderType, QGCTileCacheDatabase::kInvalidTileSet));
    const auto tileID = dbSrc->findTile(QStringLiteral("dedup_tile1"));
    QVERIFY(tileID.has_value());
    _linkTileToSet(dbSrc.get(), tileID.value(), customSetID);

    const auto srcSets = dbSrc->getTileSets();
    TileSetRecord customRec;
    for (const auto& rec : srcSets) {
        if (rec.name == QStringLiteral("SharedName")) {
            customRec = rec;
            customRec.numTiles = 1;
            break;
        }
    }
    QVERIFY(customRec.setID != 0);

    const QString exportPath = tempDir.filePath("dedup_export.db");
    const DatabaseResult exportResult = dbSrc->exportSets({customRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("dedup_target.db"));
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());

    quint64 existingSetID = 0;
    _insertTileSet(dbTgt.get(), QStringLiteral("SharedName"), existingSetID);
    QVERIFY(existingSetID != 0);

    const DatabaseResult importResult = dbTgt->importSetsMerge(exportPath, nullptr);
    QVERIFY(importResult.success);

    const auto targetSets = dbTgt->getTileSets();
    bool foundOriginal = false;
    bool foundDeduped = false;
    for (const auto& rec : targetSets) {
        if (rec.name == QStringLiteral("SharedName"))
            foundOriginal = true;
        if (rec.name == QStringLiteral("SharedName 0001"))
            foundDeduped = true;
    }
    QVERIFY(foundOriginal);
    QVERIFY(foundDeduped);
}

void QGCTileCacheDatabaseTest::_testGetTileDownloadListBatch()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    for (int i = 0; i < 10; i++) {
        _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("batch_dl_%1").arg(i),
                              QGCTile::StatePending);
    }

    const QList<QGCTile> tiles = db->getTileDownloadList(defaultSetID.value(), 5);
    QCOMPARE(tiles.size(), 5);

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT COUNT(*) FROM TilesDownload WHERE setID = ? AND state = ?")));
        query.addBindValue(defaultSetID.value());
        query.addBindValue(static_cast<int>(QGCTile::StateDownloading));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 5);
    }
    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT COUNT(*) FROM TilesDownload WHERE setID = ? AND state = ?")));
        query.addBindValue(defaultSetID.value());
        query.addBindValue(static_cast<int>(QGCTile::StatePending));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 5);
    }
}

void QGCTileCacheDatabaseTest::_testSaveTileLinksToDifferentSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setA = 0, setB = 0;
    _insertTileSet(db.get(), QStringLiteral("SetA"), setA);
    _insertTileSet(db.get(), QStringLiteral("SetB"), setB);

    const QByteArray data(10, 'A');
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, kFixedProviderType, setA));
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, kFixedProviderType, setB));

    auto tile = db->getTile(QStringLiteral("shared_hash"));
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->img, data);

    const auto tileID = db->findTile(QStringLiteral("shared_hash"));
    QVERIFY(tileID.has_value());

    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare(QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE tileID = ?")));
        query.addBindValue(tileID.value());
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
    }
}

void QGCTileCacheDatabaseTest::_testExportImportNoLingeringConnections()
{
    QTemporaryDir tempDir;
    auto db1 = _createInitializedDB(tempDir);

    const QByteArray data(20, 'L');
    QVERIFY(db1->saveTile(QStringLiteral("linger1"), QStringLiteral("png"), data, kFixedProviderType,
                          QGCTileCacheDatabase::kInvalidTileSet));

    const auto sets = db1->getTileSets();
    QVERIFY(!sets.isEmpty());
    TileSetRecord rec = sets[0];
    rec.numTiles = 1;

    const QString exportPath = tempDir.filePath("linger_export.db");

    const DatabaseResult exp1 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp1.success);

    const DatabaseResult exp2 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp2.success);

    auto db2 = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("linger_db2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());
    const DatabaseResult imp1 = db2->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp1.success);

    auto db3 = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("linger_db3.db"));
    QVERIFY(db3->init());
    QVERIFY(db3->connectDB());
    const DatabaseResult imp2 = db3->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp2.success);
}

void QGCTileCacheDatabaseTest::_testCreateTileSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    QVERIFY(UrlFactory::getMapIdFromProviderType(kFixedProviderType) != -1);

    const auto setID = db->createTileSet(QStringLiteral("Test Set"), QStringLiteral("TestMap"), 37.0, -122.0, 36.0,
                                         -121.0, 5, 5, kFixedProviderType, 100, QString());

    QVERIFY(setID.has_value());
    QVERIFY(setID.value() != 0);

    const auto foundID = db->findTileSetID(QStringLiteral("Test Set"));
    QVERIFY(foundID.has_value());
    QCOMPARE(foundID.value(), setID.value());

    const auto sets = db->getTileSets();
    bool found = false;
    for (const auto& rec : sets) {
        if (rec.setID == setID.value()) {
            found = true;
            QCOMPARE(rec.name, QStringLiteral("Test Set"));
            QCOMPARE(rec.minZoom, 5);
            QCOMPARE(rec.maxZoom, 5);
            break;
        }
    }
    QVERIFY(found);
}

void QGCTileCacheDatabaseTest::_testDeleteBingNoTileTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    if (!file.open(QFile::ReadOnly)) {
        QSKIP("BingNoTileBytes.dat resource not available");
    }
    const QByteArray noTileBytes = file.readAll();
    file.close();
    QVERIFY(!noTileBytes.isEmpty());

    QVERIFY(db->saveTile(QStringLiteral("bing_notile_1"), QStringLiteral("png"), noTileBytes, kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("normal_tile"), QStringLiteral("png"), QByteArray(50, 'N'), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    QVERIFY(db->findTile(QStringLiteral("bing_notile_1")).has_value());
    QVERIFY(db->findTile(QStringLiteral("normal_tile")).has_value());

    QSettings settings;
    settings.remove(QStringLiteral("_deleteBingNoTileTilesDone"));

    db->deleteBingNoTileTiles();

    QVERIFY(!db->findTile(QStringLiteral("bing_notile_1")).has_value());
    QVERIFY(db->findTile(QStringLiteral("normal_tile")).has_value());
}

void QGCTileCacheDatabaseTest::_testDeleteDefaultSetInvalidatesCache()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultID.has_value());

    QVERIFY(db->deleteTileSet(defaultID.value()));

    // After deleting default set, saveTile with kInvalidTileSet should fail gracefully
    // (no default set to link to), not crash or use stale ID
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Default tile set not found in database")));
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Cannot save tile: no valid tile set")));
    QVERIFY(!db->saveTile(QStringLiteral("orphan"), QStringLiteral("png"), QByteArray(10, 'O'), kFixedProviderType,
                          QGCTileCacheDatabase::kInvalidTileSet));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void QGCTileCacheDatabaseTest::_testPruneCacheMultipleBatches()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    // Save 200 tiles of 100 bytes each = 20000 bytes total
    for (int i = 0; i < 200; i++) {
        QVERIFY(db->saveTile(QStringLiteral("prune_multi_%1").arg(i), QStringLiteral("png"), QByteArray(100, 'P'),
                             kFixedProviderType, QGCTileCacheDatabase::kInvalidTileSet));
    }

    const TotalsResult before = db->computeTotals();
    QCOMPARE(before.totalCount, static_cast<quint32>(200));

    // Prune 15000 bytes — requires more than one batch of 128
    QVERIFY(db->pruneCache(15000));

    const TotalsResult after = db->computeTotals();
    QVERIFY(after.totalCount < 200);
    QVERIFY(after.totalSize <= 5000);
}

void QGCTileCacheDatabaseTest::_testSchemaVersionSetOnFreshDB()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA user_version"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), QGCTileCacheDatabase::kSchemaVersion);
}

void QGCTileCacheDatabaseTest::_testSchemaVersionResetsLegacyDB()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("legacy.db");

    // Create a legacy database (no user_version set, but has tile data)
    {
        QSqlDatabase legacy = QSqlDatabase::addDatabase("QSQLITE", "legacy_setup");
        legacy.setDatabaseName(path);
        QVERIFY(legacy.open());
        QSqlQuery q(legacy);
        QVERIFY(
            q.exec("CREATE TABLE Tiles (tileID INTEGER PRIMARY KEY, hash TEXT UNIQUE, format TEXT, tile BLOB, size "
                   "INTEGER, type INTEGER, date INTEGER)"));
        QVERIFY(
            q.exec("CREATE TABLE TileSets (setID INTEGER PRIMARY KEY, name TEXT UNIQUE, typeStr TEXT, topleftLat REAL, "
                   "topleftLon REAL, bottomRightLat REAL, bottomRightLon REAL, minZoom INTEGER, maxZoom INTEGER, type "
                   "INTEGER, numTiles INTEGER, defaultSet INTEGER, date INTEGER)"));
        QVERIFY(q.exec("CREATE TABLE SetTiles (setID INTEGER, tileID INTEGER)"));
        QVERIFY(
            q.exec("CREATE TABLE TilesDownload (setID INTEGER, hash TEXT, type INTEGER, x INTEGER, y INTEGER, z "
                   "INTEGER, state INTEGER)"));
        QVERIFY(q.exec("INSERT INTO TileSets(name, defaultSet, date) VALUES('Default Tile Set', 1, 0)"));
        QVERIFY(q.exec(
            "INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES('old_hash', 'png', X'AA', 1, 0, 0)"));
        QVERIFY(q.exec("INSERT INTO SetTiles(setID, tileID) VALUES(1, 1)"));
        legacy.close();
    }
    QSqlDatabase::removeDatabase("legacy_setup");

    // Open with the versioned code — should detect legacy and reset
    QGCTileCacheDatabase db(path);
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "Legacy database detected \\(no schema version\\)\\. Discarding cached tiles and "
                         "rebuilding\\.")));
    QVERIFY(db.init());
    verifyExpectedLogMessage();
    QVERIFY(db.connectDB());

    // Legacy tile data should be gone
    QVERIFY(!db.findTile(QStringLiteral("old_hash")).has_value());

    // Schema version should now be set
    QSqlQuery query(db.database());
    QVERIFY(query.exec("PRAGMA user_version"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), QGCTileCacheDatabase::kSchemaVersion);

    // Default tile set should be recreated
    const auto sets = db.getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
}

void QGCTileCacheDatabaseTest::_testMigrateVersion1RenumbersProviders()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());
    const QStringList setTiles = tileCacheRows(path, kTileCacheSetTilesSql);
    QCOMPARE(setTiles.size(), 5);

    QGCTileCacheDatabase db(path);
    db.setLegacyMapIds(shiftedLegacyMapIds());
    QVERIFY(db.init());

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), renumberedTileRows());
    QCOMPARE(tileCacheRows(path, kTileCacheSetsSql),
             (QStringList{QStringLiteral("1|Default Tile Set|-1"), QStringLiteral("2|Set of 1|6"),
                          QStringLiteral("3|Set of 6|11")}));
    QCOMPARE(tileCacheRows(path, kTileCacheSetTilesSql), setTiles);
    QCOMPARE(tileCacheRows(path, kTileCacheDownloadsSql),
             (QStringList{QStringLiteral("2|%1|6|11|21|5|0").arg(tileCacheHash(6, 11, 21, 5)),
                          QStringLiteral("2|%1|11|11|21|5|2").arg(tileCacheHash(11, 11, 21, 5))}));
}

void QGCTileCacheDatabaseTest::_testMigrateVersion1KeepsPermanentIds()
{
    // Where version 1 had the permanent ids already, as it did on this platform, a cache is left as it is
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());
    const QStringList queries = {kTileCacheTilesSql, kTileCacheSetsSql, kTileCacheSetTilesSql, kTileCacheDownloadsSql};
    QList<QStringList> rows;
    for (const QString& sql : queries) {
        rows.append(tileCacheRows(path, sql));
        QVERIFY(!rows.last().isEmpty());
    }

    QGCTileCacheDatabase db(path);
    QVERIFY(db.init());

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});
    for (qsizetype i = 0; i < queries.size(); i++) {
        QCOMPARE(tileCacheRows(path, queries[i]), rows[i]);
    }
}

void QGCTileCacheDatabaseTest::_testMigrateFailureResetsCache()
{
    // Renumbered, the tile of provider 1 has the key of a tile that stays where it is
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    QGCTileCacheDatabase db(path);
    db.setLegacyMapIds({{1, 6}});
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to renumber provider 1 to 6")));
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to migrate schema version 1 .* Resetting cache")));
    QVERIFY(db.init());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), QStringList());
    QCOMPARE(tileCacheRows(path, kTileCacheSetsSql), QStringList{QStringLiteral("1|Default Tile Set|-1")});
}

void QGCTileCacheDatabaseTest::_testResetFailureFailsInit()
{
    // A cache of an unknown version is reset. Here its tiles cannot be dropped: a table that is not of the cache
    // refers to one of them.
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("newer.db");
    const QString newerVersion = QString::number(QGCTileCacheDatabase::kSchemaVersion + 1);
    const QStringList rows =
        version1Rows() +
        QStringList{
            QStringLiteral("CREATE TABLE TileNotes (tileID INTEGER NOT NULL REFERENCES Tiles(tileID))"),
            QStringLiteral("INSERT INTO TileNotes(tileID) VALUES(1)"),
            QStringLiteral("PRAGMA user_version = %1").arg(newerVersion),
        };
    QCOMPARE(createVersion1TileCache(path, rows), QString());

    QGCTileCacheDatabase db(path);
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unknown schema version")));
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to drop tables")));
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // The tiles that are left are not marked as tiles of the current version
    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql), QStringList{newerVersion});
}

void QGCTileCacheDatabaseTest::_testImportVersion1_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<bool>("afterDeadImport");
    QTest::newRow("replace") << true << false;
    QTest::newRow("merge") << false << false;
    QTest::newRow("merge after an import that died") << false << true;
}

void QGCTileCacheDatabaseTest::_testImportVersion1()
{
    QFETCH(bool, replace);
    QFETCH(bool, afterDeadImport);

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(importPath, version1Rows()), QString());
    const QStringList importedTiles = tileCacheRows(importPath, kTileCacheTilesSql);

    const QString path = tempDir.filePath("tiles.db");
    QGCTileCacheDatabase db(path);
    db.setLegacyMapIds(shiftedLegacyMapIds());
    QVERIFY(db.init());
    QVERIFY(db.connectDB());

    if (afterDeadImport) {
        // A merge that dies with its copy of a file open leaves the write-ahead log of that copy, next to where the
        // next merge puts its copy. SQLite applies a log to whatever file it finds it next to: here the log has a
        // tile of another file.
        const QString otherPath = tempDir.filePath("other.db");
        QCOMPARE(createVersion1TileCache(otherPath, {}), QString());
        const QGCSqlHelper::ScopedConnection other(otherPath, /*readOnly=*/false, QStringLiteral("TileCacheTestOther"));
        QSqlQuery query(other.database());
        QVERIFY2(query.exec(QStringLiteral("INSERT INTO Tiles(tileID, hash, format, tile, size, type) "
                                           "VALUES(1, '%1', 'png', X'DD', 1, 1)")
                                .arg(tileCacheHash(1, 7, 8, 9))),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("INSERT INTO SetTiles(setID, tileID) VALUES(1, 1)")),
                 qPrintable(query.lastError().text()));
        // Before the connection closes, which removes the log
        for (const QString& suffix : {QStringLiteral("-wal"), QStringLiteral("-shm")}) {
            QVERIFY(QFile::copy(otherPath + suffix, path + QStringLiteral(".import") + suffix));
        }
    }

    const DatabaseResult result =
        replace ? db.importSetsReplace(importPath, nullptr) : db.importSetsMerge(importPath, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));
    db.disconnectDB();

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), renumberedTileRows());
    QCOMPARE(tileCacheRows(path, QStringLiteral("SELECT S.name, S.type, T.hash FROM TileSets S JOIN SetTiles L ON "
                                                "L.setID = S.setID JOIN Tiles T ON T.tileID = L.tileID WHERE "
                                                "S.defaultSet = 0 ORDER BY S.name")),
             (QStringList{QStringLiteral("Set of 1|6|%1").arg(tileCacheHash(6, 10, 20, 5)),
                          QStringLiteral("Set of 6|11|%1").arg(tileCacheHash(11, 10, 20, 5))}));

    // The file to import is not migrated itself, and nothing is left of the copy that was
    QCOMPARE(tileCacheRows(importPath, kTileCacheVersionSql), QStringList{QStringLiteral("1")});
    QCOMPARE(tileCacheRows(importPath, kTileCacheTilesSql), importedTiles);
    QCOMPARE(QDir(tempDir.path()).entryList({QStringLiteral("*.import*")}), QStringList());
}

void QGCTileCacheDatabaseTest::_testImportRefusesVersion_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<int>("version");
    QTest::addColumn<QString>("error");
    const int newerVersion = QGCTileCacheDatabase::kSchemaVersion + 1;
    QTest::newRow("merge, version 0") << false << 0 << QStringLiteral("too old");
    QTest::newRow("merge, newer version") << false << newerVersion << QStringLiteral("newer version");
    QTest::newRow("replace, version 0") << true << 0 << QStringLiteral("too old");
    QTest::newRow("replace, newer version") << true << newerVersion << QStringLiteral("newer version");
}

void QGCTileCacheDatabaseTest::_testImportRefusesVersion()
{
    QFETCH(bool, replace);
    QFETCH(int, version);
    QFETCH(QString, error);

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("other.db");
    const QString versionSql = QStringLiteral("PRAGMA user_version = %1").arg(version);
    QCOMPARE(createVersion1TileCache(importPath, version1Rows() + QStringList{versionSql}), QString());

    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);
    QVERIFY(db->saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const DatabaseResult result =
        replace ? db->importSetsReplace(importPath, nullptr) : db->importSetsMerge(importPath, nullptr);
    QVERIFY(!result.success);
    QVERIFY2(result.errorString.contains(error), qPrintable(result.errorString));
    // The cache has the tile it had, and no tile of the file
    QVERIFY(db->findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db->computeTotals().totalCount, 1U);
    // Refused before the file is copied to be migrated
    QCOMPARE(QDir(tempDir.path()).entryList({QStringLiteral("*.import*")}), QStringList());
}

void QGCTileCacheDatabaseTest::_testImportMigrateFailureKeepsCache_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<QString>("error");
    QTest::newRow("replace") << true << QStringLiteral("Failed to initialize tile cache database after import");
    QTest::newRow("merge") << false << QStringLiteral("Failed to migrate import database");
}

void QGCTileCacheDatabaseTest::_testImportMigrateFailureKeepsCache()
{
    QFETCH(bool, replace);
    QFETCH(QString, error);

    // Renumbered, the tile of provider 1 of the file has the key of a tile that stays where it is
    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(importPath, version1Rows()), QString());

    QGCTileCacheDatabase db(tempDir.filePath("tiles.db"));
    db.setLegacyMapIds({{1, 6}});
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(db.saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kFixedProviderType,
                        QGCTileCacheDatabase::kInvalidTileSet));

    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to renumber provider 1 to 6")));
    expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to migrate schema version 1 to")));
    const DatabaseResult result =
        replace ? db.importSetsReplace(importPath, nullptr) : db.importSetsMerge(importPath, nullptr);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, error);

    // The cache is open and has the tile it had, and no tile of the file
    QVERIFY(db.findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db.computeTotals().totalCount, 1U);
    // Neither the copy that was to be migrated nor the backup of the cache is left
    QCOMPARE(QDir(tempDir.path()).entryList({QStringLiteral("*.import*"), QStringLiteral("*.bak")}), QStringList());
}

void QGCTileCacheDatabaseTest::_testVersion2TilesGetCacheVariant_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("cache opened") << QStringLiteral("open");
    QTest::newRow("import replacing") << QStringLiteral("replace");
    QTest::newRow("import merging") << QStringLiteral("merge");
}

void QGCTileCacheDatabaseTest::_testVersion2TilesGetCacheVariant()
{
    QFETCH(QString, how);

    // Tiles and downloads of the providers 39 and 26, which have a cache variant, and of provider 8, which has none.
    // Version 2 has the tables of version 1.
    const QString tile = QStringLiteral(
        "INSERT INTO Tiles(tileID, hash, format, tile, size, type) VALUES(%1, '%2', 'png', X'%3', 1, %4)");
    const QString download =
        QStringLiteral("INSERT INTO TilesDownload(setID, hash, type, x, y, z) VALUES(%1, '%2', %3, 11, 21, 5)");
    const QStringList rows = {
        tile.arg(1).arg(tileCacheHash(39, 10, 20, 5), QStringLiteral("A1")).arg(39),
        tile.arg(2).arg(tileCacheHash(26, 10, 20, 5), QStringLiteral("B2")).arg(26),
        tile.arg(3).arg(tileCacheHash(8, 10, 20, 5), QStringLiteral("C3")).arg(8),
        QStringLiteral("INSERT INTO TileSets(setID, name, type) VALUES(2, 'Set of 39', 39), (3, 'Set of 8', 8)"),
        QStringLiteral("INSERT INTO SetTiles(setID, tileID) VALUES(1, 1), (1, 2), (1, 3), (2, 1), (3, 3)"),
        download.arg(2).arg(tileCacheHash(39, 11, 21, 5)).arg(39),
        download.arg(3).arg(tileCacheHash(8, 11, 21, 5)).arg(8),
        QStringLiteral("PRAGMA user_version = 2"),
    };
    // The suffixes are the first 8 hex digits of the SHA-1 of the variants. An empty variant has none.
    const QHash<int, QString> cacheVariants = {{39, QStringLiteral("https://a.example/{z}/{x}/{y}.png")},
                                               {26, QStringLiteral("alice/streets")},
                                               {8, QString()}};
    const QStringList tiles = {
        QStringLiteral("1|%1-e16c3bed|39|A1").arg(tileCacheHash(39, 10, 20, 5)),
        QStringLiteral("2|%1-52f3eade|26|B2").arg(tileCacheHash(26, 10, 20, 5)),
        QStringLiteral("3|%1|8|C3").arg(tileCacheHash(8, 10, 20, 5)),
    };
    const QStringList downloads = {
        QStringLiteral("2|%1-e16c3bed|39|11|21|5|0").arg(tileCacheHash(39, 11, 21, 5)),
        QStringLiteral("3|%1|8|11|21|5|0").arg(tileCacheHash(8, 11, 21, 5)),
    };

    QTemporaryDir tempDir;
    const QString version2Path = tempDir.filePath("version2.db");
    QCOMPARE(createVersion1TileCache(version2Path, rows), QString());
    const QStringList setTiles = tileCacheRows(version2Path, kTileCacheSetTilesSql);

    const QString path = (how == QStringLiteral("open")) ? version2Path : tempDir.filePath("tiles.db");
    QGCTileCacheDatabase db(path);
    if (how == QStringLiteral("open")) {
        db.setCacheVariants(cacheVariants);
        QVERIFY(db.init());
    } else {
        // The variants of an import are those set by the time of the import
        QVERIFY(db.init());
        QVERIFY(db.connectDB());
        db.setCacheVariants(cacheVariants);
        const DatabaseResult result = (how == QStringLiteral("replace")) ? db.importSetsReplace(version2Path, nullptr)
                                                                         : db.importSetsMerge(version2Path, nullptr);
        QVERIFY2(result.success, qPrintable(result.errorString));
        db.disconnectDB();
    }

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(QGCTileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), tiles);
    if (how != QStringLiteral("merge")) {
        // A merge takes the tiles of the sets, and leaves what the sets still had to download
        QCOMPARE(tileCacheRows(path, kTileCacheSetTilesSql), setTiles);
        QCOMPARE(tileCacheRows(path, kTileCacheDownloadsSql), downloads);
    }
}

void QGCTileCacheDatabaseTest::_testVersion1TilesGetCacheVariantOfPermanentId()
{
    // The cache variants are by permanent provider id: tile 3 gets that of 41, which it is renumbered to from 36
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    QGCTileCacheDatabase db(path);
    db.setLegacyMapIds(shiftedLegacyMapIds());
    db.setCacheVariants({{36, QStringLiteral("alice/night")}, {41, QStringLiteral("alice/streets")}});
    QVERIFY(db.init());

    QStringList tiles = renumberedTileRows();
    tiles[2] = QStringLiteral("3|%1-52f3eade|41|C3").arg(tileCacheHash(41, 3, 4, 2));
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), tiles);
}

void QGCTileCacheDatabaseTest::_testCreateTileSetKeysTilesByCacheVariant()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString type = QStringLiteral("CustomURL Custom");
    const QString sourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
    const QString sourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

    // A region of one tile, which the cache has from source A
    const QGCTileSet region = UrlFactory::getTileCount(5, -122.0, 37.0, -121.0, 36.0, type);
    QCOMPARE(region.tileCount, 1U);
    const QString hashA = UrlFactory::getTileHash(type, region.tileX0, region.tileY0, 5, sourceA);
    const QString hashB = UrlFactory::getTileHash(type, region.tileX0, region.tileY0, 5, sourceB);
    QVERIFY(
        db->saveTile(hashA, QStringLiteral("png"), QByteArray("tile"), type, QGCTileCacheDatabase::kInvalidTileSet));

    const auto tilesOfSet = [&db](quint64 setID) {
        return db->computeSetTotals(setID, false, 1, QStringLiteral("CustomURL Custom")).savedTileCount;
    };

    // A set of source B has to download the tile, under the key of source B
    const auto setB =
        db->createTileSet(QStringLiteral("Set B"), type, 37.0, -122.0, 36.0, -121.0, 5, 5, type, 1, sourceB);
    QVERIFY(setB.has_value());
    QCOMPARE(tilesOfSet(*setB), 0U);
    const QList<QGCTile> downloadsB = db->getTileDownloadList(*setB, 10);
    QCOMPARE(downloadsB.size(), 1);
    QCOMPARE(downloadsB.first().hash, hashB);

    // A set of source A has the tile already
    const auto setA =
        db->createTileSet(QStringLiteral("Set A"), type, 37.0, -122.0, 36.0, -121.0, 5, 5, type, 1, sourceA);
    QVERIFY(setA.has_value());
    QCOMPARE(tilesOfSet(*setA), 1U);
    QVERIFY(db->getTileDownloadList(*setA, 10).isEmpty());
}

void QGCTileCacheDatabaseTest::_testNewFileHasIncrementalAutoVacuum()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);
    db.reset();

    QCOMPARE(tileCacheRows(tempDir.filePath("tiles.db"), kTileCacheAutoVacuumSql), QStringList{QStringLiteral("2")});
}

void QGCTileCacheDatabaseTest::_testFreePagesReleasedInBatches_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("tile set deleted") << QStringLiteral("delete set");
    QTest::newRow("cache pruned") << QStringLiteral("prune");
    QTest::newRow("cache of unknown version reset") << QStringLiteral("unknown version");
}

void QGCTileCacheDatabaseTest::_testFreePagesReleasedInBatches()
{
    QFETCH(QString, how);

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    auto db = std::make_unique<QGCTileCacheDatabase>(path);
    QVERIFY(db->init());
    QVERIFY(db->connectDB());

    // A tile set deleted takes its tiles with it. The default set is pruned of tiles no other set has.
    quint64 setID = QGCTileCacheDatabase::kInvalidTileSet;
    if (how == QStringLiteral("delete set")) {
        _insertTileSet(db.get(), QStringLiteral("Large Set"), setID);
    }
    QVERIFY(saveLargeTiles(*db, setID));
    const qint64 filledSize = checkpointedFileSize(*db, path);
    QVERIFY(filledSize > kLargeTilesSize);

    if (how == QStringLiteral("delete set")) {
        QVERIFY(db->deleteTileSet(setID));
    } else if (how == QStringLiteral("prune")) {
        QVERIFY(db->pruneCache(kLargeTilesSize));
    } else {
        {
            QSqlQuery query(db->database());
            QVERIFY(
                query.exec(QStringLiteral("PRAGMA user_version = %1").arg(QGCTileCacheDatabase::kSchemaVersion + 1)));
        }
        db = std::make_unique<QGCTileCacheDatabase>(path);
        expectLogMessage("MapTiles.QGCTileCacheDatabase", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Unknown schema version")));
        QVERIFY(db->init());
        verifyExpectedLogMessage();
        QVERIFY(db->connectDB());
    }
    QCOMPARE(db->computeTotals().totalCount, 0U);

    // What deleted the tiles left their pages in the file: they are released a batch at a time
    constexpr int batchPages = 100;
    qint64 freePages = tileCacheFreePages(path);
    QVERIFY2(freePages > 2 * batchPages, qPrintable(QString::number(freePages)));
    while (freePages > 0) {
        const qint64 pagesLeft = db->releaseFreePages(batchPages);
        QCOMPARE(pagesLeft, tileCacheFreePages(path));
        QVERIFY2((pagesLeft < freePages) && (pagesLeft >= freePages - batchPages),
                 qPrintable(QStringLiteral("%1 pages left of %2").arg(pagesLeft).arg(freePages)));
        freePages = pagesLeft;
    }

    // Without a checkpoint here: the user gets the space back when the last page is released, not when the cache is
    // closed
    const qint64 releasedSize = QFileInfo(path).size();
    QVERIFY2(releasedSize < filledSize - kLargeTilesSize,
             qPrintable(QStringLiteral("%1 bytes of %2 left").arg(releasedSize).arg(filledSize)));
}

void QGCTileCacheDatabaseTest::_testFreePagesKeptWithoutAutoVacuum()
{
    // A file from before auto-vacuum cannot give pages back. Its unused pages are for new rows, and there is
    // nothing left to do about them.
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    QGCTileCacheDatabase db(path);
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    quint64 setID = 0;
    _insertTileSet(&db, QStringLiteral("Large Set"), setID);
    QVERIFY(saveLargeTiles(db, setID));
    QVERIFY(db.deleteTileSet(setID));
    const qint64 freePages = tileCacheFreePages(path);
    QVERIFY(freePages > 0);

    QCOMPARE(db.releaseFreePages(100), 0);
    QCOMPARE(tileCacheFreePages(path), freePages);
}

void QGCTileCacheDatabaseTest::_testResetGivesSpaceBack()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    QGCTileCacheDatabase db(path);
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(saveLargeTiles(db, QGCTileCacheDatabase::kInvalidTileSet));
    const qint64 filledSize = checkpointedFileSize(db, path);

    QVERIFY(db.resetDatabase());

    QCOMPARE(tileCacheFreePages(path), 0);
    // Without a checkpoint here: the user gets the space back with the reset, not when the cache is closed
    const qint64 resetSize = QFileInfo(path).size();
    QVERIFY2(resetSize < filledSize - kLargeTilesSize,
             qPrintable(QStringLiteral("%1 bytes of %2 left").arg(resetSize).arg(filledSize)));
}

void QGCTileCacheDatabaseTest::_testResetGivesOldFileAutoVacuum()
{
    // A file from before auto-vacuum: the mode cannot be set once a file has tables
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("0")});

    // Opening and migrating the file does not rebuild it, which a large cache would take long for
    QGCTileCacheDatabase db(path);
    QVERIFY(db.init());
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("0")});

    QVERIFY(db.connectDB());
    QVERIFY(db.resetDatabase());
    db.disconnectDB();
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("2")});
}

void QGCTileCacheDatabaseTest::_testSaveTileTypeStoredAsInteger()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const int expectedMapId = UrlFactory::getMapIdFromProviderType(kFixedProviderType);
    QVERIFY(expectedMapId != -1);

    const QString hash = QStringLiteral("type_int_test");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    // Verify the raw DB stores the type as an integer mapId, not a string
    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare("SELECT type FROM Tiles WHERE hash = ?"));
        query.addBindValue(hash);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), expectedMapId);
    }

    // Verify getTile converts the integer back to the provider name string
    auto tile = db->getTile(hash);
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->type, kFixedProviderType);
}

void QGCTileCacheDatabaseTest::_testCreateTileSetTypeStoredAsInteger()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const int expectedMapId = UrlFactory::getMapIdFromProviderType(kFixedProviderType);
    QVERIFY(expectedMapId != -1);

    const auto setID = db->createTileSet(QStringLiteral("TypeInt Set"), QStringLiteral("TestMap"), 37.0, -122.0, 36.0,
                                         -121.0, 5, 5, kFixedProviderType, 100, QString());
    QVERIFY(setID.has_value());

    // Verify TileSets.type stores the integer mapId
    const auto sets = db->getTileSets();
    bool found = false;
    for (const auto& rec : sets) {
        if (rec.setID == setID.value()) {
            found = true;
            QCOMPARE(rec.type, expectedMapId);
            break;
        }
    }
    QVERIFY(found);
}

void QGCTileCacheDatabaseTest::_testTablesExist()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"));

    QStringList tables;
    while (query.next()) {
        tables << query.value(0).toString();
    }

    const QStringList expected = {
        QStringLiteral("SetTiles"),
        QStringLiteral("Tiles"),
        QStringLiteral("TilesDownload"),
        QStringLiteral("TileSets"),
    };
    QCOMPARE(tables.size(), expected.size());
    for (const auto& name : expected) {
        QVERIFY2(tables.contains(name), qPrintable(QStringLiteral("Missing table: %1").arg(name)));
    }
}

void QGCTileCacheDatabaseTest::_testTilesTableColumns()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(Tiles)"));

    struct ColInfo
    {
        QString name;
        QString type;
        bool notnull;
        QString dflt;
        bool pk;
    };

    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({query.value(1).toString(), query.value(2).toString(), query.value(3).toBool(),
                     query.value(4).toString(), query.value(5).toBool()});
    }

    QCOMPARE(cols.size(), 7);

    auto findCol = [&](const QString& name) -> const ColInfo* {
        for (const auto& c : cols) {
            if (c.name == name)
                return &c;
        }
        return nullptr;
    };

    const ColInfo* c = findCol(QStringLiteral("tileID"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QVERIFY(c->pk);
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("hash"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("TEXT"));
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("format"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("TEXT"));
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("tile"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("BLOB"));

    c = findCol(QStringLiteral("size"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("type"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("date"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testTileSetsTableColumns()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(TileSets)"));

    struct ColInfo
    {
        QString name;
        QString type;
        bool notnull;
        QString dflt;
        bool pk;
    };

    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({query.value(1).toString(), query.value(2).toString(), query.value(3).toBool(),
                     query.value(4).toString(), query.value(5).toBool()});
    }

    QCOMPARE(cols.size(), 13);

    auto findCol = [&](const QString& name) -> const ColInfo* {
        for (const auto& c : cols) {
            if (c.name == name)
                return &c;
        }
        return nullptr;
    };

    const ColInfo* c = findCol(QStringLiteral("setID"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QVERIFY(c->pk);
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("name"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("TEXT"));
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("typeStr"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("TEXT"));

    c = findCol(QStringLiteral("topleftLat"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("REAL"));
    QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("topleftLon"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("REAL"));
    QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("bottomRightLat"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("REAL"));
    QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("bottomRightLon"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("REAL"));
    QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("minZoom"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("3"));

    c = findCol(QStringLiteral("maxZoom"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("3"));

    c = findCol(QStringLiteral("type"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("-1"));

    c = findCol(QStringLiteral("numTiles"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("0"));

    c = findCol(QStringLiteral("defaultSet"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("0"));

    c = findCol(QStringLiteral("date"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testSetTilesTableColumns()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(SetTiles)"));

    struct ColInfo
    {
        QString name;
        QString type;
        bool notnull;
    };

    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({query.value(1).toString(), query.value(2).toString(), query.value(3).toBool()});
    }

    QCOMPARE(cols.size(), 2);

    QCOMPARE(cols[0].name, QStringLiteral("setID"));
    QCOMPARE(cols[0].type, QStringLiteral("INTEGER"));
    QVERIFY(cols[0].notnull);

    QCOMPARE(cols[1].name, QStringLiteral("tileID"));
    QCOMPARE(cols[1].type, QStringLiteral("INTEGER"));
    QVERIFY(cols[1].notnull);
}

void QGCTileCacheDatabaseTest::_testTilesDownloadTableColumns()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(TilesDownload)"));

    struct ColInfo
    {
        QString name;
        QString type;
        bool notnull;
        QString dflt;
    };

    QList<ColInfo> cols;
    while (query.next()) {
        cols.append(
            {query.value(1).toString(), query.value(2).toString(), query.value(3).toBool(), query.value(4).toString()});
    }

    QCOMPARE(cols.size(), 7);

    auto findCol = [&](const QString& name) -> const ColInfo* {
        for (const auto& c : cols) {
            if (c.name == name)
                return &c;
        }
        return nullptr;
    };

    const ColInfo* c = findCol(QStringLiteral("setID"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("hash"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("TEXT"));
    QVERIFY(c->notnull);

    c = findCol(QStringLiteral("type"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("x"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("y"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("z"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("state"));
    QVERIFY(c);
    QCOMPARE(c->type, QStringLiteral("INTEGER"));
    QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testIndexesExist()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND sql IS NOT NULL ORDER BY name"));

    QStringList indexes;
    while (query.next()) {
        indexes << query.value(0).toString();
    }

    const QStringList expected = {
        QStringLiteral("idx_settiles_setid"),           QStringLiteral("idx_settiles_tileid"),
        QStringLiteral("idx_settiles_unique"),          QStringLiteral("idx_tiles_date"),
        QStringLiteral("idx_tilesdownload_setid_hash"), QStringLiteral("idx_tilesdownload_setid_state"),
    };

    QCOMPARE(indexes.size(), expected.size());
    for (const auto& name : expected) {
        QVERIFY2(indexes.contains(name), qPrintable(QStringLiteral("Missing index: %1").arg(name)));
    }
}

void QGCTileCacheDatabaseTest::_testForeignKeyCascadeDelete()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    QVERIFY(UrlFactory::getMapIdFromProviderType(kFixedProviderType) != -1);

    const QString hash = QStringLiteral("fk_cascade_hash");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), kFixedProviderType,
                         QGCTileCacheDatabase::kInvalidTileSet));

    const auto tileID = db->findTile(hash);
    QVERIFY(tileID.has_value());

    const auto setID = db->createTileSet(QStringLiteral("CascadeTestSet"), kFixedProviderType, 10.0, 20.0, 30.0, 40.0,
                                         5, 5, kFixedProviderType, 1, QString());
    QVERIFY(setID.has_value());

    _linkTileToSet(db.get(), tileID.value(), setID.value());

    // Verify the link exists
    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare("SELECT COUNT(*) FROM SetTiles WHERE setID = ?"));
        query.addBindValue(setID.value());
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
    }

    QVERIFY(db->deleteTileSet(setID.value()));

    // SetTiles rows for the deleted set should be gone (CASCADE)
    {
        QSqlQuery query(db->database());
        QVERIFY(query.prepare("SELECT COUNT(*) FROM SetTiles WHERE setID = ?"));
        query.addBindValue(setID.value());
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
    }

    // Tile itself should still exist (linked to default set)
    {
        auto tile = db->getTile(hash);
        QVERIFY(tile != nullptr);
    }
}

UT_REGISTER_TEST(QGCTileCacheDatabaseTest, TestLabel::Unit)
