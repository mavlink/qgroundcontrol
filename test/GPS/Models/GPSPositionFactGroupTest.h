#pragma once

#include "UnitTest.h"

class GPSPositionFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _integrityProjection();
    void _integrityExpiryAndReentrancy();
    void _vehicleMessages_data();
    void _vehicleMessages();
    void _vehicleSentinels_data();
    void _vehicleSentinels();
    void _localObservation();
    void _resetDuringUpdate();
};
