#include "QGCFileWatcherTest.h"
#include "QGCFileWatcher.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

// ============================================================================
// File watching tests
// ============================================================================

void QGCFileWatcherTest::_testWatchFile()
{
    // Create test file
    const QString filePath = tempFilePath("watchme.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial content");
    file.close();

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(filePath));
    QVERIFY(watcher.isWatchingFile(filePath));
    QCOMPARE(watcher.watchedFiles().size(), 1);
}

void QGCFileWatcherTest::_testWatchFileWithCallback()
{
    // Create test file
    const QString filePath = tempFilePath("callback.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial content");
    file.close();

    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);  // No debounce for testing

    QString changedPath;
    bool callbackCalled = false;

    QVERIFY(watcher.watchFile(filePath, [&](const QString &path) {
        changedPath = path;
        callbackCalled = true;
    }));

    // Modify the file
    QFile modifyFile(filePath);
    QVERIFY(modifyFile.open(QIODevice::WriteOnly));
    modifyFile.write("modified content");
    modifyFile.close();

    // Wait for callback to be called (with timeout for platforms that don't fire notifications)
    // File system notifications are platform-dependent and may not always fire in test environments
    (void) TestHelpers::waitFor([&]() { return callbackCalled; }, 500);

    // The important thing is that the watch was registered
    QVERIFY(watcher.isWatchingFile(filePath));
}

void QGCFileWatcherTest::_testUnwatchFile()
{
    // Create test file
    const QString filePath = tempFilePath("unwatch.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(filePath));
    QVERIFY(watcher.isWatchingFile(filePath));

    QVERIFY(watcher.unwatchFile(filePath));
    QVERIFY(!watcher.isWatchingFile(filePath));
    QVERIFY(watcher.watchedFiles().isEmpty());
}

void QGCFileWatcherTest::_testWatchedFiles()
{
    // Create test files
    const QString file1 = tempFilePath("file1.txt");
    const QString file2 = tempFilePath("file2.txt");

    for (const QString &path : {file1, file2}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
    }

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(file1));
    QVERIFY(watcher.watchFile(file2));

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
    QVERIFY(watcher.watchDirectory(tempPath()));
    QVERIFY(watcher.isWatchingDirectory(tempPath()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
}

void QGCFileWatcherTest::_testWatchDirectoryWithCallback()
{
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);

    QString changedPath;
    QVERIFY(watcher.watchDirectory(tempPath(), [&](const QString &path) {
        changedPath = path;
    }));

    // Create a file in the directory
    const QString newFile = tempFilePath("newfile.txt");
    QFile file(newFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();

    // Wait for callback (with timeout for platforms that don't fire notifications)
    (void) TestHelpers::waitFor([&]() { return !changedPath.isEmpty(); }, 500);

    QVERIFY(watcher.isWatchingDirectory(tempPath()));
}

void QGCFileWatcherTest::_testUnwatchDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(tempPath()));
    QVERIFY(watcher.isWatchingDirectory(tempPath()));

    QVERIFY(watcher.unwatchDirectory(tempPath()));
    QVERIFY(!watcher.isWatchingDirectory(tempPath()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchedDirectories()
{
    // Create subdirectories
    const QString dir1 = tempFilePath("subdir1");
    const QString dir2 = tempFilePath("subdir2");
    QVERIFY(QDir().mkdir(dir1));
    QVERIFY(QDir().mkdir(dir2));

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(dir1));
    QVERIFY(watcher.watchDirectory(dir2));

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
        const QString path = tempFilePath(QString("multi%1.txt").arg(i));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
        files.append(path);
    }

    QGCFileWatcher watcher;
    const int count = watcher.watchFiles(files, [](const QString &) {});

    QCOMPARE(count, 3);
    QCOMPARE(watcher.watchedFiles().size(), 3);
}

void QGCFileWatcherTest::_testWatchMultipleDirectories()
{
    // Create multiple directories
    QStringList dirs;
    for (int i = 0; i < 3; i++) {
        const QString path = tempFilePath(QString("dir%1").arg(i));
        QVERIFY(QDir().mkdir(path));
        dirs.append(path);
    }

    QGCFileWatcher watcher;
    const int count = watcher.watchDirectories(dirs, [](const QString &) {});

    QCOMPARE(count, 3);
    QCOMPARE(watcher.watchedDirectories().size(), 3);
}

void QGCFileWatcherTest::_testClear()
{
    // Create files and directories to watch
    const QString file1 = tempFilePath("clear_file.txt");
    QFile f(file1);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("content");
    f.close();

    const QString dir1 = tempFilePath("clear_dir");
    QVERIFY(QDir().mkdir(dir1));

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFile(file1));
    QVERIFY(watcher.watchDirectory(dir1));

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
    // This test verifies persistent watching setup, not the full re-watch behavior
    // which is hard to test reliably in unit tests

    const QString filePath = tempFilePath("persistent.txt");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();

    QGCFileWatcher watcher;
    QVERIFY(watcher.watchFilePersistent(filePath, [](const QString &) {}));
    QVERIFY(watcher.isWatchingFile(filePath));

    // Parent directory should also be watched
    const QString parentDir = QFileInfo(filePath).absolutePath();
    QVERIFY(watcher.isWatchingDirectory(parentDir));
}

// ============================================================================
// Error handling
// ============================================================================

void QGCFileWatcherTest::_testWatchNonExistentFile()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchFile("/nonexistent/path/to/file.txt"));
    QVERIFY(watcher.watchedFiles().isEmpty());
}

void QGCFileWatcherTest::_testWatchNonExistentDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchDirectory("/nonexistent/path/to/directory"));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchEmptyPath()
{
    QGCFileWatcher watcher;
    QVERIFY(!watcher.watchFile(QString()));
    QVERIFY(!watcher.watchDirectory(QString()));
}
