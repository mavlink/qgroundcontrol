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
#include "QGCLZMA.h"
#include "JsonHelper.h"
#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "CompInfoEvents.h"
#include "CompInfoActuators.h"
#include "QGCFileDownload.h"
#include "QGCApplication.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformationManagerLog")

const ComponentInformationManager::StateFn ComponentInformationManager::_rgStates[]= {
    ComponentInformationManager::_stateRequestCompInfoGeneral,
    ComponentInformationManager::_stateRequestCompInfoGeneralComplete,
    ComponentInformationManager::_stateRequestCompInfoParam,
    ComponentInformationManager::_stateRequestCompInfoEvents,
    ComponentInformationManager::_stateRequestCompInfoActuators,
    ComponentInformationManager::_stateRequestAllCompInfoComplete
};

const int ComponentInformationManager::_cStates = sizeof(ComponentInformationManager::_rgStates) / sizeof(ComponentInformationManager::_rgStates[0]);

const RequestMetaDataTypeStateMachine::StateFn RequestMetaDataTypeStateMachine::_rgStates[]= {
    RequestMetaDataTypeStateMachine::_stateRequestCompInfo,
    RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson,
    RequestMetaDataTypeStateMachine::_stateRequestMetaDataJsonFallback,
    RequestMetaDataTypeStateMachine::_stateRequestTranslationJson,
    RequestMetaDataTypeStateMachine::_stateRequestComplete,
};

const int RequestMetaDataTypeStateMachine::_cStates = sizeof(RequestMetaDataTypeStateMachine::_rgStates) / sizeof(RequestMetaDataTypeStateMachine::_rgStates[0]);

ComponentInformationManager::ComponentInformationManager(Vehicle* vehicle)
    : _vehicle                  (vehicle)
    , _requestTypeStateMachine  (this)
    , _fileCache(ComponentInformationCache::defaultInstance())
{
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]    = new CompInfoGeneral   (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER]  = new CompInfoParam     (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS]     = new CompInfoEvents    (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_ACTUATORS]  = new CompInfoActuators (MAV_COMP_ID_AUTOPILOT1, vehicle, this);
}

int ComponentInformationManager::stateCount(void) const
{
    return _cStates;
}

const ComponentInformationManager::StateFn* ComponentInformationManager::rgStates(void) const
{
    return &_rgStates[0];
}

float ComponentInformationManager::progress() const
{
    if (!_active)
        return 1.f;
    // here we could compute a more fine-grained progress, based on ftp download progress
    return _stateIndex / (float)_cStates;
}

void ComponentInformationManager::advance()
{
    StateMachine::advance();
    emit progressUpdate(progress());
}

void ComponentInformationManager::requestAllComponentInformation(RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData)
{
    _requestAllCompleteFn       = requestAllCompletFn;
    _requestAllCompleteFnData   = requestAllCompleteFnData;
    start();
    emit progressUpdate(progress());
}

void ComponentInformationManager::_stateRequestCompInfoGeneral(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    compMgr->_requestTypeStateMachine.request(compMgr->_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]);
}

void ComponentInformationManager::_stateRequestCompInfoGeneralComplete(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);
    compMgr->_updateAllUri();
    compMgr->advance();
}

void ComponentInformationManager::_updateAllUri()
{
    CompInfoGeneral* general = qobject_cast<CompInfoGeneral*>(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL]);
    for (auto& compInfo : _compInfoMap[MAV_COMP_ID_AUTOPILOT1]) {
        general->setUris(*compInfo);
    }
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

void ComponentInformationManager::_stateRequestCompInfoEvents(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);

    if (compMgr->_isCompTypeSupported(COMP_METADATA_TYPE_EVENTS)) {
        compMgr->_requestTypeStateMachine.request(compMgr->_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS]);
    } else {
        qCDebug(ComponentInformationManagerLog) << "_stateRequestCompInfoEvents skipping, not supported";
        compMgr->advance();
    }
}

void ComponentInformationManager::_stateRequestCompInfoActuators(StateMachine* stateMachine)
{
    ComponentInformationManager* compMgr = static_cast<ComponentInformationManager*>(stateMachine);

    if (compMgr->_isCompTypeSupported(COMP_METADATA_TYPE_ACTUATORS)) {
        compMgr->_requestTypeStateMachine.request(compMgr->_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_ACTUATORS]);
    } else {
        qCDebug(ComponentInformationManagerLog) << "_stateRequestCompInfoActuators skipping, not supported";
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
    return qobject_cast<CompInfoGeneral*>(_compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL])->isMetaDataTypeSupported(type);
}

CompInfoParam* ComponentInformationManager::compInfoParam(uint8_t compId)
{
    if (!_compInfoMap.contains(compId)) {
        // Create default info
        _compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER] = new CompInfoParam(compId, _vehicle, this);
    }
    return qobject_cast<CompInfoParam*>(_compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER]);
}

CompInfoGeneral* ComponentInformationManager::compInfoGeneral(uint8_t compId)
{
    return _compInfoMap.contains(compId) && _compInfoMap[compId].contains(COMP_METADATA_TYPE_GENERAL) ? qobject_cast<CompInfoGeneral*>(_compInfoMap[compId][COMP_METADATA_TYPE_GENERAL]) : nullptr;
}

QString ComponentInformationManager::_getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation)
{
    return QString::asprintf("%08x_%02i_%i", crc, compInfoType, (int)isTranslation);
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
    switch (_compInfo->type) {
        case COMP_METADATA_TYPE_GENERAL: return "COMP_METADATA_TYPE_GENERAL";
        case COMP_METADATA_TYPE_PARAMETER: return "COMP_METADATA_TYPE_PARAMETER";
        case COMP_METADATA_TYPE_COMMANDS: return "COMP_METADATA_TYPE_COMMANDS";
        case COMP_METADATA_TYPE_PERIPHERALS: return "COMP_METADATA_TYPE_PERIPHERALS";
        case COMP_METADATA_TYPE_EVENTS: return "COMP_METADATA_TYPE_EVENTS";
        case COMP_METADATA_TYPE_ACTUATORS: return "COMP_METADATA_TYPE_ACTUATORS";
        default: break;
    }
    return "Unknown";
}

static void _requestMessageResultHandler(void* resultHandlerData, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t &message)
{
    RequestMetaDataTypeStateMachine* requestMachine = static_cast<RequestMetaDataTypeStateMachine*>(resultHandlerData);

    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_component_information_t componentInformation;
        mavlink_msg_component_information_decode(&message, &componentInformation);
        requestMachine->compInfo()->setUriMetaData(componentInformation.general_metadata_uri, componentInformation.general_metadata_file_crc);
        // TODO: handle peripherals
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

    if (requestMachine->_compInfo->type != COMP_METADATA_TYPE_GENERAL) {
        requestMachine->advance();
        return;
    }

    if (weakLink.expired()) {
        qCDebug(ComponentInformationManagerLog) << QStringLiteral("_stateRequestCompInfo Skipping component information %1 request due to no primary link").arg(requestMachine->typeToString());
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
                        MAVLINK_MSG_ID_COMPONENT_INFORMATION);
        }
    }
}

QString RequestMetaDataTypeStateMachine::_downloadCompleteJsonWorker(const QString& fileName)
{
    QString outputFileName = fileName;

    if (fileName.endsWith(".lzma", Qt::CaseInsensitive) || fileName.endsWith(".xz", Qt::CaseInsensitive)) {
        outputFileName = (QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(_currentCacheFileTag));
        if (QGCLZMA::inflateLZMAFile(fileName, outputFileName)) {
            QFile(fileName).remove();
        } else {
            qCWarning(ComponentInformationManagerLog) << "Inflate of compressed json failed" << _currentCacheFileTag;
            outputFileName.clear();
        }
    } else {
        outputFileName = fileName;
    }

    if (_currentFileValidCrc) {
        // cache the file (this will move/remove the temp file as well)
        outputFileName = _compMgr->fileCache().insert(_currentCacheFileTag, outputFileName);
    }
    return outputFileName;
}

void RequestMetaDataTypeStateMachine::_ftpDownloadComplete(const QString& fileName, const QString& errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadComplete fileName:errorMsg" << fileName << errorMsg;

    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
    disconnect(_compInfo->vehicle->ftpManager(), &FTPManager::commandProgress, this, &RequestMetaDataTypeStateMachine::_ftpDownloadProgress);
    if (errorMsg.isEmpty()) {
        if (_currentFileName) {
            *_currentFileName = _downloadCompleteJsonWorker(fileName);
        }
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_ftpDownloadComplete failed filename:errorMsg" << fileName << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_ftpDownloadProgress(float progress)
{
    int elapsedSec = _downloadStartTime.elapsed() / 1000;
    float totalDownloadTime = elapsedSec / progress;
    // abort download if it's too slow (e.g. over telemetry link) and use the fallback.
    // (we could also check if there's a http fallback)
    const int maxDownloadTimeSec = 40;
    if (elapsedSec > 10 && progress < 0.5 && totalDownloadTime > maxDownloadTimeSec) {
        qCDebug(ComponentInformationManagerLog) << "Slow download, aborting. Total time (s):" << totalDownloadTime;
        _compInfo->vehicle->ftpManager()->cancel();
    }
}

void RequestMetaDataTypeStateMachine::_httpDownloadComplete(QString remoteFile, QString localFile, QString errorMsg)
{
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadComplete remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;

    disconnect(qobject_cast<QGCFileDownload*>(sender()), &QGCFileDownload::downloadComplete, this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);
    if (errorMsg.isEmpty()) {
        if (_currentFileName) {
            *_currentFileName = _downloadCompleteJsonWorker(localFile);
        }
    } else if (qgcApp()->runningUnitTests()) {
        // Unit test should always succeed
        qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_httpDownloadCompleteMetaDataJson failed remoteFile:localFile:errorMsg" << remoteFile << localFile << errorMsg;
    }

    advance();
}

void RequestMetaDataTypeStateMachine::_requestFile(const QString& cacheFileTag, bool crcValid, const QString& uri, QString& outputFileName)
{
    FTPManager*                         ftpManager      = _compInfo->vehicle->ftpManager();
    _currentCacheFileTag = cacheFileTag;
    _currentFileName = &outputFileName;
    _currentFileValidCrc = crcValid;
    outputFileName.clear();

    if (_compInfo->available() && !uri.isEmpty()) {
        const QString cachedFile = crcValid ? _compMgr->fileCache().access(cacheFileTag) : "";

        if (cachedFile.isEmpty()) {
            qCDebug(ComponentInformationManagerLog) << "Downloading json" << uri;
            if (_uriIsMAVLinkFTP(uri)) {
                connect(ftpManager, &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
                if (ftpManager->download(uri, QStandardPaths::writableLocation(QStandardPaths::TempLocation))) {
                    _downloadStartTime.start();
                    connect(ftpManager, &FTPManager::commandProgress, this, &RequestMetaDataTypeStateMachine::_ftpDownloadProgress);
                } else {
                    qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_requestFile FTPManager::download returned failure";
                    disconnect(ftpManager, &FTPManager::downloadComplete, this, &RequestMetaDataTypeStateMachine::_ftpDownloadComplete);
                    advance();
                }
            } else {
                QGCFileDownload* download = new QGCFileDownload(this);
                connect(download, &QGCFileDownload::downloadComplete, this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);
                if (download->download(uri)) {
                    _downloadStartTime.start();
                } else {
                    qCWarning(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_requestFile QGCFileDownload::download returned failure";
                    disconnect(download, &QGCFileDownload::downloadComplete, this, &RequestMetaDataTypeStateMachine::_httpDownloadComplete);
                    advance();
                }
            }
        } else {
            qCDebug(ComponentInformationManagerLog) << "Using cached file" << cachedFile;
            outputFileName = cachedFile;
            advance();
        }
    } else {
        qCDebug(ComponentInformationManagerLog) << "Skipping download. Component information not available for" << _currentCacheFileTag;
        advance();
    }

}

void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();
    const QString                       fileTag         = ComponentInformationManager::_getFileCacheTag(
            compInfo->type, compInfo->crcMetaData(), false);
    const QString                       uri             = compInfo->uriMetaData();
    requestMachine->_jsonMetadataCrcValid               = compInfo->crcMetaDataValid();
    requestMachine->_requestFile(fileTag, compInfo->crcMetaDataValid(), uri, requestMachine->_jsonMetadataFileName);
}

void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJsonFallback(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    if (!requestMachine->_jsonMetadataFileName.isEmpty()) {
        requestMachine->advance();
        return;
    }
    qCDebug(ComponentInformationManagerLog) << "RequestMetaDataTypeStateMachine::_stateRequestMetaDataJsonFallback: trying fallback download";

    CompInfo*                           compInfo        = requestMachine->compInfo();
    const QString                       fileTag         = ComponentInformationManager::_getFileCacheTag(
            compInfo->type, compInfo->crcMetaDataFallback(), false);
    const QString                       uri             = compInfo->uriMetaDataFallback();
    requestMachine->_jsonMetadataCrcValid               = compInfo->crcMetaDataFallbackValid();
    requestMachine->_requestFile(fileTag, compInfo->crcMetaDataFallbackValid(), uri, requestMachine->_jsonMetadataFileName);
}

void RequestMetaDataTypeStateMachine::_stateRequestTranslationJson(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();
    const QString                       fileTag         = ComponentInformationManager::_getFileCacheTag(
            compInfo->type, compInfo->crcTranslation(), true);
    const QString                       uri             = compInfo->uriTranslation();
    requestMachine->_jsonTranslationCrcValid            = compInfo->crcTranslationValid();
    requestMachine->_requestFile(fileTag, compInfo->crcTranslationValid(), uri, requestMachine->_jsonTranslationFileName);
}

void RequestMetaDataTypeStateMachine::_stateRequestComplete(StateMachine* stateMachine)
{
    RequestMetaDataTypeStateMachine*    requestMachine  = static_cast<RequestMetaDataTypeStateMachine*>(stateMachine);
    CompInfo*                           compInfo        = requestMachine->compInfo();

    compInfo->setJson(requestMachine->_jsonMetadataFileName, requestMachine->_jsonTranslationFileName);

    // if we don't have a CRC we didn't cache the file and we need to delete it
    if (!requestMachine->_jsonMetadataCrcValid && !requestMachine->_jsonMetadataFileName.isEmpty()) {
        QFile(requestMachine->_jsonMetadataFileName).remove();
    }
    if (!requestMachine->_jsonMetadataCrcValid && !requestMachine->_jsonTranslationFileName.isEmpty()) {
        QFile(requestMachine->_jsonTranslationFileName).remove();
    }

    requestMachine->advance();
}

bool RequestMetaDataTypeStateMachine::_uriIsMAVLinkFTP(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}
