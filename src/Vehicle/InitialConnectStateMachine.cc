/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InitialConnectStateMachine.h"
#include "Vehicle.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "ComponentInformationManager.h"
#include "MissionManager.h"

QGC_LOGGING_CATEGORY(InitialConnectStateMachineLog, "InitialConnectStateMachineLog")

const StateMachine::StateFn InitialConnectStateMachine::_rgStates[] = {
    InitialConnectStateMachine::_stateRequestCapabilities,
    InitialConnectStateMachine::_stateRequestProtocolVersion,
    InitialConnectStateMachine::_stateRequestCompInfo,
    InitialConnectStateMachine::_stateRequestParameters,
    InitialConnectStateMachine::_stateRequestMission,
    InitialConnectStateMachine::_stateRequestGeoFence,
    InitialConnectStateMachine::_stateRequestRallyPoints,
    InitialConnectStateMachine::_stateSignalInitialConnectComplete
};

const int InitialConnectStateMachine::_cStates = sizeof(InitialConnectStateMachine::_rgStates) / sizeof(InitialConnectStateMachine::_rgStates[0]);

InitialConnectStateMachine::InitialConnectStateMachine(Vehicle* vehicle)
    : _vehicle(vehicle)
{

}

int InitialConnectStateMachine::stateCount(void) const
{
    return _cStates;
}

const InitialConnectStateMachine::StateFn* InitialConnectStateMachine::rgStates(void) const
{
    return &_rgStates[0];
}

void InitialConnectStateMachine::statesCompleted(void) const
{

}

void InitialConnectStateMachine::_stateRequestCapabilities(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestCapabilities Skipping capability request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping capability request due to link type";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "Requesting capabilities";
            vehicle->_waitForMavlinkMessage(_waitForAutopilotVersionResultHandler, connectMachine, MAVLINK_MSG_ID_AUTOPILOT_VERSION, 1000);
            vehicle->sendMavCommandWithHandler(_capabilitiesCmdResultHandler,
                                               connectMachine,
                                               MAV_COMP_ID_AUTOPILOT1,
                                               MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES,
                                               1);                                      // Request firmware version
        }
    }
}

void InitialConnectStateMachine::_capabilitiesCmdResultHandler(void* resultHandlerData, int /*compId*/, MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    if (result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            qCDebug(InitialConnectStateMachineLog) << QStringLiteral("MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES error(%1)").arg(result);
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            qCDebug(InitialConnectStateMachineLog) << "MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES no response from vehicle";
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            qCDebug(InitialConnectStateMachineLog) << "Internal Error: MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES could not be sent due to duplicate command";
            break;
        }

        qCDebug(InitialConnectStateMachineLog) << "Setting no capabilities";
        vehicle->_setCapabilities(0);
        vehicle->_waitForMavlinkMessageClear();
        connectMachine->advance();
    }
}

void InitialConnectStateMachine::_waitForAutopilotVersionResultHandler(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    if (noResponsefromVehicle) {
        qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION timeout";
        qCDebug(InitialConnectStateMachineLog) << "Setting no capabilities";
        vehicle->_setCapabilities(0);
    } else {
        qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION received";

        mavlink_autopilot_version_t autopilotVersion;
        mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

        vehicle->_uid = (quint64)autopilotVersion.uid;
        emit vehicle->vehicleUIDChanged();

        if (autopilotVersion.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;
            FIRMWARE_VERSION_TYPE versionType;

            majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
            versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
            vehicle->setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
        }

        if (vehicle->px4Firmware()) {
            // Lower 3 bytes is custom version
            int majorVersion, minorVersion, patchVersion;
            majorVersion = autopilotVersion.flight_custom_version[2];
            minorVersion = autopilotVersion.flight_custom_version[1];
            patchVersion = autopilotVersion.flight_custom_version[0];
            vehicle->setFirmwareCustomVersion(majorVersion, minorVersion, patchVersion);

            // PX4 Firmware stores the first 16 characters of the git hash as binary, with the individual bytes in reverse order
            vehicle->_gitHash = "";
            for (int i = 7; i >= 0; i--) {
                vehicle->_gitHash.append(QString("%1").arg(autopilotVersion.flight_custom_version[i], 2, 16, QChar('0')));
            }
        } else {
            // APM Firmware stores the first 8 characters of the git hash as an ASCII character string
            char nullStr[9];
            strncpy(nullStr, (char*)autopilotVersion.flight_custom_version, 8);
            nullStr[8] = 0;
            vehicle->_gitHash = nullStr;
        }
        if (vehicle->_toolbox->corePlugin()->options()->checkFirmwareVersion() && !vehicle->_checkLatestStableFWDone) {
            vehicle->_checkLatestStableFWDone = true;
            vehicle->_firmwarePlugin->checkIfIsLatestStable(vehicle);
        }
        emit vehicle->gitHashChanged(vehicle->_gitHash);

        vehicle->_setCapabilities(autopilotVersion.capabilities);
    }
    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestProtocolVersion(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestProtocolVersion Skipping protocol version request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestProtocolVersion Skipping protocol version request due to link type";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestProtocolVersion Requesting protocol version";
            vehicle->_waitForMavlinkMessage(_waitForProtocolVersionResultHandler, connectMachine, MAVLINK_MSG_ID_PROTOCOL_VERSION, 1000);
            vehicle->sendMavCommandWithHandler(_protocolVersionCmdResultHandler,
                                               connectMachine,
                                               MAV_COMP_ID_AUTOPILOT1,
                                               MAV_CMD_REQUEST_PROTOCOL_VERSION,
                                               1);                                      // Request protocol version
        }
    }
}

void InitialConnectStateMachine::_protocolVersionCmdResultHandler(void* resultHandlerData, int /*compId*/, MAV_RESULT result, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);

    if (result != MAV_RESULT_ACCEPTED) {
        Vehicle* vehicle = connectMachine->_vehicle;

        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            qCDebug(InitialConnectStateMachineLog) << QStringLiteral("MAV_CMD_REQUEST_PROTOCOL_VERSION error(%1)").arg(result);
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            qCDebug(InitialConnectStateMachineLog) << "MAV_CMD_REQUEST_PROTOCOL_VERSION no response from vehicle";
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            qCDebug(InitialConnectStateMachineLog) << "Internal Error: MAV_CMD_REQUEST_PROTOCOL_VERSION could not be sent due to duplicate command";
            break;
        }

        // _mavlinkProtocolRequestMaxProtoVersion stays at 0 to indicate unknown
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
        vehicle->_waitForMavlinkMessageClear();
    }
    connectMachine->advance();
}

void InitialConnectStateMachine::_waitForProtocolVersionResultHandler(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    if (noResponsefromVehicle) {
        qCDebug(InitialConnectStateMachineLog) << "PROTOCOL_VERSION timeout";
        // The PROTOCOL_VERSION message didn't make it through the pipe from Vehicle->QGC.
        // This means although the vehicle may support mavlink 2, the pipe does not.
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("Setting _maxProtoVersion to 100 due to timeout on receiving PROTOCOL_VERSION message.");
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = 100;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
    } else {
        mavlink_protocol_version_t protoVersion;
        mavlink_msg_protocol_version_decode(&message, &protoVersion);

        qCDebug(InitialConnectStateMachineLog) << "PROTOCOL_VERSION received mav_version:" << protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
    }
    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestCompInfo(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestCompInfo";
    vehicle->_componentInformationManager->requestAllComponentInformation(_stateRequestCompInfoComplete, connectMachine);
}

void InitialConnectStateMachine::_stateRequestCompInfoComplete(void* requestAllCompleteFnData)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(requestAllCompleteFnData);

    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestParameters(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestParameters";
    vehicle->_parameterManager->refreshAllParameters();
}

void InitialConnectStateMachine::_stateRequestMission(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to link type";
            vehicle->_firstMissionLoadComplete();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission";
            vehicle->_missionManager->loadFromVehicle();
        }
    }
}

void InitialConnectStateMachine::_stateRequestGeoFence(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to link type";
            vehicle->_firstGeoFenceLoadComplete();
        } else {
            if (vehicle->_geoFenceManager->supported()) {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence";
                vehicle->_geoFenceManager->loadFromVehicle();
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: skipped due to no support";
                vehicle->_firstGeoFenceLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateRequestRallyPoints(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to link type";
            vehicle->_firstRallyPointLoadComplete();
        } else {
            if (vehicle->_rallyPointManager->supported()) {
                vehicle->_rallyPointManager->loadFromVehicle();
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: skipping due to no support";
                vehicle->_firstRallyPointLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateSignalInitialConnectComplete(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "Signalling initialConnectComplete";
    emit vehicle->initialConnectComplete();
}

