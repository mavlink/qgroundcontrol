/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMAutoPilotPlugin.h"
#include "AutoPilotPluginManager.h"
#include "UAS.h"
#include "FirmwarePlugin/APM/APMParameterMetaData.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/APMFirmwarePlugin.h"  // FIXME: Hack
#include "FirmwarePlugin/APM/ArduCopterFirmwarePlugin.h"
#include "VehicleComponent.h"
#include "APMAirframeComponent.h"
#include "APMAirframeComponentAirframes.h"
#include "APMAirframeComponentController.h"
#include "APMAirframeLoader.h"
#include "APMFlightModesComponent.h"
#include "APMRadioComponent.h"
#include "APMSafetyComponent.h"
#include "APMTuningComponent.h"
#include "APMSensorsComponent.h"
#include "APMPowerComponent.h"
#include "MotorComponent.h"
#include "APMCameraComponent.h"
#include "ESP8266Component.h"

/// This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_ARDUPILOT type.
APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : AutoPilotPlugin(vehicle, parent)
    , _incorrectParameterVersion(false)
    , _airframeComponent(NULL)
    , _cameraComponent(NULL)
    , _flightModesComponent(NULL)
    , _powerComponent(NULL)
    , _motorComponent(NULL)
    , _radioComponent(NULL)
    , _safetyComponent(NULL)
    , _sensorsComponent(NULL)
    , _tuningComponent(NULL)
    , _airframeFacts(new APMAirframeLoader(this, vehicle->uas(), this))
    , _esp8266Component(NULL)
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
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));

            _radioComponent = new APMRadioComponent(_vehicle, this);
            _radioComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_radioComponent));

            _flightModesComponent = new APMFlightModesComponent(_vehicle, this);
            _flightModesComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_flightModesComponent));

            _sensorsComponent = new APMSensorsComponent(_vehicle, this);
            _sensorsComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));

            _powerComponent = new APMPowerComponent(_vehicle, this);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_powerComponent));

            if (_vehicle->multiRotor() || _vehicle->vtol()) {
                _motorComponent = new MotorComponent(_vehicle, this);
                _motorComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_motorComponent));
            }

            _safetyComponent = new APMSafetyComponent(_vehicle, this);
            _safetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_safetyComponent));

            _tuningComponent = new APMTuningComponent(_vehicle, this);
            _tuningComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_tuningComponent));

            _cameraComponent = new APMCameraComponent(_vehicle, this);
            _cameraComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_cameraComponent));

            //-- Is there an ESP8266 Connected?
            if(factExists(FactSystem::ParameterProvider, MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
                _esp8266Component = new ESP8266Component(_vehicle, this);
                _esp8266Component->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_esp8266Component));
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
