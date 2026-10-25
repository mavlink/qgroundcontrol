#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Verifies APMFirmwarePlugin::startMission behavior for each vehicle state:
/// flying vehicles just switch to Auto; a grounded fixed wing switches to Auto
/// and arms (no MAV_CMD_MISSION_START); other grounded vehicles arm in Guided
/// and start the mission via MAV_CMD_MISSION_START.
/// The async cases cover the multi-vehicle path, which sends the same commands
/// without waiting inside startMission().
class APMStartMissionTest : public VehicleTestManualConnect
{
    Q_OBJECT

protected slots:
    void init() override;

private slots:
    void _flyingCopterSwitchesToAuto();
    void _disarmedCopterArmsInGuidedThenMissionStart();
    void _armedCopterSendsMissionStart();
    void _disarmedPlaneSwitchesToAutoThenArms();
    void _armedPlaneSwitchesToAuto();
    void _flyingCopterAsyncSwitchesToAuto();
    void _flyingCopterAsyncAlreadyInAutoSendsNothing();
    void _armedCopterAsyncSendsMissionStart();
    void _disarmedCopterAsyncArmsInGuidedThenMissionStart();
    void _disarmedCopterAsyncAlreadyGuidedSkipsModeChange();
    void _armedPlaneAsyncSwitchesToAuto();
    void _disarmedPlaneAsyncSwitchesToAutoThenArms();
    void _disarmedPlaneAsyncAlreadyInAutoArmsWithoutModeChange();
    void _pauseAsyncSetsPauseModeThenSkipsRepeat();

private:
    void _connectAPMVehicle(bool plane);
    void _setArmedOnGround();
    void _verifyNoMissionStartSent();
};
