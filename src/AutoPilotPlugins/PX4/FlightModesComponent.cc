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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FlightModesComponent.h"
#include "FlightModeConfig.h"
#include "VehicleComponentSummaryItem.h"

/// @brief Parameters which signal a change in setupComplete state
static const char* triggerParams[] = { "RC_MAP_MODE_SW", NULL };

struct SwitchListItem {
    const char* param;
    const char* name;
};

/// @brief Used to translate from RC_MAP_* parameters name to user string
static const SwitchListItem switchList[] = {
    { "RC_MAP_MODE_SW",     "Mode Switch:" },                // First entry must be mode switch
    { "RC_MAP_POSCTL_SW",   "Position Control Switch:" },
    { "RC_MAP_LOITER_SW",   "Loiter Switch:" },
    { "RC_MAP_RETURN_SW",   "Return Switch:" }
};
static const size_t cSwitchList = sizeof(switchList) / sizeof(switchList[0]);

FlightModesComponent::FlightModesComponent(UASInterface* uas, QObject* parent) :
    PX4Component(uas, parent),
    _name(tr("Flight Modes"))
{
}

QString FlightModesComponent::name(void) const
{
    return _name;
}

QString FlightModesComponent::description(void) const
{
    // FIXME: Better text
    return tr("The Flight Modes Component is used to set the switches associated with Flight Modes. "
              "At a minimum the Main Mode Switch must be assigned prior to flight.");
}

QString FlightModesComponent::icon(void) const
{
    return ":/files/images/px4/menu/toggle_switch.png";
}

bool FlightModesComponent::requiresSetup(void) const
{
    return true;
}

bool FlightModesComponent::setupComplete(void) const
{
    QVariant value;
    if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), triggerParams[0], value)) {
        return value.toInt() != 0;
    } else {
        Q_ASSERT(false);
        return false;
    }
}

QString FlightModesComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

const char** FlightModesComponent::setupCompleteChangedTriggerList(void) const
{
    return triggerParams;
}

QStringList FlightModesComponent::paramFilterList(void) const
{
    QStringList list;
    
    for (size_t i=0; i<cSwitchList; i++) {
        list << switchList[i].param;
    }

    return list;
}

QWidget* FlightModesComponent::setupWidget(void) const
{
    return new FlightModeConfig();
}

const QVariantList& FlightModesComponent::summaryItems(void)
{
    if (!_summaryItems.count()) {

        // Create summary items for each mode switch
        
        for (size_t i=0; i<cSwitchList; i++) {
            QString name;
            QString state;
            QVariant value;
            
            name = switchList[i].name;
            
            if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), switchList[i].param, value)) {
                int chan = value.toInt();
                
                if (chan == 0) {
                    // Switch is not mapped
                    if (i == 0) {
                        // Mode switch is required
                        Q_ASSERT(strcmp(switchList[0].param, "RC_MAP_MODE_SW") == 0);
                        state = "Setup required";
                    } else {
                        state = "None";
                    }
                } else {
                    state = tr("Chan %1").arg(chan);
                }
            } else {
                // Why is the parameter missing?
                Q_ASSERT(false);
                state = "Unknown";
            }
            
            VehicleComponentSummaryItem* item = new VehicleComponentSummaryItem(name, state, this);
            _summaryItems.append(QVariant::fromValue(item));
        }
    }
    
    return _summaryItems;
}
