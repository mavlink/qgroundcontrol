/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_MAVLinkMission.h"

#include "MissionItem.h"
#include "MissionManager.h"
#include "Vehicle.h"

#include <QtCore/QLoggingCategory>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(SITLTestLog)

void SITLMissionTest::testUploadDownload()
{
    QVERIFY(vehicle());
    MissionManager *mm = vehicle()->missionManager();
    QVERIFY(mm);

    // Create a simple 5-waypoint mission near the default SIH home position
    // SIH defaults to lat=47.397742, lon=8.545594, alt=488m
    const double homeLat = 47.397742;
    const double homeLon = 8.545594;
    const double alt = 20.0;
    const double offset = 0.001; // ~111m

    // PlanManager::writeMissionItems takes ownership of MissionItem objects — do NOT parent them
    QList<MissionItem *> items;

    // Item 0: Takeoff
    items.append(new MissionItem(0, MAV_CMD_NAV_TAKEOFF, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                 0, 0, 0, 0, homeLat, homeLon, alt, true, false));

    // Items 1-4: Waypoints in a square pattern
    const double lats[] = {homeLat + offset, homeLat + offset, homeLat - offset, homeLat - offset};
    const double lons[] = {homeLon + offset, homeLon - offset, homeLon - offset, homeLon + offset};
    for (int i = 0; i < 4; ++i) {
        items.append(new MissionItem(i + 1, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                     0, 0, 0, 0, lats[i], lons[i], alt, true, false));
    }

    const int uploadCount = items.count();

    // Upload mission — PlanManager takes ownership of items
    QSignalSpy spyUpload(mm, &MissionManager::sendComplete);
    mm->writeMissionItems(items);
    QVERIFY2(waitForSignal(spyUpload, TestTimeout::longMs(), QStringLiteral("MissionManager::sendComplete")),
             "Mission upload timed out");
    QCOMPARE(spyUpload.count(), 1);

    // Verify no error (sendComplete(bool error) — error should be false)
    const bool uploadError = spyUpload.at(0).at(0).toBool();
    QVERIFY2(!uploadError, "Mission upload reported an error");

    // Download mission back
    QSignalSpy spyDownload(mm, &MissionManager::newMissionItemsAvailable);
    mm->loadFromVehicle();
    QVERIFY2(waitForSignal(spyDownload, TestTimeout::longMs(), QStringLiteral("MissionManager::newMissionItemsAvailable")),
             "Mission download timed out");

    // Compare item count. PX4 may not include the home/takeoff item (index 0)
    // in the download, so the downloaded count can be uploadCount or uploadCount-1.
    const QList<MissionItem *> &downloaded = mm->missionItems();
    QVERIFY2(downloaded.count() >= uploadCount - 1,
             qPrintable(QStringLiteral("Expected at least %1 items, got %2")
                            .arg(uploadCount - 1)
                            .arg(downloaded.count())));

    qCInfo(SITLTestLog) << "Mission round-trip verified: uploaded" << uploadCount
            << "items, downloaded" << downloaded.count();
}

UT_REGISTER_TEST(SITLMissionTest, TestLabel::SITL, TestLabel::MAVLinkProtocol)
