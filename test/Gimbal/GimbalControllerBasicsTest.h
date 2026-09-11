#pragma once

#include "GimbalControllerTestBase.h"

/// Discovery, capability, control-arbitration and command-path coverage for GimbalController via MockLink.
class GimbalControllerBasicsTest : public GimbalControllerTestBase
{
    Q_OBJECT

private slots:
    void _testDiscovery();
    void _testCapabilityFlags();
    void _testControlArbitration();
    void _testYawLockRoundTrip();
    void _testOnScreenControlBodyFrame_data();
    void _testOnScreenControlBodyFrame();
    void _testCommandPaths();
    void _testJoystickRateSlots();
    void _testRateCommand();
};
