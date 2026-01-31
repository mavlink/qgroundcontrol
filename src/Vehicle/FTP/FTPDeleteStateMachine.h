#pragma once

#include "FTPOperationStateMachine.h"

/// FTP Delete operation state machine
class FTPDeleteStateMachine : public FTPOperationStateMachine
{
    Q_OBJECT

public:
    FTPDeleteStateMachine(Vehicle* vehicle, QObject* parent = nullptr);

    /// Start delete operation
    /// @param compId Component ID (MAV_COMP_ID_ALL uses AUTOPILOT1)
    /// @param path Path on vehicle to delete
    /// @return true if operation started
    bool deleteFile(uint8_t compId, const QString& path);

    void cancel() override;

signals:
    void deleteComplete(const QString& path, const QString& errorMsg);

protected:
    void onAckReceived(const MavlinkFTP::Request* ack) override;
    void onNakReceived(const MavlinkFTP::Request* nak) override;
    void onTimeout() override;

private:
    void _buildStateMachine();
    void _startDelete();

    QString _pathToDelete;

    QGCState* _idleState = nullptr;
    QGCState* _deletingState = nullptr;
    QGCFinalState* _completeState = nullptr;
    QGCState* _errorState = nullptr;
};
