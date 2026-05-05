#include "FTPManagerTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <cstring>

#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "FTPManagerOperations.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"
const FTPManagerTest::TestCase_t FTPManagerTest::_rgTestCases[] = {
    {"/general.json"},
};

void FTPManagerTest::cleanup()
{
    VehicleTestManualConnect::cleanup();
}

void FTPManagerTest::_testCaseWorker(const TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);
    // void downloadComplete   (const QString& file, const QString& errorMsg);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, testCase.file,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
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
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
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
    const int dataSize = sizeof(((MavlinkFTP::Request*) nullptr)->data);
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
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
    QCOMPARE(spyDownloadComplete.count(), 1);
    // void downloadComplete   (const QString& file, const QString& errorMsg);
    QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    QVERIFY(arguments[1].toString().isEmpty());
    _verifyFileSizeAndDelete(arguments[0].toString(), fileSize);
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadCancelBeforeOpen()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    QSignalSpy downloadCompleteSpy(ftpManager, &FTPManager::downloadComplete);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path()));

    ftpManager->cancelDownload();

    QCOMPARE(downloadCompleteSpy.size(), 1);
    const QList<QVariant> canceledArguments = downloadCompleteSpy.takeFirst();
    QCOMPARE(canceledArguments[1].toString(), QStringLiteral("Aborted"));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path()));
    QVERIFY_SIGNAL_WAIT(downloadCompleteSpy, TestTimeout::longMs());
    const QList<QVariant> completedArguments = downloadCompleteSpy.takeFirst();
    QVERIFY(completedArguments[1].toString().isEmpty());
    const QString completedFile = completedArguments[0].toString();
    QCOMPARE(QFileInfo(completedFile).size(), 64);

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("next.bin")));
    ftpManager->cancelDownload();
    QCOMPARE(downloadCompleteSpy.size(), 1);
    QVERIFY(!downloadCompleteSpy.takeFirst()[1].toString().isEmpty());
    QVERIFY(QFileInfo::exists(completedFile));

    _verifyFileSizeAndDelete(completedFile, 64);
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadSessionCleanup()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    QSignalSpy terminateSpy(mockFtp, &MockLinkFTP::terminateCommandReceived);
    QSignalSpy completionSpy(ftpManager, &FTPManager::downloadComplete);

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    QVERIFY(
        ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("malformed.bin")));

    MavlinkFTP::Request malformedOpenResponse{};
    malformedOpenResponse.hdr.opcode = MAV_FTP_OPCODE_ACK;
    malformedOpenResponse.hdr.req_opcode = MAV_FTP_OPCODE_OPENFILERO;
    malformedOpenResponse.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    malformedOpenResponse.hdr.session = 1;
    malformedOpenResponse.hdr.size = 0;
    ftpManager->_openFileROAckOrNak(&malformedOpenResponse);

    QCOMPARE(completionSpy.size(), 1);
    QCOMPARE(terminateSpy.size(), 0);
    QVERIFY(!completionSpy.takeFirst()[1].toString().isEmpty());
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    terminateSpy.clear();
    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    const QString missingDirectory = downloadDir.filePath(QStringLiteral("missing"));
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, missingDirectory, QStringLiteral("local-open.bin")));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    QCOMPARE(terminateSpy.size(), 1);
    QVERIFY(!completionSpy.takeFirst()[1].toString().isEmpty());
    QVERIFY(!QFileInfo::exists(missingDirectory));

    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadFinalizeFailure()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    const QString finalPath = downloadDir.filePath(QStringLiteral("blocked.bin"));
    QVERIFY(QDir().mkpath(finalPath));

    QSignalSpy completionSpy(ftpManager, &FTPManager::downloadComplete);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("blocked.bin")));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());

    const QList<QVariant> arguments = completionSpy.takeFirst();
    QCOMPARE(arguments[0].toString(), finalPath);
    QVERIFY(arguments[1].toString().contains(QStringLiteral("Unable to replace existing download")));
    QVERIFY(QFileInfo(finalPath).isDir());
    QVERIFY(!QFileInfo::exists(finalPath + QStringLiteral(".part")));

    const QString protectedPath = downloadDir.filePath(QStringLiteral("protected.bin"));
    const QByteArray protectedContents("existing file");
    QFile protectedFile(protectedPath);
    QVERIFY(protectedFile.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    QCOMPARE(protectedFile.write(protectedContents), protectedContents.size());
    protectedFile.close();

    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(),
                                 QFileInfo(protectedPath).fileName(), true,
                                 FTPManager::ExistingFilePolicy::FailIfExists));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());

    const QList<QVariant> protectedArguments = completionSpy.takeFirst();
    QCOMPARE(protectedArguments[0].toString(), protectedPath);
    QVERIFY(protectedArguments[1].toString().contains(QStringLiteral("destination already exists")));
    QVERIFY(protectedArguments[2].toString().isEmpty());
    QVERIFY(protectedFile.open(QIODevice::ReadOnly));
    QCOMPARE(protectedFile.readAll(), protectedContents);
    protectedFile.close();
    QVERIFY(!QFileInfo::exists(protectedPath + QStringLiteral(".part")));

    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadResetResponseLoss()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    QSignalSpy resetSpy(mockFtp, &MockLinkFTP::resetCommandReceived);
    QSignalSpy completionSpy(ftpManager, &FTPManager::downloadComplete);
    mockFtp->setResetCommandResponseDropCount(1);

    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    const QString localFileName = QStringLiteral("reset-warning.bin");
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());

    QCOMPARE(resetSpy.size(), 1);
    const QList<QVariant> arguments = completionSpy.takeFirst();
    QCOMPARE(arguments[0].toString(), downloadDir.filePath(localFileName));
    QVERIFY(arguments[1].toString().isEmpty());
    QVERIFY(arguments[2].toString().contains(QStringLiteral("resetting remote sessions")));
    QCOMPARE(QFileInfo(arguments[0].toString()).size(), 64);

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
        QCOMPARE(bytes[0], (char) (i % 255));
    }
    file.close();
    file.remove();
}

void FTPManagerTest::_testListDirectory()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    TEST_DEBUG(QStringLiteral("listDirectory entries: %1").arg(arguments[0].toStringList().join(',')));
    QCOMPARE(arguments[0].toStringList().count(), 6);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryLimit()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QSignalSpy completionSpy(ftpManager, &FTPManager::listDirectoryComplete);

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("negative entry limit")));
    QVERIFY(!ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), -1));
    verifyExpectedLogMessage();

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), 3));
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    QCOMPARE(completionSpy.size(), 1);

    const QList<QVariant> arguments = completionSpy.takeFirst();
    QCOMPARE(arguments[0].toStringList().size(), 3);
    QVERIFY(arguments[1].toString().isEmpty());
    QVERIFY(arguments[2].toBool());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryParsing()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    QSignalSpy completionSpy(ftpManager, &FTPManager::listDirectoryComplete);

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    MavlinkFTP::Request response{};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    QByteArray payload = QString::fromUtf8("F\xE6\x97\xA5\xE5\xBF\x97.ulg\t64").toUtf8();
    payload.append('\0');
    payload.append("Fnext.ulg\t32");
    payload.append('\0');
    QVERIFY(payload.size() <= static_cast<qsizetype>(sizeof(response.data)));
    std::memcpy(response.data, payload.constData(), static_cast<size_t>(payload.size()));
    response.hdr.size = static_cast<uint8_t>(payload.size());

    ftpManager->_listDirectoryAckOrNak(&response);

    QCOMPARE(ftpManager->_listDirectoryOperation->directoryEntries.size(), 2);
    QCOMPARE(ftpManager->_listDirectoryOperation->directoryEntries[0].section(QLatin1Char('\t'), 0, 0),
             QString::fromUtf8("F\xE6\x97\xA5\xE5\xBF\x97.ulg"));
    QCOMPARE(ftpManager->_listDirectoryOperation->directoryEntries[1], QStringLiteral("Fnext.ulg\t32"));
    QCOMPARE(ftpManager->_listDirectoryOperation->expectedOffset, 2U);

    ftpManager->cancelListDirectory();
    QCOMPARE(completionSpy.size(), 1);
    completionSpy.clear();

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    response.hdr.size = 3;
    response.data[0] = 'b';
    response.data[1] = 'a';
    response.data[2] = 'd';

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("unterminated entry")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> malformedArguments = completionSpy.takeFirst();
    QVERIFY(malformedArguments[0].toStringList().isEmpty());
    QVERIFY(malformedArguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!malformedArguments[2].toBool());

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid directory-list response size")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> emptyArguments = completionSpy.takeFirst();
    QVERIFY(emptyArguments[0].toStringList().isEmpty());
    QVERIFY(emptyArguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!emptyArguments[2].toBool());

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    response.hdr.size = 3;
    response.data[0] = 'F';
    response.data[1] = 0xFF;
    response.data[2] = '\0';

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("invalid UTF-8")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> utf8Arguments = completionSpy.takeFirst();
    QVERIFY(utf8Arguments[0].toStringList().isEmpty());
    QVERIFY(utf8Arguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!utf8Arguments[2].toBool());

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_NONE;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("received invalid response opcode")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> opcodeArguments = completionSpy.takeFirst();
    QVERIFY(opcodeArguments[0].toStringList().isEmpty());
    QVERIFY(opcodeArguments[1].toString().contains(QStringLiteral("malformed response")));

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_NAK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("received a NAK without an error code")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> nakArguments = completionSpy.takeFirst();
    QVERIFY(nakArguments[0].toStringList().isEmpty());
    QVERIFY(nakArguments[1].toString().contains(QStringLiteral("malformed response")));

    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")));
    ftpManager->cancelListDirectory();
    QCOMPARE(completionSpy.size(), 1);
    _disconnectMockLink();
}

void FTPManagerTest::_testOperationStartValidation()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    const QString invalidUri = QStringLiteral("https://example.com/file");
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    QVERIFY(!ftpManager->download(MAV_COMP_ID_AUTOPILOT1, invalidUri, destination.path()));
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_downloadOperation->inProgress());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    QVERIFY(!ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, invalidUri));
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_listDirectoryOperation->inProgress());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    QVERIFY(!ftpManager->deleteFile(MAV_COMP_ID_AUTOPILOT1, invalidUri));
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_deleteOperation->inProgress());

    QTemporaryFile source;
    QVERIFY(source.open());
    QCOMPARE(source.write("ftp operation", 13), 13);
    source.close();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    QVERIFY(!ftpManager->upload(MAV_COMP_ID_AUTOPILOT1, invalidUri, source.fileName()));
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_uploadOperation->inProgress());

    const QString embeddedNullUri = QStringLiteral("mftp:///file") + QChar::Null + QStringLiteral("hidden");
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    QVERIFY(!ftpManager->deleteFile(MAV_COMP_ID_AUTOPILOT1, embeddedNullUri));
    verifyExpectedLogMessage();
    QVERIFY(!ftpManager->_deleteOperation->inProgress());

    _disconnectMockLink();
}

void FTPManagerTest::_testScopedListDirectoryJob()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();

    FTPListDirectoryJob* const completedJob =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"));
    QVERIFY(completedJob);
    QCOMPARE(completedJob->type(), FTPJob::Type::ListDirectory);
    QVERIFY(completedJob->active());

    QSignalSpy jobCompletionSpy(completedJob, &FTPListDirectoryJob::finished);
    QSignalSpy legacyCompletionSpy(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY_SIGNAL_WAIT(jobCompletionSpy, TestTimeout::longMs());
    QCOMPARE(jobCompletionSpy.size(), 1);
    QCOMPARE(legacyCompletionSpy.size(), 1);
    const QList<QVariant> completedArguments = jobCompletionSpy.takeFirst();
    QCOMPARE(completedArguments[0].toStringList().size(), 6);
    QVERIFY(completedArguments[1].toString().isEmpty());
    QVERIFY(!completedArguments[2].toBool());
    QVERIFY(!completedJob->active());

    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    FTPListDirectoryJob* const canceledJob =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"));
    QVERIFY(canceledJob);
    QSignalSpy canceledSpy(canceledJob, &FTPListDirectoryJob::finished);

    canceledJob->cancel();

    QCOMPARE(canceledSpy.size(), 1);
    const QList<QVariant> canceledArguments = canceledSpy.takeFirst();
    QVERIFY(canceledArguments[0].toStringList().isEmpty());
    QCOMPARE(canceledArguments[1].toString(), QStringLiteral("Aborted"));
    QVERIFY(!canceledJob->active());

    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryWithTime()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    const QStringList entries = arguments[0].toStringList();
    QCOMPARE(entries.count(), 6);
    QVERIFY(arguments[1].toString().isEmpty());

    // Each entry should carry "F<name>\t<size>\t<modification time>"
    for (int i = 0; i < entries.count(); i++) {
        const QStringList fields = entries.at(i).mid(1).split(QLatin1Char('\t'));
        QCOMPARE(fields.count(), 3);
        bool ok = false;
        const qint64 mtime = fields.at(2).toLongLong(&ok);
        QVERIFY(ok);
        QCOMPARE(mtime, static_cast<qint64>(MockLinkFTP::kMockModificationTime) + i);
    }
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryWithTimeFallback()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setListDirectoryWithTimeSupported(false);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    const QStringList entries = arguments[0].toStringList();
    QCOMPARE(entries.count(), 6);
    QVERIFY(arguments[1].toString().isEmpty());

    // After falling back to a plain listing, entries carry no modification-time field.
    for (const QString& entry : entries) {
        QCOMPARE(entry.mid(1).count(QLatin1Char('\t')), 1);
    }
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponse);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry);
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    QVERIFY(ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/"));
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
    const int chunkSize = sizeof(((MavlinkFTP::Request*) nullptr)->data);
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

void FTPManagerTest::_testUploadResetResponseLoss()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->clearUploadedFiles();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    const QByteArray payload("reset-response-loss");

    const auto uploadAndWait = [ftpManager, &payload](const QString& remotePath, QString& errorMessage) {
        QTemporaryFile tempFile;
        if (!tempFile.open() || (tempFile.write(payload) != payload.size())) {
            return false;
        }
        tempFile.close();

        QSignalSpy completionSpy(ftpManager, &FTPManager::uploadComplete);
        if (!ftpManager->upload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName())) {
            return false;
        }
        if (completionSpy.isEmpty() && !completionSpy.wait(TestTimeout::longMs())) {
            return false;
        }
        if (completionSpy.size() != 1) {
            return false;
        }

        const QList<QVariant> arguments = completionSpy.takeFirst();
        errorMessage = arguments[1].toString();
        return arguments[0].toString() == remotePath;
    };

    QString errorMessage;
    const QString retryPath(QStringLiteral("/mock/upload/reset-retry.bin"));
    QSignalSpy resetSpy(mockFtp, &MockLinkFTP::resetCommandReceived);
    mockFtp->setResetCommandResponseDropCount(1);
    QVERIFY(uploadAndWait(retryPath, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(resetSpy.size() >= 2);
    QCOMPARE(mockFtp->uploadedFileContents(retryPath), payload);

    const QString failurePath(QStringLiteral("/mock/upload/reset-failure.bin"));
    resetSpy.clear();
    mockFtp->setResetCommandResponseDropCount(FTPManager::_maxRetry + 1);
    QVERIFY(uploadAndWait(failurePath, errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("resetting remote sessions")));
    QCOMPARE(resetSpy.size(), FTPManager::_maxRetry + 1);
    QCOMPARE(mockFtp->uploadedFileContents(failurePath), payload);

    const QString recoveryPath(QStringLiteral("/mock/upload/reset-recovery.bin"));
    mockFtp->setResetCommandResponseDropCount(0);
    QVERIFY(uploadAndWait(recoveryPath, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(mockFtp->uploadedFileContents(recoveryPath), payload);

    mockFtp->clearUploadedFiles();
    _disconnectMockLink();
}

UT_REGISTER_TEST(FTPManagerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)
