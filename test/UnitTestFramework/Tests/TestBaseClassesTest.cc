#include "TestBaseClassesTest.h"

#include <QtTest/QTest>

#include "LinkManager.h"
#include "MissionController.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void TestBaseClassesTest::_testVehicleTestDerived()
{
    // Create an instance of our simple derived class
    SimpleVehicleTest vehicleTest;
    // Use public wrapper to initialize
    vehicleTest.doInit();
    // Verify the vehicle was connected
    QVERIFY2(vehicleTest.wasInitCalled(), "init() should have been called");
    QVERIFY2(vehicleTest.hadVehicle(), "Vehicle should have been connected during init");
    QVERIFY(vehicleTest.vehicle() != nullptr);
    QVERIFY(vehicleTest.mockLink() != nullptr);
    // Verify vehicle properties
    QCOMPARE(vehicleTest.autopilotType(), MAV_AUTOPILOT_PX4);
    // Cleanup using public wrapper
    vehicleTest.doCleanup();
    QVERIFY(vehicleTest.wasCleanupCalled());
    // Vehicle should be disconnected after cleanup
    QVERIFY(vehicleTest.vehicle() == nullptr);
}

void TestBaseClassesTest::_testOfflineMissionTestDerived()
{
    SimpleOfflineMissionTest offlineTest;
    // Initialize using public wrapper
    offlineTest.doInit();
    // Should have mission controller without vehicle
    QVERIFY(offlineTest.hasMissionController());
    QVERIFY(offlineTest.planController() != nullptr);
    QVERIFY(offlineTest.missionController() != nullptr);
    // Should be able to add mission items offline
    MissionController* mc = offlineTest.missionController();
    QVERIFY(mc->visualItems() != nullptr);
    // Initial state: just home/settings item
    const int initialCount = mc->visualItems()->count();
    QVERIFY(initialCount >= 1);
    // Cleanup using public wrapper
    offlineTest.doCleanup();
}

void TestBaseClassesTest::_testCommsTestDerived()
{
    SimpleCommsTest commsTest;
    // Initialize using public wrapper
    commsTest.doInit();
    // Should have link manager
    QVERIFY(commsTest.hasLinkManager());
    QVERIFY(commsTest.linkManager() != nullptr);
    // Should start with no links
    QCOMPARE(commsTest.linkManager()->links().count(), 0);
    // Create a mock link using public wrapper
    SharedLinkInterfacePtr link = commsTest.doCreateMockLink("TestLink1");
    QVERIFY(link != nullptr);
    QCOMPARE(commsTest.linkManager()->links().count(), 1);
    // Wait for vehicle using public wrapper
    Vehicle* vehicle = commsTest.doWaitForVehicleConnect(TestTimeout::longMs());
    QVERIFY(vehicle != nullptr);
    // Cleanup disconnects all
    commsTest.doCleanup();
    QCOMPARE(commsTest.linkManager()->links().count(), 0);
}

UT_REGISTER_TEST(TestBaseClassesTest, TestLabel::Unit)
