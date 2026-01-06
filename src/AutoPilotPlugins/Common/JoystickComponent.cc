/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickComponent.h"

#include "Joystick.h"
#include "JoystickManager.h"

JoystickComponent::JoystickComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownJoystickVehicleComponent, parent)
    , _name(tr("Joystick"))
{
    (void) connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &VehicleComponent::setupCompleteChanged);

    _activeJoystickChanged(JoystickManager::instance()->activeJoystick());
}

QString JoystickComponent::description() const
{
    return tr("Configure joystick input, calibrate axes, and manage button assignments.");
}

bool JoystickComponent::setupComplete() const
{
    return JoystickManager::instance()->activeJoystick() == nullptr || JoystickManager::instance()->activeJoystick()->settings()->calibrated()->rawValue().toBool();
}

QUrl JoystickComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/VehicleSetup/JoystickComponent.qml"));
}

QUrl JoystickComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/JoystickComponentSummary.qml"));
}

void JoystickComponent::_activeJoystickChanged(Joystick *joystick)
{
    if (_activeJoystick) {
        disconnect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &VehicleComponent::setupCompleteChanged);
        _activeJoystick = nullptr;
    }

    if (joystick) {
        _activeJoystick = joystick;
        (void) connect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &VehicleComponent::setupCompleteChanged);
    }
}
