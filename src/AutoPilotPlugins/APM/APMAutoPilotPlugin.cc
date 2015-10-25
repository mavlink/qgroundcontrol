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

#include "APMAutoPilotPlugin.h"
#include "AutoPilotPluginManager.h"
#include "QGCMessageBox.h"
#include "UAS.h"
#include "FirmwarePlugin/APM/APMParameterLoader.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/APMFirmwarePlugin.h"  // FIXME: Hack

/// This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_ARDUPILOT type.
APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent) :
    AutoPilotPlugin(vehicle, parent),
    _incorrectParameterVersion(false)
{
    Q_ASSERT(vehicle);

    // This kicks off parameter load
    _firmwarePlugin->getParameterLoader(this, vehicle);
}

APMAutoPilotPlugin::~APMAutoPilotPlugin()
{

}

void APMAutoPilotPlugin::clearStaticData(void)
{
    APMParameterLoader::clearStaticData();
}

const QVariantList& APMAutoPilotPlugin::vehicleComponents(void)
{
    static const QVariantList emptyList;

    return emptyList;
}

/// This will perform various checks prior to signalling that the plug in ready
void APMAutoPilotPlugin::_parametersReadyPreChecks(bool missingParameters)
{
#if 0
    I believe APM has parameter version stamp, we should check that
    
    // Check for older parameter version set
    // FIXME: Firmware is moving to version stamp parameter set. Once that is complete the version stamp
    // should be used instead.
    if (parameterExists(FactSystem::defaultComponentId, "SENS_GYRO_XOFF")) {
        _incorrectParameterVersion = true;
        QGCMessageBox::warning("Setup", "This version of GroundControl can only perform vehicle setup on a newer version of firmware. "
										"Please perform a Firmware Upgrade if you wish to use Vehicle Setup.");
	}
#endif
	
    _parametersReady = true;
    _missingParameters = missingParameters;
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
