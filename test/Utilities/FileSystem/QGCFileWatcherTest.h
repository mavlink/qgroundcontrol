#pragma once

#include "BaseClasses/TempDirectoryTest.h"

/// Tests for QGCFileWatcher (QFileSystemWatcher wrapper with callbacks)
class QGCFileWatcherTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    // File watching tests
    void _testWatchFile();
    void _testWatchFileWithCallback();
    void _testUnwatchFile();
    void _testWatchedFiles();

    // Directory watching tests
    void _testWatchDirectory();
    void _testWatchDirectoryWithCallback();
    void _testUnwatchDirectory();
    void _testWatchedDirectories();

    // Bulk operations
    void _testWatchMultipleFiles();
    void _testWatchMultipleDirectories();
    void _testClear();

    // Debouncing
    void _testDebounceDelay();

    // Persistent watching
    void _testWatchFilePersistent();

    // Error handling
    void _testWatchNonExistentFile();
    void _testWatchNonExistentDirectory();
    void _testWatchEmptyPath();
};
