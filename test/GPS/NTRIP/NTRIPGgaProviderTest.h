#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSourceClearedOnStopAndFreshStart();
    void testDefaultRTKBaseProvider();
    void _vehicleSourcesAndFreshness();
    void _invalidVehicleObservations_data();
    void _invalidVehicleObservations();
    void _highLatencyObservations_data();
    void _highLatencyObservations();
    void _activeVehicleAndCommunicationLoss();
};
