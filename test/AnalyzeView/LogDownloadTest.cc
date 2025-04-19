/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogDownloadTest.h"
#include "LogDownloadController.h"
#include "LogEntry.h"
#include "MockLink.h"
#include "MultiSignalSpyV2.h"
#include "QmlObjectListModel.h"

#include "MultiVehicleManager.h"
#include "MAVLinkProtocol.h"

#include <QtCore/QDir>
#include <QtTest/QTest>

void LogDownloadTest::_downloadTest()
{
    MultiVehicleManager::instance()->init();
    MAVLinkProtocol::instance()->init();

    _connectMockLink(MAV_AUTOPILOT_PX4);

    LogDownloadController *const controller = new LogDownloadController(this);
    MultiSignalSpyV2 *multiSpyLogDownloadController = new MultiSignalSpyV2(this);
    QVERIFY(multiSpyLogDownloadController->init(controller));

    controller->refresh();
    QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", 10000));
    multiSpyLogDownloadController->clearAllSignals();
    if (controller->_getRequestingList()) {
        QVERIFY(multiSpyLogDownloadController->waitForSignal("requestingListChanged", 10000));
        QCOMPARE(controller->_getRequestingList(), false);
    }
    multiSpyLogDownloadController->clearAllSignals();

    QmlObjectListModel *const model = controller->_getModel();
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

    (void) QFile::remove(downloadFile);
}
