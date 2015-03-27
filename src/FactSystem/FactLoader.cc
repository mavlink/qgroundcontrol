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

#include "FactLoader.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "MavParamHelper.h"

#include <QFile>
#include <QDebug>

QGC_LOGGING_CATEGORY(FactLoaderLog, "FactLoaderLog")

FactLoader::FactLoader(UASInterface* uas, QObject* parent) :
    QObject(parent),
    _lastSeenComponent(-1),
    _paramMgr(NULL),
    _factsReady(false)
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
    connect(uas, &UASInterface::parameterUpdate, this, &FactLoader::_parameterUpdate);
}

FactLoader::~FactLoader()
{

}

/// Called whenever a parameter is updated or first seen.
void FactLoader::_parameterUpdate(int uas, int component, QString parameterName, mavlink_param_union_t& paramUnion)
{
    // Is this for our uas?
    if (uas != _uasId) {
        return;
    }
    
    if (_lastSeenComponent == -1) {
        _lastSeenComponent = component;
    } else {
        // Code cannot handle parameters coming form different components yet
        Q_ASSERT(component == _lastSeenComponent);
    }
    
    bool setMetaData = false;
    if (!_mapParameterName2Variant.contains(parameterName)) {
        qCDebug(FactLoaderLog) << "Adding new fact" << parameterName;
        
        FactMetaData::ValueType_t factType;
        switch (paramUnion.type) {
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
                qCritical() << "Unsupported fact type" << paramUnion.type;
                break;
        }
        
        Fact* fact = new Fact(parameterName, factType, this);
        setMetaData = true;
        
        _mapParameterName2Variant[parameterName] = QVariant::fromValue(fact);
        _mapFact2ParameterName[fact] = parameterName;
        
        // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerValueChanged, this, &FactLoader::_valueUpdated);
    }
    
    Q_ASSERT(_mapParameterName2Variant.contains(parameterName));
    
    QVariant paramVar = MavParamUnionToVariant(paramUnion);
    qCDebug(FactLoaderLog) << "Updating fact value" << parameterName << paramVar;
    
    Fact* fact = _mapParameterName2Variant[parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetValue(paramVar);
    
    if (setMetaData) {
        _addMetaDataToFact(fact);
    }
}

/// Connected to Fact::valueUpdated
///
/// Sets the new value into the Parameter Manager. Parameter is persisted after send.
void FactLoader::_valueUpdated(const QVariant& value)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    Q_ASSERT(fact);
    
    Q_ASSERT(_lastSeenComponent != -1);
    Q_ASSERT(_paramMgr);
    Q_ASSERT(_mapFact2ParameterName.contains(fact));
    
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
    
    qCDebug(FactLoaderLog) << "Set parameter" << fact->name() << typedValue;

    _paramMgr->setParameter(_lastSeenComponent, _mapFact2ParameterName[fact], typedValue);
    _paramMgr->sendPendingParameters(true /* persistAfterSend */, false /* forceSend */);
}

// Called when param mgr list is up to date
void FactLoader::_paramMgrParameterListUpToDate(void)
{
    if (!_factsReady) {
        _factsReady = true;
        
        // We don't need this any more
        disconnect(_paramMgr, SIGNAL(parameterListUpToDate()), this, SLOT(_paramMgrParameterListUpToDate()));

        // There may be parameterUpdated signals still in our queue. Flush them out.
        qgcApp()->processEvents();
        
        // We should have all paramters now so we can signal ready
        emit factsReady();
    }
}

void FactLoader::_addMetaDataToFact(Fact* fact)
{
    FactMetaData* metaData = new FactMetaData(this);
    metaData->initFromTypeOnly(fact->type());
}
