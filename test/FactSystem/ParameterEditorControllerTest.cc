#include "ParameterEditorControllerTest.h"

#include <QtCore/QString>
#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

#include "ParameterEditorController.h"
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

// Mission Planner space-delimited file with same diff
static const char* kMPParamsWithDiff =
    "# Mission Planner parameter file\n"
    "1 1 SYS_AUTOSTART 4002 6\n";

// Mission Planner space-delimited file matching vehicle default
static const char* kMPParamsNoDiff =
    "# Mission Planner parameter file\n"
    "1 1 SYS_AUTOSTART 4001 6\n";

// Garbage — no parseable lines
static const char* kBadFormatParams =
    "This is not a parameter file\n"
    "random text here\n";

// QGC file with a parameter that does not exist on the (PX4 mock) vehicle
static const char* kQGCParamsUnknownParam =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tNONEXISTENT_PARAM_XYZ\t1\t6\n";

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
}

void ParameterEditorControllerTest::_buildDiffNoDifferencesQGC()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsNoDiff)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    // parsedLineCount > 0 so returns true, but no diffs found
    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 0);
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

    // parsedLineCount == 0 → returns false
    QVERIFY(!result);
    QCOMPARE(controller.diffList()->count(), 0);
}

void ParameterEditorControllerTest::_buildDiffMissingOnVehicle()
{
    TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
    QVERIFY(tempFile.isValid());
    QVERIFY(tempFile.write(QByteArray(kQGCParamsUnknownParam)));
    QVERIFY(tempFile.file()->flush());

    ParameterEditorController controller;
    const bool result = controller.buildDiffFromFile(tempFile.path());

    // Line was parsed (parsedLineCount > 0) → returns true
    QVERIFY(result);
    QCOMPARE(controller.diffList()->count(), 1);
    const auto* diff = controller.diffList()->value<ParameterEditorDiff*>(0);
    QVERIFY(diff);
    QVERIFY(diff->noVehicleValue);
    QCOMPARE(diff->name, QStringLiteral("NONEXISTENT_PARAM_XYZ"));
}
