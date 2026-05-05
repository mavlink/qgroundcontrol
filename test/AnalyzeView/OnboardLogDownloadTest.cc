#include "OnboardLogDownloadTest.h"

#include <QtCore/QDir>

#include "MAVLinkProtocol.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "OnboardLogController.h"
#include "OnboardLogEntry.h"
#include "QmlObjectListModel.h"

void OnboardLogDownloadTest::_downloadTest()
{
    // VehicleTest::init() already connects the mock link
    OnboardLogController* const controller = new OnboardLogController(this);
    MultiSignalSpy* multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));
    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->requestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", TestTimeout::longMs()));
        QCOMPARE(controller->requestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    QmlObjectListModel* const model = controller->model();
    QVERIFY(model);
    model->value<OnboardLogEntry*>(0)->setSelected(true);
    const QString downloadTo = QDir::currentPath();
    controller->download(downloadTo);
    QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->downloadingLogs()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("downloadingLogsChanged", TestTimeout::longMs()));
        QCOMPARE(controller->downloadingLogs(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();
    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));
    (void)QFile::remove(downloadFile);
}

UT_REGISTER_TEST(OnboardLogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
