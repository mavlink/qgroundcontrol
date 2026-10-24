#pragma once

#include "VehicleConfigUITestBase.h"

/// UI tests for the PX4 Battery setup page with a PX4 MockLink vehicle connected
class PX4BatteryUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _lateParamShownAndFilterChosenOnceLoaded();
    void _filterTogglesUnusedBattery();
    void _unclaimedBatteryNoteDoesNotWidenPage();
    void _rowSeparatorsFollowTheShownRows();
    void _reportedBatteryDotFollowsTheReportedVoltage();
    void _setupCompleteIgnoresCellsForExternalBattery();

private:
    /// Sets BAT1_SOURCE and accepts the reboot notice that follows. Check QTest::currentTestFailed() afterwards.
    void _setBattery1Source(Vehicle* vehicle, int source);

    /// Opens the Battery page from the Fly view. Check QTest::currentTestFailed() afterwards.
    void _openBatteryPage();
};
