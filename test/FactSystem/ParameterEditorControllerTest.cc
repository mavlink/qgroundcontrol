#include "ParameterEditorControllerTest.h"

#include <limits>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "ParameterEditorController.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
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

// QGC file saved when these int32 parameters were floats (type 9): a positive and a negative change (like RC1_REV
// in issue 15357), and a negative value matching the vehicle
static const char* kQGCParamsWholeNumberFloats =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4002.000000000000000000\t9\n"
    "1\t1\tCOM_DL_LOSS_T\t-1.000000000000000000\t9\n"
    "1\t1\tCOM_FLTMODE1\t-1.000000000000000000\t9\n";

// QGC file with values that cannot be converted to the int32 vehicle parameters: a fraction, a fraction too small
// for a double to keep, an exponent, and a whole number out of range. Plus a non-number for a parameter not on the
// vehicle.
static const char* kQGCParamsInvalidValues =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4002.5\t9\n"
    "1\t1\tCOM_FLTMODE1\t1.0000000000000000001\t9\n"
    "1\t1\tCOM_FLTMODE2\t3e1\t9\n"
    "1\t1\tCOM_DL_LOSS_T\t5000000000.000000\t9\n"
    "1\t1\tNO_SUCH_PARAM\tabc\t6\n";

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

    ParameterEditorController controller;
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));
    QCOMPARE(controller.property("diffParsedCount").toInt(), 3);

    QSignalSpy parsedSpy(&controller, &ParameterEditorController::diffParsedCountChanged);
    QSignalSpy unchangedSpy(&controller, &ParameterEditorController::diffUnchangedCountChanged);
    QSignalSpy missingSpy(&controller, &ParameterEditorController::diffMissingParamsChanged);
    QSignalSpy noVehicleSpy(&controller, &ParameterEditorController::diffNoVehicleCountChanged);
    QSignalSpy invalidSpy(&controller, &ParameterEditorController::diffInvalidParamsChanged);

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
    QCOMPARE(invalidSpy.count(), 0);    // was already empty

    // Clearing an already-clear diff must not emit anything
    controller.clearDiff();
    QCOMPARE(parsedSpy.count(), 1);
    QCOMPARE(unchangedSpy.count(), 1);
    QCOMPARE(missingSpy.count(), 1);
    QCOMPARE(noVehicleSpy.count(), 0);
    QCOMPARE(invalidSpy.count(), 0);
}

void ParameterEditorControllerTest::_diffSignalsEmitOnlyOnChange()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kMPParamsMixed)));

    ParameterEditorController controller;

    QSignalSpy parsedSpy(&controller, &ParameterEditorController::diffParsedCountChanged);
    QSignalSpy unchangedSpy(&controller, &ParameterEditorController::diffUnchangedCountChanged);
    QSignalSpy readOnlySpy(&controller, &ParameterEditorController::diffReadOnlyCountChanged);
    QSignalSpy noVehicleSpy(&controller, &ParameterEditorController::diffNoVehicleCountChanged);
    QSignalSpy sendableSpy(&controller, &ParameterEditorController::diffSendableCountChanged);
    QSignalSpy missingSpy(&controller, &ParameterEditorController::diffMissingParamsChanged);
    QSignalSpy invalidSpy(&controller, &ParameterEditorController::diffInvalidParamsChanged);
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
    QCOMPARE(invalidSpy.count(), 0);
    QCOMPARE(otherVehicleSpy.count(), 0);
    QCOMPARE(multipleComponentsSpy.count(), 0);
}

void ParameterEditorControllerTest::_sendDiffUnknownParamStopsAfterWriteFailure()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsUnknownParam)));

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

void ParameterEditorControllerTest::_buildDiffWholeNumberFloatForIntParam()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsWholeNumberFloats)));

    ParameterEditorController controller;
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));

    QCOMPARE(controller.property("diffParsedCount").toInt(), 3);
    QCOMPARE(controller.property("diffUnchangedCount").toInt(), 1);
    QCOMPARE(controller.property("diffSendableCount").toInt(), 2);
    QVERIFY(controller.property("diffInvalidParams").toStringList().isEmpty());
    QCOMPARE(controller.diffList()->count(), 2);

    const QStringList changedNames = {QStringLiteral("SYS_AUTOSTART"), QStringLiteral("COM_DL_LOSS_T")};
    const QList<int> changedValues = {4002, -1};
    for (int i = 0; i < controller.diffList()->count(); i++) {
        const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(i);
        QVERIFY(diff);
        QCOMPARE(diff->name, changedNames.at(i));
        QVERIFY(!diff->invalidFileValue);
        QVERIFY(!diff->cannotSend);
        QVERIFY(diff->load);
        QCOMPARE(diff->fileValueVar.typeId(), QMetaType::Int);
        QCOMPARE(diff->fileValueVar.toInt(), changedValues.at(i));
    }
    verifyDiffInvariant(controller);
}

void ParameterEditorControllerTest::_buildDiffInvalidFileValues()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsInvalidValues)));

    const QStringList invalidNames = {
        QStringLiteral("SYS_AUTOSTART"), QStringLiteral("COM_FLTMODE1"),  QStringLiteral("COM_FLTMODE2"),
        QStringLiteral("COM_DL_LOSS_T"), QStringLiteral("NO_SUCH_PARAM"),
    };
    const QStringList rawFileValues = {
        QStringLiteral("4002.5"), QStringLiteral("1.0000000000000000001"),
        QStringLiteral("3e1"),    QStringLiteral("5000000000.000000"),
        QStringLiteral("abc"),
    };
    const qsizetype noVehicleIndex = invalidNames.count() - 1;

    ParameterManager* const paramMgr = parameterManager();
    QVERIFY(paramMgr);
    QList<Fact*> vehicleFacts;
    QVariantList valuesBefore;
    for (qsizetype i = 0; i < noVehicleIndex; i++) {
        Fact* const fact = paramMgr->getParameter(ParameterManager::defaultComponentId, invalidNames.at(i));
        vehicleFacts.append(fact);
        valuesBefore.append(fact->rawValue());
    }

    ParameterEditorController controller;
    QSignalSpy invalidSpy(&controller, &ParameterEditorController::diffInvalidParamsChanged);
    for (const QString& name : invalidNames) {
        expectLogMessage("QMLControls.ParameterEditorController", QtWarningMsg,
                         QRegularExpression(QStringLiteral("invalid file value.*%1").arg(name)));
    }
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));
    for (qsizetype i = 0; i < invalidNames.count(); i++) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(invalidSpy.count(), 1);

    QCOMPARE(controller.property("diffParsedCount").toInt(), invalidNames.count());
    QCOMPARE(controller.property("diffSendableCount").toInt(), 0);
    QCOMPARE(controller.property("diffSelectedCount").toInt(), 0);
    QCOMPARE(controller.property("diffInvalidParams").toStringList(), invalidNames);
    QVERIFY(controller.property("diffMissingParams").toStringList().isEmpty());
    QCOMPARE(controller.diffList()->count(), invalidNames.count());
    verifyDiffInvariant(controller);

    for (int i = 0; i < controller.diffList()->count(); i++) {
        const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(i);
        QVERIFY(diff);
        QCOMPARE(diff->name, invalidNames.at(i));
        QCOMPARE(diff->fileValue, rawFileValues.at(i));
        QVERIFY(diff->invalidFileValue);
        QVERIFY(diff->cannotSend);
        QVERIFY(!diff->load);
        // Rows on the vehicle show the vehicle's value; the row new to the vehicle has none
        if (i == noVehicleIndex) {
            QVERIFY(diff->noVehicleValue);
        } else {
            QVERIFY(!diff->noVehicleValue);
            QCOMPARE(diff->vehicleValue, vehicleFacts.at(i)->enumOrValueString());
            QCOMPARE(diff->units, vehicleFacts.at(i)->cookedUnits());
        }
    }

    controller.sendDiff();
    for (qsizetype i = 0; i < vehicleFacts.count(); i++) {
        QCOMPARE(vehicleFacts.at(i)->rawValue(), valuesBefore.at(i));
    }

    controller.clearDiff();
    QVERIFY(controller.property("diffInvalidParams").toStringList().isEmpty());
    QCOMPARE(invalidSpy.count(), 2);
}

// Drives _convertFileValue for every MAVLink integer type through a QGC-format parameter that is not on the vehicle,
// since those are converted using the file's type column. Rows without an expected value must be rejected.
void ParameterEditorControllerTest::_buildDiffFileValueConversion_data()
{
    QTest::addColumn<int>("mavParamType");
    QTest::addColumn<QString>("fileValue");
    QTest::addColumn<QVariant>("expected");

    constexpr int kUint8 = 1;
    constexpr int kInt8 = 2;
    constexpr int kUint16 = 3;
    constexpr int kInt16 = 4;
    constexpr int kUint32 = 5;
    constexpr int kInt32 = 6;
    constexpr int kUint64 = 7;
    constexpr int kInt64 = 8;
    constexpr int kReal32 = 9;
    const QVariant invalid;

    QTest::newRow("int32 negative whole float") << kInt32 << "-1.000000000000000000" << QVariant(-1);
    QTest::newRow("int32 zero float") << kInt32 << "0.0" << QVariant(0);
    QTest::newRow("int32 negative zero float") << kInt32 << "-0.0" << QVariant(0);
    QTest::newRow("int32 leading plus") << kInt32 << "+5" << QVariant(5);
    QTest::newRow("int32 trailing dot") << kInt32 << "7." << QVariant(7);
    QTest::newRow("int32 max float") << kInt32 << "2147483647.0" << QVariant(2147483647);
    QTest::newRow("int32 min float") << kInt32 << "-2147483648.0" << QVariant(static_cast<int>(-2147483648LL));
    QTest::newRow("int32 above max") << kInt32 << "2147483648.0" << invalid;
    QTest::newRow("int32 below min") << kInt32 << "-2147483649" << invalid;
    QTest::newRow("int32 fraction") << kInt32 << "1.5" << invalid;
    QTest::newRow("int32 exponent") << kInt32 << "1e3" << invalid;
    QTest::newRow("int32 empty fraction digits") << kInt32 << ".0" << invalid;

    QTest::newRow("int8 max float") << kInt8 << "127.0" << QVariant(127);
    QTest::newRow("int8 min") << kInt8 << "-128" << QVariant(-128);
    QTest::newRow("int8 above max float") << kInt8 << "128.0" << invalid;
    QTest::newRow("int8 below min") << kInt8 << "-129" << invalid;
    QTest::newRow("int8 wraps in QVariant") << kInt8 << "300" << invalid;

    QTest::newRow("int16 max float") << kInt16 << "32767.0" << QVariant(32767);
    QTest::newRow("int16 above max") << kInt16 << "32768" << invalid;

    QTest::newRow("uint8 max float") << kUint8 << "255.0" << QVariant(255u);
    QTest::newRow("uint8 above max") << kUint8 << "256" << invalid;
    QTest::newRow("uint8 negative float") << kUint8 << "-1.0" << invalid;

    QTest::newRow("uint16 max float") << kUint16 << "65535.0" << QVariant(65535u);
    QTest::newRow("uint16 above max") << kUint16 << "65536" << invalid;

    QTest::newRow("uint32 max float") << kUint32 << "4294967295.0" << QVariant(4294967295u);
    QTest::newRow("uint32 above max") << kUint32 << "4294967296" << invalid;
    QTest::newRow("uint32 negative") << kUint32 << "-1" << invalid;

    QTest::newRow("int64 max float") << kInt64 << "9223372036854775807.0"
                                     << QVariant(std::numeric_limits<qlonglong>::max());
    QTest::newRow("int64 min float") << kInt64 << "-9223372036854775808.0"
                                     << QVariant(std::numeric_limits<qlonglong>::min());
    QTest::newRow("int64 above max float") << kInt64 << "9223372036854775808.0" << invalid;
    // 2^53 + 1 is not representable as a double, so this would round if parsed as one
    QTest::newRow("int64 above 2^53 float") << kInt64 << "9007199254740993.0" << QVariant(9007199254740993LL);

    QTest::newRow("uint64 max float") << kUint64 << "18446744073709551615.0"
                                      << QVariant(std::numeric_limits<qulonglong>::max());
    QTest::newRow("uint64 above max") << kUint64 << "18446744073709551616" << invalid;
    QTest::newRow("uint64 negative float") << kUint64 << "-1.0" << invalid;

    QTest::newRow("float fraction unaffected") << kReal32 << "1.5" << QVariant(1.5f);
}

void ParameterEditorControllerTest::_buildDiffFileValueConversion()
{
    QFETCH(int, mavParamType);
    QFETCH(QString, fileValue);
    QFETCH(QVariant, expected);

    const QString fileContents =
        QStringLiteral("# Vehicle-Id Component-Id Name Value Type\n1\t1\tNO_SUCH_PARAM\t%1\t%2\n")
            .arg(fileValue)
            .arg(mavParamType);
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(fileContents.toUtf8()));

    ParameterEditorController controller;
    const bool expectInvalid = !expected.isValid();
    if (expectInvalid) {
        expectLogMessage("QMLControls.ParameterEditorController", QtWarningMsg,
                         QRegularExpression(QStringLiteral("invalid file value.*NO_SUCH_PARAM")));
    }
    QVERIFY(controller.buildDiffFromFile(tempFile.path()));
    if (expectInvalid) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QCOMPARE(diff->invalidFileValue, expectInvalid);
    QCOMPARE(diff->cannotSend, expectInvalid);
    if (!expectInvalid) {
        QCOMPARE(diff->fileValueVar.typeId(), expected.typeId());
        QCOMPARE(diff->fileValueVar, expected);
    }
}
