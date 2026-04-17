#pragma once

#include "UnitTest.h"

class MAVLinkStreamConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testSetHighRateRateAndAttitude();
    void _testSetHighRateVelAndPos();
    void _testSetHighRateAltAirspeed();
    void _testRestoreDefaultsAfterConfigure();
    void _testRestoreDefaultsFromIdle();
    void _testInterruptConfigureWithNewRequest();
};
