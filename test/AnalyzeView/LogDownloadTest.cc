#include "LogDownloadTest.h"

#include <QtCore/QDir>

#include "LogDownloadController.h"
#include "LogEntry.h"
#include "MAVLinkProtocol.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"

void LogDownloadTest::_downloadTest()
{
    // VehicleTest::init() already connects the mock link
    LogDownloadController* const controller = new LogDownloadController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));
    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);
    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);
}

void LogDownloadTest::_downloadWithGapsTest()
{
    // Configure MockLink to drop every 3rd chunk
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadDropEveryNth, 3);

    LogDownloadController* const controller = new LogDownloadController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));

    // Refresh to get log list
    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();

    // Select and download
    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);

    // Wait for download to complete (may take longer due to retries)
    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs() * 2));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }

    // Verify downloaded file matches source
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);

    // Reset failure mode
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadNone);
}

void LogDownloadTest::_downloadMultipleGapsTest()
{
    // Drop every 2nd chunk to create multiple gaps
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadDropEveryNth, 2);

    LogDownloadController* const controller = new LogDownloadController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));

    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);

    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        // Allow extra time for multiple retry attempts
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs() * 3));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }

    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);

    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadNone);
}

void LogDownloadTest::_downloadStopMidstreamTest()
{
    // Configure MockLink to stop after 50% of file is sent
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadStopMidstream);

    LogDownloadController* const controller = new LogDownloadController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));

    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);

    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        // Should complete after controller retries and gets remaining data
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs() * 2));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }

    // Verify complete file was downloaded despite midstream stop
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);

    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadNone);
}

void LogDownloadTest::_downloadEfficientGapRetryTest()
{
    // Drop chunks 3, 4, 5 to create a 3-chunk contiguous gap
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadDropRange);
    _mockLink->setLogDownloadDropRange(3, 5);
    _mockLink->clearLogRequestDataHistory();

    LogDownloadController* const controller = new LogDownloadController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));

    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();

    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);

    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs() * 2));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }

    // Verify the download completed successfully
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));

    // Now verify efficient gap retry: should have made ONE request for the entire 3-chunk gap
    // Request history: [initial request, retry for gap]
    const QList<MockLink::LogRequestDataParams> history = _mockLink->getLogRequestDataHistory();

    // Find retry requests (exclude the initial full-file request at offset 0)
    int retryRequestCount = 0;
    int totalRetryBytes = 0;
    for (const auto& req : history) {
        if (req.offset > 0 && req.offset < 1000) {  // Retry requests are for specific offsets
            retryRequestCount++;
            totalRetryBytes += req.count;
        }
    }

    // Should have made exactly 1 retry request for the 3-chunk gap (3 * 90 = 270 bytes)
    QCOMPARE(retryRequestCount, 1);
    QCOMPARE(totalRetryBytes, 3 * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);

    (void)QFile::remove(downloadFile);
    _mockLink->setLogDownloadFailureMode(MockLink::FailLogDownloadNone);
}

UT_REGISTER_TEST(LogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
