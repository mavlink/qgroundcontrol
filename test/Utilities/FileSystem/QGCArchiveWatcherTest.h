#pragma once

#include <memory>

#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

class QGCArchiveWatcher;
class QSignalSpy;

/// Tests for QGCArchiveWatcher (watches directories for archive files)
class QGCArchiveWatcherTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    // Configuration tests
    void _testDefaultConfiguration();
    void _testSetFilterMode();
    void _testSetAutoDecompress();
    void _testSetOutputDirectory();

    // Directory watching tests
    void _testWatchDirectory();
    void _testUnwatchDirectory();
    void _testWatchedDirectories();
    void _testClear();

    // Archive detection tests
    void _testScanDirectory();
    void _testArchiveDetectedSignal_data();
    void _testArchiveDetectedSignal();
    void _testFileBeforeWatchIsAdded_data();
    void _testFileBeforeWatchIsAdded();
    void _testWatchAfterDirectoryWasRemoved_data();
    void _testWatchAfterDirectoryWasRemoved();
    void _testWatchAfterWatchFailed();
    void _testFileCreatedEmptyIsReportedWithContent();
    void _testFileWithoutContentIsForgottenWhenUnwatched_data();
    void _testFileWithoutContentIsForgottenWhenUnwatched();
    void _testFileWithoutContentIsForgottenWithItsDirectory_data();
    void _testFileWithoutContentIsForgottenWithItsDirectory();
    void _testFilterModeArchivesOnly();
    void _testFilterModeCompressedOnly();
    void _testFilterModeBoth();

    // Auto-decompress tests
    void _testAutoDecompressArchive();
    void _testAutoDecompressCompressedFile();
    void _testRemoveAfterExtraction();
    void _testCancelExtractionResetsState();

    // Error handling
    void _testWatchNonExistentDirectory();
    void _testWatchEmptyPath();

private:
    void _createTestArchive(const QString& path);
    void _createTestCompressedFile(const QString& path);

    /// A watcher whose QFileSystemWatcher polls, as it does on Linux once the user has no inotify instance left.
    /// Null where that cannot be arranged.
    static std::unique_ptr<QGCArchiveWatcher> _createPollingWatcher();

    /// How long to wait for a new file to be reported
    /// QFileSystemWatcher hears of it at once from its native engine. Without one it compares the directory
    /// with what it last saw once a second (PollingInterval in qfilesystemwatcher_polling.cpp), so the report
    /// can take that second and the debounce delay. This leaves several times as much for a busy machine.
    static int _detectionTimeoutMs() { return TestTimeout::mediumMs(); }

    /// The archives a watcher has reported so far, in the order of the reports
    static QStringList _reportedArchives(const QSignalSpy& spy);

    QTemporaryDir _tempDir;
    QString _outputDirPath;
};
