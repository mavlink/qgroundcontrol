#include "FTPManagerTest.h"

#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "FTPManager.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"
const FTPManagerTest::TestCase_t FTPManagerTest::_rgTestCases[] = {
    {"/general.json"},
};

void FTPManagerTest::cleanup()
{
    _disconnectMockLink();
}

void FTPManagerTest::_testCaseWorker(const TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);
    // void downloadComplete   (const QString& file, const QString& errorMsg);
    ftpManager->download(MAV_COMP_ID_AUTOPILOT1, testCase.file,
                         QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
    QCOMPARE(spyDownloadComplete.count(), 1);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    QVERIFY2(arguments[1].toString().isEmpty(), qPrintable(arguments[1].toString()));
    _disconnectMockLink();
}

void FTPManagerTest::_sizeTestCaseWorker(int fileSize)
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);
    ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                         QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
    QCOMPARE(spyDownloadComplete.count(), 1);
    // void downloadComplete   (const QString& file, const QString& errorMsg);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    QVERIFY(arguments[1].toString().isEmpty());
    _verifyFileSizeAndDelete(arguments[0].toString(), fileSize);
    _disconnectMockLink();
}

void FTPManagerTest::_performSizeBasedTestCases_data()
{
    QTest::addColumn<int>("fileSize");
    const int dataSize = sizeof(((MavlinkFTP::Request*)nullptr)->data);
    QTest::addRow("single_packet_partial") << (dataSize - 1);
    QTest::addRow("single_packet_full") << dataSize;
    QTest::addRow("single_packet_plus_one") << (dataSize + 1);
    QTest::addRow("multi_burst") << (3 * 1024);
}

void FTPManagerTest::_performSizeBasedTestCases()
{
    QFETCH(int, fileSize);
    TEST_DEBUG(QStringLiteral("Testing size case %1 (%2 bytes)")
                   .arg(QTest::currentDataTag() ? QTest::currentDataTag() : "unknown")
                   .arg(fileSize));
    _sizeTestCaseWorker(fileSize);
}

void FTPManagerTest::_performTestCases_data()
{
    QTest::addColumn<QString>("file");
    int i = 0;
    for (const TestCase_t& testCase : _rgTestCases) {
        QTest::addRow("case_%d", i) << QString::fromLatin1(testCase.file);
        ++i;
    }
}

void FTPManagerTest::_performTestCases()
{
    QFETCH(QString, file);
    const QByteArray fileUtf8 = file.toUtf8();
    TestCase_t testCase = {fileUtf8.constData()};
    TEST_DEBUG(QStringLiteral("Testing file case %1: %2")
                   .arg(QTest::currentDataTag() ? QTest::currentDataTag() : "unknown", file));
    _testCaseWorker(testCase);
}

void FTPManagerTest::_testLostPackets()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    int fileSize = 4 * 1024;
    QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);
    _mockLink->mockLinkFTP()->enableRandomDrops(true);
    ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                         QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
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
    for (int i = 0; i < expectedSize; i++) {
        QByteArray bytes = file.read(1);
        QCOMPARE(bytes[0], (char)(i % 255));
    }
    file.close();
    file.remove();
}

void FTPManagerTest::_testListDirectory()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    TEST_DEBUG(QStringLiteral("listDirectory entries: %1").arg(arguments[0].toStringList().join(',')));
    QCOMPARE(arguments[0].toStringList().count(), 6);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QVERIFY(!arguments[1].toString().isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNakResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QVERIFY(!arguments[1].toString().isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoSecondResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QVERIFY(!arguments[1].toString().isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoSecondResponseAllowRetry()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    TEST_DEBUG(QStringLiteral("listDirectory retry entries: %1").arg(arguments[0].toStringList().join(',')));
    QCOMPARE(arguments[0].toStringList().count(), 6);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNakSecondResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakSecondResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QVERIFY(!arguments[1].toString().isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryBadSequence()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeBadSequence);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QVERIFY(!arguments[1].toString().isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryCancel()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
    ftpManager->cancelListDirectory();
    // listDirectoryComplete is signalled immediately on calling cancelListDirectory so no need to wait
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    QCOMPARE(arguments[0].toStringList().count(), 0);
    QCOMPARE(arguments[1].toString(), QStringLiteral("Aborted"));
    _disconnectMockLink();
}

void FTPManagerTest::_testUpload()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->clearUploadedFiles();
    FTPManager* ftpManager = _vehicle->ftpManager();
    const QString remotePath(QStringLiteral("/mock/upload/test.bin"));
    const int chunkSize = sizeof(((MavlinkFTP::Request*)nullptr)->data);
    const int payloadSize = (chunkSize * 2) + 7;
    QByteArray payload(payloadSize, 0);
    for (int i = 0; i < payloadSize; ++i) {
        payload[i] = static_cast<char>((i % 251) + 1);
    }
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QCOMPARE(tempFile.write(payload), static_cast<qint64>(payload.size()));
    tempFile.close();
    QSignalSpy spyUploadComplete(ftpManager, &FTPManager::uploadComplete);
    QVERIFY(ftpManager->upload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName()));
    QVERIFY_SIGNAL_WAIT(spyUploadComplete, TestTimeout::longMs());
    QCOMPARE(spyUploadComplete.count(), 1);
    QList<QVariant> arguments = spyUploadComplete.takeFirst();
    QCOMPARE(arguments[0].toString(), remotePath);
    QVERIFY(arguments[1].toString().isEmpty());
    QVERIFY(_mockLink->mockLinkFTP()->uploadedFiles().contains(remotePath));
    const QByteArray uploadedPayload = _mockLink->mockLinkFTP()->uploadedFileContents(remotePath);
    QCOMPARE(uploadedPayload, payload);
    _mockLink->mockLinkFTP()->clearUploadedFiles();
    _disconnectMockLink();
}

UT_REGISTER_TEST(FTPManagerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)
