#pragma once

#include "TestFixtures.h"

/// Unit tests for TestFixtures classes.
/// Tests verify proper setup, teardown, and helper methods.
class TestFixturesTest : public OfflineTest
{
    Q_OBJECT

public:
    TestFixturesTest() = default;

private slots:
    // TempDirTest tests
    void _testTempDirCreation();
    void _testTempFilePath();
    void _testCopyResourceToTemp();

    // OfflinePlanTest helper tests
    void _testOfflinePlanControllerCreation();
};

/// Tests for VehicleTest fixture
class VehicleTestFixtureTest : public VehicleTest
{
    Q_OBJECT

public:
    VehicleTestFixtureTest() = default;

private slots:
    // Verify fixture setup
    void _testVehicleIsConnected();
    void _testMockLinkIsAvailable();
    void _testAutopilotType();

    // Wait helper tests
    void _testWaitForParametersReadyAlreadyReady();
    void _testWaitForInitialConnectAlreadyComplete();
};

/// Tests for MissionTest fixture
class MissionTestFixtureTest : public MissionTest
{
    Q_OBJECT

public:
    MissionTestFixtureTest() = default;

private slots:
    // Verify fixture setup
    void _testPlanControllerCreated();
    void _testMissionControllerAvailable();
    void _testClearMission();
};

/// Tests for ParameterTest fixture
class ParameterTestFixtureTest : public ParameterTest
{
    Q_OBJECT

public:
    ParameterTestFixtureTest() = default;

private slots:
    // Verify fixture setup
    void _testParameterManagerAvailable();
    void _testGetParameter();
};
