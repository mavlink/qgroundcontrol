#pragma once

#include "MAVLinkFTP.h"

#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimer>


class Vehicle;

class FTPManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;

public:
    FTPManager(Vehicle* vehicle);

	/// Downloads the specified file.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File to download from component, fully qualified path. May be in the format "mftp://[;comp=<id>]..." where the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    ///     @param toDir      Local directory to download file to
    ///     @param filename   (optional)
    ///     @param checksize  (optional, default true) If true compare the filesize indicated in the open
    ///                       response with the transmitted filesize. If false the transmission is tftp style
    ///                       and the indicated filesize from MAVFTP fileopen response is ignored.
    ///                       This is used for the APM parameter download where the filesize is wrong due to
    ///                       a dynamic file creation on the vehicle.
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandProgress
    bool download(uint8_t fromCompId, const QString& fromURI, const QString& toDir, const QString& fileName="", bool checksize = true);

    /// Uploads a local file to the specified URI on the vehicle.
    ///     @param toCompId Component id of the component to upload to. If toCompId is MAV_COMP_ID_ALL, then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toURI    Destination file path on the vehicle, fully qualified. May include mftp:// scheme and optional component id selector.
    ///     @param fromFile Local filesystem path of the file to upload.
    /// @return true: upload started, false: error, no upload
    /// Signals uploadComplete, commandProgress
    bool upload(uint8_t toCompId, const QString& toURI, const QString& fromFile);

	/// Get the directory listing of the specified directory.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    Directory path to list from component. May be in the format "mftp://[;comp=<id>]..." where the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    /// @return true: process has started, false: error
    /// Signals listDirectoryComplete
    bool listDirectory(uint8_t fromCompId, const QString& fromURI);

    /// Deletes a file on the vehicle.
    ///     @param fromCompId Component id of the component to delete from. If fromCompId is MAV_COMP_ID_ALL, then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File path to delete on the component. May include mftp:// scheme and optional component id selector.
    /// @return true: process has started, false: error
    /// Signals deleteComplete
    bool deleteFile(uint8_t fromCompId, const QString& fromURI);

    /// Cancel the download operation
    /// This will emit downloadComplete() when done, and if there's currently a download in progress
    void cancelDownload();

    /// Cancel the list directory operation if running.
    /// This will emit listDirectoryComplete() with an error string when finished.
    void cancelListDirectory();

    /// Cancel the delete operation if running.
    /// This will emit deleteComplete() with an error string when finished.
    void cancelDelete();

    /// Cancel the upload operation
    /// This will emit uploadComplete() when done, and if there's currently an upload in progress
    void cancelUpload();

    static constexpr const char* mavlinkFTPScheme = "mftp";

signals:
    void downloadComplete       (const QString& file, const QString& errorMsg);
    void uploadComplete         (const QString& file, const QString& errorMsg);
    void listDirectoryComplete  (const QStringList& dirList, const QString& errorMsg);
    void deleteComplete         (const QString& file, const QString& errorMsg);

    /// Signalled during a lengthy command to show progress
    ///     @param value Amount of progress: 0.0 = none, 1.0 = complete
    void commandProgress(float value);

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

        bool inProgress() const { return fileSize > 0; }

        void reset() {
            sessionId       = 0;
            expectedOffset  = 0;
            bytesWritten    = 0;
            retryCount      = 0;
            fileSize        = 0;
            fullPathOnVehicle.clear();
            fileName.clear();
            rgMissingData.clear();
            file.close();
        }
    };

    struct ListDirectoryState_t {
        uint8_t     sessionId;
        uint32_t    expectedOffset;         ///< offset which should be coming next
        QString     fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        QStringList rgDirectoryList;
        int         retryCount;

        bool inProgress() const { return !fullPathOnVehicle.isEmpty(); }

        void reset() {
            sessionId       = 0;
            expectedOffset  = 0;
            fullPathOnVehicle.clear();
            rgDirectoryList.clear();
            retryCount      = 0;
        }
    };

    struct DeleteFileState_t {
        QString fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        int     retryCount = 0;

        bool inProgress() const { return !fullPathOnVehicle.isEmpty(); }

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
        bool        cancelled;

        bool inProgress() const { return file.isOpen(); }

        void reset() {
            sessionId       = 0;
            totalBytesSent  = 0;
            fileSize        = 0;
            lastChunkSize   = 0;
            retryCount      = 0;
            cancelled       = false;
            fullPathOnVehicle.clear();
            localFilePath.clear();
            file.close();
        }
    };

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
    QString _errorMsgFromNak            (const MavlinkFTP::Request* nak);
    void    _sendRequestExpectAck       (MavlinkFTP::Request* request);
    void    _downloadCompleteNoError    (void) { _downloadComplete(QString()); }
    void    _downloadComplete           (const QString& errorMsg);
    void    _fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str);
    void    _fillMissingBlocksWorker    (bool firstRequest);
    void    _burstReadFileWorker        (bool firstRequest);
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
    void    _writeFileWorker            (bool firstRequest);
    void    _uploadFinalize             (void);
    void    _uploadComplete             (const QString& errorMsg);
    void    _terminateUploadSessionBegin(void);
    void    _terminateUploadSessionAckOrNak(const MavlinkFTP::Request* ackOrNak);
    void    _terminateUploadSessionTimeout(void);

    void    _terminateSessionBegin      (void);
    void    _terminateSessionAckOrNak   (const MavlinkFTP::Request* ackOrNak);
    void    _terminateSessionTimeout    (void);
    void    _terminateComplete          (void);

    Vehicle*                _vehicle;
    uint8_t                 _ftpCompId = MAV_COMP_ID_AUTOPILOT1;
    QList<StateFunctions_t> _rgStateMachine;
    DownloadState_t         _downloadState;
    ListDirectoryState_t    _listDirectoryState;
    DeleteFileState_t       _deleteState;
    UploadState_t           _uploadState;
    QTimer                  _ackOrNakTimeoutTimer;
    int                     _currentStateMachineIndex   = -1;
    uint16_t                _expectedIncomingSeqNumber  = 0;

    static const int _ackOrNakTimeoutMsecs  = 1000;
    static const int _maxRetry              = 3;

public:
    /// Ack timeout used in unit tests (much shorter for faster tests)
    static constexpr int kTestAckTimeoutMs = 10;
    /// Maximum wait time for FTP operations in unit tests (generous for multi-packet transfers)
    static constexpr int kTestOperationMaxWaitMs = 5000;
};
