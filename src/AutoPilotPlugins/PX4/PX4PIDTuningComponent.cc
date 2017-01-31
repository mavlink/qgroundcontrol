/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4PIDTuningComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "AirframeComponent.h"

PX4PIDTuningComponent::PX4PIDTuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name("PID Tuning")
{
}

QString PX4PIDTuningComponent::name(void) const
{
    return _name;
}

QString PX4PIDTuningComponent::description(void) const
{
    return tr("PID Tuning is used to setup Proportional Integral Derivative controllers");
}

QString PX4PIDTuningComponent::iconResource(void) const
{
    return "/qmlimages/PIDTuningComponentIcon.png";
}

bool PX4PIDTuningComponent::requiresSetup(void) const
{
    return false;
}

bool PX4PIDTuningComponent::setupComplete(void) const
{
    return true;
}

bool PX4PIDTuningComponent::isSupported(Vehicle* vehicle)
{
    return vehicle->multiRotor() || vehicle->vtol();
}

QStringList PX4PIDTuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl PX4PIDTuningComponent::setupSource(void) const
{
    return QUrl("qrc:/qml/PX4PIDTuningComponentCopter.qml");
}

QUrl PX4PIDTuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}

QString PX4PIDTuningComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    if (plugin) {
        if (!plugin->airframeComponent()->setupComplete()) {
            return plugin->airframeComponent()->name();
        }
    } else {
        qWarning() << "Internal error: plugin cast failed";
    }

    return QString();
}
