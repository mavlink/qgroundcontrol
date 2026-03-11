#include "GenericAutoPilotPlugin.h"
#include "JoystickComponent.h"

GenericAutoPilotPlugin::GenericAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : AutoPilotPlugin(vehicle, parent)
{

}

const QVariantList& GenericAutoPilotPlugin::vehicleComponents()
{
    if (_components.isEmpty()) {
        _joystickComponent = new JoystickComponent(_vehicle, this, this);
        _joystickComponent->setupTriggerSignals();
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_joystickComponent)));
    }
    return _components;
}
