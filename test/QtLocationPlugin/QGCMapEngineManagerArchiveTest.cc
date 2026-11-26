#include "QGCMapEngineManagerArchiveTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "FileFixtures.h"
#include "QGCFormat.h"
#include "QGCMapEngineManager.h"
#include "QGCTileCacheDatabase.h"
#include "QGCWorkDirectory.h"
#include "QGeoFileTileCacheQGC.h"
#include "UnitTest.h"

namespace {

/// The directory of the tile database, which an archive is unpacked into
QDir tileDatabaseDirectory()
{
    QGeoFileTileCacheQGC::ensureInitialized();
    return QFileInfo(QGeoFileTileCacheQGC::getDatabaseFilePath()).absoluteDir();
}

/// The directories archives are being unpacked into
QStringList unpackDirectories()
{
    return tileDatabaseDirectory().entryList({QStringLiteral("qgc_tiles_*")}, QDir::Dirs);
}

}  // namespace

void QGCMapEngineManagerArchiveTest::_importArchiveRejectsInvalidInput()
{
    QTemporaryDir tempDir;
    QGCMapEngineManager manager;

    QVERIFY(!manager.importArchive(QString()));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("No archive path specified")));

    QVERIFY(!manager.importArchive(QStringLiteral("/nonexistent/archive.zip")));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Archive file not found")));

    const QString plainFile = tempDir.filePath(QStringLiteral("qgc_map_manager_plain.txt"));
    QFile file(plainFile);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("not an archive");
    file.close();

    QVERIFY(!manager.importArchive(plainFile));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Not a supported archive format")));

    QFile::remove(plainFile);
}

void QGCMapEngineManagerArchiveTest::_importArchiveNoTileDatabase()
{
    QGCMapEngineManager manager;

    QSignalSpy actionSpy(&manager, &QGCMapEngineManager::importActionChanged);
    QVERIFY(actionSpy.isValid());

    QVERIFY(manager.importArchive(QStringLiteral(":/unittest/manifest.json.zip")));

    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY(manager.errorMessage().contains(QStringLiteral("No tile database found in archive")));
}

void QGCMapEngineManagerArchiveTest::_importArchiveTellsSkippedTileSet()
{
    // The tile set file has a device name of Windows, which an extraction skips on every system
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("device.zip"));
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    ArchiveFixtures::makeZip({{.name = "Tiles/COM1.qgctiledb", .data = "tiles"}})));

    QGCMapEngineManager manager;
    expectLogMessage("Utilities.LibArchive", QtWarningMsg, QRegularExpression("device name of Windows"));
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(manager.errorMessage(),
             QStringLiteral("No tile database found in archive (entries skipped for their names or types: 1)"));
}

void QGCMapEngineManagerArchiveTest::_importSetsUnpacksArchive()
{
    // The import of the Offline Maps page takes an archive as it takes a tile set file
    QGCMapEngineManager manager;
    QVERIFY(manager.importSets(QStringLiteral(":/unittest/manifest.json.zip")));

    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().contains(QStringLiteral("No tile database found in archive")),
             qPrintable(manager.errorMessage()));
}

void QGCMapEngineManagerArchiveTest::_importArchiveUnpacksOnlyTileSetFiles()
{
    // Beside its notes the archive holds a file no disk has room for, which declares 2^50 bytes. Neither has the
    // name of a tile set file, so neither is unpacked: the archive is read to its end, and has no tile set
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("no-tiles.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip(
                         {{.name = "notes.txt", .data = "notes"},
                          {.name = "vast.bin", .data = QByteArray(512, 'x'), .declaredSize = Q_INT64_C(1) << 50}})));

    QGCMapEngineManager manager;
    QVERIFY(manager.importArchive(archivePath));

    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().contains(QStringLiteral("No tile database found in archive")),
             qPrintable(manager.errorMessage()));
}

void QGCMapEngineManagerArchiveTest::_importArchiveFindsExportedFile()
{
    // A file with the extension exportSets() gives its files, in a directory of the archive. It is no tile set, so
    // the import that is handed it fails
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = "sets/export.qgctiledb", .data = "not a database"}})));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    expectLogMessage("QtLocationPlugin.QGCMapEngineManager", QtWarningMsg, QRegularExpression("Error in task"));
    QVERIFY(manager.importArchive(archivePath));
    QTRY_VERIFY_WITH_TIMEOUT(manager.errorMessage().contains(QStringLiteral("Error in task")), TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // What was unpacked for the import is removed again, and the import has ended: the page lets the user go on
    QTRY_COMPARE_WITH_TIMEOUT(unpackDirectories(), unpackedBefore, TestTimeout::mediumMs());
    QCOMPARE(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone);
}

void QGCMapEngineManagerArchiveTest::_importArchiveRejectsSeveralTileSets()
{
    // Which of the two files is meant cannot be told, so neither is imported. The second is not unpacked: it
    // declares more than any disk holds
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("two.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip(
                         {{.name = "export.qgctiledb", .data = "not a database"},
                          {.name = "other.db", .data = QByteArray(512, 'x'), .declaredSize = Q_INT64_C(1) << 50}})));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().contains(QStringLiteral("more than one tile set file")),
             qPrintable(manager.errorMessage()));
    QCOMPARE(unpackDirectories(), unpackedBefore);
}

void QGCMapEngineManagerArchiveTest::_importArchiveStopsAtSizeLimit()
{
    // A tile set of zeros, 128 MiB in an archive of a few hundred KiB: far more than a tile set grows by. The name,
    // as a URL-encoded download may have it, holds the markers that a message substitutes
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("zeros%2C%1.zip"));
    QVERIFY(ArchiveFixtures::writeZipOfZeros(archivePath, "zeros.qgctiledb", Q_INT64_C(128) * 1024 * 1024));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::longMs());
    // The archive is small, so its limit is the least one: the message names the archive, the limit and the rule
    const QString limit = QGC::bigSizeToString(Q_UINT64_C(64) * 1024 * 1024);
    QCOMPARE(manager.errorMessage(),
             QStringLiteral("zeros%2C%1.zip unpacks to more than ") + limit +
                 QStringLiteral(", the most a tile set archive may unpack to: 20 times its size, ") + limit +
                 QStringLiteral(" at least"));
    QCOMPARE(unpackDirectories(), unpackedBefore);
}

void QGCMapEngineManagerArchiveTest::_importArchiveRemovedWithManager()
{
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = "export.qgctiledb", .data = QByteArray(200000, 't')}})));

    const QStringList unpackedBefore = unpackDirectories();

    // A manager that is destroyed while it unpacks an archive stops, and leaves nothing beside the tile database
    {
        QGCMapEngineManager manager;
        QVERIFY(manager.importArchive(archivePath));
    }
    QCOMPARE(unpackDirectories(), unpackedBefore);
}

void QGCMapEngineManagerArchiveTest::_importArchiveRejectsWhenAlreadyImporting()
{
    QGCMapEngineManager manager;
    manager.setImportAction(QGCMapEngineManager::ImportAction::ActionImporting);

    QVERIFY(!manager.importArchive(QStringLiteral(":/unittest/manifest.json.zip")));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Import already in progress")));

    // Neither does the import of the Offline Maps page start a second one, or forget the one that runs
    manager.setErrorMessage(QString());
    QVERIFY(!manager.importSets(QStringLiteral(":/unittest/manifest.json.zip")));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Import already in progress")));
    QCOMPARE(manager.importAction(), QGCMapEngineManager::ImportAction::ActionImporting);
}

void QGCMapEngineManagerArchiveTest::_importArchiveOfFormatNotInThisBuild()
{
#if defined(QGC_ENABLE_7ZIP) && defined(QGC_ENABLE_TAR)
    QSKIP("This build extracts every archive format");
#else
    // An archive that the file dialog does not list can still be picked or dropped: its name shows what it is
    QTemporaryDir tempDir;
#ifndef QGC_ENABLE_TAR
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.tar"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeTar({{.name = "export.qgctiledb", .data = "not a database"}})));
#else
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.7z"));
    QVERIFY(TestFixtures::writeFile(archivePath, QByteArray::fromHex("377abcaf271c0004") + QByteArray(56, '\x01')));
#endif

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    QVERIFY(manager.importSets(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());

    // The import ends as a failed one does: the page shows why and lets the user go on, and nothing is left
    QVERIFY2(manager.errorMessage().contains(QStringLiteral("the archive format is not supported by this build")),
             qPrintable(manager.errorMessage()));
    QVERIFY2(manager.errorMessage().contains(QStringLiteral("Unrecognized archive format")),
             qPrintable(manager.errorMessage()));
    QCOMPARE(unpackDirectories(), unpackedBefore);
#endif
}

void QGCMapEngineManagerArchiveTest::_importArchivePatterns()
{
    // The file dialog of the import lists the archives this build extracts, and no other
    QStringList expected{QStringLiteral("*.zip")};
#ifdef QGC_ENABLE_7ZIP
    expected.append(QStringLiteral("*.7z"));
#endif
#ifdef QGC_ENABLE_TAR
    expected.append({QStringLiteral("*.tar"), QStringLiteral("*.tar.gz"), QStringLiteral("*.tgz"),
                     QStringLiteral("*.tar.xz"), QStringLiteral("*.txz")});
#ifdef QGC_ENABLE_ZSTD
    expected.append({QStringLiteral("*.tar.zst"), QStringLiteral("*.tar.zstd")});
#endif
#ifdef QGC_ENABLE_BZIP2
    expected.append({QStringLiteral("*.tar.bz2"), QStringLiteral("*.tbz2"), QStringLiteral("*.tbz")});
#endif
#ifdef QGC_ENABLE_LZ4
    expected.append(QStringLiteral("*.tar.lz4"));
#endif
#endif
    expected.sort();

    const QGCMapEngineManager manager;
    QStringList patterns = manager.property("importArchivePatterns").toStringList();
    patterns.sort();
    QCOMPARE(patterns, expected);
}

void QGCMapEngineManagerArchiveTest::_importArchiveRemovesStaleDirectories()
{
    // Left behind by an import that crashed: a directory without a lock, with a tile set file in it
    const QDir databaseDirectory = tileDatabaseDirectory();
    const QString stale = QStringLiteral("qgc_tiles_stale");
    QVERIFY(databaseDirectory.mkdir(stale));
    QVERIFY(TestFixtures::writeFile(databaseDirectory.filePath(stale + QStringLiteral("/left.qgctiledb")), "left"));
    // The directory of an import that another QGC runs
    const QGCWorkDirectory running(databaseDirectory.path(), QStringLiteral("qgc_tiles_"));
    QVERIFY(running.isValid());

    QGCMapEngineManager manager;
    QVERIFY(manager.importArchive(QStringLiteral(":/unittest/manifest.json.zip")));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());

    QCOMPARE(unpackDirectories(), QStringList{QFileInfo(running.path()).fileName()});
}

void QGCMapEngineManagerArchiveTest::_importArchiveRoomToUnpack_data()
{
    QTest::addColumn<qint64>("bytesAvailable");
    QTest::addColumn<bool>("refused");

    // The archive has 1000 bytes. Free space that is not known, as QStorageInfo reports it, never refuses
    QTest::newRow("not known") << qint64{-1} << false;
    QTest::newRow("less") << qint64{999} << true;
    QTest::newRow("as much") << qint64{1000} << false;
}

void QGCMapEngineManagerArchiveTest::_importArchiveRoomToUnpack()
{
    QFETCH(qint64, bytesAvailable);
    QFETCH(bool, refused);

    QCOMPARE(QGCMapEngineManager::lacksRoomToUnpack(bytesAvailable, 1000), refused);
}

void QGCMapEngineManagerArchiveTest::_importArchiveReplacesFromBesideDatabase()
{
    // A tile set with one tile, exported as the Offline Maps page exports it
    QTemporaryDir tempDir;
    const QString exportPath = tempDir.filePath(QStringLiteral("export.qgctiledb"));
    {
        QGCTileCacheDatabase source(tempDir.filePath(QStringLiteral("source.db")));
        QVERIFY(source.init());
        QVERIFY(source.connectDB());
        QVERIFY(source.saveTile(QStringLiteral("archived_tile"), QStringLiteral("png"), QByteArray(50, 'T'),
                                QStringLiteral("Bing Road"), QGCTileCacheDatabase::kInvalidTileSet));
        QList<TileSetRecord> sets = source.getTileSets();
        QCOMPARE(sets.size(), 1);
        sets[0].numTiles = 1;
        QVERIFY(source.exportSets(sets, exportPath, nullptr).success);
    }
    QFile exported(exportPath);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath, ArchiveFixtures::makeZip({{.name = "sets/export.qgctiledb", .data = exported.readAll()}})));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    manager.setProperty("importReplace", true);
    // The tile set is unpacked beside the tile database, where it is while the extraction reports its progress
    bool unpackedBesideDatabase = false;
    (void) connect(&manager, &QGCMapEngineManager::actionProgressChanged, &manager, [&]() {
        unpackedBesideDatabase = unpackedBesideDatabase || (unpackDirectories() != unpackedBefore);
    });
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().isEmpty(), qPrintable(manager.errorMessage()));
    QVERIFY(unpackedBesideDatabase);
    QCOMPARE(unpackDirectories(), unpackedBefore);
}

UT_REGISTER_TEST(QGCMapEngineManagerArchiveTest, TestLabel::Unit)
