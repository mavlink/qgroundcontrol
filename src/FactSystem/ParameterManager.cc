/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterManager.h"
#include "AutoPilotPlugin.h"
#include "CompInfoParam.h"
#include "ComponentInformationManager.h"
#include "FactGroup.h"
#include "FactPanelController.h"
#include "FactValueSliderListModel.h"
#include "FirmwarePlugin.h"
#include "FTPManager.h"
#include "MAVLinkProtocol.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QVariantAnimation>
#include <QtQml/qqml.h>

QGC_LOGGING_CATEGORY(ParameterManagerLog, "qgc.factsystem.parametermanager")
QGC_LOGGING_CATEGORY(ParameterManagerVerbose1Log, "qgc.factsystem.parametermanager1:verbose")
QGC_LOGGING_CATEGORY(ParameterManagerVerbose2Log, "qgc.factsystem.parametermanager2:verbose")
QGC_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog, "qgc.factsystem.parametermanager.debugcachefailure") // Turn on to debug parameter cache crc misses

ParameterManager::ParameterManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _logReplay(!vehicle->vehicleLinkManager()->primaryLink().expired() && vehicle->vehicleLinkManager()->primaryLink().lock()->isLogReplay())
    , _tryftp(vehicle->apmFirmware())
{
    // qCDebug(ParameterManagerLog) << Q_FUNC_INFO << this;

    if (_vehicle->isOfflineEditingVehicle()) {
        _loadOfflineEditingParams();
        return;
    }

    _initialRequestTimeoutTimer.setSingleShot(true);
    _initialRequestTimeoutTimer.setInterval(5000);
    (void) connect(&_initialRequestTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_initialRequestTimeout);

    _waitingParamTimeoutTimer.setSingleShot(true);
    _waitingParamTimeoutTimer.setInterval(3000);
    (void) connect(&_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_waitingParamTimeout);

    // Ensure the cache directory exists
    (void) QFileInfo(QSettings().fileName()).dir().mkdir("ParamCache");
}

ParameterManager::~ParameterManager()
{
    // qCDebug(ParameterManagerLog) << Q_FUNC_INFO << this;
}

void ParameterManager::registerQmlTypes()
{
    (void) qmlRegisterType<Fact>("QGroundControl.FactSystem", 1, 0, "Fact");
    (void) qmlRegisterType<FactMetaData>("QGroundControl.FactSystem", 1, 0, "FactMetaData");
    (void) qmlRegisterType<FactPanelController>("QGroundControl.FactSystem", 1, 0, "FactPanelController");

    (void) qmlRegisterUncreatableType<FactGroup>("QGroundControl.FactSystem", 1, 0, "FactGroup", "Reference only");
    (void) qmlRegisterUncreatableType<FactValueSliderListModel>("QGroundControl.FactControls", 1, 0, "FactValueSliderListModel", "Reference only");
    (void) qmlRegisterUncreatableType<ParameterManager>("QGroundControl.Vehicle", 1, 0, "ParameterManager", "Reference only");
}

void ParameterManager::_updateProgressBar()
{
    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamCount = 0;

    for (const int compId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[compId].count();
    }
    for (const int compId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[compId].count();
    }
    for (const int compId: _waitingWriteParamNameMap.keys()) {
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
        _setLoadProgress(static_cast<double>(_totalParamCount - waitingReadParamIndexCount) / static_cast<double>(_totalParamCount));
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
        _setLoadProgress(static_cast<double>(qMax(_waitingWriteParamBatchCount - waitingWriteParamCount, 1)) / static_cast<double>(_waitingWriteParamBatchCount + 1));
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
        _setLoadProgress(static_cast<double>(qMax(_waitingReadParamNameBatchCount - waitingReadParamNameCount, 1)) / static_cast<double>(_waitingReadParamNameBatchCount + 1));
        return;
    }
}


void ParameterManager::mavlinkMessageReceived(const mavlink_message_t &message)
{
    if (_tryftp && (message.compid == MAV_COMP_ID_AUTOPILOT1) && !_initialLoadComplete)
        return;

    if (message.msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        mavlink_param_value_t param_value{};
        mavlink_msg_param_value_decode(&message, &param_value);

        // This will null terminate the name string
        char parameterNameWithNull[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1] = {};
        (void) strncpy(parameterNameWithNull, param_value.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
        const QString parameterName(parameterNameWithNull);

        mavlink_param_union_t paramUnion{};
        paramUnion.param_float = param_value.param_value;
        paramUnion.type = param_value.param_type;

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
            qCCritical(ParameterManagerLog) << "ParameterManager::_handleParamValue - unsupported MAV_PARAM_TYPE" << paramUnion.type;
            break;
        }

        _handleParamValue(message.compid, parameterName, param_value.param_count, param_value.param_index, static_cast<MAV_PARAM_TYPE>(param_value.param_type), parameterValue);
    }
}

void ParameterManager::_handleParamValue(int componentId, const QString &parameterName, int parameterCount, int parameterIndex, MAV_PARAM_TYPE mavParamType, const QVariant &parameterValue)
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
    if ((parameterIndex == 65535) && (parameterName != QStringLiteral("_HASH_CHECK")) && _initialRequestTimeoutTimer.isActive()) {
        qCDebug(ParameterManagerVerbose1Log) << "Disregarding unrequested param prior to initial list response" << parameterName;
        return;
    }

    _initialRequestTimeoutTimer.stop();

    if (_vehicle->px4Firmware() && (parameterName == "_HASH_CHECK")) {
        if (!_initialLoadComplete && !_logReplay) {
            /* we received a cache hash, potentially load from cache */
            _tryCacheHashLoad(_vehicle->id(), componentId, parameterValue);
        }
        return;
    }

    // Used to debug cache crc misses (turn on ParameterManagerDebugCacheFailureLog)
    if (!_initialLoadComplete && !_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
        if (_debugCacheMap[componentId].contains(parameterName)) {
            const ParamTypeVal &cacheParamTypeVal = _debugCacheMap[componentId][parameterName];
            const size_t dataSize = FactMetaData::typeToSize(static_cast<FactMetaData::ValueType_t>(cacheParamTypeVal.first));
            const void *const cacheData = cacheParamTypeVal.second.constData();
            const void *const vehicleData = parameterValue.constData();

            if (memcmp(cacheData, vehicleData, dataSize) != 0) {
                qCDebug(ParameterManagerVerbose1Log) << "Cache/Vehicle values differ for name:cache:actual" << parameterName << parameterValue << cacheParamTypeVal.second;
            }
            _debugCacheParamSeen[componentId][parameterName] = true;
        } else {
            qCDebug(ParameterManagerVerbose1Log) << "Parameter missing from cache" << parameterName;
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
        for (int waitingIndex = 0; waitingIndex < parameterCount; waitingIndex++) {
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
        (void) _indexBatchQueue.removeOne(parameterIndex);
        _fillIndexBatchQueue(false /* waitingParamTimeout */);
    }

    (void) _waitingReadParamNameMap[componentId].remove(parameterName);
    (void) _waitingWriteParamNameMap[componentId].remove(parameterName);
    if (!_waitingReadParamIndexMap[componentId].isEmpty()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap:" << _waitingReadParamIndexMap[componentId];
    }
    if (!_waitingReadParamNameMap[componentId].isEmpty()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamNameMap" << _waitingReadParamNameMap[componentId];
    }
    if (!_waitingWriteParamNameMap[componentId].isEmpty()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingWriteParamNameMap" << _waitingWriteParamNameMap[componentId];
    }

    // Track how many parameters we are still waiting for
    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamNameCount = 0;

    for (const int waitingComponentId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[waitingComponentId].count();
    }
    if (waitingReadParamIndexCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamIndexCount:" << waitingReadParamIndexCount;
    }

    for (const int waitingComponentId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[waitingComponentId].count();
    }
    if (waitingReadParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamNameCount:" << waitingReadParamNameCount;
    }

    for (const int waitingComponentId: _waitingWriteParamNameMap.keys()) {
        waitingWriteParamNameCount += _waitingWriteParamNameMap[waitingComponentId].count();
    }
    if (waitingWriteParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingWriteParamNameCount:" << waitingWriteParamNameCount;
    }

    const int readWaitingParamCount = waitingReadParamIndexCount + waitingReadParamNameCount;
    const int totalWaitingParamCount = readWaitingParamCount + waitingWriteParamNameCount;
    if (totalWaitingParamCount) {
        // More params to wait for, restart timer
        _waitingParamTimeoutTimer.start();
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer: totalWaitingParamCount:" << totalWaitingParamCount;
    } else if (!_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
        // Still waiting for parameters from default component
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer (still waiting for default component params)";
        _waitingParamTimeoutTimer.start();
    } else {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Not restarting _waitingParamTimeoutTimer (all requests satisfied)";
    }

    _updateProgressBar();

    Fact *fact = nullptr;
    if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
        fact = _mapCompId2FactMap[componentId][parameterName];
    } else {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

        fact = new Fact(componentId, parameterName, mavTypeToFactType(mavParamType), this);
        FactMetaData *const factMetaData = _vehicle->compInfoManager()->compInfoParam(componentId)->factMetaDataForName(parameterName, fact->type());
        fact->setMetaData(factMetaData);

        _mapCompId2FactMap[componentId][parameterName] = fact;

        // We need to know when the fact value changes so we can update the vehicle
        (void) connect(fact, &Fact::containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);

        emit factAdded(componentId, fact);
    }

    fact->containerSetRawValue(parameterValue);

    // Update param cache. The param cache is only used on PX4 Firmware since ArduPilot and Solo have volatile params
    // which invalidate the cache. The Solo also streams param updates in flight for things like gimbal values
    // which in turn causes a perf problem with all the param cache updates.
    if (!_logReplay && _vehicle->px4Firmware()) {
        if (((_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount) != 0) && (readWaitingParamCount == 0)) {
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

void ParameterManager::_factRawValueUpdateWorker(int componentId, const QString &name, FactMetaData::ValueType_t valueType, const QVariant &rawValue)
{
    if (_waitingWriteParamNameMap.contains(componentId)) {
        if (_waitingWriteParamNameMap[componentId].contains(name)) {
            (void) _waitingWriteParamNameMap[componentId].remove(name);
        } else {
            _waitingWriteParamBatchCount++;
        }
        _waitingWriteParamNameMap[componentId][name] = 0; // Add new entry and set retry count
        _updateProgressBar();
        _waitingParamTimeoutTimer.start();
        _saveRequired = true;
    } else {
        qCWarning(ParameterManagerLog) << "Internal error ParameterManager::_factValueUpdateWorker: component id not found" << componentId;
    }

    _sendParamSetToVehicle(componentId, name, valueType, rawValue);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Update parameter (_waitingParamTimeoutTimer started) - compId:name:rawValue" << componentId << name << rawValue;
}

void ParameterManager::_factRawValueUpdated(const QVariant &rawValue)
{
    Fact *const fact = qobject_cast<Fact*>(sender());
    if (!fact) {
        qCWarning(ParameterManagerLog) << "Internal error";
        return;
    }

    _factRawValueUpdateWorker(fact->componentId(), fact->name(), fact->type(), rawValue);
}

void ParameterManager::_ftpDownloadComplete(const QString &fileName, const QString &errorMsg)
{
    bool continueWithDefaultParameterdownload = true;
    bool immediateRetry = false;

    (void) disconnect(_vehicle->ftpManager(), &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
    (void) disconnect(_vehicle->ftpManager(), &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);

    if (errorMsg.isEmpty()) {
        qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parameter file received:" << fileName;
        if (_parseParamFile(fileName)) {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parsed!";
            return;
        } else {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Error in parameter file";
            /* This should not happen... */
        }
    } else if (errorMsg.contains("File Not Found")) {
        qCDebug(ParameterManagerLog) << "ParameterManager-ftp: No Parameterfile on vehicle - Start Conventional Parameter Download";
        if (_initialRequestRetryCount == 0) {
            immediateRetry = true;
        }
    } else if ((_loadProgress > 0.0001) && (_loadProgress < 0.01)) { /* FTP supported but too slow */
        qCDebug(ParameterManagerLog) << "ParameterManager-ftp progress too slow - Start Conventional Parameter Download";
    } else if (_initialRequestRetryCount == 1) {
        qCDebug(ParameterManagerLog) << "ParameterManager-ftp: Too many retries - Start Conventional Parameter Download";
    } else {
        qCDebug(ParameterManagerLog) << "ParameterManager-ftp Retry:" << _initialRequestRetryCount;
        continueWithDefaultParameterdownload = false;
    }

    if (continueWithDefaultParameterdownload) {
        _tryftp = false;
        _initialRequestRetryCount = 0;
        /* If we receive "File not Found" this indicates that the vehicle does not support
         * the parameter download via ftp. If we received this without retry, then we
         * can immediately response with the conventional parameter download request, because
         * we have no indication of communication link congestion.*/
        if (immediateRetry) {
            _initialRequestTimeout();
        } else {
            _initialRequestTimeoutTimer.start();
        }
    } else {
        _initialRequestTimeoutTimer.start();
    }
}

void ParameterManager::_ftpDownloadProgress(float progress)
{
    qCDebug(ParameterManagerVerbose1Log) << "ParameterManager::_ftpDownloadProgress:" << progress;
    _setLoadProgress(static_cast<double>(progress));
    if (progress > 0.001) {
        _initialRequestTimeoutTimer.stop();
    }
}

void ParameterManager::refreshAllParameters(uint8_t componentId)
{
    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency() || _logReplay) {
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

    if (_tryftp && ((componentId == MAV_COMP_ID_ALL) || (componentId == MAV_COMP_ID_AUTOPILOT1))) {
        FTPManager *const ftpManager = _vehicle->ftpManager();
        (void) connect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        _waitingParamTimeoutTimer.stop();
        if (ftpManager->download(MAV_COMP_ID_AUTOPILOT1,
                                 QStringLiteral("@PARAM/param.pck"),
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation),
                                 QStringLiteral(""),
                                 false /* No filesize check */)) {
            (void) connect(ftpManager, &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);
        } else {
            qCWarning(ParameterManagerLog) << "ParameterManager::refreshallParameters FTPManager::download returned failure";
            (void) disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        }
    } else {
        // Reset index wait lists
        for (int cid: _paramCountMap.keys()) {
            // Add/Update all indices to the wait list, parameter index is 0-based
            if ((componentId != MAV_COMP_ID_ALL) && (componentId != cid)) {
                continue;
            }
            for (int waitingIndex = 0; waitingIndex < _paramCountMap[cid]; waitingIndex++) {
                // This will add a new waiting index if needed and set the retry count for that index to 0
                _waitingReadParamIndexMap[cid][waitingIndex] = 0;
            }
        }

        mavlink_message_t msg{};
        mavlink_msg_param_request_list_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                 MAVLinkProtocol::getComponentId(),
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,
                                                 _vehicle->id(),
                                                 componentId);
        (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    const QString what = (componentId == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Request to refresh all parameters for component ID:" << what;
}

int ParameterManager::_actualComponentId(int componentId) const
{
    if (componentId == defaultComponentId) {
        componentId = _vehicle->defaultComponentId();
        if (componentId == defaultComponentId) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix(-1) << "Default component id not set";
        }
    }

    return componentId;
}

void ParameterManager::refreshParameter(int componentId, const QString &paramName)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParameter - name:" << paramName << ")";

    if (_waitingReadParamNameMap.contains(componentId)) {
        const QString mappedParamName = _remapParamNameToVersion(paramName);

        if (_waitingReadParamNameMap[componentId].contains(mappedParamName)) {
            (void) _waitingReadParamNameMap[componentId].remove(mappedParamName);
        } else {
            _waitingReadParamNameBatchCount++;
        }
        _waitingReadParamNameMap[componentId][mappedParamName] = 0;     // Add new wait entry and update retry count
        _updateProgressBar();
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "restarting _waitingParamTimeout";
        _waitingParamTimeoutTimer.start();
    } else {
        qCWarning(ParameterManagerLog) << "Internal error";
    }

    _readParameterRaw(componentId, paramName, -1);
}

void ParameterManager::refreshParametersPrefix(int componentId, const QString &namePrefix)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParametersPrefix - name:" << namePrefix << ")";

    for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
        if (paramName.startsWith(namePrefix)) {
            refreshParameter(componentId, paramName);
        }
    }
}

bool ParameterManager::parameterExists(int componentId, const QString &paramName) const
{
    bool ret = false;

    componentId = _actualComponentId(componentId);
    if (_mapCompId2FactMap.contains(componentId)) {
        ret = _mapCompId2FactMap[componentId].contains(_remapParamNameToVersion(paramName));
    }

    return ret;
}

Fact *ParameterManager::getParameter(int componentId, const QString &paramName)
{
    componentId = _actualComponentId(componentId);

    const QString mappedParamName = _remapParamNameToVersion(paramName);
    if (!_mapCompId2FactMap.contains(componentId) || !_mapCompId2FactMap[componentId].contains(mappedParamName)) {
        qgcApp()->reportMissingParameter(componentId, mappedParamName);
        return &_defaultFact;
    }

    return _mapCompId2FactMap[componentId][mappedParamName];
}

QStringList ParameterManager::parameterNames(int componentId) const
{
    QStringList names;

    const int compId = _actualComponentId(componentId);
    const QMap<QString, Fact*> &factMap = _mapCompId2FactMap[compId];
    for (const QString &paramName: factMap.keys()) {
        names << paramName;
    }

    return names;
}

bool ParameterManager::_fillIndexBatchQueue(bool waitingParamTimeout)
{
    if (!_indexBatchQueueActive) {
        return false;
    }

    constexpr int maxBatchSize = 10;

    if (waitingParamTimeout) {
        // We timed out, clear the queue and try again
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to timeout";
        _indexBatchQueue.clear();
    } else {
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to received parameter";
    }

    for (const int componentId: _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap count" << _waitingReadParamIndexMap[componentId].count();
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap" << _waitingReadParamIndexMap[componentId];
        }

        for (const int paramIndex: _waitingReadParamIndexMap[componentId].keys()) {
            if (_indexBatchQueue.contains(paramIndex)) {
                // Don't add more than once
                continue;
            }

            if (_indexBatchQueue.count() > maxBatchSize) {
                break;
            }

            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_disableAllRetries || (_waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam)) {
                // Give up on this index
                _failedReadParamIndexMap[componentId] << paramIndex;
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Giving up on (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
                (void) _waitingReadParamIndexMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                _indexBatchQueue.append(paramIndex);
                _readParameterRaw(componentId, "", paramIndex);
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
            }
        }
    }

    return (!_indexBatchQueue.isEmpty());
}

void ParameterManager::_waitingParamTimeout()
{
    if (_logReplay) {
        return;
    }

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "_waitingParamTimeout";

    // Now that we have timed out for possibly the first time we can activate the index batch queue
    _indexBatchQueueActive = true;

    // First check for any missing parameters from the initial index based load
    bool paramsRequested = _fillIndexBatchQueue(true /* waitingParamTimeout */);
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

    constexpr int maxBatchSize = 10;
    int batchCount = 0;
    if (!paramsRequested) {
        for (const int componentId: _waitingWriteParamNameMap.keys()) {
            for (const QString &paramName: _waitingWriteParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingWriteParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    const Fact *const fact = getParameter(componentId, paramName);
                    _sendParamSetToVehicle(componentId, paramName, fact->type(), fact->rawValue());
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Write resend for (paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingWriteParamNameMap[componentId].remove(paramName);
                    const QString errorMsg = tr("Parameter write failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showAppMessage(errorMsg);
                }
            }
        }
    }

    if (!paramsRequested) {
        for (const int componentId: _waitingReadParamNameMap.keys()) {
            for (const QString &paramName: _waitingReadParamNameMap[componentId].keys()) {
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
                    (void) _waitingReadParamNameMap[componentId].remove(paramName);
                    const QString errorMsg = tr("Parameter read failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
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

void ParameterManager::_readParameterRaw(int componentId, const QString &paramName, int paramIndex) const
{
    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_message_t msg{};
    char fixedParamName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN];

    (void) strncpy(fixedParamName, paramName.toStdString().c_str(), sizeof(fixedParamName));
    (void) mavlink_msg_param_request_read_pack_chan(MAVLinkProtocol::instance()->getSystemId(),   // QGC system id
                                                    MAVLinkProtocol::getComponentId(),     // QGC component id
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,                           // Pack into this mavlink_message_t
                                                    _vehicle->id(),                 // Target system id
                                                    componentId,                    // Target component id
                                                    fixedParamName,                 // Named parameter being requested
                                                    paramIndex);                    // Parameter index being requested, -1 for named
    (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void ParameterManager::_sendParamSetToVehicle(int componentId, const QString &paramName, FactMetaData::ValueType_t valueType, const QVariant &value) const
{
    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_param_set_t p{};
    p.param_type = factTypeToMavType(valueType);

    mavlink_param_union_t union_value{};

    bool ok = false;
    switch (valueType) {
    case FactMetaData::valueTypeUint8:
        union_value.param_uint8 = static_cast<uint8_t>(value.toUInt(&ok));
        break;
    case FactMetaData::valueTypeInt8:
        union_value.param_int8 = static_cast<int8_t>(value.toInt(&ok));
        break;
    case FactMetaData::valueTypeUint16:
        union_value.param_uint16 = static_cast<uint16_t>(value.toUInt(&ok));
        break;
    case FactMetaData::valueTypeInt16:
        union_value.param_int16 = static_cast<int16_t>(value.toInt(&ok));
        break;
    case FactMetaData::valueTypeUint32:
        union_value.param_uint32 = static_cast<uint32_t>(value.toUInt(&ok));
        break;
    case FactMetaData::valueTypeFloat:
        union_value.param_float = value.toFloat(&ok);
        break;
    default:
        qCCritical(ParameterManagerLog) << "Unsupported fact value type" << valueType;
    case FactMetaData::valueTypeInt32:
        union_value.param_int32 = static_cast<int32_t>(value.toInt(&ok));
        break;
    }

    if (!ok) {
        qCCritical(ParameterManagerLog) << "Fact Failed to Convert to Param Type:" << value;
        return;
    }

    p.param_value = union_value.param_float;
    p.target_system = static_cast<uint8_t>(_vehicle->id());
    p.target_component = static_cast<uint8_t>(componentId);

    (void) strncpy(p.param_id, paramName.toStdString().c_str(), sizeof(p.param_id));

    mavlink_message_t msg{};
    (void) mavlink_msg_param_set_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                             MAVLinkProtocol::getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &msg,
                                             &p);
    (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void ParameterManager::_writeLocalParamCache(int vehicleId, int componentId)
{
    CacheMapName2ParamTypeVal cacheMap;

    for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
        const Fact *const fact = _mapCompId2FactMap[componentId][paramName];
        cacheMap[paramName] = ParamTypeVal(fact->type(), fact->rawValue());
    }

    QFile cacheFile(parameterCacheFile(vehicleId, componentId));
    if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QDataStream ds(&cacheFile);
        ds << cacheMap;
    } else {
        qCWarning(ParameterManagerLog) << "Failed to open cache file for writing" << cacheFile.fileName();
    }
}

QDir ParameterManager::parameterCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    return (spath + QDir::separator() + QStringLiteral("ParamCache"));
}

QString ParameterManager::parameterCacheFile(int vehicleId, int componentId)
{
    return parameterCacheDir().filePath(QStringLiteral("%1_%2.v2").arg(vehicleId).arg(componentId));
}

void ParameterManager::_tryCacheHashLoad(int vehicleId, int componentId, const QVariant &hashValue)
{
    qCInfo(ParameterManagerLog) << "Attemping load from cache";

    /* The datastructure of the cache table */
    CacheMapName2ParamTypeVal cacheMap;
    QFile cacheFile(parameterCacheFile(vehicleId, componentId));
    if (!cacheFile.exists()) {
        /* no local cache, just wait for them to come in*/
        return;
    }
    (void) cacheFile.open(QIODevice::ReadOnly);

    /* Deserialize the parameter cache table */
    QDataStream ds(&cacheFile);
    ds >> cacheMap;

    /* compute the crc of the local cache to check against the remote */
    uint32_t crc32_value = 0;
    for (const QString &name: cacheMap.keys()) {
        const ParamTypeVal &paramTypeVal = cacheMap[name];
        const FactMetaData::ValueType_t factType = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);

        if (_vehicle->compInfoManager()->compInfoParam(MAV_COMP_ID_AUTOPILOT1)->factMetaDataForName(name, factType)->volatileValue()) {
            // Does not take part in CRC
            qCDebug(ParameterManagerLog) << "Volatile parameter" << name;
        } else {
            const void *const vdat = paramTypeVal.second.constData();
            const FactMetaData::ValueType_t factType = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);
            crc32_value = QGC::crc32(reinterpret_cast<const uint8_t *>(qPrintable(name)), name.length(),  crc32_value);
            crc32_value = QGC::crc32(static_cast<const uint8_t *>(vdat), FactMetaData::typeToSize(factType), crc32_value);
        }
    }

    /* if the two param set hashes match, just load from the disk */
    if (crc32_value == hashValue.toUInt()) {
        qCInfo(ParameterManagerLog) << "Parameters loaded from cache" << qPrintable(QFileInfo(cacheFile).absoluteFilePath());

        const int count = cacheMap.count();
        int index = 0;
        for (const QString &name: cacheMap.keys()) {
            const ParamTypeVal &paramTypeVal = cacheMap[name];
            const FactMetaData::ValueType_t factType = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);
            const MAV_PARAM_TYPE mavParamType = factTypeToMavType(factType);
            _handleParamValue(componentId, name, count, index++, mavParamType, paramTypeVal.second);
        }

        const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            mavlink_param_set_t p{};
            mavlink_param_union_t union_value{};

            // Return the hash value to notify we don't want any more updates
            p.param_type = MAV_PARAM_TYPE_UINT32;
            (void) strncpy(p.param_id, "_HASH_CHECK", sizeof(p.param_id));
            union_value.param_uint32 = crc32_value;
            p.param_value = union_value.param_float;
            p.target_system = static_cast<uint8_t>(_vehicle->id());
            p.target_component = static_cast<uint8_t>(componentId);

            mavlink_message_t msg{};
            (void) mavlink_msg_param_set_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                     MAVLinkProtocol::getComponentId(),
                                                     sharedLink->mavlinkChannel(),
                                                     &msg,
                                                     &p);
            (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }

        // Give the user some feedback things loaded properly
        QVariantAnimation *const ani = new QVariantAnimation(this);
        ani->setEasingCurve(QEasingCurve::OutCubic);
        ani->setStartValue(0.0);
        ani->setEndValue(1.0);
        ani->setDuration(750);

        (void) connect(ani, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
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
            for (const QString &name: cacheMap.keys()) {
                _debugCacheParamSeen[componentId][name] = false;
            }
            qgcApp()->showAppMessage(tr("Parameter cache CRC match failed"));
        }
    }
}

QString ParameterManager::readParametersFromStream(QTextStream &stream)
{
    QString missingErrors;
    QString typeErrors;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (!line.startsWith("#")) {
            const QStringList wpParams = line.split("\t");
            const int lineMavId = wpParams.at(0).toInt();
            if (wpParams.size() == 5) {
                if (_vehicle->id() != lineMavId) {
                    return QStringLiteral("The parameters in the stream have been saved from System Id %1, but the current vehicle has the System Id %2.").arg(lineMavId).arg(_vehicle->id());
                }

                const int componentId = wpParams.at(1).toInt();
                const QString paramName = wpParams.at(2);
                const QString valStr = wpParams.at(3);
                const uint mavType = wpParams.at(4).toUInt();

                if (!parameterExists(componentId, paramName)) {
                    QString error;
                    error += QStringLiteral("%1:%2").arg(componentId).arg(paramName);
                    missingErrors += error;
                    qCDebug(ParameterManagerLog) << "Skipped due to missing:" << error;
                    continue;
                }

                Fact *const fact = getParameter(componentId, paramName);
                if (fact->type() != mavTypeToFactType(static_cast<MAV_PARAM_TYPE>(mavType))) {
                    QString error;
                    error = QStringLiteral("%1:%2 ").arg(componentId).arg(paramName);
                    typeErrors += error;
                    qCDebug(ParameterManagerLog) << "Skipped due to type mismatch: %1" << error;
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

void ParameterManager::writeParametersToStream(QTextStream &stream) const
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

    for (const int componentId: _mapCompId2FactMap.keys()) {
        for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
            const Fact *const fact = _mapCompId2FactMap[componentId][paramName];
            if (fact) {
                stream << _vehicle->id() << "\t" << componentId << "\t" << paramName << "\t" << fact->rawValueStringFullPrecision() << "\t" << QStringLiteral("%1").arg(factTypeToMavType(fact->type())) << "\n";
            } else {
                qCWarning(ParameterManagerLog) << "Internal error: missing fact";
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
        qCWarning(ParameterManagerLog) << "Unsupported fact type" << factType;
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
        qCWarning(ParameterManagerLog) << "Unsupported mav param type" << mavType;
    case MAV_PARAM_TYPE_INT32:
        return FactMetaData::valueTypeInt32;
    }
}

void ParameterManager::_checkInitialLoadComplete()
{
    if (_initialLoadComplete) {
        return;
    }

    for (const int componentId: _waitingReadParamIndexMap.keys()) {
        if (!_waitingReadParamIndexMap[componentId].isEmpty()) {
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
    for (const int componentId: _debugCacheParamSeen.keys()) {
        if (!_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
            for (const QString &paramName: _debugCacheParamSeen[componentId].keys()) {
                if (!_debugCacheParamSeen[componentId][paramName]) {
                    qCDebug(ParameterManagerLog) << "Parameter in cache but not on vehicle componentId:Name" << componentId << paramName;
                }
            }
        }
    }
    _debugCacheCRC.clear();

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Initial load complete";

    // Check for index based load failures
    QString indexList;
    bool initialLoadFailures = false;
    for (const int componentId: _failedReadParamIndexMap.keys()) {
        for (const int paramIndex: _failedReadParamIndexMap[componentId]) {
            if (initialLoadFailures) {
                indexList += ", ";
            }
            indexList += QStringLiteral("%1:%2").arg(componentId).arg(paramIndex);
            initialLoadFailures = true;
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Gave up on initial load after max retries (paramIndex:" << paramIndex << ")";
        }
    }

    _missingParameters = false;
    if (initialLoadFailures) {
        _missingParameters = true;
        const QString errorMsg = tr("%1 was unable to retrieve the full set of parameters from vehicle %2. "
                                    "This will cause %1 to be unable to display its full user interface. "
                                    "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                                    "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.").arg(QCoreApplication::applicationName()).arg(_vehicle->id());
        qCDebug(ParameterManagerLog) << errorMsg;
        qgcApp()->showAppMessage(errorMsg);
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix(-1) << "The following parameter indices could not be loaded after the maximum number of retries:" << indexList;
        }
    }

    // Signal load complete
    _parametersReady = true;
    _vehicle->autopilotPlugin()->parametersReadyPreChecks();
    emit parametersReadyChanged(true);
    emit missingParametersChanged(_missingParameters);
}

void ParameterManager::_initialRequestTimeout()
{
    if (!_disableAllRetries && (++_initialRequestRetryCount <= _maxInitialRequestListRetry)) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Retrying initial parameter request list";
        refreshAllParameters();
        _initialRequestTimeoutTimer.start();
    } else if (!_vehicle->genericFirmware()) {
        const QString errorMsg = tr("Vehicle %1 did not respond to request for parameters. "
                                    "This will cause %2 to be unable to display its full user interface.").arg(_vehicle->id()).arg(QCoreApplication::applicationName());
        qCDebug(ParameterManagerLog) << errorMsg;
        qgcApp()->showAppMessage(errorMsg);
    }
}

QString ParameterManager::_remapParamNameToVersion(const QString &paramName) const
{
    if (!paramName.startsWith(QStringLiteral("r."))) {
        // No version mapping wanted
        return paramName;
    }

    const int majorVersion = _vehicle->firmwareMajorVersion();
    const int minorVersion = _vehicle->firmwareMinorVersion();

    qCDebug(ParameterManagerLog) << "_remapParamNameToVersion" << paramName << majorVersion << minorVersion;

    QString mappedParamName = paramName.right(paramName.length() - 2);
    if (majorVersion == Vehicle::versionNotSetValue) {
        // Vehicle version unknown
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMajorVersionMap_t &majorVersionRemap = _vehicle->firmwarePlugin()->paramNameRemapMajorVersionMap();
    if (!majorVersionRemap.contains(majorVersion)) {
        // No mapping for this major version
        qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: no major version mapping";
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMinorVersionRemapMap_t &remapMinorVersion = majorVersionRemap[majorVersion];
    // We must map backwards from the highest known minor version to one above the vehicle's minor version
    for (int currentMinorVersion = _vehicle->firmwarePlugin()->remapParamNameHigestMinorVersionNumber(majorVersion); currentMinorVersion>minorVersion; currentMinorVersion--) {
        if (remapMinorVersion.contains(currentMinorVersion)) {
            const FirmwarePlugin::remapParamNameMap_t &remap = remapMinorVersion[currentMinorVersion];
            if (remap.contains(mappedParamName)) {
                const QString toParamName = remap[mappedParamName];
                qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: remapped currentMinor:from:to" << currentMinorVersion << mappedParamName << toParamName;
                mappedParamName = toParamName;
            }
        }
    }

    return mappedParamName;
}

void ParameterManager::_loadOfflineEditingParams()
{
    const QString paramFilename = _vehicle->firmwarePlugin()->offlineEditingParamFile(_vehicle);
    if (paramFilename.isEmpty()) {
        return;
    }

    QFile paramFile(paramFilename);
    if (!paramFile.open(QFile::ReadOnly)) {
        qCWarning(ParameterManagerLog) << "Unable to open offline editing params file" << paramFilename;
    }

    QTextStream paramStream(&paramFile);
    while (!paramStream.atEnd()) {
        const QString line = paramStream.readLine();
        if (line.startsWith("#")) {
            continue;
        }

        const QStringList paramData = line.split("\t");
        Q_ASSERT(paramData.count() == 5);

        const int defaultComponentId = paramData.at(1).toInt();
        _vehicle->setOfflineEditingDefaultComponentId(defaultComponentId);
        const QString paramName = paramData.at(2);
        const QString valStr = paramData.at(3);
        const MAV_PARAM_TYPE paramType = static_cast<MAV_PARAM_TYPE>(paramData.at(4).toUInt());

        QVariant paramValue;
        switch (paramType) {
        case MAV_PARAM_TYPE_REAL32:
            paramValue = QVariant(valStr.toFloat());
            break;
        case MAV_PARAM_TYPE_UINT32:
            paramValue = QVariant(valStr.toUInt());
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
            qCCritical(ParameterManagerLog) << "Unknown type" << paramType;
        case MAV_PARAM_TYPE_INT32:
            paramValue = QVariant(valStr.toInt());
            break;
        }

        Fact *const fact = new Fact(defaultComponentId, paramName, mavTypeToFactType(paramType), this);

        FactMetaData *const factMetaData = _vehicle->compInfoManager()->compInfoParam(defaultComponentId)->factMetaDataForName(paramName, fact->type());
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
    Fact *const sysAutoConfigFact = getParameter(defaultComponentId, "SYS_AUTOCONFIG");
    if (sysAutoConfigFact) {
        sysAutoConfigFact->setRawValue(2);
    }
}

QString ParameterManager::_logVehiclePrefix(int componentId) const
{
    if (componentId == -1) {
        return QStringLiteral("V:%1").arg(_vehicle->id());
    } else {
        return QStringLiteral("V:%1 C:%2").arg(_vehicle->id()).arg(componentId);
    }
}

void ParameterManager::_setLoadProgress(double loadProgress)
{
    if (_loadProgress != loadProgress) {
        _loadProgress = loadProgress;
        emit loadProgressChanged(static_cast<float>(loadProgress));
    }
}

QList<int> ParameterManager::componentIds() const
{
    return _paramCountMap.keys();
}

bool ParameterManager::pendingWrites() const
{
    for (const int compId: _waitingWriteParamNameMap.keys()) {
        if (!_waitingWriteParamNameMap[compId].isEmpty()) {
            return true;
        }
    }

    return false;
}

Vehicle *ParameterManager::vehicle()
{
    return _vehicle;
}


bool ParameterManager::_parseParamFile(const QString& filename)
{
    constexpr quint16 magic_standard = 0x671B;
    constexpr quint16 magic_withdefaults = 0x671C;
    quint32 no_of_parameters_found = 0;
    constexpr int componentId = MAV_COMP_ID_AUTOPILOT1;
    enum ap_var_type {
        AP_PARAM_NONE = 0,
        AP_PARAM_INT8,
        AP_PARAM_INT16,
        AP_PARAM_INT32,
        AP_PARAM_FLOAT,
        AP_PARAM_VECTOR3F,
        AP_PARAM_GROUP
    };

    qCDebug(ParameterManagerLog) << "_parseParamFile:" << filename;
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
                                         << "total_params:" << total_params;

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
                if (no_of_parameters_found == num_params) {
                    goto Success;
                } else {
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
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: common_len + name_len > 16"
                                           << "name_len" << name_len
                                           << "common_len" << common_len;
            goto Error;
        }
        no_read = in.readRawData(&name_buffer[common_len], static_cast<int>(name_len));
        if (no_read != name_len) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: Unexpected EOF while reading parameterName"
                                           << "Expected:" << name_len
                                           << "Actual:" << no_read;
            goto Error;
        }
        name_buffer[common_len + name_len] = '\0';
        const QString parameterName(name_buffer);
        qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: parameter" << parameterName
                                             << "name_len" << name_len
                                             << "common_len" << common_len
                                             << "ptype" << ptype
                                             << "flags" << flags;

        QVariant parameterValue = 0;
        switch (static_cast<ap_var_type>(ptype)) {
        qint8 data8;
        qint16 data16;
        qint32 data32;
        float dfloat;
        case AP_PARAM_INT8:
            in >> data8;
            parameterValue = data8;
            if (withdefault) {
                in >> data8;
            }
            break;
        case AP_PARAM_INT16:
            in >> data16;
            parameterValue = data16;
            if (withdefault) {
                in >> data16;
            }
            break;
        case AP_PARAM_INT32:
            in >> data32;
            parameterValue = data32;
            if (withdefault) {
                in >> data32;
            }
            break;
        case AP_PARAM_FLOAT:
            in >> data32;
            (void) memcpy(&dfloat, &data32, 4);
            parameterValue = dfloat;
            if (withdefault) {
                in >> data32;
            }
            break;
        default:
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: type is out of range" << ptype;
            goto Error;
            break;
        }
        qCDebug(ParameterManagerVerbose2Log) << "paramValue" << parameterValue;

        if (++no_of_parameters_found > num_params) {
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: more parameters in file than expected."
                                         << "Expected:" << num_params
                                         << "Actual:" << no_of_parameters_found;
            goto Error;
        }

        const FactMetaData::ValueType_t factType = ((ptype == AP_PARAM_INT8) ? FactMetaData::valueTypeInt8 :
                                                    (ptype == AP_PARAM_INT16) ? FactMetaData::valueTypeInt16 :
                                                    (ptype == AP_PARAM_INT32) ? FactMetaData::valueTypeInt32 :
                                                    FactMetaData::valueTypeFloat);

        Fact *fact = nullptr;
        if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
            fact = _mapCompId2FactMap[componentId][parameterName];
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

            fact = new Fact(componentId, parameterName, factType, this);
            FactMetaData *const factMetaData = _vehicle->compInfoManager()->compInfoParam(componentId)->factMetaDataForName(parameterName, fact->type());
            fact->setMetaData(factMetaData);

            _mapCompId2FactMap[componentId][parameterName] = fact;

            // We need to know when the fact value changes so we can update the vehicle
            (void) connect(fact, &Fact::containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);

            emit factAdded(componentId, fact);
        }
        fact->containerSetRawValue(parameterValue);
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
