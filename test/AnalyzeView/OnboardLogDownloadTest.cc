#include "OnboardLogDownloadTest.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimeZone>
#include <QtCore/QTimer>

#include "OnboardLogController.h"
#include "OnboardLogEntry.h"
#include "MAVLinkProtocol.h"
#include "MockLinkFTP.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

// Waits until an already started listing cycle has fully completed.
static bool waitForListComplete(OnboardLogController *controller, MultiSignalSpy *multiSpy)
{
    if (!multiSpy->waitForSignal("requestingListChanged", TestTimeout::longMs())) {
        return false;
    }
    multiSpy->clearAllSignals();
    if (controller->property("requestingList").toBool()) {
        if (!multiSpy->waitForSignal("requestingListChanged", TestTimeout::longMs())) {
            return false;
        }
        if (controller->property("requestingList").toBool()) {
            return false;
        }
    }
    multiSpy->clearAllSignals();
    return true;
}

// Runs a refresh and waits until the listing cycle has fully completed.
static bool refreshAndWaitForListComplete(OnboardLogController *controller, MultiSignalSpy *multiSpy)
{
    multiSpy->clearAllSignals();
    controller->refresh();
    return waitForListComplete(controller, multiSpy);
}

// Starts a download and waits until the download cycle has fully completed.
static bool downloadAndWaitForComplete(OnboardLogController *controller, MultiSignalSpy *multiSpy, const QString &downloadTo)
{
    multiSpy->clearAllSignals();
    controller->download(downloadTo);
    if (!multiSpy->waitForSignal("downloadingLogsChanged", TestTimeout::longMs())) {
        return false;
    }
    multiSpy->clearAllSignals();
    if (controller->property("downloadingLogs").toBool()) {
        if (!multiSpy->waitForSignal("downloadingLogsChanged", TestTimeout::longMs())) {
            return false;
        }
        if (controller->property("downloadingLogs").toBool()) {
            return false;
        }
    }
    multiSpy->clearAllSignals();
    return true;
}

void OnboardLogDownloadTest::_downloadTest()
{
    // VehicleTest::init() already connects the mock link
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    // MockLink does not advertise FTP capability by default so the message transport must be used
    QCOMPARE(controller->transport(), QStringLiteral("messages"));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    model->value<QGCOnboardLogEntry*>(0)->setSelected(true);

    const QString downloadTo = QDir::currentPath();
    QVERIFY(downloadAndWaitForComplete(controller, multiSpy, downloadTo));

    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->status(), QStringLiteral("Downloaded"));
    (void)QFile::remove(downloadFile);
}

void OnboardLogDownloadTest::_selectAllTest()
{
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QVERIFY(model->count() > 0);

    QCOMPARE(controller->selectedCount(), 0);
    QVERIFY(!controller->allLogsSelected());

    controller->selectAll(true);
    QCOMPARE(controller->selectedCount(), model->count());
    QVERIFY(controller->allLogsSelected());

    controller->selectAll(false);
    QCOMPARE(controller->selectedCount(), 0);
    QVERIFY(!controller->allLogsSelected());
}

void OnboardLogDownloadTest::_cancelDownloadTest()
{
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    QGCOnboardLogEntry* const entry = model->value<QGCOnboardLogEntry*>(0);
    entry->setSelected(true);

    // download() synchronously creates the local file and requests the first chunk,
    // so canceling immediately exercises the cancel-while-downloading path.
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);
    QVERIFY(controller->_getDownloadingLogs());
    controller->cancel();

    QVERIFY(!controller->_getDownloadingLogs());
    QCOMPARE(entry->status(), QStringLiteral("Canceled"));
    QCOMPARE(controller->selectedCount(), 0);

    // The partially downloaded file must have been removed
    QVERIFY(!QFile::exists(QDir(downloadTo).filePath("log_0_UnknownDate.ulg")));
}

void OnboardLogDownloadTest::_vehicleDisconnectDuringDownloadTest()
{
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    model->value<QGCOnboardLogEntry*>(0)->setSelected(true);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    controller->download(tempDir.path());
    QVERIFY(controller->_getDownloadingLogs());

    // The active vehicle going away mid-download must tear down all download
    // state: the model entries are deleted so nothing may reference them afterwards.
    controller->_setActiveVehicle(nullptr);

    QVERIFY(!controller->_getDownloadingLogs());
    QVERIFY(!controller->_getRequestingList());
    QVERIFY(!controller->_downloadData);
    QVERIFY(!controller->_timer->isActive());
    QCOMPARE(model->count(), 0);

    // The partially downloaded file must have been removed
    QVERIFY(!QFile::exists(QDir(tempDir.path()).filePath("log_0_UnknownDate.ulg")));
}

void OnboardLogDownloadTest::_eraseAllTest()
{
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QCOMPARE(controller->_getModel()->count(), 1);

    // eraseAll sends LOG_ERASE and then automatically refreshes the list
    multiSpy->clearAllSignals();
    controller->eraseAll();
    QVERIFY(waitForListComplete(controller, multiSpy));
    QCOMPARE(controller->_getModel()->count(), 0);
}

void OnboardLogFtpDownloadTest::_ftpListAndDownloadTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    QVERIFY(_vehicle->capabilitiesKnown());
    QVERIFY(_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_FTP);

    const QList<MockLinkFTP::LogFile> logFiles = {
        { QStringLiteral("log_1.ulg"), 5000,  1700000000 },
        { QStringLiteral("log_2.ulg"), 12345, 1700086400 },
    };
    _mockLink->mockLinkFTP()->setLogFiles(logFiles);

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    QCOMPARE(controller->transport(), QStringLiteral("ftp"));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 2);

    // Entries are sorted by time so look them up by size
    QGCOnboardLogEntry *firstLog = nullptr;
    QGCOnboardLogEntry *secondLog = nullptr;
    for (int i = 0; i < model->count(); i++) {
        QGCOnboardLogEntry *const entry = model->value<QGCOnboardLogEntry*>(i);
        QVERIFY(entry);
        QVERIFY(entry->received());
        QCOMPARE(entry->status(), QStringLiteral("Available"));
        if (entry->size() == 5000) {
            firstLog = entry;
        } else if (entry->size() == 12345) {
            secondLog = entry;
        }
    }
    QVERIFY(firstLog);
    QVERIFY(secondLog);
    QCOMPARE(firstLog->time(),  QDateTime::fromSecsSinceEpoch(1700000000, QTimeZone::UTC));
    QCOMPARE(secondLog->time(), QDateTime::fromSecsSinceEpoch(1700086400, QTimeZone::UTC));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    firstLog->setSelected(true);
    QVERIFY(downloadAndWaitForComplete(controller, multiSpy, tempDir.path()));

    // FTP downloads are saved under the remote log file name
    const QString downloadFile = QDir(tempDir.path()).filePath(QStringLiteral("log_1.ulg"));
    QVERIFY(QFile::exists(downloadFile));
    QCOMPARE(firstLog->status(), QStringLiteral("Downloaded"));

    QFile file(downloadFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), _mockLink->mockLinkFTP()->logFileContents(QStringLiteral("log_1.ulg")));
}

void OnboardLogFtpDownloadTest::_ftpListFallbackTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    // Nak all FTP requests so the FTP listing fails and the controller must
    // fall back to the message-based transport.
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    expectLogMessage("AnalyzeView.OnboardLogController", QtWarningMsg, QRegularExpression(QStringLiteral("ftp: listing error")));

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    verifyExpectedLogMessage();

    QCOMPARE(controller->transport(), QStringLiteral("messages"));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    QVERIFY(model->value<QGCOnboardLogEntry*>(0)->received());
}

void OnboardLogFtpDownloadTest::_ftpMultiDownloadAndDedupTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    const QList<MockLinkFTP::LogFile> logFiles = {
        { QStringLiteral("log_1.ulg"), 4000, 1700000000 },
        { QStringLiteral("log_2.ulg"), 5000, 1700086400 },
        { QStringLiteral("log_3.ulg"), 6000, 1700172800 },
    };
    _mockLink->mockLinkFTP()->setLogFiles(logFiles);

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QmlObjectListModel* const model = controller->_getModel();
    QCOMPARE(model->count(), 3);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Pre-existing file with the same name as a remote log must not be overwritten
    const QByteArray junk = QByteArrayLiteral("do not overwrite");
    QFile junkFile(QDir(tempDir.path()).filePath(QStringLiteral("log_1.ulg")));
    QVERIFY(junkFile.open(QIODevice::WriteOnly));
    QCOMPARE(junkFile.write(junk), junk.size());
    junkFile.close();

    controller->selectAll(true);
    QVERIFY(downloadAndWaitForComplete(controller, multiSpy, tempDir.path()));

    // All selected logs downloaded sequentially, selection cleared
    QCOMPARE(controller->selectedCount(), 0);
    for (int i = 0; i < model->count(); i++) {
        QCOMPARE(model->value<QGCOnboardLogEntry*>(i)->status(), QStringLiteral("Downloaded"));
    }

    // log_1.ulg collided with the pre-existing file so it was saved with a _1 suffix
    const QHash<QString, QString> expectedFiles = {
        { QStringLiteral("log_1.ulg"), QStringLiteral("log_1_1.ulg") },
        { QStringLiteral("log_2.ulg"), QStringLiteral("log_2.ulg") },
        { QStringLiteral("log_3.ulg"), QStringLiteral("log_3.ulg") },
    };
    for (auto it = expectedFiles.constBegin(); it != expectedFiles.constEnd(); ++it) {
        QFile file(QDir(tempDir.path()).filePath(it.value()));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(it.value()));
        QCOMPARE(file.readAll(), _mockLink->mockLinkFTP()->logFileContents(it.key()));
    }

    // The pre-existing file is untouched
    QVERIFY(junkFile.open(QIODevice::ReadOnly));
    QCOMPARE(junkFile.readAll(), junk);
}

void OnboardLogFtpDownloadTest::_ftpDownloadErrorDisablesFtpTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    _mockLink->mockLinkFTP()->setLogFiles({ { QStringLiteral("log_1.ulg"), 5000, 1700000000 } });

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QCOMPARE(controller->transport(), QStringLiteral("ftp"));
    QmlObjectListModel* const model = controller->_getModel();
    QCOMPARE(model->count(), 1);

    // Listing succeeded but downloads fail: entry must be marked Error and
    // FTP must be disabled for subsequent refreshes.
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    expectLogMessage("AnalyzeView.OnboardLogController", QtWarningMsg, QRegularExpression(QStringLiteral("ftp: download error")));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    model->value<QGCOnboardLogEntry*>(0)->setSelected(true);
    QVERIFY(downloadAndWaitForComplete(controller, multiSpy, tempDir.path()));
    verifyExpectedLogMessage();

    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->status(), QStringLiteral("Error"));

    // Subsequent refreshes must use the message-based transport
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);
    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QCOMPARE(controller->transport(), QStringLiteral("messages"));
    QCOMPARE(controller->_getModel()->count(), 1);
}

void OnboardLogFtpDownloadTest::_ftpSortOrderTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    const QDateTime olderTime = QDateTime::fromSecsSinceEpoch(1700000000, QTimeZone::UTC);
    const QDateTime newerTime = QDateTime::fromSecsSinceEpoch(1700086400, QTimeZone::UTC);
    const QList<MockLinkFTP::LogFile> logFiles = {
        { QStringLiteral("log_old.ulg"), 4000, 1700000000 },
        { QStringLiteral("log_new.ulg"), 5000, 1700086400 },
    };
    _mockLink->mockLinkFTP()->setLogFiles(logFiles);

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QmlObjectListModel* const model = controller->_getModel();
    QCOMPARE(model->count(), 2);

    // Default sort is descending: newest first
    QVERIFY(!controller->sortAscending());
    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->time(), newerTime);
    QCOMPARE(model->value<QGCOnboardLogEntry*>(1)->time(), olderTime);

    // Changing the sort order re-sorts the existing model
    controller->setSortAscending(true);
    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->time(), olderTime);
    QCOMPARE(model->value<QGCOnboardLogEntry*>(1)->time(), newerTime);

    controller->toggleSortByDate();
    QVERIFY(!controller->sortAscending());
    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->time(), newerTime);
}

void OnboardLogFtpDownloadTest::_ftpCancelDownloadTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    // Large enough that the download cannot complete before cancel() is called
    _mockLink->mockLinkFTP()->setLogFiles({ { QStringLiteral("log_big.ulg"), 1000000, 1700000000 } });

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QmlObjectListModel* const model = controller->_getModel();
    QCOMPARE(model->count(), 1);
    QGCOnboardLogEntry* const entry = model->value<QGCOnboardLogEntry*>(0);
    entry->setSelected(true);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    controller->download(tempDir.path());
    QVERIFY(controller->_getDownloadingLogs());
    controller->cancel();

    QVERIFY(!controller->_getDownloadingLogs());
    QCOMPARE(entry->status(), QStringLiteral("Canceled"));
}

void OnboardLogFtpDownloadTest::_ftpEraseSelectedTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    const QList<MockLinkFTP::LogFile> logFiles = {
        { QStringLiteral("log_1.ulg"), 4000, 1700000000 },
        { QStringLiteral("log_2.ulg"), 5000, 1700086400 },
        { QStringLiteral("log_3.ulg"), 6000, 1700172800 },
    };
    _mockLink->mockLinkFTP()->setLogFiles(logFiles);

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));
    QCOMPARE(controller->transport(), QStringLiteral("ftp"));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 3);

    // Select the two oldest logs (sizes 4000 and 5000) for erase
    for (int i = 0; i < model->count(); i++) {
        QGCOnboardLogEntry* const entry = model->value<QGCOnboardLogEntry*>(i);
        QVERIFY(entry);
        entry->setSelected(entry->size() != 6000);
    }
    QCOMPARE(controller->selectedCount(), 2);

    // eraseSelected deletes only the selected logs via FTP and automatically refreshes
    multiSpy->clearAllSignals();
    controller->eraseSelected();
    QVERIFY(waitForListComplete(controller, multiSpy));

    QCOMPARE(model->count(), 1);
    QCOMPARE(model->value<QGCOnboardLogEntry*>(0)->size(), 6000U);
    QCOMPARE(controller->selectedCount(), 0);
}

void OnboardLogFtpDownloadTest::_ftpCancelEraseSelectedTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionFtpCapability);
    if (QTest::currentTestFailed()) return;

    const QList<MockLinkFTP::LogFile> logFiles = {
        { QStringLiteral("log_1.ulg"), 4000, 1700000000 },
        { QStringLiteral("log_2.ulg"), 5000, 1700086400 },
        { QStringLiteral("log_3.ulg"), 6000, 1700172800 },
    };
    _mockLink->mockLinkFTP()->setLogFiles(logFiles);

    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpy = new MultiSignalSpy(this);
    QVERIFY(multiSpy->init(controller));

    QVERIFY(refreshAndWaitForListComplete(controller, multiSpy));

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    QCOMPARE(model->count(), 3);

    controller->selectAll(true);
    QCOMPARE(controller->selectedCount(), 3);

    // eraseSelected starts deleting the first queued log (newest first), then
    // cancel drops the remaining queued deletes. The in-flight delete cannot be
    // aborted; when it completes the controller must automatically refresh so
    // the list reflects the actual vehicle state.
    multiSpy->clearAllSignals();
    controller->eraseSelected();
    QVERIFY(controller->_getDownloadingLogs());
    controller->cancel();

    QVERIFY(waitForListComplete(controller, multiSpy));

    // Only the first queued delete (log_3, newest) went through
    QCOMPARE(model->count(), 2);
    QCOMPARE(controller->selectedCount(), 0);
    QVERIFY(!controller->_getDownloadingLogs());
}

UT_REGISTER_TEST(OnboardLogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
UT_REGISTER_TEST(OnboardLogFtpDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
