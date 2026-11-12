#pragma once

#include "UnitTest.h"

class GPSCorrectionLossAlertTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _tellsOncePerInterruption();
    void _namesDependentVehicles();
    void _staysSilent_data();
    void _staysSilent();
    void _resumedNeedsAVehicleToTell();
};
