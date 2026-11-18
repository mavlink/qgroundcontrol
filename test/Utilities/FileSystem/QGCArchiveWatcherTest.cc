#include "QGCArchiveWatcherTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <sys/inotify.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "FileFixtures.h"
#include "QGCArchiveWatcher.h"
#include "QGCCompression.h"

void QGCArchiveWatcherTest::init()
{
    UnitTest::init();
    _tempDir = QTemporaryDir();
    QVERIFY(_tempDir.isValid());
    _outputDirPath = _tempDir.path() + "/output";
    QVERIFY(QDir().mkpath(_outputDirPath));
}

void QGCArchiveWatcherTest::cleanup()
{
    _outputDirPath.clear();
    UnitTest::cleanup();
}

// ============================================================================
// Helper Methods
// ============================================================================
void QGCArchiveWatcherTest::_copyResource(const QString& resource, const QString& path)
{
    QFile resourceFile(resource);
    QVERIFY(resourceFile.exists());
    QVERIFY(resourceFile.copy(path));
    // Make writable (copy from resource is read-only)
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

std::unique_ptr<QGCArchiveWatcher> QGCArchiveWatcherTest::_createPollingWatcher()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // QFileSystemWatcher asks for its inotify instance when it is constructed and polls for good if it gets
    // none. With no file descriptor to be had the request fails as it does when the instances have run out.
    struct rlimit original;
    if (getrlimit(RLIMIT_NOFILE, &original) != 0) {
        return nullptr;
    }
    struct rlimit none = original;
    none.rlim_cur = 0;
    if (setrlimit(RLIMIT_NOFILE, &none) != 0) {
        return nullptr;
    }
    auto watcher = std::make_unique<QGCArchiveWatcher>();
    const int inotifyInstance = inotify_init1(IN_CLOEXEC);
    (void) setrlimit(RLIMIT_NOFILE, &original);
    if (inotifyInstance != -1) {
        (void) close(inotifyInstance);
        return nullptr;
    }
    return watcher;
#else
    return nullptr;
#endif
}

QStringList QGCArchiveWatcherTest::_reportedArchives(const QSignalSpy& spy)
{
    QStringList archives;
    for (const QList<QVariant>& report : spy) {
        archives.append(report.at(0).toString());
    }
    return archives;
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
void QGCArchiveWatcherTest::_testWatchAndUnwatchDirectory()
{
    QGCArchiveWatcher watcher;
    QVERIFY(watcher.watchDirectory(_tempDir.path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
    QVERIFY(watcher.watchedDirectories().contains(QFileInfo(_tempDir.path()).absoluteFilePath()));
    QVERIFY(watcher.unwatchDirectory(_tempDir.path()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCArchiveWatcherTest::_testWatchedDirectoriesAndClear()
{
    const QString dir1 = _tempDir.filePath("watch1");
    const QString dir2 = _tempDir.filePath("watch2");
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
void QGCArchiveWatcherTest::_testScanDirectory_data()
{
    QTest::addColumn<int>("filterMode");
    QTest::addColumn<QStringList>("found");

    const QString archive = QStringLiteral("archive.zip");
    const QString compressed = QStringLiteral("compressed.gz");
    QTest::newRow("archives") << int(QGCArchiveWatcher::FilterMode::Archives) << QStringList{archive};
    QTest::newRow("compressed files") << int(QGCArchiveWatcher::FilterMode::Compressed) << QStringList{compressed};
    QTest::newRow("both") << int(QGCArchiveWatcher::FilterMode::Both) << QStringList{archive, compressed};
}

void QGCArchiveWatcherTest::_testScanDirectory()
{
    QFETCH(int, filterMode);
    QFETCH(QStringList, found);

    _copyResource(_zipResource, _tempDir.filePath("archive.zip"));
    _copyResource(_gzipResource, _tempDir.filePath("compressed.gz"));
    QVERIFY(TestFixtures::writeFile(_tempDir.filePath("readme.txt"), "not an archive"));

    QGCArchiveWatcher watcher;
    watcher.setFilterMode(static_cast<QGCArchiveWatcher::FilterMode>(filterMode));
    QStringList names;
    for (const QString& path : watcher.scanDirectory(_tempDir.path())) {
        names.append(QFileInfo(path).fileName());
    }
    names.sort();
    QCOMPARE(names, found);
}

void QGCArchiveWatcherTest::_testArchiveDetectedSignal_data()
{
    QTest::addColumn<bool>("polling");

    QTest::newRow("default engine") << false;
    QTest::newRow("polling engine") << true;
}

void QGCArchiveWatcherTest::_testArchiveDetectedSignal()
{
    QFETCH(bool, polling);

    const std::unique_ptr<QGCArchiveWatcher> watcher =
        polling ? _createPollingWatcher() : std::make_unique<QGCArchiveWatcher>();
    if (!watcher) {
        QSKIP("QFileSystemWatcher cannot be made to poll on this platform");
    }
    watcher->setDebounceDelay(50);  // Short debounce for testing
    watcher->setAutoDecompress(false);
    QSignalSpy spy(watcher.get(), &QGCArchiveWatcher::archiveDetected);
    QVERIFY(watcher->watchDirectory(_tempDir.path()));
    // Copy archive to watched directory (simulates file appearing)
    const QString archivePath = _tempDir.filePath("detected.zip");
    _copyResource(_zipResource, archivePath);
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, _detectionTimeoutMs());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), archivePath);
    QCOMPARE(spy.at(0).at(1).value<QGCCompression::Format>(), QGCCompression::Format::ZIP);

    // A file that appears after the directory was last looked at is reported the next time, and only that file
    const QString laterPath = _tempDir.filePath("later.json.gz");
    _copyResource(_gzipResource, laterPath);
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 2, _detectionTimeoutMs());
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toString(), laterPath);
    QCOMPARE(spy.at(1).at(1).value<QGCCompression::Format>(), QGCCompression::Format::GZIP);
}

void QGCArchiveWatcherTest::_testFileBeforeWatchIsAdded_data()
{
    QTest::addColumn<bool>("stays");

    QTest::newRow("reported") << true;
    QTest::newRow("gone before the report") << false;
}

void QGCArchiveWatcherTest::_testFileBeforeWatchIsAdded()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a symbolic link to a file that is not there yet");
#else
    QFETCH(bool, stays);

    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    const QString watchedPath = _tempDir.filePath("incoming");
    QVERIFY(QDir().mkdir(watchedPath));

    // watchDirectory() lists the files of the directory and then adds the watch. A file that lands in between
    // is not in the list, and the watch does not tell of it. The window cannot be hit from outside, so the
    // state is reached with a link that leads nowhere: it is not listed as a file until its target appears,
    // and that changes nothing in the directory.
    const QString targetPath = _tempDir.filePath("elsewhere.zip");
    const QString landedPath = watchedPath + "/landed.zip";
    QVERIFY(QFile::link(targetPath, landedPath));
    QVERIFY(watcher.watchDirectory(watchedPath));
    _copyResource(_zipResource, targetPath);

    // Watching the directory looks at it again once the watch is in place, and reports the file after the
    // debounce delay if it is still there then
    QVERIFY(watcher.watchDirectory(watchedPath));
    QStringList expected;
    if (stays) {
        QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, _detectionTimeoutMs());
        expected.append(landedPath);
    } else {
        QVERIFY(QFile::remove(targetPath));
    }

    // The archive that follows is reported after it, or alone
    const QString laterPath = watchedPath + "/later.zip";
    _copyResource(_zipResource, laterPath);
    expected.append(laterPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(laterPath), _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), expected);
#endif
}

void QGCArchiveWatcherTest::_testWatchAfterDirectoryWasRemoved_data()
{
    QTest::addColumn<int>("debounceDelay");

    // The watcher hears of the end of a watch after the debounce delay. Without one it has heard when the
    // directory is no longer listed as watched; with one the directory is watched again before it does.
    QTest::newRow("end of the watch delivered") << 0;
    QTest::newRow("end of the watch pending") << 500;
}

void QGCArchiveWatcherTest::_testWatchAfterDirectoryWasRemoved()
{
#ifdef Q_OS_WIN
    QSKIP("On Windows QFileSystemWatcher warns, without a category, when a watched directory is removed");
#else
    QFETCH(int, debounceDelay);

    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(debounceDelay);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    const QString watchedPath = _tempDir.filePath("incoming");
    QVERIFY(QDir().mkdir(watchedPath));
    _copyResource(_zipResource, watchedPath + "/old.zip");
    QVERIFY(watcher.watchDirectory(watchedPath));

    // The watch ends with the directory
    QVERIFY(QDir(watchedPath).removeRecursively());
    QVERIFY_TRUE_WAIT(watcher.watchedDirectories().isEmpty(), _detectionTimeoutMs());

    // Another directory under the same path, with a file under a name the first one had and one under a new name
    QVERIFY(QDir().mkdir(watchedPath));
    _copyResource(_zipResource, watchedPath + "/old.zip");
    _copyResource(_zipResource, watchedPath + "/other.zip");
    QVERIFY(watcher.watchDirectory(watchedPath));

    // Both were there before the directory was watched, and neither is reported, as for any directory that
    // is not being watched. The two files that follow are, the second after anything the watch started with.
    const QString laterPath = watchedPath + "/later.zip";
    _copyResource(_zipResource, laterPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(laterPath), _detectionTimeoutMs());
    const QString lastPath = watchedPath + "/last.zip";
    _copyResource(_zipResource, lastPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(lastPath), _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), (QStringList{laterPath, lastPath}));
#endif
}

void QGCArchiveWatcherTest::_testWatchAfterWatchFailed()
{
#ifndef Q_OS_UNIX
    QSKIP("Needs a directory that the permissions of POSIX keep from being read");
#else
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    const QString watchedPath = _tempDir.filePath("incoming");
    QVERIFY(QDir().mkdir(watchedPath));
    _copyResource(_zipResource, watchedPath + "/present.zip");

    // A directory that may not be read cannot be listed, and inotify refuses to watch it. QFileSystemWatcher
    // tells of the refusal in a message without a category, which can be neither expected nor ignored, so
    // the messages of this one call are kept from the log.
    QVERIFY(QFile::setPermissions(watchedPath, QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    const QtMessageHandler logHandler =
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString&) {});
    const bool watched = watcher.watchDirectory(watchedPath);
    qInstallMessageHandler(logHandler);
    QVERIFY(
        QFile::setPermissions(watchedPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
    if (watched) {
        // An engine that polls adds the directory, as does one that is asked by a user who may read anything
        QSKIP("The watch of a directory that may not be read does not fail here");
    }
    QVERIFY(watcher.watchedDirectories().isEmpty());

    // The watch that is added now starts from the file that is there, which the one that failed did not see
    QVERIFY(watcher.watchDirectory(watchedPath));
    const QString laterPath = watchedPath + "/later.zip";
    _copyResource(_zipResource, laterPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(laterPath), _detectionTimeoutMs());
    const QString lastPath = watchedPath + "/last.zip";
    _copyResource(_zipResource, lastPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(lastPath), _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), (QStringList{laterPath, lastPath}));
#endif
}

void QGCArchiveWatcherTest::_testFileCreatedEmptyIsReportedWithContent()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    QVERIFY(watcher.watchDirectory(_tempDir.path()));

    // A download that is created first and written afterwards, under a name that does not tell its format
    const QString downloadPath = _tempDir.filePath("download");
    QVERIFY(TestFixtures::writeFile(downloadPath, QByteArray()));

    // The archive is created after it, so the watcher has looked at the empty file when it reports the archive
    const QString archivePath = _tempDir.filePath("marker.zip");
    _copyResource(_zipResource, archivePath);
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), QStringList{archivePath});

    QVERIFY(TestFixtures::writeFile(downloadPath, TestFixtures::readFile(":/unittest/manifest.json.gz")));
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 2, _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), (QStringList{archivePath, downloadPath}));
    QCOMPARE(spy.at(1).at(1).value<QGCCompression::Format>(), QGCCompression::Format::GZIP);
}

void QGCArchiveWatcherTest::_testFileWithoutContentIsForgottenWhenUnwatched_data()
{
    QTest::addColumn<QString>("separator");

    // The watcher knows a directory by the path it was given, with the separator that path ends in
    QTest::newRow("path without a trailing separator") << "";
    QTest::newRow("path with a trailing separator") << "/";
}

void QGCArchiveWatcherTest::_testFileWithoutContentIsForgottenWhenUnwatched()
{
    QFETCH(QString, separator);

    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    const QString directoryPath = _tempDir.filePath("incoming");
    const QString watchedPath = directoryPath + separator;
    QVERIFY(QDir().mkdir(directoryPath));
    QVERIFY(watcher.watchDirectory(watchedPath));
    // A second directory, which stays watched
    const QString otherPath = _tempDir.filePath("other");
    QVERIFY(QDir().mkdir(otherPath));
    QVERIFY(watcher.watchDirectory(otherPath));

    // A file that the watcher waits to see written. It has looked at it when it reports the archive.
    const QString downloadPath = directoryPath + "/download";
    QVERIFY(TestFixtures::writeFile(downloadPath, QByteArray()));
    const QString markerPath = directoryPath + "/marker.zip";
    _copyResource(_zipResource, markerPath);
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, _detectionTimeoutMs());

    // The watch of the file ends with that of its directory, so the file is not reported when it is written
    QVERIFY(watcher.unwatchDirectory(watchedPath));
    QVERIFY(TestFixtures::writeFile(downloadPath, TestFixtures::readFile(":/unittest/manifest.json.gz")));

    // The watcher hears of a changed file before it hears of a directory that changed after it: a report of
    // the file would come before that of the archive in the other directory
    const QString laterPath = otherPath + "/later.zip";
    _copyResource(_zipResource, laterPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(laterPath), _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), (QStringList{markerPath, laterPath}));
}

void QGCArchiveWatcherTest::_testFileWithoutContentIsForgottenWithItsDirectory_data()
{
    QTest::addColumn<QString>("separator");

    QTest::newRow("path without a trailing separator") << "";
    QTest::newRow("path with a trailing separator") << "/";
}

void QGCArchiveWatcherTest::_testFileWithoutContentIsForgottenWithItsDirectory()
{
#ifdef Q_OS_WIN
    QSKIP("On Windows QFileSystemWatcher warns, without a category, when a watched directory is removed");
#else
    QFETCH(QString, separator);

    QGCArchiveWatcher watcher;
    // The watcher hears that a watch has ended after the debounce delay, and the directory is watched again
    // before that
    watcher.setDebounceDelay(500);
    QSignalSpy spy(&watcher, &QGCArchiveWatcher::archiveDetected);
    const QString directoryPath = _tempDir.filePath("incoming");
    const QString watchedPath = directoryPath + separator;
    QVERIFY(QDir().mkdir(watchedPath));
    QVERIFY(watcher.watchDirectory(watchedPath));

    // A file that the watcher waits to see written. It has looked at it when it reports the archive.
    const QString downloadPath = directoryPath + "/download";
    QVERIFY(TestFixtures::writeFile(downloadPath, QByteArray()));
    const QString markerPath = directoryPath + "/marker.zip";
    _copyResource(_zipResource, markerPath);
    QVERIFY_SIGNAL_COUNT_WAIT(spy, 1, _detectionTimeoutMs());

    // The watches end with the directory. Another one under the same path has a compressed file of that name.
    QVERIFY(QDir(watchedPath).removeRecursively());
    QVERIFY_TRUE_WAIT(watcher.watchedDirectories().isEmpty(), _detectionTimeoutMs());
    QVERIFY(QDir().mkdir(watchedPath));
    QVERIFY(TestFixtures::writeFile(downloadPath, TestFixtures::readFile(":/unittest/manifest.json.gz")));
    QVERIFY(watcher.watchDirectory(watchedPath));

    // That file was there before the directory was watched. The end of the first one's watch, which the
    // watcher hears of before it reports the archive that follows, does not make it a file that was written.
    const QString laterPath = directoryPath + "/later.zip";
    _copyResource(_zipResource, laterPath);
    QVERIFY_TRUE_WAIT(_reportedArchives(spy).contains(laterPath), _detectionTimeoutMs());
    QCOMPARE(_reportedArchives(spy), (QStringList{markerPath, laterPath}));
#endif
}

// ============================================================================
// Auto-decompress tests
// ============================================================================
void QGCArchiveWatcherTest::_testAutoDecompress_data()
{
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("outputName");
    QTest::addColumn<bool>("removeAfterExtraction");

    QTest::newRow("archive") << _zipResource << QStringLiteral("auto_extract.zip") << QStringLiteral("manifest.json")
                             << false;
    QTest::newRow("compressed file") << _gzipResource << QStringLiteral("auto_decompress.json.gz")
                                     << QStringLiteral("auto_decompress.json") << false;
    QTest::newRow("archive, removed after extraction")
        << _zipResource << QStringLiteral("remove_after_extract.zip") << QStringLiteral("manifest.json") << true;
}

void QGCArchiveWatcherTest::_testAutoDecompress()
{
    QFETCH(QString, resource);
    QFETCH(QString, fileName);
    QFETCH(QString, outputName);
    QFETCH(bool, removeAfterExtraction);

    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    watcher.setAutoDecompress(true);
    watcher.setRemoveAfterExtraction(removeAfterExtraction);
    watcher.setOutputDirectory(_outputDirPath);
    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QVERIFY(watcher.watchDirectory(_tempDir.path()));

    // The file appears in the watched directory
    const QString sourcePath = _tempDir.filePath(fileName);
    _copyResource(resource, sourcePath);
    QVERIFY_SIGNAL_WAIT(extractionSpy, TestTimeout::mediumMs());
    QCOMPARE(extractionSpy.count(), 1);
    QCOMPARE(extractionSpy.at(0).at(0).toString(), sourcePath);
    QVERIFY(extractionSpy.at(0).at(2).toBool());  // success
    QVERIFY(QFileInfo::exists(QDir(_outputDirPath).filePath(outputName)));
    QCOMPARE(QFile::exists(sourcePath), !removeAfterExtraction);
}

void QGCArchiveWatcherTest::_testCancelExtractionResetsState()
{
    QGCArchiveWatcher watcher;
    watcher.setDebounceDelay(50);
    watcher.setAutoDecompress(true);
    watcher.setOutputDirectory(_outputDirPath);

    QSignalSpy extractionSpy(&watcher, &QGCArchiveWatcher::extractionComplete);
    QVERIFY(watcher.watchDirectory(_tempDir.path()));

    const QString archivePath = _tempDir.filePath("cancel_extract.zip");
    _copyResource(_zipResource, archivePath);

    QVERIFY_TRUE_WAIT(watcher.isExtracting() || extractionSpy.count() > 0, _detectionTimeoutMs());
    watcher.cancelExtraction();

    QVERIFY_TRUE_WAIT(!watcher.isExtracting() && watcher.progress() == 0.0, TestTimeout::shortMs());
    // State is already settled (not extracting, progress == 0). A quarter-second
    // is ample to catch any stray queued completion signal without burning a
    // full shortMs() budget — QVERIFY_NO_SIGNAL_WAIT always waits the full timeout.
    QVERIFY_NO_SIGNAL_WAIT(extractionSpy, 250);
    QVERIFY(extractionSpy.count() <= 1);
}

// ============================================================================
// Error handling
// ============================================================================
void QGCArchiveWatcherTest::_testWatchRefused_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("warning");

    QTest::newRow("missing directory") << QStringLiteral("/nonexistent/path/to/directory")
                                       << QStringLiteral("watchDirectory: not a directory:");
    QTest::newRow("empty path") << QString() << QStringLiteral("watchDirectory: empty path");
}

void QGCArchiveWatcherTest::_testWatchRefused()
{
    QFETCH(QString, path);
    QFETCH(QString, warning);

    QGCArchiveWatcher watcher;
    expectLogMessage("Utilities.QGCArchiveWatcher", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!watcher.watchDirectory(path));
    verifyExpectedLogMessage();
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

UT_REGISTER_TEST(QGCArchiveWatcherTest, TestLabel::Unit, TestLabel::Utilities)
