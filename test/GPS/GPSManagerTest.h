#pragma once

#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionState();
    void _positionManagerLifecycle();
    void _sourceModeMatchesSetting();
    void _connectionPolling();
    void _positionReceiverGivesGcsPosition_data();
    void _positionReceiverGivesGcsPosition();
    void _discoveryLeavesPositionReceiverDevice();
    void _positionReceiverOverBluetooth();
    void _bluetoothDeviceIsThePositionReceivers();
    void _vehicleGpsTracking();
    void _vehicleGpsEllipsoidHeight_data();
    void _vehicleGpsEllipsoidHeight();
    void _vehicleEstimateTracking();
    void _groundStationGgaReportsAnyAltitudeDatum_data();
    void _groundStationGgaReportsAnyAltitudeDatum();
    void _ntripWaitsForAVehicleFromTheFirstConnection();
    void _qmlServicesAvailableBeforeInit();
};
