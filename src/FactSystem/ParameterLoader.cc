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

#include "ParameterLoader.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "FirmwarePlugin.h"
#include "APMFirmwarePlugin.h"
#include "UAS.h"

#include <QEasingCurve>
#include <QFile>
#include <QDebug>
#include <QVariantAnimation>

/* types for local parameter cache */
typedef QPair<int, QVariant> ParamTypeVal;
typedef QPair<QString, ParamTypeVal> NamedParam;
typedef QMap<int, NamedParam> MapID2NamedParam;

QGC_LOGGING_CATEGORY(ParameterLoaderVerboseLog, "ParameterLoaderVerboseLog")

Fact ParameterLoader::_defaultFact;

const char* ParameterLoader::_cachedMetaDataFilePrefix = "ParameterFactMetaData";

ParameterLoader::ParameterLoader(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _mavlink(qgcApp()->toolbox()->mavlinkProtocol())
    , _parametersReady(false)
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
    , _totalParamCount(0)
{
    Q_ASSERT(_vehicle);
    Q_ASSERT(_mavlink);

    // We signal this to ouselves in order to start timer on our thread
    connect(this, &ParameterLoader::restartWaitingParamTimer, this, &ParameterLoader::_restartWaitingParamTimer);

    _initialRequestTimeoutTimer.setSingleShot(true);
    _initialRequestTimeoutTimer.setInterval(6000);
    connect(&_initialRequestTimeoutTimer, &QTimer::timeout, this, &ParameterLoader::_initialRequestTimeout);

    _waitingParamTimeoutTimer.setSingleShot(true);
    _waitingParamTimeoutTimer.setInterval(1000);
    connect(&_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterLoader::_waitingParamTimeout);

    connect(_vehicle->uas(), &UASInterface::parameterUpdate, this, &ParameterLoader::_parameterUpdate);

    _versionParam = vehicle->firmwarePlugin()->getVersionParam();
    _defaultComponentIdParam = vehicle->firmwarePlugin()->getDefaultComponentIdParam();
    qCDebug(ParameterLoaderLog) << "Default component param" << _defaultComponentIdParam;

    // Ensure the cache directory exists
    QFileInfo(QSettings().fileName()).dir().mkdir("ParamCache");
    refreshAllParameters();
}

ParameterLoader::~ParameterLoader()
{
    delete _parameterMetaData;
}

/// Called whenever a parameter is updated or first seen.
void ParameterLoader::_parameterUpdate(int uasId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value)
{
    // Is this for our uas?
    if (uasId != _vehicle->id()) {
        return;
    }

    _initialRequestTimeoutTimer.stop();

    if (_initialLoadComplete) {
        qCDebug(ParameterLoaderLog) << "_parameterUpdate (id:" << uasId <<
                                        "componentId:" << componentId <<
                                        "name:" << parameterName <<
                                        "count:" << parameterCount <<
                                        "index:" << parameterId <<
                                        "mavType:" << mavType <<
                                        "value:" << value <<
                                        ")";
    } else {
        // This is too noisy during initial load
        qCDebug(ParameterLoaderVerboseLog) << "_parameterUpdate (id:" << uasId <<
                                       "componentId:" << componentId <<
                                       "name:" << parameterName <<
                                       "count:" << parameterCount <<
                                       "index:" << parameterId <<
                                       "mavType:" << mavType <<
                                       "value:" << value <<
                                       ")";
    }

#if 0
    // Handy for testing retry logic
    static int counter = 0;
    if (counter++ & 0x8) {
        qCDebug(ParameterLoaderLog) << "Artificial discard" << counter;
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
        _tryCacheHashLoad(uasId, componentId, value);
        return;
    }
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

        qCDebug(ParameterLoaderLog) << "Seeing component for first time, id:" << componentId << "parameter count:" << parameterCount;
    }

    // Determine default component id
    if (!_defaultComponentIdParam.isEmpty() && _defaultComponentIdParam == parameterName) {
        qCDebug(ParameterLoaderLog) << "Default component id determined" << componentId;
        _defaultComponentId = componentId;
    }

    bool componentParamsComplete = false;
    if (_waitingReadParamIndexMap[componentId].count() == 1) {
        // We need to know when we get the last param from a component in order to complete setup
        componentParamsComplete = true;
    }

    if (_waitingReadParamIndexMap[componentId].contains(parameterId) ||
        _waitingReadParamNameMap[componentId].contains(parameterName) ||
        _waitingWriteParamNameMap[componentId].contains(parameterName)) {
        // We were waiting for this parameter, restart wait timer. Otherwise it is a spurious parameter update which
        // means we should not reset the wait timer.
        _waitingParamTimeoutTimer.start();
    }

    // Remove this parameter from the waiting lists
    _waitingReadParamIndexMap[componentId].remove(parameterId);
    _waitingReadParamNameMap[componentId].remove(parameterName);
    _waitingWriteParamNameMap[componentId].remove(parameterName);
    qCDebug(ParameterLoaderVerboseLog) << "_waitingReadParamIndexMap:" << _waitingReadParamIndexMap[componentId];
    qCDebug(ParameterLoaderLog) << "_waitingReadParamNameMap" << _waitingReadParamNameMap[componentId];
    qCDebug(ParameterLoaderLog) << "_waitingWriteParamNameMap" << _waitingWriteParamNameMap[componentId];

    // Track how many parameters we are still waiting for

    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamNameCount = 0;

    foreach(int waitingComponentId, _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[waitingComponentId].count();
    }
    if (waitingReadParamIndexCount) {
        qCDebug(ParameterLoaderLog) << "waitingReadParamIndexCount:" << waitingReadParamIndexCount;
    }


    foreach(int waitingComponentId, _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[waitingComponentId].count();
    }
    if (waitingReadParamNameCount) {
        qCDebug(ParameterLoaderLog) << "waitingReadParamNameCount:" << waitingReadParamNameCount;
    }

    foreach(int waitingComponentId, _waitingWriteParamNameMap.keys()) {
        waitingWriteParamNameCount += _waitingWriteParamNameMap[waitingComponentId].count();
    }
    if (waitingWriteParamNameCount) {
        qCDebug(ParameterLoaderLog) << "waitingWriteParamNameCount:" << waitingWriteParamNameCount;
    }

    int readWaitingParamCount = waitingReadParamIndexCount + waitingReadParamNameCount;
    int totalWaitingParamCount = readWaitingParamCount + waitingWriteParamNameCount;
    if (totalWaitingParamCount) {
        qCDebug(ParameterLoaderLog) << "totalWaitingParamCount:" << totalWaitingParamCount;
    } else if (_defaultComponentId != MAV_COMP_ID_ALL) {
        // No more parameters to wait for, stop the timeout. Be careful to not stop timer if we don't have the default
        // component yet.
        _waitingParamTimeoutTimer.stop();
    }

    // Update progress bar for waiting reads
    if (readWaitingParamCount == 0) {
        // We are no longer waiting for any reads to complete
        if (_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount != 0) {
            // Set progress to 0 if not already there
            emit parameterListProgress(0);
        }
    } else {
        emit parameterListProgress((float)(_totalParamCount - readWaitingParamCount) / (float)_totalParamCount);
    }

    // Get parameter set version
    if (!_versionParam.isEmpty() && _versionParam == parameterName) {
        _parameterSetMajorVersion = value.toInt();
    }

    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(parameterName)) {
        qCDebug(ParameterLoaderLog) << "Adding new fact";

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
        connect(fact, &Fact::_containerRawValueChanged, this, &ParameterLoader::_valueUpdated);
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
            _writeLocalParamCache(uasId, componentId);
        }
    }

    _prevWaitingReadParamIndexCount = waitingReadParamIndexCount;
    _prevWaitingReadParamNameCount = waitingReadParamNameCount;
    _prevWaitingWriteParamNameCount = waitingWriteParamNameCount;

    // Don't fail initial load complete if default component isn't found yet. That will be handled in wait timeout check.
    _checkInitialLoadComplete(false /* failIfNoDefaultComponent */);
}

/// Connected to Fact::valueUpdated
///
/// Writes the parameter to mavlink, sets up for write wait
void ParameterLoader::_valueUpdated(const QVariant& value)
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
    qCDebug(ParameterLoaderLog) << "Set parameter (componentId:" << componentId << "name:" << name << value << ")";

    if (fact->rebootRequired() && !qgcApp()->runningUnitTests()) {
        qgcApp()->showMessage(QStringLiteral("Change of parameter %1 requires a Vehicle reboot to take effect").arg(name));
    }
}

void ParameterLoader::refreshAllParameters(uint8_t componentID)
{
    _dataMutex.lock();

    if (!_initialLoadComplete) {
        _initialRequestTimeoutTimer.start();
    }

    // Reset index wait lists
    foreach (int cid, _paramCountMap.keys()) {
        // Add/Update all indices to the wait list, parameter index is 0-based
        if(componentID != MAV_COMP_ID_ALL && componentID != cid)
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
    mavlink_msg_param_request_list_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, _vehicle->id(), componentID);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);

    QString what = (componentID == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(componentID);
    qCDebug(ParameterLoaderLog) << "Request to refresh all parameters for component ID:" << what;
}

void ParameterLoader::_determineDefaultComponentId(void)
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
            qWarning() << "All parameters missing, unable to determine default componet id";
        }
    }
}

/// Translates FactSystem::defaultComponentId to real component id if needed
int ParameterLoader::_actualComponentId(int componentId)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _defaultComponentId;
        if (componentId == FactSystem::defaultComponentId) {
            qWarning() << "Default component id not set";
        }
    }

    return componentId;
}

void ParameterLoader::refreshParameter(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterLoaderLog) << "refreshParameter (component id:" << componentId << "name:" << name << ")";

    _dataMutex.lock();

    Q_ASSERT(_waitingReadParamNameMap.contains(componentId));

    if (_waitingReadParamNameMap.contains(componentId)) {
        QString mappedParamName = _remapParamNameToVersion(name);

        _waitingReadParamNameMap[componentId].remove(mappedParamName);  // Remove old wait entry if there
        _waitingReadParamNameMap[componentId][mappedParamName] = 0;     // Add new wait entry and update retry count
        emit restartWaitingParamTimer();
    }

    _dataMutex.unlock();

    _readParameterRaw(componentId, name, -1);
}

void ParameterLoader::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterLoaderLog) << "refreshParametersPrefix (component id:" << componentId << "name:" << namePrefix << ")";

    foreach(const QString &name, _mapParameterName2Variant[componentId].keys()) {
        if (name.startsWith(namePrefix)) {
            refreshParameter(componentId, name);
        }
    }
}

bool ParameterLoader::parameterExists(int componentId, const QString&  name)
{
    bool ret = false;

    componentId = _actualComponentId(componentId);
    if (_mapParameterName2Variant.contains(componentId)) {
        ret = _mapParameterName2Variant[componentId].contains(_remapParamNameToVersion(name));
    }

    return ret;
}

Fact* ParameterLoader::getFact(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);

    QString mappedParamName = _remapParamNameToVersion(name);
    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(mappedParamName)) {
        qgcApp()->reportMissingParameter(componentId, mappedParamName);
        return &_defaultFact;
    }

    return _mapParameterName2Variant[componentId][mappedParamName].value<Fact*>();
}

QStringList ParameterLoader::parameterNames(int componentId)
{
    QStringList names;

    foreach(const QString &paramName, _mapParameterName2Variant[_actualComponentId(componentId)].keys()) {
        names << paramName;
    }

    return names;
}

void ParameterLoader::_setupGroupMap(void)
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

const QMap<int, QMap<QString, QStringList> >& ParameterLoader::getGroupMap(void)
{
    return _mapGroup2ParameterName;
}

void ParameterLoader::_waitingParamTimeout(void)
{
    bool paramsRequested = false;
    const int maxBatchSize = 10;
    int batchCount = 0;

    // First check for any missing parameters from the initial index based load
    batchCount = 0;
    foreach(int componentId, _waitingReadParamIndexMap.keys()) {
        foreach(int paramIndex, _waitingReadParamIndexMap[componentId].keys()) {
            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam) {
                // Give up on this index
                _failedReadParamIndexMap[componentId] << paramIndex;
                qCDebug(ParameterLoaderLog) << "Giving up on (componentId:" << componentId << "paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
                _waitingReadParamIndexMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                paramsRequested = true;
                _readParameterRaw(componentId, "", paramIndex);
                qCDebug(ParameterLoaderLog) << "Read re-request for (componentId:" << componentId << "paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";

                if (++batchCount > maxBatchSize) {
                    goto Out;
                }
            }
        }
    }

    if (!paramsRequested && _defaultComponentId == MAV_COMP_ID_ALL && !_waitingForDefaultComponent) {
        // Initial load is complete but we still don't have default component params. Wait one more cycle to see if the
        // default component finally shows up.
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
                    _writeParameterRaw(componentId, paramName, _vehicle->autopilotPlugin()->getFact(FactSystem::ParameterProvider, componentId, paramName)->rawValue());
                    qCDebug(ParameterLoaderLog) << "Write resend for (componentId:" << componentId << "paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingWriteParamNameMap[componentId].remove(paramName);
                    qgcApp()->showMessage(tr("Parameter write failed: comp:%1 param:%2").arg(componentId).arg(paramName));
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
                    qCDebug(ParameterLoaderLog) << "Read re-request for (componentId:" << componentId << "paramName:" << paramName << "retryCount:" << _waitingReadParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingReadParamNameMap[componentId].remove(paramName);
                    qgcApp()->showMessage(tr("Parameter read failed: comp:%1 param:%2").arg(componentId).arg(paramName));
                }
            }
        }
    }

Out:
    if (paramsRequested) {
        _waitingParamTimeoutTimer.start();
    }
}

void ParameterLoader::_readParameterRaw(int componentId, const QString& paramName, int paramIndex)
{
    mavlink_message_t msg;
    char fixedParamName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN];

    strncpy(fixedParamName, paramName.toStdString().c_str(), sizeof(fixedParamName));
    mavlink_msg_param_request_read_pack(_mavlink->getSystemId(),    // Our system id
                                        _mavlink->getComponentId(), // Our component id
                                        &msg,                       // Pack into this mavlink_message_t
                                        _vehicle->id(),             // Target system id
                                        componentId,                // Target component id
                                        fixedParamName,             // Named parameter being requested
                                        paramIndex);                // Parameter index being requested, -1 for named
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void ParameterLoader::_writeParameterRaw(int componentId, const QString& paramName, const QVariant& value)
{
    mavlink_param_set_t     p;
    mavlink_param_union_t   union_value;

    FactMetaData::ValueType_t factType = _vehicle->autopilotPlugin()->getFact(FactSystem::ParameterProvider, componentId, paramName)->type();
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
    mavlink_msg_param_set_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &p);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void ParameterLoader::_writeLocalParamCache(int uasId, int componentId)
{
    MapID2NamedParam cache_map;

    foreach(int id, _mapParameterId2Name[componentId].keys()) {
        const QString name(_mapParameterId2Name[componentId][id]);
        const Fact *fact = _mapParameterName2Variant[componentId][name].value<Fact*>();
        cache_map[id] = NamedParam(name, ParamTypeVal(fact->type(), fact->rawValue()));
    }

    QFile cache_file(parameterCacheFile(uasId, componentId));
    cache_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream ds(&cache_file);
    ds << cache_map;
}

QDir ParameterLoader::parameterCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    return spath + QDir::separator() + "ParamCache";
}

QString ParameterLoader::parameterCacheFile(int uasId, int componentId)
{
    return parameterCacheDir().filePath(QString("%1_%2").arg(uasId).arg(componentId));
}

void ParameterLoader::_tryCacheHashLoad(int uasId, int componentId, QVariant hash_value)
{
    uint32_t crc32_value = 0;
    /* The datastructure of the cache table */
    MapID2NamedParam cache_map;
    QFile cache_file(parameterCacheFile(uasId, componentId));
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
        qCInfo(ParameterLoaderLog) << "Parameters loaded from cache" << qPrintable(QFileInfo(cache_file).absoluteFilePath());
        /* if the two param set hashes match, just load from the disk */
        int count = cache_map.count();
        foreach(int id, cache_map.keys()) {
            const QString &name = cache_map[id].first;
            const QVariant &value = cache_map[id].second.second;
            const FactMetaData::ValueType_t fact_type = static_cast<FactMetaData::ValueType_t>(cache_map[id].second.first);
            const int mavType = _factTypeToMavType(fact_type);
            _parameterUpdate(uasId, componentId, name, count, id, mavType, value);
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
        mavlink_msg_param_set_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &p);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);

        // Give the user some feedback things loaded properly
        QVariantAnimation *ani = new QVariantAnimation(this);
        ani->setEasingCurve(QEasingCurve::OutCubic);
        ani->setStartValue(0.0);
        ani->setEndValue(1.0);
        ani->setDuration(750);

        connect(ani, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            emit parameterListProgress(value.toFloat());
        });

        // Hide 500ms after animation finishes
        connect(ani, &QVariantAnimation::finished, [this](){
            QTimer::singleShot(500, [this]() {
                emit parameterListProgress(0);
            });
        });

        ani->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void ParameterLoader::_saveToEEPROM(void)
{
    if (_saveRequired) {
        _saveRequired = false;
        if (_vehicle->firmwarePlugin()->isCapable(FirmwarePlugin::MavCmdPreflightStorageCapability)) {
            mavlink_message_t msg;
            mavlink_msg_command_long_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, _vehicle->id(), 0, MAV_CMD_PREFLIGHT_STORAGE, 1, 1, -1, -1, -1, 0, 0, 0);
            _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
            qCDebug(ParameterLoaderLog) << "_saveToEEPROM";
        } else {
            qCDebug(ParameterLoaderLog) << "_saveToEEPROM skipped due to FirmwarePlugin::isCapable";
        }
    }
}

QString ParameterLoader::readParametersFromStream(QTextStream& stream)
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

                if (!_vehicle->autopilotPlugin()->factExists(FactSystem::ParameterProvider, componentId, paramName)) {
                    QString error;
                    error = QString("Skipped parameter %1:%2 - does not exist on this vehicle\n").arg(componentId).arg(paramName);
                    errors += error;
                    qCDebug(ParameterLoaderLog) << error;
                    continue;
                }

                Fact* fact = _vehicle->autopilotPlugin()->getFact(FactSystem::ParameterProvider, componentId, paramName);
                if (fact->type() != _mavTypeToFactType((MAV_PARAM_TYPE)mavType)) {
                    QString error;
                    error  = QString("Skipped parameter %1:%2 - type mismatch %3:%4\n").arg(componentId).arg(paramName).arg(fact->type()).arg(_mavTypeToFactType((MAV_PARAM_TYPE)mavType));
                    errors += error;
                    qCDebug(ParameterLoaderLog) << error;
                    continue;
                }

                qCDebug(ParameterLoaderLog) << "Updating parameter" << componentId << paramName << valStr;
                fact->setRawValue(valStr);
            }
        }
    }

    return errors;
}

void ParameterLoader::writeParametersToStream(QTextStream &stream)
{
    stream << "# Onboard parameters for vehicle " << _vehicle->id() << "\n";
    stream << "#\n";
    stream << "# MAV ID  COMPONENT ID  PARAM NAME  VALUE (FLOAT)\n";

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

MAV_PARAM_TYPE ParameterLoader::_factTypeToMavType(FactMetaData::ValueType_t factType)
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

FactMetaData::ValueType_t ParameterLoader::_mavTypeToFactType(MAV_PARAM_TYPE mavType)
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

void ParameterLoader::_restartWaitingParamTimer(void)
{
    _waitingParamTimeoutTimer.start();
}

void ParameterLoader::_addMetaDataToDefaultComponent(void)
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
     if (_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
         // Parameter versioning is still not really figured out correctly. We need to handle ArduPilot specially based on vehicle version.
         // The current three version are hardcoded in.
         metaDataFile = ((APMFirmwarePlugin*)_vehicle->firmwarePlugin())->getParameterMetaDataFile(_vehicle);
         qCDebug(ParameterLoaderLog) << "Adding meta data to Vehicle file:major:minor" << metaDataFile;
     } else {
         // Load best parameter meta data set
         metaDataFile = parameterMetaDataFile(_vehicle->firmwareType(), _parameterSetMajorVersion, majorVersion, minorVersion);
         qCDebug(ParameterLoaderLog) << "Adding meta data to Vehicle file:major:minor" << metaDataFile << majorVersion << minorVersion;
     }

     _parameterMetaData = _vehicle->firmwarePlugin()->loadParameterMetaData(metaDataFile);

    // Loop over all parameters in default component adding meta data
    QVariantMap& factMap = _mapParameterName2Variant[_defaultComponentId];
    foreach (const QString& key, factMap.keys()) {
        _vehicle->firmwarePlugin()->addMetaDataToFact(_parameterMetaData, factMap[key].value<Fact*>(), _vehicle->vehicleType());
    }
}

/// @param failIfNoDefaultComponent true: Fails parameter load if no default component but we should have one
void ParameterLoader::_checkInitialLoadComplete(bool failIfNoDefaultComponent)
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

    if (!failIfNoDefaultComponent && _defaultComponentId == MAV_COMP_ID_ALL) {
        // We are still waiting for default component to show up
        return;
    }

    // We aren't waiting for any more initial parameter updates, initial parameter loading is complete
    _initialLoadComplete = true;

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
            qCDebug(ParameterLoaderLog) << "Gave up on initial load after max retries (componentId:" << componentId << "paramIndex:" << paramIndex << ")";
        }
    }
    if (initialLoadFailures) {
        qgcApp()->showMessage("QGroundControl was unable to retrieve the full set of parameters from the vehicle. "
                              "This will cause QGroundControl to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.");
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterLoaderLog) << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;
        }
        emit parametersReady(true /* missingParameters */);
        return;
    }

    // Check for missing default component when we should have one
    if (_defaultComponentId == FactSystem::defaultComponentId && !_defaultComponentIdParam.isEmpty()) {
        qgcApp()->showMessage("QGroundControl did not receive parameters from the default component. "
                              "This will cause QGroundControl to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.");
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterLoaderLog) << "Default component was never found, param:" << _defaultComponentIdParam;
        }
        emit parametersReady(true /* missingParameters */);
        return;
    }

    // No failures, signal good load
    _parametersReady = true;
    _determineDefaultComponentId();
    emit parametersReady(false /* no missingParameters */);
}

void ParameterLoader::_initialRequestTimeout(void)
{
    if (!_vehicle->genericFirmware()) {
        // Generic vehicles (like BeBop) may not have any parameters, so don't annoy the user
        qgcApp()->showMessage("Vehicle did not respond to request for parameters, retrying");
    }
    if (++_initialRequestRetryCount <= _maxInitialRequestListRetry) {
        refreshAllParameters();
        _initialRequestTimeoutTimer.start();
    }
}

QString ParameterLoader::parameterMetaDataFile(MAV_AUTOPILOT firmwareType, int wantedMajorVersion, int& majorVersion, int& minorVersion)
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
            qCDebug(ParameterLoaderLog) << "Direct cache hit on file:major:minor" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion;
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
                qCDebug(ParameterLoaderLog) << "Indirect cache hit on file:major:minor:want" << cacheFile.fileName() << cacheMajorVersion << cacheMinorVersion << wantedMajorVersion;
                cacheHit = true;
            }
        }
    }

    int internalMinorVersion, internalMajorVersion;
    QString internalMetaDataFile = plugin->internalParameterMetaDataFile();
    plugin->getParameterMetaDataVersionInfo(internalMetaDataFile, internalMajorVersion, internalMinorVersion);
    qCDebug(ParameterLoaderLog) << "Internal meta data file:major:minor" << internalMetaDataFile << internalMajorVersion << internalMinorVersion;
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
    qCDebug(ParameterLoaderLog) << "ParameterLoader::parameterMetaDataFile file:major:minor" << metaDataFile << majorVersion << minorVersion;

    return metaDataFile;
}

void ParameterLoader::cacheMetaDataFile(const QString& metaDataFile, MAV_AUTOPILOT firmwareType)
{
    FirmwarePlugin* plugin = qgcApp()->toolbox()->firmwarePluginManager()->firmwarePluginForAutopilot(firmwareType, MAV_TYPE_QUADROTOR);

    int newMajorVersion, newMinorVersion;
    plugin->getParameterMetaDataVersionInfo(metaDataFile, newMajorVersion, newMinorVersion);
    qCDebug(ParameterLoaderLog) << "ParameterLoader::cacheMetaDataFile file:firmware:major;minor" << metaDataFile << firmwareType << newMajorVersion << newMinorVersion;

    // Find the cache hit closest to this new file
    int cacheMajorVersion, cacheMinorVersion;
    QString cacheHit = ParameterLoader::parameterMetaDataFile(firmwareType, newMajorVersion, cacheMajorVersion, cacheMinorVersion);
    qCDebug(ParameterLoaderLog) << "ParameterLoader::cacheMetaDataFile cacheHit file:firmware:major;minor" << cacheHit << cacheMajorVersion << cacheMinorVersion;

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
        qCDebug(ParameterLoaderLog) << "ParameterLoader::cacheMetaDataFile caching file:" << cacheFile.fileName();
        QFile newFile(metaDataFile);
        newFile.copy(cacheFile.fileName());
    }
}

/// Remap a parameter from one firmware version to another
QString ParameterLoader::_remapParamNameToVersion(const QString& paramName)
{
    QString mappedParamName;

    if (!paramName.startsWith(QStringLiteral("r."))) {
        // No version mapping wanted
        return paramName;
    }

    int majorVersion = _vehicle->firmwareMajorVersion();
    int minorVersion = _vehicle->firmwareMinorVersion();

    qCDebug(ParameterLoaderLog) << "_remapParamNameToVersion" << paramName << majorVersion << minorVersion;

    mappedParamName = paramName.right(paramName.count() - 2);

    if (majorVersion == Vehicle::versionNotSetValue) {
        // Vehicle version unknown
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMajorVersionMap_t& majorVersionRemap = _vehicle->firmwarePlugin()->paramNameRemapMajorVersionMap();

    if (!majorVersionRemap.contains(majorVersion)) {
        // No mapping for this major version
        qCDebug(ParameterLoaderLog) << "_remapParamNameToVersion: no major version mapping";
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMinorVersionRemapMap_t& remapMinorVersion = majorVersionRemap[majorVersion];

    // We must map backwards from the highest known minor version to one above the vehicle's minor version

    for (int currentMinorVersion=_vehicle->firmwarePlugin()->remapParamNameHigestMinorVersionNumber(majorVersion); currentMinorVersion>minorVersion; currentMinorVersion--) {
        if (remapMinorVersion.contains(currentMinorVersion)) {
            const FirmwarePlugin::remapParamNameMap_t& remap = remapMinorVersion[currentMinorVersion];

            if (remap.contains(mappedParamName)) {
                QString toParamName = remap[mappedParamName];
                qCDebug(ParameterLoaderLog) << "_remapParamNameToVersion: remapped currentMinor:from:to"<< currentMinorVersion << mappedParamName << toParamName;
                mappedParamName = toParamName;
            }
        }
    }

    return mappedParamName;
}
