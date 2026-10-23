#pragma once

#include <QtCore/QString>

#include "QmlUITestBase.h"

class MissionController;
class PlanMasterController;

/// UI test for the tabbed Plan View panel (offline, no vehicle). The panel is behind the
/// PlanViewSettings useNewPlanPanel flag, so every test sets the flag it needs before starting the UI.
class PlanPanelUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    PlanPanelUITest() = default;

private slots:
    void _testFlagDefaultsToTabbedPanel();
    void _testFlagOffShowsTreeView();
    void _testEmptyPlanStartsOnPlanTab();
    void _testTabSelectionDrivesEditingLayer();
    void _testDefaultAltitudeChipOpensPlanTab();
    void _testMissionListFollowsItemsAndSelection();
    void _testItemsAddedOnPlanTabSwitchToMission();
    void _testIncompleteItemsAreFlagged();
    void _testDrillInShowsListOrInspector();
    void _testPanelHeightSelectsLayout();
    void _testDividerResizesListAndInspector();
    void _testItemMenuOpensFromInspector();
    void _testFenceTabAddsSelectsAndEditsFences();
    void _testFenceInspectorStepsAcrossEntriesInDrillIn();
    void _testRallyTabListsSelectsAndDeletesPoints();
    void _testRallyInspectorStepsInDrillIn();

private:
    /// Turns the panel flag on, boots the UI and opens the Plan view with the map at a known center
    void _startPlanPanel();

    /// Sets the plan home by clicking the map while the Plan tab is active, then opens the Mission tab
    void _setHomeAndOpenMission();

    /// Home set, then a takeoff plus \a waypointCount waypoints. Leaves the Mission tab active.
    void _buildMission(int waypointCount);

    bool _selectTab(const QString& tab);
    QString _currentTab();
    int _editingLayer();
    MissionController* _missionController();
    PlanMasterController* _planMasterController();
    QString _layoutMode();
    QString _tabStyle();
    void _clickMap(qreal fractionX, qreal fractionY);
};
