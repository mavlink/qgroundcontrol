#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class InitialConnectTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _performTestCases_data();
    void _performTestCases();
    void _boardVendorProductId();
    void _progressTracking();
    void _highLatencySkipsPlanRequests();
    void _genericAutopilotVersionFailureSkipsUnsupportedPlanTypes();
    void _multipleReconnects();
    void _rallyFailurePathDoesNotLeakCompletionHandler();
    void _subsystemFailureFallsThrough_data();
    void _subsystemFailureFallsThrough();
    void _stateRunMatrix_data();
    void _stateRunMatrix();
};
