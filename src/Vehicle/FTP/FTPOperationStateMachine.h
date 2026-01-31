#pragma once

#include "QGCStateMachine.h"
#include "MAVLinkFTP.h"

#include <QtCore/QTimer>

class Vehicle;

/// Base class for FTP operation state machines.
/// Provides common functionality for MAVLink FTP protocol handling.
class FTPOperationStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    FTPOperationStateMachine(const QString& operationName, Vehicle* vehicle, uint8_t compId, QObject* parent = nullptr);
    ~FTPOperationStateMachine() override = default;

    /// Check if an operation is in progress
    bool isOperationInProgress() const { return _operationInProgress; }

    /// Get the component ID for this operation
    uint8_t componentId() const { return _compId; }

    /// Cancel the current operation
    virtual void cancel() = 0;

    /// Handle incoming MAVLink FTP message
    void handleFTPMessage(const mavlink_message_t& message);

signals:
    /// Emitted when operation completes
    /// @param errorMsg Empty if successful, error message otherwise
    void operationComplete(const QString& errorMsg);

    /// Emitted during operation to show progress
    /// @param value Progress from 0.0 to 1.0
    void progressChanged(float value);

protected:
    /// Send an FTP request and wait for ack
    void sendRequest(MavlinkFTP::Request* request);

    /// Get error message from NAK response
    QString errorMsgFromNak(const MavlinkFTP::Request* nak) const;

    /// Fill request data field with a string
    void fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str);

    /// Called when an ACK is received - subclass should override
    virtual void onAckReceived(const MavlinkFTP::Request* ack) = 0;

    /// Called when a NAK is received - subclass should override
    virtual void onNakReceived(const MavlinkFTP::Request* nak) = 0;

    /// Called when timeout occurs - subclass should override
    virtual void onTimeout() = 0;

    /// Complete the operation
    void completeOperation(const QString& errorMsg = QString());

    /// Advance to next state
    void advanceToNextState();

    // Retry handling
    bool shouldRetry();
    void resetRetryCount() { _retryCount = 0; }
    int retryCount() const { return _retryCount; }

    Vehicle* _vehicle = nullptr;
    uint8_t _compId = MAV_COMP_ID_AUTOPILOT1;
    bool _operationInProgress = false;
    int _retryCount = 0;
    uint16_t _expectedSeqNumber = 0;

    QTimer _timeoutTimer;

    static constexpr int TimeoutMs = 1000;
    static constexpr int MaxRetries = 3;

private slots:
    void _onTimeout();
    void _onMavlinkMessage(const mavlink_message_t& message);
};
