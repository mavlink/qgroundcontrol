#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(FTPManagerLog)

class Vehicle;
class FTPDeleteStateMachine;
class FTPDownloadStateMachine;
class FTPListDirectoryStateMachine;
class FTPUploadStateMachine;

/// Manages MAVLink FTP operations for a vehicle.
/// Provides a unified interface for download, upload, list directory, and delete operations.
class FTPManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;

public:
    FTPManager(Vehicle* vehicle);
    ~FTPManager();

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

    /// Check if any operation is in progress
    bool isOperationInProgress() const;

    /// Handle incoming MAVLink message - routes to active state machine
    void _mavlinkMessageReceived(const mavlink_message_t& message);

    static constexpr const char* mavlinkFTPScheme = "mftp";

signals:
    void downloadComplete       (const QString& file, const QString& errorMsg);
    void uploadComplete         (const QString& file, const QString& errorMsg);
    void listDirectoryComplete  (const QStringList& dirList, const QString& errorMsg);
    void deleteComplete         (const QString& file, const QString& errorMsg);

    /// Signalled during a lengthy command to show progress
    ///     @param value Amount of progress: 0.0 = none, 1.0 = complete
    void commandProgress(float value);

private:
    bool _parseURI(uint8_t fromCompId, const QString& uri, QString& parsedURI, uint8_t& compId);

    Vehicle* _vehicle;

    FTPDeleteStateMachine*        _deleteStateMachine = nullptr;
    FTPDownloadStateMachine*      _downloadStateMachine = nullptr;
    FTPListDirectoryStateMachine* _listDirectoryStateMachine = nullptr;
    FTPUploadStateMachine*        _uploadStateMachine = nullptr;
};
