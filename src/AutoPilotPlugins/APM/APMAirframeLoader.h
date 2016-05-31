/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
