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
#include "QGCZlib.h"
#include "JsonHelper.h"
#include "CompInfoVersion.h"
#include "CompInfoParam.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformationManagerLog")

ComponentInformationManager::StateFn ComponentInformationManager::_rgStates[]= {
    ComponentInformationManager::_stateRequestCompInfoVersion,
    ComponentInformationManager::_stateRequestCompInfoParam,
    ComponentInformationManager::_stateRequestAllCompInfoComplete
};

int ComponentInformationManager::_cStates = sizeof(ComponentInformationManager::_rgStates) / sizeof(ComponentInformationManager::_rgStates[0]);

RequestMetaDataTypeStateMachine::StateFn RequestMetaDataTypeStateMachine::_rgStates[]= {
    RequestMetaDataTypeStateMachine::_stateRequestCompInfo,
    RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson,
    RequestMetaDataTypeStateMachine::_stateRequestTranslationJson,
    RequestMetaDataTypeStateMachine::_stateRequestComplete,
};

int RequestMetaDataTypeStateMachine::_cStates = sizeof(RequestMetaDataTypeStateMachine::_rgStates) / sizeof(RequestMetaDataTypeStateMachine::_rgStates[0]);

ComponentInformationManager::ComponentInformationManager(Vehicle* vehicle)
    : _vehicle                  (vehicle)
    , _requestTypeStateMachine  (this)
{
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_VERSION]    = new CompInfoVersion   (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]  = new CompInfoParam     (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
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
    compMgr->_requestTypeStateMachine.request(compMgr->_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_VERSION]);
}

void ComponentInformationManager::_stateRequestCompInfoComplete(void)
{
    advance();
}

void ComponentInformationManager::_stateRequestCompInfoParam(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);

    if (compMgr->_isCompTypeSupported(COMP_METADATA_TYPE_PARAMETER)) {
        compMgr->_requestTypeStateMachine.request(compMgr->_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]);
    } else {
        qCDebug(ComponentInformationManagerLog) << "_stateRequestCompInfoParam skipping, not supported";
        compMgr->advance();
    }
}

void ComponentInformationManager::_stateRequestAllCompInfoComplete(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    (*compMgr->_requestAllCompleteFn)(compMgr->_requestAllCompleteFnData);
    compMgr->_requestAllCompleteFn      = nullptr;
    compMgr->_requestAllCompleteFnData  = nullptr;
}

bool ComponentInformationManager::_isCompTypeSupported(COMP_METADATA_TYPE type)
{
    return qobject_cast<CompInfoVersion*>(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_VERSION])->isMetaDataTypeSupported(type);
}

CompInfoParam* ComponentInformationManager::compInfoParam(uint8_t compId)
{
    return _compInfoMap.contains(compId) && _compInfoMap[compId].contains(COMP_METADATA_TYPE_PARAMETER) ? qobject_cast<CompInfoParam*>(_compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER]) : nullptr;
}

CompInfoVersion* ComponentInformationManager::compInfoVersion(uint8_t compId)
{
    return _compInfoMap.contains(compId) && _compInfoMap[compId].contains(COMP_METADATA_TYPE_VERSION) ? qobject_cast<CompInfoVersion*>(_compInfoMap[compId][COMP_METADATA_TYPE_VERSION]) : nullptr;
}

RequestMetaDataTypeStateMachine::RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr)
    : _compMgr(compMgr)
{

}

void RequestMetaDataTypeStateMachine::request(CompInfo* compInfo)
{
    _compInfo   = compInfo;
    _stateIndex = -1;
    _jsonMetadataFileName.clear();
    _jsonTranslationFileName.clear();

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
    return _compInfo->type == COMP_METADATA_TYPE_VERSION ? "COMP_METADATA_TYPE_VERSION" : "COMP_METADATA_TYPE_PARAM";
}

static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t &message)
{
    RequestMetaDataTypeStateMachine* requestMachine = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);

    if (result == MAV_RESULT_ACCEPTED) {
        requestMachine->compInfo()->setMessage(message);
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
                    requestMachine->_compInfo->type);
    }
}

QString RequestMetaDataTypeStateMachine::_downloadCompleteJsonWorker(const QString& fileName, const QString& inflatedFileName)
{
    QString outputFileName = fileName;

    if (fileName.endsWith(".gz", Qt::CaseInsensitive)) {
        outputFileName = (QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(inflatedFileName));
        if (QGCZlib::inflateGzipFile(fileName, outputFileName)) {
            QFile(fileName).remove();
        } else {
            qCWarning(ComponentInformationManagerLog) << "Inflate of compressed json failed" << inflatedFileName;
            outputFileName.clear();
        }
    } else {
        outputFileName = fileName;
    }

    return outputFileName;
}

void RequestMetaDataTypeStateMachine::_downloadCompleteMetaDataJson(const QString& fileName, const QString& errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_downloadCompleteMetaDataJson fileName:errorMsg" << fileName << errorMsg;

    if (errorMsg.isEmpty()) {
        _jsonMetadataFileName = _downloadCompleteJsonWorker(fileName, "metadata.json");
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_downloadCompleteTranslationJson(const QString& fileName, const QString& errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_downloadCompleteTranslationJson fileName:errorMsg" << fileName << errorMsg;

    QString jsonTranslationFileName;
    if (errorMsg.isEmpty()) {
        jsonTranslationFileName = _downloadCompleteJsonWorker(fileName, "translation.json");
    }

    _compInfo->setJson(_jsonMetadataFileName, jsonTranslationFileName);

    advance();
}

void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();
    FTPManager*                         ftpManager      = compInfo->vehicle->ftpManager();

    if (compInfo->available) {
        qCDebug(ComponentInformationManagerLog) << "Downloading metadata json" << compInfo->uriMetaData;
        if (_uriIsFTP(compInfo->uriMetaData)) {
            connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_downloadCompleteMetaDataJson);
            ftpManager->download(compInfo->uriMetaData, QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        } else {
            // FIXME: NYI
            qCDebug(ComponentInformationManagerLog) << "Skipping metadata json download. http download NYI";
        }
    } else {
        qCDebug(ComponentInformationManagerLog) << "Skipping metadata json download. Component information not available";
        requestMachine->advance();
    }
}

void RequestMetaDataTypeStateMachine::_stateRequestTranslationJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();
    FTPManager*                         ftpManager      = compInfo->vehicle->ftpManager();

    if (compInfo->available) {
        if (compInfo->uriTranslation.isEmpty()) {
            qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. No translation json specified";
            requestMachine->advance();
        } else {
            qCDebug(ComponentInformationManagerLog) << "Downloading translation json" << compInfo->uriTranslation;
            if (_uriIsFTP(compInfo->uriTranslation)) {
                connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_downloadCompleteTranslationJson);
                ftpManager->download(compInfo->uriTranslation, QStandardPaths::writableLocation(QStandardPaths::TempLocation));
            } else {
                // FIXME: NYI
                qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. http download NYI";
            }
        }
    } else {
        qCDebug(ComponentInformationManagerLog) << "Skipping translation json download. Component information not available";
        requestMachine->advance();
    }
}

void RequestMetaDataTypeStateMachine::_stateRequestComplete(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();

    compInfo->setJson(requestMachine->_jsonMetadataFileName, requestMachine->_jsonTranslationFileName);
    requestMachine->advance();
}

bool RequestMetaDataTypeStateMachine::_uriIsFTP(const QString& uri)
{
    return uri.startsWith("mavlinkftp", Qt::CaseInsensitive);
}
