#pragma once

#include "TestFixtures.h"

/// Integration test for mission editing workflow:
/// Create mission -> Modify -> Remove -> Recreate
/// Note: Actual vehicle upload/download is tested by MissionManagerTest.
class MissionWorkflowTest : public MissionTest
{
    Q_OBJECT

public:
    MissionWorkflowTest() = default;

private slots:
    void _testFullMissionRoundtrip();
    void _testMissionModifyAndReupload();

private:
    void _setupMissionWaypoints(int waypointCount);
    bool _uploadMission();
    bool _downloadMission();
    bool _verifyMissionItems(int expectedWaypointCount);
};
