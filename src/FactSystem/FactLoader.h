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

#ifndef FactLoader_h
#define FactLoader_h

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "Fact.h"
#include "UASInterface.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(FactLoaderLog)

/// Connects to Parameter Manager to load/update Facts
///
/// These Facts are available for binding within QML code. For example:
/// @code{.unparsed}
///     TextInput {
///         text: autopilot.parameters["RC_MAP_THROTTLE"].value
///     }
/// @endcode

class FactLoader : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    FactLoader(UASInterface* uas, QObject* parent = NULL);
    
    ~FactLoader();
    
    /// Returns true if the full set of facts are ready
    bool factsAreReady(void) { return _factsReady; }

    /// Returns the fact QVariantMap
    const QVariantMap& factMap(void) { return _mapParameterName2Variant; }
    
signals:
    /// Signalled when the full set of facts are ready
    void factsReady(void);
    
protected:
    /// Base implementation adds generic meta data based on variant type. Derived class can override to provide
    /// more details meta data.
    virtual void _addMetaDataToFact(Fact* fact);
    
private slots:
    void _parameterUpdate(int uas, int component, QString parameterName, int mavType, QVariant value);
    void _valueUpdated(QVariant value);
    void _paramMgrParameterListUpToDate(void);
    
private:
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);

    QMap<Fact*, QString> _mapFact2ParameterName;    ///< Maps from a Fact to a parameter name
    
    int _uasId;             ///< Id for uas which this set of Facts are associated with
    int _lastSeenComponent;
    
    QGCUASParamManagerInterface* _paramMgr;
    
    QVariantMap _mapParameterName2Variant;
    
    bool _factsReady;   ///< All facts received from param mgr
};

#endif