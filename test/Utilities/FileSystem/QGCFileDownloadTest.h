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
    UT_PARAMETERIZED_TEST(_testAutoDecompress);
    UT_PARAMETERIZED_TEST(_testAutoDecompressFailure);
    void _testAutoDecompressCancelled();

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
};
