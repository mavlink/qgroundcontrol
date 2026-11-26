#pragma once

#include <functional>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "MAVLinkFTP.h"

class FTPManager;
class Vehicle;

/// An operation that FTPManager runs for a caller, made by FTPManager::download(), upload(), listDirectory() or
/// deleteFile().
///
/// The request is a child of the context given to it. It waits in the queue of its vehicle until the operations
/// before it have ended, runs, calls its callback once and deletes itself before it does. cancel(), deleting the
/// request and deleting the context all end it the same way: the callback is not called. A request that waits is
/// taken out of the queue; one that runs is ended at once, and the session it may hold on the vehicle is released.
/// A caller that has moved on needs no check in the callback.
///
/// @code
/// _request = _vehicle->ftpManager()->download(this, MAV_COMP_ID_AUTOPILOT1, uri, directory, {},
///                                             [this](const QString& file, const QString& errorMsg) { ... });
/// connect(_request, &FTPRequest::progressChanged, this, &MyClass::_showProgress);
/// // Later, if the result is no longer wanted. _request is a QPointer: the request deletes itself
/// if (_request) {
///     _request->cancel();
/// }
/// @endcode
class FTPRequest : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPRequest)

public:
    /// Ends the request without calling back
    ~FTPRequest() override;

    /// Ends the request without calling back, as deleting it does, and deletes it later
    void cancel();

signals:
    /// The operation begins on the vehicle: the operations queued before it have ended
    void started();

    /// Progress of a download or an upload, from 0.0 to 1.0
    void progressChanged(float progress);

private:
    friend class FTPManager;

    enum class Operation
    {
        Download,
        Upload,
        ListDirectory,
        DeleteFile,
    };

    FTPRequest(FTPManager* manager, QObject* context, Operation operation);

    /// Deletes the request, then calls its callback
    void _complete(const QStringList& entries, const QString& errorMsg);

    FTPManager* _manager = nullptr;  ///< Cleared once the manager is done with the request
    const Operation _operation;
    uint8_t _compId = 0;
    QString _pathOnVehicle;
    QString _resultPath;  ///< What the callback gets: the local file of a download, else _pathOnVehicle
    QString _toDir;
    QString _fileName;
    bool _checksize = true;
    qint64 _maxBytes = 0;
    QString _localFile;
    std::function<void(const QString&, const QString&)> _fileDone;
    std::function<void(const QStringList&, const QString&)> _listDone;
};

/// MAVLink FTP with one vehicle.
///
/// The operations of a vehicle run one at a time, in the order they were asked for, whichever component of the
/// vehicle they address. Each one is an FTPRequest, which reports its own result.
class FTPManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;
    friend class FTPRequest;

public:
    FTPManager(Vehicle* vehicle);
    /// Ends the requests that are left: each calls back later with an error, see FileCallback
    ~FTPManager() override;

    /// What a download, an upload or a delete calls back with: the local file of a download, the path on the vehicle
    /// of an upload or a delete, and an error message that is empty on success.
    /// A request that is left when the vehicle goes away calls back later, from the event loop, with an error.
    using FileCallback = std::function<void(const QString& path, const QString& errorMsg)>;
    /// What a directory listing calls back with: the entries, none on error, and an error message that is empty on
    /// success
    using ListCallback = std::function<void(const QStringList& entries, const QString& errorMsg)>;

    struct DownloadOptions
    {
        /// Local file name, empty for the name of the file on the vehicle. Only its last component is used.
        QString fileName{};
        /// If true compare the filesize indicated in the open response with the transmitted filesize. If false the
        /// transmission is tftp style and the indicated filesize from MAVFTP fileopen response is ignored. This is
        /// used for the APM parameter download where the filesize is wrong due to a dynamic file creation on the
        /// vehicle.
        bool checksize = true;
        /// Largest size of the file, 0 = unlimited. A file that the vehicle announces larger (with checksize), or
        /// whose data reaches beyond it, fails the download, and no part of it is kept.
        qint64 maxBytes = 0;
    };

    /// Downloads the specified file.
    ///     @param context    Owner of the request, see FTPRequest
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL,
    ///                       then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File to download from component, fully qualified path. May be in the format
    ///                       "mftp://[;comp=<id>]..." where the component id is specified. If component id is not
    ///                       specified, then the id set via fromCompId is used.
    ///     @param toDir      Local directory to download file to. Must be non-empty and already exist.
    ///     @param done       Called with the local file and the error message
    /// @return The request, queued, or nullptr for arguments that cannot be used: nothing is queued and done is not
    ///         called. Hold it in a QPointer to cancel it.
    FTPRequest* download(QObject* context, uint8_t fromCompId, const QString& fromURI, const QString& toDir,
                         const DownloadOptions& options, FileCallback done);

    /// Uploads a local file to the specified URI on the vehicle.
    ///     @param toCompId Component id of the component to upload to. If toCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toURI    Destination file path on the vehicle, fully qualified. May include mftp:// scheme and
    ///     optional component id selector.
    ///     @param fromFile Local filesystem path of the file to upload. It is opened when the upload begins.
    /// @return See download()
    FTPRequest* upload(QObject* context, uint8_t toCompId, const QString& toURI, const QString& fromFile,
                       FileCallback done);

    /// Get the directory listing of the specified directory.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    Directory path to list from component. May be in the format "mftp://[;comp=<id>]..." where
    ///     the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    /// @return See download()
    FTPRequest* listDirectory(QObject* context, uint8_t fromCompId, const QString& fromURI, ListCallback done);

    /// true when the vehicle NAK'ed kCmdListDirectoryWithTime, i.e. directory listings carry no modification times.
    bool listDirectoryWithTimeUnsupported() const { return _listDirWithTimeSupport == WithTimeSupport_t::Unsupported; }

    /// Deletes a file on the vehicle.
    ///     @param fromCompId Component id of the component to delete from. If fromCompId is MAV_COMP_ID_ALL, then
    ///     MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File path to delete on the component. May include mftp:// scheme and optional component id
    ///     selector.
    /// @return See download()
    FTPRequest* deleteFile(QObject* context, uint8_t fromCompId, const QString& fromURI, FileCallback done);

    /// Requests that wait or run
    int requestCount() const { return _queue.size() + (_current ? 1 : 0); }

    static constexpr const char* mavlinkFTPScheme = "mftp";

private slots:
    void _ackOrNakTimeout(void);

private:
    typedef void (FTPManager::*StateBeginFn)    (void);
    typedef void (FTPManager::*StateAckNakFn)   (const MavlinkFTP::Request* ackOrNak);
    typedef void (FTPManager::*StateTimeoutFn)  (void);

    struct StateFunctions_t {
        StateBeginFn    beginFn;
        StateAckNakFn   ackNakFn;
        StateTimeoutFn  timeoutFn;
    };

    struct MissingData_t {
        uint32_t offset;
        uint32_t cBytesMissing;
    };

    struct DownloadState_t {
        uint8_t                 sessionId;
        uint32_t                expectedOffset;         ///< offset which should be coming next
        uint32_t                bytesWritten;
        QList<MissingData_t>    rgMissingData;
        QString                 fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        QDir                    toDir;                  ///< Directory to download file to
        QString                 fileName;               ///< Filename (no path) for download file
        uint32_t                fileSize;               ///< Size of file being downloaded
        QFile                   file;
        int                     retryCount;
        bool                    checksize;
        int                     sessionReopenCount;     ///< Times the server expired our session and we re-opened
        int                     resumeStateIndex;       ///< State to return to after a session re-open, -1 if not resuming
        qint64 maxBytes = 0;                            ///< See DownloadOptions, 0 = unlimited

        void reset() {
            sessionId       = 0;
            expectedOffset  = 0;
            bytesWritten    = 0;
            retryCount      = 0;
            fileSize        = 0;
            sessionReopenCount = 0;
            resumeStateIndex = -1;
            maxBytes = 0;
            fullPathOnVehicle.clear();
            fileName.clear();
            rgMissingData.clear();
            file.close();
        }
    };

    struct ListDirectoryState_t {
        uint8_t                 sessionId;
        uint32_t                expectedOffset;         ///< offset which should be coming next
        QString                 fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        QStringList             rgDirectoryList;
        int                     retryCount;
        MavlinkFTP::OpCode_t    opCode;                 ///< List opcode currently in use (kCmdListDirectoryWithTime or kCmdListDirectory)

        void reset() {
            sessionId       = 0;
            expectedOffset  = 0;
            fullPathOnVehicle.clear();
            rgDirectoryList.clear();
            retryCount      = 0;
            opCode          = MavlinkFTP::kCmdListDirectory;
        }
    };

    /// Whether the vehicle supports the kCmdListDirectoryWithTime command. Cached per FTPManager
    /// instance (i.e. per vehicle) so repeated listings don't keep probing an unsupporting server.
    enum class WithTimeSupport_t { Unknown, Supported, Unsupported };

    struct DeleteFileState_t {
        QString fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        int     retryCount = 0;

        void reset() {
            fullPathOnVehicle.clear();
            retryCount = 0;
        }
    };

    struct UploadState_t {
        uint8_t     sessionId;
        uint32_t    totalBytesSent;
        uint32_t    fileSize;
        uint32_t    lastChunkSize;
        QFile       file;
        QString     fullPathOnVehicle;      ///< Fully qualified destination path on vehicle
        QString     localFilePath;          ///< Local file path being uploaded
        int         retryCount;

        void reset() {
            sessionId       = 0;
            totalBytesSent  = 0;
            fileSize        = 0;
            lastChunkSize   = 0;
            retryCount      = 0;
            fullPathOnVehicle.clear();
            localFilePath.clear();
            file.close();
        }
    };

    /// @return The request, queued, or nullptr if @a context is not set
    FTPRequest* _enqueue(QObject* context, FTPRequest::Operation operation, uint8_t compId,
                         const QString& pathOnVehicle);
    /// Starts the next request from the event loop, so that a request never begins within the call that makes it
    void _scheduleNext();
    void _startNext();
    void _startDownload();
    void _startUpload();
    void _startListDirectory();
    void _startDeleteFile();
    /// Takes a request that is ended without a callback out of the queue, or ends it if it runs
    void _dropRequest(FTPRequest* request);
    /// Ends the operation that runs, releasing a session it may hold on the vehicle
    void _abortCurrent();
    /// Ends the request that runs with its result and starts the next one
    void _finish(const QStringList& entries, const QString& errorMsg);
    void _reportProgress(float progress);
    void    _mavlinkMessageReceived     (const mavlink_message_t& message);
    void    _startStateMachine          (void);
    void    _advanceStateMachine        (void);
    void    _listDirectoryBegin         (void);
    void    _listDirectoryAckOrNak      (const MavlinkFTP::Request* ackOrNak);
    void    _listDirectoryTimeout       (void);
    void    _openFileROBegin            (void);
    void    _openFileROAckOrNak         (const MavlinkFTP::Request* ackOrNak);
    void    _openFileROTimeout          (void);
    void    _burstReadFileBegin         (void);
    void    _burstReadFileAckOrNak      (const MavlinkFTP::Request* ackOrNak);
    void    _burstReadFileTimeout       (void);
    void    _fillMissingBlocksBegin     (void);
    void    _fillMissingBlocksAckOrNak  (const MavlinkFTP::Request* ackOrNak);
    void    _fillMissingBlocksTimeout   (void);
    void    _resetSessionsBegin         (void);
    void    _resetSessionsAckOrNak      (const MavlinkFTP::Request* ackOrNak);
    void    _resetSessionsTimeout       (void);
    /// Leading ResetSessions step shared by all operations. Best effort: ack, nak and timeout all advance.
    void    _clearStaleSessionsBegin    (void);
    void    _clearStaleSessionsAckOrNak (const MavlinkFTP::Request* ackOrNak);
    void    _clearStaleSessionsTimeout  (void);
    /// Sends ResetSessions without waiting for a reply. Used when abandoning a session on failure.
    void    _sendResetSessionsNoAck     (void);
    /// Appends the leading ResetSessions step followed by rgStates to the state machine
    void    _setStateMachine            (const StateFunctions_t* rgStates, size_t cStates);
    QString _errorMsgFromNak            (const MavlinkFTP::Request* nak);
    void    _sendRequestExpectAck       (MavlinkFTP::Request* request);
    void    _downloadCompleteNoError    (void) { _downloadComplete(QString()); }
    void    _downloadComplete           (const QString& errorMsg);
    /// Writes the data of a read ack at its offset, unless it ends beyond the size limit of the download
    /// @return false if the download has failed
    bool _writeDownloadData(const MavlinkFTP::Request* ackOrNak);
    /// Fails the download because the file is larger than its size limit
    void _downloadTooLarge(void);
    void    _fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str);
    void    _fillMissingBlocksWorker    (bool firstRequest);
    void    _burstReadFileWorker        (bool firstRequest);
    /// Chunk size for the next read request, re-evaluated per request since radio detection can arrive mid-download
    uint8_t _readChunkSize              (void) const;
    /// Server expired our session mid-download: re-open the file and resume the interrupted state
    void    _downloadSessionLost        (void);
    /// Removes [offset, offset+cBytes) from the missing-data list if it lies entirely within one hole
    bool    _claimMissingRange          (uint32_t offset, uint32_t cBytes);
    void    _listDirectoryWorker        (bool firstRequest);
    bool    _parseURI                   (uint8_t fromCompId, const QString& uri, QString& parsedURI, uint8_t& compId);
    void    _listDirectoryCompleteNoError(void) { _listDirectoryComplete(QString()); }
    void    _listDirectoryComplete      (const QString& errorMsg);
    void    _deleteFileBegin            (void);
    void    _deleteFileAckOrNak         (const MavlinkFTP::Request* ackOrNak);
    void    _deleteFileTimeout          (void);
    void    _deleteCompleteNoError      (void) { _deleteComplete(QString()); }
    void    _deleteComplete             (const QString& errorMsg);

    void    _createFileBegin            (void);
    void    _createFileAckOrNak         (const MavlinkFTP::Request* ackOrNak);
    void    _createFileTimeout          (void);
    void    _writeFileBegin             (void);
    void    _writeFileAckOrNak          (const MavlinkFTP::Request* ackOrNak);
    void    _writeFileTimeout           (void);
    void _writeFileWorker(bool firstRequest);

    void _uploadCompleteNoError(void) { _uploadComplete(QString()); }
    void    _uploadComplete             (const QString& errorMsg);

    Vehicle*                _vehicle;
    QList<FTPRequest*> _queue;       ///< Requests that wait, in order
    FTPRequest* _current = nullptr;  ///< The request that runs
    bool _startScheduled = false;
    uint8_t                 _ftpCompId = MAV_COMP_ID_AUTOPILOT1;
    QList<StateFunctions_t> _rgStateMachine;
    DownloadState_t         _downloadState;
    ListDirectoryState_t    _listDirectoryState;
    DeleteFileState_t       _deleteState;
    UploadState_t           _uploadState;
    QTimer                  _ackOrNakTimeoutTimer;
    int                     _currentStateMachineIndex   = -1;
    int                     _clearStaleSessionsRetryCount = 0;
    uint16_t                _expectedIncomingSeqNumber  = 0;
    WithTimeSupport_t       _listDirWithTimeSupport     = WithTimeSupport_t::Unknown;

    // Loaded SiK radios queue up to ~2.5s; a shorter timeout misreads that as a dead stream and restarts the burst
    static const int _ackOrNakTimeoutMsecs  = 3000;
    static const int _maxRetry              = 3;

public:
    /// Bytes requested per ReadFile/BurstReadFile chunk on non-radio links: the full FTP payload.
    static constexpr uint8_t kFullReadChunkSize = sizeof(MavlinkFTP::Request::data);
    /// Chunk size on SiK/RFD radio links, so each FTP packet fits in one radio packet. A SiK radio's limit
    /// (max_data_packet_length, reported by ATI6) is 250 bytes, but only 118 with error correction enabled,
    /// which we cannot detect. On the wire: 12 MAVLink v2 + 15 FTP fixed + 91 = 118 bytes.
    static constexpr uint8_t kRadioReadChunkSize = 91;
    /// Chunk size on a signed radio link, where every packet carries a 13 byte signature.
    static constexpr uint8_t kRadioReadChunkSizeSigned = kRadioReadChunkSize - 13;
    /// Max times a download re-opens after the server expires the session (PX4 idle timer fires mid-burst)
    static constexpr int kMaxDownloadSessionReopens = 2;
    /// Keep timeout tests fast while allowing queued MockLink replies during UI startup.
    static constexpr int kTestAckTimeoutMs = 100;
    /// Maximum wait time for FTP operations in unit tests (generous for multi-packet transfers)
    static constexpr int kTestOperationMaxWaitMs = 5000;
};
