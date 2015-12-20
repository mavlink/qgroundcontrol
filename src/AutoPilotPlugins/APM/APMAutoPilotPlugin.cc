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
#include "UAS.h"
#include "FirmwarePlugin/APM/APMParameterMetaData.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/APMFirmwarePlugin.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/ArduCopterFirmwarePlugin.h"
#include "APMComponent.h"
#include "APMAirframeComponent.h"
#include "APMAirframeComponentAirframes.h"
#include "APMAirframeComponentController.h"
#include "APMAirframeLoader.h"
#include "APMRemoteParamsDownloader.h"
#include "APMFlightModesComponent.h"
#include "APMRadioComponent.h"
#include "APMSafetyComponent.h"
#include "APMTuningComponent.h"
#include "APMSensorsComponent.h"
#include "APMPowerComponent.h"

/// This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_ARDUPILOT type.
APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : AutoPilotPlugin(vehicle, parent)
    , _incorrectParameterVersion(false)
    , _airframeComponent(NULL)
    , _flightModesComponent(NULL)
    , _powerComponent(NULL)
    , _radioComponent(NULL)
    , _safetyComponent(NULL)
    , _sensorsComponent(NULL)
    , _tuningComponent(NULL)
    , _airframeFacts(new APMAirframeLoader(this, vehicle->uas(), this))
{
    APMAirframeLoader::loadAirframeFactMetaData();
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
            if(_airframeComponent) {
                _airframeComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));
            } else {
                qWarning() << "new APMAirframeComponent failed";
            }

            _flightModesComponent = new APMFlightModesComponent(_vehicle, this);
            if (_flightModesComponent) {
                _flightModesComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_flightModesComponent));
            } else {
                qWarning() << "new APMFlightModesComponent failed";
            }

            _powerComponent = new APMPowerComponent(_vehicle, this);
            if (_powerComponent) {
                _powerComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_powerComponent));
            } else {
                qWarning() << "new APMPowerComponent failed";
            }

            _radioComponent = new APMRadioComponent(_vehicle, this);
            if (_radioComponent) {
                _radioComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_radioComponent));
            } else {
                qWarning() << "new APMRadioComponent failed";
            }

            _sensorsComponent = new APMSensorsComponent(_vehicle, this);
            if (_sensorsComponent) {
                _sensorsComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));
            } else {
                qWarning() << "new APMSensorsComponent failed";
            }

            _safetyComponent = new APMSafetyComponent(_vehicle, this);
            if (_safetyComponent) {
                _safetyComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_safetyComponent));
            } else {
                qWarning() << "new APMSafetyComponent failed";
            }

            _tuningComponent = new APMTuningComponent(_vehicle, this);
            if (_tuningComponent) {
                _tuningComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_tuningComponent));
            } else {
                qWarning() << "new APMTuningComponent failed";
            }
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
        qgcApp()->showMessage("This version of GroundControl can only perform vehicle setup on a newer version of firmware. "
                              "Please perform a Firmware Upgrade if you wish to use Vehicle Setup.");
    }
#endif
    Q_UNUSED(missingParameters);
    _parametersReady = true;
    _missingParameters = false; // we apply only the parameters that do exists on the FactSystem.
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
