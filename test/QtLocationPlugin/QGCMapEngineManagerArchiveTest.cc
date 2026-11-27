#include "QGCMapEngineManagerArchiveTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "ArchiveFixtures.h"
#include "FileFixtures.h"
#include "QGCCacheTile.h"
#include "QGCFormat.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCTileCacheDatabase.h"
#include "QGCWorkDirectory.h"
#include "QGeoFileTileCacheQGC.h"
#include "QmlObjectListModel.h"
#include "TileSetFixtures.h"
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

/// The tile set files in the directories archives are being unpacked into
QStringList unpackedTileSetFiles()
{
    QStringList files;
    QDirIterator it(tileDatabaseDirectory().path(), {QStringLiteral("*.qgctiledb")}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files.append(it.next());
    }
    return files;
}

/// Image of the tile @a hash in the tile cache of the test process, empty if the cache does not hold it
QByteArray tileCacheImage(const QString& hash)
{
    // The worker answers from its thread and then deletes the task later. The answer is not received by the task:
    // the wait below runs that deletion, which would take an answer still on its way with it
    QObject receiver;
    bool answered = false;
    QByteArray image;
    QGCFetchTileTask* const task = new QGCFetchTileTask(hash);
    (void) QObject::connect(task, &QGCFetchTileTask::tileFetched, &receiver, [&answered, &image](QGCCacheTile* tile) {
        image = tile->img;
        delete tile;
        answered = true;
    });
    (void) QObject::connect(task, &QGCMapTask::error, &receiver, [&answered]() { answered = true; });
    if (!getQGCMapEngine()->addTask(task)) {
        return {};
    }
    (void) QTest::qWaitFor([&answered]() { return answered; }, TestTimeout::mediumMs());
    return image;
}

/// Empties the tile cache of the test process: a test that imported into it leaves it as it found it
void resetTileCache()
{
    // Received by an object of its own, as in tileCacheImage()
    QObject receiver;
    bool done = false;
    QGCResetTask* const task = new QGCResetTask();
    (void) QObject::connect(task, &QGCResetTask::resetCompleted, &receiver, [&done]() { done = true; });
    (void) QObject::connect(task, &QGCMapTask::error, &receiver, [&done]() { done = true; });
    if (getQGCMapEngine()->addTask(task)) {
        (void) QTest::qWaitFor([&done]() { return done; }, TestTimeout::mediumMs());
    }
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

void QGCMapEngineManagerArchiveTest::_importArchiveEndsWithMessage_data()
{
    QTest::addColumn<QByteArray>("archive");
    // What the user is told, word for word
    QTest::addColumn<QString>("message");
    // Category and text of the warning the import logs, both empty for none
    QTest::addColumn<QString>("warningCategory");
    QTest::addColumn<QString>("warning");

    const auto row = [](const char* name, const QList<ArchiveFixtures::ZipEntry>& entries, const QString& message,
                        const char* warningCategory = "", const char* warning = "") {
        QTest::newRow(name) << ArchiveFixtures::makeZip(entries) << message << QString::fromLatin1(warningCategory)
                            << QString::fromLatin1(warning);
    };
    // The file of these tests is no tile set, so the import task refuses the one it is handed: the import got to it
    const auto refusedByTask = [&row](const char* name, const QList<ArchiveFixtures::ZipEntry>& entries,
                                      const char* fileName) {
        row(name, entries, QStringLiteral("%1 is not a tile set file of QGroundControl").arg(QLatin1String(fileName)),
            "QtLocationPlugin.QGCMapEngineManager", "is not a tile set file");
    };
    const QByteArray noTileSet("not a database");
    const QString noneFound = QStringLiteral("No tile database found in archive");
    // More than any disk holds: unpacking an entry that declares it fails the import for its size
    constexpr qint64 vast = Q_INT64_C(1) << 50;

    // Neither file has the name of a tile set file, so neither is unpacked: the archive is read to its end
    row("unpacks only tile set files",
        {{.name = "notes.txt", .data = "notes"},
         {.name = "vast.bin", .data = QByteArray(512, 'x'), .declaredSize = vast}},
        noneFound);
    // A device name of Windows, which an extraction skips on every system
    row("tells skipped tile set", {{.name = "Tiles/COM1.qgctiledb", .data = "tiles"}},
        QStringLiteral("No tile database found in archive (entries skipped for their names or types: 1)"),
        "Utilities.LibArchive", "device name of Windows");
    // The extension exportSets() gives its files, in a directory of the archive
    refusedByTask("finds exported file", {{.name = "sets/export.qgctiledb", .data = noTileSet}}, "export.qgctiledb");
    refusedByTask("finds .sqlite file", {{.name = "tiles.sqlite", .data = noTileSet}}, "tiles.sqlite");
    refusedByTask("finds file with upper-case extension", {{.name = "TILES.QGCTILEDB", .data = noTileSet}},
                  "TILES.QGCTILEDB");
    // Which of the two is meant cannot be told, so neither is imported. The second is not unpacked
    row("rejects several tile sets",
        {{.name = "export.qgctiledb", .data = noTileSet},
         {.name = "other.db", .data = QByteArray(512, 'x'), .declaredSize = vast}},
        QStringLiteral("The archive holds more than one tile set file"));
    // macOS packs the extended attributes of a file into "._<name>" beside it, or under __MACOSX/ in an archive that
    // Finder makes: it has the name of the tile set file, and is no tile set
    refusedByTask(
        "ignores AppleDouble: Finder, under __MACOSX",
        {{.name = "offline.qgctiledb", .data = noTileSet}, {.name = "__MACOSX/._offline.qgctiledb", .data = noTileSet}},
        "offline.qgctiledb");
    refusedByTask(
        "ignores AppleDouble: beside the file, first",
        {{.name = "._offline.qgctiledb", .data = noTileSet}, {.name = "offline.qgctiledb", .data = noTileSet}},
        "offline.qgctiledb");
    row("ignores AppleDouble: a companion only", {{.name = "__MACOSX/._offline.qgctiledb", .data = noTileSet}},
        noneFound);
}

// Which entry of an archive is taken for the tile set, and what the user is told when the import ends without one
void QGCMapEngineManagerArchiveTest::_importArchiveEndsWithMessage()
{
    QFETCH(QByteArray, archive);
    QFETCH(QString, message);
    QFETCH(QString, warningCategory);
    QFETCH(QString, warning);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.zip"));
    QVERIFY(TestFixtures::writeFile(archivePath, archive));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    if (!warning.isEmpty()) {
        expectLogMessage(qPrintable(warningCategory), QtWarningMsg,
                         QRegularExpression(QRegularExpression::escape(warning)));
    }
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(manager.errorMessage(), message);
    // What was unpacked is removed again: after a file the import task refused, once the task's error is handled
    QTRY_COMPARE_WITH_TIMEOUT(unpackDirectories(), unpackedBefore, TestTimeout::mediumMs());
}

void QGCMapEngineManagerArchiveTest::_importFailureShownEachTime_data()
{
    QTest::addColumn<QByteArray>("entryName");
    QTest::addColumn<QString>("message");

    QTest::newRow("refused before the import task")
        << QByteArray("notes.txt") << QStringLiteral("No tile database found in archive");
    QTest::newRow("refused by the import task")
        << QByteArray("export.qgctiledb")
        << QStringLiteral("export.qgctiledb is not a tile set file of QGroundControl");
}

void QGCMapEngineManagerArchiveTest::_importFailureShownEachTime()
{
    // The pages open their error dialog when the message changes: a user who imports the same file again must be
    // told again
    QFETCH(QByteArray, entryName);
    QFETCH(QString, message);
    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(QStringLiteral("again.zip"));
    QVERIFY(TestFixtures::writeFile(archivePath,
                                    ArchiveFixtures::makeZip({{.name = entryName, .data = "not a database"}})));

    QGCMapEngineManager manager;
    QStringList shown;
    (void) connect(&manager, &QGCMapEngineManager::errorMessageChanged, &manager, [&manager, &shown]() {
        if (!manager.errorMessage().isEmpty()) {
            shown.append(manager.errorMessage());
        }
    });
    ignoreLogMessage("QtLocationPlugin.QGCMapEngineManager", QtWarningMsg,
                     QRegularExpression("is not a tile set file"));
    for (int attempt = 1; attempt <= 2; ++attempt) {
        QVERIFY(manager.importSets(archivePath));
        QTRY_COMPARE_WITH_TIMEOUT(shown.count(), attempt, TestTimeout::mediumMs());
        QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                                  TestTimeout::mediumMs());
    }
    QCOMPARE(shown, QStringList({message, message}));
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
    QCOMPARE(manager.errorMessage(), QStringLiteral("The tile set in zeros%2C%1.zip is larger than ") + limit +
                                         QStringLiteral(": a tile set archive may unpack to 20 times its size, ") +
                                         limit + QStringLiteral(" at least"));
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

void QGCMapEngineManagerArchiveTest::_importArchiveThatFailsToUnpack_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("archive");
    // What the message says of why the archive was not unpacked
    QTest::addColumn<QStringList>("reasons");

    // An archive that the file dialog does not list can still be picked or dropped: its name shows what it is
#if !defined(QGC_ENABLE_7ZIP) || !defined(QGC_ENABLE_TAR)
    const QStringList notInThisBuild{QStringLiteral("the archive format is not supported by this build"),
                                     QStringLiteral("Unrecognized archive format")};
#ifndef QGC_ENABLE_TAR
    QTest::newRow("of format not in this build")
        << "tiles.tar" << ArchiveFixtures::makeTar({{.name = "export.qgctiledb", .data = "not a database"}})
        << notInThisBuild;
#else
    QTest::newRow("of format not in this build")
        << "tiles.7z" << (QByteArray::fromHex("377abcaf271c0004") + QByteArray(56, '\x01')) << notInThisBuild;
#endif
#endif

    // The tile set file does not have the checksum the archive gives for it
    const QByteArray content("not a database");
    ArchiveFixtures::ZipWriter damaged;
    damaged.add({.name = "export.qgctiledb"}, TestFixtures::deflate(content), ArchiveFixtures::crc32Of(content) ^ 1U,
                static_cast<quint64>(content.size()));
    QTest::newRow("damaged") << "tiles.zip" << damaged.finish() << QStringList{QStringLiteral("ZIP bad CRC")};
}

// An archive that cannot be unpacked ends the import as a failed one: the page shows why and lets the user go on, and
// nothing is left
void QGCMapEngineManagerArchiveTest::_importArchiveThatFailsToUnpack()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, archive);
    QFETCH(QStringList, reasons);

    QTemporaryDir tempDir;
    const QString archivePath = tempDir.filePath(fileName);
    QVERIFY(TestFixtures::writeFile(archivePath, archive));

    const QStringList unpackedBefore = unpackDirectories();

    QGCMapEngineManager manager;
    QVERIFY(manager.importSets(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());

    for (const QString& reason : std::as_const(reasons)) {
        QVERIFY2(manager.errorMessage().contains(reason), qPrintable(manager.errorMessage()));
    }
    QCOMPARE(unpackDirectories(), unpackedBefore);
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
    QTemporaryDir tempDir;
    const QString exportPath = tempDir.filePath(QStringLiteral("export.qgctiledb"));
    QVERIFY(TileSetFixtures::writeTileSetFile(exportPath, QStringLiteral("archived_tile")));
    const QString archivePath = tempDir.filePath(QStringLiteral("tiles.zip"));
    QVERIFY(TestFixtures::writeFile(
        archivePath,
        ArchiveFixtures::makeZip({{.name = "sets/export.qgctiledb", .data = TestFixtures::readFile(exportPath)}})));

    const QStringList unpackedBefore = unpackDirectories();
    QVERIFY(tileCacheImage(QStringLiteral("archived_tile")).isEmpty());
    const auto restoreCache = qScopeGuard([]() { resetTileCache(); });

    QGCMapEngineManager manager;
    manager.setProperty("importReplace", true);
    // The tile set is unpacked beside the tile database, where it is while the extraction reports its progress
    bool unpackedBesideDatabase = false;
    (void) connect(&manager, &QGCMapEngineManager::actionProgressChanged, &manager, [&]() {
        unpackedBesideDatabase = unpackedBesideDatabase || (unpackDirectories() != unpackedBefore);
    });
    // The unpacked file is the import's own, which moves it into place instead of copying it: once the import is
    // done, and before what was unpacked is removed, the file is not there any more
    bool unpackDirectoryThereWhenDone = false;
    QStringList tileSetFilesWhenDone;
    (void) connect(&manager, &QGCMapEngineManager::importActionChanged, &manager, [&]() {
        if (manager.importAction() == QGCMapEngineManager::ImportAction::ActionDone) {
            unpackDirectoryThereWhenDone = (unpackDirectories() != unpackedBefore);
            tileSetFilesWhenDone = unpackedTileSetFiles();
        }
    });
    QVERIFY(manager.importArchive(archivePath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().isEmpty(), qPrintable(manager.errorMessage()));
    QVERIFY(unpackedBesideDatabase);
    QVERIFY(unpackDirectoryThereWhenDone);
    QCOMPARE(tileSetFilesWhenDone, QStringList());
    QCOMPARE(unpackDirectories(), unpackedBefore);
    QCOMPARE(tileCacheImage(QStringLiteral("archived_tile")), TileSetFixtures::tileImage());
}

// A tile set file the user picked is the user's: a replace import copies it into the cache, and never moves it there
void QGCMapEngineManagerArchiveTest::_importSetsKeepsPickedFile()
{
    QTemporaryDir tempDir;
    const QString pickedPath = tempDir.filePath(QStringLiteral("picked.qgctiledb"));
    QVERIFY(TileSetFixtures::writeTileSetFile(pickedPath, QStringLiteral("picked_tile")));
    QVERIFY(tileCacheImage(QStringLiteral("picked_tile")).isEmpty());
    const auto restoreCache = qScopeGuard([]() { resetTileCache(); });

    QGCMapEngineManager manager;
    manager.setProperty("importReplace", true);
    QVERIFY(manager.importSets(pickedPath));
    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(manager.errorMessage().isEmpty(), qPrintable(manager.errorMessage()));
    QCOMPARE(tileCacheImage(QStringLiteral("picked_tile")), TileSetFixtures::tileImage());
    QVERIFY(QFile::exists(pickedPath));
}

// An export that fails has ended, so that the page lets the user go on, and tells why each time, as an import does
void QGCMapEngineManagerArchiveTest::_exportFailureShownAndEnds()
{
    QTemporaryDir tempDir;
    QGCMapEngineManager manager;
    manager.loadTileSets();
    QTRY_VERIFY_WITH_TIMEOUT(manager.tileSets()->count() > 0, TestTimeout::mediumMs());
    manager.selectAll();

    QStringList shown;
    (void) connect(&manager, &QGCMapEngineManager::errorMessageChanged, &manager, [&manager, &shown]() {
        if (!manager.errorMessage().isEmpty()) {
            shown.append(manager.errorMessage());
        }
    });
    for (int attempt = 1; attempt <= 2; ++attempt) {
        expectLogMessage("QtLocationPlugin.QGCTileCacheDatabase", QtCriticalMsg,
                         QRegularExpression("create export database"));
        expectLogMessage("QtLocationPlugin.QGCMapEngineManager", QtWarningMsg, QRegularExpression("Export Tile Sets"));
        // A directory is where the tile sets are to be written
        QVERIFY(manager.exportSets(tempDir.path()));
        QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                                  TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QCOMPARE(shown.count(), attempt);
    }
    const QString message =
        QStringLiteral("Error in task: Export Tile Sets\nError description:\nError opening export database");
    QCOMPARE(shown, QStringList({message, message}));
}

UT_REGISTER_TEST(QGCMapEngineManagerArchiveTest, TestLabel::Unit)
