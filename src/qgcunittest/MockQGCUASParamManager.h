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

#ifndef MOCKQGCUASPARAMMANAGER_H
#define MOCKQGCUASPARAMMANAGER_H

#include "QGCUASParamManagerInterface.h"

/// @file
///     @brief This is a mock implementation of QGCUASParamManager for writing Unit Tests.
///
///     @author Don Gagne <don@thegagnes.com>


class MockQGCUASParamManager : public QGCUASParamManagerInterface
{
    Q_OBJECT
    
signals:
    // The following QGCSUASParamManagerInterface signals are supported
    void parameterListUpToDate();   // You can connect to this signal, but it will never be emitted
    
public:
    // Implemented QGCSUASParamManager overrides
    virtual bool getParameterValue(int component, const QString& parameter, QVariant& value) const;
    virtual int getDefaultComponentId(void) { return 0; }
    virtual int countOnboardParams(void) { return _mapParams.count(); }
    
public slots:
    // Implemented QGCSUASParamManager overrides
    void setParameter(int component, QString parameterName, QVariant value);
    virtual void setPendingParam(int componentId,  const QString& key,  const QVariant& value, bool forceSend = false)
        { Q_UNUSED(forceSend); setParameter(componentId, key, value); }
    virtual void sendPendingParameters(bool persistAfterSend = false, bool forceSend = false)
        { Q_UNUSED(persistAfterSend); Q_UNUSED(forceSend); }
    
public:
    // MockQGCUASParamManager methods
    MockQGCUASParamManager(void);
    
    /// QMap of parameters, QString key is paremeter name, QVariant value is parameter value
    typedef QMap<QString, QVariant> ParamMap_t;
    
    /// Sets current set of parameters to support calls like getParameterValue
    void setMockParameters(ParamMap_t& map) { _mapParams = map; }
    
    /// Returns the parameters which were set by calls to setParameter calls
    ParamMap_t getMockSetParameters(void) { return _mapParamsSet; }
    /// Clears the set of parameters set by setParameter calls
    void clearMockSetParameters(void) { _mapParamsSet.clear(); }
    
public:
    // Unimplemented QGCUASParamManagerInterface overrides
    virtual QList<int> getComponentForParam(const QString& parameter) const { Q_ASSERT(false); Q_UNUSED(parameter); return _bogusQListInt; }
    virtual void setParamDescriptions(const QMap<QString,QString>& paramDescs) { Q_ASSERT(false); Q_UNUSED(paramDescs); }
    virtual int countPendingParams() { Q_ASSERT(false); return 0; }
    virtual UASParameterDataModel* dataModel() { Q_ASSERT(false); return NULL; }
    
public slots:
    // Unimplemented QGCUASParamManagerInterface overrides
    virtual void requestParameterList() { Q_ASSERT(false); }
    virtual void requestParameterListIfEmpty() { Q_ASSERT(false); }
    virtual void clearAllPendingParams() { Q_ASSERT(false); }
    virtual void requestParameterUpdate(int component, const QString& parameter)
        { Q_ASSERT(false); Q_UNUSED(component); Q_UNUSED(parameter); }
    virtual void writeOnboardParamsToStream(QTextStream &stream, const QString& uasName)
        { Q_ASSERT(false); Q_UNUSED(stream); Q_UNUSED(uasName); }
    virtual void readPendingParamsFromStream(QTextStream &stream) { Q_ASSERT(false); Q_UNUSED(stream); }
    virtual void requestRcCalibrationParamsUpdate() { Q_ASSERT(false); }
    virtual void copyVolatileParamsToPersistent() { Q_ASSERT(false); }
    virtual void copyPersistentParamsToVolatile() { Q_ASSERT(false); }
    
private:
    ParamMap_t          _mapParams;
    ParamMap_t          _mapParamsSet;

    // Bogus variables used for return types of NYI methods
    QList<int>          _bogusQListInt;
};

#endif