#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkMessageType.h"
#include "VehicleTypes.h"

class Vehicle;

/// Manages GCS / operator control of a single Vehicle (MAVLink #2313).
///
/// Tracks which GCS currently holds control, the configured secondary GCS list
/// and the request / release / takeover lifecycle, and gates joystick output on
/// the owning Vehicle accordingly. Owned by and parented to that Vehicle, which
/// routes the relevant MAVLink messages here.
class GCSControlManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    GCSControlManager(Vehicle* vehicle);

    Q_PROPERTY(uint8_t     sysidInControl                        READ sysidInControl                        NOTIFY gcsControlStatusChanged)
    Q_PROPERTY(QList<int>  secondaryGCSList                      READ secondaryGCSList                      NOTIFY gcsControlStatusChanged)
    Q_PROPERTY(bool        gcsControlStatusFlags_SystemManager   READ gcsControlStatusFlags_SystemManager   NOTIFY gcsControlStatusChanged)
    Q_PROPERTY(bool        gcsControlStatusFlags_TakeoverAllowed READ gcsControlStatusFlags_TakeoverAllowed NOTIFY gcsControlStatusChanged)
    Q_PROPERTY(bool        firstControlStatusReceived            READ firstControlStatusReceived            NOTIFY gcsControlStatusChanged)
    Q_PROPERTY(int         operatorControlTakeoverTimeoutMsecs   READ operatorControlTakeoverTimeoutMsecs   CONSTANT)
    Q_PROPERTY(int         requestOperatorControlRemainingMsecs  READ requestOperatorControlRemainingMsecs  NOTIFY sendControlRequestAllowedChanged)
    Q_PROPERTY(bool        sendControlRequestAllowed             READ sendControlRequestAllowed             NOTIFY sendControlRequestAllowedChanged)

    Q_INVOKABLE void startTimerRevertAllowTakeover();
    Q_INVOKABLE void requestOperatorControl(bool allowOverride, int requestTimeoutSecs = 0);
    Q_INVOKABLE void releaseOperatorControl();

    uint8_t    sysidInControl() const { return _sysid_in_control; }
    QList<int> secondaryGCSList() const { return _secondaryGCSList; }
    bool       gcsControlStatusFlags_SystemManager() const { return _gcsControlStatusFlags_SystemManager; }
    bool       gcsControlStatusFlags_TakeoverAllowed() const { return _gcsControlStatusFlags_TakeoverAllowed; }
    bool       firstControlStatusReceived() const { return _firstControlStatusReceived; }
    int        operatorControlTakeoverTimeoutMsecs() const;
    int        requestOperatorControlRemainingMsecs() const { return _timerRequestOperatorControl.remainingTime(); }
    bool       sendControlRequestAllowed() const { return _sendControlRequestAllowed; }

    /// Handle an incoming CONTROL_STATUS message. Called by Vehicle message routing.
    void handleControlStatus(const mavlink_message_t& message);
    /// Handle an incoming MAV_CMD_REQUEST_OPERATOR_CONTROL command. Called by Vehicle message routing.
    void handleCommandRequestOperatorControl(const mavlink_message_t& message, const mavlink_command_long_t& commandLong);

signals:
    void gcsControlStatusChanged();
    void requestOperatorControlReceived(int sysIdRequestingControl, int allowTakeover, int requestTimeoutSecs);
    void sendControlRequestAllowedChanged(bool sendControlRequestAllowed);

private:
    static void _requestOperatorControlAckHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, VehicleTypes::MavCmdResultFailureCode_t failureCode);
    void _requestOperatorControlStartTimer(int requestTimeoutMsecs);

    Vehicle*   _vehicle = nullptr;

    uint8_t    _sysid_in_control = 0;
    uint8_t    _operatorControlCompId = 0;   // compid of the system manager component, learned from CONTROL_STATUS
    QList<int> _secondaryGCSList;
    uint8_t    _gcsControlStatusFlags = 0;
    bool       _gcsControlStatusFlags_SystemManager = false;
    bool       _gcsControlStatusFlags_TakeoverAllowed = false;
    bool       _firstControlStatusReceived = false;
    QTimer     _timerRevertAllowTakeover;
    QTimer     _timerRequestOperatorControl;
    bool       _sendControlRequestAllowed = true;
};
