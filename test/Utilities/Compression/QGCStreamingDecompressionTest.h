#pragma once

#include "TestFixtures.h"

/// Tests for QGCDecompressDevice and QGCArchiveFile.
/// Streaming decompression via QIODevice interface.
/// Uses TempDirTest for automatic temp directory management.
class QGCStreamingDecompressionTest : public TempDirTest
{
    Q_OBJECT

public:
    QGCStreamingDecompressionTest() = default;

private slots:

    // QGCDecompressDevice tests
    void _testDecompressDeviceFromFile();
    void _testDecompressDeviceFromQBuffer();
    void _testDecompressDeviceWithQTextStream();
    void _testDecompressDeviceWithQJsonDocument();
    void _testDecompressDeviceFormats();  // gz, xz, zst, bz2, lz4
    void _testDecompressDeviceFromQtResource();
    void _testDecompressDeviceErrors();
    void _testDecompressDeviceFormatInfo();

    // QGCArchiveFile tests
    void _testArchiveFileFromZip();
    void _testArchiveFileFrom7z();
    void _testArchiveFileFromQBuffer();
    void _testArchiveFileWithQTextStream();
    void _testArchiveFileWithQJsonDocument();
    void _testArchiveFileNotFound();
    void _testArchiveFileFromQtResource();
    void _testArchiveFileErrors();
    void _testArchiveFileMetadata();

    // Edge cases
    void _testPartialReads();
    void _testMultipleOpens();
};
