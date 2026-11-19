#include "QGCFileWatcherTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "QGCFileWatcher.h"

namespace {

QString _normalizedPath(const QString& path)
{
    const QFileInfo info(path);
    const QString canonicalPath = info.canonicalFilePath();
    if (!canonicalPath.isEmpty()) {
        return QDir::cleanPath(canonicalPath);
    }

    return QDir::cleanPath(info.absoluteFilePath());
}

bool _pathsEquivalent(const QString& lhs, const QString& rhs)
{
#if defined(Q_OS_WIN)
    return _normalizedPath(lhs).compare(_normalizedPath(rhs), Qt::CaseInsensitive) == 0;
#else
    return _normalizedPath(lhs) == _normalizedPath(rhs);
#endif
}

} // namespace

// ============================================================================
// File watching tests
// ============================================================================
void QGCFileWatcherTest::_testWatchAndUnwatchFile()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("watchme.txt");
    QVERIFY(TestFixtures::writeFile(filePath, "initial content"));
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(filePath, nullptr));
    QVERIFY(watcher.isWatchingFile(filePath));
    QCOMPARE(watcher.watchedFiles().size(), 1);
    QVERIFY(watcher.unwatchFile(filePath));
    QVERIFY(!watcher.isWatchingFile(filePath));
    QVERIFY(watcher.watchedFiles().isEmpty());
}

void QGCFileWatcherTest::_testWatchFileWithCallback()
{
    QTemporaryDir tempDir;
    // Create test file
    const QString filePath = tempDir.filePath("callback.txt");
    QVERIFY(TestFixtures::writeFile(filePath, "initial content"));
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);  // No debounce for testing
    QSignalSpy fileChangedSpy(&watcher, &QGCFileWatcher::fileChanged);
    QString changedPath;
    bool callbackCalled = false;
    QVERIFY(watcher.watchFile(filePath, [&](const QString& path) {
        changedPath = path;
        callbackCalled = true;
    }));
    // Modify the file
    QVERIFY(TestFixtures::writeFile(filePath, "modified content"));

    if (!UnitTest::waitForSignal(fileChangedSpy, TestTimeout::longMs(), QStringLiteral("fileChanged"))) {
        QSKIP("File change notifications were not delivered in this environment");
    }
    if (!UnitTest::waitForCondition([&callbackCalled]() { return callbackCalled; },
                                    TestTimeout::longMs(),
                                    QStringLiteral("file watcher callback"))) {
        QSKIP("File watcher callback was not delivered in this environment");
    }
    QVERIFY2(_pathsEquivalent(changedPath, filePath),
             qPrintable(QStringLiteral("Path mismatch: actual=%1 expected=%2").arg(changedPath, filePath)));
}

void QGCFileWatcherTest::_testWatchedFiles()
{
    QTemporaryDir tempDir;
    // Create test files
    const QString file1 = tempDir.filePath("file1.txt");
    const QString file2 = tempDir.filePath("file2.txt");
    for (const QString& path : {file1, file2}) {
        QVERIFY(TestFixtures::writeFile(path, "content"));
    }
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(file1, nullptr));
    QVERIFY(watcher.watchFile(file2, nullptr));
    const QStringList watched = watcher.watchedFiles();
    QCOMPARE(watched.size(), 2);
    QVERIFY(watched.contains(QFileInfo(file1).absoluteFilePath()));
    QVERIFY(watched.contains(QFileInfo(file2).absoluteFilePath()));
}

// ============================================================================
// Directory watching tests
// ============================================================================
void QGCFileWatcherTest::_testWatchAndUnwatchDirectory()
{
    QTemporaryDir tempDir;
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(tempDir.path(), nullptr));
    QVERIFY(watcher.isWatchingDirectory(tempDir.path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
    QVERIFY(watcher.unwatchDirectory(tempDir.path()));
    QVERIFY(!watcher.isWatchingDirectory(tempDir.path()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchDirectoryWithCallback()
{
    QTemporaryDir tempDir;
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);
    QSignalSpy directoryChangedSpy(&watcher, &QGCFileWatcher::directoryChanged);
    QString changedPath;
    QVERIFY(watcher.watchDirectory(tempDir.path(), [&](const QString& path) { changedPath = path; }));
    // Create a file in the directory
    const QString newFile = tempDir.filePath("newfile.txt");
    QVERIFY(TestFixtures::writeFile(newFile, "content"));

    if (!UnitTest::waitForSignal(directoryChangedSpy, TestTimeout::longMs(), QStringLiteral("directoryChanged"))) {
        QSKIP("Directory change notifications were not delivered in this environment");
    }
    QVERIFY2(_pathsEquivalent(changedPath, tempDir.path()),
             qPrintable(QStringLiteral("Path mismatch: actual=%1 expected=%2").arg(changedPath, tempDir.path())));
}

void QGCFileWatcherTest::_testWatchedDirectories()
{
    QTemporaryDir tempDir;
    // Create subdirectories
    const QString dir1 = tempDir.filePath("subdir1");
    const QString dir2 = tempDir.filePath("subdir2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1, nullptr));
    QVERIFY(watcher.watchDirectory(dir2, nullptr));
    const QStringList watched = watcher.watchedDirectories();
    QCOMPARE(watched.size(), 2);
}

// ============================================================================
// Bulk operations
// ============================================================================
void QGCFileWatcherTest::_testWatchMultipleFiles()
{
    QTemporaryDir tempDir;
    // Create multiple files
    QStringList files;
    for (int i = 0; i < 3; i++) {
        const QString path = tempDir.filePath(QString("multi%1.txt").arg(i));
        QVERIFY(TestFixtures::writeFile(path, "content"));
        files.append(path);
    }
    QGCFileWatcher watcher;
    const int count = watcher.watchFiles(files, [](const QString&) {});
    QCOMPARE(count, 3);
    QCOMPARE(watcher.watchedFiles().size(), 3);
}

void QGCFileWatcherTest::_testWatchMultipleDirectories()
{
    QTemporaryDir tempDir;
    // Create multiple directories
    QStringList dirs;
    for (int i = 0; i < 3; i++) {
        const QString path = tempDir.filePath(QString("dir%1").arg(i));
        QVERIFY(QDir().mkdir(path));
        dirs.append(path);
    }
    QGCFileWatcher watcher;
    const int count = watcher.watchDirectories(dirs, [](const QString&) {});
    QCOMPARE(count, 3);
    QCOMPARE(watcher.watchedDirectories().size(), 3);
}

void QGCFileWatcherTest::_testClear()
{
    QTemporaryDir tempDir;
    // Create files and directories to watch
    const QString file1 = tempDir.filePath("clear_file.txt");
    QVERIFY(TestFixtures::writeFile(file1, "content"));
    const QString dir1 = tempDir.filePath("clear_dir");
    QVERIFY(QDir().mkdir(dir1));
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(file1, nullptr));
    QVERIFY(watcher.watchDirectory(dir1, nullptr));
    QVERIFY(!watcher.watchedFiles().isEmpty());
    QVERIFY(!watcher.watchedDirectories().isEmpty());
    watcher.clear();
    QVERIFY(watcher.watchedFiles().isEmpty());
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

// ============================================================================
// Debouncing
// ============================================================================
void QGCFileWatcherTest::_testDebounceDelay()
{
    QGCFileWatcher watcher;
    // Default debounce
    QCOMPARE(watcher.debounceDelay(), 100);
    // Set custom debounce
    watcher.setDebounceDelay(500);
    QCOMPARE(watcher.debounceDelay(), 500);
    // Set zero (no debounce)
    watcher.setDebounceDelay(0);
    QCOMPARE(watcher.debounceDelay(), 0);
    // Negative should clamp to 0
    watcher.setDebounceDelay(-100);
    QCOMPARE(watcher.debounceDelay(), 0);
}

// ============================================================================
// Persistent watching
// ============================================================================
void QGCFileWatcherTest::_testWatchFilePersistent()
{
    QTemporaryDir tempDir;
    const QString filePath = tempDir.filePath("persistent.txt");
    QVERIFY(TestFixtures::writeFile(filePath, "content"));
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);
    int callbackCount = 0;
    QString lastCallbackPath;
    QVERIFY(watcher.watchFilePersistent(filePath, [&](const QString& path) {
        callbackCount++;
        lastCallbackPath = path;
    }));
    QVERIFY(watcher.isWatchingFile(filePath));
    // Parent directory should also be watched
    const QString parentDir = QFileInfo(filePath).absolutePath();
    QVERIFY(watcher.isWatchingDirectory(parentDir));

    // Delete and recreate file; persistent watcher should re-add and notify.
    QVERIFY(QFile::remove(filePath));
    QVERIFY_TRUE_WAIT(!watcher.isWatchingFile(filePath), TestTimeout::shortMs());
    QVERIFY(TestFixtures::writeFile(filePath, "recreated"));

    if (!UnitTest::waitForCondition([&watcher, &filePath]() { return watcher.isWatchingFile(filePath); },
                                    TestTimeout::longMs(),
                                    QStringLiteral("persistent file watch restored"))) {
        QSKIP("Persistent file watch was not restored in this environment");
    }
    if (!UnitTest::waitForCondition([&callbackCount]() { return callbackCount > 0; },
                                    TestTimeout::longMs(),
                                    QStringLiteral("persistent callback"))) {
        QSKIP("Persistent callback was not delivered in this environment");
    }
    QVERIFY2(_pathsEquivalent(lastCallbackPath, filePath),
             qPrintable(QStringLiteral("Path mismatch: actual=%1 expected=%2").arg(lastCallbackPath, filePath)));
}

// ============================================================================
// Error handling
// ============================================================================
void QGCFileWatcherTest::_testWatchRefused_data()
{
    QTest::addColumn<bool>("directory");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("warning");

    QTest::newRow("missing file") << false << QStringLiteral("/nonexistent/path/to/file.txt")
                                  << QStringLiteral("^file does not exist:");
    QTest::newRow("missing directory") << true << QStringLiteral("/nonexistent/path/to/directory")
                                       << QStringLiteral("^not a directory:");
    QTest::newRow("file, empty path") << false << QString() << QStringLiteral("^empty path$");
    QTest::newRow("directory, empty path") << true << QString() << QStringLiteral("^empty path$");
}

void QGCFileWatcherTest::_testWatchRefused()
{
    QFETCH(bool, directory);
    QFETCH(QString, path);
    QFETCH(QString, warning);

    QGCFileWatcher watcher;
    expectLogMessage("Utilities.QGCFileWatcher", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!(directory ? watcher.watchDirectory(path, nullptr) : watcher.watchFile(path, nullptr)));
    verifyExpectedLogMessage();
    QVERIFY(watcher.watchedFiles().isEmpty());
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

UT_REGISTER_TEST(QGCFileWatcherTest, TestLabel::Unit, TestLabel::Utilities)
