#include "CustomAutoPilotPlugin.h"
#include "ParameterManager.h"
#include "QGCCorePlugin.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

CustomAutoPilotPlugin::CustomAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : PX4AutoPilotPlugin(vehicle, parent)
{
    // Whenever we go on/out of advanced mode the available list of settings pages will change
    (void) connect(QGCCorePlugin::instance(), &QGCCorePlugin::showAdvancedUIChanged, this, &CustomAutoPilotPlugin::_advancedChanged);
}

void CustomAutoPilotPlugin::_advancedChanged(bool)
{
    _components.clear();
    emit vehicleComponentsChanged();
}

const QVariantList &CustomAutoPilotPlugin::vehicleComponents()
{
    if (!_components.isEmpty() || _incorrectParameterVersion) {
        return _components;
    }

    const QVariantList &baseComponents =
        PX4AutoPilotPlugin::vehicleComponents();

    const bool showAdvanced =
        QGCCorePlugin::instance()->showAdvancedUI();

    for (const QVariant &componentVariant : baseComponents) {
        VehicleComponent *component =
            componentVariant.value<VehicleComponent *>();

        if (!component) {
            continue;
        }

        const QString name = component->name();

        if (component->KnownVehicleComponent()
            == AutoPilotPlugin::KnownSafetyVehicleComponent) {
            _components.append(componentVariant);
            continue;
        }

        if (name == QStringLiteral("Actuators")) {
            _components.append(componentVariant);
            continue;
        }

        if (showAdvanced) {
            switch (component->KnownVehicleComponent()) {
            case AutoPilotPlugin::KnownRadioVehicleComponent:
            case AutoPilotPlugin::KnownFlightModesVehicleComponent:
            case AutoPilotPlugin::KnownSensorsVehicleComponent:
            case AutoPilotPlugin::KnownPowerVehicleComponent:
            case AutoPilotPlugin::KnownJoystickVehicleComponent:
                _components.append(componentVariant);
                break;

            default:
                break;
            }
        }
    }

    return _components;
}