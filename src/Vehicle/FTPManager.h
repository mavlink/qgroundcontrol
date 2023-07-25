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

    friend class Vehicle;
    
public:
    FTPManager(Vehicle* vehicle);

	/// Downloads the specified file.
    ///     @param fromURI  File to download from vehicle, fully qualified path. May be in the format "mftp://[;comp=<id>]..." where the component id is specified.
    ///                     If component id is not specified MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toDir    Local directory to download file to
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    bool download(const QString& fromURI, const QString& toDir);

    /// Cancel the current operation
    /// This will emit downloadComplete() when done, and if there's currently a download in progress
    void cancel();

    static const char* mavlinkFTPScheme;

signals:
    void downloadComplete(const QString& file, const QString& errorMsg);
    
    // Signals associated with all commands
    
    /// Signalled after a command has completed
    void commandComplete(void);
    
    void commandError(const QString& msg);
    
    /// Signalled during a lengthy command to show progress
    ///     @param value Amount of progress: 0.0 = none, 1.0 = complete
    void commandProgress(float value);
	
private slots:
    void _ackOrNakTimeout(void);

private:
    typedef void (FTPManager::*StateBeginFn)            (void);
    typedef void (FTPManager::*StateAckNakFn)    (const MavlinkFTP::Request* ackOrNak);
    typedef void (FTPManager::*StateTimeoutFn)          (void);

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


    void    _mavlinkMessageReceived     (const mavlink_message_t& message);
    void    _startStateMachine          (void);
    void    _advanceStateMachine        (void);
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
    void    _emitErrorMessage           (const QString& msg);
    void    _fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str);
    void    _fillMissingBlocksWorker    (bool firstRequest);
    void    _burstReadFileWorker        (bool firstRequest);
    bool    _parseURI                   (const QString& uri, QString& parsedURI, uint8_t& compId);

    void    _terminateSessionBegin      (void);
    void    _terminateSessionAckOrNak   (const MavlinkFTP::Request* ackOrNak);
    void    _terminateSessionTimeout    (void);
    void    _terminateComplete          (void);

    Vehicle*                _vehicle;
    uint8_t                 _ftpCompId = MAV_COMP_ID_AUTOPILOT1;
    QList<StateFunctions_t> _rgStateMachine;
    DownloadState_t         _downloadState;
    QTimer                  _ackOrNakTimeoutTimer;
    int                     _currentStateMachineIndex   = -1;
    uint16_t                _expectedIncomingSeqNumber  = 0;
    
    static const int _ackOrNakTimeoutMsecs  = 1000;
    static const int _maxRetry              = 3;
};

