/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QObject>
#include <QDir>
#include <QTimer>
#include <QQueue>

#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(FTPManagerLog)

class Vehicle;

class FTPManager : public QObject
{
    Q_OBJECT
    
public:
    FTPManager(Vehicle* vehicle);

	/// Downloads the specified file.
    ///     @param from     File to download from vehicle, fully qualified path. May be in the format mavlinkftp://...
    ///     @param toDir    Local directory to download file to
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    bool download(const QString& from, const QString& toDir);
	
	/// Stream downloads the specified file.
    ///     @param from     File to download from UAS, fully qualified path. May be in the format mavlinkftp://...
    ///     @param toDir    Local directory to download file to
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    bool burstDownload(const QString& from, const QString& toDir);
	
	/// Lists the specified directory. Emits listEntry signal for each entry, followed by listComplete signal.
	///		@param dirPath Fully qualified path to list
	void listDirectory(const QString& dirPath);
	
    /// Upload the specified file to the specified location
    void upload(const QString& toPath, const QFileInfo& uploadFile);
    
    /// Create a remote directory
    void createDirectory(const QString& directory);

    void mavlinkMessageReceived(mavlink_message_t message);

signals:
    void downloadComplete   (const QString& file, const QString& errorMsg);
    void uploadComplete     (const QString& errorMsg);

    /// Signalled to indicate a new directory entry was received.
    void listEntry(const QString& entry);
    
    // Signals associated with all commands
    
    /// Signalled after a command has completed
    void commandComplete(void);
    
    void commandError(const QString& msg);
    
    /// Signalled during a lengthy command to show progress
    ///     @param value Amount of progress: 0.0 = none, 1.0 = complete
    void commandProgress(int value);
	
private slots:
	void _ackTimeout(void);

private:
    bool    _sendOpcodeOnlyCmd     (MavlinkFTP::OpCode_t opcode, MavlinkFTP::OpCode_t newWaitState);
    void    _setupAckTimeout       (void);
    void    _clearAckTimeout       (void);
    void    _emitErrorMessage      (const QString& msg);
    void    _emitListEntry         (const QString& entry);
    void    _sendRequestExpectAck  (MavlinkFTP::Request* request);
    void    _sendRequestNoAck      (MavlinkFTP::Request* request);
    void    _sendMessageOnLink     (LinkInterface* link, mavlink_message_t message);
    void    _fillRequestWithString (MavlinkFTP::Request* request, const QString& str);
    void    _handlOpenFileROAck    (MavlinkFTP::Request* ack);
    void    _handleReadFileAck     (MavlinkFTP::Request* ack, bool burstReadFile);
    void    _listAckResponse       (MavlinkFTP::Request* listAck);
    void    _createAckResponse     (MavlinkFTP::Request* createAck);
    void    _writeAckResponse      (MavlinkFTP::Request* writeAck);
    void    _writeFileDatablock    (void);
    void    _sendListCommand       (void);
    void    _sendResetCommand      (void);
    void    _downloadComplete      (const QString& errorMsg);
    void    _uploadComplete        (const QString& errorMsg);
    bool    _downloadWorker        (const QString& from, const QString& toDir, bool burstReadFile);
    void    _requestMissingData    (void);
    
    MavlinkFTP::OpCode_t    _waitState              = MavlinkFTP::kCmdNone;     ///< Current operation of state machine
    MavlinkFTP::OpCode_t    _openFileType           = MavlinkFTP::kCmdReadFile; ///< Type of read to use after open file succeeds
    QTimer                  _ackTimer;                                          ///< Used to signal a timeout waiting for an ack
    int                     _ackNumTries;                                       ///< current number of tries
    Vehicle*                _vehicle;
    LinkInterface*          _dedicatedLink          = nullptr;                  ///< Link to use for communication
    MavlinkFTP::Request     _lastOutgoingRequest;                               ///< contains the last outgoing packet
    unsigned                _listOffset;                                        ///< offset for the current List operation
    QString                 _listPath;                                          ///< path for the current List operation
    uint8_t                 _activeSession          = 0;                        ///< currently active session, 0 for none
    uint32_t                _readOffset;                                        ///< current read offset
    
    uint32_t    _writeOffset;               ///< current write offset
    uint32_t    _writeSize;                 ///< current write data size
    uint32_t    _writeFileSize;             ///< Size of file being uploaded
    QByteArray  _writeFileAccumulator;      ///< Holds file being uploaded
    
    struct MissingData {
        uint32_t offset;
        uint32_t size;
    };
    uint32_t            _requestedDownloadOffset;               ///< current download offset
    QByteArray          _readFileAccumulator;                   ///< Holds file being downloaded
    QDir                _readFileDownloadDir;                   ///< Directory to download file to
    QString             _readFileDownloadFilename;              ///< Filename (no path) for download file
    int                 _downloadFileSize;                      ///< Size of file being downloaded

    static const int _ackTimerTimeoutMsecs = 5000;
    static const int _ackTimerMaxRetries = 6;
};

