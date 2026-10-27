#pragma once

#include <QtCore/QString>

#include "QmlUITestBase.h"

class MissionController;
class PlanMasterController;

/// UI test for the tabbed Plan View panel. It is behind the
/// PlanViewSettings useNewPlanPanel flag, so every test sets the flag it needs before starting the UI.
/// Tests are offline unless they connect a MockLink.
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
    void _testItemMenuActsOnInspectedItem();
    void _testVehicleClassComboChangesToolStrip_data();
    void _testVehicleClassComboChangesToolStrip();
    void _testComplexItemEditorFitsInspector();
    void _testOpenedPlanShowsMissionTab();
    void _testConnectedVehicleFixesVehicleChoiceAndEnablesVehicleActions();
    void _testFenceTabAddsSelectsAndEditsFences();
    void _testFenceInspectorStepsAcrossEntriesInDrillIn();
    void _testRallyTabListsSelectsAndDeletesPoints();
    void _testRallyInspectorStepsInDrillIn();

private:
    /// Turns the panel flag on, boots the UI and opens the Plan view with the map at a known center
    void _startPlanPanel();

    /// Opens the Plan view of an already booted UI and puts the map at a known center
    void _openPlanView();

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
