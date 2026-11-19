#pragma once

#include "UnitTest.h"

class QGCFileDownloadTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testFileDownload);
    void _testFileDownloadEmptyUrl();
    void _testFileDownloadConcurrentStartRejected();
    void _testFileDownloadHashVerificationFailure();
    void _testFileDownloadOutputPathIsDirectory();
    UT_PARAMETERIZED_TEST(_testFileDownloadCancel);
    void _testFileDownloadSameFileNameStaysSeparate();
    void _testFileDownloadLocalSourceInTempDirSurvives();
    UT_PARAMETERIZED_TEST(_testFileDownloadFailureRemovesPartialFile);
    UT_PARAMETERIZED_TEST(_testFileDownloadHttpErrorStatus);
    UT_PARAMETERIZED_TEST(_testFileDownloadFollowsRedirect);
    UT_PARAMETERIZED_TEST(_testFileDownloadRefusesRedirect);
    UT_PARAMETERIZED_TEST(_testStartToMemoryDeliversContent);
    UT_PARAMETERIZED_TEST(_testStartToMemoryFailsAsStart);
    UT_PARAMETERIZED_TEST(_testStartToMemoryLimit);
    UT_PARAMETERIZED_TEST(_testAutoDecompress);
    UT_PARAMETERIZED_TEST(_testAutoDecompressBodyNotCompressed);

private:
    /// Expects a warning for each "category|pattern"
    void _expectWarnings(const QStringList& warnings);
    void _verifyWarnings(const QStringList& warnings);
};
