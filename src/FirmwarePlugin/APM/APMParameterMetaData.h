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

#ifndef APMParameterMetaData_H
#define APMParameterMetaData_H

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "FactSystem.h"
#include "AutoPilotPlugin.h"
#include "Vehicle.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataLog)

/// Collection of Parameter Facts for PX4 AutoPilot

typedef QMap<QString, FactMetaData> NametoFactMetaDataMap;

class APMParameterMetaData : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    APMParameterMetaData(QObject* parent = NULL);

    /// Override from ParameterLoader
    virtual QString getDefaultComponentIdParam(void) const { return QString("SYSID_SW_TYPE"); }    
    
    // Overrides from ParameterLoader
    static void addMetaDataToFact(Fact* fact);

private:
    enum {
        XmlStateNone,
        XmlStateFoundVehicles,
        XmlStateFoundLibraries,
        XmlStateFoundParameters,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundParameter,
        XmlStateDone
    };
    

    static void _loadParameterFactMetaData(void);
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk);
    static bool skipParameterXMLBlock(QXmlStreamReader& xml, const QString& blockName);
    static bool parseParameterAttributes(QXmlStreamReader& xml, FactMetaData* metaData);
    static QString mavTypeToString(MAV_TYPE vehicleTypeEnum);

    static bool _parameterMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, NametoFactMetaDataMap> _parametersMap; ///< Maps from a parameter name to FactMetaData
};

#endif
