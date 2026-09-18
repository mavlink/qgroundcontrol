#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanup() override;
    void testSourceClearedOnStopAndFreshStart();
    void _invalidProviderAltitude_data();
    void _invalidProviderAltitude();
    void testDefaultRTKBaseProvider();
    void _activeVehicleAndCommunicationLoss();
    void _gcsObservation_data();
    void _gcsObservation();
    void _gcsSelectionAndFreshness();
};
