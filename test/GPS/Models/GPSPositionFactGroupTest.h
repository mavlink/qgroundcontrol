#pragma once

#include "UnitTest.h"

class GPSPositionFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _vehicleMessages_data();
    void _vehicleMessages();
    void _localObservation();
    void _resetDuringUpdate();
};
