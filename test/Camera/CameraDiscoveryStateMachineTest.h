#pragma once

#include "UnitTest.h"

class CameraDiscoveryStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testStartDiscovery();
    void _testInfoReceived();
    void _testRequestFailed();
    void _testMaxRetries();
    void _testHeartbeatTracking();
    void _testTimeout();
};
