#include "HealthAndArmingCheckReportTest.h"

#include "HealthAndArmingCheckReport.h"
#include "QmlObjectListModel.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

// ---------------------------------------------------------------------------
// HealthAndArmingCheckReport
// ---------------------------------------------------------------------------

void HealthAndArmingCheckReportTest::_testInitialState()
{
    HealthAndArmingCheckReport report;

    // Fresh report: nothing has been reported by any vehicle yet.
    QVERIFY(!report.supported());

    // Default capability bits are OPEN — the UI assumes arming/takeoff/mission
    // are allowed until a report explicitly denies them. This keeps a vehicle
    // that doesn't publish arming checks usable.
    QVERIFY(report.canArm());
    QVERIFY(report.canTakeoff());
    QVERIFY(report.canStartMission());

    QVERIFY(!report.hasWarningsOrErrors());
    QCOMPARE(report.gpsState(), QString());

    // The problems model is owned by the report and starts empty.
    QmlObjectListModel* problems = report.problemsForCurrentMode();
    QVERIFY(problems);
    QCOMPARE(problems->count(), 0);
}

void HealthAndArmingCheckReportTest::_testUpdateSkipsNonAutopilotCompId()
{
    HealthAndArmingCheckReport report;

    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);

    // Passing an empty Results object is fine here because the compid guard
    // short-circuits before the results are touched. Using MAV_COMP_ID_CAMERA
    // (100) proves we only accept AUTOPILOT1.
    events::HealthAndArmingChecks::Results results{};
    report.update(/*compid*/ 100, results, /*flightModeGroup*/ 0);

    QCOMPARE(updatedSpy.count(), 0);
    QVERIFY(!report.supported());
    QCOMPARE(report.problemsForCurrentMode()->count(), 0);
}

void HealthAndArmingCheckReportTest::_testUpdateSkipsUninitializedFlightModeGroup()
{
    HealthAndArmingCheckReport report;

    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);

    // The report logs a warning and bails when flightModeGroup == -1. Use
    // UnitTest::expectLogMessage (not QTest::ignoreMessage) so the
    // "uncategorized log messages" lint in the QGC test harness passes.
    expectLogMessage(QtWarningMsg,
                     QRegularExpression(QStringLiteral("Flight mode group not set")));
    events::HealthAndArmingChecks::Results results{};
    report.update(MAV_COMP_ID_AUTOPILOT1, results, /*flightModeGroup*/ -1);

    QCOMPARE(updatedSpy.count(), 0);
    QVERIFY(!report.supported());
    QCOMPARE(report.problemsForCurrentMode()->count(), 0);
}

void HealthAndArmingCheckReportTest::_testSetModeGroups()
{
    // setModeGroups() has no public getter, but the behavior it drives is
    // exercised indirectly by update(): _canTakeoff and _canStartMission are
    // only re-evaluated when their respective mode groups are != -1.
    //
    // We assert the well-formed no-arg case here: setting both mode groups
    // must not mutate any observable report state on its own.
    HealthAndArmingCheckReport report;

    const bool beforeCanArm         = report.canArm();
    const bool beforeCanTakeoff     = report.canTakeoff();
    const bool beforeCanMission     = report.canStartMission();

    report.setModeGroups(/*takeoff*/ 0, /*mission*/ 1);

    QCOMPARE(report.canArm(),          beforeCanArm);
    QCOMPARE(report.canTakeoff(),      beforeCanTakeoff);
    QCOMPARE(report.canStartMission(), beforeCanMission);
}

// ---------------------------------------------------------------------------
// HealthAndArmingCheckProblem
// ---------------------------------------------------------------------------

void HealthAndArmingCheckReportTest::_testProblemConstruction()
{
    HealthAndArmingCheckProblem problem(
        QStringLiteral("No global position"),
        QStringLiteral("GPS fix lost for 10s"),
        QStringLiteral("error"));

    QCOMPARE(problem.message(),     QStringLiteral("No global position"));
    QCOMPARE(problem.description(), QStringLiteral("GPS fix lost for 10s"));
    QCOMPARE(problem.severity(),    QStringLiteral("error"));
    QVERIFY(!problem.expanded());
}

void HealthAndArmingCheckReportTest::_testProblemExpandedSignal()
{
    HealthAndArmingCheckProblem problem(QStringLiteral("Low battery"),
                                        QStringLiteral(""),
                                        QStringLiteral("warning"));

    QSignalSpy expandedSpy(&problem, &HealthAndArmingCheckProblem::expandedChanged);

    // HealthAndArmingCheckProblem::setExpanded always emits the signal (no
    // dedup) — callers rely on re-setting the same value to force a refresh.
    problem.setExpanded(true);
    QVERIFY(problem.expanded());
    QCOMPARE(expandedSpy.count(), 1);

    problem.setExpanded(false);
    QVERIFY(!problem.expanded());
    QCOMPARE(expandedSpy.count(), 2);
}

UT_REGISTER_TEST(HealthAndArmingCheckReportTest, TestLabel::Unit)
