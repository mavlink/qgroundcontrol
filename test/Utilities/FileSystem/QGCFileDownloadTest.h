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
    void _testFileDownloadNonExistentLocalFile();
    void _testFileDownloadOutputPathIsDirectory();
    void _testFileDownloadCancelSingleCompletion();
    void _testFileDownloadSameFileNameStaysSeparate();
    void _testFileDownloadLocalSourceInTempDirSurvives();
    void _testFileDownloadFailureRemovesPartialFile_data();
    void _testFileDownloadFailureRemovesPartialFile();
    void _testFileDownloadHttpErrorStatus_data();
    void _testFileDownloadHttpErrorStatus();
    void _testFileDownloadFollowsRedirect_data();
    void _testFileDownloadFollowsRedirect();
    void _testFileDownloadRefusesRedirect_data();
    void _testFileDownloadRefusesRedirect();
    void _testStartToMemoryDeliversContent_data();
    void _testStartToMemoryDeliversContent();
    void _testStartToMemoryFailsAsStart_data();
    void _testStartToMemoryFailsAsStart();
    void _testStartToMemoryLimit_data();
    void _testStartToMemoryLimit();
    void _testStartToMemoryAutoDecompress_data();
    void _testStartToMemoryAutoDecompress();
    void _testStartToMemoryCancel();
    void _testAutoDecompressGzip();
    void _testAutoDecompressDisabled();
    void _testAutoDecompressUncompressedFile();
    void _testAutoDecompressBodyNotCompressed_data();
    void _testAutoDecompressBodyNotCompressed();

    // Integration tests (download + compression)
    void _testDownloadAndExtractZip();
    void _testDownloadDecompressAndParse();
};
