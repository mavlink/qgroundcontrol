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

#ifndef PARAMETERLOADER_H
#define PARAMETERLOADER_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "FactSystem.h"
#include "UASInterface.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(ParameterLoaderLog)

/// Connects to Parameter Manager to load/update Facts
class ParameterLoader : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    ParameterLoader(UASInterface* uas, QObject* parent = NULL);
    
    ~ParameterLoader();
    
    /// Returns true if the full set of facts are ready
    bool parametersAreReady(void) { return _parametersReady; }
    
    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(void);
    
    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& name);
    
    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);
    
    /// Returns true if the specifed fact exists
    bool factExists(int             componentId,    ///< fact component, -1=default component
                    const QString&  name);          ///< fact name
    
    /// Returns the specified Fact.
    /// WARNING: Will assert if fact does not exists. If that possibily exists, check for existince first with
    /// factExists.
    Fact* getFact(int               componentId,    ///< fact component, -1=default component
                  const QString&    name);          ///< fact name
    
    /// Return the parameter for which the default component id is derived from. Return an empty
    /// string is this is not available.
    virtual QString getDefaultComponentIdParam(void) const = 0;
    
signals:
    /// Signalled when the full set of facts are ready
    void parametersReady(void);
    
protected:
    /// Base implementation adds generic meta data based on variant type. Derived class can override to provide
    /// more details meta data.
    virtual void _addMetaDataToFact(Fact* fact);
    
private slots:
    void _parameterUpdate(int uas, int componentId, QString parameterName, int mavType, QVariant value);
    void _valueUpdated(const QVariant& value);
    void _paramMgrParameterListUpToDate(void);
    
private:
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);
    int _actualComponentId(int componentId);
    void _determineDefaultComponentId(void);

    /// First mapping is by component id
    /// Second mapping is parameter name, to Fact
    QMap<int, QMap<Fact*, QString> > _mapFact2ParameterName;
    
    int _uasId;             ///< Id for uas which this set of Facts are associated with
    
    QGCUASParamManagerInterface* _paramMgr;
    
    /// First mapping id\s by component id
    /// Second mapping is parameter name, to Fact* in QVariant
    QMap<int, QVariantMap> _mapParameterName2Variant;
    
    bool _parametersReady;   ///< All params received from param mgr
    int _defaultComponentId;
    QString _defaultComponentIdParam;
};

#endif