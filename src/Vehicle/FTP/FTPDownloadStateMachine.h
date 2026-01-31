#pragma once

#include "FTPOperationStateMachine.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>

/// FTP Download operation state machine
/// Handles: OpenFileRO → BurstReadFile → FillMissingBlocks → ResetSessions → Complete
class FTPDownloadStateMachine : public FTPOperationStateMachine
{
    Q_OBJECT

public:
    FTPDownloadStateMachine(Vehicle* vehicle, QObject* parent = nullptr);

    /// Start download operation
    /// @param compId Component ID
    /// @param remotePath Path on vehicle to download
    /// @param localDir Local directory to save to
    /// @param fileName Optional filename (derived from remotePath if empty)
    /// @param checkSize Whether to verify file size (false for dynamic files like params)
    /// @return true if operation started
    bool download(uint8_t compId, const QString& remotePath, const QString& localDir,
                  const QString& fileName = QString(), bool checkSize = true);

    void cancel() override;

signals:
    void downloadComplete(const QString& localPath, const QString& errorMsg);

protected:
    void onAckReceived(const MavlinkFTP::Request* ack) override;
    void onNakReceived(const MavlinkFTP::Request* nak) override;
    void onTimeout() override;

private:
    enum class Phase {
        Idle,
        OpeningFile,
        BurstReading,
        FillingMissingBlocks,
        ResettingSessions,
        Complete
    };

    struct MissingBlock {
        uint32_t offset;
        uint32_t size;
    };

    void _buildStateMachine();
    void _openFile();
    void _startBurstRead(bool firstRequest);
    void _fillMissingBlocks(bool firstRequest);
    void _resetSessions();
    void _finishDownload(const QString& errorMsg);

    QString _remotePath;
    QDir _localDir;
    QString _fileName;
    bool _checkSize = true;
    bool _cancelled = false;

    uint8_t _sessionId = 0;
    uint32_t _fileSize = 0;
    uint32_t _expectedOffset = 0;
    uint32_t _bytesWritten = 0;
    QList<MissingBlock> _missingBlocks;
    QFile _file;
    Phase _phase = Phase::Idle;

    QGCState* _idleState = nullptr;
    QGCState* _openingState = nullptr;
    QGCState* _burstReadingState = nullptr;
    QGCState* _fillingBlocksState = nullptr;
    QGCState* _resettingState = nullptr;
    QGCFinalState* _completeState = nullptr;
    QGCState* _errorState = nullptr;
};
