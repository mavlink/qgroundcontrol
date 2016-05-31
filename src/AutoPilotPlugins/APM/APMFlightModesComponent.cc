/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMFlightModesComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"
#include "APMRadioComponent.h"

APMFlightModesComponent::APMFlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Flight Modes"))
{
}

QString APMFlightModesComponent::name(void) const
{
    return _name;
}

QString APMFlightModesComponent::description(void) const
{
    return QStringLiteral("The Flight Modes Component is used to assign FLight Modes to Channel 5.");
}

QString APMFlightModesComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/FlightModesComponentIcon.png");
}

bool APMFlightModesComponent::requiresSetup(void) const
{
    return true;
}

bool APMFlightModesComponent::setupComplete(void) const
{
    return true;
}

QStringList APMFlightModesComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMFlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFlightModesComponent.qml"));
}

QUrl APMFlightModesComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFlightModesComponentSummary.qml"));
}

QString APMFlightModesComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    } else if (!plugin->radioComponent()->setupComplete()) {
        return plugin->radioComponent()->name();
    }
    
    return QString();
}
