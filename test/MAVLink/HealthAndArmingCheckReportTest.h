#pragma once

#include "UnitTest.h"

/// Unit tests for HealthAndArmingCheckReport and its associated
/// HealthAndArmingCheckProblem QObject.
///
/// The `update()` method consumes a libevents `Results` struct, which is
/// expensive to synthesise outside the real MAVLink LibEvents parser, so this
/// test suite focuses on the properties that are observable without invoking
/// the libevents path: the default/initial state, early-return guards
/// (non-autopilot compid, uninitialised flightModeGroup), setModeGroups
/// wiring, warning acknowledgement (driven through the decoded-check rebuild),
/// and the HealthAndArmingCheckProblem value object.
class HealthAndArmingCheckReportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testUpdateSkipsNonAutopilotCompId();
    void _testUpdateSkipsUninitializedFlightModeGroup();
    void _testSetModeGroups();
    void _testSeverityCounts_data();
    void _testSeverityCounts();
    void _testAcknowledgeAllWarnings_data();
    void _testAcknowledgeAllWarnings();
    void _testAcknowledgementLastsWhileStillWarning_data();
    void _testAcknowledgementLastsWhileStillWarning();
    void _testNewWarningNotAcknowledged();

    void _testProblemConstruction();
    void _testProblemExpandedSignal();
};
