#pragma once

#include <memory>

#include "MissionControllerManagerTest.h"

class MissionController;
class PlanMasterController;

/// Tests for the MissionController incremental tree model sync (PlanTreeView branch).
/// Verifies that _visualItemsTree stays in sync with _visualItems when
/// inserting/removing waypoints, calling removeAll, and loading plans.
class MissionControllerTreeTest : public MissionControllerManagerTest
{
    Q_OBJECT

public:
    ~MissionControllerTreeTest() override;

private slots:
    void cleanup() override;

    // Tree structure after init
    void _testTreeStructureAfterInit();

    // Insert waypoints → tree sync
    void _testInsertWaypointUpdatesTree();

    // Remove waypoints → tree sync
    void _testRemoveWaypointUpdatesTree();

    // removeAll → tree rebuilt
    void _testRemoveAllRebuildsTree();

    // Multiple inserts keep group persistent indexes valid
    void _testPersistentGroupIndexes();

    // recalcChildItems is a no-op for tree (no crash)
    void _testRecalcChildItemsNoCrash();

private:
    void _initForTest();

    std::unique_ptr<PlanMasterController> _masterController;
    MissionController* _missionController = nullptr;
};
