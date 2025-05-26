/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AutoPilotPlugin.h"
#include "FirmwarePlugin.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

#include <QtCore/QCoreApplication>

QGC_LOGGING_CATEGORY(AutoPilotPluginLog, "qgc.autopilotplugin.autopilotplugin");

AutoPilotPlugin::AutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _firmwarePlugin(vehicle->firmwarePlugin())
{
    // qCDebug(AutoPilotPluginLog) << Q_FUNC_INFO << this;
}

AutoPilotPlugin::~AutoPilotPlugin()
{
    // qCDebug(AutoPilotPluginLog) << Q_FUNC_INFO << this;
}

void AutoPilotPlugin::_recalcSetupComplete()
{
    bool newSetupComplete = true;

    for (const QVariant &componentVariant : vehicleComponents()) {
        const VehicleComponent *const component = qobject_cast<const VehicleComponent*>(qvariant_cast<const QObject*>(componentVariant));
        if (component) {
            if (!component->setupComplete()) {
                newSetupComplete = false;
                break;
            }
        } else {
            qCWarning(AutoPilotPluginLog) << "Incorrectly typed VehicleComponent";
        }
    }

    if (_setupComplete != newSetupComplete) {
        _setupComplete = newSetupComplete;
        emit setupCompleteChanged();
    }
}

void AutoPilotPlugin::parametersReadyPreChecks()
{
    _recalcSetupComplete();

    // Connect signals in order to keep setupComplete up to date
    for (QVariant componentVariant : vehicleComponents()) {
        VehicleComponent *const component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject*>(componentVariant));
        if (component) {
            (void) connect(component, &VehicleComponent::setupCompleteChanged, this, &AutoPilotPlugin::_recalcSetupComplete);
        } else {
            qCWarning(AutoPilotPluginLog) << "Incorrectly typed VehicleComponent";
        }
    }

    if (!_setupComplete) {
        // Take the user to Vehicle Config Summary
        qgcApp()->showVehicleConfig();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        qgcApp()->showAppMessage(tr("One or more vehicle components require setup prior to flight."));
    }
}

VehicleComponent *AutoPilotPlugin::findKnownVehicleComponent(KnownVehicleComponent knownVehicleComponent)
{
    if (knownVehicleComponent != UnknownVehicleComponent) {
        for (const QVariant &componentVariant: vehicleComponents()) {
            VehicleComponent *const component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject *>(componentVariant));
            if (component && (component->KnownVehicleComponent() == knownVehicleComponent)) {
                return component;
            }
        }
    }

    return nullptr;
}
