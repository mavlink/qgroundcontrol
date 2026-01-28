#pragma once

#include "TestFixtures.h"

/// Unit tests for Vehicle initial connection sequence.
/// Tests various autopilot types, failure modes, and connection properties.
class InitialConnectTest : public UnitTest
{
    Q_OBJECT

public:
    InitialConnectTest() = default;

private slots:
    void cleanup() override;

    // Basic connection tests
    void _testPX4Connection();
    void _testArduPilotConnection();

    // Property verification tests
    void _testVehicleId();
    void _testBoardVendorProductId();
    void _testFirmwareTypeProperties();
    void _testVehicleTypeProperties();

    // Signal tests
    void _testInitialConnectCompleteSignal();
    void _testCapabilitiesKnownSignal();

    // Failure mode tests
    void _testFailNone();
    void _testFailAutopilotVersionFailure();
    void _testFailAutopilotVersionLost();
    void _testFailParamNoResponse();
    void _testFailMissingParamOnInitial();
    void _testFailMissingParamOnAllRequests();
};
