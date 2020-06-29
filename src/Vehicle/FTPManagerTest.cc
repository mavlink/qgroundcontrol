/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FTPManagerTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"
#include "FTPManager.h"

const FTPManagerTest::TestCase_t FTPManagerTest::_rgTestCases[] = {
    {  "/version.json" },
};

void FTPManagerTest::_testCaseWorker(const TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();
    FTPManager*             ftpManager  = vehicle->ftpManager();

    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);

    // void downloadComplete   (const QString& file, const QString& errorMsg);
    ftpManager->download(testCase.file, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QCOMPARE(spyDownloadComplete.wait(10000), true);
    QCOMPARE(spyDownloadComplete.count(), 1);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    qDebug() << arguments[0].toString();
    QVERIFY(arguments[1].toString().isEmpty());

    _disconnectMockLink();
}

void FTPManagerTest::_performTestCases(void)
{
    int index = 0;
    for (const TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}
