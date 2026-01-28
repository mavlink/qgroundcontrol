#pragma once

#include "TestFixtures.h"

#include <QtCore/QTemporaryDir>

#include <memory>

/// Tests for QGCArchiveWatcher (watches directories for archive files).
/// Uses TempDirTest for automatic temp directory management.
/// Creates additional output directory for extraction tests.
class QGCArchiveWatcherTest : public TempDirTest
{
    Q_OBJECT

public:
    QGCArchiveWatcherTest() = default;

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
    void _testArchiveDetectedSignal();
    void _testFilterModeArchivesOnly();
    void _testFilterModeCompressedOnly();
    void _testFilterModeBoth();

    // Auto-decompress tests
    void _testAutoDecompressArchive();
    void _testAutoDecompressCompressedFile();

    // Error handling
    void _testWatchNonExistentDirectory();
    void _testWatchEmptyPath();

private:
    void _createTestArchive(const QString &path);
    void _createTestCompressedFile(const QString &path);

    std::unique_ptr<QTemporaryDir> _outputDir;  ///< Separate output directory for extraction tests
};
