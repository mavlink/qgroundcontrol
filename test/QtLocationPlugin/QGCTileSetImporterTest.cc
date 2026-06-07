#include "QGCTileSetImporterTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QtEndian>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileSetImporter.h"
#include "QGCCompression.h"
#include "QGCCompressionTypes.h"

#include <cstring>
#include <functional>

namespace {
const QString kProviderType = QStringLiteral("CustomURL Custom");
}

std::unique_ptr<QGCTileCacheDatabase> QGCTileSetImporterTest::_createInitializedDB(QTemporaryDir &tempDir)
{
    auto db = std::make_unique<QGCTileCacheDatabase>(tempDir.filePath("tiles.db"));
    if (!db->init() || !db->connectDB()) {
        return nullptr;
    }
    return db;
}

// Builds a minimal valid MBTiles file: tiles(zoom_level, tile_column, tile_row, tile_data).
// Two tiles at zoom 1 with distinct TMS rows so the Y-flip is observable.
QString QGCTileSetImporterTest::_writeMBTilesFixture(QTemporaryDir &tempDir, const QString &fileName)
{
    const QString path = tempDir.filePath(fileName);
    {
        QSqlDatabase src = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("mbtiles_fixture"));
        src.setDatabaseName(path);
        if (!src.open()) {
            return QString();
        }
        QSqlQuery q(src);
        q.exec(QStringLiteral("CREATE TABLE tiles (zoom_level INTEGER, tile_column INTEGER, tile_row INTEGER, "
                              "tile_data BLOB)"));
        // PNG magic so getImageFormat recognises the blobs.
        const QByteArray png = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("tileA");
        const QByteArray png2 = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("tileB");

        q.prepare(QStringLiteral("INSERT INTO tiles(zoom_level, tile_column, tile_row, tile_data) VALUES(?, ?, ?, ?)"));
        q.addBindValue(1); q.addBindValue(0); q.addBindValue(0); q.addBindValue(png); q.exec();
        q.addBindValue(1); q.addBindValue(1); q.addBindValue(1); q.addBindValue(png2); q.exec();
        src.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("mbtiles_fixture"));
    return path;
}

void QGCTileSetImporterTest::_testImportMBTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);
    QVERIFY(UrlFactory::getQtMapIdFromProviderType(kProviderType) != -1);

    const QString path = _writeMBTilesFixture(tempDir, QStringLiteral("fixture.mbtiles"));
    QVERIFY(!path.isEmpty());

    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("Imported MB"), kProviderType);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE(result.tileCount, static_cast<quint32>(2));
    QVERIFY(result.setID != 0);

    // The named set must exist alongside the default set.
    const auto setID = db->findTileSetID(QStringLiteral("Imported MB"));
    QVERIFY(setID.has_value());
    QCOMPARE(setID.value(), result.setID);
}

void QGCTileSetImporterTest::_testImportMBTilesYFlip()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QString path = _writeMBTilesFixture(tempDir, QStringLiteral("flip.mbtiles"));
    QVERIFY(!path.isEmpty());

    QVERIFY(QGCTileSetImporter::import(*db, path, QStringLiteral("Flip Set"), kProviderType).success);

    // zoom=1 tms_row=0 -> xyz y = (2^1 - 1) - 0 = 1 ; tms_row=1 -> xyz y = 0.
    const QString hash0 = UrlFactory::getTileHash(kProviderType, 0, 1, 1); // column 0, flipped row
    const QString hash1 = UrlFactory::getTileHash(kProviderType, 1, 0, 1); // column 1, flipped row
    QVERIFY(db->findTile(hash0).has_value());
    QVERIFY(db->findTile(hash1).has_value());

    // The un-flipped (raw TMS) hashes must NOT be present, proving the flip happened.
    QVERIFY(!db->findTile(UrlFactory::getTileHash(kProviderType, 0, 0, 1)).has_value());
}

void QGCTileSetImporterTest::_testImportUnsupportedExtension()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, tempDir.filePath("foo.xyz"), QStringLiteral("Bad"), kProviderType);
    QVERIFY(!result.success);
    QVERIFY(result.errorString.contains(QStringLiteral("Unsupported")));
}

void QGCTileSetImporterTest::_testImportMissingFile()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, tempDir.filePath("nope.mbtiles"), QStringLiteral("Missing"), kProviderType);
    QVERIFY(!result.success);
}

namespace {
void appendVarint(QByteArray &out, quint64 v)
{
    do {
        quint8 b = v & 0x7F;
        v >>= 7;
        if (v) {
            b |= 0x80;
        }
        out.append(static_cast<char>(b));
    } while (v);
}


// Build a 127-byte PMTiles v3 header. Compression fields default to None(1), tile_type png(2).
struct PMFields {
    quint64 rootOffset = 0;
    quint64 rootBytes = 0;
    quint64 leafOffset = 0;
    quint64 leafBytes = 0;
    quint64 tileDataOffset = 0;
    quint64 tileDataBytes = 0;
    quint64 addressedTiles = 1;
};

QByteArray buildPMHeader(const PMFields &f)
{
    constexpr int kHeaderSize = 127;
    QByteArray header(kHeaderSize, '\0');
    std::memcpy(header.data(), "PMTiles", 7);
    header[7] = 3;
    auto put = [&header](int off, quint64 v) { qToLittleEndian<quint64>(v, header.data() + off); };
    put(8, f.rootOffset);
    put(16, f.rootBytes);
    put(24, 0); put(32, 0);
    put(40, f.leafOffset);
    put(48, f.leafBytes);
    put(56, f.tileDataOffset);
    put(64, f.tileDataBytes);
    put(72, f.addressedTiles); put(80, 1); put(88, 1);
    header[96] = 1;  // clustered
    header[97] = 1;  // internal_compression None
    header[98] = 1;  // tile_compression None
    header[99] = 2;  // tile_type png
    header[100] = 0; // min_zoom
    header[101] = 0; // max_zoom
    return header;
}

} // namespace

QString QGCTileSetImporterTest::_writeMalformedPMTiles(
    QTemporaryDir &tempDir, const QString &fileName,
    const std::function<void(QByteArray &, QByteArray &, QByteArray &)> &mutate)
{
    // Caller fully owns header/rootDir/tile bytes (offsets included) via the mutate callback.
    QByteArray header;
    QByteArray rootDir;
    QByteArray tile;
    mutate(header, rootDir, tile);

    const QString path = tempDir.filePath(fileName);
    QFile fout(path);
    if (!fout.open(QIODevice::WriteOnly)) {
        return QString();
    }
    fout.write(header);
    fout.write(rootDir);
    fout.write(tile);
    fout.close();
    return path;
}

QString QGCTileSetImporterTest::_writePMTilesFixture(QTemporaryDir &tempDir, const QString &fileName)
{
    // Hand-build a minimal v3 archive: uncompressed root dir + uncompressed PNG tile,
    // single entry at z/x/y = 0/0/0 (tile_id 0), run_length 1.
    const QByteArray tile = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("pmtileA");

    QByteArray rootDir;
    appendVarint(rootDir, 1);            // num entries
    appendVarint(rootDir, 0);            // tile_id delta (first id = 0)
    appendVarint(rootDir, 1);            // run_length
    appendVarint(rootDir, tile.size());  // length
    appendVarint(rootDir, 1);            // offset (+1 framing -> real offset 0)

    constexpr int kHeaderSize = 127;
    const quint64 rootOffset = kHeaderSize;
    const quint64 rootBytes = static_cast<quint64>(rootDir.size());
    const quint64 tileDataOffset = rootOffset + rootBytes;

    QByteArray header(kHeaderSize, '\0');
    std::memcpy(header.data(), "PMTiles", 7);
    header[7] = 3; // version
    auto put = [&header](int off, quint64 v) { qToLittleEndian<quint64>(v, header.data() + off); };
    put(8, rootOffset);
    put(16, rootBytes);
    put(24, 0); put(32, 0);                 // json metadata
    put(40, 0); put(48, 0);                 // leaf dirs
    put(56, tileDataOffset);
    put(64, static_cast<quint64>(tile.size()));
    put(72, 1); put(80, 1); put(88, 1);     // addressed/entries/contents counts
    header[96] = 1;                         // clustered
    header[97] = 1;                         // internal_compression = None
    header[98] = 1;                         // tile_compression = None
    header[99] = 2;                         // tile_type = png
    header[100] = 0;                        // min_zoom
    header[101] = 0;                        // max_zoom

    const QString path = tempDir.filePath(fileName);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return QString();
    }
    f.write(header);
    f.write(rootDir);
    f.write(tile);
    f.close();
    return path;
}

void QGCTileSetImporterTest::_testPMTilesZxyToTileId()
{
    // Published PMTiles v3 spec vectors.
    QCOMPARE(QGCTileSetImporter::zxyToTileId(0, 0, 0), static_cast<quint64>(0));
    QCOMPARE(QGCTileSetImporter::zxyToTileId(1, 0, 0), static_cast<quint64>(1));
    QCOMPARE(QGCTileSetImporter::zxyToTileId(1, 0, 1), static_cast<quint64>(2));
    QCOMPARE(QGCTileSetImporter::zxyToTileId(1, 1, 1), static_cast<quint64>(3));
    QCOMPARE(QGCTileSetImporter::zxyToTileId(1, 1, 0), static_cast<quint64>(4));
    QCOMPARE(QGCTileSetImporter::zxyToTileId(2, 0, 0), static_cast<quint64>(5));
}

void QGCTileSetImporterTest::_testImportPMTiles()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);
    QVERIFY(UrlFactory::getQtMapIdFromProviderType(kProviderType) != -1);

    const QString path = _writePMTilesFixture(tempDir, QStringLiteral("fixture.pmtiles"));
    QVERIFY(!path.isEmpty());

    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("Imported PM"), kProviderType);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE(result.tileCount, static_cast<quint32>(1));
    QVERIFY(result.setID != 0);

    // tile_id 0 == z/x/y 0/0/0; PMTiles is XYZ so no flip.
    const QString hash = UrlFactory::getTileHash(kProviderType, 0, 0, 0);
    QVERIFY(db->findTile(hash).has_value());

    const auto setID = db->findTileSetID(QStringLiteral("Imported PM"));
    QVERIFY(setID.has_value());
    QCOMPARE(setID.value(), result.setID);
}

void QGCTileSetImporterTest::_testPMTilesDirEntryCountTooLarge()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QByteArray tile = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("t");

    const QString path = _writeMalformedPMTiles(
        tempDir, QStringLiteral("s2.pmtiles"),
        [&](QByteArray &header, QByteArray &rootDir, QByteArray &outTile) {
            // numEntries varint claims a billion entries; blob is a few bytes.
            appendVarint(rootDir, 1000000000ULL);
            const quint64 rootOffset = 127;
            PMFields f;
            f.rootOffset = rootOffset;
            f.rootBytes = static_cast<quint64>(rootDir.size());
            f.tileDataOffset = rootOffset + f.rootBytes;
            f.tileDataBytes = static_cast<quint64>(tile.size());
            header = buildPMHeader(f);
            outTile = tile;
        });
    QVERIFY(!path.isEmpty());

    // S2 is rejected in deserializeDirectory before the tile set is created, so the
    // importer returns the error without emitting the "PMTiles import failed" warning.
    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("S2"), kProviderType);
    QVERIFY(!result.success);
    QVERIFY(!result.errorString.isEmpty());
}

void QGCTileSetImporterTest::_testPMTilesTileOutsideDataRegion()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QByteArray tile = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("t");

    const QString path = _writeMalformedPMTiles(
        tempDir, QStringLiteral("s3.pmtiles"),
        [&](QByteArray &header, QByteArray &rootDir, QByteArray &outTile) {
            // One entry whose offset+length exceeds the declared tileDataBytes.
            appendVarint(rootDir, 1);   // num entries
            appendVarint(rootDir, 0);   // tile_id delta
            appendVarint(rootDir, 1);   // run_length
            appendVarint(rootDir, 9999); // length far past the region
            appendVarint(rootDir, 1);   // offset (+1 framing -> 0)

            const quint64 rootOffset = 127;
            PMFields f;
            f.rootOffset = rootOffset;
            f.rootBytes = static_cast<quint64>(rootDir.size());
            f.tileDataOffset = rootOffset + f.rootBytes;
            f.tileDataBytes = static_cast<quint64>(tile.size()); // small region
            header = buildPMHeader(f);
            outTile = tile;
        });
    QVERIFY(!path.isEmpty());

    expectLogMessage("QtLocationPlugin.QGCTileSetImporter", QtWarningMsg,
                     QRegularExpression(QStringLiteral("PMTiles import failed")));
    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("S3"), kProviderType);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);
    QVERIFY(result.errorString.contains(QStringLiteral("tile-data region")));
}

void QGCTileSetImporterTest::_testPMTilesLeafCycle()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    // Root has a single leaf entry (run_length 0) at leaf offset 0. The leaf directory
    // itself is another leaf entry pointing back to leaf offset 0 -> cycle. Must return
    // promptly with an error, not recurse forever.
    QByteArray leafDir;
    {
        appendVarint(leafDir, 1);   // num entries
        appendVarint(leafDir, 0);   // tile_id delta
        appendVarint(leafDir, 0);   // run_length 0 -> leaf pointer
        appendVarint(leafDir, 0);   // length placeholder (patched below)
        appendVarint(leafDir, 1);   // offset (+1 framing -> leaf offset 0, same as itself)
    }

    QByteArray rootDir;
    {
        appendVarint(rootDir, 1);                      // num entries
        appendVarint(rootDir, 0);                      // tile_id delta
        appendVarint(rootDir, 0);                      // run_length 0 -> leaf pointer
        appendVarint(rootDir, leafDir.size());         // length of leaf dir
        appendVarint(rootDir, 1);                      // offset (+1 framing -> leaf offset 0)
    }

    const QString path = _writeMalformedPMTiles(
        tempDir, QStringLiteral("s4.pmtiles"),
        [&](QByteArray &header, QByteArray &outRoot, QByteArray &outTile) {
            const quint64 rootOffset = 127;
            PMFields f;
            f.rootOffset = rootOffset;
            f.rootBytes = static_cast<quint64>(rootDir.size());
            f.leafOffset = rootOffset + f.rootBytes;
            f.leafBytes = static_cast<quint64>(leafDir.size());
            f.tileDataOffset = f.leafOffset + f.leafBytes;
            f.tileDataBytes = 0;
            header = buildPMHeader(f);
            outRoot = rootDir;
            // Layout written is header + rootDir + tile; place the leaf bytes as the "tile" blob
            // so they live at leafOffset.
            outTile = leafDir;
        });
    QVERIFY(!path.isEmpty());

    expectLogMessage("QtLocationPlugin.QGCTileSetImporter", QtWarningMsg,
                     QRegularExpression(QStringLiteral("PMTiles import failed")));
    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("S4"), kProviderType);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);
    QVERIFY(!result.errorString.isEmpty());
}

void QGCTileSetImporterTest::_testPMTilesZoomTooLarge()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QByteArray tile = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("t");

    const QString path = _writeMalformedPMTiles(
        tempDir, QStringLiteral("s5.pmtiles"),
        [&](QByteArray &header, QByteArray &rootDir, QByteArray &outTile) {
            // tile_id so large that decoding zoom exceeds 31 -> tileIdToZxy returns false.
            appendVarint(rootDir, 1);                          // num entries
            appendVarint(rootDir, 0xFFFFFFFFFFFFFFFFULL);        // tile_id delta (huge)
            appendVarint(rootDir, 1);                          // run_length
            appendVarint(rootDir, static_cast<quint64>(tile.size())); // length
            appendVarint(rootDir, 1);                          // offset (+1 -> 0)

            const quint64 rootOffset = 127;
            PMFields f;
            f.rootOffset = rootOffset;
            f.rootBytes = static_cast<quint64>(rootDir.size());
            f.tileDataOffset = rootOffset + f.rootBytes;
            f.tileDataBytes = static_cast<quint64>(tile.size());
            f.addressedTiles = 1;
            header = buildPMHeader(f);
            outTile = tile;
        });
    QVERIFY(!path.isEmpty());

    expectLogMessage("QtLocationPlugin.QGCTileSetImporter", QtWarningMsg,
                     QRegularExpression(QStringLiteral("PMTiles import failed")));
    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("S5"), kProviderType);
    verifyExpectedLogMessage();
    QVERIFY(!result.success);
    QVERIFY(!result.errorString.isEmpty());
}

void QGCTileSetImporterTest::_testDecompressCapRejectsOversize()
{
    // 40-byte gzip stream that inflates to 4096 'A' bytes (deterministic, mtime=0).
    const QByteArray gz = QByteArray::fromHex(
        "1f8b08000000000002ffedc1010d000000c2a06cef5fca1e0e28000000e0dd004034a6fe00100000");

    // libarchive logs an environment-internal size-limit warning; the behavior under
    // test is the empty return, so suppress the noise.
    ignoreLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Size limit exceeded")));

    // Cap below the inflated size -> rejected (empty).
    const QByteArray capped = QGCCompression::decompressData(gz, QGCCompression::Format::GZIP, 1024);
    QVERIFY(capped.isEmpty());

    // Unlimited (0) -> full 4096-byte output.
    const QByteArray full = QGCCompression::decompressData(gz, QGCCompression::Format::GZIP, 0);
    QCOMPARE(full.size(), 4096);
    QCOMPARE(full.at(0), 'A');
}

void QGCTileSetImporterTest::_testImportMBTilesDedupSchema()
{
    QTemporaryDir tempDir;
    auto db = _createInitializedDB(tempDir);
    QVERIFY(db);

    const QString path = tempDir.filePath(QStringLiteral("dedup.mbtiles"));
    {
        QSqlDatabase src = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("mbtiles_dedup"));
        src.setDatabaseName(path);
        QVERIFY(src.open());
        QSqlQuery q(src);
        // mb-util dedup schema: map(zoom,col,row,tile_id) JOIN images(tile_id,tile_data). No `tiles`.
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE map (zoom_level INTEGER, tile_column INTEGER, "
                                      "tile_row INTEGER, tile_id TEXT)")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE images (tile_id TEXT, tile_data BLOB)")));

        const QByteArray pngA = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("dedupA");
        const QByteArray pngB = QByteArray::fromHex("89504e470d0a1a0a") + QByteArray("dedupB");
        q.prepare(QStringLiteral("INSERT INTO images(tile_id, tile_data) VALUES(?, ?)"));
        q.addBindValue(QStringLiteral("a")); q.addBindValue(pngA); QVERIFY(q.exec());
        q.addBindValue(QStringLiteral("b")); q.addBindValue(pngB); QVERIFY(q.exec());

        // Two map rows share image "a" (dedup), one uses "b" -> 3 imported tiles.
        q.prepare(QStringLiteral("INSERT INTO map(zoom_level, tile_column, tile_row, tile_id) VALUES(?,?,?,?)"));
        q.addBindValue(1); q.addBindValue(0); q.addBindValue(0); q.addBindValue(QStringLiteral("a")); QVERIFY(q.exec());
        q.addBindValue(1); q.addBindValue(1); q.addBindValue(0); q.addBindValue(QStringLiteral("a")); QVERIFY(q.exec());
        q.addBindValue(1); q.addBindValue(1); q.addBindValue(1); q.addBindValue(QStringLiteral("b")); QVERIFY(q.exec());
        src.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("mbtiles_dedup"));

    const QGCTileSetImporter::Result result =
        QGCTileSetImporter::import(*db, path, QStringLiteral("Dedup MB"), kProviderType);
    QVERIFY2(result.success, qPrintable(result.errorString));
    QCOMPARE(result.tileCount, static_cast<quint32>(3));
}

UT_REGISTER_TEST(QGCTileSetImporterTest, TestLabel::Unit)
