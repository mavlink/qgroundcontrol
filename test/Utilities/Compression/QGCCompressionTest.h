#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

/// Comprehensive tests for QGCCompression (format detection, ZIP, GZIP, LZMA)
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
    void _testFormatHelpers();

    // ZIP archive operations
    void _testZipRoundtrip();
    void _testZipNestedDirectories();
    void _testZipEmptyFiles();
    void _testZipFromResource();

    // Single-file decompression
    void _testDecompressGzip();
    void _testDecompressLZMA();

private:
    void _createTestData();
    bool _compareDirectories(const QString &dir1, const QString &dir2);
    bool _compareFiles(const QString &file1, const QString &file2);

    QTemporaryDir *_testDataDir = nullptr;
    QTemporaryDir *_tempOutputDir = nullptr;

    QString _testTextFile;
    QString _testBinaryFile;
    QString _testNestedDir;
    QString _testNestedFile;
};
