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

#include "SensorsComponent.h"
#include "QGCPX4SensorCalibration.h"
#include "VehicleComponentSummaryItem.h"

// These two list must be kept in sync

/// @brief Parameters which signal a change in setupComplete state
static const char* triggerParams[] = {  "SENS_MAG_XOFF", "SENS_GYRO_XOFF", "SENS_ACC_XOFF", "SENS_DPRES_OFF", NULL };

/// @brief Used to translate from parameter name to user string
static const char* triggerSensors[] = { "Compass",       "Gyro",           "Acceleromter",  "Airspeed",       NULL };

SensorsComponent::SensorsComponent(UASInterface* uas, QObject* parent) :
    PX4Component(uas, parent),
    _name(tr("Sensors"))
{
}

QString SensorsComponent::name(void) const
{
    return _name;
}

QString SensorsComponent::description(void) const
{
    return tr("The Sensors Component allows you to calibrate the sensors within your vehicle. "
              "Prior to flight you must calibrate the Magnetometer, Gyroscope and Accelerometer.");
}

QString SensorsComponent::icon(void) const
{
    return ":/files/images/px4/menu/sensors.png";
}

bool SensorsComponent::requiresSetup(void) const
{
    return true;
}

bool SensorsComponent::setupComplete(void) const
{
    const char** prgTriggers = setupCompleteChangedTriggerList();
    Q_ASSERT(prgTriggers);
    
    while (*prgTriggers != NULL) {
        QVariant value;
        
        if (!_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), *prgTriggers, value)) {
            Q_ASSERT(false);
            return false;
        }
        
        if (value.toFloat() == 0.0f) {
            return false;
        }
        
        prgTriggers++;
    }

    return true;
}

QString SensorsComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

const char** SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    return triggerParams;
}

QStringList SensorsComponent::paramFilterList(void) const
{
    QStringList list;
    
    list << "SENS_*";
    
    return list;
}

QWidget* SensorsComponent::setupWidget(void) const
{
    return new QGCPX4SensorCalibration;
}

const QVariantList& SensorsComponent::summaryItems(void)
{
    if (!_summaryItems.count()) {
        QString name;
        QString state;
        
        // Summary item for each Sensor
        
        int i = 0;
        while (triggerParams[i] != NULL) {
            QVariant value;
            
            name = tr("%1:").arg(triggerSensors[i]);
            
            if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), triggerParams[i], value)) {
                if (value.toFloat() == 0.0f) {
                    state = "Setup required";
                } else {
                    state = "Ready";
                }
            } else {
                // Why is the parameter missing?
                Q_ASSERT(false);
            }

            VehicleComponentSummaryItem* item = new VehicleComponentSummaryItem(name, state, this);
            _summaryItems.append(QVariant::fromValue(item));
            
            i++;
        }
        
        // Summary item for each orientation param
        
        static const char* orientationSensors[] = { "Autopilot orientation:", "Compass orientation:" };
        static const char* orientationParams[] = {  "SENS_BOARD_ROT",         "SENS_EXT_MAG_ROT" };
        static const size_t cOrientationSensors = sizeof(orientationSensors)/sizeof(orientationSensors[0]);
        
        static const char* orientationValues[] = {
                "Line of flight",
                "Yaw:45",
                "Yaw:90",
                "Yaw:135",
                "Yaw:180",
                "Yaw:225",
                "Yaw:270",
                "Yaw:315",
                "Roll:180",
                "Roll:180 Yaw:45",
                "Roll:180 Yaw:90",
                "Roll:180 Yaw:135",
                "Pitch:180",
                "Roll:180 Yaw:225",
                "Roll:180 Yaw:270",
                "Roll:180 Yaw:315",
                "Roll:90",
                "Roll:90 Yaw:45",
                "Roll:90 Yaw:90",
                "Roll:90 Yaw:135",
                "Roll:270",
                "Roll:270 Yaw:45",
                "Roll:270 Yaw:90",
                "Roll:270 Yaw:135",
                "Pitch:90",
                "Pitch:270",
                "Pitch:180",
                "Pitch:180 Yaw:90",
                "Pitch:180 Yaw:270",
                "Roll:90 Pitch:90",
                "Roll:180 Pitch:90",
                "Roll:270 Pitch:90",
                "Roll:90 Pitch:180",
                "Roll:270 Pitch:180",
                "Roll:90 Pitch:270",
                "Roll:180 Pitch:270",
                "Roll:270 Pitch:270",
                "Roll:90 Pitch:180 Yaw:90",
                "Roll:90 Yaw:270"
        };
        static const size_t cOrientationValues = sizeof(orientationValues)/sizeof(orientationValues[0]);
        
        for (size_t i=0; i<cOrientationSensors; i++) {
            QVariant value;
            
            name = orientationSensors[i];
            
            if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), orientationParams[i], value)) {
                int index = value.toInt();
                if (index < 0 || index >= (int)cOrientationValues) {
                    state = "Setup required";
                } else {
                    state = orientationValues[index];
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
