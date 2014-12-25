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

#ifndef PX4AUTOPILOT_H
#define PX4AUTOPILOT_H

#include "AutoPilotPlugin.h"
#include "AutoPilotPluginManager.h"
#include "UASInterface.h"
#include "PX4ParameterFacts.h"
#include "AirframeComponent.h"
#include "RadioComponent.h"
#include "FlightModesComponent.h"
#include "SensorsComponent.h"
#include "SafetyComponent.h"

#include <QImage>

/// @file
///     @brief This is the PX4 specific implementation of the AutoPilot class.
///     @author Don Gagne <don@thegagnes.com>

class PX4AutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    PX4AutoPilotPlugin(UASInterface* uas, QObject* parent);
    ~PX4AutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    virtual bool pluginIsReady(void) const;
    virtual const QVariantList& components(void);
    virtual const QVariantMap& parameters(void);
    virtual QUrl setupBackgroundImage(void);

    static QList<AutoPilotPluginManager::FullMode_t> getModes(void);
    static QString getShortModeText(uint8_t baseMode, uint32_t customMode);
    static void clearStaticData(void);
    
    // These methods should only be used by objects within the plugin
    AirframeComponent* airframeComponent(void) { return _airframeComponent; }
    RadioComponent* radioComponent(void) { return _radioComponent; }
    FlightModesComponent* flightModesComponent(void) { return _flightModesComponent; }
    SensorsComponent* sensorsComponent(void) { return _sensorsComponent; }
    SafetyComponent* safetyComponent(void) { return _safetyComponent; }
    
private:
    UASInterface*           _uas;
    PX4ParameterFacts*      _parameterFacts;
    QVariantList            _components;
    AirframeComponent*      _airframeComponent;
    RadioComponent*         _radioComponent;
    FlightModesComponent*   _flightModesComponent;
    SensorsComponent*       _sensorsComponent;
    SafetyComponent*        _safetyComponent;
};

#endif
