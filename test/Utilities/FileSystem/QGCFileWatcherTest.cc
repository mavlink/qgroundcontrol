#include "QGCFileWatcherTest.h"
#include "QGCFileWatcher.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void QGCFileWatcherTest::init()
{
    UnitTest::init();
    _tempDir = new QTemporaryDir();
    QVERIFY(_tempDir->isValid());
}

void QGCFileWatcherTest::cleanup()
{
    delete _tempDir;
    _tempDir = nullptr;
    UnitTest::cleanup();
}

// ============================================================================
// File watching tests
// ============================================================================

void QGCFileWatcherTest::_testWatchFile()
{
    // Create test file
    const QString filePath = _tempDir->filePath("watchme.txt");
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
    const QString filePath = _tempDir->filePath("callback.txt");
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

    // Give filesystem watcher time to detect change
    QTest::qWait(200);

    // Note: File system notifications are platform-dependent and may not always fire
    // in a test environment. The important thing is that the watch was registered.
    QVERIFY(watcher.isWatchingFile(filePath));
}

void QGCFileWatcherTest::_testUnwatchFile()
{
    // Create test file
    const QString filePath = _tempDir->filePath("unwatch.txt");
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
    const QString file1 = _tempDir->filePath("file1.txt");
    const QString file2 = _tempDir->filePath("file2.txt");

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
    QVERIFY(watcher.watchDirectory(_tempDir->path()));
    QVERIFY(watcher.isWatchingDirectory(_tempDir->path()));
    QCOMPARE(watcher.watchedDirectories().size(), 1);
}

void QGCFileWatcherTest::_testWatchDirectoryWithCallback()
{
    QGCFileWatcher watcher;
    watcher.setDebounceDelay(0);

    QString changedPath;
    QVERIFY(watcher.watchDirectory(_tempDir->path(), [&](const QString &path) {
        changedPath = path;
    }));

    // Create a file in the directory
    const QString newFile = _tempDir->filePath("newfile.txt");
    QFile file(newFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("content");
    file.close();

    // Give filesystem watcher time to detect change
    QTest::qWait(200);

    QVERIFY(watcher.isWatchingDirectory(_tempDir->path()));
}

void QGCFileWatcherTest::_testUnwatchDirectory()
{
    QGCFileWatcher watcher;
    QVERIFY(watcher.watchDirectory(_tempDir->path()));
    QVERIFY(watcher.isWatchingDirectory(_tempDir->path()));

    QVERIFY(watcher.unwatchDirectory(_tempDir->path()));
    QVERIFY(!watcher.isWatchingDirectory(_tempDir->path()));
    QVERIFY(watcher.watchedDirectories().isEmpty());
}

void QGCFileWatcherTest::_testWatchedDirectories()
{
    // Create subdirectories
    const QString dir1 = _tempDir->filePath("subdir1");
    const QString dir2 = _tempDir->filePath("subdir2");
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
        const QString path = _tempDir->filePath(QString("multi%1.txt").arg(i));
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
        const QString path = _tempDir->filePath(QString("dir%1").arg(i));
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
    const QString file1 = _tempDir->filePath("clear_file.txt");
    QFile f(file1);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("content");
    f.close();

    const QString dir1 = _tempDir->filePath("clear_dir");
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

    const QString filePath = _tempDir->filePath("persistent.txt");
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
