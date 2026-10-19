#include "HealthAndArmingCheckReportTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "EventHandler.h"
#include "HealthAndArmingCheckReport.h"
#include "HealthReportFixtures.h"
#include "LibEvents.h"
#include "QmlObjectListModel.h"

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

    const HealthAndArmingCheckReport::ModeGroupResults& modeGroupResults = report.modeGroupResults();
    QCOMPARE(modeGroupResults.currentModeGroup, -1);
    QCOMPARE(modeGroupResults.canArm, 0ull);
    QCOMPARE(modeGroupResults.canRun, 0ull);

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

void HealthAndArmingCheckReportTest::_testFirstReportInUnmappedMode()
{
    using namespace TestFixtures::HealthReport;

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile = writeMetadata(metadataDir.path(), 10, 20, 30);
    QVERIFY(!metadataFile.isEmpty());

    EventHandler handler(
        nullptr, QStringLiteral("dev"), [](auto) {}, [](auto&) {}, 255, MAV_COMP_ID_MISSIONPLANNER, 1,
        MAV_COMP_ID_AUTOPILOT1);
    handler.setMetadata(metadataFile);
    EventStream eventStream(1);
    const uint32_t canArmModeGroups = modeGroupBit(ManualModeGroup) | modeGroupBit(MissionModeGroup);
    for (const mavlink_message_t& message : eventStream.report(canArmModeGroups, AllModeGroups)) {
        handler.handleEvents(message);
    }

    // A vehicle connecting in a mode missing from the metadata still has its report used
    HealthAndArmingCheckReport report;
    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);
    expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Flight mode group not set")));
    report.update(MAV_COMP_ID_AUTOPILOT1, handler, -1);
    verifyExpectedLogMessage();

    QCOMPARE(updatedSpy.count(), 1);
    QVERIFY(report.supported());
    QCOMPARE(report.modeGroupResults().currentModeGroup, -1);
    QCOMPARE(report.modeGroupResults().canArm, quint64(canArmModeGroups));
    QCOMPARE(report.modeGroupResults().canRun, quint64(AllModeGroups));

    // Without a current mode, checks for every mode are listed
    QCOMPARE(report.problemsForCurrentMode()->count(), 1);
    QCOMPARE(report.errorCount(), 1);
}

void HealthAndArmingCheckReportTest::_testUpdateFromHealthReport_data()
{
    using namespace TestFixtures::HealthReport;

    QTest::addColumn<uint>("canArmModeGroups");
    QTest::addColumn<uint>("canRunModeGroups");
    QTest::addColumn<int>("expectedCurrentModeErrors");

    QTest::newRow("allReady") << uint(AllModeGroups) << uint(AllModeGroups) << 0;
    QTest::newRow("otherModeBlocked") << uint(modeGroupBit(ManualModeGroup) | modeGroupBit(MissionModeGroup))
                                      << uint(AllModeGroups & ~modeGroupBit(MissionModeGroup)) << 0;
    QTest::newRow("currentModeBlocked") << uint(modeGroupBit(TakeoffModeGroup)) << uint(AllModeGroups) << 1;
}

void HealthAndArmingCheckReportTest::_testUpdateFromHealthReport()
{
    using namespace TestFixtures::HealthReport;

    QFETCH(uint, canArmModeGroups);
    QFETCH(uint, canRunModeGroups);
    QFETCH(int, expectedCurrentModeErrors);

    constexpr uint32_t manualCustomMode = 10;
    constexpr uint32_t takeoffCustomMode = 20;
    constexpr uint32_t missionCustomMode = 30;

    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile =
        writeMetadata(metadataDir.path(), manualCustomMode, takeoffCustomMode, missionCustomMode);
    QVERIFY(!metadataFile.isEmpty());

    EventHandler handler(
        nullptr, QStringLiteral("dev"), [](auto) {}, [](auto&) {}, 255, MAV_COMP_ID_MISSIONPLANNER, 1,
        MAV_COMP_ID_AUTOPILOT1);
    handler.setMetadata(metadataFile);
    QVERIFY(handler.healthAndArmingChecksSupported());
    QCOMPARE(handler.getModeGroup(manualCustomMode), ManualModeGroup);
    QCOMPARE(handler.getModeGroup(takeoffCustomMode), TakeoffModeGroup);
    QCOMPARE(handler.getModeGroup(missionCustomMode), MissionModeGroup);

    HealthAndArmingCheckReport report;

    QSignalSpy checksUpdatedSpy(&handler, &EventHandler::healthAndArmingChecksUpdated);
    EventStream eventStream(1);
    for (const mavlink_message_t& message : eventStream.report(canArmModeGroups, canRunModeGroups)) {
        handler.handleEvents(message);
    }
    QCOMPARE(checksUpdatedSpy.count(), 1);

    report.update(MAV_COMP_ID_AUTOPILOT1, handler, handler.getModeGroup(manualCustomMode));

    QVERIFY(report.supported());
    const HealthAndArmingCheckReport::ModeGroupResults& modeGroupResults = report.modeGroupResults();
    QCOMPARE(modeGroupResults.currentModeGroup, ManualModeGroup);
    QCOMPARE(modeGroupResults.canArm, quint64(canArmModeGroups));
    QCOMPARE(modeGroupResults.canRun, quint64(canRunModeGroups));
    QCOMPARE(report.errorCount(), expectedCurrentModeErrors);
}

void HealthAndArmingCheckReportTest::_testUnmappedModeKeepsResultsFresh()
{
    using namespace TestFixtures::HealthReport;

    constexpr uint32_t manualCustomMode = 10;
    QTemporaryDir metadataDir;
    QVERIFY(metadataDir.isValid());
    const QString metadataFile = writeMetadata(metadataDir.path(), manualCustomMode, 20, 30);
    QVERIFY(!metadataFile.isEmpty());

    EventHandler handler(
        nullptr, QStringLiteral("dev"), [](auto) {}, [](auto&) {}, 255, MAV_COMP_ID_MISSIONPLANNER, 1,
        MAV_COMP_ID_AUTOPILOT1);
    handler.setMetadata(metadataFile);
    EventStream eventStream(1);
    const auto sendReport = [&handler, &eventStream](uint32_t canArmModeGroups, uint32_t canRunModeGroups) {
        for (const mavlink_message_t& message : eventStream.report(canArmModeGroups, canRunModeGroups)) {
            handler.handleEvents(message);
        }
    };

    sendReport(AllModeGroups, AllModeGroups);
    HealthAndArmingCheckReport report;
    report.update(MAV_COMP_ID_AUTOPILOT1, handler, handler.getModeGroup(manualCustomMode));
    QCOMPARE(report.modeGroupResults().currentModeGroup, ManualModeGroup);

    // A mode missing from the metadata must not leave the previous mode's group behind
    QSignalSpy updatedSpy(&report, &HealthAndArmingCheckReport::updated);
    expectLogMessage("MAVLink.LibEvents.HealthAndArmingCheckReport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Flight mode group not set")));
    report.update(MAV_COMP_ID_AUTOPILOT1, handler, -1);
    verifyExpectedLogMessage();
    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(report.modeGroupResults().currentModeGroup, -1);

    // Later reports in the same unmapped mode still refresh the results, without warning again
    const uint32_t canArmModeGroups = modeGroupBit(ManualModeGroup);
    const uint32_t canRunModeGroups = modeGroupBit(ManualModeGroup) | modeGroupBit(TakeoffModeGroup);
    sendReport(canArmModeGroups, canRunModeGroups);
    report.update(MAV_COMP_ID_AUTOPILOT1, handler, -1);
    QCOMPARE(updatedSpy.count(), 2);
    QCOMPARE(report.modeGroupResults().currentModeGroup, -1);
    QCOMPARE(report.modeGroupResults().canArm, quint64(canArmModeGroups));
    QCOMPARE(report.modeGroupResults().canRun, quint64(canRunModeGroups));
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
