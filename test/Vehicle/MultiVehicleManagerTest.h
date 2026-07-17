#pragma once

#include "BaseClasses/VehicleTest.h"

/// Regression test for issue #13251 (crash 2): double vehicle delete when
/// VehicleLinkManager::allLinksRemoved is delivered twice for the same vehicle
/// (e.g. link teardown racing with heartbeat-timeout removal). The second delivery
/// must hit the not-found guard in MultiVehicleManager::_deleteVehiclePhase1 instead
/// of double-scheduling _deleteVehiclePhase2 -> double deleteLater -> crash.
class MultiVehicleManagerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _testDuplicateAllLinksRemoved();
};
