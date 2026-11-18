#pragma once

#include "UnitTest.h"

/// Tests for QGCFileWatcher (QFileSystemWatcher wrapper with callbacks)
class QGCFileWatcherTest : public UnitTest
{
    Q_OBJECT

private slots:
    // File watching tests
    void _testWatchAndUnwatchFile();
    void _testWatchFileWithCallback();
    void _testWatchedFiles();

    // Directory watching tests
    void _testWatchAndUnwatchDirectory();
    void _testWatchDirectoryWithCallback();
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
    void _testWatchRefused_data();
    void _testWatchRefused();
};
