#include "QGCTileCacheDatabaseTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "QGCCacheTile.h"
#include "QGCMapUrlEngine.h"
#include "QGCTile.h"
#include "QGCTileCacheDatabase.h"

std::unique_ptr<QGCTileCacheDatabase> QGCTileCacheDatabaseTest::_createInitializedDB()
{
    auto db = std::make_unique<QGCTileCacheDatabase>(tempPath("tiles.db"));
    if (!db->init() || !db->connectDB()) {
        return nullptr;
    }
    return db;
}

void QGCTileCacheDatabaseTest::_insertTileSet(QGCTileCacheDatabase *db, const QString &name, quint64 &outSetID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT INTO TileSets(name, date) VALUES(?, ?)"));
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    QVERIFY(query.exec());
    outSetID = query.lastInsertId().toULongLong();
}

void QGCTileCacheDatabaseTest::_insertDownloadRecord(QGCTileCacheDatabase *db, quint64 setID, const QString &hash, int state)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)"));
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
    QGCTileCacheDatabase db(tempPath("tiles.db"));
    QVERIFY(!db.isValid());
    QVERIFY(!db.hasFailed());

    QVERIFY(db.init());
    QVERIFY(db.isValid());
    QVERIFY(!db.hasFailed());
}

void QGCTileCacheDatabaseTest::_testInitWithEmptyPath()
{
    const QString emptyPath;
    QGCTileCacheDatabase db(emptyPath);
    QVERIFY(!db.init());
    QVERIFY(db.hasFailed());
    QVERIFY(!db.isValid());
}

void QGCTileCacheDatabaseTest::_testConnectDisconnect()
{
    QGCTileCacheDatabase db(tempPath("tiles.db"));
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
    auto db = _createInitializedDB();
    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0].defaultSet);
    QCOMPARE(sets[0].name, QStringLiteral("Default Tile Set"));
}

void QGCTileCacheDatabaseTest::_testSaveTileAndGetTile()
{
    auto db = _createInitializedDB();

    const QString hash = QStringLiteral("test_hash_1234");
    const QString format = QStringLiteral("png");
    const QByteArray img("fake_tile_data_bytes");
    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    const QString type = providerTypes.first();

    QVERIFY(db->saveTile(hash, format, img, type, QGCTileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(hash);
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->hash, hash);
    QCOMPARE(tile->format, format);
    QCOMPARE(tile->img, img);
    QCOMPARE(tile->type, type);
}

void QGCTileCacheDatabaseTest::_testGetTileNotFound()
{
    auto db = _createInitializedDB();
    auto tile = db->getTile(QStringLiteral("nonexistent_hash"));
    QVERIFY(tile == nullptr);
}

void QGCTileCacheDatabaseTest::_testFindTile()
{
    auto db = _createInitializedDB();

    const QString hash = QStringLiteral("findme_hash");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto id = db->findTile(hash);
    QVERIFY(id.has_value());
    QVERIFY(id.value() != 0);

    const auto missing = db->findTile(QStringLiteral("no_such_hash"));
    QVERIFY(!missing.has_value());
}

void QGCTileCacheDatabaseTest::_testGetTileSetsMultiple()
{
    auto db = _createInitializedDB();

    quint64 customSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("My Custom Set"), customSetID);
    QVERIFY(customSetID != 0);

    const auto sets = db->getTileSets();
    QCOMPARE(sets.size(), 2);

    bool foundDefault = false;
    bool foundCustom = false;
    for (const auto &rec : sets) {
        if (rec.defaultSet) foundDefault = true;
        if (rec.name == QStringLiteral("My Custom Set")) foundCustom = true;
    }
    QVERIFY(foundDefault);
    QVERIFY(foundCustom);
}

void QGCTileCacheDatabaseTest::_testRenameTileSet()
{
    auto db = _createInitializedDB();

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("Original Name"), setID);

    QVERIFY(db->renameTileSet(setID, QStringLiteral("New Name")));

    const auto foundID = db->findTileSetID(QStringLiteral("New Name"));
    QVERIFY(foundID.has_value());
    QCOMPARE(foundID.value(), setID);
}

void QGCTileCacheDatabaseTest::_testFindTileSetID()
{
    auto db = _createInitializedDB();

    const auto foundID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(foundID.has_value());
    QVERIFY(foundID.value() != 0);

    const auto missingID = db->findTileSetID(QStringLiteral("No Such Set"));
    QVERIFY(!missingID.has_value());
}

void QGCTileCacheDatabaseTest::_testDeleteTileSet()
{
    auto db = _createInitializedDB();

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
    auto db = _createInitializedDB();

    QVERIFY(db->saveTile(QStringLiteral("h1"), QStringLiteral("png"), QByteArray("d1"), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
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
    auto db = _createInitializedDB();

    const QByteArray data10(10, 'A');
    const QByteArray data20(20, 'B');
    QVERIFY(db->saveTile(QStringLiteral("ct1"), QStringLiteral("png"), data10, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("ct2"), QStringLiteral("png"), data20, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const TotalsResult totals = db->computeTotals();
    QCOMPARE(totals.totalCount, static_cast<quint32>(2));
    QCOMPARE(totals.totalSize, static_cast<quint64>(30));
    QCOMPARE(totals.defaultCount, static_cast<quint32>(2));
    QCOMPARE(totals.defaultSize, static_cast<quint64>(30));
}

void QGCTileCacheDatabaseTest::_testComputeSetTotalsDefault()
{
    auto db = _createInitializedDB();

    const QByteArray data(15, 'X');
    QVERIFY(db->saveTile(QStringLiteral("sd1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    const SetTotalsResult result = db->computeSetTotals(defaultSetID.value(), true, 0, QStringLiteral("T"));
    QCOMPARE(result.savedTileCount, static_cast<quint32>(1));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(15));
}

void QGCTileCacheDatabaseTest::_testPruneCache()
{
    auto db = _createInitializedDB();

    const QByteArray data(100, 'Z');
    QVERIFY(db->saveTile(QStringLiteral("pr1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("pr2"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    QVERIFY(db->findTile(QStringLiteral("pr1")).has_value());

    QVERIFY(db->pruneCache(200));

    QVERIFY(!db->findTile(QStringLiteral("pr1")).has_value());
    QVERIFY(!db->findTile(QStringLiteral("pr2")).has_value());
}

void QGCTileCacheDatabaseTest::_testUpdateTileDownloadState()
{
    auto db = _createInitializedDB();

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
    auto db = _createInitializedDB();

    const QByteArray data(50, 'E');
    QVERIFY(db->saveTile(QStringLiteral("exp1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("exp2"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto sets = db->getTileSets();
    QVERIFY(!sets.isEmpty());

    TileSetRecord defaultRec = sets[0];
    defaultRec.numTiles = 2;

    const QString exportPath = tempPath("export.db");
    int lastProgress = 0;
    auto progressCb = [&lastProgress](int p) { lastProgress = p; };

    const DatabaseResult exportResult = db->exportSets({defaultRec}, exportPath, progressCb);
    QVERIFY(exportResult.success);
    QVERIFY(lastProgress > 0);

    db.reset();

    auto db2 = std::make_unique<QGCTileCacheDatabase>(tempPath("tiles2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());

    QVERIFY(!db2->findTile(QStringLiteral("exp1")).has_value());

    const DatabaseResult importResult = db2->importSetsReplace(exportPath, nullptr);
    QVERIFY(importResult.success);
    QVERIFY(db2->isValid());

    QVERIFY(db2->findTile(QStringLiteral("exp1")).has_value());
    QVERIFY(db2->findTile(QStringLiteral("exp2")).has_value());
}

void QGCTileCacheDatabaseTest::_linkTileToSet(QGCTileCacheDatabase *db, quint64 tileID, quint64 setID)
{
    QSqlQuery query(db->database());
    QVERIFY(query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"));
    query.addBindValue(tileID);
    query.addBindValue(setID);
    QVERIFY(query.exec());
}

void QGCTileCacheDatabaseTest::_testGetTileDownloadList()
{
    auto db = _createInitializedDB();

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_a"), QGCTile::StatePending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_b"), QGCTile::StatePending);
    _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("dl_c"), QGCTile::StatePending);

    const QList<QGCTile> tiles = db->getTileDownloadList(defaultSetID.value(), 2);
    QCOMPARE(tiles.size(), 2);

    QStringList hashes;
    for (const auto &t : tiles) {
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
    auto dbSrc = _createInitializedDB();

    const QByteArray data(40, 'M');
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(dbSrc->saveTile(QStringLiteral("exp_m2"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto srcSets = dbSrc->getTileSets();
    QVERIFY(!srcSets.isEmpty());
    TileSetRecord srcRec = srcSets[0];
    srcRec.numTiles = 2;

    const QString exportPath = tempPath("merge_export.db");
    const DatabaseResult exportResult = dbSrc->exportSets({srcRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<QGCTileCacheDatabase>(tempPath("merge_target.db"));
    QVERIFY(dbTgt->init());
    QVERIFY(dbTgt->connectDB());

    QVERIFY(dbTgt->saveTile(QStringLiteral("trg_m1"), QStringLiteral("png"), QByteArray(25, 'N'), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

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
    auto db = _createInitializedDB();

    quint64 customSetID = 0;
    _insertTileSet(db.get(), QStringLiteral("Stats Set"), customSetID);
    QVERIFY(customSetID != 0);

    const QByteArray data20(20, 'A');
    const QByteArray data30(30, 'B');
    QVERIFY(db->saveTile(QStringLiteral("st1"), QStringLiteral("png"), data20, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("st2"), QStringLiteral("png"), data30, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto tile1 = db->findTile(QStringLiteral("st1"));
    const auto tile2 = db->findTile(QStringLiteral("st2"));
    QVERIFY(tile1.has_value());
    QVERIFY(tile2.has_value());

    _linkTileToSet(db.get(), tile1.value(), customSetID);
    _linkTileToSet(db.get(), tile2.value(), customSetID);

    const SetTotalsResult result = db->computeSetTotals(customSetID, false, 5, QStringLiteral("T"));
    QCOMPARE(result.savedTileCount, static_cast<quint32>(2));
    QCOMPARE(result.savedTileSize, static_cast<quint64>(50));
}

void QGCTileCacheDatabaseTest::_testSaveDuplicateTile()
{
    auto db = _createInitializedDB();

    const QByteArray data1(10, 'X');
    const QByteArray data2(10, 'Y');

    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("png"), data1, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    // Second save with same hash succeeds (links existing tile to same set via OR IGNORE)
    QVERIFY(db->saveTile(QStringLiteral("dup_hash"), QStringLiteral("png"), data2, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    auto tile = db->getTile(QStringLiteral("dup_hash"));
    QVERIFY(tile != nullptr);
    QCOMPARE(tile->img, data1);
}

void QGCTileCacheDatabaseTest::_testOperationsAfterDisconnect()
{
    auto db = _createInitializedDB();
    db->disconnectDB();

    QVERIFY(!db->saveTile(QStringLiteral("x"), QStringLiteral("png"), QByteArray("d"), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->getTile(QStringLiteral("x")) == nullptr);
    QVERIFY(!db->findTile(QStringLiteral("x")).has_value());
    QVERIFY(db->getTileSets().isEmpty());

    const TotalsResult totals = db->computeTotals();
    QCOMPARE(totals.totalCount, static_cast<quint32>(0));
    QCOMPARE(totals.totalSize, static_cast<quint64>(0));

    QVERIFY(!db->pruneCache(100));
    QVERIFY(!db->resetDatabase());
}

void QGCTileCacheDatabaseTest::_testPruneCacheCleansSetTiles()
{
    auto db = _createInitializedDB();

    const QByteArray data(100, 'P');
    QVERIFY(db->saveTile(QStringLiteral("ps1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("ps2"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

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
    auto db = _createInitializedDB();

    quint64 setID = 0;
    _insertTileSet(db.get(), QStringLiteral("CleanupSet"), setID);
    QVERIFY(setID != 0);

    QVERIFY(db->saveTile(QStringLiteral("cleanup1"), QStringLiteral("png"), QByteArray(50, 'C'), QStringLiteral("T"), setID));

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
    auto dbSrc = _createInitializedDB();

    quint64 customSetID = 0;
    _insertTileSet(dbSrc.get(), QStringLiteral("SharedName"), customSetID);
    QVERIFY(customSetID != 0);

    QVERIFY(dbSrc->saveTile(QStringLiteral("dedup_tile1"), QStringLiteral("png"), QByteArray(30, 'D'), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    const auto tileID = dbSrc->findTile(QStringLiteral("dedup_tile1"));
    QVERIFY(tileID.has_value());
    _linkTileToSet(dbSrc.get(), tileID.value(), customSetID);

    const auto srcSets = dbSrc->getTileSets();
    TileSetRecord customRec;
    for (const auto &rec : srcSets) {
        if (rec.name == QStringLiteral("SharedName")) {
            customRec = rec;
            customRec.numTiles = 1;
            break;
        }
    }
    QVERIFY(customRec.setID != 0);

    const QString exportPath = tempPath("dedup_export.db");
    const DatabaseResult exportResult = dbSrc->exportSets({customRec}, exportPath, nullptr);
    QVERIFY(exportResult.success);
    dbSrc.reset();

    auto dbTgt = std::make_unique<QGCTileCacheDatabase>(tempPath("dedup_target.db"));
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
    for (const auto &rec : targetSets) {
        if (rec.name == QStringLiteral("SharedName")) foundOriginal = true;
        if (rec.name == QStringLiteral("SharedName 0001")) foundDeduped = true;
    }
    QVERIFY(foundOriginal);
    QVERIFY(foundDeduped);
}

void QGCTileCacheDatabaseTest::_testGetTileDownloadListBatch()
{
    auto db = _createInitializedDB();

    const auto defaultSetID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultSetID.has_value());

    for (int i = 0; i < 10; i++) {
        _insertDownloadRecord(db.get(), defaultSetID.value(), QStringLiteral("batch_dl_%1").arg(i), QGCTile::StatePending);
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
    auto db = _createInitializedDB();

    quint64 setA = 0, setB = 0;
    _insertTileSet(db.get(), QStringLiteral("SetA"), setA);
    _insertTileSet(db.get(), QStringLiteral("SetB"), setB);

    const QByteArray data(10, 'A');
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, QStringLiteral("T"), setA));
    QVERIFY(db->saveTile(QStringLiteral("shared_hash"), QStringLiteral("png"), data, QStringLiteral("T"), setB));

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
    auto db1 = _createInitializedDB();

    const QByteArray data(20, 'L');
    QVERIFY(db1->saveTile(QStringLiteral("linger1"), QStringLiteral("png"), data, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

    const auto sets = db1->getTileSets();
    QVERIFY(!sets.isEmpty());
    TileSetRecord rec = sets[0];
    rec.numTiles = 1;

    const QString exportPath = tempPath("linger_export.db");

    const DatabaseResult exp1 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp1.success);

    const DatabaseResult exp2 = db1->exportSets({rec}, exportPath, nullptr);
    QVERIFY(exp2.success);

    auto db2 = std::make_unique<QGCTileCacheDatabase>(tempPath("linger_db2.db"));
    QVERIFY(db2->init());
    QVERIFY(db2->connectDB());
    const DatabaseResult imp1 = db2->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp1.success);

    auto db3 = std::make_unique<QGCTileCacheDatabase>(tempPath("linger_db3.db"));
    QVERIFY(db3->init());
    QVERIFY(db3->connectDB());
    const DatabaseResult imp2 = db3->importSetsMerge(exportPath, nullptr);
    QVERIFY(imp2.success);
}

void QGCTileCacheDatabaseTest::_testCreateTileSet()
{
    auto db = _createInitializedDB();

    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    const QString providerType = providerTypes.first();

    const auto setID = db->createTileSet(
        QStringLiteral("Test Set"), QStringLiteral("TestMap"),
        37.0, -122.0, 36.0, -121.0,
        5, 5, providerType, 100);

    QVERIFY(setID.has_value());
    QVERIFY(setID.value() != 0);

    const auto foundID = db->findTileSetID(QStringLiteral("Test Set"));
    QVERIFY(foundID.has_value());
    QCOMPARE(foundID.value(), setID.value());

    const auto sets = db->getTileSets();
    bool found = false;
    for (const auto &rec : sets) {
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
    auto db = _createInitializedDB();

    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    if (!file.open(QFile::ReadOnly)) {
        QSKIP("BingNoTileBytes.dat resource not available");
    }
    const QByteArray noTileBytes = file.readAll();
    file.close();
    QVERIFY(!noTileBytes.isEmpty());

    QVERIFY(db->saveTile(QStringLiteral("bing_notile_1"), QStringLiteral("png"), noTileBytes, QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
    QVERIFY(db->saveTile(QStringLiteral("normal_tile"), QStringLiteral("png"), QByteArray(50, 'N'), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));

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
    auto db = _createInitializedDB();

    const auto defaultID = db->findTileSetID(QStringLiteral("Default Tile Set"));
    QVERIFY(defaultID.has_value());

    QVERIFY(db->deleteTileSet(defaultID.value()));

    // After deleting default set, saveTile with kInvalidTileSet should fail gracefully
    // (no default set to link to), not crash or use stale ID
    QVERIFY(!db->saveTile(QStringLiteral("orphan"), QStringLiteral("png"), QByteArray(10, 'O'),
                           QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
}

void QGCTileCacheDatabaseTest::_testPruneCacheMultipleBatches()
{
    auto db = _createInitializedDB();

    // Save 200 tiles of 100 bytes each = 20000 bytes total
    for (int i = 0; i < 200; i++) {
        QVERIFY(db->saveTile(QStringLiteral("prune_multi_%1").arg(i), QStringLiteral("png"),
                              QByteArray(100, 'P'), QStringLiteral("T"), QGCTileCacheDatabase::kInvalidTileSet));
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
    auto db = _createInitializedDB();

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA user_version"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), QGCTileCacheDatabase::kSchemaVersion);
}

void QGCTileCacheDatabaseTest::_testSchemaVersionResetsLegacyDB()
{
    const QString path = tempPath("legacy.db");

    // Create a legacy database (no user_version set, but has tile data)
    {
        QSqlDatabase legacy = QSqlDatabase::addDatabase("QSQLITE", "legacy_setup");
        legacy.setDatabaseName(path);
        QVERIFY(legacy.open());
        QSqlQuery q(legacy);
        QVERIFY(q.exec("CREATE TABLE Tiles (tileID INTEGER PRIMARY KEY, hash TEXT UNIQUE, format TEXT, tile BLOB, size INTEGER, type INTEGER, date INTEGER)"));
        QVERIFY(q.exec("CREATE TABLE TileSets (setID INTEGER PRIMARY KEY, name TEXT UNIQUE, typeStr TEXT, topleftLat REAL, topleftLon REAL, bottomRightLat REAL, bottomRightLon REAL, minZoom INTEGER, maxZoom INTEGER, type INTEGER, numTiles INTEGER, defaultSet INTEGER, date INTEGER)"));
        QVERIFY(q.exec("CREATE TABLE SetTiles (setID INTEGER, tileID INTEGER)"));
        QVERIFY(q.exec("CREATE TABLE TilesDownload (setID INTEGER, hash TEXT, type INTEGER, x INTEGER, y INTEGER, z INTEGER, state INTEGER)"));
        QVERIFY(q.exec("INSERT INTO TileSets(name, defaultSet, date) VALUES('Default Tile Set', 1, 0)"));
        QVERIFY(q.exec("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES('old_hash', 'png', X'AA', 1, 0, 0)"));
        QVERIFY(q.exec("INSERT INTO SetTiles(setID, tileID) VALUES(1, 1)"));
        legacy.close();
    }
    QSqlDatabase::removeDatabase("legacy_setup");

    // Open with the versioned code — should detect legacy and reset
    QGCTileCacheDatabase db(path);
    QVERIFY(db.init());
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

void QGCTileCacheDatabaseTest::_testSaveTileTypeStoredAsInteger()
{
    auto db = _createInitializedDB();

    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    const QString type = providerTypes.first();
    const int expectedMapId = UrlFactory::getQtMapIdFromProviderType(type);
    QVERIFY(expectedMapId != -1);

    const QString hash = QStringLiteral("type_int_test");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), type, QGCTileCacheDatabase::kInvalidTileSet));

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
    QCOMPARE(tile->type, type);
}

void QGCTileCacheDatabaseTest::_testCreateTileSetTypeStoredAsInteger()
{
    auto db = _createInitializedDB();

    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    const QString providerType = providerTypes.first();
    const int expectedMapId = UrlFactory::getQtMapIdFromProviderType(providerType);
    QVERIFY(expectedMapId != -1);

    const auto setID = db->createTileSet(
        QStringLiteral("TypeInt Set"), QStringLiteral("TestMap"),
        37.0, -122.0, 36.0, -121.0,
        5, 5, providerType, 100);
    QVERIFY(setID.has_value());

    // Verify TileSets.type stores the integer mapId
    const auto sets = db->getTileSets();
    bool found = false;
    for (const auto &rec : sets) {
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
    auto db = _createInitializedDB();
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
    for (const auto &name : expected) {
        QVERIFY2(tables.contains(name), qPrintable(QStringLiteral("Missing table: %1").arg(name)));
    }
}

void QGCTileCacheDatabaseTest::_testTilesTableColumns()
{
    auto db = _createInitializedDB();
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(Tiles)"));

    struct ColInfo { QString name; QString type; bool notnull; QString dflt; bool pk; };
    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toBool(),
            query.value(4).toString(),
            query.value(5).toBool()
        });
    }

    QCOMPARE(cols.size(), 7);

    auto findCol = [&](const QString &name) -> const ColInfo* {
        for (const auto &c : cols) { if (c.name == name) return &c; }
        return nullptr;
    };

    const ColInfo *c = findCol(QStringLiteral("tileID"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QVERIFY(c->pk); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("hash"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("TEXT")); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("format"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("TEXT")); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("tile"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("BLOB"));

    c = findCol(QStringLiteral("size"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("type"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("date"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testTileSetsTableColumns()
{
    auto db = _createInitializedDB();
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(TileSets)"));

    struct ColInfo { QString name; QString type; bool notnull; QString dflt; bool pk; };
    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toBool(),
            query.value(4).toString(),
            query.value(5).toBool()
        });
    }

    QCOMPARE(cols.size(), 13);

    auto findCol = [&](const QString &name) -> const ColInfo* {
        for (const auto &c : cols) { if (c.name == name) return &c; }
        return nullptr;
    };

    const ColInfo *c = findCol(QStringLiteral("setID"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QVERIFY(c->pk); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("name"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("TEXT")); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("typeStr"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("TEXT"));

    c = findCol(QStringLiteral("topleftLat"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("REAL")); QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("topleftLon"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("REAL")); QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("bottomRightLat"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("REAL")); QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("bottomRightLon"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("REAL")); QCOMPARE(c->dflt, QStringLiteral("0.0"));

    c = findCol(QStringLiteral("minZoom"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("3"));

    c = findCol(QStringLiteral("maxZoom"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("3"));

    c = findCol(QStringLiteral("type"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("-1"));

    c = findCol(QStringLiteral("numTiles"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("0"));

    c = findCol(QStringLiteral("defaultSet"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("0"));

    c = findCol(QStringLiteral("date"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testSetTilesTableColumns()
{
    auto db = _createInitializedDB();
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(SetTiles)"));

    struct ColInfo { QString name; QString type; bool notnull; };
    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toBool()
        });
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
    auto db = _createInitializedDB();
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("PRAGMA table_info(TilesDownload)"));

    struct ColInfo { QString name; QString type; bool notnull; QString dflt; };
    QList<ColInfo> cols;
    while (query.next()) {
        cols.append({
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toBool(),
            query.value(4).toString()
        });
    }

    QCOMPARE(cols.size(), 7);

    auto findCol = [&](const QString &name) -> const ColInfo* {
        for (const auto &c : cols) { if (c.name == name) return &c; }
        return nullptr;
    };

    const ColInfo *c = findCol(QStringLiteral("setID"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("hash"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("TEXT")); QVERIFY(c->notnull);

    c = findCol(QStringLiteral("type"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("x"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("y"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("z"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER"));

    c = findCol(QStringLiteral("state"));
    QVERIFY(c); QCOMPARE(c->type, QStringLiteral("INTEGER")); QCOMPARE(c->dflt, QStringLiteral("0"));
}

void QGCTileCacheDatabaseTest::_testIndexesExist()
{
    auto db = _createInitializedDB();
    QVERIFY(db);

    QSqlQuery query(db->database());
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND sql IS NOT NULL ORDER BY name"));

    QStringList indexes;
    while (query.next()) {
        indexes << query.value(0).toString();
    }

    const QStringList expected = {
        QStringLiteral("idx_settiles_setid"),
        QStringLiteral("idx_settiles_tileid"),
        QStringLiteral("idx_settiles_unique"),
        QStringLiteral("idx_tiles_date"),
        QStringLiteral("idx_tilesdownload_setid_hash"),
        QStringLiteral("idx_tilesdownload_setid_state"),
    };

    QCOMPARE(indexes.size(), expected.size());
    for (const auto &name : expected) {
        QVERIFY2(indexes.contains(name), qPrintable(QStringLiteral("Missing index: %1").arg(name)));
    }
}

void QGCTileCacheDatabaseTest::_testForeignKeyCascadeDelete()
{
    auto db = _createInitializedDB();
    QVERIFY(db);

    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    const QString type = providerTypes.first();

    const QString hash = QStringLiteral("fk_cascade_hash");
    QVERIFY(db->saveTile(hash, QStringLiteral("png"), QByteArray("data"), type, QGCTileCacheDatabase::kInvalidTileSet));

    const auto tileID = db->findTile(hash);
    QVERIFY(tileID.has_value());

    const auto setID = db->createTileSet(
        QStringLiteral("CascadeTestSet"), type,
        10.0, 20.0, 30.0, 40.0, 5, 5, type, 1);
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
