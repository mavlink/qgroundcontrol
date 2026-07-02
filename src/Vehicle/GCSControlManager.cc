#include "GCSControlManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "MAVLinkProtocol.h"
#include "SettingsManager.h"
#include "FlyViewSettings.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(GCSControlManagerLog, "Vehicle.GCSControlManager")

#define REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS 10000

GCSControlManager::GCSControlManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
}

int GCSControlManager::operatorControlTakeoverTimeoutMsecs() const
{
    return REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS;
}

void GCSControlManager::startTimerRevertAllowTakeover()
{
    _timerRevertAllowTakeover.stop();
    _timerRevertAllowTakeover.setSingleShot(true);
    _timerRevertAllowTakeover.setInterval(operatorControlTakeoverTimeoutMsecs());
    // Disconnect any previous connections to avoid multiple handlers
    disconnect(&_timerRevertAllowTakeover, &QTimer::timeout, nullptr, nullptr);

    connect(&_timerRevertAllowTakeover, &QTimer::timeout, this, [this](){
        if (MAVLinkProtocol::instance()->getSystemId() == _sysid_in_control) {
            this->requestOperatorControl(false);
        }
    });
    _timerRevertAllowTakeover.start();
}

void GCSControlManager::_computeOperatorControlRange(uint8_t &rangeLow, uint8_t &rangeHigh) const
{
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    const QString secondaryStr = SettingsManager::instance()->flyViewSettings()->operatorControlSecondaryGCS()->rawValue().toString().trimmed();

    if (secondaryStr.isEmpty()) {
        rangeLow = myId;
        rangeHigh = 0;
        return;
    }

    uint8_t lo = myId;
    uint8_t hi = myId;
    // Accept any non-digit separator (commas, spaces, or a mix) so the field is forgiving
    const QStringList parts = secondaryStr.split(QRegularExpression(QStringLiteral("[^0-9]+")), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok = false;
        const int val = part.toInt(&ok);
        if (ok && val >= 1 && val <= 255) {
            const uint8_t id = static_cast<uint8_t>(val);
            if (id < lo) lo = id;
            if (id > hi) hi = id;
        }
    }

    rangeLow = lo;
    rangeHigh = (hi > lo) ? hi : 0;
}

void GCSControlManager::requestOperatorControl(bool allowOverride, int requestTimeoutSecs)
{
    int safeRequestTimeoutSecs;
    int requestTimeoutSecsMin = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedMin().toInt();
    int requestTimeoutSecsMax = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedMax().toInt();
    if (requestTimeoutSecs >= requestTimeoutSecsMin && requestTimeoutSecs <= requestTimeoutSecsMax) {
        safeRequestTimeoutSecs = requestTimeoutSecs;
    } else {
        safeRequestTimeoutSecs = SettingsManager::instance()->flyViewSettings()->requestControlTimeout()->cookedDefaultValue().toInt();
    }

    uint8_t rangeLow, rangeHigh;
    _computeOperatorControlRange(rangeLow, rangeHigh);

    const VehicleTypes::MavCmdAckHandlerInfo_t handlerInfo = {&GCSControlManager::_requestOperatorControlAckHandler, this, nullptr, nullptr};
    _vehicle->sendMavCommandWithHandler(
        &handlerInfo,
        (_operatorControlCompId != 0) ? _operatorControlCompId : _vehicle->defaultComponentId(),
        MAV_CMD_REQUEST_OPERATOR_CONTROL,
        1,                                  // param1: Action - 1: Request control
        allowOverride ? 1.0f : 0.0f,        // param2: Allow takeover
        static_cast<float>(safeRequestTimeoutSecs), // param3: Timeout in seconds
        static_cast<float>(rangeLow),        // param4: GCS sysid (range low)
        static_cast<float>(rangeHigh)        // param5: GCS sysid upper range (0 = single GCS)
    );

    if (requestTimeoutSecs > 0) {
        _requestOperatorControlStartTimer(requestTimeoutSecs * 1000);
    }
}

void GCSControlManager::releaseOperatorControl()
{
    // Releasing control makes any pending request countdown or takeover revert meaningless
    _timerRevertAllowTakeover.stop();
    _timerRequestOperatorControl.stop();
    disconnect(&_timerRequestOperatorControl, &QTimer::timeout, nullptr, nullptr);
    if (!_sendControlRequestAllowed) {
        _sendControlRequestAllowed = true;
        emit sendControlRequestAllowedChanged(true);
    }

    const VehicleTypes::MavCmdAckHandlerInfo_t handlerInfo = {&GCSControlManager::_requestOperatorControlAckHandler, this, nullptr, nullptr};
    _vehicle->sendMavCommandWithHandler(
        &handlerInfo,
        (_operatorControlCompId != 0) ? _operatorControlCompId : _vehicle->defaultComponentId(),
        MAV_CMD_REQUEST_OPERATOR_CONTROL,
        0,                                  // param1: Action - 0: Release control
        0,                                  // param2: Allow takeover (irrelevant for release)
        0,                                  // param3: Timeout (irrelevant for release)
        static_cast<float>(MAVLinkProtocol::instance()->getSystemId()), // param4: GCS sysid
        0                                   // param5: GCS sysid upper range (0 = single GCS)
    );
}

void GCSControlManager::_requestOperatorControlAckHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, VehicleTypes::MavCmdResultFailureCode_t failureCode)
{
    // COMMAND_ACK may come from the system-manager component (not necessarily the autopilot)
    Q_UNUSED(compId);

    // If duplicated or no response, show popup to user. Otherwise only log it.
    switch (failureCode) {
        case VehicleTypes::MavCmdResultFailureDuplicateCommand:
            QGC::showAppMessage(tr("Waiting for previous operator control request"));
            return;
        case VehicleTypes::MavCmdResultFailureNoResponseToCommand:
            QGC::showAppMessage(tr("No response to operator control request"));
            return;
        default:
            break;
    }

    GCSControlManager* manager = static_cast<GCSControlManager*>(resultHandlerData);
    if (!manager) {
        return;
    }

    if (ack.result == MAV_RESULT_ACCEPTED) {
        qCDebug(GCSControlManagerLog) << "Operator control request accepted";
    } else {
        qCDebug(GCSControlManagerLog) << "Operator control request rejected";
    }
}

void GCSControlManager::_requestOperatorControlStartTimer(int requestTimeoutMsecs)
{
    // First flag requests not allowed
    _sendControlRequestAllowed = false;
    emit sendControlRequestAllowedChanged(false);
    // Setup timer to re enable it again after timeout
    _timerRequestOperatorControl.stop();
    _timerRequestOperatorControl.setSingleShot(true);
    _timerRequestOperatorControl.setInterval(requestTimeoutMsecs);
    // Disconnect any previous connections to avoid multiple handlers
    disconnect(&_timerRequestOperatorControl, &QTimer::timeout, nullptr, nullptr);
    connect(&_timerRequestOperatorControl, &QTimer::timeout, this, [this](){
        _sendControlRequestAllowed = true;
        emit sendControlRequestAllowedChanged(true);
    });
    _timerRequestOperatorControl.start();
}

void GCSControlManager::handleControlStatus(const mavlink_message_t& message)
{
    mavlink_control_status_t controlStatus;
    mavlink_msg_control_status_decode(&message, &controlStatus);

    if (controlStatus.flags & GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER) {
        // This component manages GCS control of the whole system. Operator control
        // commands must be addressed to it, which is not necessarily the autopilot
        _operatorControlCompId = message.compid;
    }

    bool updateControlStatusSignals = false;
    if (_gcsControlStatusFlags != controlStatus.flags) {
        _gcsControlStatusFlags = controlStatus.flags;
        _gcsControlStatusFlags_SystemManager = controlStatus.flags & GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER;
        _gcsControlStatusFlags_TakeoverAllowed = controlStatus.flags & GCS_CONTROL_STATUS_FLAGS_TAKEOVER_ALLOWED;
        updateControlStatusSignals = true;
    }

    if (_sysid_in_control != controlStatus.gcs_main) {
        _sysid_in_control = controlStatus.gcs_main;
        const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
        _vehicle->setJoystickSendAllowed(_sysid_in_control == 0 || _sysid_in_control == myId);
        if (_sysid_in_control != myId) {
            // Control moved away from this GCS, so a pending revert to takeover not allowed no longer applies
            _timerRevertAllowTakeover.stop();
        }
        updateControlStatusSignals = true;
    }

    QList<int> newSecondaryList;
    for (int i = 0; i < 10; i++) {
        if (controlStatus.gcs_secondary[i] != 0) {
            newSecondaryList.append(controlStatus.gcs_secondary[i]);
        }
    }
    if (_secondaryGCSList != newSecondaryList) {
        _secondaryGCSList = newSecondaryList;
        updateControlStatusSignals = true;
    }

    if (!_firstControlStatusReceived) {
        _firstControlStatusReceived = true;
        updateControlStatusSignals = true;
    }

    if (updateControlStatusSignals) {
        emit gcsControlStatusChanged();
    }

    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    if (!sendControlRequestAllowed() && (_sysid_in_control == myId || _sysid_in_control == 0 || _gcsControlStatusFlags_TakeoverAllowed)) {
        _timerRequestOperatorControl.stop();
        disconnect(&_timerRequestOperatorControl, &QTimer::timeout, nullptr, nullptr);
        _sendControlRequestAllowed = true;
        emit sendControlRequestAllowedChanged(true);
    }
}

void GCSControlManager::handleCommandRequestOperatorControl(const mavlink_message_t& message, const mavlink_command_long_t& commandLong)
{
    // Acknowledge the takeover notification so the autopilot can clear its pending notification state
    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t ackMessage{};
        (void) mavlink_msg_command_ack_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &ackMessage,
            MAV_CMD_REQUEST_OPERATOR_CONTROL,
            MAV_RESULT_ACCEPTED,
            0,                  // progress
            0,                  // result_param2
            message.sysid,
            message.compid);
        (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), ackMessage);
    }

    emit requestOperatorControlReceived(
        static_cast<int>(commandLong.param4),   // GCS sysid requesting control
        static_cast<int>(commandLong.param2),   // Allow takeover
        static_cast<int>(commandLong.param3)    // Request timeout in seconds
    );
}
