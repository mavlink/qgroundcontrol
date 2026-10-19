#pragma once

#include "UnitTest.h"

/// Unit tests for HealthAndArmingCheckReport and its associated
/// HealthAndArmingCheckProblem QObject.
///
/// `update()` is driven end to end by a real EventHandler fed with the
/// HealthReport fixture events. The remaining tests cover the initial state,
/// the non-autopilot compid guard, reports in modes missing from the metadata,
/// warning acknowledgement (driven through the decoded-check rebuild), and the
/// HealthAndArmingCheckProblem value object.
class HealthAndArmingCheckReportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testUpdateSkipsNonAutopilotCompId();
    void _testFirstReportInUnmappedMode();
    void _testUpdateFromHealthReport_data();
    void _testUpdateFromHealthReport();
    void _testUnmappedModeKeepsResultsFresh();
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
