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

#include "SafetyComponent.h"
#include "PX4RCCalibration.h"
#include "VehicleComponentSummaryItem.h"
#include "QGCQmlWidgetHolder.h"

/// @brief Parameters which signal a change in setupComplete state
static const char* triggerParams[] = { NULL };

SafetyComponent::SafetyComponent(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    PX4Component(uas, autopilot, parent),
    _name(tr("Safety"))
{
}

QString SafetyComponent::name(void) const
{
    return _name;
}

QString SafetyComponent::description(void) const
{
    return tr("The Safety Component is used to setup triggers for Return to Land as well as the settings for Return to Land itself.");
}

QString SafetyComponent::icon(void) const
{
    return ":/files/images/px4/menu/remote.png";
}

bool SafetyComponent::requiresSetup(void) const
{
    return false;
}

bool SafetyComponent::setupComplete(void) const
{
    // FIXME: What aboout invalid settings?
    return true;
}

QString SafetyComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires setup";
    } else {
        stateDescription = "Setup complete";
    }
    return QString(stateDescription);
}

const char** SafetyComponent::setupCompleteChangedTriggerList(void) const
{
    return triggerParams;
}

QStringList SafetyComponent::paramFilterList(void) const
{
    QStringList list;
    
    return list;
}

QWidget* SafetyComponent::setupWidget(void) const
{
    QGCQmlWidgetHolder* holder = new QGCQmlWidgetHolder();
    Q_CHECK_PTR(holder);
    
    holder->setAutoPilot(_autopilot);
    
    holder->setSource(QUrl::fromUserInput("qrc:/qml/SafetyComponent.qml"));
    
    return holder;
}

const QVariantList& SafetyComponent::summaryItems(void)
{
    // FIXME: No summary items yet
#if 0
    if (!_summaryItems.count()) {
        QString name;
        QString state;
        
        // FIXME: Need to pull receiver type from RSSI value
        name = "Receiver type:";
        state = "n/a";

        VehicleComponentSummaryItem* item = new VehicleComponentSummaryItem(name, state, this);
        _summaryItems.append(QVariant::fromValue(item));
        
        static const char* stickParams[] = { "RC_MAP_ROLL", "RC_MAP_PITCH", "RC_MAP_YAW", "RC_MAP_THROTTLE" };
        
        QString summary("Chan ");
        
        bool allSticksMapped = true;
        for (size_t i=0; i<sizeof(stickParams)/sizeof(stickParams[0]); i++) {
            QVariant value;
            
            if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), stickParams[i], value)) {
                if (value.toInt() == 0) {
                    allSticksMapped = false;
                    break;
                } else {
                    if (i != 0) {
                        summary += ",";
                    }
                    summary += value.toString();
                }
            } else {
                // Why is the parameter missing?
                Q_ASSERT(false);
                summary += "?";
            }
        }
        
        if (!allSticksMapped) {
            summary = "Not mapped";
        }

        name = "Ail, Ele, Rud, Throt:";
        state = summary;
        
        item = new VehicleComponentSummaryItem(name, state, this);
        _summaryItems.append(QVariant::fromValue(item));
    }
#endif
    
    return _summaryItems;
}
