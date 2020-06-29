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
#include "FTPManager.h"

#include <QStandardPaths>

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformationManagerLog")

ComponentInformationManager::StateFn ComponentInformationManager::_rgStates[]= {
    ComponentInformationManager::_stateRequestCompInfoVersion,
    //ComponentInformationManager::_stateRequestCompInfoParam,
    ComponentInformationManager::_stateRequestAllCompInfoComplete
};

int ComponentInformationManager::_cStates = sizeof(ComponentInformationManager::_rgStates) / sizeof(ComponentInformationManager::_rgStates[0]);

RequestMetaDataTypeStateMachine::StateFn RequestMetaDataTypeStateMachine::_rgStates[]= {
    RequestMetaDataTypeStateMachine::_stateRequestCompInfo,
    RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson,
    RequestMetaDataTypeStateMachine::_stateRequestTranslationJson,
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
    _compInfoAvailable  = false;
    _type               = type;
    _stateIndex         = -1;

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

void RequestMetaDataTypeStateMachine::handleComponentInformation(const mavlink_message_t& message)
{
    mavlink_component_information_t componentInformation;
    mavlink_msg_component_information_decode(&message, &componentInformation);

    _compInfo.metadataUID       = componentInformation.metadata_uid;
    _compInfo.metadataURI       = componentInformation.metadata_uri;
    _compInfo.translationUID    = componentInformation.translation_uid;
    _compInfo.translationURI    = componentInformation.translation_uri;
    _compInfoAvailable          = true;
}

static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t &message)
{
    RequestMetaDataTypeStateMachine* requestMachine = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);

    if (result == MAV_RESULT_ACCEPTED) {
        requestMachine->handleComponentInformation(message);
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

void RequestMetaDataTypeStateMachine::_downloadComplete(const QString& file, const QString& errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_downloadComplete" << file << errorMsg;
    advance();
}

void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    Vehicle*                            vehicle         = requestMachine->_compMgr->vehicle();
    FTPManager*                         ftpManager      = vehicle->ftpManager();

    if (requestMachine->_compInfoAvailable) {
        ComponentInformation_t& compInfo = requestMachine->_compInfo;

        qCDebug(ComponentInformationManagerLog) << "Downloading metadata json" << compInfo.translationURI;
        if (_uriIsFTP(compInfo.metadataURI)) {
            connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_downloadComplete);
            ftpManager->download(compInfo.metadataURI, QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        } else {
            // FIXME: NYI
            qCDebug(ComponentInformationManagerLog) << "Skipping metadata json download. http download NYI";
        }
    } else {
        qCDebug(ComponentInformationManagerLog) << "Skipping metadata json download. Component informatiom not available";
        requestMachine->advance();
    }
}

void RequestMetaDataTypeStateMachine::_stateRequestTranslationJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    Vehicle*                            vehicle         = requestMachine->_compMgr->vehicle();
    FTPManager*                         ftpManager      = vehicle->ftpManager();

    if (requestMachine->_compInfoAvailable) {
        ComponentInformation_t& compInfo = requestMachine->_compInfo;
        if (compInfo.translationURI.isEmpty()) {
            qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. No translation json specified";
            requestMachine->advance();
        } else {
            qCDebug(ComponentInformationManagerLog) << "Downloading translation json" << compInfo.translationURI;
            if (_uriIsFTP(compInfo.translationURI)) {
                connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_downloadComplete);
                ftpManager->download(compInfo.metadataURI, QStandardPaths::writableLocation(QStandardPaths::TempLocation));
            } else {
                // FIXME: NYI
                qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. http download NYI";
            }
        }
    } else {
        qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. Component informatiom not available";
        requestMachine->advance();
    }
}

bool RequestMetaDataTypeStateMachine::_uriIsFTP(const QString& uri)
{
    return uri.startsWith("mavlinkftp", Qt::CaseInsensitive);
}
