/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "FirmwarePlugin.h"
#include "UAS.h"
#include "JsonHelper.h"
#include "ComponentInformationManager.h"
#include "CompInfoParam.h"
#include "FTPManager.h"

#include <QEasingCurve>
#include <QFile>
#include <QDebug>
#include <QVariantAnimation>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(ParameterManagerVerbose1Log,           "ParameterManagerVerbose1Log")
QGC_LOGGING_CATEGORY(ParameterManagerVerbose2Log,           "ParameterManagerVerbose2Log")
QGC_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog,  "ParameterManagerDebugCacheFailureLog") // Turn on to debug parameter cache crc misses

const QHash<int, QString> _mavlinkCompIdHash {
    { MAV_COMP_ID_CAMERA,   "Camera1" },
    { MAV_COMP_ID_CAMERA2,  "Camera2" },
    { MAV_COMP_ID_CAMERA3,  "Camera3" },
    { MAV_COMP_ID_CAMERA4,  "Camera4" },
    { MAV_COMP_ID_CAMERA5,  "Camera5" },
    { MAV_COMP_ID_CAMERA6,  "Camera6" },
    { MAV_COMP_ID_SERVO1,   "Servo1" },
    { MAV_COMP_ID_SERVO2,   "Servo2" },
    { MAV_COMP_ID_SERVO3,   "Servo3" },
    { MAV_COMP_ID_SERVO4,   "Servo4" },
    { MAV_COMP_ID_SERVO5,   "Servo5" },
    { MAV_COMP_ID_SERVO6,   "Servo6" },
    { MAV_COMP_ID_SERVO7,   "Servo7" },
    { MAV_COMP_ID_SERVO8,   "Servo8" },
    { MAV_COMP_ID_SERVO9,   "Servo9" },
    { MAV_COMP_ID_SERVO10,  "Servo10" },
    { MAV_COMP_ID_SERVO11,  "Servo11" },
    { MAV_COMP_ID_SERVO12,  "Servo12" },
    { MAV_COMP_ID_SERVO13,  "Servo13" },
    { MAV_COMP_ID_SERVO14,  "Servo14" },
    { MAV_COMP_ID_GIMBAL,   "Gimbal1" },
    { MAV_COMP_ID_ADSB,     "ADSB" },
    { MAV_COMP_ID_OSD,      "OSD" },
    { MAV_COMP_ID_FLARM,    "FLARM" },
    { MAV_COMP_ID_GIMBAL2,  "Gimbal2" },
    { MAV_COMP_ID_GIMBAL3,  "Gimbal3" },
    { MAV_COMP_ID_GIMBAL4,  "Gimbal4" },
    { MAV_COMP_ID_GIMBAL5,  "Gimbal5" },
    { MAV_COMP_ID_GIMBAL6,  "Gimbal6" },
    { MAV_COMP_ID_IMU,      "IMU1" },
    { MAV_COMP_ID_IMU_2,    "IMU2" },
    { MAV_COMP_ID_IMU_3,    "IMU3" },
    { MAV_COMP_ID_GPS,      "GPS1" },
    { MAV_COMP_ID_GPS2,     "GPS2" }
};

ParameterManager::ParameterManager(Vehicle* vehicle)
    : QObject                           (vehicle)
    , _vehicle                          (vehicle)
    , _mavlink                          (nullptr)
    , _loadProgress                     (0.0)
    , _parametersReady                  (false)
    , _missingParameters                (false)
    , _initialLoadComplete              (false)
    , _waitingForDefaultComponent       (false)
    , _saveRequired                     (false)
    , _metaDataAddedToFacts             (false)
    , _logReplay                        (!vehicle->vehicleLinkManager()->primaryLink().expired() && vehicle->vehicleLinkManager()->primaryLink().lock()->isLogReplay())
    , _prevWaitingReadParamIndexCount   (0)
    , _prevWaitingReadParamNameCount    (0)
    , _prevWaitingWriteParamNameCount   (0)
    , _initialRequestRetryCount         (0)
    , _disableAllRetries                (false)
    , _indexBatchQueueActive            (false)
    , _totalParamCount                  (0)
    , _tryftp                           (vehicle->apmFirmware())
{
    if (_vehicle->isOfflineEditingVehicle()) {
        _loadOfflineEditingParams();
        return;
    }

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    _initialRequestTimeoutTimer.setSingleShot(true);
    _initialRequestTimeoutTimer.setInterval(5000);
    connect(&_initialRequestTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_initialRequestTimeout);

    _waitingParamTimeoutTimer.setSingleShot(true);
    _waitingParamTimeoutTimer.setInterval(3000);
    connect(&_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_waitingParamTimeout);

    // Ensure the cache directory exists
    QFileInfo(QSettings().fileName()).dir().mkdir("ParamCache");
}

void ParameterManager::_updateProgressBar(void)
{
    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamCount = 0;

    for (int compId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[compId].count();
    }
    for(int compId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[compId].count();
    }
    for(int compId: _waitingWriteParamNameMap.keys()) {
        waitingWriteParamCount += _waitingWriteParamNameMap[compId].count();
    }

    if (waitingReadParamIndexCount == 0) {
        if (_readParamIndexProgressActive) {
            _readParamIndexProgressActive = false;
            _setLoadProgress(0.0);
            return;
        }
    } else {
        _readParamIndexProgressActive = true;
        _setLoadProgress((double)(_totalParamCount - waitingReadParamIndexCount) / (double)_totalParamCount);
        return;
    }

    if (waitingWriteParamCount == 0) {
        if (_writeParamProgressActive) {
            _writeParamProgressActive = false;
            _waitingWriteParamBatchCount = 0;
            _setLoadProgress(0.0);
            emit pendingWritesChanged(false);
            return;
        }
    } else {
        _writeParamProgressActive = true;
        _setLoadProgress((double)(qMax(_waitingWriteParamBatchCount - waitingWriteParamCount, 1)) / (double)(_waitingWriteParamBatchCount + 1));
        emit pendingWritesChanged(true);
        return;
    }

    if (waitingReadParamNameCount == 0) {
        if (_readParamNameProgressActive) {
            _readParamNameProgressActive = false;
            _waitingReadParamNameBatchCount = 0;
            _setLoadProgress(0.0);
            return;
        }
    } else {
        _readParamNameProgressActive = true;
        _setLoadProgress((double)(qMax(_waitingReadParamNameBatchCount - waitingReadParamNameCount, 1)) / (double)(_waitingReadParamNameBatchCount + 1));
        return;
    }
}


void ParameterManager::mavlinkMessageReceived(mavlink_message_t message)
{
    if (_tryftp && message.compid == MAV_COMP_ID_AUTOPILOT1 && !_initialLoadComplete)
        return;

    if (message.msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        mavlink_param_value_t param_value;
        mavlink_msg_param_value_decode(&message, &param_value);

        // This will null terminate the name string
        QByteArray bytes(param_value.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
        QString parameterName(bytes);

        mavlink_param_union_t paramUnion;
        paramUnion.param_float  = param_value.param_value;
        paramUnion.type         = param_value.param_type;

        QVariant parameterValue;

        switch (paramUnion.type) {
        case MAV_PARAM_TYPE_REAL32:
            parameterValue = QVariant(paramUnion.param_float);
            break;
        case MAV_PARAM_TYPE_UINT8:
            parameterValue = QVariant(paramUnion.param_uint8);
            break;
        case MAV_PARAM_TYPE_INT8:
            parameterValue = QVariant(paramUnion.param_int8);
            break;
        case MAV_PARAM_TYPE_UINT16:
            parameterValue = QVariant(paramUnion.param_uint16);
            break;
        case MAV_PARAM_TYPE_INT16:
            parameterValue = QVariant(paramUnion.param_int16);
            break;
        case MAV_PARAM_TYPE_UINT32:
            parameterValue = QVariant(paramUnion.param_uint32);
            break;
        case MAV_PARAM_TYPE_INT32:
            parameterValue = QVariant(paramUnion.param_int32);
            break;
        default:
            qCritical() << "ParameterManager::_handleParamValue - unsupported MAV_PARAM_TYPE" << paramUnion.type;
            break;
        }

        _handleParamValue(message.compid, parameterName, param_value.param_count, param_value.param_index, static_cast<MAV_PARAM_TYPE>(param_value.param_type), parameterValue);
    }
}

/// Called whenever a parameter is updated or first seen.
void ParameterManager::_handleParamValue(int componentId, QString parameterName, int parameterCount, int parameterIndex, MAV_PARAM_TYPE mavParamType, QVariant parameterValue)
{

    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) <<
                                            "_parameterUpdate" <<
                                            "name:" << parameterName <<
                                            "count:" << parameterCount <<
                                            "index:" << parameterIndex <<
                                            "mavType:" << mavParamType <<
                                            "value:" << parameterValue <<
                                            ")";

    // ArduPilot has this strange behavior of streaming parameters that we didn't ask for. This even happens before it responds to the
    // PARAM_REQUEST_LIST. We disregard any of this until the initial request is responded to.
    if (parameterIndex == 65535 && parameterName != "_HASH_CHECK" && _initialRequestTimeoutTimer.isActive()) {
        qCDebug(ParameterManagerVerbose1Log) << "Disregarding unrequested param prior to initial list response" << parameterName;
        return;
    }

    _initialRequestTimeoutTimer.stop();

#if 0
    if (!_initialLoadComplete && !_indexBatchQueueActive) {
        // Handy for testing retry logic
        static int counter = 0;
        if (counter++ & 0x8) {
            qCDebug(ParameterManagerLog) << "Artificial discard" << counter;
            return;
        }
    }
#endif

#if 0
    // Use this to test missing default component id
    if (componentId == 50) {
        return;
    }
#endif

    if (_vehicle->px4Firmware() && parameterName == "_HASH_CHECK") {
        if (!_initialLoadComplete && !_logReplay) {
            /* we received a cache hash, potentially load from cache */
            _tryCacheHashLoad(_vehicle->id(), componentId, parameterValue);
        }
        return;
    }

    // Used to debug cache crc misses (turn on ParameterManagerDebugCacheFailureLog)
    if (!_initialLoadComplete && !_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
        if (_debugCacheMap[componentId].contains(parameterName)) {
            const ParamTypeVal& cacheParamTypeVal   = _debugCacheMap[componentId][parameterName];
            size_t              dataSize            = FactMetaData::typeToSize(static_cast<FactMetaData::ValueType_t>(cacheParamTypeVal.first));
            const void*         cacheData           = cacheParamTypeVal.second.constData();
            const void*         vehicleData         = parameterValue.constData();

            if (memcmp(cacheData, vehicleData, dataSize)) {
                qDebug() << "Cache/Vehicle values differ for name:cache:actual" << parameterName << parameterValue << cacheParamTypeVal.second;
            }
            _debugCacheParamSeen[componentId][parameterName] = true;
        } else {
            qDebug() << "Parameter missing from cache" << parameterName;
        }
    }

    _initialRequestTimeoutTimer.stop();
    _waitingParamTimeoutTimer.stop();

    // Update our total parameter counts
    if (!_paramCountMap.contains(componentId)) {
        _paramCountMap[componentId] = parameterCount;
        _totalParamCount += parameterCount;
    }

    // If we've never seen this component id before, setup the index wait lists.
    if (!_waitingReadParamIndexMap.contains(componentId)) {
        // Add all indices to the wait list, parameter index is 0-based
        for (int waitingIndex=0; waitingIndex<parameterCount; waitingIndex++) {
            // This will add the new component id, as well as the the new waiting index and set the retry count for that index to 0
            _waitingReadParamIndexMap[componentId][waitingIndex] = 0;
        }

        // The read and write waiting lists for this component are initialized the empty
        _waitingReadParamNameMap[componentId] = QMap<QString, int>();
        _waitingWriteParamNameMap[componentId] = QMap<QString, int>();

        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Seeing component for first time - paramcount:" << parameterCount;
    }

    if (!_waitingReadParamIndexMap[componentId].contains(parameterIndex) &&
            !_waitingReadParamNameMap[componentId].contains(parameterName) &&
            !_waitingWriteParamNameMap[componentId].contains(parameterName)) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Unrequested param update" << parameterName;
    }

    // Remove this parameter from the waiting lists
    if (_waitingReadParamIndexMap[componentId].contains(parameterIndex)) {
        _waitingReadParamIndexMap[componentId].remove(parameterIndex);
        _indexBatchQueue.removeOne(parameterIndex);
        _fillIndexBatchQueue(false /* waitingParamTimeout */);
    }
    _waitingReadParamNameMap[componentId].remove(parameterName);
    _waitingWriteParamNameMap[componentId].remove(parameterName);
    if (_waitingReadParamIndexMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap:" << _waitingReadParamIndexMap[componentId];
    }
    if (_waitingReadParamNameMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamNameMap" << _waitingReadParamNameMap[componentId];
    }
    if (_waitingWriteParamNameMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingWriteParamNameMap" << _waitingWriteParamNameMap[componentId];
    }

    // Track how many parameters we are still waiting for

    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamNameCount = 0;

    for(int waitingComponentId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[waitingComponentId].count();
    }
    if (waitingReadParamIndexCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamIndexCount:" << waitingReadParamIndexCount;
    }

    for(int waitingComponentId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[waitingComponentId].count();
    }
    if (waitingReadParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamNameCount:" << waitingReadParamNameCount;
    }

    for(int waitingComponentId: _waitingWriteParamNameMap.keys()) {
        waitingWriteParamNameCount += _waitingWriteParamNameMap[waitingComponentId].count();
    }
    if (waitingWriteParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingWriteParamNameCount:" << waitingWriteParamNameCount;
    }

    int readWaitingParamCount = waitingReadParamIndexCount + waitingReadParamNameCount;
    int totalWaitingParamCount = readWaitingParamCount + waitingWriteParamNameCount;
    if (totalWaitingParamCount) {
        // More params to wait for, restart timer
        _waitingParamTimeoutTimer.start();
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer: totalWaitingParamCount:" << totalWaitingParamCount;
    } else {
        if (!_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
            // Still waiting for parameters from default component
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer (still waiting for default component params)";
            _waitingParamTimeoutTimer.start();
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Not restarting _waitingParamTimeoutTimer (all requests satisfied)";
        }
    }

    _updateProgressBar();

    Fact* fact = nullptr;
    if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
        fact = _mapCompId2FactMap[componentId][parameterName];
    } else {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

        fact = new Fact(componentId, parameterName, mavTypeToFactType(mavParamType), this);
        FactMetaData* factMetaData = _vehicle->compInfoManager()->compInfoParam(componentId)->factMetaDataForName(parameterName, fact->type());
        fact->setMetaData(factMetaData);

        _mapCompId2FactMap[componentId][parameterName] = fact;

        // We need to know when the fact value changes so we can update the vehicle
        connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);

        emit factAdded(componentId, fact);
    }

    fact->_containerSetRawValue(parameterValue);

    // Update param cache. The param cache is only used on PX4 Firmware since ArduPilot and Solo have volatile params
    // which invalidate the cache. The Solo also streams param updates in flight for things like gimbal values
    // which in turn causes a perf problem with all the param cache updates.
    if (!_logReplay && _vehicle->px4Firmware()) {
        if (_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount != 0 && readWaitingParamCount == 0) {
            // All reads just finished, update the cache
            _writeLocalParamCache(_vehicle->id(), componentId);
        }
    }

    _prevWaitingReadParamIndexCount = waitingReadParamIndexCount;
    _prevWaitingReadParamNameCount = waitingReadParamNameCount;
    _prevWaitingWriteParamNameCount = waitingWriteParamNameCount;

    _checkInitialLoadComplete();

    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_parameterUpdate complete";
}

/// Writes the parameter update to mavlink, sets up for write wait
void ParameterManager::_factRawValueUpdateWorker(int componentId, const QString& name, FactMetaData::ValueType_t valueType, const QVariant& rawValue)
{
    if (_waitingWriteParamNameMap.contains(componentId)) {
        if (_waitingWriteParamNameMap[componentId].contains(name)) {
            _waitingWriteParamNameMap[componentId].remove(name);
        } else {
            _waitingWriteParamBatchCount++;
        }
        _waitingWriteParamNameMap[componentId][name] = 0; // Add new entry and set retry count
        _updateProgressBar();
        _waitingParamTimeoutTimer.start();
        _saveRequired = true;
    } else {
        qWarning() << "Internal error ParameterManager::_factValueUpdateWorker: component id not found" << componentId;
    }

    _sendParamSetToVehicle(componentId, name, valueType, rawValue);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Update parameter (_waitingParamTimeoutTimer started) - compId:name:rawValue" << componentId << name << rawValue;
}

void ParameterManager::_factRawValueUpdated(const QVariant& rawValue)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    if (!fact) {
        qWarning() << "Internal error";
        return;
    }

    _factRawValueUpdateWorker(fact->componentId(), fact->name(), fact->type(), rawValue);
}

void ParameterManager::_ftpDownloadComplete(const QString& fileName, const QString& errorMsg)
{
    bool continueWithDefaultParameterdownload = true;
    bool immediateRetry = false;

    disconnect(_vehicle->ftpManager(), &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
    disconnect(_vehicle->ftpManager(), &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);

    if (errorMsg.isEmpty()) {
        qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parameter file received:" << fileName;
        if (_parseParamFile(fileName)) {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parsed!";
            return;
        } else {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Error in parameter file";
            /* This should not happen... */
        }
    } else {
        if (errorMsg.contains("File Not Found")) {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp: No Parameterfile on vehicle - Start Conventional Parameter Download";
            if (_initialRequestRetryCount == 0)
                immediateRetry = true;
        } else if (_loadProgress > 0.0001 && _loadProgress < 0.01) { /* FTP supported but too slow */
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp progress too slow - Start Conventional Parameter Download";
        } else if (_initialRequestRetryCount == 1) {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp: Too many retries - Start Conventional Parameter Download";
        } else {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp Retry: " << _initialRequestRetryCount;
            continueWithDefaultParameterdownload = false;
        }
    }

    if (continueWithDefaultParameterdownload) {
        _tryftp = false;
        _initialRequestRetryCount = 0;
        /* If we receive "File not Found" this indicates that the vehicle does not support
         * the parameter download via ftp. If we received this without retry, then we
         * can immediately response with the conventional parameter download request, because
         * we have no indication of communication link congestion.*/
        if (immediateRetry)
            _initialRequestTimeout();
        else
            _initialRequestTimeoutTimer.start();
    } else
        _initialRequestTimeoutTimer.start();

}


void ParameterManager::_ftpDownloadProgress(float progress)
{
    qCDebug(ParameterManagerVerbose1Log) << "ParameterManager::_ftpDownloadProgress: " << progress;
    _setLoadProgress(static_cast<double>(progress));
    if (progress > 0.001)
        _initialRequestTimeoutTimer.stop();
}

void ParameterManager::refreshAllParameters(uint8_t componentId)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();

    if (weakLink.expired()) {
        return;
    }

    if (weakLink.lock()->linkConfiguration()->isHighLatency() || _logReplay) {
        // These links don't load params
        _parametersReady = true;
        _missingParameters = true;
        _initialLoadComplete = true;
        _waitingForDefaultComponent = false;
        emit parametersReadyChanged(_parametersReady);
        emit missingParametersChanged(_missingParameters);
    }

    if (!_initialLoadComplete) {
        _initialRequestTimeoutTimer.start();
    }

    if (_tryftp && (componentId == MAV_COMP_ID_ALL || componentId == MAV_COMP_ID_AUTOPILOT1)) {
        FTPManager* ftpManager = _vehicle->ftpManager();
        connect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        _waitingParamTimeoutTimer.stop();
        if (ftpManager->download(MAV_COMP_ID_AUTOPILOT1, "@PARAM/param.pck",
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation),
                                 "", false /* No filesize check */)) {
            connect(ftpManager, &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);
        } else {
            qCWarning(ParameterManagerLog) << "ParameterManager::refreshallParameters FTPManager::download returned failure";
            disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        }
    } else {
        // Reset index wait lists
        for (int cid: _paramCountMap.keys()) {
            // Add/Update all indices to the wait list, parameter index is 0-based
            if(componentId != MAV_COMP_ID_ALL && componentId != cid)
                continue;
            for (int waitingIndex = 0; waitingIndex < _paramCountMap[cid]; waitingIndex++) {
                // This will add a new waiting index if needed and set the retry count for that index to 0
                _waitingReadParamIndexMap[cid][waitingIndex] = 0;
            }
        }
        MAVLinkProtocol*        mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_param_request_list_pack_chan(mavlink->getSystemId(),
                                                 mavlink->getComponentId(),
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,
                                                 _vehicle->id(),
                                                 componentId);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    QString what = (componentId == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Request to refresh all parameters for component ID:" << what;
}

/// Translates FactSystem::defaultComponentId to real component id if needed
int ParameterManager::_actualComponentId(int componentId)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _vehicle->defaultComponentId();
        if (componentId == FactSystem::defaultComponentId) {
            qWarning() << _logVehiclePrefix(-1) << "Default component id not set";
        }
    }

    return componentId;
}

void ParameterManager::refreshParameter(int componentId, const QString& paramName)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParameter - name:" << paramName << ")";

    if (_waitingReadParamNameMap.contains(componentId)) {
        QString mappedParamName = _remapParamNameToVersion(paramName);

        if (_waitingReadParamNameMap[componentId].contains(mappedParamName)) {
            _waitingReadParamNameMap[componentId].remove(mappedParamName);
        } else {
            _waitingReadParamNameBatchCount++;
        }
        _waitingReadParamNameMap[componentId][mappedParamName] = 0;     // Add new wait entry and update retry count
        _updateProgressBar();
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "restarting _waitingParamTimeout";
        _waitingParamTimeoutTimer.start();
    } else {
        qWarning() << "Internal error";
    }

    _readParameterRaw(componentId, paramName, -1);
}

void ParameterManager::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParametersPrefix - name:" << namePrefix << ")";

    for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
        if (paramName.startsWith(namePrefix)) {
            refreshParameter(componentId, paramName);
        }
    }
}

bool ParameterManager::parameterExists(int componentId, const QString& paramName)
{
    bool ret = false;

    componentId = _actualComponentId(componentId);
    if (_mapCompId2FactMap.contains(componentId)) {
        ret = _mapCompId2FactMap[componentId].contains(_remapParamNameToVersion(paramName));
    }

    return ret;
}

Fact* ParameterManager::getParameter(int componentId, const QString& paramName)
{
    componentId = _actualComponentId(componentId);

    QString mappedParamName = _remapParamNameToVersion(paramName);
    if (!_mapCompId2FactMap.contains(componentId) || !_mapCompId2FactMap[componentId].contains(mappedParamName)) {
        qgcApp()->reportMissingParameter(componentId, mappedParamName);
        return &_defaultFact;
    }

    return _mapCompId2FactMap[componentId][mappedParamName];
}

QStringList ParameterManager::parameterNames(int componentId)
{
    QStringList names;

    for(const QString &paramName: _mapCompId2FactMap[_actualComponentId(componentId)].keys()) {
        names << paramName;
    }

    return names;
}

/// Requests missing index based parameters from the vehicle.
///     @param waitingParamTimeout: true: being called due to timeout, false: being called to re-fill the batch queue
/// return true: Parameters were requested, false: No more requests needed
bool ParameterManager::_fillIndexBatchQueue(bool waitingParamTimeout)
{
    if (!_indexBatchQueueActive) {
        return false;
    }

    const int maxBatchSize = 10;

    if (waitingParamTimeout) {
        // We timed out, clear the queue and try again
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to timeout";
        _indexBatchQueue.clear();
    } else {
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to received parameter";
    }

    for(int componentId: _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap count" << _waitingReadParamIndexMap[componentId].count();
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap" << _waitingReadParamIndexMap[componentId];
        }

        for(int paramIndex: _waitingReadParamIndexMap[componentId].keys()) {
            if (_indexBatchQueue.contains(paramIndex)) {
                // Don't add more than once
                continue;
            }

            if (_indexBatchQueue.count() > maxBatchSize) {
                break;
            }

            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_disableAllRetries || _waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam) {
                // Give up on this index
                _failedReadParamIndexMap[componentId] << paramIndex;
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Giving up on (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
                _waitingReadParamIndexMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                _indexBatchQueue.append(paramIndex);
                _readParameterRaw(componentId, "", paramIndex);
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
            }
        }
    }

    return _indexBatchQueue.count() != 0;
}

void ParameterManager::_waitingParamTimeout(void)
{
    if (_logReplay) {
        return;
    }

    bool paramsRequested = false;
    const int maxBatchSize = 10;
    int batchCount = 0;

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "_waitingParamTimeout";

    // Now that we have timed out for possibly the first time we can activate the index batch queue
    _indexBatchQueueActive = true;

    // First check for any missing parameters from the initial index based load
    paramsRequested = _fillIndexBatchQueue(true /* waitingParamTimeout */);

    if (!paramsRequested && !_waitingForDefaultComponent && !_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
        // Initial load is complete but we still don't have any default component params. Wait one more cycle to see if the
        // any show up.
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer - still don't have default component params" << _vehicle->defaultComponentId();
        _waitingParamTimeoutTimer.start();
        _waitingForDefaultComponent = true;
        return;
    }
    _waitingForDefaultComponent = false;

    _checkInitialLoadComplete();

    if (!paramsRequested) {
        for(int componentId: _waitingWriteParamNameMap.keys()) {
            for(const QString &paramName: _waitingWriteParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingWriteParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    Fact* fact = getParameter(componentId, paramName);
                    _sendParamSetToVehicle(componentId, paramName, fact->type(), fact->rawValue());
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Write resend for (paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingWriteParamNameMap[componentId].remove(paramName);
                    QString errorMsg = tr("Parameter write failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showAppMessage(errorMsg);
                }
            }
        }
    }

    if (!paramsRequested) {
        for(int componentId: _waitingReadParamNameMap.keys()) {
            for(const QString &paramName: _waitingReadParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingReadParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingReadParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    _readParameterRaw(componentId, paramName, -1);
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramName:" << paramName << "retryCount:" << _waitingReadParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingReadParamNameMap[componentId].remove(paramName);
                    QString errorMsg = tr("Parameter read failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showAppMessage(errorMsg);
                }
            }
        }
    }

Out:
    if (paramsRequested) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer - re-request";
        _waitingParamTimeoutTimer.start();
    }
}

void ParameterManager::_readParameterRaw(int componentId, const QString& paramName, int paramIndex)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        char                    fixedParamName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN];
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();


        strncpy(fixedParamName, paramName.toStdString().c_str(), sizeof(fixedParamName));
        mavlink_msg_param_request_read_pack_chan(_mavlink->getSystemId(),   // QGC system id
                                                 _mavlink->getComponentId(),     // QGC component id
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,                           // Pack into this mavlink_message_t
                                                 _vehicle->id(),                 // Target system id
                                                 componentId,                    // Target component id
                                                 fixedParamName,                 // Named parameter being requested
                                                 paramIndex);                    // Parameter index being requested, -1 for named
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void ParameterManager::_sendParamSetToVehicle(int componentId, const QString& paramName, FactMetaData::ValueType_t valueType, const QVariant& value)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_param_set_t     p;
        mavlink_param_union_t   union_value;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        memset(&p, 0, sizeof(p));

        p.param_type = factTypeToMavType(valueType);

        switch (valueType) {
        case FactMetaData::valueTypeUint8:
            union_value.param_uint8 = (uint8_t)value.toUInt();
            break;

        case FactMetaData::valueTypeInt8:
            union_value.param_int8 = (int8_t)value.toInt();
            break;

        case FactMetaData::valueTypeUint16:
            union_value.param_uint16 = (uint16_t)value.toUInt();
            break;

        case FactMetaData::valueTypeInt16:
            union_value.param_int16 = (int16_t)value.toInt();
            break;

        case FactMetaData::valueTypeUint32:
            union_value.param_uint32 = (uint32_t)value.toUInt();
            break;

        case FactMetaData::valueTypeFloat:
            union_value.param_float = value.toFloat();
            break;

        default:
            qCritical() << "Unsupported fact falue type" << valueType;
            // fall through

        case FactMetaData::valueTypeInt32:
            union_value.param_int32 = (int32_t)value.toInt();
            break;
        }

        p.param_value = union_value.param_float;
        p.target_system = (uint8_t)_vehicle->id();
        p.target_component = (uint8_t)componentId;

        strncpy(p.param_id, paramName.toStdString().c_str(), sizeof(p.param_id));

        mavlink_message_t msg;
        mavlink_msg_param_set_encode_chan(_mavlink->getSystemId(),
                                          _mavlink->getComponentId(),
                                          sharedLink->mavlinkChannel(),
                                          &msg,
                                          &p);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void ParameterManager::_writeLocalParamCache(int vehicleId, int componentId)
{
    CacheMapName2ParamTypeVal cacheMap;

    for (const QString& paramName: _mapCompId2FactMap[componentId].keys()) {
        const Fact *fact = _mapCompId2FactMap[componentId][paramName];
        cacheMap[paramName] = ParamTypeVal(fact->type(), fact->rawValue());
    }

    QFile cacheFile(parameterCacheFile(vehicleId, componentId));
    cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream ds(&cacheFile);
    ds << cacheMap;
}

QDir ParameterManager::parameterCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    return spath + QDir::separator() + "ParamCache";
}

QString ParameterManager::parameterCacheFile(int vehicleId, int componentId)
{
    return parameterCacheDir().filePath(QString("%1_%2.v2").arg(vehicleId).arg(componentId));
}

void ParameterManager::_tryCacheHashLoad(int vehicleId, int componentId, QVariant hash_value)
{
    qCInfo(ParameterManagerLog) << "Attemping load from cache";

    uint32_t crc32_value = 0;
    /* The datastructure of the cache table */
    CacheMapName2ParamTypeVal cacheMap;
    QFile cacheFile(parameterCacheFile(vehicleId, componentId));
    if (!cacheFile.exists()) {
        /* no local cache, just wait for them to come in*/
        return;
    }
    cacheFile.open(QIODevice::ReadOnly);

    /* Deserialize the parameter cache table */
    QDataStream ds(&cacheFile);
    ds >> cacheMap;

    /* compute the crc of the local cache to check against the remote */

    for (const QString& name: cacheMap.keys()) {
        const ParamTypeVal&             paramTypeVal    = cacheMap[name];
        const FactMetaData::ValueType_t fact_type       = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);

        if (_vehicle->compInfoManager()->compInfoParam(MAV_COMP_ID_AUTOPILOT1)->factMetaDataForName(name, fact_type)->volatileValue()) {
            // Does not take part in CRC
            qCDebug(ParameterManagerLog) << "Volatile parameter" << name;
        } else {
            const void *vdat = paramTypeVal.second.constData();
            const FactMetaData::ValueType_t fact_type = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);
            crc32_value = QGC::crc32((const uint8_t *)qPrintable(name), name.length(),  crc32_value);
            crc32_value = QGC::crc32((const uint8_t *)vdat, FactMetaData::typeToSize(fact_type), crc32_value);
        }
    }

    /* if the two param set hashes match, just load from the disk */
    if (crc32_value == hash_value.toUInt()) {
        qCInfo(ParameterManagerLog) << "Parameters loaded from cache" << qPrintable(QFileInfo(cacheFile).absoluteFilePath());

        int count = cacheMap.count();
        int index = 0;
        for (const QString& name: cacheMap.keys()) {
            const ParamTypeVal& paramTypeVal = cacheMap[name];
            const FactMetaData::ValueType_t fact_type = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);
            const MAV_PARAM_TYPE mavParamType = factTypeToMavType(fact_type);
            _handleParamValue(componentId, name, count, index++, mavParamType, paramTypeVal.second);
        }

        WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();

        if (!weakLink.expired()) {
            mavlink_param_set_t     p;
            mavlink_param_union_t   union_value;
            SharedLinkInterfacePtr  sharedLink = weakLink.lock();

            // Return the hash value to notify we don't want any more updates
            memset(&p, 0, sizeof(p));
            p.param_type = MAV_PARAM_TYPE_UINT32;
            strncpy(p.param_id, "_HASH_CHECK", sizeof(p.param_id));
            union_value.param_uint32 = crc32_value;
            p.param_value = union_value.param_float;
            p.target_system = (uint8_t)_vehicle->id();
            p.target_component = (uint8_t)componentId;
            mavlink_message_t msg;
            mavlink_msg_param_set_encode_chan(_mavlink->getSystemId(),
                                              _mavlink->getComponentId(),
                                              sharedLink->mavlinkChannel(),
                                              &msg,
                                              &p);
            _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }

        // Give the user some feedback things loaded properly
        QVariantAnimation *ani = new QVariantAnimation(this);
        ani->setEasingCurve(QEasingCurve::OutCubic);
        ani->setStartValue(0.0);
        ani->setEndValue(1.0);
        ani->setDuration(750);

        connect(ani, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            _setLoadProgress(value.toDouble());
        });

        // Hide 500ms after animation finishes
        connect(ani, &QVariantAnimation::finished, this, [this] {
            QTimer::singleShot(500, [this] {
                _setLoadProgress(0);
            });
        });

        ani->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        qCInfo(ParameterManagerLog) << "Parameters cache match failed" << qPrintable(QFileInfo(cacheFile).absoluteFilePath());
        if (ParameterManagerDebugCacheFailureLog().isDebugEnabled()) {
            _debugCacheCRC[componentId] = true;
            _debugCacheMap[componentId] = cacheMap;
            for (const QString& name: cacheMap.keys()) {
                _debugCacheParamSeen[componentId][name] = false;
            }
            qgcApp()->showAppMessage(tr("Parameter cache CRC match failed"));
        }
    }
}

QString ParameterManager::readParametersFromStream(QTextStream& stream)
{
    QString missingErrors;
    QString typeErrors;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#")) {
            QStringList wpParams = line.split("\t");
            int lineMavId = wpParams.at(0).toInt();
            if (wpParams.size() == 5) {
                if (_vehicle->id() != lineMavId) {
                    return QString("The parameters in the stream have been saved from System Id %1, but the current vehicle has the System Id %2.").arg(lineMavId).arg(_vehicle->id());
                }

                int     componentId = wpParams.at(1).toInt();
                QString paramName = wpParams.at(2);
                QString valStr = wpParams.at(3);
                uint    mavType = wpParams.at(4).toUInt();

                if (!parameterExists(componentId, paramName)) {
                    QString error;
                    error += QStringLiteral("%1:%2 ").arg(componentId).arg(paramName);
                    missingErrors += error;
                    qCDebug(ParameterManagerLog) << QStringLiteral("Skipped due to missing: %1").arg(error);
                    continue;
                }

                Fact* fact = getParameter(componentId, paramName);
                if (fact->type() != mavTypeToFactType((MAV_PARAM_TYPE)mavType)) {
                    QString error;
                    error  = QStringLiteral("%1:%2 ").arg(componentId).arg(paramName);
                    typeErrors += error;
                    qCDebug(ParameterManagerLog) << QStringLiteral("Skipped due to type mismatch: %1").arg(error);
                    continue;
                }

                qCDebug(ParameterManagerLog) << "Updating parameter" << componentId << paramName << valStr;
                fact->setRawValue(valStr);
            }
        }
    }

    QString errors;

    if (!missingErrors.isEmpty()) {
        errors = tr("Parameters not loaded since they are not currently on the vehicle: %1\n").arg(missingErrors);
    }

    if (!typeErrors.isEmpty()) {
        errors += tr("Parameters not loaded due to type mismatch: %1").arg(typeErrors);
    }

    return errors;
}

void ParameterManager::writeParametersToStream(QTextStream& stream)
{
    stream << "# Onboard parameters for Vehicle " << _vehicle->id() << "\n";
    stream << "#\n";

    stream << "# Stack: " << _vehicle->firmwareTypeString() << "\n";
    stream << "# Vehicle: " << _vehicle->vehicleTypeString() << "\n";
    stream << "# Version: "
           << _vehicle->firmwareMajorVersion() << "."
           << _vehicle->firmwareMinorVersion() << "."
           << _vehicle->firmwarePatchVersion() << " "
           << _vehicle->firmwareVersionTypeString() << "\n";
    stream << "# Git Revision: " << _vehicle->gitHash() << "\n";

    stream << "#\n";
    stream << "# Vehicle-Id Component-Id Name Value Type\n";

    for (int componentId: _mapCompId2FactMap.keys()) {
        for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
            Fact* fact = _mapCompId2FactMap[componentId][paramName];
            if (fact) {
                stream << _vehicle->id() << "\t" << componentId << "\t" << paramName << "\t" << fact->rawValueStringFullPrecision() << "\t" << QString("%1").arg(factTypeToMavType(fact->type())) << "\n";
            } else {
                qWarning() << "Internal error: missing fact";
            }
        }
    }

    stream.flush();
}

MAV_PARAM_TYPE ParameterManager::factTypeToMavType(FactMetaData::ValueType_t factType)
{
    switch (factType) {
    case FactMetaData::valueTypeUint8:
        return MAV_PARAM_TYPE_UINT8;

    case FactMetaData::valueTypeInt8:
        return MAV_PARAM_TYPE_INT8;

    case FactMetaData::valueTypeUint16:
        return MAV_PARAM_TYPE_UINT16;

    case FactMetaData::valueTypeInt16:
        return MAV_PARAM_TYPE_INT16;

    case FactMetaData::valueTypeUint32:
        return MAV_PARAM_TYPE_UINT32;

    case FactMetaData::valueTypeUint64:
        return MAV_PARAM_TYPE_UINT64;

    case FactMetaData::valueTypeInt64:
        return MAV_PARAM_TYPE_INT64;

    case FactMetaData::valueTypeFloat:
        return MAV_PARAM_TYPE_REAL32;

    case FactMetaData::valueTypeDouble:
        return MAV_PARAM_TYPE_REAL64;

    default:
        qWarning() << "Unsupported fact type" << factType;
        // fall through

    case FactMetaData::valueTypeInt32:
        return MAV_PARAM_TYPE_INT32;
    }
}

FactMetaData::ValueType_t ParameterManager::mavTypeToFactType(MAV_PARAM_TYPE mavType)
{
    switch (mavType) {
    case MAV_PARAM_TYPE_UINT8:
        return FactMetaData::valueTypeUint8;

    case MAV_PARAM_TYPE_INT8:
        return FactMetaData::valueTypeInt8;

    case MAV_PARAM_TYPE_UINT16:
        return FactMetaData::valueTypeUint16;

    case MAV_PARAM_TYPE_INT16:
        return FactMetaData::valueTypeInt16;

    case MAV_PARAM_TYPE_UINT32:
        return FactMetaData::valueTypeUint32;

    case MAV_PARAM_TYPE_UINT64:
        return FactMetaData::valueTypeUint64;

    case MAV_PARAM_TYPE_INT64:
        return FactMetaData::valueTypeInt64;

    case MAV_PARAM_TYPE_REAL32:
        return FactMetaData::valueTypeFloat;

    case MAV_PARAM_TYPE_REAL64:
        return FactMetaData::valueTypeDouble;

    default:
        qWarning() << "Unsupported mav param type" << mavType;
        // fall through

    case MAV_PARAM_TYPE_INT32:
        return FactMetaData::valueTypeInt32;
    }
}

void ParameterManager::_checkInitialLoadComplete(void)
{
    // Already processed?
    if (_initialLoadComplete) {
        return;
    }

    for (int componentId: _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            // We are still waiting on some parameters, not done yet
            return;
        }
    }

    if (!_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
        // No default component params yet, not done yet
        return;
    }

    // We aren't waiting for any more initial parameter updates, initial parameter loading is complete
    _initialLoadComplete = true;

    // Parameter cache crc failure debugging
    for (int componentId: _debugCacheParamSeen.keys()) {
        if (!_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
            for (const QString& paramName: _debugCacheParamSeen[componentId].keys()) {
                if (!_debugCacheParamSeen[componentId][paramName]) {
                    qDebug() << "Parameter in cache but not on vehicle componentId:Name" << componentId << paramName;
                }
            }
        }
    }
    _debugCacheCRC.clear();

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Initial load complete";

    // Check for index based load failures
    QString indexList;
    bool initialLoadFailures = false;
    for (int componentId: _failedReadParamIndexMap.keys()) {
        for (int paramIndex: _failedReadParamIndexMap[componentId]) {
            if (initialLoadFailures) {
                indexList += ", ";
            }
            indexList += QString("%1:%2").arg(componentId).arg(paramIndex);
            initialLoadFailures = true;
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Gave up on initial load after max retries (paramIndex:" << paramIndex << ")";
        }
    }

    _missingParameters = false;
    if (initialLoadFailures) {
        _missingParameters = true;
        QString errorMsg = tr("%1 was unable to retrieve the full set of parameters from vehicle %2. "
                              "This will cause %1 to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.").arg(qgcApp()->applicationName()).arg(_vehicle->id());
        qCDebug(ParameterManagerLog) << errorMsg;
        qgcApp()->showAppMessage(errorMsg);
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix(-1) << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;
        }
    }

    // Signal load complete
    _parametersReady = true;
    _vehicle->autopilotPlugin()->parametersReadyPreChecks();
    emit parametersReadyChanged(true);
    emit missingParametersChanged(_missingParameters);
}

void ParameterManager::_initialRequestTimeout(void)
{
    if (!_disableAllRetries && ++_initialRequestRetryCount <= _maxInitialRequestListRetry) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Retrying initial parameter request list";
        refreshAllParameters();
        _initialRequestTimeoutTimer.start();
    } else {
        if (!_vehicle->genericFirmware()) {
            QString errorMsg = tr("Vehicle %1 did not respond to request for parameters. "
                                  "This will cause %2 to be unable to display its full user interface.").arg(_vehicle->id()).arg(qgcApp()->applicationName());
            qCDebug(ParameterManagerLog) << errorMsg;
            qgcApp()->showAppMessage(errorMsg);
        }
    }
}

/// Remap a parameter from one firmware version to another
QString ParameterManager::_remapParamNameToVersion(const QString& paramName)
{
    QString mappedParamName;

    if (!paramName.startsWith(QStringLiteral("r."))) {
        // No version mapping wanted
        return paramName;
    }

    int majorVersion = _vehicle->firmwareMajorVersion();
    int minorVersion = _vehicle->firmwareMinorVersion();

    qCDebug(ParameterManagerLog) << "_remapParamNameToVersion" << paramName << majorVersion << minorVersion;

    mappedParamName = paramName.right(paramName.count() - 2);

    if (majorVersion == Vehicle::versionNotSetValue) {
        // Vehicle version unknown
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMajorVersionMap_t& majorVersionRemap = _vehicle->firmwarePlugin()->paramNameRemapMajorVersionMap();

    if (!majorVersionRemap.contains(majorVersion)) {
        // No mapping for this major version
        qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: no major version mapping";
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMinorVersionRemapMap_t& remapMinorVersion = majorVersionRemap[majorVersion];

    // We must map backwards from the highest known minor version to one above the vehicle's minor version

    for (int currentMinorVersion=_vehicle->firmwarePlugin()->remapParamNameHigestMinorVersionNumber(majorVersion); currentMinorVersion>minorVersion; currentMinorVersion--) {
        if (remapMinorVersion.contains(currentMinorVersion)) {
            const FirmwarePlugin::remapParamNameMap_t& remap = remapMinorVersion[currentMinorVersion];

            if (remap.contains(mappedParamName)) {
                QString toParamName = remap[mappedParamName];
                qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: remapped currentMinor:from:to"<< currentMinorVersion << mappedParamName << toParamName;
                mappedParamName = toParamName;
            }
        }
    }

    return mappedParamName;
}

/// The offline editing vehicle can have custom loaded params bolted into it.
void ParameterManager::_loadOfflineEditingParams(void)
{
    QString paramFilename = _vehicle->firmwarePlugin()->offlineEditingParamFile(_vehicle);
    if (paramFilename.isEmpty()) {
        return;
    }

    QFile paramFile(paramFilename);
    if (!paramFile.open(QFile::ReadOnly)) {
        qCWarning(ParameterManagerLog) << "Unable to open offline editing params file" << paramFilename;
    }

    QTextStream paramStream(&paramFile);

    while (!paramStream.atEnd()) {
        QString line = paramStream.readLine();

        if (line.startsWith("#")) {
            continue;
        }

        QStringList paramData = line.split("\t");
        Q_ASSERT(paramData.count() == 5);

        int defaultComponentId = paramData.at(1).toInt();
        _vehicle->setOfflineEditingDefaultComponentId(defaultComponentId);
        QString paramName = paramData.at(2);
        QString valStr = paramData.at(3);
        MAV_PARAM_TYPE paramType = static_cast<MAV_PARAM_TYPE>(paramData.at(4).toUInt());

        QVariant paramValue;
        switch (paramType) {
        case MAV_PARAM_TYPE_REAL32:
            paramValue = QVariant(valStr.toFloat());
            break;
        case MAV_PARAM_TYPE_UINT32:
            paramValue = QVariant(valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT32:
            paramValue = QVariant(valStr.toInt());
            break;
        case MAV_PARAM_TYPE_UINT16:
            paramValue = QVariant((quint16)valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT16:
            paramValue = QVariant((qint16)valStr.toInt());
            break;
        case MAV_PARAM_TYPE_UINT8:
            paramValue = QVariant((quint8)valStr.toUInt());
            break;
        case MAV_PARAM_TYPE_INT8:
            paramValue = QVariant((qint8)valStr.toUInt());
            break;
        default:
            qCritical() << "Unknown type" << paramType;
            paramValue = QVariant(valStr.toInt());
            break;
        }

        Fact* fact = new Fact(defaultComponentId, paramName, mavTypeToFactType(paramType), this);

        FactMetaData* factMetaData = _vehicle->compInfoManager()->compInfoParam(defaultComponentId)->factMetaDataForName(paramName, fact->type());
        fact->setMetaData(factMetaData);

        _mapCompId2FactMap[defaultComponentId][paramName] = fact;
    }

    _parametersReady = true;
    _initialLoadComplete = true;
    _debugCacheCRC.clear();
}

void ParameterManager::resetAllParametersToDefaults()
{
    _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                             MAV_CMD_PREFLIGHT_STORAGE,
                             true,  // showError
                             2,     // Reset params to default
                             -1);   // Don't do anything with mission storage
}

void ParameterManager::resetAllToVehicleConfiguration()
{
    //-- https://github.com/PX4/Firmware/pull/11760
    Fact* sysAutoConfigFact = getParameter(-1, "SYS_AUTOCONFIG");
    if(sysAutoConfigFact) {
        sysAutoConfigFact->setRawValue(2);
    }
}

QString ParameterManager::_logVehiclePrefix(int componentId)
{
    if (componentId == -1) {
        return QString("V:%1").arg(_vehicle->id());
    } else {
        return QString("V:%1 C:%2").arg(_vehicle->id()).arg(componentId);
    }
}

void ParameterManager::_setLoadProgress(double loadProgress)
{
    if (_loadProgress != loadProgress) {
        _loadProgress = loadProgress;
        emit loadProgressChanged(static_cast<float>(loadProgress));
    }
}

QList<int> ParameterManager::componentIds(void)
{
    return _paramCountMap.keys();
}

bool ParameterManager::pendingWrites(void)
{
    for (int compId: _waitingWriteParamNameMap.keys()) {
        if (_waitingWriteParamNameMap[compId].count()) {
            return true;
        }
    }

    return false;
}


/* Parse the binary parameter file and inject the parameters in the qgc
 * fact system.
 *
 * See: https://github.com/ArduPilot/ardupilot/tree/master/libraries/AP_Filesystem
 *
 */
bool ParameterManager::_parseParamFile(const QString& filename)
{
    const quint16 magic_standard = 0x671B;
    const quint16 magic_withdefaults = 0x671C;
    quint32 no_of_parameters_found = 0;
    const int componentId = MAV_COMP_ID_AUTOPILOT1; /* Only main autopilot for the moment */
    enum ap_var_type {
        AP_PARAM_NONE    = 0,
        AP_PARAM_INT8,
        AP_PARAM_INT16,
        AP_PARAM_INT32,
        AP_PARAM_FLOAT,
        AP_PARAM_VECTOR3F,
        AP_PARAM_GROUP
    };

    qCDebug(ParameterManagerLog) << "_parseParamFile: " << filename;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: Could not open downloaded parameter file.";
        return false;
    }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 magic, num_params, total_params;
    in >> magic;
    in >> num_params;
    in >> total_params;

    if (in.status() != QDataStream::Ok) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: Could not read Header";
        goto Error;
    }

    qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: magic: 0x" << Qt::hex << magic;
    qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: num_params:" << num_params
                                 << " total_params:" << total_params;

    if ((magic != magic_standard) && (magic != magic_withdefaults)) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: File does not start with Magic";
        goto Error;
    }
    if (num_params > total_params) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: total_params > num_params";
        goto Error;
    }
    if (num_params != total_params) {
        /* We requested all parameters, so this is an error here */
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: total_params != num_params";
        goto Error;
    }

    while (in.status() == QDataStream::Ok) {
        quint8 byte = 0;
        quint8 flags = 0;
        quint8 ptype = 0;
        quint8 name_len = 0;
        quint8 common_len = 0;
        bool withdefault = false;
        int no_read = 0;
        char name_buffer[17];

        while (byte == 0x0) { // Eat padding bytes
            in >> byte;
            if (in.status() != QDataStream::Ok) {
                if (no_of_parameters_found == num_params)
                    goto Success;
                else {
                    qCDebug(ParameterManagerLog) << "_parseParamFile: Error: unexpected EOF"
                                                 << "number of parameters expected:" << num_params
                                                 << "actual:" << no_of_parameters_found;
                    goto Error;
                }
            }
        }
        ptype = byte & 0x0F;
        flags = (byte >> 4) & 0x0F;
        withdefault = (flags & 0x01) == 0x01;
        in >> byte;
        if (in.status() != QDataStream::Ok) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: Unexpected EOF while reading flags";
            goto Error;
        }
        name_len = ((byte >> 4) & 0x0F) + 1;
        common_len = byte & 0x0F;
        if ((name_len + common_len) > 16) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: common_len + name_len > 16 "
                                         << "name_len" << name_len
                                         << "common_len" << common_len;
            goto Error;
        }
        no_read = in.readRawData(&name_buffer[common_len], (int) name_len);
        if (no_read != name_len) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: Unexpected EOF while reading parameterName"
                                         << "Expected:" << name_len
                                         << "Actual:" << no_read;
            goto Error;
        }
        name_buffer[common_len + name_len] = '\0';
        QString parameterName(name_buffer);
        qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: parameter" << parameterName
                                     << "name_len" << name_len
                                     << "common_len" << common_len
                                     << "ptype" << ptype
                                     << "flags" << flags;

        QVariant parameterValue = 0;
        switch ((enum ap_var_type) ptype) {
        qint8 data8;
        qint16 data16;
        qint32 data32;
        float dfloat;
        case AP_PARAM_INT8:
            in >> data8;
            parameterValue = data8;
            if (withdefault)
                in >> data8;
            break;
        case AP_PARAM_INT16:
            in >> data16;
            parameterValue = data16;
            if (withdefault)
                in >> data16;
            break;
        case AP_PARAM_INT32:
            in >> data32;
            parameterValue = data32;
            if (withdefault)
                in >> data32;
            break;
        case AP_PARAM_FLOAT:
            in >> data32;
            memcpy(&dfloat, &data32, 4);
            parameterValue = dfloat;
            if (withdefault)
                in >> data32;
            break;
        default:
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: type is out of range" << ptype;
            goto Error;
            break;
        }
        qCDebug(ParameterManagerVerbose2Log) << "paramValue" << parameterValue;

        if (++no_of_parameters_found > num_params){
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: more parameters in file than expected."
                                         << "Expected:" << num_params
                                         << "Actual:" << no_of_parameters_found;
            goto Error;
        }

        FactMetaData::ValueType_t factType = (ptype == AP_PARAM_INT8  ? FactMetaData::valueTypeInt8  :
                                              ptype == AP_PARAM_INT16 ? FactMetaData::valueTypeInt16 :
                                              ptype == AP_PARAM_INT32 ? FactMetaData::valueTypeInt32 :
                                              FactMetaData::valueTypeFloat);

        Fact* fact = nullptr;
        if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
            fact = _mapCompId2FactMap[componentId][parameterName];
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

            fact = new Fact(componentId, parameterName, factType, this);
            FactMetaData* factMetaData = _vehicle->compInfoManager()->compInfoParam(componentId)->factMetaDataForName(parameterName, fact->type());
            fact->setMetaData(factMetaData);

            _mapCompId2FactMap[componentId][parameterName] = fact;

            // We need to know when the fact value changes so we can update the vehicle
            connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);

            emit factAdded(componentId, fact);
        }
        fact->_containerSetRawValue(parameterValue);
    }
Success:
    file.close();
    /* Create empty waiting lists as we have all parameters */
    _paramCountMap[componentId] = num_params;
    _totalParamCount += num_params;
    _waitingReadParamIndexMap[componentId] = QMap<int, int>();
    _waitingReadParamNameMap[componentId] = QMap<QString, int>();
    _waitingWriteParamNameMap[componentId] = QMap<QString, int>();
    _checkInitialLoadComplete();
    _setLoadProgress(0.0);
    return true;

Error:
    file.close();
    return false;
}
