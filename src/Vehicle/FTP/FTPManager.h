#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <cstdint>
#include <memory>
#include <optional>

#include "MAVLinkFTP.h"
class Vehicle;
class FTPDeleteJob;
class FTPDownloadJob;
class FTPJob;
class FTPListDirectoryJob;
class FTPUploadJob;

class FTPManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;
#ifdef QGC_UNITTEST_BUILD
    friend class FTPManagerTest;
#endif

public:
    enum class StartError : uint8_t
    {
        None,
        Busy,
        InvalidUri,
        RemotePathTooLong,
        InvalidArgument,
        SourceMissing,
        SourceTooLarge,
        SourceOpenFailed,
    };
    Q_ENUM(StartError)

    template <typename JobType>
    class StartResult
    {
    public:
        [[nodiscard]] static StartResult success(JobType* job)
        {
            return StartResult(job, job ? StartError::None : StartError::InvalidArgument);
        }

        [[nodiscard]] static StartResult failure(StartError error)
        {
            return StartResult(nullptr, error == StartError::None ? StartError::InvalidArgument : error);
        }

        [[nodiscard]] bool succeeded() const { return _error == StartError::None; }

        [[nodiscard]] explicit operator bool() const { return succeeded(); }

        [[nodiscard]] JobType* job() const { return _job.data(); }

        [[nodiscard]] StartError error() const { return _error; }

    private:
        StartResult(JobType* job, StartError error) : _job(job), _error(error) {}

        QPointer<JobType> _job;
        StartError _error = StartError::InvalidArgument;
    };

    using DownloadStartResult = StartResult<FTPDownloadJob>;
    using UploadStartResult = StartResult<FTPUploadJob>;
    using ListDirectoryStartResult = StartResult<FTPListDirectoryJob>;
    using DeleteStartResult = StartResult<FTPDeleteJob>;

    enum class ExistingFilePolicy : uint8_t
    {
        Replace,
        FailIfExists,
    };

    FTPManager(Vehicle* vehicle);
    ~FTPManager() override;

    /// Starts a download and returns the manager-owned job plus any synchronous start error.
    /// Callers must not delete or reparent a returned job.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File to download from component, fully qualified path. May be in the format
    ///     "mftp://[;comp=<id>]..." where the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    ///     @param toDir      Local directory to download file to
    ///     @param filename   (optional)
    ///     @param checksize  (optional, default true) If true compare the filesize indicated in the open
    ///                       response with the transmitted filesize. If false the transmission is tftp style
    ///                       and the indicated filesize from MAVFTP fileopen response is ignored.
    ///                       This is used for the APM parameter download where the filesize is wrong due to
    ///                       a dynamic file creation on the vehicle.
    ///     @param existingFilePolicy Whether an existing local destination may be replaced.
    ///     @param maximumFileSize Optional upper bound for both the reported size and received data ranges.
    ///                           Required when checksize is false.
    [[nodiscard]] DownloadStartResult startDownload(uint8_t fromCompId, const QString& fromURI, const QString& toDir,
                                                    const QString& fileName = "", bool checksize = true,
                                                    ExistingFilePolicy existingFilePolicy = ExistingFilePolicy::Replace,
                                                    std::optional<uint32_t> maximumFileSize = std::nullopt);

    /// Uploads a local file to the specified URI on the vehicle and reports synchronous start errors.
    ///     @param toCompId Component id of the component to upload to. If toCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toURI    Destination file path on the vehicle, fully qualified. May include mftp:// scheme and
    ///     optional component id selector.
    ///     @param fromFile Local filesystem path of the file to upload.
    [[nodiscard]] UploadStartResult startUpload(uint8_t toCompId, const QString& toURI, const QString& fromFile);

    /// Gets the directory listing of the specified directory and reports synchronous start errors.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    Directory path to list from component. May be in the format "mftp://[;comp=<id>]..." where
    ///     the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    ///     @param maxEntries Maximum entries to accumulate, or zero for no limit.
    [[nodiscard]] ListDirectoryStartResult startListDirectory(uint8_t fromCompId, const QString& fromURI,
                                                              int maxEntries = 0);

    /// Deletes a file on the vehicle and reports synchronous start errors.
    ///     @param fromCompId Component id of the component to delete from. If fromCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File path to delete on the component. May include mftp:// scheme and optional component id
    ///     selector.
    [[nodiscard]] DeleteStartResult startDeleteFile(uint8_t fromCompId, const QString& fromURI);

private slots:
    void _ackOrNakTimeout(void);

private:
    friend class FTPJob;

    typedef void (FTPManager::*StateBeginFn)(void);
    typedef void (FTPManager::*StateAckNakFn)(const MavlinkFTP::Request* ackOrNak);
    typedef void (FTPManager::*StateTimeoutFn)(void);

    struct StateFunctions_t
    {
        StateBeginFn beginFn;
        StateAckNakFn ackNakFn;
        StateTimeoutFn timeoutFn;
    };

    class DownloadOperation;
    class UploadOperation;
    class ListDirectoryOperation;
    class DeleteOperation;

    /// Whether the vehicle supports MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME. Cached per FTPManager
    /// instance (i.e. per vehicle) so repeated listings don't keep probing an unsupporting server.
    enum class WithTimeSupport_t
    {
        Unknown,
        Supported,
        Unsupported
    };

    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _startStateMachine(void);
    void _advanceStateMachine(void);
    void _listDirectoryBegin(void);
    void _listDirectoryAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _listDirectoryTimeout(void);
    void _openFileROBegin(void);
    void _openFileROAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _openFileROTimeout(void);
    void _burstReadFileBegin(void);
    void _burstReadFileAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _burstReadFileTimeout(void);
    void _fillMissingBlocksBegin(void);
    void _fillMissingBlocksAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _fillMissingBlocksTimeout(void);
    void _resetSessionsBegin(void);
    void _resetSessionsAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _resetSessionsTimeout(void);
    MavlinkFTP::ResponseValidationResult _validateResponseEnvelope(const MavlinkFTP::Request* response,
                                                                   MAV_FTP_OPCODE expectedRequestOpCode,
                                                                   const char* handlerName) const;
    bool _validateDownloadDataRange(const MavlinkFTP::Request* response,
                                    std::optional<uint32_t> remainingBytes = std::nullopt);
    QString _errorMsgFromNak(const MavlinkFTP::Request* nak);
    void _sendRequestExpectAck(MavlinkFTP::Request* request);
    void _cancelJob(FTPJob* job);
    void cancelDownload();
    void cancelListDirectory();
    void cancelDelete();
    void cancelUpload();
    void _emitCommandProgress(float value);

    void _downloadCompleteNoError(void) { _downloadComplete(QString()); }

    void _downloadComplete(const QString& errorMsg, const QString& warningMsg = QString());
    void _terminateDownload(const QString& errorMsg);
    void _fillMissingBlocksWorker(bool firstRequest);
    void _burstReadFileWorker(bool firstRequest);
    void _listDirectoryWorker(bool firstRequest);

    void _listDirectoryCompleteNoError(void) { _listDirectoryComplete(QString()); }

    void _listDirectoryComplete(const QString& errorMsg);
    void _deleteFileBegin(void);
    void _deleteFileAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _deleteFileTimeout(void);

    void _deleteCompleteNoError(void) { _deleteComplete(QString()); }

    void _deleteComplete(const QString& errorMsg);

    void _createFileBegin(void);
    void _createFileAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _createFileTimeout(void);
    void _writeFileBegin(void);
    void _writeFileAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _writeFileTimeout(void);
    void _writeFileWorker(bool firstRequest);
    void _uploadFinalize(void);
    void _uploadComplete(const QString& errorMsg);
    void _terminateUploadSessionBegin(void);
    void _terminateUploadSessionAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _terminateUploadSessionTimeout(void);

    void _terminateSessionBegin(void);
    void _terminateSessionAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void _terminateSessionTimeout(void);
    void _terminateComplete(void);

    Vehicle* _vehicle;
    uint8_t _ftpCompId = MAV_COMP_ID_AUTOPILOT1;
    QList<StateFunctions_t> _rgStateMachine;
    std::unique_ptr<DownloadOperation> _downloadOperation;
    std::unique_ptr<ListDirectoryOperation> _listDirectoryOperation;
    std::unique_ptr<DeleteOperation> _deleteOperation;
    std::unique_ptr<UploadOperation> _uploadOperation;
    QPointer<FTPJob> _activeJob;
    int _resetSessionsRetryCount = 0;
    QTimer _ackOrNakTimeoutTimer;
    int _currentStateMachineIndex = -1;
    uint16_t _expectedIncomingSeqNumber = 0;
    WithTimeSupport_t _listDirWithTimeSupport = WithTimeSupport_t::Unknown;

    static const int _ackOrNakTimeoutMsecs = 1000;
    static const int _maxRetry = 3;

public:
    /// Ack timeout used in unit tests (much shorter for faster tests)
    static constexpr int kTestAckTimeoutMs = 10;
    /// Maximum wait time for FTP operations in unit tests (generous for multi-packet transfers)
    static constexpr int kTestOperationMaxWaitMs = 5000;
};
