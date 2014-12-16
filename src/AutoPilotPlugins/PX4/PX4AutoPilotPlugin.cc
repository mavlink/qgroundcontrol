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

#include "PX4AutoPilotPlugin.h"
#include "AirframeComponent.h"
#include "RadioComponent.h"
#include "SensorsComponent.h"
#include "FlightModesComponent.h"
#include "AutoPilotPluginManager.h"
#include "UASManager.h"
#include "QGCUASParamManagerInterface.h"
#include "PX4ParameterFacts.h"

/// @file
///     @brief This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_PX4 type.
///     @author Don Gagne <don@thegagnes.com>


enum PX4_CUSTOM_MAIN_MODE {
    PX4_CUSTOM_MAIN_MODE_MANUAL = 1,
    PX4_CUSTOM_MAIN_MODE_ALTCTL,
    PX4_CUSTOM_MAIN_MODE_POSCTL,
    PX4_CUSTOM_MAIN_MODE_AUTO,
    PX4_CUSTOM_MAIN_MODE_ACRO,
    PX4_CUSTOM_MAIN_MODE_OFFBOARD,
};

enum PX4_CUSTOM_SUB_MODE_AUTO {
    PX4_CUSTOM_SUB_MODE_AUTO_READY = 1,
    PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,
    PX4_CUSTOM_SUB_MODE_AUTO_LOITER,
    PX4_CUSTOM_SUB_MODE_AUTO_MISSION,
    PX4_CUSTOM_SUB_MODE_AUTO_RTL,
    PX4_CUSTOM_SUB_MODE_AUTO_LAND,
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS
};

union px4_custom_mode {
    struct {
        uint16_t reserved;
        uint8_t main_mode;
        uint8_t sub_mode;
    };
    uint32_t data;
    float data_float;
};

PX4AutoPilotPlugin::PX4AutoPilotPlugin(UASInterface* uas, QObject* parent) :
    AutoPilotPlugin(parent),
    _uas(uas),
    _parameterFacts(NULL)
{
    Q_ASSERT(uas);
    
    _parameterFacts = new PX4ParameterFacts(uas, this);
    Q_CHECK_PTR(_parameterFacts);
    
    connect(_parameterFacts, &PX4ParameterFacts::factsReady, this, &PX4AutoPilotPlugin::pluginReady);
    
    PX4ParameterFacts::loadParameterFactMetaData();
}

PX4AutoPilotPlugin::~PX4AutoPilotPlugin()
{
    delete _parameterFacts;
    PX4ParameterFacts::deleteParameterFactMetaData();
}

QList<AutoPilotPluginManager::FullMode_t> PX4AutoPilotPlugin::getModes(void)
{
    union px4_custom_mode                       px4_cm;
    AutoPilotPluginManager::FullMode_t          fullMode;
    QList<AutoPilotPluginManager::FullMode_t>   modeList;
    
    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_MANUAL;
    fullMode.baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED;
    fullMode.customMode = px4_cm.data;
    modeList << fullMode;
    
    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_ALTCTL;
    fullMode.baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED;
    fullMode.customMode = px4_cm.data;
    modeList << fullMode;
    
    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_POSCTL;
    fullMode.baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED;
    fullMode.customMode = px4_cm.data;
    modeList << fullMode;
    
    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_AUTO;
    fullMode.baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED;
    fullMode.customMode = px4_cm.data;
    modeList << fullMode;
    
    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_OFFBOARD;
    fullMode.baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED;
    fullMode.customMode = px4_cm.data;
    modeList << fullMode;
    
    return modeList;
}

QString PX4AutoPilotPlugin::getShortModeText(uint8_t baseMode, uint32_t customMode)
{
    QString mode;
    
    Q_ASSERT(baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED);
    
    if (baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        union px4_custom_mode px4_mode;
        px4_mode.data = customMode;
        
        if (px4_mode.main_mode == PX4_CUSTOM_MAIN_MODE_MANUAL) {
            mode = "|MANUAL";
        } else if (px4_mode.main_mode == PX4_CUSTOM_MAIN_MODE_ALTCTL) {
            mode = "|ALTCTL";
        } else if (px4_mode.main_mode == PX4_CUSTOM_MAIN_MODE_POSCTL) {
            mode = "|POSCTL";
        } else if (px4_mode.main_mode == PX4_CUSTOM_MAIN_MODE_AUTO) {
            mode = "|AUTO";
            if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_READY) {
                mode += "|READY";
            } else if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF) {
                mode += "|TAKEOFF";
            } else if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_LOITER) {
                mode += "|LOITER";
            } else if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION) {
                mode += "|MISSION";
            } else if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_RTL) {
                mode += "|RTL";
            } else if (px4_mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_LAND) {
                mode += "|LAND";
            }
        } else if (px4_mode.main_mode == PX4_CUSTOM_MAIN_MODE_OFFBOARD) {
            mode = "|OFFBOARD";
        }
    } else {
        // use base_mode - not autopilot-specific
        if (baseMode == 0) {
            mode = "|PREFLIGHT";
        } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_AUTO) {
            mode = "|AUTO";
        } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_MANUAL) {
            mode = "|MANUAL";
            if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_GUIDED) {
                mode += "|GUIDED";
            } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_STABILIZE) {
                mode += "|STABILIZED";
            }
        }

    }
    
    return mode;
}

void PX4AutoPilotPlugin::clearStaticData(void)
{
    PX4ParameterFacts::clearStaticData();
}

bool PX4AutoPilotPlugin::pluginIsReady(void) const
{
    return _parameterFacts->factsAreReady();
}

const QVariantList& PX4AutoPilotPlugin::components(void)
{
    if (_components.count() == 0) {
        VehicleComponent* component;

        Q_ASSERT(_uas);
        
        component = new AirframeComponent(_uas);
        Q_CHECK_PTR(component);
        _components.append(QVariant::fromValue(component));
        
        component = new RadioComponent(_uas);
        Q_CHECK_PTR(component);
        _components.append(QVariant::fromValue(component));
        
        component = new FlightModesComponent(_uas);
        Q_CHECK_PTR(component);
        _components.append(QVariant::fromValue(component));
        
        component = new SensorsComponent(_uas);
        Q_CHECK_PTR(component);
        _components.append(QVariant::fromValue(component));
    }
    
    return _components;
}

const QVariantMap& PX4AutoPilotPlugin::parameters(void)
{
    return _parameterFacts->factMap();
}

QUrl PX4AutoPilotPlugin::setupBackgroundImage(void)
{
    return QUrl::fromUserInput("qrc:/qml/octo_x.png");
}
