#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

/// Tests for QGCCompression (decompression-only: format detection, extraction, decompression)
class QGCCompressionTest : public UnitTest
{
    Q_OBJECT

public:
    QGCCompressionTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    // Format detection
    void _testFormatDetection();
    void _testFormatDetectionFromContent();
    void _testFormatHelpers();

    // Archive extraction from Qt resources
    void _testZipFromResource();
    void _test7zFromResource();
    void _testListArchive();
    void _testListArchiveDetailed();
    void _testListArchiveNaturalSort();
    void _testGetArchiveStats();
    void _testValidateArchive();
    void _testFileExists();
    void _testExtractArchiveFiltered();
    void _testExtractSingleFile();
    void _testExtractFileData();
    void _testExtractMultipleFiles();
    void _testExtractByPattern();

    // Single-file decompression from Qt resources
    void _testDecompressFromResource();
    void _testDecompressData();
    void _testDecompressIfNeeded();

    // Progress callbacks
    void _testProgressCallbackExtract();

    // Edge cases and error handling
    void _testCorruptArchive();
    void _testNonExistentInput();

    // QIODevice-based operations
    void _testDecompressFromDevice();
    void _testExtractFromDevice();
    void _testExtractFileDataFromDevice();

    // QGCCompressionJob (async operations)
    void _testCompressionJobExtract();
    void _testCompressionJobCancel();
    void _testCompressionJobAsyncStatic();

    // QUrl utilities
    void _testToLocalPath();
    void _testIsLocalUrl();
    void _testUrlOverloads();

    // Sparse file handling
    void _testSparseFileExtraction();

    // Thread safety
    void _testThreadLocalState();
    void _testConcurrentExtractions();

    // Boundary conditions
    void _testEmptyArchive();
    void _testSingleByteFile();
    void _testLargeFileDecompression();
    void _testMaxEntriesArchive();

    // Cross-platform path handling
    void _testWindowsPathSeparators();
    void _testSpecialCharactersInPath();
    void _testUnicodePaths();

    // Benchmarks (run with --benchmark flag)
    void _benchmarkExtractZip();
    void _benchmarkExtract7z();
    void _benchmarkDecompressGzip();
    void _benchmarkDecompressXz();
    void _benchmarkConcurrentExtraction();
    void _benchmarkListArchive();

private:
    bool _compareFiles(const QString &file1, const QString &file2);

    QTemporaryDir *_tempOutputDir = nullptr;
};
