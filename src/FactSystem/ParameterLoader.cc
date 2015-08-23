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
#include "QGCMessageBox.h"
#include "UASMessageHandler.h"

#include <QFile>
#include <QDebug>

QGC_LOGGING_CATEGORY(ParameterLoaderLog, "ParameterLoaderLog")

Fact ParameterLoader::_defaultFact;

ParameterLoader::ParameterLoader(AutoPilotPlugin* autopilot, UASInterface* uas, QObject* parent) :
    QObject(parent),
    _autopilot(autopilot),
    _uas(uas),
    _mavlink(MAVLinkProtocol::instance()),
    _parametersReady(false),
    _initialLoadComplete(false),
    _defaultComponentId(FactSystem::defaultComponentId),
    _totalParamCount(0)
{
    Q_ASSERT(_autopilot);
    Q_ASSERT(_uas);
    Q_ASSERT(_mavlink);
    
    // We signal this to ouselves in order to start timer on our thread
    connect(this, &ParameterLoader::restartWaitingParamTimer, this, &ParameterLoader::_restartWaitingParamTimer);
    
    _waitingParamTimeoutTimer.setSingleShot(true);
    _waitingParamTimeoutTimer.setInterval(1000);
    connect(&_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterLoader::_waitingParamTimeout);
    
    // FIXME: Why not direct connect?
    connect(_uas, SIGNAL(parameterUpdate(int, int, QString, int, int, int, QVariant)), this, SLOT(_parameterUpdate(int, int, QString, int, int, int, QVariant)));
    
    // Request full param list
    refreshAllParameters();
}

ParameterLoader::~ParameterLoader()
{

}

/// Called whenever a parameter is updated or first seen.
void ParameterLoader::_parameterUpdate(int uasId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value)
{
    bool setMetaData = false;
    
    // Is this for our uas?
    if (uasId != _uas->getUASID()) {
        return;
    }
    
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
    
    _dataMutex.lock();
    
    // Restart our waiting for param timer
    _waitingParamTimeoutTimer.start();
    
    // Update our total parameter counts
    if (!_paramCountMap.contains(componentId)) {
        _paramCountMap[componentId] = parameterCount;
        _totalParamCount += parameterCount;
    }
    
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
    qCDebug(ParameterLoaderLog) << "_waitingReadParamIndexMap:" << _waitingReadParamIndexMap[componentId];
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
        _defaultComponentIdParam = getDefaultComponentIdParam();
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
        connect(fact, &Fact::_containerValueChanged, this, &ParameterLoader::_valueUpdated);
    }
    
    Q_ASSERT(_mapParameterName2Variant[componentId].contains(parameterName));
    
    Fact* fact = _mapParameterName2Variant[componentId][parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetValue(value);
    
    if (setMetaData) {
        _addMetaDataToFact(fact);
    }
    
    _dataMutex.unlock();
    
    if (waitingParamCount == 0) {
        // Now that we know vehicle is up to date persist
        _saveToEEPROM();
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
}

void ParameterLoader::_addMetaDataToFact(Fact* fact)
{
    FactMetaData* metaData = new FactMetaData(fact->type(), this);
    fact->setMetaData(metaData);
}

void ParameterLoader::refreshAllParameters(void)
{
    _dataMutex.lock();
    
    // Reset index wait lists
    foreach (int componentId, _paramCountMap.keys()) {
        // Add/Update all indices to the wait list, parameter index is 0-based
        for (int waitingIndex=0; waitingIndex<_paramCountMap[componentId]; waitingIndex++) {
            // This will add a new waiting index if needed and set the retry count for that index to 0
            _waitingReadParamIndexMap[componentId][waitingIndex] = 0;
        }
    }
    
    _dataMutex.unlock();
    
    MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();
    Q_ASSERT(mavlink);
    
    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, _uas->getUASID(), MAV_COMP_ID_ALL);
    _uas->sendMessage(msg);
    
    qCDebug(ParameterLoaderLog) << "Request to refresh all parameters";
}

void ParameterLoader::_determineDefaultComponentId(void)
{
    if (_defaultComponentId == FactSystem::defaultComponentId) {
        // We don't have a default component id yet. That means the plugin can't provide
        // the param to trigger off of. Instead we use the most prominent component id in
        // the set of parameters. Better than nothing!
        
        _defaultComponentId = -1;
        foreach(int componentId, _mapParameterName2Variant.keys()) {
            if (_mapParameterName2Variant[componentId].count() > _defaultComponentId) {
                _defaultComponentId = componentId;
            }
        }
        Q_ASSERT(_defaultComponentId != -1);
    }
}

/// Translates FactSystem::defaultComponentId to real component id if needed
int ParameterLoader::_actualComponentId(int componentId)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _defaultComponentId;
        Q_ASSERT(componentId != FactSystem::defaultComponentId);
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

    foreach(QString name, _mapParameterName2Variant[componentId].keys()) {
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

QStringList ParameterLoader::parameterNames(void)
{
	QStringList names;
	
	foreach(QString paramName, _mapParameterName2Variant[_defaultComponentId].keys()) {
		names << paramName;
	}
	
	return names;
}

void ParameterLoader::_setupGroupMap(void)
{
    foreach (int componentId, _mapParameterName2Variant.keys()) {
        foreach (QString name, _mapParameterName2Variant[componentId].keys()) {
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
            foreach(QString paramName, _waitingWriteParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                _writeParameterRaw(componentId, paramName, _autopilot->getFact(FactSystem::ParameterProvider, componentId, paramName)->value());
                qCDebug(ParameterLoaderLog) << "Write resend for (componentId:" << componentId << "paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap[componentId][paramName] << ")";
                
                if (++batchCount > maxBatchSize) {
                    goto Out;
                }
            }
        }
    }
    
    if (!paramsRequested) {
        foreach(int componentId, _waitingReadParamNameMap.keys()) {
            foreach(QString paramName, _waitingReadParamNameMap[componentId].keys()) {
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
                                        _uas->getUASID(),           // Target system id
                                        componentId,                // Target component id
                                        fixedParamName,             // Named parameter being requested
                                        paramIndex);                // Parameter index being requested, -1 for named
    _uas->sendMessage(msg);
}

void ParameterLoader::_writeParameterRaw(int componentId, const QString& paramName, const QVariant& value)
{
    bool floatHack = _uas->getAutopilotType() == MAV_AUTOPILOT_ARDUPILOTMEGA;

    mavlink_param_set_t     p;
    mavlink_param_union_t   union_value;
    
    FactMetaData::ValueType_t factType = _autopilot->getFact(FactSystem::ParameterProvider, componentId, paramName)->type();
    p.param_type = _factTypeToMavType(factType);
    
    switch (factType) {
        case FactMetaData::valueTypeUint8:
            if (floatHack) {
                union_value.param_float = (uint8_t)value.toUInt();
            } else {                
                union_value.param_uint8 = (uint8_t)value.toUInt();
            }
            break;
            
        case FactMetaData::valueTypeInt8:
            if (floatHack) {
                union_value.param_float = (int8_t)value.toInt();
            } else {
                union_value.param_int8 = (int8_t)value.toInt();
            }
            break;
            
        case FactMetaData::valueTypeUint16:
            if (floatHack) {
                union_value.param_float = (uint16_t)value.toUInt();
            } else {
                union_value.param_uint16 = (uint16_t)value.toUInt();
            }
            break;
            
        case FactMetaData::valueTypeInt16:
            if (floatHack) {
                union_value.param_float = (int16_t)value.toInt();
            } else {
                union_value.param_int16 = (int16_t)value.toInt();
            }
            break;
            
        case FactMetaData::valueTypeUint32:
            if (floatHack) {
                union_value.param_float = (uint32_t)value.toUInt();
            } else {
                union_value.param_uint32 = (uint32_t)value.toUInt();
            }
            break;
            
        case FactMetaData::valueTypeFloat:
            union_value.param_float = value.toFloat();
            break;
            
        default:
            qCritical() << "Unsupported fact type" << factType;
            // fall through
            
        case FactMetaData::valueTypeInt32:
            if (floatHack) {
                union_value.param_float = (int32_t)value.toInt();
            } else {
                union_value.param_int32 = (int32_t)value.toInt();
            }
            break;
    }
    
    p.param_value = union_value.param_float;
    p.target_system = (uint8_t)_uas->getUASID();
    p.target_component = (uint8_t)componentId;
        
    strncpy(p.param_id, paramName.toStdString().c_str(), sizeof(p.param_id));
    
    mavlink_message_t msg;
    mavlink_msg_param_set_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &p);
    _uas->sendMessage(msg);
}

void ParameterLoader::_saveToEEPROM(void)
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, _uas->getUASID(), 0, MAV_CMD_PREFLIGHT_STORAGE, 1, 1, -1, -1, -1, 0, 0, 0);
    _uas->sendMessage(msg);
    qCDebug(ParameterLoaderLog) << "_saveToEEPROM";
}

QString ParameterLoader::readParametersFromStream(QTextStream& stream)
{
    QString errors;
    bool userWarned = false;
    
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#")) {
            QStringList wpParams = line.split("\t");
            int lineMavId = wpParams.at(0).toInt();
            if (wpParams.size() == 5) {
                if (!userWarned && (_uas->getUASID() != lineMavId)) {
                    userWarned = true;
                    QString msg("The parameters in the stream have been saved from System Id %1, but the current vehicle has the System Id %2.");
                    QGCMessageBox::StandardButton button = QGCMessageBox::warning("Parameter Load",
                                                                                  msg.arg(lineMavId).arg(_uas->getUASID()),
                                                                                  QGCMessageBox::Ok | QGCMessageBox::Cancel,
                                                                                  QGCMessageBox::Cancel);
                    if (button == QGCMessageBox::Cancel) {
                        return QString();
                    }
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
                fact->setValue(valStr);
            }
        }
    }
    
    return errors;
}

void ParameterLoader::writeParametersToStream(QTextStream &stream, const QString& name)
{
    stream << "# Onboard parameters for system " << name << "\n";
    stream << "#\n";
    stream << "# MAV ID  COMPONENT ID  PARAM NAME  VALUE (FLOAT)\n";

    foreach (int componentId, _mapParameterName2Variant.keys()) {
        foreach (QString paramName, _mapParameterName2Variant[componentId].keys()) {
            Fact* fact = _mapParameterName2Variant[componentId][paramName].value<Fact*>();
            Q_ASSERT(fact);
            
            stream << _uas->getUASID() << "\t" << componentId << "\t" << paramName << "\t" << fact->valueString() << "\t" << QString("%1").arg(_factTypeToMavType(fact->type())) << "\n";
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
    
    UASMessageHandler* msgHandler = UASMessageHandler::instance();
    if (msgHandler->getErrorCountTotal()) {
        QString errors;
        bool firstError = true;
        bool errorsFound = false;
        
        msgHandler->lockAccess();
        foreach (UASMessage* msg, msgHandler->messages()) {
            if (msg->severityIsError()) {
                if (!firstError) {
                    errors += "\n";
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
            QString errorMsg = QString("Errors were detected during vehicle startup. You should resolve these prior to flight.\n%1").arg(errors);
            qgcApp()->showToolBarMessage(errorMsg);
        }
    }
    
    // Warn of parameter load failure
    
    if (initialLoadFailures) {
        QGCMessageBox::critical("Parameter Load Failure",
                                "QGroundControl was unable to retrieve the full set of parameters from the vehicle. "
                                "This will cause QGroundControl to be unable to display it's full user interface. "
                                "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                                "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.");
        qCWarning(ParameterLoaderLog) << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;

    } else {
        // No failed parameters, ok to signal ready
        _parametersReady = true;
        _determineDefaultComponentId();
        _setupGroupMap();
        emit parametersReady();
    }
}
