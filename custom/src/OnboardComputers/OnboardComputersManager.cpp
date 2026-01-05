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
#include "ParameterManager.h"
#include "Vehicle.h"
#include "qtmetamacros.h"
#include "f4_autonomy/version.h"
#include "SettingsManager.h"
#include "FlyViewSettings.h"

QGC_LOGGING_CATEGORY(OnboardComputersManagerLog, "OnboardComputersManager")

//===================================================================================================

OnboardComputersManager::OnboardComputerStruct::OnboardComputerStruct(uint8_t compId_, Vehicle* vehicle_)
    : compId(compId_), vehicle(vehicle_) {}

//===================================================================================================

OnboardComputersManager::OnboardComputersManager(Vehicle* vehicle, QObject *parent) :
    QObject(parent),
    _vehicle(vehicle) {
    qCDebug(OnboardComputersManagerLog) << "OnboardComputersManager Created";

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    connect(MultiVehicleManager::instance(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this,
            &OnboardComputersManager::_vehicleReady);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &OnboardComputersManager::_mavlinkMessageReceived);
    connect(_vehicle,&Vehicle::textMessageReceived,this, &OnboardComputersManager::_vehicleMessageReceived);
    connect(&_timeoutCheckTimer, &QTimer::timeout, this, &OnboardComputersManager::_checkTimeouts);
    _timeoutCheckTimer.start(_timeoutCheckInterval);
}

void OnboardComputersManager::_vehicleReady(bool ready) { _vehicleReadyState = ready; }

void OnboardComputersManager::_mavlinkMessageReceived(const mavlink_message_t& message) {
    if (message.sysid == _vehicle->id() &&
            (message.compid >= MAV_COMP_ID_ONBOARD_COMPUTER && message.compid <= MAV_COMP_ID_ONBOARD_COMPUTER4)) {
        switch (message.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
            _handleHeartbeat(message);
            break;
        case MAVLINK_MSG_ID_COMPANION_VERSION:
            _handleCompanionVersion(message);
            break;
        case MAVLINK_MSG_ID_ONBOARD_COMPUTER_STATUS:
            // TODO
            break;
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
        if (compIter.lastHeartbeat.elapsed() > _timeoutCheckInterval*2) {
            uint8_t compId = iter.key();
            iter = _onboardComputers.erase(iter);
            qCDebug(OnboardComputersManagerLog) << "Deleting onboard computer: " << compId << ", due to timeout.";
            if (_currentComputerComponent == compId) {
                if (_onboardComputers.isEmpty()) {
                    _currentComputerComponent = 0;
                    emit currentComputerComponentChanged(0);
                }else{
                    setCurrentComputerComponent(_onboardComputers.first().compId);
                }
            }
            _vehicle->parameterManager()->removeComponent(compId);
            emit onboardComputerTimeout(compId);
            emit computersInfoChanged();
            emit computersListChanged();
            continue;
        }
        ++iter;
    }
}

void OnboardComputersManager::setCurrentComputerComponent(int sel) {
    qCDebug(OnboardComputersManagerLog) << "Setting current computer ID to " << sel;

    if (_onboardComputers.contains(sel)) {
        _currentComputerComponent = sel;
        emit currentComputerComponentChanged(_onboardComputers[sel].compId);
    }
}

QList<QVariantMap> OnboardComputersManager::computersInfo()
{
    QList<QVariantMap> result;
    result.reserve(_onboardComputers.size());
    for (auto id : _onboardComputers.keys()) {
        result<<computerInfo(id);
    }
    return result;
}

QVariantMap OnboardComputersManager::computerInfo(uint8_t compId)
{
    if (!_onboardComputers.contains(compId)) {
        return QVariantMap();
    }
    auto &computer=_onboardComputers[compId];
    auto &info=computer.info;
    QVariantMap infoMap;
    infoMap["Component Id"].setValue(computer.compId);
    if (info.vendor_id == 0) {
        return infoMap;
    }
    infoMap["Capabilities"].setValue(info.capabilities);
    infoMap["UID"].setValue(info.uid);
    infoMap["Flight Version"].setValue(info.flight_sw_version);
    infoMap["Middleware version"].setValue(info.middleware_sw_version);
    infoMap["OS version"].setValue(info.os_sw_version);
    infoMap["Board version"].setValue(info.board_version);
    infoMap["Vendor Id"].setValue(info.vendor_id);
    infoMap["Product Id"].setValue(info.product_id);
    infoMap["Flight hash"].setValue(QString(reinterpret_cast<char*>(info.flight_custom_version)));
    infoMap["Middleware hash"].setValue(QString(reinterpret_cast<char*>(info.middleware_custom_version)));
    infoMap["OS hash"].setValue(QString(reinterpret_cast<char*>(info.os_custom_version)));

    return infoMap;

}

void OnboardComputersManager::_vehicleMessageReceived(int sysid, int componentid, int severity, QString text, QString description)
{
    // While we do not have osVersion or trying to request it, we reading logs, in case we get version from it.
    if(!_onboardComputers.contains(componentid) ||
        _onboardComputers[componentid].osVersion.toUint32() != 0 ||
        _onboardComputers[componentid].infoRequestCnt >= _companionVersionMaxRetryCount ||
        !text.contains("F4 software version")){
        return;
    }
    auto &computer = _onboardComputers[componentid];
    static const QRegularExpression expr("v(\\d+)\\.(\\d+)\\.(\\d+)-\\w*(\\d+)");
    QRegularExpressionMatch match(expr.match(text));
    if(match.hasMatch()){
        computer.osVersion.major = match.captured(1).toInt(),
        computer.osVersion.minor = match.captured(2).toInt(),
        computer.osVersion.patch = match.captured(3).toInt(),
        computer.osVersion.firmwareVersion = match.captured(4).toInt();
    }

}

void OnboardComputersManager::_handleHeartbeat(const mavlink_message_t& message) {
    // Get the ID of the computer from 191 to 194

    uint8_t computerId = message.compid;
    if (_onboardComputers.contains(computerId)) {
        // If we already know that onboard computer, just reset the hearbeat timer
        auto &computer = _onboardComputers[computerId];
        computer.lastHeartbeat.start();

        //if we do not have vendor_id, asking for that
        if (0 == computer.info.vendor_id) {

            // if still have pending message to answer, ignore...
            if(_vehicle->isMavCommandPending(computerId,MAV_CMD_REQUEST_MESSAGE)){
                return;
            }

            if (computer.infoRequestCnt < _companionVersionMaxRetryCount) {
                qCDebug(OnboardComputersManagerLog) << "CompId:" << computerId << "Retry COMPANION_VERSION request #" << computer.infoRequestCnt;
                _vehicle->sendMavCommand(computerId, MAV_CMD_REQUEST_MESSAGE, true,
                                                 MAVLINK_MSG_ID_COMPANION_VERSION, //first param set id of message
                                                 0, 0, 0, 0, 0,
                                                 0);
                computer.infoRequestCnt++;
            } else if (computer.infoRequestCnt == _companionVersionMaxRetryCount) {
                qCDebug(OnboardComputersManagerLog) << "CompId:" << computerId << "Stop COMPANION_VERSION request due to retry limit";
                emit onboardComputerInfoRecieveError(computerId);
                if(computer.osVersion.toUint32() == 0){
                    qgcApp()->showAppMessage(tr("WARNING:\nCan't receive VGM firmware version from dialect! Try to restart VGM."));
                } else {
                    qgcApp()->showAppMessage(tr("INFO:\nCan't receive VGM firmware version from dialect! Using version from boot message."));
                    computer.info.vendor_id = 0xf4;
                    computer.info.os_sw_version = computer.osVersion.toUint32();
                }
                computer.infoRequestCnt++;
            }
        }
    } else {
        // If we see this computer for the first time, add it to the existing list
        _onboardComputers[computerId] = OnboardComputerStruct(message.compid, _vehicle);
        _onboardComputers[computerId].lastHeartbeat.start();

        qCDebug(OnboardComputersManagerLog) << "CompId:" << computerId << "Request COMPANION_VERSION form VGM.";
        auto settings = SettingsManager::instance()->flyViewSettings();
        if(settings->enableVGMDialect()->rawValue().toBool()){
        _vehicle->sendMavCommand(computerId, MAV_CMD_REQUEST_MESSAGE, true,
                                 MAVLINK_MSG_ID_COMPANION_VERSION, //first param set id of message
                                 0, 0, 0, 0, 0, 0); // do not touch other params
        } else {
            _onboardComputers[computerId].infoRequestCnt = _companionVersionMaxRetryCount + 1;
        }
        // If current computer index is not set (is 0, while onboard computers has id 191-194), we set it to this computer
        if (_currentComputerComponent == 0) {
            setCurrentComputerComponent(computerId);
        }
        emit computersListChanged();
        _vehicle->parameterManager()->refreshAllParameters();
    }
}

void OnboardComputersManager::_handleCompanionVersion(const mavlink_message_t &message)
{
    uint8_t computerId = message.compid;
    qCDebug(OnboardComputersManagerLog) << "CompId:" << computerId << "Handling COMPANION_VERSION message";
    if (!_onboardComputers.contains(computerId)) {
        qCDebug(OnboardComputersManagerLog) << "CompId:" << computerId << "Get COMPANION_VERSION of non existing onboardComputerId";
        return;
    }

    auto &comp = _onboardComputers[computerId];
    //TODO: CUSTOM MAV_LINK DIALECT
    mavlink_msg_companion_version_decode(&message, &comp.info);

    if( comp.osVersion.toUint32() != 0 &&
        comp.osVersion.toUint32() != comp.info.os_sw_version){
        qCWarning(OnboardComputersManagerLog)<<"VGM version from dialect and boot msg are not similar!";
    }
    if(comp.info.os_sw_version != 0){
        comp.osVersion.fromUint32(comp.info.os_sw_version);
    } else {
        qCWarning(OnboardComputersManagerLog)<< "VGM response with 0x0 OS version code.";
        comp.info.os_sw_version = comp.osVersion.toUint32();
    }

    // if current computer do not have vendor_id (not VGM), then changing it to VGM
    if (comp.info.vendor_id != 0xf4) {
        setCurrentComputerComponent(computerId);
        return;
    }
    emit onboardComputerInfoUpdated(computerId);
    emit computersInfoChanged();
}

void OnboardComputersManager::rebootAllOnboardComputers() {
    for (const auto& computer : _onboardComputers) {
        qCDebug(OnboardComputersManagerLog) << "Rebooting all available onboard computers";
        _vehicle->sendMavCommand(computer.compId, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
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
    if (!_onboardComputers.contains(_currentComputerComponent)) {
        qCWarning(OnboardComputersManagerLog) << "Cannot set external position estimate to an unknown onboard computer";
        return;
    }
    auto currentComputer = _onboardComputers[_currentComputerComponent];
    Vehicle::MavCmdAckHandlerInfo_t externalPositionAckHandler{/* .resultHandler = */ _handleExternalPositionAck,
                                                               /* .resultHandlerData =  */ this,
                                                               /* .progressHandler =  */ nullptr,
                                                               /* .progressHandlerData =  */ nullptr};
    qCDebug(OnboardComputersManagerLog) << "Sending external pose estimate to comp id: " << currentComputer.compId;
    _vehicle->sendMavCommandWithHandler(&externalPositionAckHandler, currentComputer.compId,
                                        MAV_CMD_EXTERNAL_POSITION_ESTIMATE, 0.0, 0.0, 0.0, 0.0, coord.latitude(),
                                        coord.longitude(), coord.altitude());
}

bool OnboardComputersManager::checkVersion(QString desctiption, int major, int minor, int patch)
{
    OnboardComputerStruct::OsVersion version{static_cast<uint8_t>(major),
                                             static_cast<uint8_t>(minor),
                                             static_cast<uint8_t>(patch)};
    if(!version.isValid()){
        return false;
    }

    for(auto &comp:_onboardComputers){
        if( comp.osVersion.isValid() &&
            comp.osVersion.compatible(version)){
            return true;
        }
    }
    return false;
}
