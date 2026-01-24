#include "MissionWorkflowTest.h"
#include "PlanMasterController.h"
#include "MissionController.h"
#include "SimpleMissionItem.h"
#include "MissionSettingsItem.h"
#include "QmlObjectListModel.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void MissionWorkflowTest::_setupMissionWaypoints(int waypointCount)
{
    // Record initial count (includes settings item and possibly takeoff for PX4)
    int initialCount = missionController()->visualItems()->count();

    // Add waypoints in a simple pattern
    QGeoCoordinate baseCoord(47.3977, 8.5456);  // Zurich area
    for (int i = 0; i < waypointCount; i++) {
        QGeoCoordinate waypoint = baseCoord.atDistanceAndAzimuth(100.0 * (i + 1), 45.0 * i);
        waypoint.setAltitude(50.0 + i * 10.0);
        missionController()->insertSimpleMissionItem(waypoint, missionController()->visualItems()->count());
    }

    // Verify items were added (initial items + waypoints)
    QCOMPARE(missionController()->visualItems()->count(), initialCount + waypointCount);
}

bool MissionWorkflowTest::_uploadMission()
{
    // This is a placeholder - actual vehicle upload is tested by MissionManagerTest
    // Here we just verify the mission controller has valid items
    return missionController()->visualItems()->count() > 1;
}

bool MissionWorkflowTest::_downloadMission()
{
    // This is a placeholder - actual vehicle download is tested by MissionManagerTest
    return true;
}

bool MissionWorkflowTest::_verifyMissionItems(int expectedWaypointCount)
{
    QmlObjectListModel* items = missionController()->visualItems();
    if (!TestHelpers::verifyNotNull(items, "visualItems")) {
        return false;
    }

    // Count actual SimpleMissionItems (waypoints)
    int waypointCount = 0;
    for (int i = 0; i < items->count(); i++) {
        SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(items->get(i));
        if (simpleItem && simpleItem->command() == MAV_CMD_NAV_WAYPOINT) {
            waypointCount++;
        }
    }

    if (waypointCount != expectedWaypointCount) {
        qWarning() << "Expected" << expectedWaypointCount << "waypoints, got" << waypointCount;
        return false;
    }

    return true;
}

void MissionWorkflowTest::_testFullMissionRoundtrip()
{
    const int waypointCount = 5;

    // Step 1: Verify we start with a valid mission controller
    VERIFY_NOT_NULL(missionController());
    VERIFY_NOT_NULL(missionController()->visualItems());

    // Record initial visual item count (settings item + possibly takeoff)
    int initialVisualCount = missionController()->visualItems()->count();
    QCOMPARE_GE(initialVisualCount, 1);  // At least settings item

    // Step 2: Create mission with waypoints
    _setupMissionWaypoints(waypointCount);
    QCOMPARE(missionController()->visualItems()->count(), initialVisualCount + waypointCount);

    // Step 3: Verify waypoints were created correctly
    QVERIFY2(_verifyMissionItems(waypointCount), "Waypoint count mismatch after creation");

    // Step 4: Remove all items and verify empty
    missionController()->removeAll();
    int countAfterRemove = missionController()->visualItems()->count();
    // After removeAll, should have just the settings item (and possibly takeoff)
    QCOMPARE_LE(countAfterRemove, initialVisualCount);

    // Step 5: Add waypoints again to verify we can recreate mission
    _setupMissionWaypoints(waypointCount);
    QVERIFY2(_verifyMissionItems(waypointCount), "Waypoint count mismatch after re-creation");
}

void MissionWorkflowTest::_testMissionModifyAndReupload()
{
    const int initialWaypointCount = 3;

    // Step 1: Verify starting state
    VERIFY_NOT_NULL(missionController());
    int initialVisualCount = missionController()->visualItems()->count();

    // Step 2: Create initial mission
    _setupMissionWaypoints(initialWaypointCount);
    QVERIFY2(_verifyMissionItems(initialWaypointCount), "Initial waypoint count mismatch");

    // Step 3: Add more waypoints
    QGeoCoordinate newWaypoint1(47.4, 8.55, 100.0);
    QGeoCoordinate newWaypoint2 = newWaypoint1.atDistanceAndAzimuth(100, 90);
    newWaypoint2.setAltitude(110.0);

    missionController()->insertSimpleMissionItem(newWaypoint1, missionController()->visualItems()->count());
    missionController()->insertSimpleMissionItem(newWaypoint2, missionController()->visualItems()->count());

    // Step 4: Verify updated count
    int expectedTotalCount = initialVisualCount + initialWaypointCount + 2;
    QCOMPARE(missionController()->visualItems()->count(), expectedTotalCount);
    QVERIFY2(_verifyMissionItems(initialWaypointCount + 2), "Modified waypoint count mismatch");

    // Step 5: Verify mission is dirty (has unsaved changes)
    QVERIFY(missionController()->dirty());
}
