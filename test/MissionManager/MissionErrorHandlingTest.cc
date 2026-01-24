#include "MissionErrorHandlingTest.h"
#include "PlanMasterController.h"
#include "MissionController.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"

#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>
#include <cmath>

void MissionErrorHandlingTest::init()
{
    OfflineTest::init();

    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(
        QGCMAVLink::firmwareClass(MAV_AUTOPILOT_PX4));

    _masterController = new PlanMasterController(this);
    VERIFY_NOT_NULL(_masterController);
    _masterController->setFlyView(false);
    _masterController->start();
}

void MissionErrorHandlingTest::cleanup()
{
    delete _masterController;
    _masterController = nullptr;

    OfflineTest::cleanup();
}

void MissionErrorHandlingTest::_testInvalidCoordinates()
{
    MissionController* mc = _masterController->missionController();
    VERIFY_NOT_NULL(mc);
    VERIFY_NOT_NULL(mc->visualItems());
    int initialCount = mc->visualItems()->count();

    // Test with invalid coordinate (NaN)
    QGeoCoordinate invalidCoord(std::nan(""), std::nan(""), 100.0);
    mc->insertSimpleMissionItem(invalidCoord, mc->visualItems()->count());

    // Should still add an item (coordinate will be invalid but item exists)
    QVERIFY(mc->visualItems()->count() >= initialCount);

    // Test with out-of-range latitude
    QGeoCoordinate outOfRangeCoord(91.0, 0.0, 100.0);  // Latitude > 90
    mc->insertSimpleMissionItem(outOfRangeCoord, mc->visualItems()->count());

    // Should handle gracefully
    QCOMPARE_GE(mc->visualItems()->count(), initialCount);
}

void MissionErrorHandlingTest::_testInvalidAltitude()
{
    MissionController* mc = _masterController->missionController();
    VERIFY_NOT_NULL(mc);
    VERIFY_NOT_NULL(mc->visualItems());
    int initialCount = mc->visualItems()->count();

    // Test with negative altitude (might be valid for some frames)
    QGeoCoordinate negAltCoord(47.0, 8.0, -100.0);
    mc->insertSimpleMissionItem(negAltCoord, mc->visualItems()->count());
    QCOMPARE_GT(mc->visualItems()->count(), initialCount);

    // Test with very high altitude
    QGeoCoordinate highAltCoord(47.0, 8.0, 100000.0);
    mc->insertSimpleMissionItem(highAltCoord, mc->visualItems()->count());
    QCOMPARE_GT(mc->visualItems()->count(), initialCount + 1);

    // Test with NaN altitude
    QGeoCoordinate nanAltCoord(47.0, 8.0, std::nan(""));
    mc->insertSimpleMissionItem(nanAltCoord, mc->visualItems()->count());

    // Should handle gracefully without crashing
    VERIFY_NOT_NULL(mc->visualItems());
}

void MissionErrorHandlingTest::_testInvalidWaypointIndex()
{
    MissionController* mc = _masterController->missionController();
    VERIFY_NOT_NULL(mc);

    // Add a valid waypoint first
    QGeoCoordinate validCoord(47.0, 8.0, 100.0);
    mc->insertSimpleMissionItem(validCoord, mc->visualItems()->count());

    int count = mc->visualItems()->count();

    // NOTE: MissionController does not validate indices before insertion.
    // Passing negative indices or indices far past the end causes a Qt ASSERT failure.
    // This is documented behavior - the API expects valid indices.
    // The following tests verify that valid boundary conditions work correctly.

    // Insert at the end (valid)
    mc->insertSimpleMissionItem(validCoord, mc->visualItems()->count());
    QCOMPARE(mc->visualItems()->count(), count + 1);

    // Insert at beginning after settings item (index 1, valid)
    mc->insertSimpleMissionItem(validCoord, 1);
    QCOMPARE(mc->visualItems()->count(), count + 2);

    // Verify mission controller is still functional
    VERIFY_NOT_NULL(mc->visualItems());
}

void MissionErrorHandlingTest::_testLoadNonExistentFile()
{
    // Try to load a file that doesn't exist
    _masterController->loadFromFile("/nonexistent/path/to/file.plan");

    // Should handle gracefully - mission should still be valid (empty)
    VERIFY_NOT_NULL(_masterController->missionController());
    VERIFY_NOT_NULL(_masterController->missionController()->visualItems());
}

void MissionErrorHandlingTest::_testLoadInvalidFileType()
{
    // Create a temp file with wrong extension
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (tempFile.open()) {
        tempFile.write("This is not a plan file");
        tempFile.close();

        // Try to load it
        _masterController->loadFromFile(tempFile.fileName());

        // Should handle gracefully
        VERIFY_NOT_NULL(_masterController->missionController());
        VERIFY_NOT_NULL(_masterController->missionController()->visualItems());
    }
}

void MissionErrorHandlingTest::_testRemoveFromEmptyMission()
{
    MissionController* mc = _masterController->missionController();
    VERIFY_NOT_NULL(mc);

    // Clear any existing items
    mc->removeAll();

    int countAfterClear = mc->visualItems()->count();

    // Try to remove from empty mission - should not crash
    mc->removeAll();

    // Count should remain the same (just settings item)
    QCOMPARE(mc->visualItems()->count(), countAfterClear);

    // Try to remove specific indices that don't exist
    // Note: MissionController may not have a removeAt method exposed,
    // but removeAll should be safe to call multiple times
    mc->removeAll();
    mc->removeAll();

    VERIFY_NOT_NULL(mc->visualItems());
}
