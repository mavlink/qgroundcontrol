#pragma once

#include "PortableTest.h"

/// Tests for CompressionJob: a Compression operation on a worker thread
class QGCCompressionJobTest : public PortableTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testExtraction);
    UT_PARAMETERIZED_TEST(_testExtractionFilterThrows);
    UT_PARAMETERIZED_TEST(_testDecompression);
    UT_PARAMETERIZED_TEST(_testDecompressionRefusesOutputPath);
    void _testDecompressionOfInputThatCannotBeTaken();
    void _testDecompressionProgressOfUnknownSize();
    UT_PARAMETERIZED_TEST(_testDecompressionLeavesNewerInput);
    UT_PARAMETERIZED_TEST(_testCancelledWhileRunning);
    UT_PARAMETERIZED_TEST(_testEndedWithoutCallback);
};
