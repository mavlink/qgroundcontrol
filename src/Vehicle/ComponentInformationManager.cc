/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComponentInformationManager.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformationManagerLog")

ComponentInformationManager::StateFn ComponentInformationManager::_rgStates[]= {
    ComponentInformationManager::_stateRequestCompInfoVersion,
    ComponentInformationManager::_stateRequestCompInfoParam,
    ComponentInformationManager::_stateRequestAllCompInfoComplete
};

int ComponentInformationManager::_cStates = sizeof(ComponentInformationManager::_rgStates) / sizeof(ComponentInformationManager::_rgStates[0]);

RequestMetaDataTypeStateMachine::StateFn RequestMetaDataTypeStateMachine::_rgStates[]= {
    RequestMetaDataTypeStateMachine::_stateRequestCompInfo,
};

int RequestMetaDataTypeStateMachine::_cStates = sizeof(RequestMetaDataTypeStateMachine::_rgStates) / sizeof(RequestMetaDataTypeStateMachine::_rgStates[0]);

ComponentInformationManager::ComponentInformationManager(Vehicle* vehicle)
    : _vehicle                  (vehicle)
    , _requestTypeStateMachine  (this)
{

}

int ComponentInformationManager::stateCount(void) const
{
    return _cStates;
}

const ComponentInformationManager::StateFn* ComponentInformationManager::rgStates(void) const
{
    return &_rgStates[0];
}

void ComponentInformationManager::_componentInformationReceived(const mavlink_message_t& message)
{
    mavlink_component_information_t componentInformation;
    mavlink_msg_component_information_decode(&message, &componentInformation);

    ComponentInformation_t* pCompInfo = nullptr;
    switch (componentInformation.metadata_type) {
    case COMP_METADATA_TYPE_VERSION:
        qCDebug(ComponentInformationManagerLog) << "COMPONENT_INFORMATION COMP_METADATA_TYPE_VERSION received";
        _versionCompInfoAvailable = true;
        pCompInfo = &_versionCompInfo;
        break;
    case COMP_METADATA_TYPE_PARAMETER:
        qCDebug(ComponentInformationManagerLog) << "COMPONENT_INFORMATION COMP_METADATA_TYPE_PARAMETER received";
        _paramCompInfoAvailable = true;
        pCompInfo = &_parameterCompInfo;
        break;
    }

    if (pCompInfo) {
        pCompInfo->metadataUID      = componentInformation.metadata_uid;
        pCompInfo->metadataURI      = componentInformation.metadata_uri;
        pCompInfo->translationUID   = componentInformation.translation_uid;
        pCompInfo->translationURI   = componentInformation.translation_uri;
    }
}

void ComponentInformationManager::requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData)
{
    _requestAllCompleteFn       = requestAllCompletFn;
    _requestAllCompleteFnData   = requestAllCompleteFnData;
    start();
}

void ComponentInformationManager::_stateRequestCompInfoVersion(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    compMgr->_requestTypeStateMachine.request(COMP_METADATA_TYPE_VERSION);
}

void ComponentInformationManager::_stateRequestCompInfoComplete(void)
{
    advance();
}

void ComponentInformationManager::_stateRequestCompInfoParam(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    compMgr->_requestTypeStateMachine.request(COMP_METADATA_TYPE_PARAMETER);
}

void ComponentInformationManager::_stateRequestAllCompInfoComplete(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    (*compMgr->_requestAllCompleteFn)(compMgr->_requestAllCompleteFnData);
    compMgr->_requestAllCompleteFn      = nullptr;
    compMgr->_requestAllCompleteFnData  = nullptr;
}

RequestMetaDataTypeStateMachine::RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr)
    : _compMgr(compMgr)
{

}

void RequestMetaDataTypeStateMachine::request(COMP_METADATA_TYPE type)
{
    _type       = type;
    _stateIndex = -1;

    start();
}

int RequestMetaDataTypeStateMachine::stateCount(void) const
{
    return _cStates;
}

const RequestMetaDataTypeStateMachine::StateFn* RequestMetaDataTypeStateMachine::rgStates(void) const
{
    return &_rgStates[0];
}

void RequestMetaDataTypeStateMachine::statesCompleted(void) const
{
    _compMgr->_stateRequestCompInfoComplete();
}

QString RequestMetaDataTypeStateMachine::typeToString(void)
{
    return _type == COMP_METADATA_TYPE_VERSION ? "COMP_METADATA_TYPE_VERSION" : "COMP_METADATA_TYPE_PARAM";
}

static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t &message)
{
    RequestMetaDataTypeStateMachine* requestMachine = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);

    if (result == MAV_RESULT_ACCEPTED) {
        requestMachine->compMgr()->_componentInformationReceived(message);
    } else {
        switch (failureCode) {
        case Vehicle::RequestMessageFailureCommandError:
            qCDebug(ComponentInformationManagerLog) << QStringLiteral("MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION %1 error(%2)").arg(requestMachine->typeToString()).arg(QGCMAVLink::mavResultToString(result));
            break;
        case Vehicle::RequestMessageFailureCommandNotAcked:
            qCDebug(ComponentInformationManagerLog) << QStringLiteral("MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION %1 no response to command from vehicle").arg(requestMachine->typeToString());
            break;
        case Vehicle::RequestMessageFailureMessageNotReceived:
            qCDebug(ComponentInformationManagerLog) << QStringLiteral("MAV_CMD_REQUEST_MESSAGE COMPONENT_INFORMATION %1 vehicle did not send requested message").arg(requestMachine->typeToString());
            break;
        default:
            break;
        }
    }
    requestMachine->advance();
}

void RequestMetaDataTypeStateMachine::_stateRequestCompInfo(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    Vehicle*                            vehicle         = requestMachine->_compMgr->vehicle();
    LinkInterface*                      link            = vehicle->priorityLink();

    if (link->highLatency() || link->isPX4Flow() || link->isLogReplay()) {
        qCDebug(ComponentInformationManagerLog) << QStringLiteral("Skipping component information % 1 request due to link type").arg(requestMachine->typeToString());
        stateMachine->advance();
    } else {
        qCDebug(ComponentInformationManagerLog) << "Requesting component information" << requestMachine->typeToString();
        vehicle->requestMessage(
                    _requestMessageResultHandler,
                    stateMachine,
                    MAV_COMP_ID_AUTOPILOT1,
                    MAVLINK_MSG_ID_COMPONENT_INFORMATION,
                    requestMachine->_type);
    }
}

