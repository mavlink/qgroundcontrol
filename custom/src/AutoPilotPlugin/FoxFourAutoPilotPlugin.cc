/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FoxFourAutoPilotPlugin.h"
#include "ParameterManager.h"
#include "QGCCorePlugin.h"
#include "Vehicle.h"

FoxFourAutoPilotPlugin::FoxFourAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : APMAutoPilotPlugin(vehicle, parent)
{
    _onboardComputersMngr = new OnboardComputersManager(vehicle, this);
}

FoxFourAutoPilotPlugin::~FoxFourAutoPilotPlugin(){
    delete _onboardComputersMngr;
}

const QVariantList &FoxFourAutoPilotPlugin::vehicleComponents(){
    if (_components.isEmpty()) {
        _components = APMAutoPilotPlugin::vehicleComponents();
    }
    return _components;
}

void FoxFourAutoPilotPlugin::rebootOnboardComputers(){
    if (!_onboardComputersMngr) {
            qCWarning(VehicleLog) << "Cannot reboot onboard computers: no manager is present";
            return;
        }
        qWarning() << "Rebooting onboard computers";
        _onboardComputersMngr->rebootAllOnboardComputers();
}
