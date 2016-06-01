/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMPowerComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMPowerComponent::APMPowerComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent),
    _name("Power")
{
}

QString APMPowerComponent::name(void) const
{
    return _name;
}

QString APMPowerComponent::description(void) const
{
    return tr("The Power Component is used to setup battery parameters.");
}

QString APMPowerComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/PowerComponentIcon.png");
}

bool APMPowerComponent::requiresSetup(void) const
{
    return true;
}

bool APMPowerComponent::setupComplete(void) const
{
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, QStringLiteral("BATT_CAPACITY"))->rawValue().toInt() != 0;
}

QStringList APMPowerComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    list << QStringLiteral("BATT_CAPACITY");

    return list;
}

QUrl APMPowerComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMPowerComponent.qml"));
}

QUrl APMPowerComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMPowerComponentSummary.qml"));
}

QString APMPowerComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    return QString();
}
