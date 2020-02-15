/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef PX4AIRFRAMELOADER_H
#define PX4AIRFRAMELOADER_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "ParameterManager.h"
#include "FactSystem.h"
#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// @file PX4AirframeLoader.h
///     @author Lorenz Meier <lm@qgroundcontrol.org>

Q_DECLARE_LOGGING_CATEGORY(PX4AirframeLoaderLog)

/// Collection of Parameter Facts for PX4 AutoPilot

class PX4AirframeLoader : QObject
{
    Q_OBJECT

public:
    /// @param uas Uas which this set of facts is associated with
    PX4AirframeLoader(AutoPilotPlugin* autpilot,UASInterface* uas, QObject* parent = nullptr);

    static void loadAirframeMetaData(void);

    /// @return Location of PX4 airframe fact meta data file
    static QString aiframeMetaDataFile(void);

private:
    enum {
        XmlStateNone,
        XmlStateFoundAirframes,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundAirframe,
        XmlStateDone
    };

    static bool _airframeMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
};

#endif // PX4AIRFRAMELOADER_H
