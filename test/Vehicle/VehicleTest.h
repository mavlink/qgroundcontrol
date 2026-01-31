#pragma once

#include "UnitTest.h"

/// Tests for Vehicle class state machine integration
/// Verifies that Vehicle properly coordinates InitialConnectStateMachine,
/// ComponentInformationManager, and related state machines
class VehicleTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Basic vehicle state tests
    void _testVehicleCreation();
    void _testVehicleInitialState();

    // State machine integration tests
    void _testInitialConnectStateMachineIntegration();
    void _testStateMachineCompletion();
    void _testStateMachineProgressSignals();

    // Firmware-specific tests
    void _testPX4Vehicle();
    void _testArduPilotVehicle();

    // Error handling tests
    void _testConnectionTimeout();
    void _testReconnectAfterDisconnect();

    // Multi-vehicle tests
    void _testMultipleVehicleStateMachines();
};
