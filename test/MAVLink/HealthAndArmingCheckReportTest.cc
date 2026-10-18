#include "HealthAndArmingCheckReportTest.h"

#include "EventHandler.h"
#include "HealthAndArmingCheckReport.h"
#include "QmlObjectListModel.h"

#include "LibEvents.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

namespace {
/// Stub EventHandler for update() early-return tests (callbacks are never invoked).
EventHandler makeStubEventHandler()
{
    return EventHandler(
        /*parent*/ nullptr,
        /*profile*/ QString(),
        /*handleEventCB*/ [](auto) {},
        /*sendRequestCB*/ [](auto&) {},
        /*ourSystemId*/ 1, /*ourComponentId*/ 1,
        /*systemId*/ 1, /*componentId*/ 1);
}
} // namespace

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
    QCOMPARE(report.errorCount(), 0);
    QCOMPARE(report.warningCount(), 0);
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

    // The compid guard short-circuits before the handler is dereferenced, so a
    // stub EventHandler is fine. Using MAV_COMP_ID_CAMERA (100) proves we only
    // accept AUTOPILOT1.
    EventHandler stubHandler = makeStubEventHandler();
    report.update(/*compid*/ 100, stubHandler, /*flightModeGroup*/ 0);

    QCOMPARE(updatedSpy.count(), 0);
    QVERIFY(!report.supported());
    QCOMPARE(report.problemsForCurrentMode()->count(), 0);
}

void HealthAndArmingCheckReportTest::_testUpdateSkipsUninitializedFlightModeGroup()
{
    HealthAndArmingCheckReport report;

    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);

    // The report logs a warning and bails when flightModeGroup == -1.
    expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport", QtWarningMsg, QRegularExpression(QStringLiteral("Flight mode group not set")));
    EventHandler stubHandler = makeStubEventHandler();
    report.update(MAV_COMP_ID_AUTOPILOT1, stubHandler, /*flightModeGroup*/ -1);
    verifyExpectedLogMessage();

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

void HealthAndArmingCheckReportTest::_testSeverityCounts_data()
{
    QTest::addColumn<QStringList>("severities");
    QTest::addColumn<int>("expectedErrors");
    QTest::addColumn<int>("expectedWarnings");

    QTest::newRow("empty") << QStringList{} << 0 << 0;
    QTest::newRow("errorsOnly") << QStringList{"error", "error"} << 2 << 0;
    QTest::newRow("warningsOnly") << QStringList{"warning"} << 0 << 1;
    QTest::newRow("mixed") << QStringList{"warning", "error", "", "warning"} << 1 << 2;
    QTest::newRow("infoOnly") << QStringList{""} << 0 << 0;
}

void HealthAndArmingCheckReportTest::_testSeverityCounts()
{
    QFETCH(QStringList, severities);
    QFETCH(int, expectedErrors);
    QFETCH(int, expectedWarnings);

    HealthAndArmingCheckReport report;
    for (const QString& severity : severities) {
        report.problemsForCurrentMode()->append(
            new HealthAndArmingCheckProblem(QStringLiteral("Problem"), QString(), severity, &report));
    }

    QCOMPARE(report.errorCount(), expectedErrors);
    QCOMPARE(report.warningCount(), expectedWarnings);
}

void HealthAndArmingCheckReportTest::_testAcknowledgeAllWarnings_data()
{
    QTest::addColumn<QStringList>("severities");
    QTest::addColumn<int>("expectedErrors");
    QTest::addColumn<bool>("expectedHasWarningsOrErrors");

    QTest::newRow("warningsOnly") << QStringList{"warning", "warning"} << 0 << false;
    QTest::newRow("errorsNotAcknowledged") << QStringList{"warning", "error"} << 1 << true;
    QTest::newRow("infoUnaffected") << QStringList{"", "warning"} << 0 << false;
}

void HealthAndArmingCheckReportTest::_testAcknowledgeAllWarnings()
{
    QFETCH(QStringList, severities);
    QFETCH(int, expectedErrors);
    QFETCH(bool, expectedHasWarningsOrErrors);

    HealthAndArmingCheckReport report;
    QList<HealthAndArmingCheckReport::Check> checks;
    for (const QString& severity : severities) {
        checks.append({.ackKey = static_cast<quint64>(checks.count()), .severity = severity, .inCurrentMode = true});
    }
    report._rebuild(checks);
    QVERIFY(report.hasWarningsOrErrors());

    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);
    report.acknowledgeAllWarnings();

    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(report.warningCount(), 0);
    QCOMPARE(report.errorCount(), expectedErrors);
    QCOMPARE(report.hasWarningsOrErrors(), expectedHasWarningsOrErrors);

    // Acknowledged warnings stay listed
    QmlObjectListModel* const problems = report.problemsForCurrentMode();
    QCOMPARE(problems->count(), severities.count());
    for (int i = 0; i < problems->count(); i++) {
        const auto* const problem = problems->value<const HealthAndArmingCheckProblem*>(i);
        QCOMPARE(problem->acknowledged(), problem->severity() == QStringLiteral("warning"));
    }
}

void HealthAndArmingCheckReportTest::_testAcknowledgementLastsWhileStillWarning_data()
{
    QTest::addColumn<bool>("nextPresent");
    QTest::addColumn<QString>("nextMessage");
    QTest::addColumn<QString>("nextSeverity");
    QTest::addColumn<bool>("nextInCurrentMode");
    QTest::addColumn<bool>("acknowledgedOnReturn");

    QTest::newRow("unchanged") << true << QStringLiteral("Wind 10 m/s") << QStringLiteral("warning") << true << true;
    QTest::newRow("valuesChanged") << true << QStringLiteral("Wind 12 m/s") << QStringLiteral("warning") << true
                                   << true;
    QTest::newRow("otherModeOnly") << true << QStringLiteral("Wind 10 m/s") << QStringLiteral("warning") << false
                                   << true;
    QTest::newRow("becameError") << true << QStringLiteral("Wind 10 m/s") << QStringLiteral("error") << true << false;
    QTest::newRow("becameInfo") << true << QStringLiteral("Wind 10 m/s") << QString() << true << false;
    QTest::newRow("resolved") << false << QString() << QString() << false << false;
}

void HealthAndArmingCheckReportTest::_testAcknowledgementLastsWhileStillWarning()
{
    QFETCH(bool, nextPresent);
    QFETCH(QString, nextMessage);
    QFETCH(QString, nextSeverity);
    QFETCH(bool, nextInCurrentMode);
    QFETCH(bool, acknowledgedOnReturn);

    constexpr quint64 ackKey = 42;
    const HealthAndArmingCheckReport::Check warning{.ackKey = ackKey,
                                                    .message = QStringLiteral("Wind 10 m/s"),
                                                    .severity = QStringLiteral("warning"),
                                                    .inCurrentMode = true};

    HealthAndArmingCheckReport report;
    report._rebuild({warning});
    report.acknowledgeAllWarnings();
    QCOMPARE(report.warningCount(), 0);

    QList<HealthAndArmingCheckReport::Check> nextChecks;
    if (nextPresent) {
        nextChecks.append(
            {.ackKey = ackKey, .message = nextMessage, .severity = nextSeverity, .inCurrentMode = nextInCurrentMode});
    }
    report._rebuild(nextChecks);
    report._rebuild({warning});

    QCOMPARE(report.warningCount(), acknowledgedOnReturn ? 0 : 1);
    QCOMPARE(report.hasWarningsOrErrors(), !acknowledgedOnReturn);
    QCOMPARE(report.problemsForCurrentMode()->value<const HealthAndArmingCheckProblem*>(0)->acknowledged(),
             acknowledgedOnReturn);
}

void HealthAndArmingCheckReportTest::_testNewWarningNotAcknowledged()
{
    using Check = HealthAndArmingCheckReport::Check;
    const Check first{.ackKey = 1, .severity = QStringLiteral("warning"), .inCurrentMode = true};
    const Check second{.ackKey = 2, .severity = QStringLiteral("warning"), .inCurrentMode = true};

    HealthAndArmingCheckReport report;
    report._rebuild({first});
    report.acknowledgeAllWarnings();
    report._rebuild({first, second});

    QCOMPARE(report.warningCount(), 1);
    QVERIFY(report.hasWarningsOrErrors());
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
