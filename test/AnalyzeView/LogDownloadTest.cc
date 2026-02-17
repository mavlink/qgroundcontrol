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
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", 10000));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", 10000));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    QmlObjectListModel* const model = controller->_getModel();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);
    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", 10000));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getDownloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", 10000));
        QCOMPARE(controller->_getDownloadingLogs(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);
}

UT_REGISTER_TEST(LogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
