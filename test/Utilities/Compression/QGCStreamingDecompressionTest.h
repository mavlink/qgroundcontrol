#pragma once

#include "UnitTest.h"

/// Tests for QGCDecompressDevice and QGCArchiveFile
/// Streaming decompression via QIODevice interface
class QGCStreamingDecompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Both devices
    UT_PARAMETERIZED_TEST(_testReadsContent);

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
    UT_PARAMETERIZED_TEST(_testErrorString);
};
