/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#ifndef APMAirframeLoader_H
#define APMAirframeLoader_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "ParameterLoader.h"
#include "FactSystem.h"
#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// @file APMAirframeLoader.h
///     @author Lorenz Meier <lm@qgroundcontrol.org>

Q_DECLARE_LOGGING_CATEGORY(APMAirframeLoaderLog)

/// Collection of Parameter Facts for PX4 AutoPilot

class APMAirframeLoader : QObject
{
    Q_OBJECT

public:
    /// @param uas Uas which this set of facts is associated with
    APMAirframeLoader(AutoPilotPlugin* autpilot,UASInterface* uas, QObject* parent = NULL);

    static void loadAirframeFactMetaData(void);

private:
    static bool _airframeMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
};

#endif // APMAirframeLoader_H
