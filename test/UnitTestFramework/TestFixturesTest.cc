#include "TestFixturesTest.h"
#include "MockLink.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

// ============================================================================
// TestFixturesTest - TempDirTest tests
// ============================================================================

void TestFixturesTest::_testTempDirCreation()
{
    // Test that QTemporaryDir works as expected (this is what TempDirTest uses)
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(QDir(tempDir.path()).exists());
}

void TestFixturesTest::_testTempFilePath()
{
    // Test that temp file path construction works
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString filePath = tempDir.filePath("test.txt");
    QVERIFY(!filePath.isEmpty());
    QVERIFY(filePath.endsWith("test.txt"));
    QVERIFY(filePath.startsWith(tempDir.path()));
}

void TestFixturesTest::_testCopyResourceToTemp()
{
    // Test that copying a non-existent resource fails gracefully
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Try to copy non-existent resource
    QString destPath = tempDir.filePath("test.txt");
    bool copyResult = QFile::copy(":/nonexistent/file.txt", destPath);
    QVERIFY(!copyResult);
}

// ============================================================================
// OfflinePlanTest helper test
// ============================================================================

void TestFixturesTest::_testOfflinePlanControllerCreation()
{
    // Test that createOfflinePlanController works
    PlanMasterController *controller = createOfflinePlanController();
    VERIFY_NOT_NULL(controller);

    // Controller should have mission controller
    VERIFY_NOT_NULL(controller->missionController());

    // Cleanup
    delete controller;
}

// ============================================================================
// VehicleTestFixtureTest - VehicleTest fixture tests
// ============================================================================

void VehicleTestFixtureTest::_testVehicleIsConnected()
{
    VERIFY_NOT_NULL(vehicle());
    QVERIFY(vehicle()->isInitialConnectComplete());
}

void VehicleTestFixtureTest::_testMockLinkIsAvailable()
{
    VERIFY_NOT_NULL(mockLink());
    QVERIFY(mockLink()->isConnected());
}

void VehicleTestFixtureTest::_testAutopilotType()
{
    // Default is PX4
    QCOMPARE(autopilotType(), MAV_AUTOPILOT_PX4);
    QCOMPARE(vehicle()->firmwareType(), MAV_AUTOPILOT_PX4);
}

void VehicleTestFixtureTest::_testWaitForParametersReadyAlreadyReady()
{
    // Parameters should already be loaded from init
    // This should return immediately
    QVERIFY(waitForParametersReady(100));
}

void VehicleTestFixtureTest::_testWaitForInitialConnectAlreadyComplete()
{
    // Initial connect should already be complete from init
    // This should return immediately
    QVERIFY(waitForInitialConnect(100));
}

// ============================================================================
// MissionTestFixtureTest - MissionTest fixture tests
// ============================================================================

void MissionTestFixtureTest::_testPlanControllerCreated()
{
    VERIFY_NOT_NULL(planController());
}

void MissionTestFixtureTest::_testMissionControllerAvailable()
{
    VERIFY_NOT_NULL(missionController());
}

void MissionTestFixtureTest::_testClearMission()
{
    // clearMission should succeed
    QVERIFY(clearMission());

    // After clearing, mission should be empty (just home position)
    VERIFY_NOT_NULL(missionController());
    QCOMPARE(missionController()->visualItems()->count(), 1); // Home position only
}

// ============================================================================
// ParameterTestFixtureTest - ParameterTest fixture tests
// ============================================================================

void ParameterTestFixtureTest::_testParameterManagerAvailable()
{
    VERIFY_NOT_NULL(parameterManager());
    QVERIFY(parameterManager()->parametersReady());
}

void ParameterTestFixtureTest::_testGetParameter()
{
    // Try to get a standard PX4 parameter
    Fact *fact = getParameter("BAT1_V_CHARGED");
    VERIFY_NOT_NULL(fact);
}

// No local Q_OBJECT classes in this file - moc generated from header
