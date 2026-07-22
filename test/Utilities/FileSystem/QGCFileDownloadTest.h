#pragma once

#include "UnitTest.h"

class QGCFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFileDownload();
    void _testFileDownloadEmptyUrl();
    void _testFileDownloadConcurrentStartRejected();
    void _testFileDownloadStartupReentrancyRejected();
    void _testFileDownloadStartupCancellation();
    void _testFileDownloadHashVerificationFailure();
    void _testFileDownloadHashVerificationSuccess();
    void _testFileDownloadCustomOutputPath();
    void _testFileDownloadNonExistentLocalFile();
    void _testFileDownloadOutputPathIsDirectory();
    void _testFileDownloadCancelSingleCompletion();
    void _testFileDownloadMaximumSize();
    void _testFileDownloadSizeLimitReentrantStart();
    void _testFileDownloadCompletionReentrantStartPreservesResult();
    void _testFileDownloadDeferredStartFailureCompletes();
    void _testFileDownloadProgressReentrantRestart();
    void _testFileDownloadDirectConnectionDeletion();
    void _testFileDownloadFailurePreservesExistingOutput();
    void _testAutoDecompressCancelRestart();
    void _testAutoDecompressGzip();
    void _testAutoDecompressMaximumSize();
    void _testAutoDecompressDisabled();
    void _testAutoDecompressUncompressedFile();

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
    void _testDownloadDecompressAndParse();
};
