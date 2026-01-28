#pragma once

#include "TestFixtures.h"

class PlanMasterController;
class MissionController;

/// Tests for plan file save/load roundtrip.
/// Verifies that missions can be saved to .plan files and loaded back correctly.
/// Uses OfflineTest since it works with offline PlanMasterController.
class PlanFileRoundtripTest : public OfflineTest
{
    Q_OBJECT

public:
    PlanFileRoundtripTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void _testEmptyMissionRoundtrip();
    void _testSimpleMissionRoundtrip();
    void _testComplexMissionRoundtrip();
    void _testCorruptedFileHandling();

private:
    void _createMission(int waypointCount);
    bool _compareMissions(MissionController* original, MissionController* loaded);

    PlanMasterController* _masterController = nullptr;
    QString _tempFilePath;
};
