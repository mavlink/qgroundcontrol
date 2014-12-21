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

#include <QFile>
#include <QDebug>

Q_LOGGING_CATEGORY(FactLoaderLog, "FactLoaderLog")

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
    
    // We track parameters changes to keep Facts up to date. UASInterface::parameterChanged has multiple overrides so we need to
    // use SIGNAL/SLOT style connect
    connect(uas, SIGNAL(parameterChanged(int, int, QString, QVariant)), this, SLOT(_parameterChanged(int, int, QString, QVariant)));
}

FactLoader::~FactLoader()
{
    foreach(Fact* fact, _mapFact2ParameterName.keys()) {
        delete fact;
    }
    _mapParameterName2Variant.clear();
    _mapFact2ParameterName.clear();
}

/// Connected to QGCUASParmManager::parameterChanged
///
/// When a new parameter is seen it is added to the system. If the parameter is already known it is updated.
void FactLoader::_parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    // Is this for our uas?
    if (uas != _uasId) {
        return;
    }
    
    if (_lastSeenComponent == -1) {
        _lastSeenComponent = component;
    } else {
        // Code cannot handle parameters coming form different components yets
        Q_ASSERT(component == _lastSeenComponent);
    }
    
    if (!_mapParameterName2Variant.contains(parameterName)) {
        Fact* fact = new Fact(this);
        
        _mapParameterName2Variant[parameterName] = QVariant::fromValue(fact);
        _mapFact2ParameterName[fact] = parameterName;
        
        // We need to know when the fact changes from QML so that we can send the new value to the parameter manager
        connect(fact, &Fact::_containerValueChanged, this, &FactLoader::_valueUpdated);

        qCDebug(FactLoaderLog) << "Adding new fact" << parameterName;
    }
    
    Q_ASSERT(_mapParameterName2Variant.contains(parameterName));
    
    qCDebug(FactLoaderLog) << "Updating fact value" << parameterName << value;
    
    Fact* fact = _mapParameterName2Variant[parameterName].value<Fact*>();
    Q_ASSERT(fact);
    fact->_containerSetValue(value);
}

/// Connected to Fact::valueUpdated
///
/// Sets the new value into the Parameter Manager. Paramter is persisted after send.
void FactLoader::_valueUpdated(QVariant value)
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
            typedValue = QVariant(value.value<int>());
            
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue = QVariant(value.value<uint>());
            break;
            
        case FactMetaData::valueTypeFloat:
            typedValue = QVariant(value.toFloat());
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue = QVariant(value.toDouble());
            break;
    }
    
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
