#pragma once

#include "FTPOperationStateMachine.h"

#include <QtCore/QFile>

/// FTP Upload operation state machine
/// Handles: CreateFile → WriteFile → ResetSessions → Complete
class FTPUploadStateMachine : public FTPOperationStateMachine
{
    Q_OBJECT

public:
    FTPUploadStateMachine(Vehicle* vehicle, QObject* parent = nullptr);

    /// Start upload operation
    /// @param compId Component ID
    /// @param localPath Local file to upload
    /// @param remotePath Destination path on vehicle
    /// @return true if operation started
    bool upload(uint8_t compId, const QString& localPath, const QString& remotePath);

    void cancel() override;

signals:
    void uploadComplete(const QString& remotePath, const QString& errorMsg);

protected:
    void onAckReceived(const MavlinkFTP::Request* ack) override;
    void onNakReceived(const MavlinkFTP::Request* nak) override;
    void onTimeout() override;

private:
    enum class Phase {
        Idle,
        CreatingFile,
        WritingFile,
        ResettingSessions,
        Complete
    };

    void _buildStateMachine();
    void _createFile();
    void _writeFile(bool firstRequest);
    void _resetSessions();
    void _finishUpload(const QString& errorMsg);

    QString _localPath;
    QString _remotePath;
    bool _cancelled = false;

    uint8_t _sessionId = 0;
    uint32_t _fileSize = 0;
    uint32_t _totalBytesSent = 0;
    uint32_t _lastChunkSize = 0;
    QFile _file;
    Phase _phase = Phase::Idle;

    QGCState* _idleState = nullptr;
    QGCState* _creatingState = nullptr;
    QGCState* _writingState = nullptr;
    QGCState* _resettingState = nullptr;
    QGCFinalState* _completeState = nullptr;
    QGCState* _errorState = nullptr;
};
