#include "SensorsComponentBase.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "FactPanelController.h"

Q_LOGGING_CATEGORY(SensorsComponentBaseLog, "AutoPilotPlugins.Common.Sensors.SensorsComponentBase")

SensorsComponentBase::SensorsComponentBase(Vehicle* vehicle, AutoPilotPlugin* autopilot,
                                           AutoPilotPlugin::KnownVehicleComponent knownComponent,
                                           QObject* parent)
    : VehicleComponent(vehicle, autopilot, knownComponent, parent)
    , _name(tr("Sensors"))
{
}

QString SensorsComponentBase::description() const
{
    return tr("Sensors component is used to calibrate the sensors within your vehicle.");
}

bool SensorsComponentBase::parameterNonZero(const QString& param) const
{
    if (!_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, param)) {
        return false;
    }
    return _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, param)->rawValue().toFloat() != 0.0f;
}

bool SensorsComponentBase::parameterEquals(const QString& param, const QVariant& value) const
{
    if (!_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, param)) {
        return false;
    }
    return _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, param)->rawValue() == value;
}

bool SensorsComponentBase::parameterExists(const QString& param) const
{
    return _vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, param);
}

QVariant SensorsComponentBase::parameterValue(const QString& param) const
{
    if (!_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, param)) {
        return QVariant(0);
    }
    return _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, param)->rawValue();
}
