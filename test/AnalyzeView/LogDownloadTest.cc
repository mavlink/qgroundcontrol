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
#include "MultiSignalSpy.h"
#include "QmlObjectListModel.h"

#include <QtCore/QDir>

void LogDownloadTest::_downloadTest()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);

    LogDownloadController* const controller = new LogDownloadController(this);

    _rgLogDownloadControllerSignals[requestingListChangedSignalIndex] = SIGNAL(requestingListChanged());
    _rgLogDownloadControllerSignals[downloadingLogsChangedSignalIndex] = SIGNAL(downloadingLogsChanged());
    _rgLogDownloadControllerSignals[modelChangedSignalIndex] = SIGNAL(modelChanged());

    _multiSpyLogDownloadController = new MultiSignalSpy(this);
    QVERIFY(_multiSpyLogDownloadController->init(controller, _rgLogDownloadControllerSignals, _cLogDownloadControllerSignals));

    controller->refresh();
    QVERIFY(_multiSpyLogDownloadController->waitForSignalByIndex(requestingListChangedSignalIndex, 10000));
    _multiSpyLogDownloadController->clearAllSignals();
    if (controller->requestingList()) {
        QVERIFY(_multiSpyLogDownloadController->waitForSignalByIndex(requestingListChangedSignalIndex, 10000));
        QCOMPARE(controller->requestingList(), false);
    }
    _multiSpyLogDownloadController->clearAllSignals();

    QmlObjectListModel* const model = controller->model();
    QVERIFY(model);
    model->value<QGCLogEntry*>(0)->setSelected(true);

    const QString downloadTo = QDir::currentPath();
    controller->downloadToDirectory(downloadTo);
    QVERIFY(_multiSpyLogDownloadController->waitForSignalByIndex(downloadingLogsChangedSignalIndex, 10000));
    _multiSpyLogDownloadController->clearAllSignals();
    if (controller->downloadingLogs()) {
        QVERIFY(_multiSpyLogDownloadController->waitForSignalByIndex(downloadingLogsChangedSignalIndex, 10000));
        QCOMPARE(controller->downloadingLogs(), false);
    }
    _multiSpyLogDownloadController->clearAllSignals();

    const QString downloadFile = QDir(downloadTo).filePath("log_0_UnknownDate.ulg");
    QVERIFY(UnitTest::fileCompare(downloadFile, _mockLink->logDownloadFile()));

    QFile::remove(downloadFile);
}
