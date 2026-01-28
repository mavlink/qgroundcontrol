#pragma once

#include "TestFixtures.h"

class PlanMasterController;

/// Tests for error handling in mission operations.
/// Verifies graceful handling of invalid inputs and error conditions.
/// Uses OfflineTest since it works with offline PlanMasterController.
class MissionErrorHandlingTest : public OfflineTest
{
    Q_OBJECT

public:
    MissionErrorHandlingTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void _testInvalidCoordinates();
    void _testInvalidAltitude();
    void _testInvalidWaypointIndex();
    void _testLoadNonExistentFile();
    void _testLoadInvalidFileType();
    void _testRemoveFromEmptyMission();

private:
    PlanMasterController* _masterController = nullptr;
};
