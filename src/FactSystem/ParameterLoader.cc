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

#include <QFile>
#include <QDebug>

QGC_LOGGING_CATEGORY(ParameterLoaderLog, "ParameterLoaderLog")

ParameterLoader::ParameterLoader(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _paramMgr(NULL),
    _parametersReady(false),
    _defaultComponentId(FactSystem::defaultComponentId)
{
    Q_ASSERT(uas);

    _uasId = uas->getUASID();
    
    _paramMgr = uas->getParamManager();
    Q_ASSERT(_paramMgr);
    
    // We need to be initialized before param mgr starts sending parameters so we catch each one
    Q_ASSERT(!_paramMgr->parametersReady());
    
    // We need to know when the param mgr is done sending the initial set of paramters
    connect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_paramMgrParameterListUpToDate()));
    
    // We track parameters changes to keep Facts up to date.
    connect(uas, &UASInterface::parameterUpdate, this, &ParameterLoader::_parameterUpdate);
}

ParameterLoader::~ParameterLoader()
{

}

/// Called whenever a parameter is updated or first seen.
void ParameterLoader::_parameterUpdate(int uas, int componentId, QString parameterName, int mavType, QVariant value)
{
    bool setMetaData = false;
    
    // Is this for our uas?
    if (uas != _uasId) {
        return;
    }
    
    // Attempt to determine default component id
    if (_defaultComponentId == FactSystem::defaultComponentId && _defaultComponentIdParam.isEmpty()) {
        _defaultComponentIdParam = getDefaultComponentIdParam();
    }
    if (!_defaultComponentIdParam.isEmpty() && _defaultComponentIdParam == parameterName) {
        _defaultComponentId = componentId;
    }
    
    if (!_mapParameterName2Variant.contains(componentId) || !_mapParameterName2Variant[componentId].contains(parameterName)) {
        // These should not get our of sync
        Q_ASSERT(_mapParameterName2Variant.contains(componentId) == _mapFact2ParameterName.contains(componentId));
        
        qCDebug(ParameterLoaderLog) << "Adding new fact (component:" << componentId << "name:" << parameterName << ")";
        
        FactMetaData::ValueType_t factType;
        switch (mavType) {
            case MAV_PARAM_TYPE_UINT8:
                factType = FactMetaData::valueTypeUint8;
                break;
            case MAV_PARAM_TYPE_INT8:
                factType = FactMetaData::valueTypeUint8;
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
        _mapFact2ParameterName[componentId][fact] = parameterName;
        
        // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerValueChanged, this, &ParameterLoader::_valueUpdated);
    }
    
    Q_ASSERT(_mapParameterName2Variant[componentId].contains(parameterName));
    
    qCDebug(ParameterLoaderLog) << "Updating fact value (component:" << componentId << "name:" << parameterName << value << ")";
    
    Fact* fact = _mapParameterName2Variant[componentId][parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetValue(value);
    
    if (setMetaData) {
        _addMetaDataToFact(fact);
    }
}

/// Connected to Fact::valueUpdated
///
/// Sets the new value into the Parameter Manager. Parameter is persisted after send.
void ParameterLoader::_valueUpdated(const QVariant& value)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    Q_ASSERT(fact);
    
    int componentId = fact->componentId();
    
    Q_ASSERT(_paramMgr);
    Q_ASSERT(_mapFact2ParameterName.contains(componentId));
    Q_ASSERT(_mapFact2ParameterName[componentId].contains(fact));
    
    QVariant typedValue;
    switch (fact->type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue.setValue(QVariant(value.toInt()));
            break;
            
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue.setValue(value.toUInt());
            break;
            
        case FactMetaData::valueTypeFloat:
            typedValue.setValue(value.toFloat());
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue.setValue(value.toDouble());
            break;
    }
    
    qCDebug(ParameterLoaderLog) << "Set parameter (componentId:" << componentId << "name:" << fact->name() << typedValue << ")";

    _paramMgr->setParameter(componentId, fact->name(), typedValue);
    _paramMgr->sendPendingParameters(true /* persistAfterSend */, false /* forceSend */);
}

// Called when param mgr list is up to date
void ParameterLoader::_paramMgrParameterListUpToDate(void)
{
    if (!_parametersReady) {
        _parametersReady = true;
        
        // We don't need this any more
        disconnect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_paramMgrParameterListUpToDate()));

        // There may be parameterUpdated signals still in our queue. Flush them out.
        qgcApp()->processEvents();
        
        _determineDefaultComponentId();
        
        // We should have all parameters now so we can signal ready
        emit parametersReady();
    }
}

void ParameterLoader::_addMetaDataToFact(Fact* fact)
{
    FactMetaData* metaData = new FactMetaData(this);
    metaData->initFromTypeOnly(fact->type());
}

void ParameterLoader::refreshAllParameters(void)
{
    Q_ASSERT(_paramMgr);
    _paramMgr->requestParameterList();
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
    Q_ASSERT(_paramMgr);
    
    _paramMgr->requestParameterUpdate(_actualComponentId(componentId), name);
}

void ParameterLoader::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    Q_ASSERT(_paramMgr);
    
    componentId = _actualComponentId(componentId);
    Q_ASSERT(_mapFact2ParameterName.contains(componentId));
    foreach(QString name, _mapParameterName2Variant[componentId].keys()) {
        if (name.startsWith(namePrefix)) {
            refreshParameter(componentId, name);
        }
    }
}

bool ParameterLoader::factExists(int componentId, const QString&  name)
{
    componentId = _actualComponentId(componentId);
    if (_mapParameterName2Variant.contains(componentId)) {
        return _mapParameterName2Variant[componentId].contains(name);
    }
    return false;
}

Fact* ParameterLoader::getFact(int componentId, const QString& name)
{
    componentId = _actualComponentId(componentId);
    Q_ASSERT(_mapParameterName2Variant.contains(componentId));
    Q_ASSERT(_mapParameterName2Variant[componentId].contains(name));
    Fact* fact = _mapParameterName2Variant[componentId][name].value<Fact*>();
    Q_ASSERT(fact);
    return fact;
}
