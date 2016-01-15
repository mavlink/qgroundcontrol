/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ParameterLoader.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "FirmwarePlugin.h"
#include "UAS.h"

#include <QFile>
#include <QDebug>

/* types for local parameter cache */
typedef QPair<int, QVariant> ParamTypeVal;
typedef QPair<QString, ParamTypeVal> NamedParam;
typedef QMap<int, NamedParam> MapID2NamedParam;

QGC_LOGGING_CATEGORY(ParameterLoaderLog, "ParameterLoaderLog")
QGC_LOGGING_CATEGORY(ParameterLoaderVerboseLog, "ParameterLoaderVerboseLog")

Fact ParameterLoader::_defaultFact;

ParameterLoader::ParameterLoader(AutoPilotPlugin* autopilot, Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _autopilot(autopilot)
    , _vehicle(vehicle)
    , _mavlink(qgcApp()->toolbox()->mavlinkProtocol())
    , _parametersReady(false)
    , _initialLoadComplete(false)
    , _defaultComponentId(FactSystem::defaultComponentId)
    , _totalParamCount(0)
{
    Q_ASSERT(_autopilot);
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

    _cacheTimeoutTimer.setSingleShot(true);
    _cacheTimeoutTimer.setInterval(2500);
    connect(&_cacheTimeoutTimer, &QTimer::timeout, this, &ParameterLoader::_timeoutRefreshAll);

    connect(_vehicle->uas(), &UASInterface::parameterUpdate, this, &ParameterLoader::_parameterUpdate);

    /* Initially attempt a local cache load, refresh over the link if it fails */
    _tryCacheLookup();
}

ParameterLoader::~ParameterLoader()
{

}

/// Called whenever a parameter is updated or first seen.
void ParameterLoader::_parameterUpdate(int uasId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value)
{
    bool setMetaData = false;

    // Is this for our uas?
    if (uasId != _vehicle->id()) {
        return;
    }

    _initialRequestTimeoutTimer.stop();

    qCDebug(ParameterLoaderLog) << "_parameterUpdate (usaId:" << uasId <<
                                    "componentId:" << componentId <<
                                    "name:" << parameterName <<
                                    "count:" << parameterCount <<
                                    "index:" << parameterId <<
                                    "mavType:" << mavType <<
                                    "value:" << value <<
                                    ")";

#if 0
    // Handy for testing retry logic
    static int counter = 0;
    if (counter++ & 0x3) {
        qCDebug(ParameterLoaderLog) << "Artificial discard" << counter;
        return;
    }
#endif

    if (parameterName == "_HASH_CHECK") {
        /* we received a cache hash, potentially load from cache */
        _cacheTimeoutTimer.stop();
        _tryCacheHashLoad(uasId, value);
        return;
    }
    _dataMutex.lock();

    // Restart our waiting for param timer
    _waitingParamTimeoutTimer.start();

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

    int waitingParamCount = waitingReadParamIndexCount + waitingReadParamNameCount + waitingWriteParamNameCount;
    if (waitingParamCount) {
        qCDebug(ParameterLoaderLog) << "waitingParamCount:" << waitingParamCount;
    } else {
        // No more parameters to wait for, stop the timeout
        _waitingParamTimeoutTimer.stop();
    }

    // Update progress bar
    if (waitingParamCount == 0) {
        emit parameterListProgress(0);
    } else {
        emit parameterListProgress((float)(_totalParamCount - waitingParamCount) / (float)_totalParamCount);
    }

    // Attempt to determine default component id
    if (_defaultComponentId == FactSystem::defaultComponentId && _defaultComponentIdParam.isEmpty()) {
        _defaultComponentIdParam = _vehicle->firmwarePlugin()->getDefaultComponentIdParam();
    }
    if (!_defaultComponentIdParam.isEmpty() && _defaultComponentIdParam == parameterName) {
        _defaultComponentId = componentId;
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
        setMetaData = true;

        _mapParameterName2Variant[componentId][parameterName] = QVariant::fromValue(fact);

        // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerRawValueChanged, this, &ParameterLoader::_valueUpdated);
    }

    _dataMutex.unlock();

    Q_ASSERT(_mapParameterName2Variant[componentId].contains(parameterName));

    Fact* fact = _mapParameterName2Variant[componentId][parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetRawValue(value);

    if (setMetaData) {
        _vehicle->firmwarePlugin()->addMetaDataToFact(fact, _vehicle->vehicleType());
    }

    if (waitingParamCount == 0) {
        // Now that we know vehicle is up to date persist
        _saveToEEPROM();
        _writeLocalParamCache();
    }

    _checkInitialLoadComplete();
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
    if (_defaultComponentId == FactSystem::defaultComponentId) {
        // We don't have a default component id yet. That means the plugin can't provide
        // the param to trigger off of. Instead we use the most prominent component id in
        // the set of parameters. Better than nothing!

        _defaultComponentId = -1;
        int largestCompParamCount = 0;
        foreach(int componentId, _mapParameterName2Variant.keys()) {
            int compParamCount = _mapParameterName2Variant[componentId].count();
            if (compParamCount > largestCompParamCount) {
                largestCompParamCount = compParamCount;
                _defaultComponentId = componentId;
            }
        }

        if (_defaultComponentId == -1) {
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
        _waitingReadParamNameMap[componentId].remove(name); // Remove old wait entry if there
        _waitingReadParamNameMap[componentId][name] = 0;    // Add new wait entry and update retry count
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
        ret = _mapParameterName2Variant[componentId].contains(name);
    }

    return ret;
}

Fact* ParameterLoader::getFact(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);

    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(name)) {
        qgcApp()->reportMissingParameter(componentId, name);
        return &_defaultFact;
    }

    return _mapParameterName2Variant[componentId][name].value<Fact*>();
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

    // We timed out waiting for some parameters from the initial set. Re-request those.

    batchCount = 0;
    foreach(int componentId, _waitingReadParamIndexMap.keys()) {
        foreach(int paramIndex, _waitingReadParamIndexMap[componentId].keys()) {
            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetry) {
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
    // We need to check for initial load complete here as well, since it could complete on a max retry failure
    _checkInitialLoadComplete();

    if (!paramsRequested) {
        foreach(int componentId, _waitingWriteParamNameMap.keys()) {
            foreach(const QString &paramName, _waitingWriteParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                _writeParameterRaw(componentId, paramName, _autopilot->getFact(FactSystem::ParameterProvider, componentId, paramName)->rawValue());
                qCDebug(ParameterLoaderLog) << "Write resend for (componentId:" << componentId << "paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";

                if (++batchCount > maxBatchSize) {
                    goto Out;
                }
            }
        }
    }

    if (!paramsRequested) {
        foreach(int componentId, _waitingReadParamNameMap.keys()) {
            foreach(const QString &paramName, _waitingReadParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingReadParamNameMap[componentId][paramName]++;   // Bump retry count
                _readParameterRaw(componentId, paramName, -1);
                qCDebug(ParameterLoaderLog) << "Read re-request for (componentId:" << componentId << "paramName:" << paramName << "retryCount:" << _waitingReadParamNameMap[componentId][paramName] << ")";

                if (++batchCount > maxBatchSize) {
                    goto Out;
                }
            }
        }
    }

Out:
    if (paramsRequested) {
        _waitingParamTimeoutTimer.start();
    }
}

void ParameterLoader::_tryCacheLookup()
{
    /* Start waiting for 2.5 seconds to get a cache hit and avoid loading all params over the radio */
    _cacheTimeoutTimer.start();

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    Q_ASSERT(mavlink);

    mavlink_message_t msg;
    mavlink_msg_param_request_read_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, _vehicle->id(), MAV_COMP_ID_ALL, "_HASH_CHECK", -1);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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

    FactMetaData::ValueType_t factType = _autopilot->getFact(FactSystem::ParameterProvider, componentId, paramName)->type();
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

void ParameterLoader::_writeLocalParamCache()
{
    QMap<int, MapID2NamedParam> cache_map;

    foreach(int component, _mapParameterId2Name.keys()) {
        foreach(int id, _mapParameterId2Name[component].keys()) {
            const QString name(_mapParameterId2Name[component][id]);
            const Fact *fact = _mapParameterName2Variant[component][name].value<Fact*>();
            cache_map[component][id] = NamedParam(name, ParamTypeVal(fact->type(), fact->rawValue()));
        }
    }

    QFile cache_file(QFileInfo(QSettings().fileName()).path() + QDir::separator() + "param_cache");
    cache_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream ds(&cache_file);
    ds << cache_map;
}

void ParameterLoader::_tryCacheHashLoad(int uasId, QVariant hash_value)
{
    uint32_t crc32_value = 0;
    /* The datastructure of the cache table */
    QMap<int, MapID2NamedParam> cache_map;
    const QDir settingsDir(QFileInfo(QSettings().fileName()).dir());
    QFile cache_file(settingsDir.filePath("param_cache"));
    if (!cache_file.exists()) {
        /* no local cache, immediately refresh all params */
        refreshAllParameters();
        return;
    }
    cache_file.open(QIODevice::ReadOnly);

    /* Deserialize the parameter cache table */
    QDataStream ds(&cache_file);
    ds >> cache_map;

    /* compute the crc of the local cache to check against the remote */
    foreach(int component, cache_map.keys()) {
        foreach(int id, cache_map[component].keys()) {
            const QString name(cache_map[component][id].first);
            const void *vdat = cache_map[component][id].second.second.constData();
            crc32_value = QGC::crc32((const uint8_t *)qPrintable(name), name.length(),  crc32_value);
            crc32_value = QGC::crc32((const uint8_t *)vdat, sizeof(uint32_t), crc32_value);
        }
    }

    if (crc32_value == hash_value.toUInt()) {
        /* if the two param set hashes match, just load from the disk */
        foreach(int component, cache_map.keys()) {
            int count = cache_map[component].count();
            foreach(int id, cache_map[component].keys()) {
                const QString &name = cache_map[component][id].first;
                const QVariant &value = cache_map[component][id].second.second;
                const int mavType = _factTypeToMavType(static_cast<FactMetaData::ValueType_t>(cache_map[component][id].second.first));
                _parameterUpdate(uasId, component, name, count, id, mavType, value);
            }
        }
    } else {
        /* cache and remote hashes differ. Immediately request all params */
        refreshAllParameters();
    }
}

void ParameterLoader::_saveToEEPROM(void)
{
    if (_vehicle->firmwarePlugin()->isCapable(FirmwarePlugin::MavCmdPreflightStorageCapability)) {
        mavlink_message_t msg;
        mavlink_msg_command_long_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, _vehicle->id(), 0, MAV_CMD_PREFLIGHT_STORAGE, 1, 1, -1, -1, -1, 0, 0, 0);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
        qCDebug(ParameterLoaderLog) << "_saveToEEPROM";
    } else {
        qCDebug(ParameterLoaderLog) << "_saveToEEPROM skipped due to FirmwarePlugin::isCapable";
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

                if (!_autopilot->factExists(FactSystem::ParameterProvider, componentId, paramName)) {
                    QString error;
                    error = QString("Skipped parameter %1:%2 - does not exist on this vehicle\n").arg(componentId).arg(paramName);
                    errors += error;
                    qCDebug(ParameterLoaderLog) << error;
                    continue;
                }

                Fact* fact = _autopilot->getFact(FactSystem::ParameterProvider, componentId, paramName);
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
            Q_ASSERT(fact);

            stream << _vehicle->id() << "\t" << componentId << "\t" << paramName << "\t" << fact->rawValueString() << "\t" << QString("%1").arg(_factTypeToMavType(fact->type())) << "\n";
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

void ParameterLoader::_checkInitialLoadComplete(void)
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


    // We aren't waiting for any more initial parameter updates, initial parameter loading is complete
    _initialLoadComplete = true;

    // Check for load failures
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

    // Check for any errors during vehicle boot

    UASMessageHandler* msgHandler = qgcApp()->toolbox()->uasMessageHandler();
    if (msgHandler->getErrorCountTotal()) {
        QString errors;
        bool firstError = true;
        bool errorsFound = false;

        msgHandler->lockAccess();
        foreach (UASMessage* msg, msgHandler->messages()) {
            if (msg->severityIsError()) {
                if (!firstError) {
                    errors += "<br>";
                }
                errors += " - ";
                errors += msg->getText();
                firstError = false;
                errorsFound = true;
            }
        }
        msgHandler->showErrorsInToolbar();
        msgHandler->unlockAccess();

        if (errorsFound) {
            QString errorMsg = QString("<b>Critical safety issue detected:</b><br>%1").arg(errors);
            qgcApp()->showMessage(errorMsg);
        }
    }

    // Warn of parameter load failure

    if (initialLoadFailures) {
        qgcApp()->showMessage("QGroundControl was unable to retrieve the full set of parameters from the vehicle. "
                              "This will cause QGroundControl to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.");
        qCWarning(ParameterLoaderLog) << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;
        emit parametersReady(true);
    } else {
        // No failed parameters, ok to signal ready
        _parametersReady = true;
        _determineDefaultComponentId();
        _setupGroupMap();
        emit parametersReady(false);
    }
}

void ParameterLoader::_initialRequestTimeout(void)
{
    qgcApp()->showMessage("Vehicle did not respond to request for parameters, retrying");
    refreshAllParameters();
    _initialRequestTimeoutTimer.start();
}

void ParameterLoader::_timeoutRefreshAll()
{
    refreshAllParameters();
}

