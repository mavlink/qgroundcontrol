#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "FTPManager.h"

class FTPManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _performTestCases_data();
    void _performTestCases();
    void _testDownloadUri_data();
    void _testDownloadUri();
    void _performSizeBasedTestCases_data();
    void _performSizeBasedTestCases();
    void _testLostPackets();
    void _testListDirectory();
    void _testListDirectoryWithTime();
    void _testListDirectoryWithTimeFallback_data();
    void _testListDirectoryWithTimeFallback();
    void _testListDirectoryWithTimeFailAfterSupported();
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
    void _testReadChunkSizeFollowsLinkType_data();
    void _testReadChunkSizeFollowsLinkType();
    void _testReadChunkSizeShrinksWhenRadioDetectedMidDownload();
    UT_PARAMETERIZED_TEST(_testDownloadFileName);
    void _testControllerNamesRefusedFileName();
    void _testDownloadRejectsMissingDestination();
    UT_PARAMETERIZED_TEST(_testDownloadSizeLimit);
    void _testQueuedRequestsRunInOrder();
    void _testFailedRequestDoesNotDisturbNext();
    void _testCancelQueuedRequest();
    void _testCancelRunningRequest();
    void _testOwnerDestroyedDropsItsRequests();
    void _testVehicleGoneEndsRequests();
    void _testMetadataAndCameraDefinitionTogether();

    // Overrides from UnitTest
    void cleanup() override;

private:
    /// What a request called back with
    struct Result
    {
        int calls = 0;
        QString path;
        QStringList entries;
        QString error;
    };

    /// Records the result in @a result and, if @a order is set, appends @a name to it
    static FTPManager::FileCallback _record(Result& result, QStringList* order = nullptr, const QString& name = {});
    static FTPManager::ListCallback _recordList(Result& result);
    /// Makes a download of the MockLinkFTP size file of @a fileSize into the temporary directory
    FTPRequest* _queueSizeFile(QObject* owner, int fileSize, Result& result, QStringList* order = nullptr,
                               const QString& name = {});

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
