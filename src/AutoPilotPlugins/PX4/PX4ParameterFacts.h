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

#ifndef PX4ParameterFacts_h
#define PX4ParameterFacts_h

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "Fact.h"
#include "UASInterface.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

// FIXME: This file should be auto-generated from the Parameter XML file.

Q_DECLARE_LOGGING_CATEGORY(PX4ParameterFactsLog)
Q_DECLARE_LOGGING_CATEGORY(PX4ParameterFactsMetaDataLog)

/// Collection of Parameter Facts for PX4 AutoPilot
///
/// These Facts are available for binding within QML code. For example:
/// @code{.unparsed}
///     TextInput {
///         text: parameters["RC_MAP_THROTTLE"].value
///     }
/// @endcode

class PX4ParameterFacts : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    PX4ParameterFacts(UASInterface* uas, QObject* parent = NULL);
    
    ~PX4ParameterFacts();
    
    /// Returns true if the full set of facts are ready
    bool factsAreReady(void) { return _factsReady; }

    /// Returns the fact QVariantMap
    const QVariantMap& factMap(void) { return _mapParameterName2Variant; }
    
    static void loadParameterFactMetaData(void);
    static void deleteParameterFactMetaData(void);
    static void clearStaticData(void);
    
signals:
    /// Signalled when the full set of facts are ready
    void factsReady(void);
    
private slots:
    void _parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void _valueUpdated(QVariant value);
    void _paramMgrParameterListUpToDate(void);
    
private:
    static FactMetaData* _parseParameter(QXmlStreamReader& xml, const QString& group);
    static void _initMetaData(FactMetaData* metaData);
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);

    QMap<Fact*, QString> _mapFact2ParameterName;    ///< Maps from a Fact to a parameter name
    
    static bool _parameterMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
    
    int _uasId;             ///< Id for uas which this set of Facts are associated with
    int _lastSeenComponent;
    
    QGCUASParamManagerInterface* _paramMgr;
    
    QVariantMap _mapParameterName2Variant;
    
    bool _factsReady;   ///< All facts received from param mgr
};

#endif