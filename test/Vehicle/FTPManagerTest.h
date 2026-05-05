#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class FTPManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _performTestCases_data();
    void _performTestCases();
    void _performSizeBasedTestCases_data();
    void _performSizeBasedTestCases();
    void _testLostPackets();
    void _testDownloadCancelBeforeOpen();
    void _testDownloadSessionCleanup();
    void _testDownloadResetResponseLoss();
    void _testDownloadFinalizeFailure();
    void _testListDirectory();
    void _testListDirectoryLimit();
    void _testListDirectoryParsing();
    void _testScopedListDirectoryJob();
    void _testOperationStartValidation();
    void _testListDirectoryWithTime();
    void _testListDirectoryWithTimeFallback();
    void _testListDirectoryNoResponse();
    void _testListDirectoryNakResponse();
    void _testListDirectoryNoSecondResponse();
    void _testListDirectoryNoSecondResponseAllowRetry();
    void _testListDirectoryNakSecondResponse();
    void _testListDirectoryBadSequence();
    void _testListDirectoryCancel();
    void _testUpload();
    void _testUploadResetResponseLoss();

    // Overrides from UnitTest
    void cleanup() override;

private:
    struct TestCase_t
    {
        const char* file;
    };

    void _testCaseWorker(const TestCase_t& testCase);
    void _sizeTestCaseWorker(int fileSize);
    void _verifyFileSizeAndDelete(const QString& filename, int expectedSize);

    static const TestCase_t _rgTestCases[];
};
