/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "FirmwarePlugin.h"
#include "UAS.h"
#include "JsonHelper.h"

#include <QEasingCurve>
#include <QFile>
#include <QDebug>
#include <QVariantAnimation>
#include <QJsonArray>

/* types for local parameter cache */
typedef QPair<int, QVariant> ParamTypeVal;
typedef QPair<QString, ParamTypeVal> NamedParam;
typedef QMap<int, NamedParam> MapID2NamedParam;

QGC_LOGGING_CATEGORY(ParameterManagerVerbose1Log, "ParameterManagerVerbose1Log")
QGC_LOGGING_CATEGORY(ParameterManagerVerbose2Log, "ParameterManagerVerbose2Log")

Fact ParameterManager::_defaultFact;

const char* ParameterManager::_cachedMetaDataFilePrefix =   "ParameterFactMetaData";
const char* ParameterManager::_jsonParametersKey =          "parameters";
const char* ParameterManager::_jsonCompIdKey =              "compId";
const char* ParameterManager::_jsonParamNameKey =           "name";
const char* ParameterManager::_jsonParamValueKey =          "value";

ParameterManager::ParameterManager(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _mavlink(NULL)
    , _loadProgress(0.0)
    , _parametersReady(false)
    , _missingParameters(false)
    , _initialLoadComplete(false)
    , _waitingForDefaultComponent(false)
    , _saveRequired(false)
    , _defaultComponentId(MAV_COMP_ID_ALL)
    , _parameterSetMajorVersion(-1)
    , _parameterMetaData(NULL)
    , _prevWaitingReadParamIndexCount(0)
    , _prevWaitingReadParamNameCount(0)
    , _prevWaitingWriteParamNameCount(0)
    , _initialRequestRetryCount(0)
    , _disableAllRetries(false)
    , _totalParamCount(0)
{
    _versionParam = vehicle->firmwarePlugin()->getVersionParam();

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

    connect(_vehicle->uas(), &UASInterface::parameterUpdate, this, &ParameterManager::_parameterUpdate);

    _defaultComponentIdParam = vehicle->firmwarePlugin()->getDefaultComponentIdParam();
    qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Default component param" << _defaultComponentIdParam;

    // Ensure the cache directory exists
    QFileInfo(QSettings().fileName()).dir().mkdir("ParamCache");
    refreshAllParameters();
}

ParameterManager::~ParameterManager()
{
    delete _parameterMetaData;
}

/// Called whenever a parameter is updated or first seen.
void ParameterManager::_parameterUpdate(int vehicleId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value)
{
    // Is this for our uas?
    if (vehicleId != _vehicle->id()) {
        return;
    }

    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) <<
                                            "_parameterUpdate" <<
                                            "name:" << parameterName <<
                                            "count:" << parameterCount <<
                                            "index:" << parameterId <<
                                            "mavType:" << mavType <<
                                            "value:" << value <<
                                            ")";

    _initialRequestTimeoutTimer.stop();

#if 0
    // Handy for testing retry logic
    static int counter = 0;
    if (counter++ & 0x8) {
        qCDebug(ParameterManagerLog) << "Artificial discard" << counter;
        return;
    }
#endif

#if 0
    // Use this to test missing default component id
    if (componentId == 50) {
        return;
    }
#endif

    if (_vehicle->px4Firmware() && parameterName == "_HASH_CHECK") {
        /* we received a cache hash, potentially load from cache */
        _tryCacheHashLoad(vehicleId, componentId, value);
        return;
    }

    _initialRequestTimeoutTimer.stop();
    _waitingParamTimeoutTimer.stop();

    _dataMutex.lock();

    // Update our total parameter counts
    if (!_paramCountMap.contains(componentId)) {
        _paramCountMap[componentId] = parameterCount;
        _totalParamCount += parameterCount;
    }

    _mapParameterId2Name[componentId][parameterId] = parameterName;

    // If we've never seen this component id before, setup the wait lists.
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

    // Determine default component id
    if (!_defaultComponentIdParam.isEmpty() && _defaultComponentIdParam == parameterName) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Default component id determined";
        _defaultComponentId = componentId;
    }

    bool componentParamsComplete = false;
    if (_waitingReadParamIndexMap[componentId].count() == 1) {
        // We need to know when we get the last param from a component in order to complete setup
        componentParamsComplete = true;
    }

    if (!_waitingReadParamIndexMap[componentId].contains(parameterId) &&
        !_waitingReadParamNameMap[componentId].contains(parameterName) &&
        !_waitingWriteParamNameMap[componentId].contains(parameterName)) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix() << "Unrequested param update" << parameterName;
    }

    // Remove this parameter from the waiting lists
    _waitingReadParamIndexMap[componentId].remove(parameterId);
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

    foreach(int waitingComponentId, _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[waitingComponentId].count();
    }
    if (waitingReadParamIndexCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamIndexCount:" << waitingReadParamIndexCount;
    }

    foreach(int waitingComponentId, _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[waitingComponentId].count();
    }
    if (waitingReadParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamNameCount:" << waitingReadParamNameCount;
    }

    foreach(int waitingComponentId, _waitingWriteParamNameMap.keys()) {
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
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix() << "Restarting _waitingParamTimeoutTimer: totalWaitingParamCount:" << totalWaitingParamCount;
    } else {
        if (_defaultComponentId == MAV_COMP_ID_ALL && !_defaultComponentIdParam.isEmpty()) {
            // Still waiting for default component id, restart timer
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix() << "Restarting _waitingParamTimeoutTimer (still waiting for default component)";
            _waitingParamTimeoutTimer.start();
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix() << "Not restarting _waitingParamTimeoutTimer (all requests satisfied)";
        }
    }

    // Update progress bar for waiting reads
    if (readWaitingParamCount == 0) {
        // We are no longer waiting for any reads to complete
        if (_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount != 0) {
            // Set progress to 0 if not already there
            _setLoadProgress(0.0);
        }
    } else {
        _setLoadProgress((double)(_totalParamCount - readWaitingParamCount) / (double)_totalParamCount);
    }

    // Get parameter set version
    if (!_versionParam.isEmpty() && _versionParam == parameterName) {
        _parameterSetMajorVersion = value.toInt();
    }

    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(parameterName)) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

        FactMetaData::ValueType_t factType;
        switch (mavType) {
            case MAV_PARAM_TYPE_UINT8:
                factType = FactMetaData::valueTypeUint8;
                break;
            case MAV_PARAM_TYPE_INT8:
                factType = FactMetaData::valueTypeInt8;
                break;
            case MAV_PARAM_TYPE_UINT16:
                factType = FactMetaData::valueTypeUint16;
                break;
            case MAV_PARAM_TYPE_INT16:
                factType = FactMetaData::valueTypeInt16;
                break;
            case MAV_PARAM_TYPE_UINT32:
                factType = FactMetaData::valueTypeUint32;
                break;
            case MAV_PARAM_TYPE_INT32:
                factType = FactMetaData::valueTypeInt32;
                break;
            case MAV_PARAM_TYPE_REAL32:
                factType = FactMetaData::valueTypeFloat;
                break;
            case MAV_PARAM_TYPE_REAL64:
                factType = FactMetaData::valueTypeDouble;
                break;
            default:
                factType = FactMetaData::valueTypeInt32;
                qCritical() << "Unsupported fact type" << mavType;
                break;
        }

        Fact* fact = new Fact(componentId, parameterName, factType, this);

        _mapParameterName2Variant[componentId][parameterName] = QVariant::fromValue(fact);

        // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_valueUpdated);
    }

    _dataMutex.unlock();

    Q_ASSERT(_mapParameterName2Variant[componentId].contains(parameterName));

    Fact* fact = _mapParameterName2Variant[componentId][parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetRawValue(value);

    if (componentParamsComplete) {
        if (componentId == _defaultComponentId) {
            // Add meta data to default component. We need to do this before we setup the group map since group
            // map requires meta data.
            _addMetaDataToDefaultComponent();
        }

        // When we are getting the very last component param index, reset the group maps to update for the
        // new params. By handling this here, we can pick up components which finish up later than the default
        // component param set.
        _setupGroupMap();
    }

    if (_prevWaitingWriteParamNameCount != 0 &&  waitingWriteParamNameCount == 0) {
        // If all the writes just finished the vehicle is up to date, so persist.
        _saveToEEPROM();
    }

    // Update param cache. The param cache is only used on PX4 Firmware since ArduPilot and Solo have volatile params
    // which invalidate the cache. The Solo also streams param updates in flight for things like gimbal values
    // which in turn causes a perf problem with all the param cache updates.
    if (_vehicle->px4Firmware()) {
        if (_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount != 0 && readWaitingParamCount == 0) {
            // All reads just finished, update the cache
            _writeLocalParamCache(vehicleId, componentId);
        }
    }

    _prevWaitingReadParamIndexCount = waitingReadParamIndexCount;
    _prevWaitingReadParamNameCount = waitingReadParamNameCount;
    _prevWaitingWriteParamNameCount = waitingWriteParamNameCount;

    // Don't fail initial load complete if default component isn't found yet. That will be handled in wait timeout check.
    _checkInitialLoadComplete(false /* failIfNoDefaultComponent */);

    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_parameterUpdate complete";
}

/// Connected to Fact::valueUpdated
///
/// Writes the parameter to mavlink, sets up for write wait
void ParameterManager::_valueUpdated(const QVariant& value)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    Q_ASSERT(fact);

    int componentId = fact->componentId();
    QString name = fact->name();

    _dataMutex.lock();

    Q_ASSERT(_waitingWriteParamNameMap.contains(componentId));
    _waitingWriteParamNameMap[componentId].remove(name);    // Remove any old entry
    _waitingWriteParamNameMap[componentId][name] = 0;       // Add new entry and set retry count
    _waitingParamTimeoutTimer.start();
    _saveRequired = true;

    _dataMutex.unlock();

    _writeParameterRaw(componentId, fact->name(), value);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Set parameter - name:" << name << value << "(_waitingParamTimeoutTimer started)";

    if (fact->rebootRequired() && !qgcApp()->runningUnitTests()) {
        qgcApp()->showMessage(QStringLiteral("Change of parameter %1 requires a Vehicle reboot to take effect").arg(name));
    }
}

void ParameterManager::refreshAllParameters(uint8_t componentId)
{
    _dataMutex.lock();

    if (!_initialLoadComplete) {
        _initialRequestTimeoutTimer.start();
    }

    // Reset index wait lists
    foreach (int cid, _paramCountMap.keys()) {
        // Add/Update all indices to the wait list, parameter index is 0-based
        if(componentId != MAV_COMP_ID_ALL && componentId != cid)
            continue;
        for (int waitingIndex = 0; waitingIndex < _paramCountMap[cid]; waitingIndex++) {
            // This will add a new waiting index if needed and set the retry count for that index to 0
            _waitingReadParamIndexMap[cid][waitingIndex] = 0;
        }
    }

    _dataMutex.unlock();

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    Q_ASSERT(mavlink);

    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack_chan(mavlink->getSystemId(),
                                             mavlink->getComponentId(),
                                             _vehicle->priorityLink()->mavlinkChannel(),
                                             &msg,
                                             _vehicle->id(),
                                             componentId);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);

    QString what = (componentId == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Request to refresh all parameters for component ID:" << what;
}

void ParameterManager::_determineDefaultComponentId(void)
{
    if (_defaultComponentId == MAV_COMP_ID_ALL) {
        // We don't have a default component id yet. That means the plugin can't provide
        // the param to trigger off of. Instead we use the most prominent component id in
        // the set of parameters. Better than nothing!

        int largestCompParamCount = 0;
        foreach(int componentId, _mapParameterName2Variant.keys()) {
            int compParamCount = _mapParameterName2Variant[componentId].count();
            if (compParamCount > largestCompParamCount) {
                largestCompParamCount = compParamCount;
                _defaultComponentId = componentId;
            }
        }

        if (_defaultComponentId == MAV_COMP_ID_ALL) {
            qWarning() << _logVehiclePrefix() << "All parameters missing, unable to determine default component id";
        }
    }
}

/// Translates FactSystem::defaultComponentId to real component id if needed
int ParameterManager::_actualComponentId(int componentId)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _defaultComponentId;
        if (componentId == FactSystem::defaultComponentId) {
            qWarning() << _logVehiclePrefix() << "Default component id not set";
        }
    }

    return componentId;
}

void ParameterManager::refreshParameter(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParameter - name:" << name << ")";

    _dataMutex.lock();

    Q_ASSERT(_waitingReadParamNameMap.contains(componentId));

    if (_waitingReadParamNameMap.contains(componentId)) {
        QString mappedParamName = _remapParamNameToVersion(name);

        _waitingReadParamNameMap[componentId].remove(mappedParamName);  // Remove old wait entry if there
        _waitingReadParamNameMap[componentId][mappedParamName] = 0;     // Add new wait entry and update retry count
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "restarting _waitingParamTimeout";
        _waitingParamTimeoutTimer.start();
    }

    _dataMutex.unlock();

    _readParameterRaw(componentId, name, -1);
}

void ParameterManager::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParametersPrefix - name:" << namePrefix << ")";

    foreach(const QString &name, _mapParameterName2Variant[componentId].keys()) {
        if (name.startsWith(namePrefix)) {
            refreshParameter(componentId, name);
        }
    }
}

bool ParameterManager::parameterExists(int componentId, const QString&  name)
{
    bool ret = false;

    componentId = _actualComponentId(componentId);
    if (_mapParameterName2Variant.contains(componentId)) {
        ret = _mapParameterName2Variant[componentId].contains(_remapParamNameToVersion(name));
    }

    return ret;
}

Fact* ParameterManager::getParameter(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);

    QString mappedParamName = _remapParamNameToVersion(name);
    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(mappedParamName)) {
        qgcApp()->reportMissingParameter(componentId, mappedParamName);
        return &_defaultFact;
    }

    return _mapParameterName2Variant[componentId][mappedParamName].value<Fact*>();
}

QStringList ParameterManager::parameterNames(int componentId)
{
    QStringList names;

    foreach(const QString &paramName, _mapParameterName2Variant[_actualComponentId(componentId)].keys()) {
        names << paramName;
    }

    return names;
}

void ParameterManager::_setupGroupMap(void)
{
    // Must be able to handle being called multiple times
    _mapGroup2ParameterName.clear();

    foreach (int componentId, _mapParameterName2Variant.keys()) {
        foreach (const QString &name, _mapParameterName2Variant[componentId].keys()) {
            Fact* fact = _mapParameterName2Variant[componentId][name].value<Fact*>();
            _mapGroup2ParameterName[componentId][fact->group()] += name;
        }
    }
}

const QMap<int, QMap<QString, QStringList> >& ParameterManager::getGroupMap(void)
{
    return _mapGroup2ParameterName;
}

void ParameterManager::_waitingParamTimeout(void)
{
    bool paramsRequested = false;
    const int maxBatchSize = 10;
    int batchCount = 0;

    qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "_waitingParamTimeout";

    // First check for any missing parameters from the initial index based load

    batchCount = 0;
    foreach(int componentId, _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "_waitingReadParamIndexMap" << _waitingReadParamIndexMap[componentId];
        }

        foreach(int paramIndex, _waitingReadParamIndexMap[componentId].keys()) {            
            if (++batchCount > maxBatchSize) {
                goto Out;
            }

            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_disableAllRetries || _waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam) {
                // Give up on this index
                _failedReadParamIndexMap[componentId] << paramIndex;
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Giving up on (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
                _waitingReadParamIndexMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                paramsRequested = true;
                _readParameterRaw(componentId, "", paramIndex);
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
            }
        }
    }

    if (!paramsRequested && _defaultComponentId == MAV_COMP_ID_ALL && !_defaultComponentIdParam.isEmpty() && !_waitingForDefaultComponent) {
        // Initial load is complete but we still don't have default component params. Wait one more cycle to see if the
        // default component finally shows up.
        qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Restarting _waitingParamTimeoutTimer - still don't have default component id";
        _waitingParamTimeoutTimer.start();
        _waitingForDefaultComponent = true;
        return;
    }
    _waitingForDefaultComponent = false;

    // Check for initial load complete success/failure. Fail load if we don't have a default component at this point.
    _checkInitialLoadComplete(true /* failIfNoDefaultComponent */);

    if (!paramsRequested) {
        foreach(int componentId, _waitingWriteParamNameMap.keys()) {
            foreach(const QString &paramName, _waitingWriteParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingWriteParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    _writeParameterRaw(componentId, paramName, getParameter(componentId, paramName)->rawValue());
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Write resend for (paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingWriteParamNameMap[componentId].remove(paramName);
                    QString errorMsg = tr("Parameter write failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showMessage(errorMsg);
                }
            }
        }
    }

    if (!paramsRequested) {
        foreach(int componentId, _waitingReadParamNameMap.keys()) {
            foreach(const QString &paramName, _waitingReadParamNameMap[componentId].keys()) {
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
                    qgcApp()->showMessage(errorMsg);
                }
            }
        }
    }

Out:
    if (paramsRequested) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Restarting _waitingParamTimeoutTimer - re-request";
        _waitingParamTimeoutTimer.start();
    }
}

void ParameterManager::_readParameterRaw(int componentId, const QString& paramName, int paramIndex)
{
    mavlink_message_t msg;
    char fixedParamName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN];

    strncpy(fixedParamName, paramName.toStdString().c_str(), sizeof(fixedParamName));
    mavlink_msg_param_request_read_pack_chan(_mavlink->getSystemId(),   // QGC system id
                                             _mavlink->getComponentId(),     // QGC component id
                                             _vehicle->priorityLink()->mavlinkChannel(),
                                             &msg,                           // Pack into this mavlink_message_t
                                             _vehicle->id(),                 // Target system id
                                             componentId,                    // Target component id
                                             fixedParamName,                 // Named parameter being requested
                                             paramIndex);                    // Parameter index being requested, -1 for named
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void ParameterManager::_writeParameterRaw(int componentId, const QString& paramName, const QVariant& value)
{
    mavlink_param_set_t     p;
    mavlink_param_union_t   union_value;

    FactMetaData::ValueType_t factType = getParameter(componentId, paramName)->type();
    p.param_type = _factTypeToMavType(factType);

    switch (factType) {
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
            qCritical() << "Unsupported fact type" << factType;
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
                                      _vehicle->priorityLink()->mavlinkChannel(),
                                      &msg,
                                      &p);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void ParameterManager::_writeLocalParamCache(int vehicleId, int componentId)
{
    MapID2NamedParam cache_map;

    foreach(int id, _mapParameterId2Name[componentId].keys()) {
        const QString name(_mapParameterId2Name[componentId][id]);
        const Fact *fact = _mapParameterName2Variant[componentId][name].value<Fact*>();
        cache_map[id] = NamedParam(name, ParamTypeVal(fact->type(), fact->rawValue()));
    }

    QFile cache_file(parameterCacheFile(vehicleId, componentId));
    cache_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream ds(&cache_file);
    ds << cache_map;
}

QDir ParameterManager::parameterCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    return spath + QDir::separator() + "ParamCache";
}

QString ParameterManager::parameterCacheFile(int vehicleId, int componentId)
{
    return parameterCacheDir().filePath(QString("%1_%2").arg(vehicleId).arg(componentId));
}

void ParameterManager::_tryCacheHashLoad(int vehicleId, int componentId, QVariant hash_value)
{
    uint32_t crc32_value = 0;
    /* The datastructure of the cache table */
    MapID2NamedParam cache_map;
    QFile cache_file(parameterCacheFile(vehicleId, componentId));
    if (!cache_file.exists()) {
        /* no local cache, just wait for them to come in*/
        return;
    }
    cache_file.open(QIODevice::ReadOnly);

    /* Deserialize the parameter cache table */
    QDataStream ds(&cache_file);
    ds >> cache_map;

    /* compute the crc of the local cache to check against the remote */

    foreach(int id, cache_map.keys()) {
        const QString name(cache_map[id].first);
        const void *vdat = cache_map[id].second.second.constData();
        const FactMetaData::ValueType_t fact_type = static_cast<FactMetaData::ValueType_t>(cache_map[id].second.first);
        crc32_value = QGC::crc32((const uint8_t *)qPrintable(name), name.length(),  crc32_value);
        crc32_value = QGC::crc32((const uint8_t *)vdat, FactMetaData::typeToSize(fact_type), crc32_value);
    }

    if (crc32_value == hash_value.toUInt()) {
        qCInfo(ParameterManagerLog) << "Parameters loaded from cache" << qPrintable(QFileInfo(cache_file).absoluteFilePath());
        /* if the two param set hashes match, just load from the disk */
        int count = cache_map.count();
        foreach(int id, cache_map.keys()) {
            const QString &name = cache_map[id].first;
            const QVariant &value = cache_map[id].second.second;
            const FactMetaData::ValueType_t fact_type = static_cast<FactMetaData::ValueType_t>(cache_map[id].second.first);
            const int mavType = _factTypeToMavType(fact_type);
            _parameterUpdate(vehicleId, componentId, name, count, id, mavType, value);
        }
        // Return the hash value to notify we don't want any more updates
        mavlink_param_set_t     p;
        mavlink_param_union_t   union_value;
        p.param_type = MAV_PARAM_TYPE_UINT32;
        strncpy(p.param_id, "_HASH_CHECK", sizeof(p.param_id));
        union_value.param_uint32 = crc32_value;
        p.param_value = union_value.param_float;
        p.target_system = (uint8_t)_vehicle->id();
        p.target_component = (uint8_t)componentId;
        mavlink_message_t msg;
        mavlink_msg_param_set_encode_chan(_mavlink->getSystemId(),
                                          _mavlink->getComponentId(),
                                          _vehicle->priorityLink()->mavlinkChannel(),
                                          &msg,
                                          &p);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);

        // Give the user some feedback things loaded properly
        QVariantAnimation *ani = new QVariantAnimation(this);
        ani->setEasingCurve(QEasingCurve::OutCubic);
        ani->setStartValue(0.0);
        ani->setEndValue(1.0);
        ani->setDuration(750);

        connect(ani, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            _setLoadProgress(value.toDouble());
        });

        // Hide 500ms after animation finishes
        connect(ani, &QVariantAnimation::finished, [this](){
            QTimer::singleShot(500, [this]() {
                _setLoadProgress(0);
            });
        });

        ani->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void ParameterManager::_saveToEEPROM(void)
{
    if (_saveRequired) {
        _saveRequired = false;
        if (_vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::MavCmdPreflightStorageCapability)) {
            _vehicle->sendMavCommand(MAV_COMP_ID_ALL,
                                     MAV_CMD_PREFLIGHT_STORAGE,
                                     true,  // showError
                                     1,     // Write parameters to EEPROM
                                     -1);   // Don't do anything with mission storage
            qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "_saveToEEPROM";
        } else {
            qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "_saveToEEPROM skipped due to FirmwarePlugin::isCapable";
        }
    }
}

QString ParameterManager::readParametersFromStream(QTextStream& stream)
{
    QString errors;

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
                    error = QString("Skipped parameter %1:%2 - does not exist on this vehicle\n").arg(componentId).arg(paramName);
                    errors += error;
                    qCDebug(ParameterManagerLog) << error;
                    continue;
                }

                Fact* fact = getParameter(componentId, paramName);
                if (fact->type() != _mavTypeToFactType((MAV_PARAM_TYPE)mavType)) {
                    QString error;
                    error  = QString("Skipped parameter %1:%2 - type mismatch %3:%4\n").arg(componentId).arg(paramName).arg(fact->type()).arg(_mavTypeToFactType((MAV_PARAM_TYPE)mavType));
                    errors += error;
                    qCDebug(ParameterManagerLog) << error;
                    continue;
                }

                qCDebug(ParameterManagerLog) << "Updating parameter" << componentId << paramName << valStr;
                fact->setRawValue(valStr);
            }
        }
    }

    return errors;
}

void ParameterManager::writeParametersToStream(QTextStream &stream)
{
    stream << "# Onboard parameters for Vehicle " << _vehicle->id() << "\n";
    stream << "#\n";
    stream << "# Vehicle-Id Component-Id Name Value Type\n";

    foreach (int componentId, _mapParameterName2Variant.keys()) {
        foreach (const QString &paramName, _mapParameterName2Variant[componentId].keys()) {
            Fact* fact = _mapParameterName2Variant[componentId][paramName].value<Fact*>();
            if (fact) {
                stream << _vehicle->id() << "\t" << componentId << "\t" << paramName << "\t" << fact->rawValueStringFullPrecision() << "\t" << QString("%1").arg(_factTypeToMavType(fact->type())) << "\n";
            } else {
                qWarning() << "Internal error: missing fact";
            }
        }
    }

    stream.flush();
}

MAV_PARAM_TYPE ParameterManager::_factTypeToMavType(FactMetaData::ValueType_t factType)
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

        case FactMetaData::valueTypeFloat:
            return MAV_PARAM_TYPE_REAL32;

        default:
            qWarning() << "Unsupported fact type" << factType;
            // fall through

        case FactMetaData::valueTypeInt32:
            return MAV_PARAM_TYPE_INT32;
    }
}

FactMetaData::ValueType_t ParameterManager::_mavTypeToFactType(MAV_PARAM_TYPE mavType)
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

        case MAV_PARAM_TYPE_REAL32:
            return FactMetaData::valueTypeFloat;

        default:
            qWarning() << "Unsupported mav param type" << mavType;
            // fall through

        case MAV_PARAM_TYPE_INT32:
            return FactMetaData::valueTypeInt32;
    }
}

void ParameterManager::_addMetaDataToDefaultComponent(void)
{
     if (_defaultComponentId == MAV_COMP_ID_ALL) {
         // We don't know what the default component is so we can't support meta data
         return;
     }

     if (_parameterMetaData) {
         return;
     }

     QString metaDataFile;
     int majorVersion, minorVersion;

     // Load best parameter meta data set
     metaDataFile = parameterMetaDataFile(_vehicle, _vehicle->firmwareType(), _parameterSetMajorVersion, majorVersion, minorVersion);
     qCDebug(ParameterManagerLog) << "Adding meta data to Vehicle file:major:minor" << metaDataFile << majorVersion << minorVersion;

     _parameterMetaData = _vehicle->firmwarePlugin()->loadParameterMetaData(metaDataFile);

    // Loop over all parameters in default component adding meta data
    QVariantMap& factMap = _mapParameterName2Variant[_defaultComponentId];
    foreach (const QString& key, factMap.keys()) {
        _vehicle->firmwarePlugin()->addMetaDataToFact(_parameterMetaData, factMap[key].value<Fact*>(), _vehicle->vehicleType());
    }
}

/// @param failIfNoDefaultComponent true: Fails parameter load if no default component but we should have one
void ParameterManager::_checkInitialLoadComplete(bool failIfNoDefaultComponent)
{
    // Already processed?
    if (_initialLoadComplete) {
        return;
    }

    foreach (int componentId, _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            // We are still waiting on some parameters, not done yet
            return;
        }
    }

    if (!failIfNoDefaultComponent && _defaultComponentId == MAV_COMP_ID_ALL  && !_defaultComponentIdParam.isEmpty()) {
        // We are still waiting for default component to show up
        return;
    }

    // We aren't waiting for any more initial parameter updates, initial parameter loading is complete
    _initialLoadComplete = true;

    qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Initial load complete";

    // Check for index based load failures
    QString indexList;
    bool initialLoadFailures = false;
    foreach (int componentId, _failedReadParamIndexMap.keys()) {
        foreach (int paramIndex, _failedReadParamIndexMap[componentId]) {
            if (initialLoadFailures) {
                indexList += ", ";
            }
            indexList += QString("%1").arg(paramIndex);
            initialLoadFailures = true;
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Gave up on initial load after max retries (paramIndex:" << paramIndex << ")";
        }
    }

    _missingParameters = false;
    if (initialLoadFailures) {
        _missingParameters = true;
        QString errorMsg = tr("QGroundControl was unable to retrieve the full set of parameters from vehicle %1. "
                              "This will cause QGroundControl to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.").arg(_vehicle->id());
        qCDebug(ParameterManagerLog) << errorMsg;
        qgcApp()->showMessage(errorMsg);
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix() << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;
        }
    } else if (_defaultComponentId == MAV_COMP_ID_ALL && !_defaultComponentIdParam.isEmpty()) {
        // Missing default component when we should have one
        _missingParameters = true;
        QString errorMsg = tr("QGroundControl did not receive parameters from the default component for vehicle %1. "
                              "This will cause QGroundControl to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.").arg(_vehicle->id());
        qCDebug(ParameterManagerLog) << errorMsg;
        qgcApp()->showMessage(errorMsg);
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix() << "Default component was never found, param:" << _defaultComponentIdParam;
        }
    }

    // Signal load complete
    _parametersReady = true;
    _determineDefaultComponentId();
    _vehicle->autopilotPlugin()->parametersReadyPreChecks();
    emit parametersReadyChanged(true);
    emit missingParametersChanged(_missingParameters);
}

void ParameterManager::_initialRequestTimeout(void)
{
    if (!_disableAllRetries && ++_initialRequestRetryCount <= _maxInitialRequestListRetry) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix() << "Retyring initial parameter request list";
        refreshAllParameters();
        _initialRequestTimeoutTimer.start();
    } else {
        if (!_vehicle->genericFirmware()) {
            // Generic vehicles (like BeBop) may not have any parameters, so don't annoy the user
            QString errorMsg = tr("Vehicle %1 did not respond to request for parameters. "
                                  "This will cause QGroundControl to be unable to display its full user interface.").arg(_vehicle->id());
            qCDebug(ParameterManagerLog) << errorMsg;
            qgcApp()->showMessage(errorMsg);
        }
    }
}

QString ParameterManager::parameterMetaDataFile(Vehicle* vehicle, MAV_AUTOPILOT firmwareType, int wantedMajorVersion, int& majorVersion, int& minorVersion)
{
    bool            cacheHit = false;
    FirmwarePlugin* plugin = qgcApp()->toolbox()->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

    // Cached files are stored in settings location
    QSettings settings;
    QDir cacheDir = QFileInfo(settings.fileName()).dir();

    // First look for a direct cache hit
    int cacheMinorVersion, cacheMajorVersion;
    QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType).arg(wantedMajorVersion)));
    if (cacheFile.exists()) {
        plugin->getParameterMetaDataVersionInfo(cacheFile.fileName(), cacheMajorVersion, cacheMinorVersion);
        if (wantedMajorVersion != cacheMajorVersion) {
            qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName() << "major version does not match file name" << "actual:excepted" << cacheMajorVersion << wantedMajorVersion;
        } else {
            qCDebug(ParameterManagerLog) << "Direct cache hit on file:major:minor" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion;
            cacheHit = true;
        }
    }

    if (!cacheHit) {
        // No direct hit, look for lower param set version
        QString wildcard = QString("%1.%2.*.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType);
        QStringList cacheHits = cacheDir.entryList(QStringList(wildcard), QDir::Files, QDir::Name);

        // Find the highest major version number which is below the vehicles major version number
        int cacheHitIndex = -1;
        cacheMajorVersion = -1;
        QRegExp regExp(QString("%1\\.%2\\.(\\d*)\\.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType));
        for (int i=0; i< cacheHits.count(); i++) {
            if (regExp.exactMatch(cacheHits[i]) && regExp.captureCount() == 1) {
                int majorVersion = regExp.capturedTexts()[0].toInt();
                if (majorVersion > cacheMajorVersion && majorVersion < wantedMajorVersion) {
                    cacheMajorVersion = majorVersion;
                    cacheHitIndex = i;
                }
            }
        }

        if (cacheHitIndex != -1) {
            // We have a cache hit on a lower major version, read minor version as well
            int majorVersion;
            cacheFile.setFileName(cacheDir.filePath(cacheHits[cacheHitIndex]));
            plugin->getParameterMetaDataVersionInfo(cacheFile.fileName(), majorVersion, cacheMinorVersion);
            if (majorVersion != cacheMajorVersion) {
                qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName() << "major version does not match file name" << "actual:excepted" << majorVersion << cacheMajorVersion;
                cacheHit = false;
            } else {
                qCDebug(ParameterManagerLog) << "Indirect cache hit on file:major:minor:want" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion << wantedMajorVersion;
                cacheHit = true;
            }
        }
    }

    int internalMinorVersion, internalMajorVersion;
    QString internalMetaDataFile = plugin->internalParameterMetaDataFile(vehicle);
    plugin->getParameterMetaDataVersionInfo(internalMetaDataFile, internalMajorVersion, internalMinorVersion);
    qCDebug(ParameterManagerLog) << "Internal meta data file:major:minor" << internalMetaDataFile << internalMajorVersion << internalMinorVersion;
    if (cacheHit) {
        // Cache hit is available, we need to check if internal meta data is a better match, if so use internal version
        if (internalMajorVersion == wantedMajorVersion) {
            if (cacheMajorVersion == wantedMajorVersion) {
                // Both internal and cache are direct hit on major version, Use higher minor version number
                cacheHit = cacheMinorVersion > internalMinorVersion;
            } else {
                // Direct internal hit, but not direct hit in cache, use internal
                cacheHit = false;
            }
        } else {
            if (cacheMajorVersion == wantedMajorVersion) {
                // Direct hit on cache, no direct hit on internal, use cache
                cacheHit = true;
            } else {
                // No direct hit anywhere, use internal
                cacheHit = false;
            }
        }
    }

    QString metaDataFile;
    if (cacheHit && !qgcApp()->runningUnitTests()) {
        majorVersion = cacheMajorVersion;
        minorVersion = cacheMinorVersion;
        metaDataFile = cacheFile.fileName();
    } else {
        majorVersion = internalMajorVersion;
        minorVersion = internalMinorVersion;
        metaDataFile = internalMetaDataFile;
    }
    qCDebug(ParameterManagerLog) << "ParameterManager::parameterMetaDataFile file:major:minor" << metaDataFile << majorVersion << minorVersion;

    return metaDataFile;
}

void ParameterManager::cacheMetaDataFile(const QString& metaDataFile, MAV_AUTOPILOT firmwareType)
{
    FirmwarePlugin* plugin = qgcApp()->toolbox()->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

    int newMajorVersion, newMinorVersion;
    plugin->getParameterMetaDataVersionInfo(metaDataFile, newMajorVersion, newMinorVersion);
    qCDebug(ParameterManagerLog) << "ParameterManager::cacheMetaDataFile file:firmware:major;minor" << metaDataFile << firmwareType << newMajorVersion << newMinorVersion;

    // Find the cache hit closest to this new file
    int cacheMajorVersion, cacheMinorVersion;
    QString cacheHit = ParameterManager::parameterMetaDataFile(NULL, firmwareType, newMajorVersion, cacheMajorVersion, cacheMinorVersion);
    qCDebug(ParameterManagerLog) << "ParameterManager::cacheMetaDataFile cacheHit file:firmware:major;minor" << cacheHit << cacheMajorVersion << cacheMinorVersion;

    bool cacheNewFile = false;
    if (cacheHit.isEmpty()) {
        // No cache hits, store the new file
        cacheNewFile = true;
    } else if (cacheMajorVersion == newMajorVersion) {
        // Direct hit on major version in cache:
        //      Cache new file if newer minor version
        //      Also delete older cache file
        if (newMinorVersion > cacheMinorVersion) {
            cacheNewFile = true;
            QFile::remove(cacheHit);
        }
    } else {
        // Indirect hit in cache, store new file
        cacheNewFile = true;
    }

    if (cacheNewFile) {
        // Cached files are stored in settings location. Copy from current file to cache naming.

        QSettings settings;
        QDir cacheDir = QFileInfo(settings.fileName()).dir();
        QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType).arg(newMajorVersion)));
        qCDebug(ParameterManagerLog) << "ParameterManager::cacheMetaDataFile caching file:" << cacheFile.fileName();
        QFile newFile(metaDataFile);
        newFile.copy(cacheFile.fileName());
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

        _defaultComponentId = paramData.at(1).toInt();
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

        // Get parameter set version
        if (!_versionParam.isEmpty() && _versionParam == paramName) {
            _parameterSetMajorVersion = paramValue.toInt();
        }

        Fact* fact = new Fact(_defaultComponentId, paramName, _mavTypeToFactType(paramType), this);
        _mapParameterName2Variant[_defaultComponentId][paramName] = QVariant::fromValue(fact);
    }

    _addMetaDataToDefaultComponent();
    _setupGroupMap();
    _parametersReady = true;
    _initialLoadComplete = true;
}

void ParameterManager::saveToJson(int componentId, const QStringList& paramsToSave, QJsonObject& saveObject)
{
    QList<int>  rgCompIds;
    QStringList rgParamNames;

    if (componentId == MAV_COMP_ID_ALL) {
        rgCompIds = _mapParameterName2Variant.keys();
    } else {
        rgCompIds.append(_actualComponentId(componentId));
    }

    QJsonArray rgParams;

    // Loop over all requested component ids
    for (int i=0; i<rgCompIds.count(); i++) {
        int compId = rgCompIds[i];

        if (!_mapParameterName2Variant.contains(compId)) {
            qCDebug(ParameterManagerLog) << "ParameterManager::saveToJson no params for compId" << compId;
            continue;
        }

        // Build list of parameter names if not specified
        if (paramsToSave.count() == 0) {
            rgParamNames = 	parameterNames(compId);
        } else {
            rgParamNames = paramsToSave;
        }

        // Loop over parameter names adding each to json array
        for (int j=0; j<rgParamNames.count(); j++) {
            QString paramName = rgParamNames[j];

            if (!parameterExists(compId, paramName)) {
                qCDebug(ParameterManagerLog) << "ParameterManager::saveToJson param not found compId:param" << compId << paramName;
                continue;
            }

            QJsonObject paramJson;
            Fact* fact = getParameter(compId, paramName);
            paramJson.insert(_jsonCompIdKey, QJsonValue(compId));
            paramJson.insert(_jsonParamNameKey, QJsonValue(fact->name()));
            paramJson.insert(_jsonParamValueKey, QJsonValue(fact->rawValue().toDouble()));

            rgParams.append(QJsonValue(paramJson));
        }
    }

    saveObject.insert(_jsonParametersKey, QJsonValue(rgParams));
}

bool ParameterManager::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    QList<QJsonValue::Type> keyTypes;

    errorString.clear();

    if (required) {
        if (!JsonHelper::validateRequiredKeys(json, QStringList(_jsonParametersKey), errorString)) {
            return false;
        }
    } else if (!json.contains(_jsonParametersKey)) {
        return true;
    }

    keyTypes = { QJsonValue::Array };
    if (!JsonHelper::validateKeyTypes(json, QStringList(_jsonParametersKey), keyTypes, errorString)) {
        return false;
    }

    QJsonArray rgParams = json[_jsonParametersKey].toArray();
    for (int i=0; i<rgParams.count(); i++) {
        QJsonValueRef paramValue = rgParams[i];

        if (!paramValue.isObject()) {
            errorString = tr("%1 key is not a json object").arg(_jsonParametersKey);
            return false;
        }
        QJsonObject param = paramValue.toObject();

        QStringList requiredKeys = { _jsonCompIdKey, _jsonParamNameKey, _jsonParamValueKey };
        if (!JsonHelper::validateRequiredKeys(param, requiredKeys, errorString)) {
            return false;
        }
        keyTypes = { QJsonValue::Double, QJsonValue::String, QJsonValue::Double };
        if (!JsonHelper::validateKeyTypes(param, requiredKeys, keyTypes, errorString)) {
            return false;
        }

        int compId = param[_jsonCompIdKey].toInt();
        QString name = param[_jsonParamNameKey].toString();
        double value = param[_jsonParamValueKey].toDouble();

        if (!parameterExists(compId, name)) {
            qCDebug(ParameterManagerLog) << "ParameterManager::loadFromJson param not found compId:param" << compId << name;
            continue;
        }

        Fact* fact = getParameter(compId, name);
        fact->setRawValue(value);
    }

    return true;
}

void ParameterManager::resetAllParametersToDefaults(void)
{
    _vehicle->sendMavCommand(MAV_COMP_ID_ALL,
                             MAV_CMD_PREFLIGHT_STORAGE,
                             true,  // showError
                             2,     // Reset params to default
                             -1);   // Don't do anything with mission storage
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
    _loadProgress = loadProgress;
    emit loadProgressChanged(loadProgress);
}
