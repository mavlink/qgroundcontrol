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
    void _testListDirectory();
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
    void _testFailedDownloadResetsSession();
    void _testDownloadClearsStaleSession();
    void _testDownloadProceedsWhenResetUnanswered();
    void _testDownloadResumesAfterSessionExpiredInBurst();
    void _testDownloadResumesAfterSessionExpiredInFill();
    void _testCancelDownloadIsImmediate();
    void _testCancelDownloadBeforeOpen();
    void _testLateBurstPacketIsKept();
    void _testInvalidUriDoesNotBlockNextOperation();

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
    /// Verifies the size-based test file pattern byte for byte, catching resumed downloads that re-wrote or skipped a block
    void _verifyFileContentsAndDelete(const QString& filename, int expectedSize);
    /// Downloads a size-based test file and returns the errorMsg from downloadComplete
    QString _downloadSizeFile(int fileSize, QString* downloadedPath = nullptr);

    static const TestCase_t _rgTestCases[];
};
