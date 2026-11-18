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
    void _testWatchAndUnwatchDirectory();
    void _testWatchedDirectoriesAndClear();

    // Archive detection tests
    void _testScanDirectory_data();
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

    // Auto-decompress tests
    void _testAutoDecompress_data();
    void _testAutoDecompress();
    void _testCancelExtractionResetsState();

    // Error handling
    void _testWatchRefused_data();
    void _testWatchRefused();

private:
    /// Copies a test resource to a file that can be written and removed
    void _copyResource(const QString& resource, const QString& path);

    static inline const QString _zipResource = QStringLiteral(":/unittest/manifest.json.zip");
    static inline const QString _gzipResource = QStringLiteral(":/unittest/manifest.json.gz");

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
