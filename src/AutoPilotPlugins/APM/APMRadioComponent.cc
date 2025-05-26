/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMRadioComponent.h"
#include "ParameterManager.h"
#include "Fact.h"
#include "Vehicle.h"

APMRadioComponent::APMRadioComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownRadioVehicleComponent, parent)
{
    for (const QString &mapParam : _mapParams) {
        Fact *const fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, mapParam);
        (void) connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }

    _connectSetupTriggers();
}

bool APMRadioComponent::setupComplete() const
{
    // The best we can do to detect the need for a radio calibration is look for attitude
    // controls to be mapped as well as all attitude control rc min/max/trim still at defaults.
    QList<int> mapValues;

    // First check for all attitude controls mapped
    for (int i = 0; i < _mapParams.count(); i++) {
        mapValues << _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, _mapParams[i])->rawValue().toInt();
        if (mapValues[i] <= 0) {
            return false;
        }
    }

    // Next check RC#_MIN/MAX/TRIM all at defaults
    for (const QString &mapParam : _mapParams) {
        const int channel = _vehicle->parameterManager()->getParameter(-1, mapParam)->rawValue().toInt();
        if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MIN").arg(channel))->rawValue().toInt() != 1100) {
            return true;
        }
        if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MAX").arg(channel))->rawValue().toInt() != 1900) {
            return true;
        }
        if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_TRIM").arg(channel))->rawValue().toInt() != 1500) {
            return true;
        }
    }

    return false;
}

void APMRadioComponent::_connectSetupTriggers()
{
    for (Fact *fact : _triggerFacts) {
        (void) disconnect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }
    _triggerFacts.clear();

    // Get the channels for attitude controls and connect to those values for triggers
    for (const QString &mapParam : _mapParams) {
        const int channel = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, mapParam)->rawValue().toInt();

        Fact *fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MIN").arg(channel));
        _triggerFacts << fact;
        (void) connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);

        fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_MAX").arg(channel));
        _triggerFacts << fact;
        (void) connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);

        fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("RC%1_TRIM").arg(channel));
        _triggerFacts << fact;
        (void) connect(fact, &Fact::valueChanged, this, &APMRadioComponent::_triggerChanged);
    }
}

void APMRadioComponent::_triggerChanged()
{
    emit setupCompleteChanged();

    // Control mapping may have changed so we need to reset triggers
    _connectSetupTriggers();
}
