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
#include "FirmwarePlugin/APM/APMParameterMetaData.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/APMFirmwarePlugin.h"  // FIXME: Hack
#include "APMComponent.h"
#include "APMAirframeComponent.h"
#include "APMAirframeComponentAirframes.h"
#include "APMAirframeComponentController.h"
#include "APMAirframeLoader.h"
#include "apmremoteparamscontroller.h"

/// This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_ARDUPILOT type.
APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : AutoPilotPlugin(vehicle, parent)
    , _incorrectParameterVersion(false)
    , _airframeComponent(NULL)
{
    Q_ASSERT(vehicle);
    _airframeFacts = new APMAirframeLoader(this, vehicle->uas(), this);
    APMAirframeLoader::loadAirframeFactMetaData();

    qmlRegisterType<APMRemoteParamsController>  ("QGroundControl.Controllers", 1, 0, "APMRemoteParamsController");
}

APMAutoPilotPlugin::~APMAutoPilotPlugin()
{

}

const QVariantList& APMAutoPilotPlugin::vehicleComponents(void)
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        Q_ASSERT(_vehicle);

        if (parametersReady()) {
            _airframeComponent = new APMAirframeComponent(_vehicle, this);
            Q_CHECK_PTR(_airframeComponent);
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));
        } else {
            qWarning() << "Call to vehicleCompenents prior to parametersReady";
        }
    }

    return _components;
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
    _missingParameters = false; missingParameters;
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
