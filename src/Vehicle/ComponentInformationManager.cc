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
#include "QGCFileDownload.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

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
    if (!_compInfoMap.contains(compId)) {
        // Create default info
        _compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER] = new CompInfoParam(compId, _vehicle, this);
    }
    return qobject_cast<CompInfoParam*>(_compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER]);
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
    WeakLinkInterfacePtr                weakLink        = vehicle->vehicleLinkManager()->primaryLink();

    if (weakLink.expired()) {
        qCDebug(ComponentInformationManagerLog) << QStringLiteral("_stateRequestCompInfo Skipping component information %1 request due to no primary link").arg(requestMachine->typeToString());
        stateMachine->advance();
    } else if (!qgcApp()->toolbox()->settingsManager()->appSettings()->useComponentInformationQuery()->rawValue().toBool()) {
        qCDebug(ComponentInformationManagerLog) << QStringLiteral("_stateRequestCompInfo Skipping component information %1 request due to application setting").arg(requestMachine->typeToString());
        stateMachine->advance();
    } else {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isPX4Flow() || sharedLink->isLogReplay()) {
            qCDebug(ComponentInformationManagerLog) << QStringLiteral("_stateRequestCompInfo Skipping component information %1 request due to link type").arg(requestMachine->typeToString());
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

void RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson(const QString& fileName, const QString& errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson fileName:errorMsg" << fileName << errorMsg;

    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson);
    if (errorMsg.isEmpty()) {
        _jsonMetadataFileName = _downloadCompleteJsonWorker(fileName, "metadata.json");
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson failed filename:errorMsg" << fileName << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson(const QString& fileName, const QString& errorMsg)
{
    QString jsonTranslationFileName;

    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson fileName:errorMsg" << fileName << errorMsg;

    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson);
    if (errorMsg.isEmpty()) {
        _jsonTranslationFileName = _downloadCompleteJsonWorker(fileName, "translation.json");
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson failed filename:errorMsg" << fileName << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson(QString remoteFile, QString localFile, QString errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;

    disconnect(qobject_cast<QGCFileDownload*>(sender()), &QGCFileDownload::downloadComplete, this, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson);
    if (errorMsg.isEmpty()) {
        _jsonMetadataFileName = _downloadCompleteJsonWorker(localFile, "metadata.json");
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson failed remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson(QString remoteFile, QString localFile, QString errorMsg)
{
    QString jsonTranslationFileName;

    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;

    disconnect(qobject_cast<QGCFileDownload*>(sender()), &QGCFileDownload::downloadComplete, this, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson);
    if (errorMsg.isEmpty()) {
        _jsonTranslationFileName = _downloadCompleteJsonWorker(localFile, "translation.json");
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson failed remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();
    FTPManager*                         ftpManager      = compInfo->vehicle->ftpManager();

    if (compInfo->available) {
        qCDebug(ComponentInformationManagerLog) << "Downloading metadata json" << compInfo->uriMetaData;
        if (_uriIsMAVLinkFTP(compInfo->uriMetaData)) {
            connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson);
            if (!ftpManager->download(compInfo->uriMetaData, QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
                qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson FTPManager::download returned failure";
                disconnect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteMetaDataJson);
                requestMachine->advance();
            }
        } else {
            QGCFileDownload* download = new QGCFileDownload(requestMachine);
            connect(download, &QGCFileDownload::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson);
            if (!download->download(compInfo->uriMetaData)) {
                qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson QGCFileDownload::download returned failure";
                disconnect(download, &QGCFileDownload::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson);
                requestMachine->advance();
            }
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
            if (_uriIsMAVLinkFTP(compInfo->uriTranslation)) {
                connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson);
                if (!ftpManager->download(compInfo->uriTranslation, QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
                    qCWarning(ComponentInformationManagerLog) << "_stateRequestTranslationJson::_stateRequestMetaDataJson FTPManager::download returned failure";
                    connect(ftpManager, &FTPManager::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_ftpDownloadCompleteTranslationJson);
                    requestMachine->advance();
                }
            } else {
                QGCFileDownload* download = new QGCFileDownload(requestMachine);
                connect(download, &QGCFileDownload::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson);
                if (!download->download(compInfo->uriTranslation)) {
                    qCWarning(ComponentInformationManagerLog) << "_stateRequestTranslationJson::_stateRequestMetaDataJson QGCFileDownload::download returned failure";
                    disconnect(download, &QGCFileDownload::downloadComplete, requestMachine, &RequestMetaDataTypeStateMachine::_httpDownloadCompleteTranslationJson);
                    requestMachine->advance();
                }
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

    if (!requestMachine->_jsonMetadataFileName.isEmpty()) {
        QFile(requestMachine->_jsonMetadataFileName).remove();
    }
    if (!requestMachine->_jsonTranslationFileName.isEmpty()) {
        QFile(requestMachine->_jsonTranslationFileName).remove();
    }

    requestMachine->advance();
}

bool RequestMetaDataTypeStateMachine::_uriIsMAVLinkFTP(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}
