#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

/// Tests for QGCDecompressDevice and QGCArchiveFile
/// Streaming decompression via QIODevice interface
class QGCStreamingDecompressionTest : public UnitTest
{
    Q_OBJECT

public:
    QGCStreamingDecompressionTest() = default;

private slots:
    void init() override;
    void cleanup() override;

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

private:
    QTemporaryDir *_tempDir = nullptr;
};
