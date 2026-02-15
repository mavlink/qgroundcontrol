#include "MAVLinkLogDownloadTest.h"

#include <QtCore/QDir>

#include "MAVLinkLogController.h"
#include "MAVLinkLogEntry.h"
#include "MAVLinkProtocol.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"

void MAVLinkLogDownloadTest::_downloadTest()
{
    // VehicleTest::init() already connects the mock link
    MAVLinkLogController* const controller = new MAVLinkLogController(this);
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
    model->value<QGCMAVLinkLogEntry*>(0)->setSelected(true);
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

UT_REGISTER_TEST(MAVLinkLogDownloadTest, TestLabel::Integration, TestLabel::AnalyzeView, TestLabel::Vehicle)
