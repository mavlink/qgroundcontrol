#include "QGCArchiveWatcherTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>

#include "QGCArchiveWatcher.h"
#include "QGCCompression.h"

void QGCArchiveWatcherTest::init()
{
    TempDirectoryTest::init();
    _outputDirPath = createSubDir("output");
    QVERIFY(!_outputDirPath.isEmpty());
}

void QGCArchiveWatcherTest::cleanup()
{
    _outputDirPath.clear();
    TempDirectoryTest::cleanup();
}

// ============================================================================
// Helper Methods
// ============================================================================
void QGCArchiveWatcherTest::_createTestArchive(const QString& path)
{
    // Copy test ZIP from resources
    QFile resourceFile(":/unittest/manifest.json.zip");
    QVERIFY(resourceFile.exists());
    QVERIFY(resourceFile.copy(path));
    // Make writable (copy from resource is read-only)
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

void QGCArchiveWatcherTest::_createTestCompressedFile(const QString& path)
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
    QVERIFY(watcher.outputDirectory().isEmpty());
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
    QVERIFY(watcher.outputDirectory().isEmpty());
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
    QVERIFY(watcher.watchDirectory(tempDir()->path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
    QVERIFY(watcher.watchedDirectories().contains(QFileInfo(tempDir()->path()).absoluteFilePath()));
}

void QGCArchiveWatcherTest::_testUnwatchDirectory()
{
    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(tempDir()->path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
    QVERIFY(watcher.unwatchDirectory(tempDir()->path()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCArchiveWatcherTest::_testWatchedDirectories()
{
    // Create subdirectories
    const QString dir1 = tempDir()->filePath("watch1");
    const QString dir2 = tempDir()->filePath("watch2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));
    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1));
    QVERIFY(watcher.watchDirectory(dir2));
    QCOMPARE(watcher.watchedDirectories().size(), 2);
}

void QGCArchiveWatcherTest::_testClear()
{
    const QString dir1 = tempDir()->filePath("cleardir1");
    const QString dir2 = tempDir()->filePath("cleardir2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));
    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1));
    QVERIFY(watcher.watchDirectory(dir2));
    QCOMPARE(watcher.watchedDirectories().size(), 2);
    watcher.clear();
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

// ============================================================================
// Archive detection tests
// ============================================================================
void QGCArchiveWatcherTest::_testScanDirectory()
{
    // Create test archives in temp directory
    _createTestArchive(tempDir()->filePath("test1.zip"));
    _createTestCompressedFile(tempDir()->filePath("test2.gz"));
    // Create a non-archive file
    QFile nonArchive(tempDir()->filePath("readme.txt"));
    QVERIFY(nonArchive.open(QIODevice::WriteOnly));
    nonArchive.write("not an archive");
    nonArchive.close();
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Both);
    const QStringList archives = watcher.scanDirectory(tempDir()->path());
    QCOMPARE(archives.size(), 2);
}

void QGCArchiveWatcherTest::_testArchiveDetectedSignal()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);  // Short debounce for testing
    watcher.setAutoDecompress(false);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    QVERIFY(watcher.watchDirectory(tempDir()->path()));
    // Copy archive to watched directory (simulates file appearing)
    const QString archivePath = tempDir()->filePath("detected.zip");
    _createTestArchive(archivePath);
    // Wait for detection
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), archivePath);
    QCOMPARE(spy.at(0).at(1).value<QGCCompression::Format>(), QGCCompression::Format::ZIP);
}

void QGCArchiveWatcherTest::_testFilterModeArchivesOnly()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Archives);
    // Create both archive and compressed file
    _createTestArchive(tempDir()->filePath("archive.zip"));
    _createTestCompressedFile(tempDir()->filePath("compressed.gz"));
    const QStringList detected = watcher.scanDirectory(tempDir()->path());
    // Should only detect ZIP, not .gz
    QCOMPARE(detected.size(), 1);
    QVERIFY(detected.at(0).endsWith(".zip"));
}

void QGCArchiveWatcherTest::_testFilterModeCompressedOnly()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Compressed);
    // Create both archive and compressed file
    _createTestArchive(tempDir()->filePath("archive.zip"));
    _createTestCompressedFile(tempDir()->filePath("compressed.gz"));
    const QStringList detected = watcher.scanDirectory(tempDir()->path());
    // Should only detect .gz, not ZIP
    QCOMPARE(detected.size(), 1);
    QVERIFY(detected.at(0).endsWith(".gz"));
}

void QGCArchiveWatcherTest::_testFilterModeBoth()
{
    QGCArchiveWatcher watcher;
    watcher.setFilterMode(QGCArchiveWatcher::FilterMode::Both);
    // Create both archive and compressed file
    _createTestArchive(tempDir()->filePath("archive.zip"));
    _createTestCompressedFile(tempDir()->filePath("compressed.gz"));
    const QStringList detected = watcher.scanDirectory(tempDir()->path());
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
    watcher.setOutputDirectory(_outputDirPath);
    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QSignalSpy progressSpy(&watcher, &QGCArchiveWatcher::progressChanged);
    QVERIFY(watcher.watchDirectory(tempDir()->path()));
    // Copy archive to watched directory
    const QString archivePath = tempDir()->filePath("auto_extract.zip");
    _createTestArchive(archivePath);
    // Wait for extraction to complete
    QVERIFY(extractionSpy.wait(5000));
    QCOMPARE(extractionSpy.count(), 1);
    QCOMPARE(extractionSpy.at(0).at(0).toString(), archivePath);
    QVERIFY(extractionSpy.at(0).at(2).toBool());  // success
    // Verify file was extracted
    QVERIFY(QFileInfo::exists(QDir(_outputDirPath).filePath("manifest.json")));
}

void QGCArchiveWatcherTest::_testAutoDecompressCompressedFile()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    watcher.setAutoDecompress(true);
    watcher.setOutputDirectory(_outputDirPath);
    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QVERIFY(watcher.watchDirectory(tempDir()->path()));
    // Copy compressed file to watched directory
    const QString compressedPath = tempDir()->filePath("auto_decompress.json.gz");
    _createTestCompressedFile(compressedPath);
    // Wait for decompression to complete
    QVERIFY(extractionSpy.wait(5000));
    QCOMPARE(extractionSpy.count(), 1);
    QCOMPARE(extractionSpy.at(0).at(0).toString(), compressedPath);
    QVERIFY(extractionSpy.at(0).at(2).toBool());  // success
    // Verify file was decompressed
    QVERIFY(QFileInfo::exists(QDir(_outputDirPath).filePath("auto_decompress.json")));
}

// ============================================================================
// Error handling
// ============================================================================
void QGCArchiveWatcherTest::_testWatchNonExistentDirectory()
{
    QGCArchiveWatcher watcher;
    QVERIFY(!watcher.watchDirectory("/nonexistent/path/to/directory"));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCArchiveWatcherTest::_testWatchEmptyPath()
{
    QGCArchiveWatcher watcher;
    QVERIFY(!watcher.watchDirectory(QString()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

UT_REGISTER_TEST(QGCArchiveWatcherTest, TestLabel::Unit, TestLabel::Utilities)
