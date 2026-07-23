#include "OnboardLogUITest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTemporaryDir>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include <algorithm>

#include "MockLink.h"
#include "MockLinkFTP.h"
#include "QGCFileDialogController.h"

UT_REGISTER_TEST(OnboardLogUITest, TestLabel::Integration)

void OnboardLogUITest::_navigateToOnboardLogsPage()
{
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewAnalyze")));
    QVERIFY2(clickButton(QStringLiteral("analyzeButton_Onboard Logs")), "Onboard Logs analyze page button not found");
}

void OnboardLogUITest::_downloadFirstLogAndVerify(const QString& downloadDir)
{
    // The page refreshes automatically when loaded; wait for the first log entry row
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_0"), 15000), "log entry row never appeared");

    // Select the first log
    QVERIFY(clickButton(QStringLiteral("onboardLogCheckbox_0")));

    // Download button opens a save-directory dialog; arm the test hook to accept with our dir
    QGCFileDialogController::setTestNextFileForAccept(downloadDir);
    QVERIFY(clickButton(QStringLiteral("onboardLog_downloadButton")));

    // Wait for the download to complete
    QQuickItem* const statusLabel = findVisibleItem(_rootItem, QStringLiteral("onboardLogStatus_0"), 2000);
    QVERIFY2(statusLabel, "status label not found");
    QTRY_COMPARE_WITH_TIMEOUT(statusLabel->property("text").toString(), QStringLiteral("Downloaded"), 30000);
}

void OnboardLogUITest::_messagesDownloadUITest()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> mockLink, Vehicle* /*vehicle*/) {
                        _navigateToOnboardLogsPage();
                        if (QTest::currentTestFailed())
                            return;

                        QTemporaryDir tempDir;
                        QVERIFY(tempDir.isValid());

                        _downloadFirstLogAndVerify(tempDir.path());
                        if (QTest::currentTestFailed())
                            return;

                        // Message-based transport names the file log_<id>_<date>
                        const QString downloadFile =
                            QDir(tempDir.path()).filePath(QStringLiteral("log_0_UnknownDate.ulg"));
                        QVERIFY(QFile::exists(downloadFile));
                        QVERIFY(UnitTest::fileCompare(downloadFile, mockLink->logDownloadFile()));
                    });
}

void OnboardLogUITest::_verifyButtonEnabled(const QString& objectName, bool expected)
{
    QQuickItem* const item = findVisibleItem(_rootItem, objectName, 2000);
    QVERIFY2(item, qPrintable(objectName));
    QTRY_COMPARE_WITH_TIMEOUT(item->property("enabled").toBool(), expected, 5000);
}

// Verifies the enabled state of the selection-gated buttons: Download always,
// Erase Selected on the FTP transport (it must not be visible on the message transport).
void OnboardLogUITest::_verifySelectionButtonStates(bool hasSelection)
{
    _verifyButtonEnabled(QStringLiteral("onboardLog_downloadButton"), hasSelection);
    if (_ftpTransport) {
        _verifyButtonEnabled(QStringLiteral("onboardLog_eraseSelectedButton"), hasSelection);
    } else {
        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLog_eraseSelectedButton"), 500) == nullptr,
                 "Erase Selected button must not be visible on the message transport");
    }
}

// Verifies the enabled state of every action button while the page is idle
// with logCount logs listed and nothing selected.
void OnboardLogUITest::_verifyIdleButtonStates(int logCount)
{
    _verifyButtonEnabled(QStringLiteral("onboardLog_refreshButton"), true);
    _verifyButtonEnabled(QStringLiteral("onboardLog_selectAllButton"), logCount > 0);
    _verifyButtonEnabled(QStringLiteral("onboardLog_sortButton"), logCount > 1);
    _verifyButtonEnabled(QStringLiteral("onboardLog_eraseAllButton"), logCount > 0);
    _verifyButtonEnabled(QStringLiteral("onboardLog_cancelButton"), false);
    _verifySelectionButtonStates(false);
}

// Exercises the transport-independent page behavior: idle button states,
// select-all toggling with selection-gated Download, and sort order flipping
// (when more than one log).
void OnboardLogUITest::_fullPageStateChecks(int logCount)
{
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_0"), 15000), "log entry rows never appeared");

    _verifyIdleButtonStates(logCount);
    if (QTest::currentTestFailed())
        return;

    // Select All selects every row and toggles to Deselect All
    QQuickItem* const selectAllButton = findVisibleItem(_rootItem, QStringLiteral("onboardLog_selectAllButton"));
    QVERIFY(selectAllButton);
    QCOMPARE(selectAllButton->property("text").toString(), QStringLiteral("Select All"));

    QVERIFY(clickButton(QStringLiteral("onboardLog_selectAllButton")));
    for (int i = 0; i < logCount; i++) {
        QQuickItem* const checkbox = findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_%1").arg(i));
        QVERIFY(checkbox);
        QTRY_VERIFY_WITH_TIMEOUT(checkbox->property("checked").toBool(), 5000);
    }
    QTRY_COMPARE_WITH_TIMEOUT(selectAllButton->property("text").toString(), QStringLiteral("Deselect All"), 5000);

    // Download / Erase Selected become available once something is selected
    _verifySelectionButtonStates(true);
    if (QTest::currentTestFailed())
        return;

    QVERIFY(clickButton(QStringLiteral("onboardLog_selectAllButton")));
    for (int i = 0; i < logCount; i++) {
        QQuickItem* const checkbox = findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_%1").arg(i));
        QVERIFY(checkbox);
        QTRY_VERIFY_WITH_TIMEOUT(!checkbox->property("checked").toBool(), 5000);
    }
    QTRY_COMPARE_WITH_TIMEOUT(selectAllButton->property("text").toString(), QStringLiteral("Select All"), 5000);

    // ... and unavailable again with nothing selected
    _verifySelectionButtonStates(false);
    if (QTest::currentTestFailed())
        return;

    // Sort button flips the row order (dates swap between rows)
    if (logCount > 1) {
        QQuickItem* const sortButton = findVisibleItem(_rootItem, QStringLiteral("onboardLog_sortButton"));
        QVERIFY(sortButton);
        QCOMPARE(sortButton->property("text").toString(), QStringLiteral("Sort Ascending"));

        QQuickItem* const dateLabel0 = findVisibleItem(_rootItem, QStringLiteral("onboardLogDate_0"));
        QQuickItem* const dateLabel1 = findVisibleItem(_rootItem, QStringLiteral("onboardLogDate_1"));
        QVERIFY(dateLabel0);
        QVERIFY(dateLabel1);
        const QString date0 = dateLabel0->property("text").toString();
        const QString date1 = dateLabel1->property("text").toString();
        QVERIFY(date0 != date1);

        QVERIFY(clickButton(QStringLiteral("onboardLog_sortButton")));
        QTRY_COMPARE_WITH_TIMEOUT(sortButton->property("text").toString(), QStringLiteral("Sort Descending"), 5000);

        // The Repeater re-instantiates delegates after the sort, re-find the labels
        QQuickItem* const dateLabel0Sorted = findVisibleItem(_rootItem, QStringLiteral("onboardLogDate_0"), 5000);
        QQuickItem* const dateLabel1Sorted = findVisibleItem(_rootItem, QStringLiteral("onboardLogDate_1"), 5000);
        QVERIFY(dateLabel0Sorted);
        QVERIFY(dateLabel1Sorted);
        QTRY_COMPARE_WITH_TIMEOUT(dateLabel0Sorted->property("text").toString(), date1, 5000);
        QCOMPARE(dateLabel1Sorted->property("text").toString(), date0);

        // Toggle back to the default descending order
        QVERIFY(clickButton(QStringLiteral("onboardLog_sortButton")));
        QTRY_COMPARE_WITH_TIMEOUT(sortButton->property("text").toString(), QStringLiteral("Sort Ascending"), 5000);
    }
}

// Clicks Erase All, rejects the confirmation once (logs must remain), then
// accepts it and verifies the list refreshes to empty with buttons disabled.
void OnboardLogUITest::_eraseAllAndVerifyEmpty()
{
    QVERIFY(clickButton(QStringLiteral("onboardLog_eraseAllButton")));
    QVERIFY(waitForDialog(QStringLiteral("Delete All Onboard Log Files")));
    QVERIFY(rejectDialog());
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_0"), 2000), "logs must remain after rejecting the erase confirmation");

    QVERIFY(clickButton(QStringLiteral("onboardLog_eraseAllButton")));
    QVERIFY(waitForDialog(QStringLiteral("Delete All Onboard Log Files")));
    QVERIFY(acceptDialog());

    // eraseAll auto-refreshes: the rows must disappear
    QTRY_VERIFY_WITH_TIMEOUT(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_0"), 50) == nullptr, 15000);

    _verifyIdleButtonStates(0);
}

void OnboardLogUITest::_messagesFullPageUITest()
{
    _ftpTransport = false;
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
                        _navigateToOnboardLogsPage();
                        if (QTest::currentTestFailed())
                            return;

                        // MockLink lists exactly one log via LOG_REQUEST_LIST
                        _fullPageStateChecks(1);
                        if (QTest::currentTestFailed())
                            return;

                        _eraseAllAndVerifyEmpty();
                    });
}

void OnboardLogUITest::_ftpFullPageUITest()
{
    _ftpTransport = true;
    runWithMockLink([] { return MockLink::startPX4MockLink(MockConfiguration::OptionFtpCapability); },
                    [&](QPointer<MockLink> mockLink, Vehicle* /*vehicle*/) {
                        // log_new is newest so it sorts to row 0. The burst read delay paces
                        // the download so it can be canceled mid-transfer and runs long enough
                        // to outlast the 500ms progress-status throttle.
                        const QList<MockLinkFTP::LogFile> logFiles = {
                            {QStringLiteral("log_old.ulg"), 4000, 1700000000},
                            {QStringLiteral("log_new.ulg"), 150000, 1700086400},
                        };
                        mockLink->mockLinkFTP()->setLogFiles(logFiles);
                        mockLink->mockLinkFTP()->setBurstReadDelayMs(20);

                        _navigateToOnboardLogsPage();
                        if (QTest::currentTestFailed())
                            return;

                        _fullPageStateChecks(2);
                        if (QTest::currentTestFailed())
                            return;

                        // Start a slow download and verify the button states while it runs
                        QVERIFY(clickButton(QStringLiteral("onboardLogCheckbox_0")));

                        QTemporaryDir tempDir;
                        QVERIFY(tempDir.isValid());
                        QGCFileDialogController::setTestNextFileForAccept(tempDir.path());
                        QVERIFY(clickButton(QStringLiteral("onboardLog_downloadButton")));

                        _verifyButtonEnabled(QStringLiteral("onboardLog_cancelButton"), true);
                        if (QTest::currentTestFailed())
                            return;
                        _verifyButtonEnabled(QStringLiteral("onboardLog_refreshButton"), false);
                        _verifyButtonEnabled(QStringLiteral("onboardLog_selectAllButton"), false);
                        _verifyButtonEnabled(QStringLiteral("onboardLog_downloadButton"), false);
                        _verifyButtonEnabled(QStringLiteral("onboardLog_sortButton"), false);
                        _verifyButtonEnabled(QStringLiteral("onboardLog_eraseAllButton"), false);
                        _verifyButtonEnabled(QStringLiteral("onboardLog_eraseSelectedButton"), false);
                        if (QTest::currentTestFailed())
                            return;

                        // Cancel the download mid-transfer
                        QVERIFY(clickButton(QStringLiteral("onboardLog_cancelButton")));
                        QQuickItem* const statusLabel = findVisibleItem(_rootItem, QStringLiteral("onboardLogStatus_0"), 2000);
                        QVERIFY(statusLabel);
                        QTRY_COMPARE_WITH_TIMEOUT(statusLabel->property("text").toString(), QStringLiteral("Canceled"), 10000);
                        _verifyIdleButtonStates(2);
                        if (QTest::currentTestFailed())
                            return;

                        // Multi-log download via Select All: the second log shows Waiting
                        // while the first downloads and progress text replaces its status
                        QVERIFY(clickButton(QStringLiteral("onboardLog_selectAllButton")));
                        _verifyButtonEnabled(QStringLiteral("onboardLog_downloadButton"), true);
                        if (QTest::currentTestFailed())
                            return;

                        QTemporaryDir multiDir;
                        QVERIFY(multiDir.isValid());
                        QGCFileDialogController::setTestNextFileForAccept(multiDir.path());
                        QVERIFY(clickButton(QStringLiteral("onboardLog_downloadButton")));

                        // Row 0 (log_new, 150KB) downloads first while row 1 queues as Waiting
                        QQuickItem* const statusLabel1 = findVisibleItem(_rootItem, QStringLiteral("onboardLogStatus_1"), 2000);
                        QVERIFY(statusLabel1);
                        QTRY_COMPARE_WITH_TIMEOUT(statusLabel1->property("text").toString(), QStringLiteral("Waiting"), 10000);

                        // In-progress status shows "<size> (<rate>/s)" while downloading.
                        // Poll every event-loop pass since the progress text can be brief.
                        QStringList observedStatuses;
                        QElapsedTimer downloadTimer;
                        downloadTimer.start();
                        while ((statusLabel->property("text").toString() != QStringLiteral("Downloaded")) && (downloadTimer.elapsed() < 60000)) {
                            const QString currentStatus = statusLabel->property("text").toString();
                            if (observedStatuses.isEmpty() || (observedStatuses.constLast() != currentStatus)) {
                                observedStatuses.append(currentStatus);
                            }
                            QCoreApplication::processEvents();
                        }
                        QCOMPARE(statusLabel->property("text").toString(), QStringLiteral("Downloaded"));
                        const bool sawProgressStatus = std::any_of(observedStatuses.cbegin(), observedStatuses.cend(),
                                                                   [](const QString& s) { return s.contains(QStringLiteral("/s)")); });
                        QVERIFY2(sawProgressStatus, qPrintable(QStringLiteral("statuses seen: ") + observedStatuses.join(QStringLiteral(", "))));

                        QTRY_COMPARE_WITH_TIMEOUT(statusLabel1->property("text").toString(), QStringLiteral("Downloaded"), 30000);
                        _verifyIdleButtonStates(2);
                        if (QTest::currentTestFailed())
                            return;

                        // Both files saved under their remote names with correct contents
                        for (const auto& logFile : logFiles) {
                            const QString downloadFile = QDir(multiDir.path()).filePath(logFile.name);
                            QVERIFY2(QFile::exists(downloadFile), qPrintable(logFile.name));
                            QFile file(downloadFile);
                            QVERIFY(file.open(QIODevice::ReadOnly));
                            QCOMPARE(file.readAll(), mockLink->mockLinkFTP()->logFileContents(logFile.name));
                        }

                        // Refresh re-lists from the vehicle: a newly added log appears
                        QList<MockLinkFTP::LogFile> updatedLogFiles = logFiles;
                        updatedLogFiles.append({QStringLiteral("log_extra.ulg"), 3000, 1700172800});
                        mockLink->mockLinkFTP()->setLogFiles(updatedLogFiles);
                        QVERIFY(clickButton(QStringLiteral("onboardLog_refreshButton")));
                        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_2"), 15000), "third log never appeared after refresh");
                        _verifyIdleButtonStates(3);
                        if (QTest::currentTestFailed())
                            return;

                        // Selective erase with a single row selected
                        QVERIFY(clickButton(QStringLiteral("onboardLogCheckbox_0")));
                        _verifySelectionButtonStates(true);
                        if (QTest::currentTestFailed())
                            return;

                        // Rejecting the confirmation leaves all logs in place
                        QVERIFY(clickButton(QStringLiteral("onboardLog_eraseSelectedButton")));
                        QVERIFY(waitForDialog(QStringLiteral("Delete Selected Onboard Log Files")));
                        QVERIFY(rejectDialog());
                        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_2"), 2000),
                                 "logs must remain after rejecting the selective erase confirmation");

                        // Accepting erases only the selected log (row 0 = log_extra) and auto-refreshes
                        QVERIFY(clickButton(QStringLiteral("onboardLog_eraseSelectedButton")));
                        QVERIFY(waitForDialog(QStringLiteral("Delete Selected Onboard Log Files")));
                        QVERIFY(acceptDialog());
                        QTRY_VERIFY_WITH_TIMEOUT(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_2"), 50) == nullptr, 15000);
                        QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("onboardLogCheckbox_1"), 2000),
                                 "two logs must remain after selective erase");
                        _verifyIdleButtonStates(2);
                        if (QTest::currentTestFailed())
                            return;

                        _eraseAllAndVerifyEmpty();
                    });
}

void OnboardLogUITest::_ftpDownloadUITest()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(MockConfiguration::OptionFtpCapability); },
                    [&](QPointer<MockLink> mockLink, Vehicle* /*vehicle*/) {
                        const QList<MockLinkFTP::LogFile> logFiles = {
                            {QStringLiteral("log_1.ulg"), 5000, 1700000000},
                            {QStringLiteral("log_2.ulg"), 12345, 1700086400},
                        };
                        mockLink->mockLinkFTP()->setLogFiles(logFiles);

                        _navigateToOnboardLogsPage();
                        if (QTest::currentTestFailed())
                            return;

                        QTemporaryDir tempDir;
                        QVERIFY(tempDir.isValid());

                        _downloadFirstLogAndVerify(tempDir.path());
                        if (QTest::currentTestFailed())
                            return;

                        // Entries are sorted newest-first, so row 0 is log_2.ulg. FTP downloads keep the remote name.
                        const QString downloadFile = QDir(tempDir.path()).filePath(QStringLiteral("log_2.ulg"));
                        QVERIFY(QFile::exists(downloadFile));

                        QFile file(downloadFile);
                        QVERIFY(file.open(QIODevice::ReadOnly));
                        QCOMPARE(file.readAll(), mockLink->mockLinkFTP()->logFileContents(QStringLiteral("log_2.ulg")));
                    });
}
