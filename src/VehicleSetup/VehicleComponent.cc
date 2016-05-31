/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "VehicleComponent.h"
#include "AutoPilotPlugin.h"

VehicleComponent::VehicleComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    QObject(parent),
    _vehicle(vehicle),
    _autopilot(autopilot)
{
    Q_ASSERT(vehicle);
    Q_ASSERT(autopilot);
}

VehicleComponent::~VehicleComponent()
{
    
}

void VehicleComponent::addSummaryQmlComponent(QQmlContext* context, QQuickItem* parent)
{
    Q_ASSERT(context);
    
    // FIXME: We own this object now, need to delete somewhere
    QQmlComponent component(context->engine(), QUrl::fromUserInput("qrc:/qml/VehicleComponentSummaryButton.qml"));
    if (component.status() == QQmlComponent::Error) {
        qDebug() << component.errors();
        Q_ASSERT(false);
    }
    
    QQuickItem* item = qobject_cast<QQuickItem*>(component.create(context));
    Q_ASSERT(item);
    item->setParentItem(parent);
    item->setProperty("vehicleComponent", QVariant::fromValue(this));
}

void VehicleComponent::setupTriggerSignals(void)
{
    // Watch for changed on trigger list params
    foreach (const QString &paramName, setupCompleteChangedTriggerList()) {
        if (_autopilot->parameterExists(FactSystem::defaultComponentId, paramName)) {
            Fact* fact = _autopilot->getParameterFact(FactSystem::defaultComponentId, paramName);
            connect(fact, &Fact::valueChanged, this, &VehicleComponent::_triggerUpdated);
        }
    }
}

void VehicleComponent::_triggerUpdated(QVariant /*value*/)
{
    emit setupCompleteChanged(setupComplete());
}

bool VehicleComponent::allowSetupWhileArmed(void) const
{
    // Default is to not allow setup while armed
    return false;
}
