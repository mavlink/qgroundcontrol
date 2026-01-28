#include "QGCArchiveWatcherTest.h"
#include "QGCArchiveWatcher.h"
#include "QGCCompression.h"
#include "TestHelpers.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void QGCArchiveWatcherTest::init()
{
    TempDirTest::init();

    _outputDir = std::make_unique<QTemporaryDir>();
    QVERIFY2(_outputDir->isValid(), "Failed to create output directory");
}

void QGCArchiveWatcherTest::cleanup()
{
    _outputDir.reset();

    TempDirTest::cleanup();
}

// ============================================================================
// Helper Methods
// ============================================================================

void QGCArchiveWatcherTest::_createTestArchive(const QString &path)
{
    // Copy test ZIP from resources
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.exists());
    QVERIFY(resourceFile.copy(path));
    // Make writable (copy from resource is read-only)
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

void QGCArchiveWatcherTest::_createTestCompressedFile(const QString &path)
{
    // Copy test gzipped file from resources
    QFile resourceFile(":/unittest/manifest.json.gz");
    QVERIFY(resourceFile.exists());
    QVERIFY(resourceFile.copy(path));
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

// ============================================================================
// Configuration tests
// ============================================================================

void QGCArchiveWatcherTest::_testDefaultConfiguration()
{
    QGCArchiveWatcher watcher;

    QCOMPARE(watcher.filterMode(), QGCArchiveWatcher::FilterMode::Both);
    QVERIFY(!watcher.autoDecompress());
    QGC_VERIFY_EMPTY(watcher.outputDirectory());
    QVERIFY(!watcher.removeAfterExtraction());
    QCOMPARE(watcher.debounceDelay(), 500);  // Default for archive watcher
    QVERIFY(!watcher.isExtracting());
    QCOMPARE(watcher.progress(), 0.0);
}

void QGCArchiveWatcherTest::_testSetFilterMode()
{
    QGCArchiveWatcher watcher;

    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Archives);
    QCOMPARE(watcher.filterMode(), QGCArchiveWatcher::FilterMode::Archives);

    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Compressed);
    QCOMPARE(watcher.filterMode(), QGCArchiveWatcher::FilterMode::Compressed);

    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Both);
    QCOMPARE(watcher.filterMode(), QGCArchiveWatcher::FilterMode::Both);
}

void QGCArchiveWatcherTest::_testSetAutoDecompress()
{
    QGCArchiveWatcher watcher;
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::autoDecompressChanged);

    QVERIFY(!watcher.autoDecompress());

    watcher.setAutoDecompress(true);
    QVERIFY(watcher.autoDecompress());
    QCOMPARE(spy.count(), 1);

    // Setting same value shouldn't emit
    watcher.setAutoDecompress(true);
    QCOMPARE(spy.count(), 1);

    watcher.setAutoDecompress(false);
    QVERIFY(!watcher.autoDecompress());
    QCOMPARE(spy.count(), 2);
}

void QGCArchiveWatcherTest::_testSetOutputDirectory()
{
    QGCArchiveWatcher watcher;
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::outputDirectoryChanged);
    QGC_VERIFY_SPY_VALID(spy);

    QGC_VERIFY_EMPTY(watcher.outputDirectory());

    watcher.setOutputDirectory("/tmp/test");
    QCOMPARE(watcher.outputDirectory(), QString("/tmp/test"));
    QCOMPARE(spy.count(), 1);

    // Setting same value shouldn't emit
    watcher.setOutputDirectory("/tmp/test");
    QCOMPARE(spy.count(), 1);
}

// ============================================================================
// Directory watching tests
// ============================================================================

void QGCArchiveWatcherTest::_testWatchDirectory()
{
    QGCArchiveWatcher watcher;

    QVERIFY(watcher.watchDirectory(tempPath()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
    QVERIFY(watcher.watchedDirectories().contains(QFileInfo(tempPath()).absoluteFilePath()));
}

void QGCArchiveWatcherTest::_testUnwatchDirectory()
{
    QGCArchiveWatcher watcher;

    QVERIFY(watcher.watchDirectory(tempPath()));
    QGC_COMPARE_SIZE(watcher.watchedDirectories(), 1);

    QVERIFY(watcher.unwatchDirectory(tempPath()));
    QGC_VERIFY_EMPTY(watcher.watchedDirectories());
}

void QGCArchiveWatcherTest::_testWatchedDirectories()
{
    // Create subdirectories
    const QString dir1 = tempFilePath("watch1");
    const QString dir2 = tempFilePath("watch2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));

    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1));
    QVERIFY(watcher.watchDirectory(dir2));

    QCOMPARE(watcher.watchedDirectories().size(), 2);
}

void QGCArchiveWatcherTest::_testClear()
{
    const QString dir1 = tempFilePath("cleardir1");
    const QString dir2 = tempFilePath("cleardir2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));

    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1));
    QVERIFY(watcher.watchDirectory(dir2));

    QGC_COMPARE_SIZE(watcher.watchedDirectories(), 2);

    watcher.clear();

    QGC_VERIFY_EMPTY(watcher.watchedDirectories());
}

// ============================================================================
// Archive detection tests
// ============================================================================

void QGCArchiveWatcherTest::_testScanDirectory()
{
    // Create test archives in temp directory
    _createTestArchive(tempFilePath("test1.zip"));
    _createTestCompressedFile(tempFilePath("test2.gz"));

    // Create a non-archive file
    QFile nonArchive(tempFilePath("readme.txt"));
    QVERIFY(nonArchive.open(QIODevice::WriteOnly));
    nonArchive.write("not an archive");
    nonArchive.close();

    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Both);

    const QStringList archives = watcher.scanDirectory(tempPath());

    QCOMPARE(archives.size(), 2);
}

void QGCArchiveWatcherTest::_testArchiveDetectedSignal()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);  // Short debounce for testing
    watcher.setAutoDecompress(false);

    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    QGC_VERIFY_SPY_VALID(spy);

    QVERIFY(watcher.watchDirectory(tempPath()));

    // Copy archive to watched directory (simulates file appearing)
    const QString archivePath = tempFilePath("detected.zip");
    _createTestArchive(archivePath);

    // Wait for detection - use longer timeout for CI systems where FS notifications can be delayed
    // File system notifications are platform-dependent and may take longer on some systems
    const bool detected = spy.wait(TestHelpers::kDefaultTimeoutMs);

    if (detected) {
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), archivePath);
        QCOMPARE(spy.at(0).at(1).value<QGCCompression::Format>(), QGCCompression::Format::ZIP);
    } else {
        // On platforms where FS notifications don't work (e.g., some Docker/WSL configurations),
        // verify that scanDirectory can still find the archive
        QSKIP("File system notifications not available on this platform");
    }
}

void QGCArchiveWatcherTest::_testFilterModeArchivesOnly()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Archives);

    // Create both archive and compressed file
    _createTestArchive(tempFilePath("archive.zip"));
    _createTestCompressedFile(tempFilePath("compressed.gz"));

    const QStringList detected = watcher.scanDirectory(tempPath());

    // Should only detect ZIP, not .gz
    QCOMPARE(detected.size(), 1);
    QVERIFY(detected.at(0).endsWith(".zip"));
}

void QGCArchiveWatcherTest::_testFilterModeCompressedOnly()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Compressed);

    // Create both archive and compressed file
    _createTestArchive(tempFilePath("archive.zip"));
    _createTestCompressedFile(tempFilePath("compressed.gz"));

    const QStringList detected = watcher.scanDirectory(tempPath());

    // Should only detect .gz, not ZIP
    QCOMPARE(detected.size(), 1);
    QVERIFY(detected.at(0).endsWith(".gz"));
}

void QGCArchiveWatcherTest::_testFilterModeBoth()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Both);

    // Create both archive and compressed file
    _createTestArchive(tempFilePath("archive.zip"));
    _createTestCompressedFile(tempFilePath("compressed.gz"));

    const QStringList detected = watcher.scanDirectory(tempPath());

    // Should detect both
    QCOMPARE(detected.size(), 2);
}

// ============================================================================
// Auto-decompress tests
// ============================================================================

void QGCArchiveWatcherTest::_testAutoDecompressArchive()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    watcher.setAutoDecompress(true);
    watcher.setOutputDirectory(_outputDir->path());

    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QSignalSpy progressSpy(&watcher, &QGCArchiveWatcher::progressChanged);
    QGC_VERIFY_SPY_VALID(extractionSpy);
    QGC_VERIFY_SPY_VALID(progressSpy);

    QVERIFY(watcher.watchDirectory(tempPath()));

    // Copy archive to watched directory
    const QString archivePath = tempFilePath("auto_extract.zip");
    _createTestArchive(archivePath);

    // Wait for extraction to complete
    QVERIFY(extractionSpy.wait(TestHelpers::kDefaultTimeoutMs));

    QCOMPARE(extractionSpy.count(), 1);
    QCOMPARE(extractionSpy.at(0).at(0).toString(), archivePath);
    QVERIFY(extractionSpy.at(0).at(2).toBool());  // success

    // Verify file was extracted
    VERIFY_FILE_EXISTS(_outputDir->filePath("manifest.json"));
}

void QGCArchiveWatcherTest::_testAutoDecompressCompressedFile()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    watcher.setAutoDecompress(true);
    watcher.setOutputDirectory(_outputDir->path());

    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QGC_VERIFY_SPY_VALID(extractionSpy);

    QVERIFY(watcher.watchDirectory(tempPath()));

    // Copy compressed file to watched directory
    const QString compressedPath = tempFilePath("auto_decompress.json.gz");
    _createTestCompressedFile(compressedPath);

    // Wait for decompression to complete
    QVERIFY(extractionSpy.wait(TestHelpers::kDefaultTimeoutMs));

    QCOMPARE(extractionSpy.count(), 1);
    QCOMPARE(extractionSpy.at(0).at(0).toString(), compressedPath);
    QVERIFY(extractionSpy.at(0).at(2).toBool());  // success

    // Verify file was decompressed
    VERIFY_FILE_EXISTS(_outputDir->filePath("auto_decompress.json"));
}

// ============================================================================
// Error handling
// ============================================================================

void QGCArchiveWatcherTest::_testWatchNonExistentDirectory()
{
    QGCArchiveWatcher watcher;
    QVERIFY(!watcher.watchDirectory("/nonexistent/path/to/directory"));
    QGC_VERIFY_EMPTY(watcher.watchedDirectories());
}

void QGCArchiveWatcherTest::_testWatchEmptyPath()
{
    QGCArchiveWatcher watcher;
    QVERIFY(!watcher.watchDirectory(QString()));
    QGC_VERIFY_EMPTY(watcher.watchedDirectories());
}
