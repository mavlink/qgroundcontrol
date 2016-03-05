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

#ifndef PX4ParameterMetaData_H
#define PX4ParameterMetaData_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "FactSystem.h"
#include "AutoPilotPlugin.h"
#include "Vehicle.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(PX4ParameterMetaDataLog)

/// Loads and holds parameter fact meta data for PX4 stack
class PX4ParameterMetaData : public QObject
{
    Q_OBJECT
    
public:
    PX4ParameterMetaData(void);

    void loadParameterFactMetaDataFile  (const QString& metaDataFile);
    void addMetaDataToFact              (Fact* fact, MAV_TYPE vehicleType);

    static void getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion);

private:
    enum {
        XmlStateNone,
        XmlStateFoundParameters,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundParameter,
        XmlStateDone
    };    

    QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk);

    bool _parameterMetaDataLoaded;   ///< true: parameter meta data already loaded
    QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
};

#endif
