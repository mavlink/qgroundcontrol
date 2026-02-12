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
    void _testFileDownloadCancelSingleCompletion();
    void _testAutoDecompressGzip();
    void _testAutoDecompressDisabled();
    void _testAutoDecompressUncompressedFile();

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
    void _testDownloadDecompressAndParse();
};
