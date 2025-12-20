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

}

QString JoystickComponent::description() const
{
    return tr("Configure joystick input, calibrate axes, and manage button assignments.");
}

bool JoystickComponent::setupComplete() const
{
    return true;
#if 0
    Joystick *const joystick = _activeJoystick;
    if (!joystick) {
        return true;
    }

    if (joystick->axisCount() == 0 || !joystick->requiresCalibration()) {
        return true;
    }

    return joystick->isCalibrated();
#endif
}

QUrl JoystickComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/VehicleSetup/JoystickComponent.qml"));
}

QUrl JoystickComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/JoystickComponentSummary.qml"));
}
