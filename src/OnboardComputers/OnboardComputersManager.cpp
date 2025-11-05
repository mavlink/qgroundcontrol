/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OnboardComputersManager.h"

#include <QtQml/QQmlEngine>

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle/Vehicle.h"
#include "qtmetamacros.h"

QGC_LOGGING_CATEGORY(OnboardComputersManagerLog, "OnboardComputersManager")

//===================================================================================================

OnboardComputersManager::OnboardComputerStruct::OnboardComputerStruct(uint8_t compID_, Vehicle* vehicle_)
    : compID(compID_), vehicle(vehicle_) {}

//===================================================================================================

OnboardComputersManager::OnboardComputersManager(Vehicle* vehicle) : _vehicle(vehicle) {
    qCDebug(OnboardComputersManagerLog) << "OnboardComputersManager Created";

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this,
            &OnboardComputersManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &OnboardComputersManager::_mavlinkMessageReceived);

    connect(&_timeoutCheckTimer, &QTimer::timeout, this, &OnboardComputersManager::_checkTimeouts);
    _timeoutCheckTimer.start(_timeoutCheckInterval);
}

void OnboardComputersManager::_vehicleReady(bool ready) { _vehicleReadyState = ready; }

void OnboardComputersManager::_mavlinkMessageReceived(const mavlink_message_t& message) {
    //-- Only pay attention to camera components, as identified by their compId
    if (message.sysid == _vehicle->id() &&
        (message.compid >= MAV_COMP_ID_ONBOARD_COMPUTER && message.compid <= MAV_COMP_ID_ONBOARD_COMPUTER4)) {
        switch (message.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                _handleHeartbeat(message);
                break;
            case MAVLINK_MSG_ID_ONBOARD_COMPUTER_STATUS:
                // TODO
                [[fallthrough]];
            default:
                break;
        }
    }
}

void OnboardComputersManager::_checkTimeouts()
{
    if (_onboardComputers.isEmpty())
        return;

    auto iter = _onboardComputers.begin();
    while (iter != _onboardComputers.end())
    {
        auto &compIter = iter.value();
        // Check all computers for timeout. If has some, emitting timeout on his ID, and removing it form list.
        if (compIter.lastHeartbeat.elapsed() > _timeoutCheckInterval*2){
            uint8_t compID = iter.key();
            iter = _onboardComputers.erase(iter);
            qCDebug(OnboardComputersManagerLog) << "Deleting onboard computer: " << compID << ", due to timeout.";
            if (_currentComputerIndex == compID){
                if (_onboardComputers.isEmpty()){
                    _currentComputerIndex = 0;
                    emit currentComputerChanged(0);
                }else
                    setCurrentComputer(_onboardComputers.first().compID);
            }
            emit onboardComputerTimeout(MAV_COMP_ID_ONBOARD_COMPUTER + compID - 1);
            continue;
        }
        ++iter;
    }
}

void OnboardComputersManager::setCurrentComputer(int sel) {
    qCDebug(OnboardComputersManagerLog) << "Setting current computer ID to " << sel;

    if (_onboardComputers.contains(sel)) {
        _currentComputerIndex = sel;
        emit currentComputerChanged(_onboardComputers[sel].compID);
    }
}

void OnboardComputersManager::_handleHeartbeat(const mavlink_message_t& message) {
    // Get the ID of the computer from 1 to 4
    uint8_t computerId = message.compid - MAV_COMP_ID_ONBOARD_COMPUTER + 1;
    if (_onboardComputers.contains(computerId)) {
        // If we already know that onboard computer, just reset the hearbeat timer
        _onboardComputers[computerId].lastHeartbeat.start();
    } else {
        // If we see this computer for the first time, add it to the existing list
        _onboardComputers[computerId] = OnboardComputerStruct(message.compid, _vehicle);
        _onboardComputers[computerId].lastHeartbeat.start();

        // If current computer index is not set (is 0, while set is 1-4), we set it to this computer
        setCurrentComputer(computerId);
    }
}

void OnboardComputersManager::rebootAllOnboardComputers() {
    for (const auto& computer : _onboardComputers) {
        qDebug(OnboardComputersManagerLog) << "Rebooting all available onboard computers";
        _vehicle->sendMavCommand(computer.compID, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
                                 false,           // do not show errors
                                 0,               // do nothing to autopilot
                                 3,               // reboot onboard computer
                                 0, 0, 0, 0, 0);  // param 3-7 unused
    }
}

void _handleExternalPositionAck(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack,
                                Vehicle::MavCmdResultFailureCode_t failureCode) {
    [[maybe_unused]] OnboardComputersManager* onboardComputersManager =
        static_cast<OnboardComputersManager*>(resultHandlerData);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
            case Vehicle::MavCmdResultCommandResultOnly:
                qCDebug(OnboardComputersManagerLog)
                    << QStringLiteral("MAV_CMD_EXTERNAL_POSITION_ESTIMATE error(%1)").arg(ack.result);

                break;
            case Vehicle::MavCmdResultFailureNoResponseToCommand:
                qCDebug(OnboardComputersManagerLog)
                    << "MAV_CMD_EXTERNAL_POSITION_ESTIMATE failed: no response from vehicle";
                break;
            case Vehicle::MavCmdResultFailureDuplicateCommand:
                qCDebug(OnboardComputersManagerLog) << "MAV_CMD_EXTERNAL_POSITION_ESTIMATE failed: duplicate command";
                break;
        }
        qgcApp()->showAppMessage(QString("Failed to set extermal position estimate for component: %1").arg(compId));
    }
}

void OnboardComputersManager::sendExternalPositionEstimate(const QGeoCoordinate& coord) {
    if (!_onboardComputers.contains(_currentComputerIndex)) {
        qCWarning(OnboardComputersManagerLog) << "Cannot set external position estimate to an unknown onboard computer";
        return;
    }
    auto currentComputer = _onboardComputers[_currentComputerIndex];
    Vehicle::MavCmdAckHandlerInfo_t externalPositionAckHandler{/* .resultHandler = */ _handleExternalPositionAck,
                                                               /* .resultHandlerData =  */ this,
                                                               /* .progressHandler =  */ nullptr,
                                                               /* .progressHandlerData =  */ nullptr};
    qCDebug(OnboardComputersManagerLog) << "Sending external pose estimate to comp id: " << currentComputer.compID;
    _vehicle->sendMavCommandWithHandler(&externalPositionAckHandler, currentComputer.compID,
                                        MAV_CMD_EXTERNAL_POSITION_ESTIMATE, 0.0, 0.0, 0.0, 0.0, coord.latitude(),
                                        coord.longitude(), coord.altitude());
}
