/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleComponent.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>

QGC_LOGGING_CATEGORY(VehicleComponentLog, "qgc.autopilotplugin.vehiclecomponent");

VehicleComponent::VehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _autopilot(autopilot)
    , _KnownVehicleComponent(KnownVehicleComponent)
{
    // qCDebug(VehicleComponentLog) << Q_FUNC_INFO << this;

    if (!vehicle || !autopilot) {
        qCWarning(VehicleComponentLog) << "Internal error";
    }
}

VehicleComponent::~VehicleComponent()
{
    // qCDebug(VehicleComponentLog) << Q_FUNC_INFO << this;
}

void VehicleComponent::addSummaryQmlComponent(QQmlContext *context, QQuickItem *parent)
{
    if (!context) {
        qCWarning(VehicleComponentLog) << "Internal error";
        return;
    }

    QQmlComponent component = new QQmlComponent(context->engine(), QUrl::fromUserInput("qrc:/qml/VehicleComponentSummaryButton.qml"), this);
    if (component.status() == QQmlComponent::Error) {
        qCWarning(VehicleComponentLog) << component.errors();
        return;
    }

    QQuickItem *const item = qobject_cast<QQuickItem*>(component.create(context));
    if (!item) {
        qCWarning(VehicleComponentLog) << "Internal error";
        return;
    }

    item->setParentItem(parent);
    item->setProperty("vehicleComponent", QVariant::fromValue(this));
}

void VehicleComponent::setupTriggerSignals()
{
    // Watch for changed on trigger list params
    for (const QString &paramName: setupCompleteChangedTriggerList()) {
        if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, paramName)) {
            Fact *const fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, paramName);
            (void) connect(fact, &Fact::valueChanged, this, &VehicleComponent::_triggerUpdated);
        }
    }
}
