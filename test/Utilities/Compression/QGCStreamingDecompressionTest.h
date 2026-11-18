#pragma once

#include "UnitTest.h"

/// Tests for QGCDecompressDevice and QGCArchiveFile
/// Streaming decompression via QIODevice interface
class QGCStreamingDecompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Both devices
    void _testReadsContent_data();
    void _testReadsContent();

    // QGCDecompressDevice tests
    void _testDecompressDeviceWithQTextStream();
    void _testDecompressDeviceWithQJsonDocument();
    void _testDecompressDeviceErrors();
    void _testDecompressDeviceFormatInfo();

    // QGCArchiveFile tests
    void _testArchiveFileWithQTextStream();
    void _testArchiveFileWithQJsonDocument();
    void _testArchiveFileNotFound();
    void _testArchiveFileErrors();
    void _testArchiveFileMetadata();

    // Edge cases
    void _testPartialReads();
    void _testMultipleOpens();
    void _testErrorString_data();
    void _testErrorString();
};
