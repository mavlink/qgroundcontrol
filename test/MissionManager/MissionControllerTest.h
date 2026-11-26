#pragma once

#include <memory>

#include "MissionControllerManagerTest.h"
#include "UnitTest.h"

class MissionController;
class PlanMasterController;
class VisualMissionItem;

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT

public:
    MissionControllerTest();
    ~MissionControllerTest() override;

private slots:
    void cleanup() override;

    void _testLoadJsonSectionAvailable();
    void _testGlobalAltFrame();
    void _testFlightPathSegmentCacheReuse();
    void _testSplineWaypointPath();
    void _testSplineWaypointStraightWhenNotFlownAsSpline_data();
    void _testSplineWaypointStraightWhenNotFlownAsSpline();
    void _testSplineFollowsLoadedPlanVehicleType();
    void _testSplineTerrainProfile();
    void _testSplineAfterTakeoffStaysLevel();
    void _testSplineFromHomeStartsAtTakeoffAltitude();
    void _testLegFromHomeStartsAtTakeoffAltitude_data();
    void _testLegFromHomeStartsAtTakeoffAltitude();
    void _testReturnLegStaysLevel_data();
    void _testReturnLegStaysLevel();
    void _testSplineSampleCountGrowsWithLength();
    void _testSplineAfterLandHasNoIncomingVelocity();
    void _testDuplicateSplineWaypointCarriesNoVelocity();
    void _testSplineAfterComplexItemFollowsItsLastLeg();
    void _testTerrainSplineSamplesJoinEndpoints();
    void _testSplineSampleAltitudeUnknownWithUnknownEndpoint_data();
    void _testSplineSampleAltitudeUnknownWithUnknownEndpoint();
    void _testTerrainSplineBelowTerrainCollides();
    void _testSplineExitIgnoresNextRun();
    void _testNavigationStopBlocksSplineVelocity_data();
    void _testNavigationStopBlocksSplineVelocity();
    void _testReturnLegIsMeasuredStraight();
    void _testSplitSegmentTracksSegmentRebuild_data();
    void _testSplitSegmentTracksSegmentRebuild();
    void _testSplitSegmentWhenCurrentItemGainsCoordinate();
    void _testSplitSegmentNotOnLegFromHome();
    void _testGimbalRecalc();
    void _testVehicleYawRecalc();
    void _testLoiterTurnsDistance();
    void _testMissionReposition();
    void _testMissionOffset_data();
    void _testMissionOffset();
    void _testMissionRotate();
    void _testMissionTransformsInvalidHome();
    void _testLoadPlanRoundTripComplexItems();
    void _testInsertSurveyAppliesAltFrameInMixedMode();
    void _testInsertNonSurveyComplexItemMixedModeNoCrash();
    void _testInsertComplexItemFromKML();
    void _testInsertValidityHomePositionGating();
    void _testLandToolInsertsSingleRtl_data();
    void _testLandToolInsertsSingleRtl();
    void _testVTOLTakeoffModes_data();
    void _testVTOLTakeoffModes();
    void _testArduPilotVTOLOrdinaryTakeoffCompatibility();
    void _testUnsupportedVTOLMulticopterTakeoff();
    void _testVTOLMulticopterTakeoffAfterFixedWingTransition();
    void _testVTOLTakeoffJsonRoundTrip_data();
    void _testVTOLTakeoffJsonRoundTrip();
    void _testUnitsChangeDoesNotDirtyPlan_data();
    void _testUnitsChangeDoesNotDirtyPlan();
    void _testApplyDefaultMissionAltitudeOfferedOnlyForExistingItems_data();
    void _testApplyDefaultMissionAltitudeOfferedOnlyForExistingItems();
    void _testApplyDefaultMissionAltitudeSkipsItemsInOtherFrames_data();
    void _testApplyDefaultMissionAltitudeSkipsItemsInOtherFrames();

    // Parameterized tests - runs once per autopilot type
    UT_PARAMETERIZED_TEST(_testEmptyVehicle);

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _initForVehicleType(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType);
    void _startMasterController();
    void _setupVisualItemSignals(VisualMissionItem* visualItem);

    std::unique_ptr<PlanMasterController> _masterController;
    MissionController* _missionController = nullptr;
};
