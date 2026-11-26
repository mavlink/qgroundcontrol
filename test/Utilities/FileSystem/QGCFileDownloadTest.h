#pragma once

#include "UnitTest.h"

class QGCFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFileDownload();
    void _testFileDownloadEmptyUrl();
    void _testFileDownloadConcurrentStartRejected();
    void _testFileDownloadHashVerificationFailure();
    void _testFileDownloadHashVerificationSuccess();
    void _testFileDownloadCustomOutputPath();
    void _testFileDownloadOutputDirectory();
    void _testFileDownloadNonExistentLocalFile();
    void _testFileDownloadOutputPathIsDirectory();
    void _testFileDownloadCancelSingleCompletion();
    UT_PARAMETERIZED_TEST(_testFileDownloadSizeLimit);
    UT_PARAMETERIZED_TEST(_testAutoDecompress);
    UT_PARAMETERIZED_TEST(_testAutoDecompressFailure);
    void _testAutoDecompressCancelled();
    UT_PARAMETERIZED_TEST(_testAutoDecompressEndedWithoutResult);
    void _testAutoDecompressCancelledWhileRunning();
    UT_PARAMETERIZED_TEST(_testAutoDecompressLeavesNewerFile);
    void _testAutoDecompressRemovesStaleDirectories();
    UT_PARAMETERIZED_TEST(_testAutoDecompressReplacesExistingFile);

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
};
