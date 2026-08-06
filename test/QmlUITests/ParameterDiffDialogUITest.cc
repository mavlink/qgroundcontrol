#include "ParameterDiffDialogUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>
#include <algorithm>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "LogManager.h"
#include "MockLink.h"
#include "ParameterManager.h"
#include "QGCFileDialogController.h"
#include "Vehicle.h"

UT_REGISTER_TEST(ParameterDiffDialogUITest, TestLabel::Integration)

// QGC tab-delimited file with SYS_AUTOSTART changed from 4001 → 4002
static const char* kQGCParamsWithDiff =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4002\t6\n";

// Mission Planner file where the parameter already matches the vehicle default
static const char* kMPParamsNoDiff =
    "# Mission Planner parameter file\n"
    "SYS_AUTOSTART,4001\n";

// Mission Planner file with a parameter not on the vehicle (cannot be sent)
static const char* kMPParamsMissingParam =
    "# Mission Planner parameter file\n"
    "NO_SUCH_PARAM,1\n";

// QGC file with a parameter not on the vehicle (will be sent as new)
static const char* kQGCParamsUnknownParam =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tNO_SUCH_PARAM\t1\t6\n";

// QGC file with two changed parameters, for checkbox select/deselect coverage
static const char* kQGCParamsTwoDiffs =
    "# Onboard parameters for Vehicle 1\n"
    "#\n"
    "# Vehicle-Id Component-Id Name Value Type\n"
    "1\t1\tSYS_AUTOSTART\t4002\t6\n"
    "1\t1\tCOM_DL_LOSS_T\t20\t6\n";

// Arm the file dialog test hook with filePath and drive Tools → "Load from
// file for review..." so ParameterEditor builds the diff and opens the dialog.
void ParameterDiffDialogUITest::_openDiffDialogForFile(const QString& filePath)
{
    QGCFileDialogController::setTestNextFileForAccept(filePath);

    QVERIFY2(clickButton(QStringLiteral("parameterEditor_toolsButton")), "Failed to click Tools button");
    QVERIFY2(clickButton(QStringLiteral("parameterEditor_toolLoadFromFile")),
             "Failed to click Load from file menu item");
    QVERIFY2(waitForDialog(QStringLiteral("Load Parameters")), "Load Parameters dialog never appeared");
}

bool ParameterDiffDialogUITest::_labelTextContains(const QString& objectName, const QString& substring)
{
    QQuickItem* label = findVisibleItem(_rootItem, objectName, 2000);
    if (!label) {
        QTest::qFail(qPrintable(QStringLiteral("Label not found: %1").arg(objectName)), __FILE__, __LINE__);
        return false;
    }
    const QString text = label->property("text").toString();
    if (!text.contains(substring)) {
        QTest::qFail(
            qPrintable(QStringLiteral("Label %1 text '%2' does not contain '%3'").arg(objectName, text, substring)),
            __FILE__, __LINE__);
        return false;
    }
    return true;
}

void ParameterDiffDialogUITest::_testDiffDialogCases()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            navigateToConfigureView();
            if (QTest::currentTestFailed())
                return;

            clickSidebarButton(QStringLiteral("vehicleConfig_parametersButton"));
            if (QTest::currentTestFailed())
                return;

            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("parameterEditor_toolsButton"), 3000),
                     "Parameter editor Tools button never appeared");

            // -------------------------------------------------------------------------
            // Case: all parameters already match the vehicle (MP format)
            // -------------------------------------------------------------------------
            {
                TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
                QVERIFY(tempFile.isValid());
                QVERIFY(tempFile.write(QByteArray(kMPParamsNoDiff)));
                QVERIFY(tempFile.file()->flush());

                _openDiffDialogForFile(tempFile.path());
                if (QTest::currentTestFailed())
                    return;

                if (!_labelTextContains(QStringLiteral("diffSummaryLabel"),
                                        QStringLiteral("1 already matches the Vehicle")))
                    return;
                verifyVisibility(QStringLiteral("diffGrid"), false, QStringLiteral("all-match case"));
                verifyVisibility(QStringLiteral("diffCannotSendHintLabel"), false, QStringLiteral("all-match case"));
                if (QTest::currentTestFailed())
                    return;

                QVERIFY2(rejectDialog(), "Failed to close all-match dialog");
            }

            // -------------------------------------------------------------------------
            // Case: parameter not on vehicle, MP format — cannot be sent
            // -------------------------------------------------------------------------
            {
                TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.param"));
                QVERIFY(tempFile.isValid());
                QVERIFY(tempFile.write(QByteArray(kMPParamsMissingParam)));
                QVERIFY(tempFile.file()->flush());

                _openDiffDialogForFile(tempFile.path());
                if (QTest::currentTestFailed())
                    return;

                if (!_labelTextContains(QStringLiteral("diffSummaryLabel"),
                                        QStringLiteral("1 not found on the Vehicle and cannot be sent")))
                    return;
                // Cannot-send param is shown in the grid as a disabled row
                verifyVisibility(QStringLiteral("diffGrid"), true, QStringLiteral("missing-param case"));
                verifyVisibility(QStringLiteral("diffCannotSendHintLabel"), true, QStringLiteral("missing-param case"));
                verifyEnabled(QStringLiteral("diffRowCheckbox_NO_SUCH_PARAM"), false,
                              QStringLiteral("missing-param case"));
                // No sendable rows means no Ok button
                verifyVisibility(QStringLiteral("diffOkHintLabel"), false, QStringLiteral("missing-param case"));
                if (QTest::currentTestFailed())
                    return;

                QVERIFY2(rejectDialog(), "Failed to close missing-param dialog");
            }

            // -------------------------------------------------------------------------
            // Case: parameter not on vehicle, QGC format — sent as new to vehicle
            // -------------------------------------------------------------------------
            {
                TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
                QVERIFY(tempFile.isValid());
                QVERIFY(tempFile.write(QByteArray(kQGCParamsUnknownParam)));
                QVERIFY(tempFile.file()->flush());

                _openDiffDialogForFile(tempFile.path());
                if (QTest::currentTestFailed())
                    return;

                if (!_labelTextContains(QStringLiteral("diffSummaryLabel"),
                                        QStringLiteral("1 will be changed (including 1 not currently on the Vehicle)")))
                    return;
                verifyVisibility(QStringLiteral("diffGrid"), true, QStringLiteral("new-to-vehicle case"));
                verifyVisibility(QStringLiteral("diffNoVehicleHintLabel"), true, QStringLiteral("new-to-vehicle case"));
                if (QTest::currentTestFailed())
                    return;

                // Accepting sends a PARAM_SET for the unknown param. MockLink (like real
                // firmware) rejects it with PARAM_ERROR DOES_NOT_EXIST. QGC must report
                // exactly one write failure and stop - no crash, and no redundant
                // follow-up "Parameter read failed" popup (any extra app message would
                // fail this test's strict log checking).
                expectAppMessage(QRegularExpression(QStringLiteral("Parameter write failed: param: NO_SUCH_PARAM")));
                QVERIFY2(acceptDialog(), "Failed to accept new-to-vehicle dialog");

                // The write-failed message is the last step of the async failure flow
                auto writeFailedCaptured = []() {
                    const auto msgs = LogManager::capturedMessages();
                    return std::any_of(msgs.cbegin(), msgs.cend(), [](const LogEntry& entry) {
                        return entry.message.contains(QStringLiteral("Parameter write failed: param: NO_SUCH_PARAM"));
                    });
                };
                QTRY_VERIFY_WITH_TIMEOUT(writeFailedCaptured(), 10000);
                verifyExpectedLogMessage();
                if (QTest::currentTestFailed())
                    return;

                // Dismiss the app message dialog(s) so the next case can use the menus.
                // Bounded so a runaway popup loop fails loudly instead of hitting the ctest timeout.
                int dismissAttempts = 0;
                while (findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 1000)) {
                    QVERIFY2(++dismissAttempts <= 5, "App message dialogs keep re-appearing - runaway popup loop");
                    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")),
                             "Failed to dismiss app message dialog");
                }

                // The dismiss loop above idled for at least a second after the last
                // dialog - long enough for a (buggy) post-failure refresh to have
                // produced its "Parameter read failed" popup. Verify it never appeared.
                const auto msgs = LogManager::capturedMessages();
                const bool readFailed = std::any_of(msgs.cbegin(), msgs.cend(), [](const LogEntry& entry) {
                    return entry.message.contains(QStringLiteral("Parameter read failed: param: NO_SUCH_PARAM"));
                });
                QVERIFY2(!readFailed,
                         "Unexpected redundant 'Parameter read failed' after DOES_NOT_EXIST write rejection");
            }

            // -------------------------------------------------------------------------
            // Case: checkbox behavior — select all / deselect all, and only the
            // checked subset of params is sent
            // -------------------------------------------------------------------------
            {
                TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
                QVERIFY(tempFile.isValid());
                QVERIFY(tempFile.write(QByteArray(kQGCParamsTwoDiffs)));
                QVERIFY(tempFile.file()->flush());

                _openDiffDialogForFile(tempFile.path());
                if (QTest::currentTestFailed())
                    return;

                // Both rows start checked
                verifyChecked(QStringLiteral("diffRowCheckbox_SYS_AUTOSTART"), true,
                              QStringLiteral("checkbox case: initial"));
                verifyChecked(QStringLiteral("diffRowCheckbox_COM_DL_LOSS_T"), true,
                              QStringLiteral("checkbox case: initial"));
                if (QTest::currentTestFailed())
                    return;

                // Header checkbox deselects all rows...
                QVERIFY2(clickButton(QStringLiteral("diffCheckAllCheckbox")), "Failed to click check-all checkbox");
                verifyChecked(QStringLiteral("diffRowCheckbox_SYS_AUTOSTART"), false,
                              QStringLiteral("checkbox case: deselect all"));
                verifyChecked(QStringLiteral("diffRowCheckbox_COM_DL_LOSS_T"), false,
                              QStringLiteral("checkbox case: deselect all"));
                // ...which disables Ok since there is nothing to send
                verifyEnabled(QStringLiteral("popupDialog_acceptButton"), false,
                              QStringLiteral("checkbox case: deselect all"));
                if (QTest::currentTestFailed())
                    return;

                // ...and selects them all again
                QVERIFY2(clickButton(QStringLiteral("diffCheckAllCheckbox")), "Failed to click check-all checkbox");
                verifyChecked(QStringLiteral("diffRowCheckbox_SYS_AUTOSTART"), true,
                              QStringLiteral("checkbox case: reselect all"));
                verifyChecked(QStringLiteral("diffRowCheckbox_COM_DL_LOSS_T"), true,
                              QStringLiteral("checkbox case: reselect all"));
                verifyEnabled(QStringLiteral("popupDialog_acceptButton"), true,
                              QStringLiteral("checkbox case: reselect all"));
                if (QTest::currentTestFailed())
                    return;

                // Uncheck just SYS_AUTOSTART, then accept: only COM_DL_LOSS_T may be sent
                QVERIFY2(clickButton(QStringLiteral("diffRowCheckbox_SYS_AUTOSTART")),
                         "Failed to uncheck SYS_AUTOSTART row");
                verifyChecked(QStringLiteral("diffRowCheckbox_SYS_AUTOSTART"), false,
                              QStringLiteral("checkbox case: single deselect"));
                verifyChecked(QStringLiteral("diffRowCheckbox_COM_DL_LOSS_T"), true,
                              QStringLiteral("checkbox case: single deselect"));
                if (QTest::currentTestFailed())
                    return;

                QVERIFY2(acceptDialog(), "Failed to accept checkbox-case dialog");

                // COM_DL_LOSS_T (checked) reaches the vehicle...
                QTRY_COMPARE_WITH_TIMEOUT(mockLink->paramValue(1, QStringLiteral("COM_DL_LOSS_T")).toInt(), 20, 5000);
                // ...while SYS_AUTOSTART (unchecked) must remain at the vehicle default
                QCOMPARE(mockLink->paramValue(1, QStringLiteral("SYS_AUTOSTART")).toInt(), 4001);

                // Let in-flight param traffic settle before the next case
                waitForParamRefreshQuiet(vehicle);
                if (QTest::currentTestFailed())
                    return;
            }

            // -------------------------------------------------------------------------
            // Case: real difference (QGC format) — accept sends the change to the vehicle
            // -------------------------------------------------------------------------
            {
                TestFixtures::TempFileFixture tempFile(QStringLiteral("test_XXXXXX.params"));
                QVERIFY(tempFile.isValid());
                QVERIFY(tempFile.write(QByteArray(kQGCParamsWithDiff)));
                QVERIFY(tempFile.file()->flush());

                _openDiffDialogForFile(tempFile.path());
                if (QTest::currentTestFailed())
                    return;

                if (!_labelTextContains(QStringLiteral("diffSummaryLabel"), QStringLiteral("1 will be changed")))
                    return;
                verifyVisibility(QStringLiteral("diffGrid"), true, QStringLiteral("changed case"));
                verifyVisibility(QStringLiteral("diffCannotSendHintLabel"), false, QStringLiteral("changed case"));
                if (QTest::currentTestFailed())
                    return;

                // SYS_AUTOSTART is reboot-required, so accepting pops the reboot advisory
                expectAppMessage(QRegularExpression(QStringLiteral("Reboot vehicle for changes to take effect")));
                QVERIFY2(acceptDialog(), "Failed to accept diff dialog");

                // Verify the new value actually reached the (mock) vehicle
                QTRY_COMPARE_WITH_TIMEOUT(mockLink->paramValue(1, QStringLiteral("SYS_AUTOSTART")).toInt(), 4002, 5000);
                verifyExpectedLogMessage();

                Fact* fact = vehicle->parameterManager()->getParameter(1, QStringLiteral("SYS_AUTOSTART"));
                QVERIFY2(fact, "SYS_AUTOSTART fact not found");
                QTRY_COMPARE_WITH_TIMEOUT(fact->rawValue().toInt(), 4002, 5000);

                // Let in-flight PARAM_SET/PARAM_REQUEST_READ traffic settle before link teardown
                waitForParamRefreshQuiet(vehicle);
            }
        });
}
