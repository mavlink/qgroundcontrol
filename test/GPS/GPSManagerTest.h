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
    void _vehicleGpsTracking();
    void _vehicleEstimateTracking();
    void _groundStationGgaNeedsMeanSeaLevel_data();
    void _groundStationGgaNeedsMeanSeaLevel();
    void _qmlServicesAvailableBeforeInit();
};
