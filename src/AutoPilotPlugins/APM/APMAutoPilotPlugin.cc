/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMAutoPilotPlugin.h"
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
#include "APMLightsComponent.h"
#include "APMSubFrameComponent.h"
#include "ESP8266Component.h"

/// This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_ARDUPILOT type.
APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : AutoPilotPlugin(vehicle, parent)
    , _incorrectParameterVersion(false)
    , _airframeComponent(NULL)
    , _cameraComponent(NULL)
    , _lightsComponent(NULL)
    , _subFrameComponent(NULL)
    , _flightModesComponent(NULL)
    , _powerComponent(NULL)
#if 0
        // Temporarily removed, waiting for new command implementation
    , _motorComponent(NULL)
#endif
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
        if (_vehicle->parameterManager()->parametersReady()) {
            _airframeComponent = new APMAirframeComponent(_vehicle, this);
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));

            if ( _vehicle->supportsRadio() ) {
                _radioComponent = new APMRadioComponent(_vehicle, this);
                _radioComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_radioComponent));
            }

            _flightModesComponent = new APMFlightModesComponent(_vehicle, this);
            _flightModesComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_flightModesComponent));

            _sensorsComponent = new APMSensorsComponent(_vehicle, this);
            _sensorsComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));

            _powerComponent = new APMPowerComponent(_vehicle, this);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_powerComponent));

#if 0
    // Temporarily removed, waiting for new command implementation

            if (_vehicle->multiRotor() || _vehicle->vtol()) {
                _motorComponent = new MotorComponent(_vehicle, this);
                _motorComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_motorComponent));
            }
#endif

            _safetyComponent = new APMSafetyComponent(_vehicle, this);
            _safetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_safetyComponent));

            _tuningComponent = new APMTuningComponent(_vehicle, this);
            _tuningComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_tuningComponent));

            _cameraComponent = new APMCameraComponent(_vehicle, this);
            _cameraComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_cameraComponent));

            if (_vehicle->sub()) {
                _lightsComponent = new APMLightsComponent(_vehicle, this);
                _lightsComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_lightsComponent));

                if(_vehicle->firmwareMajorVersion() > 3 || (_vehicle->firmwareMajorVersion() == 3 && _vehicle->firmwareMinorVersion() >= 5)) {
                    _subFrameComponent = new APMSubFrameComponent(_vehicle, this);
                    _subFrameComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue((VehicleComponent*)_subFrameComponent));
                }
            }

            //-- Is there an ESP8266 Connected?
            if(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
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
