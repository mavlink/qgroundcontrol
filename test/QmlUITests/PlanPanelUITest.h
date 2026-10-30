#pragma once

#include <QtCore/QString>

#include "QmlUITestBase.h"

class MissionController;
class PlanMasterController;
class QQuickItem;

/// UI test for the tabbed Plan View panel. It is behind the
/// PlanViewSettings useNewPlanPanel flag, so every test sets the flag it needs before starting the UI.
/// Tests are offline unless they connect a MockLink.
class PlanPanelUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    PlanPanelUITest() = default;

private slots:
    void _testFlagDefaults();
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
    void _testGeoMapMarkerClickSelectsItem();
    void _testGeoMapMarkerShowsWhileTerrainAltitudePending();
    void _testGeoMapTerrainRelativeDragKeepsTheMarkerUnderTheCursor_data();
    void _testGeoMapTerrainRelativeDragKeepsTheMarkerUnderTheCursor();
    void _testGeoMapMissionPathReturnsToPlannedHome();
    void _testFenceTabAddsFenceInsideTheGeoMapViewport_data();
    void _testFenceTabAddsFenceInsideTheGeoMapViewport();
    void _testFenceTabAddsFenceInsideTheViewportOverTheDateline_data();
    void _testFenceTabAddsFenceInsideTheViewportOverTheDateline();
    void _testGeoMapKeepsTabbedPanelWhenTheSettingIsTurnedOff();
    void _testGeoMapDoesNotRecenterOnOffscreenVehicleInPlanView();
    void _testGeoMapControlsReceiveClicksOverMissionMarkers();
    void _testGeoMapDimsMissionOutsideTheMissionLayer();
    void _testGeoMapDimsLandingPatternOutsideTheMissionLayer();
    void _testGeoMapMarkerClickOutsideTheMissionLayerReachesTheMap();
    void _testGeoMapMarkerDragMovesOnlyTheCurrentItem_data();
    void _testGeoMapMarkerDragMovesOnlyTheCurrentItem();
    void _testGeoMapRallyMarkerClickSelectsPoint();
    void _testGeoMapRallyMarkerDragMovesOnlyTheCurrentPoint_data();
    void _testGeoMapRallyMarkerDragMovesOnlyTheCurrentPoint();
    void _testGeoMapRallyMarkerHeightFollowsFirmwareAndFrame_data();
    void _testGeoMapRallyMarkerHeightFollowsFirmwareAndFrame();
    void _testGeoMapRallyGroundMarkingMatchesMarkerSize_data();
    void _testGeoMapRallyGroundMarkingMatchesMarkerSize();
    void _testGeoMapModeButtonTogglesCameraMode();
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
    /// Turns the panel flag on, boots the UI and opens the Plan view with the map at a known center.
    /// \a geoMapEngine selects the GeoMap engine for the Plan map instead of QtLocation.
    void _startPlanPanel(bool geoMapEngine = false);

    /// Opens the Plan view of an already booted UI and puts the map at a known center
    void _openPlanView();

    /// Sets the plan home by clicking the map while the Plan tab is active, then opens the Mission tab
    void _setHomeAndOpenMission();

    /// Home set, then a takeoff plus \a waypointCount waypoints. Leaves the Mission tab active.
    void _buildMission(int waypointCount);

    /// Map click on the Plan tab sets home, which the other tabs need
    void _setHome();

    /// Home set, then one rally point added at the map center on the GeoMap engine. \a marker is its map marker.
    void _addGeoMapRallyPoint(QQuickItem*& marker);

    bool _selectTab(const QString& tab);
    QString _currentTab();
    int _editingLayer();
    MissionController* _missionController();
    PlanMasterController* _planMasterController();
    QString _layoutMode();
    QString _tabStyle();
    void _clickMap(qreal fractionX, qreal fractionY);
};
