#include "ParameterEditorControllerTest.h"

#include <QtCore/QString>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include "ParameterEditorController.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "Fixtures/RAIIFixtures.h"
#include "UnitTest.h"

UT_REGISTER_TEST(ParameterEditorControllerTest, TestLabel::Integration, TestLabel::Vehicle)

// QGC tab-delimited file with SYS_AUTOSTART changed from 4001 → 4002
static const char* kQGCParamsWithDiff =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4002\t6\n";

// QGC tab-delimited file with SYS_AUTOSTART matching vehicle default (4001)
static const char* kQGCParamsNoDiff =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4001\t6\n";

// Mission Planner 2-column file with same diff
static const char* kMPParamsWithDiff =
    "# Mission Planner parameter file\n"
    "SYS_AUTOSTART,4002\n";

// Mission Planner 2-column file matching vehicle default
static const char* kMPParamsNoDiff =
    "# Mission Planner parameter file\n"
    "SYS_AUTOSTART,4001\n";

// Mission Planner 2-column file with a parameter not on the vehicle
static const char* kMPParamsMissingParam =
    "# Mission Planner parameter file\n"
    "NO_SUCH_PARAM,1\n";

// Garbage — no parseable lines
static const char* kBadFormatParams =
    "This is not a parameter file\n"
    "random text here\n";

// QGC file with a parameter that does not exist on the (PX4 mock) vehicle
static const char* kQGCParamsUnknownParam =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tNO_SUCH_PARAM\t1\t6\n";

// Mission Planner file mixing a changed, an unchanged and a missing parameter
static const char* kMPParamsMixed =
    "# Mission Planner parameter file\n"
    "SYS_AUTOSTART,4002\n"
    "COM_DL_LOSS_T,10\n"
    "NO_SUCH_PARAM,1\n";

// Verify the diff summary counter invariant: parsed == diffList (sendable + cannot-send) + unchanged + readOnly.
// MP-format missing params are included in diffList as cannot-send rows.
static void verifyDiffInvariant(ParameterEditorController& controller)
{
    QCOMPARE(controller.property("diffParsedCount").toInt(),
             controller.diffList()->count()
                 + controller.property("diffUnchangedCount").toInt()
                 + controller.property("diffReadOnlyCount").toInt());
}

void ParameterEditorControllerTest::_buildDiffQGCFormat()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsWithDiff)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QCOMPARE(diff->name, QStringLiteral("SYS_AUTOSTART"));
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 0);
    QCOMPARE(controller.property("diffNoVehicleCount").toInt(), 0);
    QVERIFY(controller.property("diffMissingParams").toStringList().isEmpty());
    verifyDiffInvariant(controller);

    // Selected count starts at the sendable count and tracks load checkbox changes
    QCOMPARE(controller.property("diffSelectedCount").toInt(), 1);
    auto* mutableDiff = controller.diffList()->value<ParameterEditorDiff*>(0);
    mutableDiff->setProperty("load", false);
    QCOMPARE(controller.property("diffSelectedCount").toInt(), 0);
    mutableDiff->setProperty("load", true);
    QCOMPARE(controller.property("diffSelectedCount").toInt(), 1);
}

void ParameterEditorControllerTest::_buildDiffMPFormat()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsWithDiff)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QCOMPARE(diff->name, QStringLiteral("SYS_AUTOSTART"));
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffNoDifferencesQGC()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsNoDiff)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 0);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 1);
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffNoDifferencesMP()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsNoDiff)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 0);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 1);
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffBadFormat()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.txt"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kBadFormatParams)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    expectAppMessage(QRegularExpression("No valid parameters found"));
    const bool result = controller.buildDiffFromFile(tempFile.path());
    verifyExpectedLogMessage();

    QVERIFY(!result);
    QCOMPARE(controller.diffList()->count(), 0);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 0);
}

void ParameterEditorControllerTest::_buildDiffMissingOnVehicle()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsUnknownParam)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QVERIFY(diff->noVehicleValue);
    QCOMPARE(diff->name, QStringLiteral("NO_SUCH_PARAM"));
    // File value must be converted from string to the typed variant (file type 6 = INT32),
    // otherwise the PARAM_VALUE ack type check in _mavlinkParamSet never matches.
    QCOMPARE(diff->fileValueVar.typeId(), QMetaType::Int);
    QCOMPARE(diff->fileValueVar.toInt(), 1);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    QCOMPARE(controller.property("diffNoVehicleCount").toInt(), 1);
    QVERIFY(controller.property("diffMissingParams").toStringList().isEmpty());
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffMPMissingParam()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsMissingParam)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    // Unknown MP param becomes a disabled cannot-send row in the diff list
    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QCOMPARE(diff->name, QStringLiteral("NO_SUCH_PARAM"));
    QVERIFY(diff->cannotSend);
    QVERIFY(!diff->load);
    const QStringList missingParams = controller.property("diffMissingParams").toStringList();
    QCOMPARE(missingParams.count(), 1);
    QCOMPARE(missingParams.at(0), QStringLiteral("NO_SUCH_PARAM"));
    QCOMPARE(controller.property("diffParsedCount").toInt(), 1);
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffMixedMP()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsMixed)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    QVERIFY(result);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 3);
    // Diff list holds the changed row plus the cannot-send row
    QCOMPARE(controller.diffList()->count(), 2);
    const auto* changedDiff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(changedDiff);
    QCOMPARE(changedDiff->name, QStringLiteral("SYS_AUTOSTART"));
    QVERIFY(!changedDiff->cannotSend);
    const auto* missingDiff = controller.diffList()->value<ParameterEditorDiff*>(1);
    QVERIFY(missingDiff);
    QCOMPARE(missingDiff->name, QStringLiteral("NO_SUCH_PARAM"));
    QVERIFY(missingDiff->cannotSend);
    QVERIFY(!missingDiff->load);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 1);
    const QStringList missingParams = controller.property("diffMissingParams").toStringList();
    QCOMPARE(missingParams, QStringList{QStringLiteral("NO_SUCH_PARAM")});
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_clearDiffResetsState()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsMixed)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));
    QCOMPARE(controller.property("diffParsedCount").toInt(), 3);

    QSignalSpy parsedSpy(&controller, &ParameterEditorController::diffParsedCountChanged);
    QSignalSpy unchangedSpy(&controller, &ParameterEditorController::diffUnchangedCountChanged);
    QSignalSpy missingSpy(&controller, &ParameterEditorController::diffMissingParamsChanged);
    QSignalSpy noVehicleSpy(&controller, &ParameterEditorController::diffNoVehicleCountChanged);

    controller.clearDiff();

    QCOMPARE(controller.diffList()->count(), 0);
    QCOMPARE(controller.property("diffParsedCount").toInt(), 0);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 0);
    QCOMPARE(controller.property("diffReadOnlyCount").toInt(), 0);
    QCOMPARE(controller.property("diffNoVehicleCount").toInt(), 0);
    QVERIFY(controller.property("diffMissingParams").toStringList().isEmpty());

    // Signals emit exactly once for properties that changed, never for ones that didn't
    QCOMPARE(parsedSpy.count(), 1);
    QCOMPARE(unchangedSpy.count(), 1);
    QCOMPARE(missingSpy.count(), 1);
    QCOMPARE(noVehicleSpy.count(), 0);  // was already 0

    // Clearing an already-clear diff must not emit anything
    controller.clearDiff();
    QCOMPARE(parsedSpy.count(), 1);
    QCOMPARE(unchangedSpy.count(), 1);
    QCOMPARE(missingSpy.count(), 1);
    QCOMPARE(noVehicleSpy.count(), 0);
}

void ParameterEditorControllerTest::_diffSignalsEmitOnlyOnChange()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsMixed)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;

    QSignalSpy parsedSpy(&controller, &ParameterEditorController::diffParsedCountChanged);
    QSignalSpy unchangedSpy(&controller, &ParameterEditorController::diffUnchangedCountChanged);
    QSignalSpy readOnlySpy(&controller, &ParameterEditorController::diffReadOnlyCountChanged);
    QSignalSpy noVehicleSpy(&controller, &ParameterEditorController::diffNoVehicleCountChanged);
    QSignalSpy sendableSpy(&controller, &ParameterEditorController::diffSendableCountChanged);
    QSignalSpy missingSpy(&controller, &ParameterEditorController::diffMissingParamsChanged);
    QSignalSpy otherVehicleSpy(&controller, &ParameterEditorController::diffOtherVehicleChanged);
    QSignalSpy multipleComponentsSpy(&controller, &ParameterEditorController::diffMultipleComponentsChanged);

    QVERIFY(controller.buildDiffFromFile(tempFile.path()));

    // Mixed MP file: parsed 0→3, unchanged 0→1, sendable 0→1, missing []→[NO_SUCH_PARAM].
    // Each changed property emits exactly once with the final value.
    QCOMPARE(parsedSpy.count(), 1);
    QCOMPARE(parsedSpy.takeFirst().at(0).toInt(), 3);
    QCOMPARE(unchangedSpy.count(), 1);
    QCOMPARE(unchangedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(sendableSpy.count(), 1);
    QCOMPARE(sendableSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(missingSpy.count(), 1);
    QCOMPARE(missingSpy.takeFirst().at(0).toStringList(), QStringList{QStringLiteral("NO_SUCH_PARAM")});

    // Properties whose values did not change must not emit at all
    QCOMPARE(readOnlySpy.count(), 0);
    QCOMPARE(noVehicleSpy.count(), 0);
    QCOMPARE(otherVehicleSpy.count(), 0);
    QCOMPARE(multipleComponentsSpy.count(), 0);
}

void ParameterEditorControllerTest::_sendDiffUnknownParamStopsAfterWriteFailure()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsUnknownParam)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));
    QCOMPARE(controller.diffList()->count(), 1);

    ParameterManager* const paramMgr = parameterManager();
    QVERIFY(paramMgr);
    QSignalSpy writeFailureSpy(paramMgr, &ParameterManager::_paramSetFailure);
    QSignalSpy readSuccessSpy(paramMgr, &ParameterManager::_paramRequestReadSuccess);
    QSignalSpy readFailureSpy(paramMgr, &ParameterManager::_paramRequestReadFailure);
    QVERIFY(writeFailureSpy.isValid());
    QVERIFY(readSuccessSpy.isValid());
    QVERIFY(readFailureSpy.isValid());

    // MockLink rejects the PARAM_SET with PARAM_ERROR DOES_NOT_EXIST: exactly one
    // write-failed notification, and NO follow-up refresh read of the unknown param
    expectAppMessage(QRegularExpression(QStringLiteral("Parameter write failed: param: NO_SUCH_PARAM")));
    controller.sendDiff();

    // PARAM_ERROR is a definitive rejection - no retries, one ack interval suffices
    const int maxWaitTimeMs = ParameterManager::kWaitForParamValueAckMs + TestTimeout::shortMs();
    QVERIFY_SIGNAL_WAIT(writeFailureSpy, maxWaitTimeMs);
    QCOMPARE(writeFailureSpy.count(), 1);
    verifyExpectedLogMessage();

    // Wait long enough that a (buggy) post-failure refresh would have completed,
    // then verify no PARAM_REQUEST_READ was ever issued for the unknown param
    QVERIFY_NO_SIGNAL_WAIT(readFailureSpy, maxWaitTimeMs);
    QCOMPARE(readFailureSpy.count(), 0);
    QCOMPARE(readSuccessSpy.count(), 0);
}
