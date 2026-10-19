#pragma once

#include "BaseClasses/VehicleTest.h"
#include "VehicleReadiness.h"

/// Readiness rules for each source, driven directly through VehicleReadiness::_evaluate
class VehicleReadinessTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEvaluate_data();
    void _testEvaluate();

private:
    struct EvaluateCase
    {
        const char* name = nullptr;
        VehicleReadiness::Inputs inputs{};
        VehicleReadiness::Result expected{};
    };

    static QList<EvaluateCase> _evaluateCases();
};

/// Readiness wiring on a PX4 vehicle, driven by libevents health reports
class VehicleReadinessPX4Test : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleReadinessPX4Test(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _testHealthReportReadiness();
    void _testWaitsForHealthReportDuringInitialConnect();
    void _testCurrentModeIntentionRefreshesReadiness();
    void _testUnmappedModeMakesReadinessUnknown();
    void _testMissingHealthReportFallsBack();
    void _testFirmwareOverridesActions();
};

/// Readiness wiring on an ArduPilot vehicle, driven by the SYS_STATUS prearm check bit, libevents health reports
/// and the checklist
class VehicleReadinessAPMTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    explicit VehicleReadinessAPMTest(QObject* parent = nullptr)
        : VehicleTestAPM(parent)
    {}

private slots:
    void _testPrearmCheckReadiness();
    void _testHealthReportUsesArduPilotActionModes();
    void _testChecklistBlocksArming();
};
