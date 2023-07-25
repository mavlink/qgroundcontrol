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
    {  "/general.json" },
};

void FTPManagerTest::cleanup(void)
{
    _disconnectMockLink();
}

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

void FTPManagerTest::_sizeTestCaseWorker(int fileSize)
{
    _connectMockLinkNoInitialConnectSequence();

    FTPManager* ftpManager  = _vehicle->ftpManager();
    QString     filename    = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);

    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);

    ftpManager->download(filename, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QCOMPARE(spyDownloadComplete.wait(10000), true);
    QCOMPARE(spyDownloadComplete.count(), 1);

    // void downloadComplete   (const QString& file, const QString& errorMsg);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    QVERIFY(arguments[1].toString().isEmpty());

    _verifyFileSizeAndDelete(arguments[0].toString(), fileSize);

    _disconnectMockLink();
}

void FTPManagerTest::_performSizeBasedTestCases(void)
{
    // We test various boundary conditions on file sizes with respect to buffer sizes
    const QList<int> rgSizeTestCases = {
        // File fits one Read Ack packet, partially filling data
        sizeof(((MavlinkFTP::Request*)0)->data) - 1,
        // File fits one Read Ack packet, exactly filling all data
        sizeof(((MavlinkFTP::Request*)0)->data),
        // File is larger than a single Read Ack packets, requires multiple Reads
        sizeof(((MavlinkFTP::Request*)0)->data) + 1,
        // File is large enough to require multiple bursts
        3 * 1024,
    };

    for (int fileSize: rgSizeTestCases) {
        _sizeTestCaseWorker(fileSize);
    }
}

void FTPManagerTest::_performTestCases(void)
{
    int index = 0;
    for (const TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}

void FTPManagerTest::_testLostPackets(void)
{
    _connectMockLinkNoInitialConnectSequence();

    FTPManager* ftpManager  = _vehicle->ftpManager();
    int         fileSize    = 4 * 1024;
    QString     filename    = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);

    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);

    _mockLink->mockLinkFTP()->enableRandromDrops(true);
    ftpManager->download(filename, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    QCOMPARE(spyDownloadComplete.wait(10000), true);
    QCOMPARE(spyDownloadComplete.count(), 1);

    // void downloadComplete   (const QString& file, const QString& errorMsg);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    QVERIFY(arguments[1].toString().isEmpty());

    _verifyFileSizeAndDelete(arguments[0].toString(), fileSize);

    _disconnectMockLink();
}

void FTPManagerTest::_verifyFileSizeAndDelete(const QString& filename, int expectedSize)
{
    QFileInfo fileInfo(filename);

    QVERIFY(fileInfo.exists());
    QCOMPARE(fileInfo.size(), expectedSize);

    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    for (int i=0; i<expectedSize; i++) {
        QByteArray bytes = file.read(1);
        QCOMPARE(bytes[0], (char)(i % 255));
    }
    file.close();
    file.remove();
}
