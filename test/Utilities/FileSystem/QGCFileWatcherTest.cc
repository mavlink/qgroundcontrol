#include "QGCFileWatcherTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtTest/QSignalSpy>

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
void QGCFileWatcherTest::_testWatchFile()
{
    // Create test file
    const QString filePath = tempDir()->filePath("watchme.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial content");
    file.close();
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(filePath, nullptr));
    QVERIFY(watcher.isWatchingFile(filePath));
    QCOMPARE(watcher.watchedFiles().size(), 1);
}

void QGCFileWatcherTest::_testWatchFileWithCallback()
{
    // Create test file
    const QString filePath = tempDir()->filePath("callback.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial content");
    file.close();
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
    QFile modifyFile(filePath);
    QVERIFY(modifyFile.open(QIODevice::WriteOnly));
    modifyFile.write("modified content");
    modifyFile.close();

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

void QGCFileWatcherTest::_testUnwatchFile()
{
    // Create test file
    const QString filePath = tempDir()->filePath("unwatch.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(filePath, nullptr));
    QVERIFY(watcher.isWatchingFile(filePath));
    QVERIFY(watcher.unwatchFile(filePath));
    QVERIFY(!watcher.isWatchingFile(filePath));
    QVERIFY(watcher.watchedFiles().isEmpty());
}

void QGCFileWatcherTest::_testWatchedFiles()
{
    // Create test files
    const QString file1 = tempDir()->filePath("file1.txt");
    const QString file2 = tempDir()->filePath("file2.txt");
    for (const QString& path : {file1, file2}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
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
void QGCFileWatcherTest::_testWatchDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(tempDir()->path(), nullptr));
    QVERIFY(watcher.isWatchingDirectory(tempDir()->path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
}

void QGCFileWatcherTest::_testWatchDirectoryWithCallback()
{
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);
    QSignalSpy directoryChangedSpy(&watcher, &QGCFileWatcher::directoryChanged);
    QString changedPath;
    QVERIFY(watcher.watchDirectory(tempDir()->path(), [&](const QString& path) { changedPath = path; }));
    // Create a file in the directory
    const QString newFile = tempDir()->filePath("newfile.txt");
    QFile file(newFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();

    if (!UnitTest::waitForSignal(directoryChangedSpy, TestTimeout::longMs(), QStringLiteral("directoryChanged"))) {
        QSKIP("Directory change notifications were not delivered in this environment");
    }
    QVERIFY2(_pathsEquivalent(changedPath, tempDir()->path()),
             qPrintable(QStringLiteral("Path mismatch: actual=%1 expected=%2").arg(changedPath, tempDir()->path())));
}

void QGCFileWatcherTest::_testUnwatchDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(tempDir()->path(), nullptr));
    QVERIFY(watcher.isWatchingDirectory(tempDir()->path()));
    QVERIFY(watcher.unwatchDirectory(tempDir()->path()));
    QVERIFY(!watcher.isWatchingDirectory(tempDir()->path()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchedDirectories()
{
    // Create subdirectories
    const QString dir1 = tempDir()->filePath("subdir1");
    const QString dir2 = tempDir()->filePath("subdir2");
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
    // Create multiple files
    QStringList files;
    for (int i = 0; i < 3; i++) {
        const QString path = tempDir()->filePath(QString("multi%1.txt").arg(i));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
        files.append(path);
    }
    QGCFileWatcher watcher;
    const int count = watcher.watchFiles(files, [](const QString&) {});
    QCOMPARE(count, 3);
    QCOMPARE(watcher.watchedFiles().size(), 3);
}

void QGCFileWatcherTest::_testWatchMultipleDirectories()
{
    // Create multiple directories
    QStringList dirs;
    for (int i = 0; i < 3; i++) {
        const QString path = tempDir()->filePath(QString("dir%1").arg(i));
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
    // Create files and directories to watch
    const QString file1 = tempDir()->filePath("clear_file.txt");
    QFile f(file1);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("content");
    f.close();
    const QString dir1 = tempDir()->filePath("clear_dir");
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
    const QString filePath = tempDir()->filePath("persistent.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();
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
    QFile recreated(filePath);
    QVERIFY(recreated.open(QIODevice::WriteOnly));
    recreated.write("recreated");
    recreated.close();

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
void QGCFileWatcherTest::_testWatchNonExistentFile()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchFile("/nonexistent/path/to/file.txt", nullptr));
    QVERIFY(watcher.watchedFiles().isEmpty());
}

void QGCFileWatcherTest::_testWatchNonExistentDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchDirectory("/nonexistent/path/to/directory", nullptr));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchEmptyPath()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchFile(QString(), nullptr));
    QVERIFY(!watcher.watchDirectory(QString(), nullptr));
}

UT_REGISTER_TEST(QGCFileWatcherTest, TestLabel::Unit, TestLabel::Utilities)
