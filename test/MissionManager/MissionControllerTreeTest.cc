#include "MissionControllerTreeTest.h"

#include "AppSettings.h"
#include "MissionController.h"
#include "MissionSettingsItem.h"
#include "PlanMasterController.h"
#include "QmlObjectTreeModel.h"
#include "SettingsManager.h"
#include "SimpleMissionItem.h"
#include "TestFixtures.h"

using namespace TestFixtures;

MissionControllerTreeTest::~MissionControllerTreeTest() = default;

void MissionControllerTreeTest::cleanup()
{
    _masterController.reset();
    _missionController = nullptr;
    MissionControllerManagerTest::cleanup();
}

void MissionControllerTreeTest::_initForTest()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(
        QGCMAVLink::firmwareClass(MAV_AUTOPILOT_PX4));

    _masterController = std::make_unique<PlanMasterController>();
    _masterController->setFlyView(false);
    _missionController = _masterController->missionController();

    SignalSpyFixture spy(_missionController);
    QVERIFY(spy.spy());
    spy.expect("visualItemsChanged");
    _masterController->start();
    QVERIFY(spy.waitAndVerify(TestTimeout::mediumMs()));

    // Sanity — visualItems has MissionSettingsItem at index 0
    QVERIFY(_missionController->visualItems());
    QCOMPARE(_missionController->visualItems()->count(), 1);
}

// ===========================================================================
// Tree structure after init
// ===========================================================================

void MissionControllerTreeTest::_testTreeStructureAfterInit()
{
    _initForTest();

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();
    QVERIFY(tree);

    // Root should have 3 top-level groups: Mission Items, GeoFence, Rally Points
    QCOMPARE(tree->rowCount(), 3);

    // Verify group node types
    const QModelIndex missionGroup = tree->index(0, 0);
    const QModelIndex fenceGroup   = tree->index(1, 0);
    const QModelIndex rallyGroup   = tree->index(2, 0);
    QVERIFY(missionGroup.isValid());
    QVERIFY(fenceGroup.isValid());
    QVERIFY(rallyGroup.isValid());

    QCOMPARE(tree->data(missionGroup, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("missionGroup"));
    QCOMPARE(tree->data(fenceGroup, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("fenceGroup"));
    QCOMPARE(tree->data(rallyGroup, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("rallyGroup"));

    // Mission group should contain 1 child (the MissionSettingsItem)
    QCOMPARE(tree->rowCount(missionGroup), 1);

    // Fence group should have 1 child (fenceEditor marker)
    QCOMPARE(tree->rowCount(fenceGroup), 1);
    const QModelIndex fenceChild = tree->index(0, 0, fenceGroup);
    QCOMPARE(tree->data(fenceChild, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("fenceEditor"));

    // Rally group should have 1 child (rallyHeader marker)
    QCOMPARE(tree->rowCount(rallyGroup), 1);
    const QModelIndex rallyChild = tree->index(0, 0, rallyGroup);
    QCOMPARE(tree->data(rallyChild, QmlObjectTreeModel::NodeTypeRole).toString(), QStringLiteral("rallyHeader"));
}

// ===========================================================================
// Insert waypoints → tree sync
// ===========================================================================

void MissionControllerTreeTest::_testInsertWaypointUpdatesTree()
{
    _initForTest();

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();
    const QModelIndex missionGroup = tree->index(0, 0);

    // Before insert: 1 mission item (MissionSettingsItem)
    const int initialMissionChildren = tree->rowCount(missionGroup);
    QCOMPARE(initialMissionChildren, _missionController->visualItems()->count());

    // Insert 3 waypoints
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 3);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }

    // _visualItems should have 4 items (settings + 3 waypoints)
    QCOMPARE(_missionController->visualItems()->count(), 4);

    // Tree's mission group should mirror that count
    QCOMPARE(tree->rowCount(missionGroup), 4);

    // Verify tree children correspond to visualItems
    for (int i = 0; i < _missionController->visualItems()->count(); i++) {
        QObject* listObj = _missionController->visualItems()->get(i);
        const QModelIndex treeChild = tree->index(i, 0, missionGroup);
        QObject* treeObj = tree->data(treeChild, Qt::UserRole).value<QObject*>();
        QCOMPARE(treeObj, listObj);
    }
}

// ===========================================================================
// Remove waypoints → tree sync
// ===========================================================================

void MissionControllerTreeTest::_testRemoveWaypointUpdatesTree()
{
    _initForTest();

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();
    const QModelIndex missionGroup = tree->index(0, 0);

    // Insert 3 waypoints
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 3);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }
    QCOMPARE(tree->rowCount(missionGroup), 4); // settings + 3 waypoints

    // Remove middle waypoint (visualItem index 2)
    _missionController->removeVisualItem(2);

    QCOMPARE(_missionController->visualItems()->count(), 3);
    QCOMPARE(tree->rowCount(missionGroup), 3);

    // Verify correspondence
    for (int i = 0; i < _missionController->visualItems()->count(); i++) {
        QObject* listObj = _missionController->visualItems()->get(i);
        const QModelIndex treeChild = tree->index(i, 0, missionGroup);
        QObject* treeObj = tree->data(treeChild, Qt::UserRole).value<QObject*>();
        QCOMPARE(treeObj, listObj);
    }
}

// ===========================================================================
// removeAll → tree rebuilt
// ===========================================================================

void MissionControllerTreeTest::_testRemoveAllRebuildsTree()
{
    _initForTest();

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();

    // Insert waypoints
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 3);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }

    const QModelIndex missionGroupBefore = tree->index(0, 0);
    QCOMPARE(tree->rowCount(missionGroupBefore), 4);

    // Remove all
    _missionController->removeAll();

    // After removeAll, tree should still have 3 groups
    QCOMPARE(tree->rowCount(), 3);

    // Mission group should have 1 child (the new MissionSettingsItem)
    const QModelIndex missionGroupAfter = tree->index(0, 0);
    QCOMPARE(tree->rowCount(missionGroupAfter), 1);

    // That child should be the new settings item
    const QModelIndex settingsChild = tree->index(0, 0, missionGroupAfter);
    QObject* settingsObj = tree->data(settingsChild, Qt::UserRole).value<QObject*>();
    QCOMPARE(settingsObj, static_cast<QObject*>(_missionController->visualItems()->get(0)));
}

// ===========================================================================
// Persistent group indexes remain valid across operations
// ===========================================================================

void MissionControllerTreeTest::_testPersistentGroupIndexes()
{
    _initForTest();

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();

    // Capture persistent indexes for each group
    QPersistentModelIndex missionPersist(tree->index(0, 0));
    QPersistentModelIndex fencePersist(tree->index(1, 0));
    QPersistentModelIndex rallyPersist(tree->index(2, 0));

    // Insert waypoints
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 5);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }

    // All persistent indexes should still be valid
    QVERIFY(missionPersist.isValid());
    QVERIFY(fencePersist.isValid());
    QVERIFY(rallyPersist.isValid());

    // They should still be at the same rows (group structure didn't change)
    QCOMPARE(missionPersist.row(), 0);
    QCOMPARE(fencePersist.row(), 1);
    QCOMPARE(rallyPersist.row(), 2);

    // Remove some waypoints
    _missionController->removeVisualItem(3);
    _missionController->removeVisualItem(2);

    // Persistent indexes should still be valid
    QVERIFY(missionPersist.isValid());
    QVERIFY(fencePersist.isValid());
    QVERIFY(rallyPersist.isValid());
}

// ===========================================================================
// recalcChildItems is a no-op for the tree (no crash)
// ===========================================================================

void MissionControllerTreeTest::_testRecalcChildItemsNoCrash()
{
    _initForTest();

    // Insert enough waypoints to exercise child item recalc
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 4);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }

    QmlObjectTreeModel* tree = _missionController->visualItemsTree();
    const QModelIndex missionGroup = tree->index(0, 0);

    // Tree should be consistent — recalcChildItems happens automatically during insert
    QCOMPARE(tree->rowCount(missionGroup), _missionController->visualItems()->count());

    // Verify no crash and tree is still functional
    QVERIFY(tree->index(0, 0, missionGroup).isValid());
    QCOMPARE(tree->rowCount(), 3); // 3 groups
}

#include "UnitTest.h"

UT_REGISTER_TEST(MissionControllerTreeTest, TestLabel::Integration, TestLabel::MissionManager)
