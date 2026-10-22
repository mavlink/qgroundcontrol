#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class PlanManager;

/// Verifies that PlanManager uses the MAVLink opaque plan ids to follow plan changes made on the vehicle,
/// without reloading plans it has just transferred itself.
class PlanManagerOpaqueIdTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _vehicleChangeReloads_data();
    void _vehicleChangeReloads();
    void _ownUploadDoesNotReload_data();
    void _ownUploadDoesNotReload();
    void _flyViewFollowsVehicleChange_data();
    void _flyViewFollowsVehicleChange();
    void _planViewOffersVehicleChange_data();
    void _planViewOffersVehicleChange();

private:
    static void _addPlanTypeRows();
    void _connect(MAV_AUTOPILOT autopilot, MockConfiguration::Options options = MockConfiguration::OptionNone);
    PlanManager* _planManager(MAV_MISSION_TYPE planType) const;
};
