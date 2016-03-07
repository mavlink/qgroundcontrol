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

QGC_LOGGING_CATEGORY(ParameterLoaderVerboseLog, "ParameterLoaderVerboseLog")

Fact ParameterLoader::_defaultFact;

const char* ParameterLoader::_cachedMetaDataFilePrefix = "ParameterFactMetaData";

ParameterLoader::ParameterLoader(Vehicle* vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _mavlink(qgcApp()->toolbox()->mavlinkProtocol())
    , _dedicatedLink(_vehicle->priorityLink())
    , _parametersReady(false)
    , _initialLoadComplete(false)
    , _saveRequired(false)
    , _defaultComponentId(FactSystem::defaultComponentId)
    , _parameterSetMajorVersion(-1)
    , _parameterMetaData(NULL)
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
        _tryCacheHashLoad(uasId, componentId, value);
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

    // Determine default component id
    if (!_defaultComponentIdParam.isEmpty() && _defaultComponentIdParam == parameterName) {
        _defaultComponentId = componentId;
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

    if (waitingParamCount == 0) {
        // Now that we know vehicle is up to date persist
        _saveToEEPROM();
        _writeLocalParamCache(uasId, componentId);
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
    _vehicle->sendMessageOnLink(_dedicatedLink, msg);

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
                _writeParameterRaw(componentId, paramName, _vehicle->autopilotPlugin()->getFact(FactSystem::ParameterProvider, componentId, paramName)->rawValue());
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
    _vehicle->sendMessageOnLink(_dedicatedLink, msg);
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
    _vehicle->sendMessageOnLink(_dedicatedLink, msg);
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
    }
}

void ParameterLoader::_saveToEEPROM(void)
{
    if (_saveRequired) {
        _saveRequired = false;
        if (_vehicle->firmwarePlugin()->isCapable(FirmwarePlugin::MavCmdPreflightStorageCapability)) {
            mavlink_message_t msg;
            mavlink_msg_command_long_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, _vehicle->id(), 0, MAV_CMD_PREFLIGHT_STORAGE, 1, 1, -1, -1, -1, 0, 0, 0);
            _vehicle->sendMessageOnLink(_dedicatedLink, msg);
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

/// Adds meta data to all params after initial load completes
void ParameterLoader::_addMetaDataToAll(void)
{
     if (_defaultComponentId == FactSystem::defaultComponentId) {
         // We don't know what the default component is so we can't support meta data
         return;
     }

     if (_parameterMetaData) {
         // This should only be called once
         qWarning() << "Internal Error: ParameterLoader::_addMetaDataToAll with _parameterMetaData non NULL";
         return;
     }

    // Load best parameter meta data set
     int majorVersion, minorVersion;
     QString metaDataFile = parameterMetaDataFile(_vehicle->firmwareType(), _parameterSetMajorVersion, majorVersion, minorVersion);
     _parameterMetaData = _vehicle->firmwarePlugin()->loadParameterMetaData(metaDataFile);
     qCDebug(ParameterLoaderLog) << "Adding meta data to Vehicle file:major:minor" << metaDataFile << majorVersion << minorVersion;

    // Loop over all parameters in default component adding meta data
    QVariantMap& factMap = _mapParameterName2Variant[_defaultComponentId];
    foreach (const QString& key, factMap.keys()) {
        _vehicle->firmwarePlugin()->addMetaDataToFact(_parameterMetaData, factMap[key].value<Fact*>(), _vehicle->vehicleType());
    }
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

    // We can now add meta data since we should know parameter set version
    _addMetaDataToAll();

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
