#include "NTRIPConnectedVehicles.h"

#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"

void NTRIPConnectedVehicles::attach()
{
    auto* const vehicles = MultiVehicleManager::instance();
    if (!vehicles) {
        return;
    }
    // Each signal follows the change of the list it tells of.
    (void) connect(vehicles, &MultiVehicleManager::vehicleAdded, this, &NTRIPConnectedVehicles::_update,
                   Qt::UniqueConnection);
    (void) connect(vehicles, &MultiVehicleManager::vehicleRemoved, this, &NTRIPConnectedVehicles::_update,
                   Qt::UniqueConnection);
    _update();
}

void NTRIPConnectedVehicles::_update()
{
    const auto* const vehicles = MultiVehicleManager::instance();
    _setPresent(vehicles && vehicles->vehicles()->count() > 0);
}
