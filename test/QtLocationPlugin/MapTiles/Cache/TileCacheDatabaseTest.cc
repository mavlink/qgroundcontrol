#include "TileCacheDatabaseTest.h"

#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "CacheTile.h"
#include "DownloadTile.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapProviderTable.h"
#include "QGCSqlHelper.h"
#include "TileCacheDatabase.h"
#include "TileCacheTestHelpers.h"
#include "TileRegion.h"

namespace {

/// A tile set of one zoom level for TileCacheDatabase::createTileSet()
TileSetRecord tileCacheSetRecord(const QString& name, const QString& mapName, double topleftLat, double topleftLon,
                                 double bottomRightLat, double bottomRightLon, int zoom, quint32 numTiles,
                                 int mapId = kTileCacheTestMapId)
{
    return TileSetRecord{.name = name,
                         .mapName = mapName,
                         .topleftLat = topleftLat,
                         .topleftLon = topleftLon,
                         .bottomRightLat = bottomRightLat,
                         .bottomRightLon = bottomRightLon,
                         .minZoom = zoom,
                         .maxZoom = zoom,
                         .mapId = mapId,
                         .numTiles = numTiles};
}

QString tileCacheHash(int mapId, int x, int y, int z)
{
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, z);
}

/// What begins a PNG and a JPEG picture, in hex: the cache tells a picture by it
const QString kTileCachePngHex = QStringLiteral("89504E470D0A1A0A");
const QString kTileCacheJpegHex = QStringLiteral("FFD8FF");

/// The map of the tiles that are imported from MBTiles files
constexpr int kTileCacheImportedMapId = MapProviderTable::kImportedMapId;

/// The set that TileCacheDatabase::importMBTiles() makes of the tiles of a file, before it has read the file
TileSetRecord tileCacheMBTilesSet(const QString& name)
{
    return TileSetRecord{.name = name, .mapName = QStringLiteral("MBTiles Imported"), .mapId = kTileCacheImportedMapId};
}

/// The tile set \a setID of \a db, as TileCacheDatabase::exportSets() takes it
TileSetRecord tileCacheSetRecordOf(TileCacheDatabase& db, quint64 setID)
{
    for (const TileSetRecord& set : db.getTileSets()) {
        if (set.setID == setID) {
            return set;
        }
    }
    return {};
}

const QString kTileCacheImportedTilesSql =
    QStringLiteral("SELECT hash, format, type, hex(tile), size FROM Tiles ORDER BY hash");

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

constexpr int kLargeTileCount = 20;
constexpr qint64 kLargeTilesSize = static_cast<qint64>(kLargeTileCount) * kTileCacheLargeTileSize;

/// Saves tiles that take hundreds of pages together, in the default set when \a setID is not a set
bool saveLargeTiles(TileCacheDatabase& db, quint64 setID)
{
    const QByteArray image(kTileCacheLargeTileSize, 'T');
    for (int i = 0; i < kLargeTileCount; i++) {
        if (!db.saveTile(QStringLiteral("large_%1").arg(i), QStringLiteral("png"), image, kTileCacheTestMapId, setID)) {
            return false;
        }
    }
    return true;
}

/// The size of the file once all that was written is in the file itself, and not in its write-ahead log
qint64 checkpointedFileSize(const TileCacheDatabase& db, const QString& path)
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

std::unique_ptr<TileCacheDatabase> TileCacheDatabaseTest::_createInitializedDB(QTemporaryDir& tempDir)
{
    auto db = std::make_unique<TileCacheDatabase>(tempDir.filePath("tiles.db"));
    if (!db->init() || !db->connectDB()) {
        return nullptr;
    }
    return db;
}

void TileCacheDatabaseTest::_insertTileSet(TileCacheDatabase* db, const QString& name, quint64& outSetID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT INTO TileSets(name, date) VALUES(?, ?)"));
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    QVERIFY(query.exec());
    outSetID = query.lastInsertId().toULongLong();
}

void TileCacheDatabaseTest::_insertDownloadRecord(TileCacheDatabase* db, quint64 setID, const QString& hash,
                                                  DownloadTile::TileState state)
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
    query.addBindValue(static_cast<int>(state));
    QVERIFY(query.exec());
}

void TileCacheDatabaseTest::_testInitWithValidPath()
{
    QTemporaryDir tempDir;
    TileCacheDatabase db(tempDir.filePath("tiles.db"));
    QVERIFY(!db.isValid());
    QVERIFY(!db.hasFailed());

    QVERIFY(db.init());
    QVERIFY(db.isValid());
    QVERIFY(!db.hasFailed());
}

void TileCacheDatabaseTest::_testInitWithEmptyPath()
{
    // Expected: critical about missing cache directory
    expectLogMessage("MapTiles.TileCacheDatabase", QtCriticalMsg,
                     QRegularExpression("Could not find suitable cache directory"));

    const QString emptyPath;
    TileCacheDatabase db(emptyPath);
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    QVERIFY(db.hasFailed());
    QVERIFY(!db.isValid());
}

// A file that cannot be opened is no cache, as no path is none
void TileCacheDatabaseTest::_testInitWithPathThatCannotBeOpened()
{
    QTemporaryDir tempDir;
    TileCacheDatabase db(tempDir.path());
    expectLogMessage("MapTiles.TileCacheDatabase", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Map Cache SQL error \\(open db\\)")));
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    QVERIFY(db.hasFailed());
    QVERIFY(!db.isValid());
}

void TileCacheDatabaseTest::_testConnectDisconnect()
{
    QTemporaryDir tempDir;
    TileCacheDatabase db(tempDir.filePath("tiles.db"));
    QVERIFY(db.init());

    QVERIFY(db.connectDB());
    QVERIFY(db.isValid());

    db.disconnectDB();

    QVERIFY(db.connectDB());
    QVERIFY(db.isValid());

    db.disconnectDB();
}

void TileCacheDatabaseTest::_testDefaultTileSetCreated()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
    QCOMPARE(sets[0].name, QStringLiteral("Default Tile Set"));
}

void TileCacheDatabaseTest::_testSaveTileAndGetTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString hash = QStringLiteral("test_hash_1234");
    const QString format = QStringLiteral("png");
    const QByteArray img("fake_tile_data_bytes");
    QVERIFY(db->saveTile(hash, format, img, kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(hash);
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->hash, hash);
    QCOMPARE(tile->format, format);
    QCOMPARE(tile->img, img);
    QCOMPARE(tile->mapId, kTileCacheTestMapId);
    // The map of a tile is stored as its id, not as its name
    QCOMPARE(tileCacheRows(tempDir.filePath("tiles.db"), QStringLiteral("SELECT typeof(type), type FROM Tiles")),
             QStringList{QStringLiteral("integer|%1").arg(kTileCacheTestMapId)});

    QVERIFY(db->getTile(QStringLiteral("nonexistent_hash")) == nullptr);
}

void TileCacheDatabaseTest::_testFindTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString hash = QStringLiteral("findme_hash");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    const auto id = db->findTile(hash);
    QVERIFY(id.has_value());
    QVERIFY(id.value() != 0);

    const auto missing = db->findTile(QStringLiteral("no_such_hash"));
    QVERIFY(!missing.has_value());
}

void TileCacheDatabaseTest::_testGetTileSetsMultiple()
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

void TileCacheDatabaseTest::_testRenameTileSet()
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

void TileCacheDatabaseTest::_testFindTileSetID()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto foundID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(foundID.has_value());
    QVERIFY(foundID.value() != 0);

    const auto missingID = db->findTileSetID(QStringLiteral("No Such Set"));
    QVERIFY(!missingID.has_value());
}

// A set goes with the tiles that no other set has, and with its links to the tiles that stay
void TileCacheDatabaseTest::_testDeleteTileSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString linksSql = QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = %1");

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("ToDelete"), setID);
    QVERIFY(setID != 0);
    const QByteArray data(50, 'C');
    QVERIFY(db->saveTile(QStringLiteral("of the set"), QStringLiteral("png"), data, kTileCacheTestMapId, setID));
    // A tile that a map saved before the set was made of its place is of the default set as well
    QVERIFY(db->saveTile(QStringLiteral("of both"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("of both"), QStringLiteral("png"), data, kTileCacheTestMapId, setID));
    QCOMPARE(db->getTileSets().size(), 2);
    QCOMPARE(tileCacheRows(cachePath, linksSql.arg(setID)), QStringList{QStringLiteral("2")});

    QVERIFY(db->deleteTileSet(setID));

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles")),
             QStringList{QStringLiteral("of both")});
    QCOMPARE(tileCacheRows(cachePath, linksSql.arg(setID)), QStringList{QStringLiteral("0")});
}

void TileCacheDatabaseTest::_testResetDatabase()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    QVERIFY(db->saveTile(QStringLiteral("h1"), QStringLiteral("png"), QByteArray("d1"), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->findTile(QStringLiteral("h1")).has_value());

    QVERIFY(db->resetDatabase());
    QVERIFY(db->isValid());

    QVERIFY(!db->findTile(QStringLiteral("h1")).has_value());

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
}

void TileCacheDatabaseTest::_testComputeTotalsOfDefaultSet_data()
{
    QTest::addColumn<bool>("sizeIndex");
    QTest::newRow("sizes read from their index") << true;
    // A cache whose index of the sizes could not be created
    QTest::newRow("cache without the index of the sizes") << false;
}

// The default set counts the tiles that no other set has
void TileCacheDatabaseTest::_testComputeTotalsOfDefaultSet()
{
    QFETCH(bool, sizeIndex);

    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Region"), setID);
    QVERIFY(db->saveTile(QStringLiteral("default_1"), QStringLiteral("png"), QByteArray(10, 'A'), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("default_2"), QStringLiteral("png"), QByteArray(20, 'B'), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("region_1"), QStringLiteral("png"), QByteArray(40, 'C'), kTileCacheTestMapId,
                         setID));
    if (!sizeIndex) {
        QSqlQuery query(db->database());
        QVERIFY(query.exec(QStringLiteral("DROP INDEX idx_tiles_size")));
    }

    const TileCacheTotals totals = db->computeTotals();
    QCOMPARE(totals.totalCount, 3U);
    QCOMPARE(totals.totalSize, quint64(70));
    QCOMPARE(totals.defaultCount, 2U);
    QCOMPARE(totals.defaultSize, quint64(30));
}

void TileCacheDatabaseTest::_testComputeSetTotalsDefault()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(15, 'X');
    QVERIFY(db->saveTile(QStringLiteral("sd1"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    const TileSetTotals result = db->computeSetTotals(defaultSetID.value(), true, 0, kTileCacheTestMapId);
    QCOMPARE(result.savedTileCount, static_cast<quint32>(1));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(15));
}

// "Clear Cache": what a map saved as it was looked at goes, and what the user downloaded as a set stays
void TileCacheDatabaseTest::_testDeleteCachedTilesKeepsTilesOfSets()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Offline"), setID);

    const QByteArray data(100, 'C');
    QVERIFY(db->saveTile(QStringLiteral("cached"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("of the set"), QStringLiteral("png"), data, kTileCacheTestMapId, setID));
    // A tile that a map saved before the set was made of its place is of both
    QVERIFY(db->saveTile(QStringLiteral("cached, and of the set"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("cached, and of the set"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         setID));

    QVERIFY(db->deleteCachedTiles());

    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles ORDER BY hash")),
             QStringList({QStringLiteral("cached, and of the set"), QStringLiteral("of the set")}));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT name FROM TileSets ORDER BY setID")),
             QStringList({QStringLiteral("Default Tile Set"), QStringLiteral("Offline")}));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = %1").arg(setID)),
             QStringList{QStringLiteral("2")});
    // The cache takes tiles as before
    QVERIFY(db->saveTile(QStringLiteral("cached again"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
}

// A prune deletes tiles of the default set, with their links, and none of an offline set
void TileCacheDatabaseTest::_testPruneCache()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString hashesSql = QStringLiteral("SELECT hash FROM Tiles ORDER BY hash");
    const QString linksSql = QStringLiteral("SELECT COUNT(*) FROM SetTiles");

    const QByteArray data(100, 'Z');
    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Offline"), setID);
    QVERIFY(db->saveTile(QStringLiteral("of a set"), QStringLiteral("png"), data, kTileCacheTestMapId, setID));
    QVERIFY(db->saveTile(QStringLiteral("pr1"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("pr2"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QCOMPARE(tileCacheRows(cachePath, linksSql), QStringList{QStringLiteral("3")});

    QVERIFY(db->pruneCache(200));
    QCOMPARE(tileCacheRows(cachePath, hashesSql), QStringList{QStringLiteral("of a set")});
    QCOMPARE(tileCacheRows(cachePath, linksSql), QStringList{QStringLiteral("1")});

    // More than is left to prune: the prune ends, and is no failure
    QVERIFY(db->pruneCache(200));
    QCOMPARE(tileCacheRows(cachePath, hashesSql), QStringList{QStringLiteral("of a set")});
}

// The cache is full of what was fetched once: the tiles of the places that are still looked at are to stay
void TileCacheDatabaseTest::_testPruneCacheKeepsTilesUsedLately()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));

    // The tile that is used was saved before the others
    const QByteArray data(100, 'U');
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    constexpr qint64 kDay = 24 * 60 * 60;
    const QList<std::pair<QString, qint64>> tiles = {{QStringLiteral("saved_first_used_today"), now - (30 * kDay)},
                                                     {QStringLiteral("saved_second"), now - (20 * kDay)},
                                                     {QStringLiteral("saved_third"), now - (10 * kDay)}};
    for (const auto& [hash, date] : tiles) {
        QVERIFY(
            db->saveTile(hash, QStringLiteral("png"), data, kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));
        QVERIFY(tileCacheExec(cachePath,
                              QStringLiteral("UPDATE Tiles SET date = %1 WHERE hash = '%2'").arg(date).arg(hash)));
    }
    db->markTileUsed(QStringLiteral("saved_first_used_today"));

    QVERIFY(db->pruneCache(data.size()));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles ORDER BY hash")),
             QStringList({QStringLiteral("saved_first_used_today"), QStringLiteral("saved_third")}));

    QVERIFY(db->pruneCache(data.size()));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles")),
             QStringList{QStringLiteral("saved_first_used_today")});
}

// A map reads its tiles again and again: the use of a tile is written once a day, not at each of them, and only
// where a prune reads it
void TileCacheDatabaseTest::_testMarkTileUsedWritesOnceADay()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString dateSql = QStringLiteral("SELECT date FROM Tiles WHERE hash = 'used'");

    QVERIFY(db->saveTile(QStringLiteral("used"), QStringLiteral("png"), QByteArray(100, 'U'), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 anHourAgo = now - (60 * 60);
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("UPDATE Tiles SET date = %1 WHERE hash = 'used'").arg(anHourAgo)));
    db->markTileUsed(QStringLiteral("used"));
    QCOMPARE(tileCacheRows(cachePath, dateSql), QStringList{QString::number(anHourAgo)});

    const qint64 twoDaysAgo = now - (2 * 24 * 60 * 60);
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("UPDATE Tiles SET date = %1 WHERE hash = 'used'").arg(twoDaysAgo)));
    db->markTileUsed(QStringLiteral("used"));
    const QStringList date = tileCacheRows(cachePath, dateSql);
    QCOMPARE(date.size(), 1);
    QCOMPARE_GE(date.constFirst().toLongLong(), now);

    // A tile that the cache lacks is none to mark
    db->markTileUsed(QStringLiteral("not in the cache"));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{QStringLiteral("1")});

    // The tile of an offline set is never pruned: its use is not written at all
    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Offline"), setID);
    QVERIFY(db->saveTile(QStringLiteral("of a set"), QStringLiteral("png"), QByteArray(100, 'S'), kTileCacheTestMapId,
                         setID));
    QVERIFY(
        tileCacheExec(cachePath, QStringLiteral("UPDATE Tiles SET date = %1 WHERE hash = 'of a set'").arg(twoDaysAgo)));
    db->markTileUsed(QStringLiteral("of a set"));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT date FROM Tiles WHERE hash = 'of a set'")),
             QStringList{QString::number(twoDaysAgo)});
}

void TileCacheDatabaseTest::_testUpdateTileDownloadState()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString statesSql = QStringLiteral("SELECT hash, state FROM TilesDownload ORDER BY hash");
    const auto row = [](const QString& hash, DownloadTile::TileState state) {
        return QStringLiteral("%1|%2").arg(hash).arg(static_cast<int>(state));
    };

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_hash_1"),
                          DownloadTile::TileState::Pending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_hash_2"),
                          DownloadTile::TileState::Pending);

    QVERIFY(db->updateTileDownloadState(defaultSetID.value(), DownloadTile::TileState::Downloading,
                                        QStringLiteral("dl_hash_1")));
    QCOMPARE(tileCacheRows(cachePath, statesSql),
             QStringList({row(QStringLiteral("dl_hash_1"), DownloadTile::TileState::Downloading),
                          row(QStringLiteral("dl_hash_2"), DownloadTile::TileState::Pending)}));

    // A tile that is downloaded is none to download any more
    QVERIFY(db->updateTileDownloadState(defaultSetID.value(), DownloadTile::TileState::Complete,
                                        QStringLiteral("dl_hash_1")));
    QCOMPARE(tileCacheRows(cachePath, statesSql),
             QStringList{row(QStringLiteral("dl_hash_2"), DownloadTile::TileState::Pending)});

    QVERIFY(db->updateAllTileDownloadStates(defaultSetID.value(), DownloadTile::TileState::Error));
    QCOMPARE(tileCacheRows(cachePath, statesSql),
             QStringList{row(QStringLiteral("dl_hash_2"), DownloadTile::TileState::Error)});
}

// The states of the tiles of a download are stored together: a tile whose state fails leaves the others as they were,
// so that none is taken for stored that is not
void TileCacheDatabaseTest::_testUpdateTileDownloadStatesIsAllOrNone()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Downloading"), setID);
    for (const QString& hash : {QStringLiteral("fetched"), QStringLiteral("failed"), QStringLiteral("in flight")}) {
        _insertDownloadRecord(db.get(), setID, hash, DownloadTile::TileState::Downloading);
    }
    const QList<DownloadTileState> states = {{QStringLiteral("fetched"), DownloadTile::TileState::Complete},
                                             {QStringLiteral("failed"), DownloadTile::TileState::Error}};
    const QString statesSql = QStringLiteral("SELECT hash, state FROM TilesDownload ORDER BY hash");

    // The database refuses the state of the tile that failed
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("CREATE TRIGGER refuse_error BEFORE UPDATE ON TilesDownload "
                                                    "WHEN NEW.state = 2 BEGIN SELECT RAISE(ABORT, 'refused'); END")));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("set TilesDownload state")));
    QVERIFY(!db->updateTileDownloadStates(setID, states));
    verifyExpectedLogMessage();
    QCOMPARE(tileCacheRows(cachePath, statesSql),
             QStringList({QStringLiteral("failed|1"), QStringLiteral("fetched|1"), QStringLiteral("in flight|1")}));

    QVERIFY(tileCacheExec(cachePath, QStringLiteral("DROP TRIGGER refuse_error")));
    QVERIFY(db->updateTileDownloadStates(setID, states));
    QCOMPARE(tileCacheRows(cachePath, statesSql),
             QStringList({QStringLiteral("failed|2"), QStringLiteral("in flight|1")}));
}

void TileCacheDatabaseTest::_testExportImportReplace()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data(50, 'E');
    QVERIFY(db->saveTile(QStringLiteral("exp1"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("exp2"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    const auto sets = db->getTileSets();
    QVERIFY(!sets.isEmpty());

    TileSetRecord defaultRec = sets[0];
    defaultRec.numTiles = 2;

    const QString exportPath = tempDir.filePath("export.db");
    int lastProgress = 0;
    auto progressCb = [&lastProgress](int p) { lastProgress = p; };

    const TileCacheResult exportResult = db->exportSets({defaultRec}, exportPath, progressCb);
    QVERIFY(exportResult.success);
    QVERIFY(lastProgress > 0);
    QCOMPARE(tileCacheRows(exportPath, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});

    db.reset();

    auto db2 = std::make_unique<TileCacheDatabase>(tempDir.filePath("tiles2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());

    QVERIFY(!db2->findTile(QStringLiteral("exp1")).has_value());

    const TileCacheResult importResult = db2->importSetsReplace(exportPath, nullptr);
    QVERIFY(importResult.success);
    QVERIFY(db2->isValid());

    QVERIFY(db2->findTile(QStringLiteral("exp1")).has_value());
    QVERIFY(db2->findTile(QStringLiteral("exp2")).has_value());
}

// A file to import can be read-only, as one that was copied from a medium that is. The cache is a copy of it, and
// has tiles to save.
void TileCacheDatabaseTest::_testImportReplaceOfReadOnlyFileLeavesCacheWritable()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QByteArray data(50, 'E');
    QVERIFY(db->saveTile(QStringLiteral("exported"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    TileSetRecord defaultSet = db->getTileSets().constFirst();
    defaultSet.numTiles = 1;
    const QString exportPath = tempDir.filePath(QStringLiteral("export.db"));
    QVERIFY(db->exportSets({defaultSet}, exportPath, nullptr).success);
    QVERIFY(QFile::setPermissions(exportPath, QFile::ReadOwner));

    const TileCacheResult result = db->importSetsReplace(exportPath, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QVERIFY(db->findTile(QStringLiteral("exported")).has_value());

    QVERIFY(db->saveTile(QStringLiteral("saved after"), QStringLiteral("png"), data, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->findTile(QStringLiteral("saved after")).has_value());
}

void TileCacheDatabaseTest::_linkTileToSet(TileCacheDatabase* db, quint64 tileID, quint64 setID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"));
    query.addBindValue(tileID);
    query.addBindValue(setID);
    QVERIFY(query.exec());
}

void TileCacheDatabaseTest::_testGetTileDownloadList()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_a"), DownloadTile::TileState::Pending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_b"), DownloadTile::TileState::Pending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_c"), DownloadTile::TileState::Pending);

    const std::optional<QList<DownloadTile>> tiles = db->getTileDownloadList(defaultSetID.value(), 2);
    QVERIFY(tiles.has_value());
    QStringList hashes;
    for (const auto& t : *tiles) {
        hashes.append(t.hash);
    }
    hashes.sort();
    QCOMPARE(hashes, QStringList({QStringLiteral("dl_a"), QStringLiteral("dl_b")}));

    // The tiles of the list are marked as being downloaded, and no other
    const auto row = [](const QString& hash, DownloadTile::TileState state) {
        return QStringLiteral("%1|%2").arg(hash).arg(static_cast<int>(state));
    };
    QCOMPARE(tileCacheRows(tempDir.filePath(QStringLiteral("tiles.db")),
                           QStringLiteral("SELECT hash, state FROM TilesDownload ORDER BY hash")),
             QStringList({row(QStringLiteral("dl_a"), DownloadTile::TileState::Downloading),
                          row(QStringLiteral("dl_b"), DownloadTile::TileState::Downloading),
                          row(QStringLiteral("dl_c"), DownloadTile::TileState::Pending)}));
}

// A list that cannot be read is not an empty one, which would make the set look complete
void TileCacheDatabaseTest::_testGetTileDownloadListFailureIsNone()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_a"));
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("CREATE TRIGGER refuse_state BEFORE UPDATE ON TilesDownload "
                                                    "BEGIN SELECT RAISE(ABORT, 'refused'); END")));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("batch set TilesDownload state.*refused")));
    QVERIFY(!db->getTileDownloadList(defaultSetID.value(), 2).has_value());
    verifyExpectedLogMessage();

    // The tile is still to download
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash, state FROM TilesDownload")),
             QStringList{QStringLiteral("dl_a|%1").arg(static_cast<int>(DownloadTile::TileState::Pending))});
}

void TileCacheDatabaseTest::_testImportSetsMerge()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);

    const QByteArray data(40, 'M');
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m1"), QStringLiteral("png"), data, kTileCacheTestMapId,
                            TileCacheDatabase::kInvalidTileSet));
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m2"), QStringLiteral("png"), data, kTileCacheTestMapId,
                            TileCacheDatabase::kInvalidTileSet));

    const auto srcSets = dbSrc->getTileSets();
    QVERIFY(!srcSets.isEmpty());
    TileSetRecord srcRec = srcSets[0];
    srcRec.numTiles = 2;

    const QString exportPath = tempDir.filePath("merge_export.db");
    const TileCacheResult exportResult = dbSrc->exportSets({srcRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<TileCacheDatabase>(tempDir.filePath("merge_target.db"));
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());

    QVERIFY(dbTgt->saveTile(QStringLiteral("trg_m1"), QStringLiteral("png"), QByteArray(25, 'N'), kTileCacheTestMapId,
                            TileCacheDatabase::kInvalidTileSet));

    int lastProgress = 0;
    auto progressCb = [&lastProgress](int p) { lastProgress = p; };

    const TileCacheResult importResult = dbTgt->importSetsMerge(exportPath, progressCb);
    QVERIFY(importResult.success);
    QVERIFY(lastProgress > 0);

    QVERIFY(dbTgt->findTile(QStringLiteral("trg_m1")).has_value());
    QVERIFY(dbTgt->findTile(QStringLiteral("exp_m1")).has_value());
    QVERIFY(dbTgt->findTile(QStringLiteral("exp_m2")).has_value());
}

void TileCacheDatabaseTest::_testComputeSetTotalsNonDefault()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 customSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("Stats Set"), customSetID);
    QVERIFY(customSetID != 0);

    const QByteArray data20(20, 'A');
    const QByteArray data30(30, 'B');
    QVERIFY(db->saveTile(QStringLiteral("st1"), QStringLiteral("png"), data20, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("st2"), QStringLiteral("png"), data30, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    const auto tile1 = db->findTile(QStringLiteral("st1"));
    const auto tile2 = db->findTile(QStringLiteral("st2"));
    QVERIFY(tile1.has_value());
    QVERIFY(tile2.has_value());

    _linkTileToSet(db.get(), tile1.value(), customSetID);
    _linkTileToSet(db.get(), tile2.value(), customSetID);

    // With a tile left to download, the set still counts the tiles it was created for
    _insertDownloadRecord(db.get(), customSetID, QStringLiteral("st3"), DownloadTile::TileState::Error);
    const TileSetTotals result = db->computeSetTotals(customSetID, false, 5, kTileCacheTestMapId);
    QCOMPARE(result.totalTileCount, static_cast<quint32>(5));
    QCOMPARE(result.savedTileCount, static_cast<quint32>(2));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(50));
    // Its size is an estimate until then: that of an average tile of its provider for each of its tiles
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(kTileCacheTestMapId);
    QCOMPARE(result.totalTileSize, static_cast<quint64>(5) * provider->averageSize());

    // With nothing left to download, it is complete with the tiles it has
    QVERIFY(db->updateTileDownloadState(customSetID, DownloadTile::TileState::Complete, QStringLiteral("st3")));
    const TileSetTotals doneResult = db->computeSetTotals(customSetID, false, 5, kTileCacheTestMapId);
    QCOMPARE(doneResult.totalTileCount, static_cast<quint32>(2));
    QCOMPARE(doneResult.savedTileCount, static_cast<quint32>(2));
    QCOMPARE(doneResult.totalTileSize, static_cast<quint64>(50));
}

void TileCacheDatabaseTest::_testSaveDuplicateTile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QByteArray data1(10, 'X');
    const QByteArray data2(20, 'Y');

    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("png"), data1, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    // The tile saved last takes the place of the one with its key: it was fetched because that one could not be used
    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("jpg"), data2, kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(QStringLiteral("dup_hash"));
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->img, data2);
    QCOMPARE(tile->format, QStringLiteral("jpg"));
    const TileCacheTotals totals = db->computeTotals();
    QCOMPARE(totals.totalCount, static_cast<quint32>(1));
    QCOMPARE(totals.totalSize, static_cast<quint64>(data2.size()));
}

void TileCacheDatabaseTest::_testOperationsAfterDisconnect()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    db->disconnectDB();

    const char* categoryPattern = "MapTiles.TileCacheDatabase";
    const QRegularExpression disconnectedPattern(QStringLiteral("Database not connected"));

    expectLogMessage(categoryPattern, QtWarningMsg, disconnectedPattern);
    QVERIFY(!db->saveTile(QStringLiteral("x"), QStringLiteral("png"), QByteArray("d"), kTileCacheTestMapId,
                          TileCacheDatabase::kInvalidTileSet));
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
    const TileCacheTotals totals = db->computeTotals();
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

void TileCacheDatabaseTest::_testDeleteTileSetWithMoreTilesThanSqlParameters()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("LargeSet"), setID);
    QVERIFY(setID != 0);

    // A statement of SQLite has at most 32766 parameters, unless SQLite was built for more
    constexpr int kTileCount = 33000;
    QSqlQuery query(db->database());
    QVERIFY(query.prepare(
        QStringLiteral("WITH RECURSIVE numbers(n) AS (SELECT 1 UNION ALL SELECT n + 1 FROM numbers WHERE n < ?) "
                       "INSERT INTO Tiles(hash, format, tile, size, type, date) "
                       "SELECT 'large' || n, 'png', x'00', 1, 6, 0 FROM numbers")));
    query.addBindValue(kTileCount);
    QVERIFY(query.exec());
    QVERIFY(query.prepare(QStringLiteral("INSERT INTO SetTiles(tileID, setID) SELECT tileID, ? FROM Tiles")));
    query.addBindValue(setID);
    QVERIFY(query.exec());
    QCOMPARE(db->computeTotals().totalCount, static_cast<quint32>(kTileCount));

    QVERIFY(db->deleteTileSet(setID));

    QCOMPARE(db->computeTotals().totalCount, 0U);
}

void TileCacheDatabaseTest::_testImportSetsMergeDeduplicatesName()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);

    quint64 customSetID = 0;
    _insertTileSet(dbSrc.get(), QStringLiteral("SharedName"), customSetID);
    QVERIFY(customSetID != 0);

    QVERIFY(dbSrc->saveTile(QStringLiteral("dedup_tile1"), QStringLiteral("png"), QByteArray(30, 'D'),
                            kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));
    const auto tileID = dbSrc->findTile(QStringLiteral("dedup_tile1"));
    QVERIFY(tileID.has_value());
    _linkTileToSet(dbSrc.get(), tileID.value(), customSetID);

    TileSetRecord customRec = tileCacheSetRecordOf(*dbSrc, customSetID);
    QCOMPARE(customRec.name, QStringLiteral("SharedName"));
    customRec.numTiles = 1;

    const QString exportPath = tempDir.filePath("dedup_export.db");
    const TileCacheResult exportResult = dbSrc->exportSets({customRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<TileCacheDatabase>(tempDir.filePath("dedup_target.db"));
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());

    // The cache has a set of the name, and one of the first name that tells a second set of it apart
    quint64 existingSetID = 0;
    _insertTileSet(dbTgt.get(), QStringLiteral("SharedName"), existingSetID);
    QVERIFY(existingSetID != 0);
    _insertTileSet(dbTgt.get(), QStringLiteral("SharedName 0001"), existingSetID);

    const TileCacheResult importResult = dbTgt->importSetsMerge(exportPath, nullptr);
    QVERIFY(importResult.success);

    QCOMPARE(tileCacheRows(tempDir.filePath("dedup_target.db"),
                           QStringLiteral("SELECT name FROM TileSets WHERE defaultSet = 0 ORDER BY name")),
             QStringList(
                 {QStringLiteral("SharedName"), QStringLiteral("SharedName 0001"), QStringLiteral("SharedName 0002")}));
}

// A statement copies a chunk of the tiles of a set: every tile of a set of several chunks is copied once, and one
// that the cache has under its key already stays as it is
void TileCacheDatabaseTest::_testExportAndMergeSetOfSeveralChunks()
{
    constexpr int kTileCount = 4500;
    const QString tileCount = QString::number(kTileCount);
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);
    QVERIFY(dbSrc);

    quint64 setID = 0;
    _insertTileSet(dbSrc.get(), QStringLiteral("Large"), setID);
    {
        QSqlQuery query(dbSrc->database());
        QVERIFY(
            query.exec(QStringLiteral("WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM n WHERE i < %1) "
                                      "INSERT INTO Tiles(hash, format, tile, size, type, date) "
                                      "SELECT 'large_' || i, 'png', CAST('exported' AS BLOB), 8, 0, i FROM n")
                           .arg(kTileCount)));
        QVERIFY(
            query.exec(QStringLiteral("INSERT INTO SetTiles(tileID, setID) SELECT tileID, %1 FROM Tiles").arg(setID)));
    }
    const TileSetRecord record = tileCacheSetRecordOf(*dbSrc, setID);
    QCOMPARE(record.setID, setID);

    const QString exportPath = tempDir.filePath("large_export.db");
    QList<int> exportProgress;
    const TileCacheResult exported = dbSrc->exportSets(
        {record}, exportPath, [&exportProgress](int percentage) { exportProgress.append(percentage); });
    QVERIFY2(exported.success, qPrintable(exported.errorString));
    QCOMPARE(tileCacheRows(exportPath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{tileCount});
    QCOMPARE(tileCacheRows(exportPath, QStringLiteral("SELECT COUNT(*) FROM SetTiles")), QStringList{tileCount});
    QCOMPARE_GT(exportProgress.size(), 2);
    QVERIFY(std::is_sorted(exportProgress.cbegin(), exportProgress.cend()));
    QCOMPARE(exportProgress.constLast(), 100);
    dbSrc.reset();

    const QString targetPath = tempDir.filePath("large_target.db");
    auto dbTgt = std::make_unique<TileCacheDatabase>(targetPath);
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());
    QVERIFY(dbTgt->saveTile(QStringLiteral("large_7"), QStringLiteral("png"), QByteArrayLiteral("kept"),
                            kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));

    QList<int> importProgress;
    const TileCacheResult imported =
        dbTgt->importSetsMerge(exportPath, [&importProgress](int percentage) { importProgress.append(percentage); });
    QVERIFY2(imported.success, qPrintable(imported.errorString));
    QCOMPARE_GT(importProgress.size(), 2);
    QVERIFY(std::is_sorted(importProgress.cbegin(), importProgress.cend()));
    QCOMPARE(importProgress.constLast(), 100);

    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{tileCount});
    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT COUNT(*), T.numTiles FROM SetTiles S JOIN TileSets T "
                                                      "ON T.setID = S.setID WHERE T.name = 'Large'")),
             QStringList{tileCount + QLatin1Char('|') + tileCount});
    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT CAST(tile AS TEXT) FROM Tiles WHERE hash = 'large_7'")),
             QStringList{QStringLiteral("kept")});
}

void TileCacheDatabaseTest::_testImportMBTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString path = tempDir.filePath(QStringLiteral("airfield.mbtiles"));
    // The last tile is no picture
    QVERIFY(tileCacheCreateMBTiles(path, QStringLiteral("('format', 'png'), ('bounds', '8.5, 47.3, 8.6, 47.4')"),
                                   QStringLiteral("(1, 0, 0, x'%1AA'), (2, 1, 3, x'%2BBBB'), (2, 2, 1, x'CCCC')")
                                       .arg(kTileCachePngHex, kTileCacheJpegHex)));
    QVERIFY(TileCacheDatabase::isMBTiles(path));
    // A tile cache has a table of tiles as well
    QVERIFY(!TileCacheDatabase::isMBTiles(cachePath));

    QList<int> progress;
    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("airfield")), /*replace=*/false,
                          [&progress](int percentage) { progress.append(percentage); });
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE(progress.constLast(), 100);

    // The rows of a tile count from the north in the cache, and each tile has the format of its picture
    const QStringList expectedTiles = {
        QStringLiteral("%1|png|42|%2AA|9").arg(tileCacheHash(kTileCacheImportedMapId, 0, 1, 1), kTileCachePngHex),
        QStringLiteral("%1|jpg|42|%2BBBB|5").arg(tileCacheHash(kTileCacheImportedMapId, 1, 0, 2), kTileCacheJpegHex),
    };
    QCOMPARE(tileCacheRows(cachePath, kTileCacheImportedTilesSql), expectedTiles);

    const QList<TileSetRecord> sets = db->getTileSets();
    QCOMPARE(sets.size(), 2);
    const TileSetRecord& set = sets.constLast();
    QCOMPARE(set.name, QStringLiteral("airfield"));
    QCOMPARE(set.mapName, QStringLiteral("MBTiles Imported"));
    QCOMPARE(set.mapId, kTileCacheImportedMapId);
    QVERIFY(!set.defaultSet);
    QCOMPARE(set.minZoom, 1);
    QCOMPARE(set.maxZoom, 2);
    QCOMPARE(set.topleftLat, 47.4);
    QCOMPARE(set.topleftLon, 8.5);
    QCOMPARE(set.bottomRightLat, 47.3);
    QCOMPARE(set.bottomRightLon, 8.6);
    // The set is complete with the tiles it has
    QCOMPARE(set.numTiles, 2U);
    QCOMPARE(db->computeSetTotals(set.setID, false, set.numTiles, set.mapId).savedTileCount, 2U);
}

void TileCacheDatabaseTest::_testImportMBTilesIgnoresBoundsThatAreNoArea_data()
{
    QTest::addColumn<QString>("bounds");  // west, south, east, north

    // toDouble() reads "nan", which no range check refuses
    QTest::newRow("edge that is not a number") << QStringLiteral("8.5, nan, 8.6, 47.4");
    QTest::newRow("west edge east of the east edge") << QStringLiteral("8.6, 47.3, 8.5, 47.4");
    QTest::newRow("edge beyond the pole") << QStringLiteral("8.5, 47.3, 8.6, 97.4");
    QTest::newRow("three edges") << QStringLiteral("8.5, 47.3, 8.6");
}

// The set gets the area of its tiles, as without bounds
void TileCacheDatabaseTest::_testImportMBTilesIgnoresBoundsThatAreNoArea()
{
    QFETCH(QString, bounds);

    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString path = tempDir.filePath(QStringLiteral("bounds.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, QStringLiteral("('bounds', '%1')").arg(bounds),
                                   QStringLiteral("(3, 4, 5, x'%1AA')").arg(kTileCachePngHex)));

    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("bounds")), /*replace=*/false, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));

    // The area of the tile: column 4 of 8, and the third row from the north
    const TileSetRecord set = db->getTileSets().constLast();
    QCOMPARE(set.name, QStringLiteral("bounds"));
    QCOMPARE(set.topleftLon, 0.);
    QCOMPARE(set.bottomRightLon, 45.);
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(kTileCacheImportedMapId);
    QCOMPARE(set.topleftLat, provider->tileY2lat(2, 3));
    QCOMPARE(set.bottomRightLat, provider->tileY2lat(3, 3));
}

void TileCacheDatabaseTest::_testImportMBTilesReplacesTilesImportedBefore()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString olderPath = tempDir.filePath(QStringLiteral("older.mbtiles"));
    const QString newerPath = tempDir.filePath(QStringLiteral("newer.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(olderPath, QString(),
                                   QStringLiteral("(3, 4, 5, x'%1AA'), (3, 5, 5, x'%1BB')").arg(kTileCachePngHex)));
    // Without metadata, and with a view for its tiles, as files with a table of the pictures have
    QVERIFY(tileCacheCreateMBTiles(newerPath, QString(),
                                   QStringLiteral("(3, 4, 5, x'%1CC'), (2, 0, 0, x'%1DD')").arg(kTileCacheJpegHex)));
    QVERIFY(tileCacheExec(newerPath, QStringLiteral("ALTER TABLE tiles RENAME TO pictures")));
    QVERIFY(tileCacheExec(newerPath, QStringLiteral("CREATE VIEW tiles AS SELECT * FROM pictures")));

    const TileSetRecord importedSet = tileCacheMBTilesSet(QStringLiteral("field"));
    QVERIFY(db->importMBTiles(olderPath, importedSet, /*replace=*/false, nullptr).success);
    const TileCacheResult result = db->importMBTiles(newerPath, importedSet, /*replace=*/false, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));

    // The tile that both files have is of the file that was imported last
    const QStringList expectedTiles = {
        QStringLiteral("%1|jpg|42|%2DD|4").arg(tileCacheHash(kTileCacheImportedMapId, 0, 3, 2), kTileCacheJpegHex),
        QStringLiteral("%1|jpg|42|%2CC|4").arg(tileCacheHash(kTileCacheImportedMapId, 4, 2, 3), kTileCacheJpegHex),
        QStringLiteral("%1|png|42|%2BB|9").arg(tileCacheHash(kTileCacheImportedMapId, 5, 2, 3), kTileCachePngHex),
    };
    QCOMPARE(tileCacheRows(cachePath, kTileCacheImportedTilesSql), expectedTiles);

    // Each file is a set of its own, with the tiles of the file
    const QList<TileSetRecord> sets = db->getTileSets();
    QCOMPARE(sets.size(), 3);
    QCOMPARE(sets[1].name, QStringLiteral("field"));
    QCOMPARE(sets[1].numTiles, 2U);
    const TileSetRecord& newer = sets[2];
    QCOMPARE(newer.name, QStringLiteral("field 0001"));
    QCOMPARE(newer.numTiles, 2U);
    QCOMPARE(db->computeSetTotals(newer.setID, false, newer.numTiles, newer.mapId).savedTileCount, 2U);
    QCOMPARE(newer.minZoom, 2);
    QCOMPARE(newer.maxZoom, 3);
    // The area of its deepest tile: column 4 of 8, and the third row from the north
    QCOMPARE(newer.topleftLon, 0.);
    QCOMPARE(newer.bottomRightLon, 45.);
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(kTileCacheImportedMapId);
    QCOMPARE(newer.topleftLat, provider->tileY2lat(2, 3));
    QCOMPARE(newer.bottomRightLat, provider->tileY2lat(3, 3));
}

void TileCacheDatabaseTest::_testImportMBTilesRefusesFileWithoutPictures_data()
{
    QTest::addColumn<QString>("metadataRows");
    QTest::addColumn<QString>("error");

    QTest::newRow("vector tiles") << QStringLiteral("('format', 'pbf')")
                                  << QStringLiteral("The file has vector tiles, which cannot be shown");
    QTest::newRow("no picture") << QString() << QStringLiteral("The file has no map tiles that can be shown");
}

void TileCacheDatabaseTest::_testImportMBTilesRefusesFileWithoutPictures()
{
    QFETCH(QString, metadataRows);
    QFETCH(QString, error);

    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString path = tempDir.filePath(QStringLiteral("vector.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, metadataRows, QStringLiteral("(1, 0, 0, x'1F8B0800')")));

    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("vector")), /*replace=*/false, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, error);
    // No set without tiles is left behind
    QCOMPARE(db->getTileSets().size(), 1);
    QCOMPARE(tileCacheRows(tempDir.filePath(QStringLiteral("tiles.db")), kTileCacheImportedTilesSql), QStringList());
}

void TileCacheDatabaseTest::_testImportMBTilesInPlaceOfCache_data()
{
    QTest::addColumn<QString>("tileRows");
    QTest::addColumn<bool>("imported");

    QTest::newRow("file with a picture") << QStringLiteral("(1, 0, 0, x'%1AA')").arg(kTileCachePngHex) << true;
    // What the user chose to replace is not lost to a file that cannot be imported
    QTest::newRow("file without a picture") << QStringLiteral("(1, 0, 0, x'1F8B0800')") << false;
}

void TileCacheDatabaseTest::_testImportMBTilesInPlaceOfCache()
{
    QFETCH(QString, tileRows);
    QFETCH(bool, imported);

    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    // A tile of the default set, and a set with a tile and with one that it is to download
    quint64 earlierSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("Earlier"), earlierSetID);
    QVERIFY(db->saveTile(QStringLiteral("default_tile"), QStringLiteral("png"), QByteArray(10, 'D'),
                         kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("earlier_tile"), QStringLiteral("png"), QByteArray(10, 'E'),
                         kTileCacheTestMapId, earlierSetID));
    _insertDownloadRecord(db.get(), earlierSetID, QStringLiteral("earlier_download"));

    const QString path = tempDir.filePath(QStringLiteral("field.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, QString(), tileRows));
    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("field")), /*replace=*/true, nullptr);
    QCOMPARE(result.success, imported);

    const QString importedHash = tileCacheHash(kTileCacheImportedMapId, 0, 1, 1);
    const QStringList setNames = tileCacheRows(cachePath, QStringLiteral("SELECT name FROM TileSets ORDER BY setID"));
    const QStringList hashes = tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles ORDER BY hash"));
    const QStringList linkCount = tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM SetTiles"));
    const QStringList downloads = tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM TilesDownload"));
    if (imported) {
        QCOMPARE(setNames, QStringList({QStringLiteral("Default Tile Set"), QStringLiteral("field")}));
        QCOMPARE(hashes, QStringList{importedHash});
        QCOMPARE(linkCount, QStringList{QStringLiteral("1")});
        QCOMPARE(downloads, QStringList());
    } else {
        QCOMPARE(setNames, QStringList({QStringLiteral("Default Tile Set"), QStringLiteral("Earlier")}));
        QCOMPARE(hashes, QStringList({QStringLiteral("default_tile"), QStringLiteral("earlier_tile")}));
        QCOMPARE(linkCount, QStringList{QStringLiteral("2")});
        QCOMPARE(downloads, QStringList{QStringLiteral("earlier_download")});
    }
}

// The tiles are committed a chunk at a time, so that the write-ahead log, which holds a transaction whole, does not
// grow to the size of the map: the disk would have to hold the map twice
void TileCacheDatabaseTest::_testImportMBTilesOfSeveralChunksKeepsLogSmall()
{
    // More than three chunks of tiles that take a page each
    constexpr int kTileCount = 7000;
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString path = tempDir.filePath(QStringLiteral("large.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, QString(), QStringLiteral("(13, 0, 0, x'%1')").arg(kTileCachePngHex)));
    QVERIFY(tileCacheExec(
        path,
        QStringLiteral("WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM n WHERE i < %1) "
                       "INSERT INTO tiles SELECT 13, i % 100, i / 100, CAST(x'%2' || randomblob(3000) AS BLOB) FROM n")
            .arg(kTileCount - 1)
            .arg(kTileCachePngHex)));

    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("large")), /*replace=*/false, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE_LT(QFileInfo(cachePath + QStringLiteral("-wal")).size(), QFileInfo(cachePath).size() / 2);

    const QString tileCount = QString::number(kTileCount);
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{tileCount});
    QCOMPARE(
        tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*), T.numTiles, T.minZoom, T.maxZoom FROM SetTiles S "
                                                "JOIN TileSets T ON T.setID = S.setID WHERE T.name = 'large'")),
        QStringList{QStringLiteral("%1|%1|13|13").arg(tileCount)});
}

// An import that fails after tiles were committed leaves no set that looks whole, and none of its tiles
void TileCacheDatabaseTest::_testImportMBTilesThatFailsLeavesNoSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    quint64 earlierSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("Earlier"), earlierSetID);
    QVERIFY(db->saveTile(QStringLiteral("earlier_tile"), QStringLiteral("png"), QByteArray(10, 'E'),
                         kTileCacheTestMapId, earlierSetID));
    // The cache refuses the last tile of the file, which is in its second chunk
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("CREATE TRIGGER refuse_tile BEFORE INSERT ON Tiles "
                                                    "WHEN NEW.size = 777 BEGIN SELECT RAISE(ABORT, 'refused'); END")));

    const QString path = tempDir.filePath(QStringLiteral("broken.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, QString(), QStringLiteral("(13, 0, 0, x'%1')").arg(kTileCachePngHex)));
    QVERIFY(tileCacheExec(
        path, QStringLiteral("WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM n WHERE i < 2500) "
                             "INSERT INTO tiles SELECT 13, i % 100, i / 100, x'%1AA' FROM n")
                  .arg(kTileCachePngHex)));
    QVERIFY(
        tileCacheExec(path, QStringLiteral("INSERT INTO tiles VALUES (13, 0, 99, CAST(x'%1' || zeroblob(769) AS BLOB))")
                                .arg(kTileCachePngHex)));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg, QRegularExpression(QStringLiteral("import MBTiles")));
    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("broken")), /*replace=*/false, nullptr);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);

    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT name FROM TileSets ORDER BY setID")),
             QStringList({QStringLiteral("Default Tile Set"), QStringLiteral("Earlier")}));
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT hash FROM Tiles")),
             QStringList{QStringLiteral("earlier_tile")});
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM SetTiles")),
             QStringList{QStringLiteral("1")});
}

// A file that cannot be read to its end is no map: the tiles before the row that fails are not taken for the file
void TileCacheDatabaseTest::_testImportMBTilesThatCannotBeReadLeavesNoSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));

    // The tiles of a file can be a view. The picture of the last row of this one cannot be computed: the argument of
    // abs() is the lowest integer there.
    const QString path = tempDir.filePath(QStringLiteral("unreadable.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(
        path, QString(),
        QStringLiteral("(13, 0, 0, x'%1'), (13, 1, 0, x'%1'), (13, 2, 99, x'%1')").arg(kTileCachePngHex)));
    QVERIFY(tileCacheExec(path, QStringLiteral("ALTER TABLE tiles RENAME TO pictures")));
    QVERIFY(tileCacheExec(
        path, QStringLiteral("CREATE VIEW tiles AS SELECT zoom_level, tile_column, tile_row, CASE WHEN tile_row = 99 "
                             "THEN abs(-9223372036854775807 - (tile_row = 99)) ELSE tile_data END AS tile_data "
                             "FROM pictures")));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("import MBTiles.*integer overflow")));
    const TileCacheResult result =
        db->importMBTiles(path, tileCacheMBTilesSet(QStringLiteral("unreadable")), /*replace=*/false, nullptr);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);

    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT name FROM TileSets")),
             QStringList{QStringLiteral("Default Tile Set")});
    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{QStringLiteral("0")});
}

// As for an MBTiles file: the chunks of a set that were committed before a chunk failed go with the set
void TileCacheDatabaseTest::_testImportSetsMergeThatFailsLeavesNoSet()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);
    quint64 setID = 0;
    _insertTileSet(dbSrc.get(), QStringLiteral("Large"), setID);
    {
        QSqlQuery query(dbSrc->database());
        QVERIFY(
            query.exec(QStringLiteral("WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM n WHERE i < 2500) "
                                      "INSERT INTO Tiles(hash, format, tile, size, type, date) "
                                      "SELECT 'large_' || i, 'png', CAST('exported' AS BLOB), 8, 0, i FROM n")));
        QVERIFY(
            query.exec(QStringLiteral("INSERT INTO SetTiles(tileID, setID) SELECT tileID, %1 FROM Tiles").arg(setID)));
    }
    const QString exportPath = tempDir.filePath("large_export.db");
    QVERIFY(dbSrc->exportSets({tileCacheSetRecordOf(*dbSrc, setID)}, exportPath, nullptr).success);
    dbSrc.reset();

    // The cache refuses a tile of the second chunk
    const QString targetPath = tempDir.filePath("target.db");
    auto dbTgt = std::make_unique<TileCacheDatabase>(targetPath);
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());
    QVERIFY(tileCacheExec(targetPath,
                          QStringLiteral("CREATE TRIGGER refuse_tile BEFORE INSERT ON Tiles "
                                         "WHEN NEW.hash = 'large_2400' BEGIN SELECT RAISE(ABORT, 'refused'); END")));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("copy tiles of set")));
    const TileCacheResult result = dbTgt->importSetsMerge(exportPath, nullptr);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);

    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT name FROM TileSets")),
             QStringList{QStringLiteral("Default Tile Set")});
    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT COUNT(*) FROM Tiles")), QStringList{QStringLiteral("0")});
}

// A file with a set that is not whole would look complete to who imports it
void TileCacheDatabaseTest::_testExportThatFailsLeavesNoFile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Twice"), setID);
    QVERIFY(
        db->saveTile(QStringLiteral("tile"), QStringLiteral("png"), QByteArray(10, 'T'), kTileCacheTestMapId, setID));

    // A file cannot have two sets of one name: the first is exported, and the second fails
    const TileSetRecord set = tileCacheSetRecordOf(*db, setID);
    const QString exportPath = tempDir.filePath("failed_export.db");
    const TileCacheResult result = db->exportSets({set, set}, exportPath, nullptr);
    QVERIFY(!result.success);
    QVERIFY(!QFileInfo::exists(exportPath));
    QVERIFY(!QFileInfo::exists(exportPath + QStringLiteral("-wal")));

    // The cache has let go of the file: it exports again
    QVERIFY(db->exportSets({set}, exportPath, nullptr).success);
}

void TileCacheDatabaseTest::_testSaveTileLinksToDifferentSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    quint64 setA = 0, setB = 0;
    _insertTileSet(db.get(), QStringLiteral("SetA"), setA);
    _insertTileSet(db.get(), QStringLiteral("SetB"), setB);

    const QByteArray data(10, 'A');
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, kTileCacheTestMapId, setA));
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, kTileCacheTestMapId, setB));

    auto tile = db->getTile(QStringLiteral("shared_hash"));
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->img, data);

    // One tile, of both sets
    QCOMPARE(tileCacheRows(tempDir.filePath(QStringLiteral("tiles.db")),
                           QStringLiteral("SELECT COUNT(DISTINCT tileID), COUNT(*) FROM SetTiles")),
             QStringList{QStringLiteral("1|2")});
}

void TileCacheDatabaseTest::_testExportImportNoLingeringConnections()
{
    QTemporaryDir tempDir;
    auto db1 = _createInitializedDB(tempDir);

    const QByteArray data(20, 'L');
    QVERIFY(db1->saveTile(QStringLiteral("linger1"), QStringLiteral("png"), data, kTileCacheTestMapId,
                          TileCacheDatabase::kInvalidTileSet));

    const auto sets = db1->getTileSets();
    QVERIFY(!sets.isEmpty());
    TileSetRecord rec = sets[0];
    rec.numTiles = 1;

    const QString exportPath = tempDir.filePath("linger_export.db");

    const TileCacheResult exp1 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp1.success);

    const TileCacheResult exp2 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp2.success);

    auto db2 = std::make_unique<TileCacheDatabase>(tempDir.filePath("linger_db2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());
    const TileCacheResult imp1 = db2->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp1.success);

    auto db3 = std::make_unique<TileCacheDatabase>(tempDir.filePath("linger_db3.db"));
    QVERIFY(db3->init());
    QVERIFY(db3->connectDB());
    const TileCacheResult imp2 = db3->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp2.success);
}

void TileCacheDatabaseTest::_testCreateTileSet()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto setID = db->createTileSet(
        tileCacheSetRecord(QStringLiteral("Test Set"), QStringLiteral("TestMap"), 37.0, -122.0, 36.0, -121.0, 5, 100),
        QString());

    QVERIFY(setID.has_value());
    QVERIFY(setID.value() != 0);

    const auto foundID = db->findTileSetID(QStringLiteral("Test Set"));
    QVERIFY(foundID.has_value());
    QCOMPARE(foundID.value(), setID.value());

    const TileSetRecord set = tileCacheSetRecordOf(*db, setID.value());
    QCOMPARE(set.setID, setID.value());
    QCOMPARE(set.name, QStringLiteral("Test Set"));
    QCOMPARE(set.minZoom, 5);
    QCOMPARE(set.maxZoom, 5);
    QCOMPARE(set.mapId, kTileCacheTestMapId);
}

void TileCacheDatabaseTest::_testDeleteDefaultSetInvalidatesCache()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const auto defaultID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultID.has_value());
    // The database remembers the default set from the first tile it saves in it
    QVERIFY(db->saveTile(QStringLiteral("saved before"), QStringLiteral("png"), QByteArray(10, 'B'),
                         kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));

    QVERIFY(db->deleteTileSet(defaultID.value()));

    // After deleting default set, saveTile with kInvalidTileSet should fail gracefully
    // (no default set to link to), not crash or use stale ID
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Default tile set not found in database")));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Cannot save tile: no valid tile set")));
    QVERIFY(!db->saveTile(QStringLiteral("orphan"), QStringLiteral("png"), QByteArray(10, 'O'), kTileCacheTestMapId,
                          TileCacheDatabase::kInvalidTileSet));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void TileCacheDatabaseTest::_testPruneCacheMultipleBatches()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    // Save 200 tiles of 100 bytes each = 20000 bytes total
    for (int i = 0; i < 200; i++) {
        QVERIFY(db->saveTile(QStringLiteral("prune_multi_%1").arg(i), QStringLiteral("png"), QByteArray(100, 'P'),
                             kTileCacheTestMapId, TileCacheDatabase::kInvalidTileSet));
    }

    const TileCacheTotals before = db->computeTotals();
    QCOMPARE(before.totalCount, static_cast<quint32>(200));

    // Prune 15000 bytes — requires more than one batch of 128
    QVERIFY(db->pruneCache(15000));

    const TileCacheTotals after = db->computeTotals();
    QVERIFY(after.totalCount < 200);
    QVERIFY(after.totalSize <= 5000);
}

void TileCacheDatabaseTest::_testSchemaVersionResetsLegacyDB()
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
    TileCacheDatabase db(path);
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
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
    QCOMPARE(query.value(0).toInt(), TileCacheDatabase::kSchemaVersion);

    // Default tile set should be recreated
    const auto sets = db.getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
}

void TileCacheDatabaseTest::_testMigrateVersion1RenumbersProviders()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());
    const QStringList setTiles = tileCacheRows(path, kTileCacheSetTilesSql);
    QCOMPARE(setTiles.size(), 5);

    TileCacheDatabase db(path);
    db.setLegacyMapIds(shiftedLegacyMapIds());
    QVERIFY(db.init());

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), renumberedTileRows());
    QCOMPARE(tileCacheRows(path, kTileCacheSetsSql),
             (QStringList{QStringLiteral("1|Default Tile Set|-1"), QStringLiteral("2|Set of 1|6"),
                          QStringLiteral("3|Set of 6|11")}));
    QCOMPARE(tileCacheRows(path, kTileCacheSetTilesSql), setTiles);
    QCOMPARE(tileCacheRows(path, kTileCacheDownloadsSql),
             (QStringList{QStringLiteral("2|%1|6|11|21|5|0").arg(tileCacheHash(6, 11, 21, 5)),
                          QStringLiteral("2|%1|11|11|21|5|2").arg(tileCacheHash(11, 11, 21, 5))}));
}

void TileCacheDatabaseTest::_testMigrateVersion1KeepsPermanentIds()
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

    TileCacheDatabase db(path);
    QVERIFY(db.init());

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});
    for (qsizetype i = 0; i < queries.size(); i++) {
        QCOMPARE(tileCacheRows(path, queries[i]), rows[i]);
    }
}

void TileCacheDatabaseTest::_testMigrateFailureResetsCache_data()
{
    QTest::addColumn<int>("legacyId");
    QTest::addColumn<int>("mapId");
    QTest::addColumn<QString>("warning");

    // Renumbered, the tile of provider 1 has the key of a tile that stays where it is
    QTest::newRow("key that another tile has") << 1 << 6 << QStringLiteral("Failed to renumber provider 1 to 6");
    // The ids are renumbered from the highest down, which keeps a renumbered row apart from those still to renumber
    // only while an id grows
    QTest::newRow("lower id") << 6 << 1 << QStringLiteral("Cannot renumber provider 6 to the lower id 1");
}

void TileCacheDatabaseTest::_testMigrateFailureResetsCache()
{
    QFETCH(int, legacyId);
    QFETCH(int, mapId);
    QFETCH(QString, warning);

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    TileCacheDatabase db(path);
    db.setLegacyMapIds({{legacyId, mapId}});
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg, QRegularExpression(warning));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to migrate schema version 1 .* Resetting cache")));
    QVERIFY(db.init());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), QStringList());
    QCOMPARE(tileCacheRows(path, kTileCacheSetsSql), QStringList{QStringLiteral("1|Default Tile Set|-1")});
}

void TileCacheDatabaseTest::_testResetFailureFailsInit()
{
    // A cache of an unknown version is reset. Here its tiles cannot be dropped: a table that is not of the cache
    // refers to one of them.
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("newer.db");
    const QString newerVersion = QString::number(TileCacheDatabase::kSchemaVersion + 1);
    const QStringList rows =
        version1Rows() +
        QStringList{
            QStringLiteral("CREATE TABLE TileNotes (tileID INTEGER NOT NULL REFERENCES Tiles(tileID))"),
            QStringLiteral("INSERT INTO TileNotes(tileID) VALUES(1)"),
            QStringLiteral("PRAGMA user_version = %1").arg(newerVersion),
        };
    QCOMPARE(createVersion1TileCache(path, rows), QString());

    TileCacheDatabase db(path);
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unknown schema version")));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to drop tables")));
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!db.isValid());

    // The tiles that are left are not marked as tiles of the current version
    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql), QStringList{newerVersion});
}

// A cache whose default set cannot be looked up is no cache: it is removed, with the files SQLite keeps next to it
void TileCacheDatabaseTest::_testDefaultSetLookupFailureRemovesCache()
{
    // The tile sets are a view here, whose name cannot be computed: the argument of abs() is the lowest integer
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath(QStringLiteral("tiles.db"));
    QVERIFY(tileCacheExec(path, QStringLiteral("CREATE TABLE Notes (n INTEGER)")));
    QVERIFY(tileCacheExec(path, QStringLiteral("INSERT INTO Notes VALUES (1)")));
    QVERIFY(tileCacheExec(path, QStringLiteral("CREATE VIEW TileSets AS SELECT n AS setID, "
                                               "abs(-9223372036854775807 - (n = 1)) AS name FROM Notes")));

    TileCacheDatabase db(path);
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Looking for default tile set.*integer overflow")));
    QVERIFY(!db.init());
    verifyExpectedLogMessage();
    QVERIFY(db.hasFailed());
    QVERIFY(!db.isValid());

    for (const QString& suffix : {QString(), QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        QVERIFY2(!QFileInfo::exists(path + suffix), qPrintable(path + suffix));
    }
}

void TileCacheDatabaseTest::_testImportVersion1_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<bool>("afterDeadImport");
    QTest::newRow("replace") << true << false;
    QTest::newRow("merge") << false << false;
    QTest::newRow("merge after an import that died") << false << true;
}

void TileCacheDatabaseTest::_testImportVersion1()
{
    QFETCH(bool, replace);
    QFETCH(bool, afterDeadImport);

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(importPath, version1Rows()), QString());
    const QStringList importedTiles = tileCacheRows(importPath, kTileCacheTilesSql);

    const QString path = tempDir.filePath("tiles.db");
    TileCacheDatabase db(path);
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

    const TileCacheResult result =
        replace ? db.importSetsReplace(importPath, nullptr) : db.importSetsMerge(importPath, nullptr);
    QVERIFY2(result.success, qPrintable(result.errorString));
    db.disconnectDB();

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});
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

void TileCacheDatabaseTest::_testImportRefusesVersion_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<int>("version");
    QTest::addColumn<QString>("error");
    const int newerVersion = TileCacheDatabase::kSchemaVersion + 1;
    QTest::newRow("merge, version 0") << false << 0 << QStringLiteral("too old");
    QTest::newRow("merge, newer version") << false << newerVersion << QStringLiteral("newer version");
    QTest::newRow("replace, version 0") << true << 0 << QStringLiteral("too old");
    QTest::newRow("replace, newer version") << true << newerVersion << QStringLiteral("newer version");
}

void TileCacheDatabaseTest::_testImportRefusesVersion()
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
    QVERIFY(db->saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));

    const TileCacheResult result =
        replace ? db->importSetsReplace(importPath, nullptr) : db->importSetsMerge(importPath, nullptr);
    QVERIFY(!result.success);
    QVERIFY2(result.errorString.contains(error), qPrintable(result.errorString));
    // The cache has the tile it had, and no tile of the file
    QVERIFY(db->findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db->computeTotals().totalCount, 1U);
    // Refused before the file is copied to be migrated
    QCOMPARE(QDir(tempDir.path()).entryList({QStringLiteral("*.import*")}), QStringList());
}

void TileCacheDatabaseTest::_testImportMigrateFailureKeepsCache_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<QString>("error");
    QTest::newRow("replace") << true << QStringLiteral("Failed to initialize tile cache database after import");
    QTest::newRow("merge") << false << QStringLiteral("Failed to migrate import database");
}

void TileCacheDatabaseTest::_testImportMigrateFailureKeepsCache()
{
    QFETCH(bool, replace);
    QFETCH(QString, error);

    // Renumbered, the tile of provider 1 of the file has the key of a tile that stays where it is
    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(importPath, version1Rows()), QString());

    TileCacheDatabase db(tempDir.filePath("tiles.db"));
    db.setLegacyMapIds({{1, 6}});
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(db.saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kTileCacheTestMapId,
                        TileCacheDatabase::kInvalidTileSet));

    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to renumber provider 1 to 6")));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to migrate schema version 1 to")));
    const TileCacheResult result =
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

void TileCacheDatabaseTest::_testImportReplaceThatCannotSwapKeepsCache_data()
{
    QTest::addColumn<bool>("readOnlyDirectory");
    QTest::addColumn<QString>("takenPath");
    QTest::addColumn<QString>("error");
    // Neither the copy of the file can be written, nor the cache moved aside
    QTest::newRow("read-only directory") << true << QString() << QStringLiteral("Failed to copy import database");
    QTest::newRow("copy path taken by a directory")
        << false << QStringLiteral("tiles.db.import") << QStringLiteral("Failed to copy import database");
    // The cache cannot be moved aside, but could be removed
    QTest::newRow("backup path taken by a directory")
        << false << QStringLiteral("tiles.db.bak") << QStringLiteral("Failed to back up tile cache database");
}

// The cache is replaced only by a file that was copied next to it, and only when it can be moved aside: an import
// that failed would have nothing to put back
void TileCacheDatabaseTest::_testImportReplaceThatCannotSwapKeepsCache()
{
    QFETCH(bool, readOnlyDirectory);
    QFETCH(QString, takenPath);
    QFETCH(QString, error);

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(importPath, version1Rows()), QString());

    const QDir cacheDir(tempDir.filePath("cache"));
    QVERIFY(cacheDir.mkpath(QStringLiteral(".")));
    TileCacheDatabase db(cacheDir.filePath("tiles.db"));
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(db.saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kTileCacheTestMapId,
                        TileCacheDatabase::kInvalidTileSet));

    const QFile::Permissions permissions = QFile::permissions(cacheDir.path());
    const auto restorePermissions = qScopeGuard([&] { (void) QFile::setPermissions(cacheDir.path(), permissions); });
    if (readOnlyDirectory) {
        QVERIFY(QFile::setPermissions(cacheDir.path(), QFile::ReadOwner | QFile::ExeOwner));
        if (QFile(cacheDir.filePath("probe")).open(QIODevice::WriteOnly)) {
            QSKIP("This user can write to a read-only directory");
        }
    } else {
        QVERIFY(cacheDir.mkdir(takenPath));
    }

    const TileCacheResult result = db.importSetsReplace(importPath, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, error);

    // The cache is open and has the tile it had, and no tile of the file
    QVERIFY(db.findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db.computeTotals().totalCount, 1U);
    // The copy of the file is not left
    QCOMPARE(cacheDir.entryList({QStringLiteral("*.import*")}, QDir::Files), QStringList());
}

// Neither the file nor the cache has a path to compare: the file is not the cache for that
void TileCacheDatabaseTest::_testImportReplaceOfMissingFileIntoMissingCacheSaysFileCannotBeOpened()
{
    QTemporaryDir tempDir;
    TileCacheDatabase db(tempDir.filePath("tiles.db"));

    const TileCacheResult result = db.importSetsReplace(tempDir.filePath("missing.db"), nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, QStringLiteral("Error opening import database"));
}

void TileCacheDatabaseTest::_testImportRefusesWorkingFileOfCache_data()
{
    QTest::addColumn<bool>("replace");
    QTest::addColumn<QString>("suffix");
    QTest::newRow("replace, backup of the cache") << true << QStringLiteral(".bak");
    QTest::newRow("replace, copy of an import") << true << QStringLiteral(".import");
    QTest::newRow("merge, copy of an import") << false << QStringLiteral(".import");
}

// An import removes its working files before it writes them, and when it is done: one of them, as what an import
// that died left, would be removed before it is read, or after
void TileCacheDatabaseTest::_testImportRefusesWorkingFileOfCache()
{
    QFETCH(bool, replace);
    QFETCH(QString, suffix);

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    const QString importPath = path + suffix;
    {
        TileCacheDatabase left(importPath);
        QVERIFY(left.init());
        QVERIFY(left.connectDB());
        QVERIFY(left.saveTile(QStringLiteral("left"), QStringLiteral("png"), QByteArray(8, 'L'), kTileCacheTestMapId,
                              TileCacheDatabase::kInvalidTileSet));
    }
    const QStringList leftTiles = tileCacheRows(importPath, kTileCacheTilesSql);
    QCOMPARE(leftTiles.size(), 1);

    TileCacheDatabase db(path);
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(db.saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kTileCacheTestMapId,
                        TileCacheDatabase::kInvalidTileSet));

    const TileCacheResult result =
        replace ? db.importSetsReplace(importPath, nullptr) : db.importSetsMerge(importPath, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString,
             QStringLiteral("Import path must differ from the working files of the active database"));

    // The file is as it was, and so is the cache
    QCOMPARE(tileCacheRows(importPath, kTileCacheTilesSql), leftTiles);
    QVERIFY(db.findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db.computeTotals().totalCount, 1U);
}

// The cache is no file to import, which would be replaced by itself or read while it is written, and none to export to
void TileCacheDatabaseTest::_testImportAndExportRefuseTheCacheItself()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    QVERIFY(db->saveTile(QStringLiteral("kept"), QStringLiteral("png"), QByteArray(8, 'K'), kTileCacheTestMapId,
                         TileCacheDatabase::kInvalidTileSet));
    const QString importError = QStringLiteral("Import path must differ from the active database");

    TileCacheResult result = db->importSetsReplace(cachePath, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, importError);

    // By another spelling of its path
    result = db->importSetsMerge(tempDir.filePath(QStringLiteral("./tiles.db")), nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, importError);

    result = db->exportSets({db->getTileSets().constFirst()}, cachePath, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, QStringLiteral("Export path must differ from the active database"));

    QVERIFY(db->findTile(QStringLiteral("kept")).has_value());
    QCOMPARE(db->computeTotals().totalCount, 1U);
}

// The tiles of the default set of a file go to that of the cache, which has them after the first import: the second
// one has nothing to add, and says so
void TileCacheDatabaseTest::_testImportSetsMergeOfNothingNewFails()
{
    QTemporaryDir tempDir;
    auto dbSrc = _createInitializedDB(tempDir);
    QVERIFY(dbSrc->saveTile(QStringLiteral("exported"), QStringLiteral("png"), QByteArray(8, 'E'), kTileCacheTestMapId,
                            TileCacheDatabase::kInvalidTileSet));
    const QString exportPath = tempDir.filePath(QStringLiteral("export.db"));
    QVERIFY(dbSrc->exportSets({dbSrc->getTileSets().constFirst()}, exportPath, nullptr).success);
    dbSrc.reset();

    const QString targetPath = tempDir.filePath(QStringLiteral("target.db"));
    TileCacheDatabase db(targetPath);
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(db.importSetsMerge(exportPath, nullptr).success);

    const TileCacheResult result = db.importSetsMerge(exportPath, nullptr);
    QVERIFY(!result.success);
    QCOMPARE(result.errorString, QStringLiteral("No unique tiles in imported database"));
    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT hash FROM Tiles")),
             QStringList{QStringLiteral("exported")});
    QCOMPARE(tileCacheRows(targetPath, QStringLiteral("SELECT name FROM TileSets")),
             QStringList{QStringLiteral("Default Tile Set")});
}

void TileCacheDatabaseTest::_testVersion2TilesGetCacheVariant_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("cache opened") << QStringLiteral("open");
    QTest::newRow("import replacing") << QStringLiteral("replace");
    QTest::newRow("import merging") << QStringLiteral("merge");
}

void TileCacheDatabaseTest::_testVersion2TilesGetCacheVariant()
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
    TileCacheDatabase db(path);
    if (how == QStringLiteral("open")) {
        db.setCacheVariants(cacheVariants);
        QVERIFY(db.init());
    } else {
        // The variants of an import are those set by the time of the import
        QVERIFY(db.init());
        QVERIFY(db.connectDB());
        db.setCacheVariants(cacheVariants);
        const TileCacheResult result = (how == QStringLiteral("replace")) ? db.importSetsReplace(version2Path, nullptr)
                                                                          : db.importSetsMerge(version2Path, nullptr);
        QVERIFY2(result.success, qPrintable(result.errorString));
        db.disconnectDB();
    }

    QCOMPARE(tileCacheRows(path, kTileCacheVersionSql),
             QStringList{QString::number(TileCacheDatabase::kSchemaVersion)});
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), tiles);
    if (how != QStringLiteral("merge")) {
        // A merge takes the tiles of the sets, and leaves what the sets still had to download
        QCOMPARE(tileCacheRows(path, kTileCacheSetTilesSql), setTiles);
        QCOMPARE(tileCacheRows(path, kTileCacheDownloadsSql), downloads);
    }
}

void TileCacheDatabaseTest::_testVersion1TilesGetCacheVariantOfPermanentId()
{
    // The cache variants are by permanent provider id: tile 3 gets that of 41, which it is renumbered to from 36
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    TileCacheDatabase db(path);
    db.setLegacyMapIds(shiftedLegacyMapIds());
    db.setCacheVariants({{36, QStringLiteral("alice/night")}, {41, QStringLiteral("alice/streets")}});
    QVERIFY(db.init());

    QStringList tiles = renumberedTileRows();
    tiles[2] = QStringLiteral("3|%1-52f3eade|41|C3").arg(tileCacheHash(41, 3, 4, 2));
    QCOMPARE(tileCacheRows(path, kTileCacheTilesSql), tiles);
}

void TileCacheDatabaseTest::_testCreateTileSetKeysTilesByCacheVariant()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    const QString type = kTileCacheCustomUrlType;
    const QString sourceA = kTileCacheTestSourceA;
    const QString sourceB = kTileCacheTestSourceB;

    // A region of one tile, which the cache has from source A
    const SharedMapProvider provider = tileCacheCustomUrlProvider();
    const TileRegion region = provider->getTileCount(5, -122.0, 37.0, -121.0, 36.0);
    QCOMPARE(region.tileCount, 1U);
    const QString hashA = provider->getTileHash(region.tileX0, region.tileY0, 5, sourceA);
    const QString hashB = provider->getTileHash(region.tileX0, region.tileY0, 5, sourceB);
    QVERIFY(db->saveTile(hashA, QStringLiteral("png"), QByteArray("tile"), provider->mapId(),
                         TileCacheDatabase::kInvalidTileSet));

    const auto tilesOfSet = [&db, &provider](quint64 setID) {
        return db->computeSetTotals(setID, false, 1, provider->mapId()).savedTileCount;
    };
    const TileSetRecord record =
        tileCacheSetRecord(QString(), type, 37.0, -122.0, 36.0, -121.0, 5, 1, provider->mapId());

    // A set of source B has to download the tile, under the key of source B
    TileSetRecord recordB = record;
    recordB.name = QStringLiteral("Set B");
    const auto setB = db->createTileSet(recordB, sourceB);
    QVERIFY(setB.has_value());
    QCOMPARE(tilesOfSet(*setB), 0U);
    const std::optional<QList<DownloadTile>> downloadsB = db->getTileDownloadList(*setB, 10);
    QVERIFY(downloadsB.has_value());
    QCOMPARE(downloadsB->size(), 1);
    QCOMPARE(downloadsB->first().hash, hashB);

    // A set of source A has the tile already
    TileSetRecord recordA = record;
    recordA.name = QStringLiteral("Set A");
    const auto setA = db->createTileSet(recordA, sourceA);
    QVERIFY(setA.has_value());
    QCOMPARE(tilesOfSet(*setA), 1U);
    // Nothing to download, which is no failure
    const std::optional<QList<DownloadTile>> downloadsA = db->getTileDownloadList(*setA, 10);
    QVERIFY(downloadsA.has_value());
    QVERIFY(downloadsA->isEmpty());
}

// The tiles that the cache has already are looked up some hundreds at a time: none of a set with more tiles than
// that is left out, or is to download although the cache has it
void TileCacheDatabaseTest::_testCreateTileSetOfMoreTilesThanALookup()
{
    constexpr int kLookupTiles = 500;
    constexpr int kZoom = 10;
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));

    // The columns of the region one after the other, each from its first row to its last
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(kTileCacheTestMapId);
    const TileRegion region = provider->getTileCount(kZoom, 0.0, 10.0, 10.0, 0.0);
    const int rows = region.tileY1 - region.tileY0 + 1;
    const int tileCount = static_cast<int>(region.tileCount);
    QCOMPARE_GT(tileCount, kLookupTiles + 1);

    // The cache has the first and the last tile, and those where the first lookup ends and the second one begins
    const QList<int> cached = {0, kLookupTiles - 1, kLookupTiles, tileCount - 1};
    for (const int index : cached) {
        const QString hash =
            provider->getTileHash(region.tileX0 + (index / rows), region.tileY0 + (index % rows), kZoom, QString());
        QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray(8, 'C'), kTileCacheTestMapId,
                             TileCacheDatabase::kInvalidTileSet));
    }

    const auto setID = db->createTileSet(tileCacheSetRecord(QStringLiteral("Large"), kTileCacheTestProviderType, 10.0,
                                                            0.0, 0.0, 10.0, kZoom, static_cast<quint32>(tileCount)),
                                         QString());
    QVERIFY(setID.has_value());

    QCOMPARE(tileCacheRows(cachePath, QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = %1").arg(*setID)),
             QStringList{QString::number(cached.size())});
    const QString toDownload = QString::number(tileCount - cached.size());
    QCOMPARE(
        tileCacheRows(
            cachePath,
            QStringLiteral("SELECT COUNT(*), COUNT(DISTINCT hash) FROM TilesDownload WHERE setID = %1").arg(*setID)),
        QStringList{toDownload + QLatin1Char('|') + toDownload});
    QCOMPARE(tileCacheRows(cachePath,
                           QStringLiteral("SELECT COUNT(*) FROM TilesDownload D JOIN Tiles T ON T.hash = D.hash")),
             QStringList{QStringLiteral("0")});
}

void TileCacheDatabaseTest::_testCreateTileSetAcrossAntimeridian()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);

    // Zoom 3 has 8 columns: the region covers the last one and the first one, and the rows 3 and 4
    const auto setID = db->createTileSet(tileCacheSetRecord(QStringLiteral("Antimeridian"), kTileCacheTestProviderType,
                                                            10.0, 170.0, -10.0, -170.0, 3, 4),
                                         QString());
    QVERIFY(setID.has_value());

    const std::optional<QList<DownloadTile>> downloads = db->getTileDownloadList(*setID, 10);
    QVERIFY(downloads.has_value());
    QStringList hashes;
    for (const DownloadTile& tile : *downloads) {
        hashes.append(tile.hash);
    }
    hashes.sort();

    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(kTileCacheTestMapId);
    QStringList expected;
    for (const int x : {0, 7}) {
        for (const int y : {3, 4}) {
            expected.append(provider->getTileHash(x, y, 3, QString()));
        }
    }
    expected.sort();
    QCOMPARE(hashes, expected);
}

void TileCacheDatabaseTest::_testFreePagesReleasedInBatches_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("tile set deleted") << QStringLiteral("delete set");
    QTest::newRow("cache pruned") << QStringLiteral("prune");
    QTest::newRow("cache of unknown version reset") << QStringLiteral("unknown version");
}

void TileCacheDatabaseTest::_testFreePagesReleasedInBatches()
{
    QFETCH(QString, how);

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    auto db = std::make_unique<TileCacheDatabase>(path);
    QVERIFY(db->init());
    QVERIFY(db->connectDB());

    // A tile set deleted takes its tiles with it. The default set is pruned of tiles no other set has.
    quint64 setID = TileCacheDatabase::kInvalidTileSet;
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
            QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = %1").arg(TileCacheDatabase::kSchemaVersion + 1)));
        }
        db = std::make_unique<TileCacheDatabase>(path);
        expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
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

void TileCacheDatabaseTest::_testFreePagesKeptWithoutAutoVacuum()
{
    // A file from before auto-vacuum cannot give pages back. Its unused pages are for new rows, and there is
    // nothing left to do about them.
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());

    TileCacheDatabase db(path);
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

void TileCacheDatabaseTest::_testResetGivesSpaceBack()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    TileCacheDatabase db(path);
    QVERIFY(db.init());
    QVERIFY(db.connectDB());
    QVERIFY(saveLargeTiles(db, TileCacheDatabase::kInvalidTileSet));
    const qint64 filledSize = checkpointedFileSize(db, path);

    QVERIFY(db.resetDatabase());

    QCOMPARE(tileCacheFreePages(path), 0);
    // Without a checkpoint here: the user gets the space back with the reset, not when the cache is closed
    const qint64 resetSize = QFileInfo(path).size();
    QVERIFY2(resetSize < filledSize - kLargeTilesSize,
             qPrintable(QStringLiteral("%1 bytes of %2 left").arg(resetSize).arg(filledSize)));
}

void TileCacheDatabaseTest::_testResetGivesOldFileAutoVacuum()
{
    // A file from before auto-vacuum: the mode cannot be set once a file has tables
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("version1.db");
    QCOMPARE(createVersion1TileCache(path, version1Rows()), QString());
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("0")});

    // Opening and migrating the file does not rebuild it, which a large cache would take long for
    TileCacheDatabase db(path);
    QVERIFY(db.init());
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("0")});

    QVERIFY(db.connectDB());
    QVERIFY(db.resetDatabase());
    db.disconnectDB();
    QCOMPARE(tileCacheRows(path, kTileCacheAutoVacuumSql), QStringList{QStringLiteral("2")});
}

void TileCacheDatabaseTest::_testNewFileHasSchema_data()
{
    QTest::addColumn<QString>("sql");
    QTest::addColumn<QStringList>("rows");

    QTest::newRow("schema version") << kTileCacheVersionSql
                                    << QStringList{QString::number(TileCacheDatabase::kSchemaVersion)};
    // Incremental: the mode in which a file gives back the space of deleted tiles
    QTest::newRow("auto-vacuum") << kTileCacheAutoVacuumSql << QStringList{QStringLiteral("2")};
    QTest::newRow("tables") << QStringLiteral(
                                   "SELECT name FROM sqlite_master WHERE type = 'table' AND "
                                   "name NOT LIKE 'sqlite_%' ORDER BY name")
                            << QStringList({QStringLiteral("SetTiles"), QStringLiteral("TileSets"),
                                            QStringLiteral("Tiles"), QStringLiteral("TilesDownload")});
    QTest::newRow("indexes") << QStringLiteral(
                                    "SELECT name FROM sqlite_master WHERE type = 'index' AND "
                                    "sql IS NOT NULL ORDER BY name")
                             << QStringList({QStringLiteral("idx_settiles_setid"),
                                             QStringLiteral("idx_settiles_unique"), QStringLiteral("idx_tiles_date"),
                                             QStringLiteral("idx_tiles_size"),
                                             QStringLiteral("idx_tilesdownload_setid_hash"),
                                             QStringLiteral("idx_tilesdownload_setid_state")});

    // A column as its name, its type, whether it is NOT NULL, its default and whether it is the primary key
    const QString columnsSql =
        QStringLiteral("SELECT name, type, \"notnull\", dflt_value, pk FROM pragma_table_info('%1')");
    QTest::newRow("columns of Tiles") << columnsSql.arg(QStringLiteral("Tiles"))
                                      << QStringList(
                                             {QStringLiteral("tileID|INTEGER|1||1"), QStringLiteral("hash|TEXT|1||0"),
                                              QStringLiteral("format|TEXT|1||0"), QStringLiteral("tile|BLOB|0||0"),
                                              QStringLiteral("size|INTEGER|0||0"), QStringLiteral("type|INTEGER|0||0"),
                                              QStringLiteral("date|INTEGER|0|0|0")});
    QTest::newRow("columns of TileSets")
        << columnsSql.arg(QStringLiteral("TileSets"))
        << QStringList({QStringLiteral("setID|INTEGER|1||1"), QStringLiteral("name|TEXT|1||0"),
                        QStringLiteral("typeStr|TEXT|0||0"), QStringLiteral("topleftLat|REAL|0|0.0|0"),
                        QStringLiteral("topleftLon|REAL|0|0.0|0"), QStringLiteral("bottomRightLat|REAL|0|0.0|0"),
                        QStringLiteral("bottomRightLon|REAL|0|0.0|0"), QStringLiteral("minZoom|INTEGER|0|3|0"),
                        QStringLiteral("maxZoom|INTEGER|0|3|0"), QStringLiteral("type|INTEGER|0|-1|0"),
                        QStringLiteral("numTiles|INTEGER|0|0|0"), QStringLiteral("defaultSet|INTEGER|0|0|0"),
                        QStringLiteral("date|INTEGER|0|0|0")});
    QTest::newRow("columns of SetTiles") << columnsSql.arg(QStringLiteral("SetTiles"))
                                         << QStringList({QStringLiteral("setID|INTEGER|1||0"),
                                                         QStringLiteral("tileID|INTEGER|1||0")});
    QTest::newRow("columns of TilesDownload")
        << columnsSql.arg(QStringLiteral("TilesDownload"))
        << QStringList({QStringLiteral("setID|INTEGER|1||0"), QStringLiteral("hash|TEXT|1||0"),
                        QStringLiteral("type|INTEGER|0||0"), QStringLiteral("x|INTEGER|0||0"),
                        QStringLiteral("y|INTEGER|0||0"), QStringLiteral("z|INTEGER|0||0"),
                        QStringLiteral("state|INTEGER|0|0|0")});
}

void TileCacheDatabaseTest::_testNewFileHasSchema()
{
    QFETCH(QString, sql);
    QFETCH(QStringList, rows);

    QTemporaryDir tempDir;
    QVERIFY(_createInitializedDB(tempDir));

    QCOMPARE(tileCacheRows(tempDir.filePath(QStringLiteral("tiles.db")), sql), rows);
}

// A cache of an earlier version has an index of the links by their tile, which the unique index of the links covers
void TileCacheDatabaseTest::_testInitDropsIndexThatAnotherCovers()
{
    QTemporaryDir tempDir;
    const QString cachePath = tempDir.filePath(QStringLiteral("tiles.db"));
    const QString indexSql = QStringLiteral("SELECT name FROM sqlite_master WHERE name = 'idx_settiles_tileid'");
    QVERIFY(_createInitializedDB(tempDir));
    QVERIFY(tileCacheExec(cachePath, QStringLiteral("CREATE INDEX idx_settiles_tileid ON SetTiles(tileID)")));
    QCOMPARE(tileCacheRows(cachePath, indexSql), QStringList{QStringLiteral("idx_settiles_tileid")});

    QVERIFY(_createInitializedDB(tempDir));
    QCOMPARE(tileCacheRows(cachePath, indexSql), QStringList());
}

UT_REGISTER_TEST(TileCacheDatabaseTest, TestLabel::Unit)
