#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "FTPManager.h"

class FTPManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testDownloadUri_data();
    void _testDownloadUri();
    void _performSizeBasedTestCases_data();
    void _performSizeBasedTestCases();
    void _testLostPackets();
    void _testListDirectoryWithTime();
    void _testListDirectoryWithTimeFallback_data();
    void _testListDirectoryWithTimeFallback();
    void _testListDirectoryWithTimeFailAfterSupported();
    UT_PARAMETERIZED_TEST(_testListDirectoryFails);
    void _testListDirectoryNoSecondResponseAllowRetry();
    void _testListDirectoryCancel();
    void _testUpload();
    void _testUploadNakNamesPath();
    void _testUploadFileGoneWhenItsTurnComes();
    void _testFailedDownloadResetsSession();
    void _testDownloadClearsStaleSession();
    void _testDownloadProceedsWhenResetUnanswered();
    void _testDownloadResumesAfterSessionExpiredInBurst();
    void _testDownloadResumesAfterSessionExpiredInFill();
    void _testCancelDownloadBeforeOpen();
    void _testLateBurstPacketIsKept();
    void _testReadChunkSizeFollowsLinkType_data();
    void _testReadChunkSizeFollowsLinkType();
    void _testReadChunkSizeShrinksWhenRadioDetectedMidDownload();
    UT_PARAMETERIZED_TEST(_testDownloadFileName);
    void _testRefusedRequestQueuesNothing();
    UT_PARAMETERIZED_TEST(_testDownloadSizeLimit);
    void _testQueuedRequestsRunInOrder();
    void _testFailedRequestDoesNotDisturbNext();
    void _testRequestsOfDifferentComponents();
    void _testCancelQueuedRequest();
    UT_PARAMETERIZED_TEST(_testCancelRunningRequest);
    void _testCancelFromStarted();
    void _testOwnerDestroyedDropsItsRequests();
    void _testVehicleGoneEndsRequests();
    void _testVehicleGoneWithRequestOfItsOwnChild();
    void _testCancelAfterVehicleGone();
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
    /// Name of the MockLinkFTP size file of @a fileSize
    static QString _sizeFileName(int fileSize);
    /// Where _queueSizeFile() downloads the size file of @a fileSize to
    static QString _sizeFilePath(int fileSize);
    /// Makes a download of the MockLinkFTP size file of @a fileSize into the temporary directory
    FTPRequest* _queueSizeFile(QObject* owner, int fileSize, Result& result, QStringList* order = nullptr,
                               const QString& name = {});

    /// Removes the downloaded size file @a filename
    /// @return How it differs from the size-based test file pattern, byte for byte, which catches resumed downloads
    ///         that re-wrote or skipped a block. Empty if it does not.
    static QString _sizeFileMismatch(const QString& filename, int expectedSize);
    /// Verifies and removes a downloaded size file, see _sizeFileMismatch()
    void _verifyFileContentsAndDelete(const QString& filename, int expectedSize);
    /// Downloads a size-based test file and returns the error message of its result
    QString _downloadSizeFile(int fileSize, QString* downloadedPath = nullptr);
    /// Downloads a size-based test file, verifies and removes it
    /// @return What went wrong, empty if nothing
    QString _downloadAndVerifySizeFile(int fileSize);
};
