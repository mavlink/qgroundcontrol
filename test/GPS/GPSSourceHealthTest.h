#pragma once

#include "UnitTest.h"

class GPSSourceHealthTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _normalizesObservation_data();
    void _normalizesObservation();
    void _ageAndRecovery();
    void _independentSatelliteExpiry();
    void _settingsStatus();
};
