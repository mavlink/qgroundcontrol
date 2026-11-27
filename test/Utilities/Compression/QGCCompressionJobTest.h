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
    void _testDecompressionProgressOfUnknownSize();
    UT_PARAMETERIZED_TEST(_testCancelledWhileRunning);
    void _testDeletedWhileRunning();
    void _testCallbackDeletesContext();
    UT_PARAMETERIZED_TEST(_testEndedWithoutCallback);
    void _testRunsBesideGlobalPool();
};
