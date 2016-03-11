/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
