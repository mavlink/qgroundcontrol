#pragma once

#include "FTPOperationStateMachine.h"

#include <QtCore/QStringList>

/// FTP List Directory operation state machine
class FTPListDirectoryStateMachine : public FTPOperationStateMachine
{
    Q_OBJECT

public:
    FTPListDirectoryStateMachine(Vehicle* vehicle, QObject* parent = nullptr);

    /// Start list directory operation
    /// @param compId Component ID
    /// @param path Path on vehicle to list
    /// @return true if operation started
    bool listDirectory(uint8_t compId, const QString& path);

    void cancel() override;

signals:
    void listDirectoryComplete(const QStringList& entries, const QString& errorMsg);

protected:
    void onAckReceived(const MavlinkFTP::Request* ack) override;
    void onNakReceived(const MavlinkFTP::Request* nak) override;
    void onTimeout() override;

private:
    void _buildStateMachine();
    void _sendListRequest(bool firstRequest);

    QString _pathToList;
    QStringList _directoryEntries;
    uint32_t _expectedOffset = 0;

    QGCState* _idleState = nullptr;
    QGCState* _listingState = nullptr;
    QGCFinalState* _completeState = nullptr;
    QGCState* _errorState = nullptr;
};
