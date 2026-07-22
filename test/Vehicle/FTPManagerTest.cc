#include "FTPManagerTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <cstring>
#include <limits>
#include <type_traits>

#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "FTPManagerOperations.h"
#include "MAVLinkFTP.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

static_assert(std::is_copy_assignable_v<FTPManager::DownloadStartResult>);
static_assert(std::is_move_assignable_v<FTPManager::DownloadStartResult>);
static_assert(std::is_copy_assignable_v<FTPManager::UploadStartResult>);
static_assert(std::is_move_assignable_v<FTPManager::UploadStartResult>);

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
    FTPDownloadJob* const job = ftpManager
                                    ->startDownload(MAV_COMP_ID_AUTOPILOT1, testCase.file,
                                                    QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                    .job();
    QVERIFY(job);
    QSignalSpy spyDownloadComplete(job, &FTPDownloadJob::finished);
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
    FTPDownloadJob* const job = ftpManager
                                    ->startDownload(MAV_COMP_ID_AUTOPILOT1, filename,
                                                    QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                    .job();
    QVERIFY(job);
    QSignalSpy spyDownloadComplete(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
    QCOMPARE(spyDownloadComplete.count(), 1);
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
    _mockLink->mockLinkFTP()->enableRandomDrops(true);
    FTPDownloadJob* const job = ftpManager
                                    ->startDownload(MAV_COMP_ID_AUTOPILOT1, filename,
                                                    QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                    .job();
    QVERIFY(job);
    QSignalSpy spyDownloadComplete(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(spyDownloadComplete, TestTimeout::longMs());
    QCOMPARE(spyDownloadComplete.count(), 1);
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
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    FTPDownloadJob* job = ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path()).job();
    QVERIFY(job);
    QSignalSpy downloadCompleteSpy(job, &FTPDownloadJob::finished);

    job->cancel();

    QCOMPARE(downloadCompleteSpy.size(), 1);
    const QList<QVariant> canceledArguments = downloadCompleteSpy.takeFirst();
    QCOMPARE(canceledArguments[1].toString(), QStringLiteral("Aborted"));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    job = ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path()).job();
    QVERIFY(job);
    QSignalSpy completedSpy(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(completedSpy, TestTimeout::longMs());
    const QList<QVariant> completedArguments = completedSpy.takeFirst();
    QVERIFY(completedArguments[1].toString().isEmpty());
    const QString completedFile = completedArguments[0].toString();
    QCOMPARE(QFileInfo(completedFile).size(), 64);

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    job = ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("next.bin"))
              .job();
    QVERIFY(job);
    QSignalSpy secondCancelSpy(job, &FTPDownloadJob::finished);
    job->cancel();
    QCOMPARE(secondCancelSpy.size(), 1);
    QVERIFY(!secondCancelSpy.takeFirst()[1].toString().isEmpty());
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
    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    FTPDownloadJob* job =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("malformed.bin"))
            .job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);

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
    job =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, missingDirectory, QStringLiteral("local-open.bin"))
            .job();
    QVERIFY(job);
    QSignalSpy missingDirectorySpy(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(missingDirectorySpy, TestTimeout::longMs());
    QCOMPARE(terminateSpy.size(), 1);
    QVERIFY(!missingDirectorySpy.takeFirst()[1].toString().isEmpty());
    QVERIFY(!QFileInfo::exists(missingDirectory));

    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadSizeLimit()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    QSignalSpy terminateSpy(mockFtp, &MockLinkFTP::terminateCommandReceived);
    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    const QString localFileName = QStringLiteral("oversized.bin");

    FTPDownloadJob* job =
        ftpManager
            ->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName, true,
                            FTPManager::ExistingFilePolicy::Replace, std::optional<uint32_t>{63U})
            .job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());

    QCOMPARE(terminateSpy.size(), 1);
    const QList<QVariant> arguments = completionSpy.takeFirst();
    QVERIFY(arguments[1].toString().contains(QStringLiteral("exceeds expected maximum")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());
    QVERIFY(!QFileInfo::exists(downloadDir.filePath(localFileName + QStringLiteral(".part"))));

    terminateSpy.clear();
    const QString exactFileName = QStringLiteral("exact-size.bin");
    job = ftpManager
              ->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), exactFileName, true,
                              FTPManager::ExistingFilePolicy::Replace, std::optional<uint32_t>{64U})
              .job();
    QVERIFY(job);
    QSignalSpy exactCompletionSpy(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(exactCompletionSpy, TestTimeout::longMs());
    const QList<QVariant> exactArguments = exactCompletionSpy.takeFirst();
    QVERIFY2(exactArguments[1].toString().isEmpty(), qPrintable(exactArguments[1].toString()));
    QCOMPARE(QFileInfo(exactArguments[0].toString()).size(), 64);
    QCOMPARE(terminateSpy.size(), 0);
    QVERIFY(QFile::remove(exactArguments[0].toString()));

    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadPacketSizeLimit()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    const QString localFileName = QStringLiteral("packet-oversized.bin");
    FTPDownloadJob* const job =
        ftpManager
            ->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName, true,
                            FTPManager::ExistingFilePolicy::Replace, std::optional<uint32_t>{64U})
            .job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);

    MavlinkFTP::Request openResponse{};
    openResponse.hdr.opcode = MAV_FTP_OPCODE_ACK;
    openResponse.hdr.req_opcode = MAV_FTP_OPCODE_OPENFILERO;
    openResponse.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    openResponse.hdr.session = 1;
    MavlinkFTP::setOpenFileLength(openResponse, 64U);
    ftpManager->_openFileROAckOrNak(&openResponse);
    QVERIFY(ftpManager->_downloadOperation->file.isOpen());

    MavlinkFTP::Request oversizedData{};
    oversizedData.hdr.opcode = MAV_FTP_OPCODE_ACK;
    oversizedData.hdr.req_opcode = MAV_FTP_OPCODE_BURSTREADFILE;
    oversizedData.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    oversizedData.hdr.session = 1;
    oversizedData.hdr.offset = 64U;
    oversizedData.hdr.size = 1U;
    oversizedData.data[0] = 0x5A;
    ftpManager->_burstReadFileAckOrNak(&oversizedData);

    QCOMPARE(ftpManager->_downloadOperation->file.size(), 0);
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    QVERIFY(completionSpy.takeFirst()[1].toString().contains(QStringLiteral("exceeds reported file size")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadPacketValidation()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    auto beginManualDownload = [&](const QString& localFileName, bool checkSize) {
        const std::optional<uint32_t> maximumFileSize =
            checkSize ? std::nullopt : std::optional<uint32_t>{(std::numeric_limits<uint32_t>::max)()};
        FTPDownloadJob* const job =
            ftpManager
                ->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName, checkSize,
                                FTPManager::ExistingFilePolicy::Replace, maximumFileSize)
                .job();
        if (!job) {
            return static_cast<FTPDownloadJob*>(nullptr);
        }

        MavlinkFTP::Request openResponse{};
        openResponse.hdr.opcode = MAV_FTP_OPCODE_ACK;
        openResponse.hdr.req_opcode = MAV_FTP_OPCODE_OPENFILERO;
        openResponse.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
        openResponse.hdr.session = 1;
        MavlinkFTP::setOpenFileLength(openResponse, 64U);
        ftpManager->_openFileROAckOrNak(&openResponse);
        return ftpManager->_downloadOperation->file.isOpen() ? job : nullptr;
    };

    FTPDownloadJob* job = beginManualDownload(QStringLiteral("empty-burst.bin"), true);
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);
    MavlinkFTP::Request emptyBurst{};
    emptyBurst.hdr.opcode = MAV_FTP_OPCODE_ACK;
    emptyBurst.hdr.req_opcode = MAV_FTP_OPCODE_BURSTREADFILE;
    emptyBurst.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    emptyBurst.hdr.session = 1;
    emptyBurst.hdr.burstComplete = 1;
    ftpManager->_burstReadFileAckOrNak(&emptyBurst);
    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    QVERIFY(completionSpy.takeFirst()[1].toString().contains(QStringLiteral("empty data block")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    job = beginManualDownload(QStringLiteral("empty-missing-block.bin"), true);
    QVERIFY(job);
    QSignalSpy missingBlockSpy(job, &FTPDownloadJob::finished);
    FTPManager::DownloadOperation::MissingData missingData;
    missingData.offset = 0;
    missingData.bytesMissing = 1;
    ftpManager->_downloadOperation->missingData.append(missingData);
    MavlinkFTP::Request emptyMissingBlock{};
    emptyMissingBlock.hdr.opcode = MAV_FTP_OPCODE_ACK;
    emptyMissingBlock.hdr.req_opcode = MAV_FTP_OPCODE_READFILE;
    emptyMissingBlock.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    emptyMissingBlock.hdr.session = 1;
    ftpManager->_fillMissingBlocksAckOrNak(&emptyMissingBlock);
    QVERIFY_SIGNAL_WAIT(missingBlockSpy, TestTimeout::longMs());
    QVERIFY(missingBlockSpy.takeFirst()[1].toString().contains(QStringLiteral("empty data block")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    job = beginManualDownload(QStringLiteral("offset-overflow.bin"), false);
    QVERIFY(job);
    QSignalSpy overflowSpy(job, &FTPDownloadJob::finished);
    ftpManager->_downloadOperation->expectedOffset = (std::numeric_limits<uint32_t>::max)();
    MavlinkFTP::Request overflowingData{};
    overflowingData.hdr.opcode = MAV_FTP_OPCODE_ACK;
    overflowingData.hdr.req_opcode = MAV_FTP_OPCODE_BURSTREADFILE;
    overflowingData.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    overflowingData.hdr.session = 1;
    overflowingData.hdr.offset = (std::numeric_limits<uint32_t>::max)();
    overflowingData.hdr.size = 1;
    ftpManager->_burstReadFileAckOrNak(&overflowingData);
    QVERIFY_SIGNAL_WAIT(overflowSpy, TestTimeout::longMs());
    QVERIFY(overflowSpy.takeFirst()[1].toString().contains(QStringLiteral("FTP offset limit")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadLocalWriteFailure()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    const QString localFileName = QStringLiteral("write-failure.bin");
    FTPDownloadJob* const job =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName).job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);

    MavlinkFTP::Request openResponse{};
    openResponse.hdr.opcode = MAV_FTP_OPCODE_ACK;
    openResponse.hdr.req_opcode = MAV_FTP_OPCODE_OPENFILERO;
    openResponse.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    openResponse.hdr.session = 1;
    MavlinkFTP::setOpenFileLength(openResponse, 64U);
    ftpManager->_openFileROAckOrNak(&openResponse);
    const QString partialFilePath = ftpManager->_downloadOperation->file.fileName();
    ftpManager->_downloadOperation->file.close();
    QVERIFY(ftpManager->_downloadOperation->file.open(QIODevice::ReadOnly));

    MavlinkFTP::Request data{};
    data.hdr.opcode = MAV_FTP_OPCODE_ACK;
    data.hdr.req_opcode = MAV_FTP_OPCODE_BURSTREADFILE;
    data.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;
    data.hdr.session = 1;
    data.hdr.size = 1;
    data.data[0] = 0x5A;
    expectLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("QIODevice::write .*ReadOnly device")));
    ftpManager->_burstReadFileAckOrNak(&data);
    verifyExpectedLogMessage();

    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    const QString errorMessage = completionSpy.takeFirst()[1].toString();
    QVERIFY(errorMessage.contains(QStringLiteral("Unable to write local file")));
    QVERIFY(errorMessage.contains(partialFilePath));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
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

    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    FTPDownloadJob* job =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QStringLiteral("blocked.bin"))
            .job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);
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

    job = ftpManager
              ->startDownload(MAV_COMP_ID_AUTOPILOT1, filename, downloadDir.path(), QFileInfo(protectedPath).fileName(),
                              true, FTPManager::ExistingFilePolicy::FailIfExists)
              .job();
    QVERIFY(job);
    QSignalSpy protectedCompletionSpy(job, &FTPDownloadJob::finished);
    QVERIFY_SIGNAL_WAIT(protectedCompletionSpy, TestTimeout::longMs());

    const QList<QVariant> protectedArguments = protectedCompletionSpy.takeFirst();
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
    mockFtp->setResetCommandResponseDropCount(1);

    const QString remoteFile = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(64);
    const QString localFileName = QStringLiteral("reset-warning.bin");
    FTPDownloadJob* const job =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, remoteFile, downloadDir.path(), localFileName).job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPDownloadJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("negative entry limit")));
    const FTPManager::ListDirectoryStartResult invalidLimitResult =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), -1);
    QVERIFY(!invalidLimitResult.job());
    QCOMPARE(invalidLimitResult.error(), FTPManager::StartError::InvalidArgument);
    verifyExpectedLogMessage();

    FTPListDirectoryJob* const job =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), 3).job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPListDirectoryJob::finished);
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

    FTPListDirectoryJob* job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPListDirectoryJob::finished);
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

    job->cancel();
    QCOMPARE(completionSpy.size(), 1);
    completionSpy.clear();

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy malformedSpy(job, &FTPListDirectoryJob::finished);
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

    QCOMPARE(malformedSpy.size(), 1);
    const QList<QVariant> malformedArguments = malformedSpy.takeFirst();
    QVERIFY(malformedArguments[0].toStringList().isEmpty());
    QVERIFY(malformedArguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!malformedArguments[2].toBool());

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy emptySpy(job, &FTPListDirectoryJob::finished);
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_ACK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid directory-list response size")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(emptySpy.size(), 1);
    const QList<QVariant> emptyArguments = emptySpy.takeFirst();
    QVERIFY(emptyArguments[0].toStringList().isEmpty());
    QVERIFY(emptyArguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!emptyArguments[2].toBool());

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy utf8Spy(job, &FTPListDirectoryJob::finished);
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

    QCOMPARE(utf8Spy.size(), 1);
    const QList<QVariant> utf8Arguments = utf8Spy.takeFirst();
    QVERIFY(utf8Arguments[0].toStringList().isEmpty());
    QVERIFY(utf8Arguments[1].toString().contains(QStringLiteral("malformed response")));
    QVERIFY(!utf8Arguments[2].toBool());

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy opcodeSpy(job, &FTPListDirectoryJob::finished);
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_NONE;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("received invalid response opcode")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(opcodeSpy.size(), 1);
    const QList<QVariant> opcodeArguments = opcodeSpy.takeFirst();
    QVERIFY(opcodeArguments[0].toStringList().isEmpty());
    QVERIFY(opcodeArguments[1].toString().contains(QStringLiteral("malformed response")));

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy nakSpy(job, &FTPListDirectoryJob::finished);
    response = {};
    response.hdr.opcode = MAV_FTP_OPCODE_NAK;
    response.hdr.req_opcode = ftpManager->_listDirectoryOperation->opCode;
    response.hdr.seqNumber = ftpManager->_expectedIncomingSeqNumber;

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("received a NAK without an error code")));
    ftpManager->_listDirectoryAckOrNak(&response);
    verifyExpectedLogMessage();

    QCOMPARE(nakSpy.size(), 1);
    const QList<QVariant> nakArguments = nakSpy.takeFirst();
    QVERIFY(nakArguments[0].toStringList().isEmpty());
    QVERIFY(nakArguments[1].toString().contains(QStringLiteral("malformed response")));

    job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(job);
    QSignalSpy cancelSpy(job, &FTPListDirectoryJob::finished);
    job->cancel();
    QCOMPARE(cancelSpy.size(), 1);
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
    const FTPManager::DownloadStartResult invalidDownloadResult =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, invalidUri, destination.path());
    QVERIFY(!invalidDownloadResult.job());
    QCOMPARE(invalidDownloadResult.error(), FTPManager::StartError::InvalidUri);
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_downloadOperation->inProgress());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("require a maximum file size")));
    const FTPManager::DownloadStartResult invalidArgumentResult = ftpManager->startDownload(
        MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/dynamic-file"), destination.path(), QString(), false);
    QVERIFY(!invalidArgumentResult.job());
    QCOMPARE(invalidArgumentResult.error(), FTPManager::StartError::InvalidArgument);
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_downloadOperation->inProgress());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    const FTPManager::ListDirectoryStartResult invalidListResult =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, invalidUri);
    QVERIFY(!invalidListResult.job());
    QCOMPARE(invalidListResult.error(), FTPManager::StartError::InvalidUri);
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_listDirectoryOperation->inProgress());

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    const FTPManager::DeleteStartResult invalidDeleteResult =
        ftpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, invalidUri);
    QVERIFY(!invalidDeleteResult.job());
    QCOMPARE(invalidDeleteResult.error(), FTPManager::StartError::InvalidUri);
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_deleteOperation->inProgress());

    QTemporaryFile source;
    QVERIFY(source.open());
    QCOMPARE(source.write("ftp operation", 13), 13);
    source.close();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    const FTPManager::UploadStartResult invalidUploadResult =
        ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, invalidUri, source.fileName());
    QVERIFY(!invalidUploadResult.job());
    QCOMPARE(invalidUploadResult.error(), FTPManager::StartError::InvalidUri);
    verifyExpectedLogMessage();
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_uploadOperation->inProgress());

    const QString embeddedNullUri = QStringLiteral("mftp:///file") + QChar::Null + QStringLiteral("hidden");
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to parse MAVLink FTP URI")));
    const FTPManager::DeleteStartResult embeddedNullResult =
        ftpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, embeddedNullUri);
    QVERIFY(!embeddedNullResult.job());
    QCOMPARE(embeddedNullResult.error(), FTPManager::StartError::InvalidUri);
    verifyExpectedLogMessage();
    QVERIFY(!ftpManager->_deleteOperation->inProgress());

    const QString missingSource = destination.filePath(QStringLiteral("missing.bin"));
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("Source file missing")));
    const FTPManager::UploadStartResult missingSourceResult =
        ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/upload.bin"), missingSource);
    QVERIFY(!missingSourceResult.job());
    QCOMPARE(missingSourceResult.error(), FTPManager::StartError::SourceMissing);
    verifyExpectedLogMessage();

    const QString oversizedRemotePath(MavlinkFTP::dataCapacity + 1, QLatin1Char('x'));

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("remote path is too long")));
    const FTPManager::DownloadStartResult oversizedDownloadResult =
        ftpManager->startDownload(MAV_COMP_ID_AUTOPILOT1, oversizedRemotePath, destination.path());
    QVERIFY(!oversizedDownloadResult.job());
    QCOMPARE(oversizedDownloadResult.error(), FTPManager::StartError::RemotePathTooLong);
    verifyExpectedLogMessage();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("remote path is too long")));
    const FTPManager::UploadStartResult oversizedUploadResult =
        ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, oversizedRemotePath, source.fileName());
    QVERIFY(!oversizedUploadResult.job());
    QCOMPARE(oversizedUploadResult.error(), FTPManager::StartError::RemotePathTooLong);
    verifyExpectedLogMessage();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("remote path is too long")));
    const FTPManager::ListDirectoryStartResult oversizedListResult =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, oversizedRemotePath);
    QVERIFY(!oversizedListResult.job());
    QCOMPARE(oversizedListResult.error(), FTPManager::StartError::RemotePathTooLong);
    verifyExpectedLogMessage();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(QStringLiteral("remote path is too long")));
    const FTPManager::DeleteStartResult oversizedDeleteResult =
        ftpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, oversizedRemotePath);
    QVERIFY(!oversizedDeleteResult.job());
    QCOMPARE(oversizedDeleteResult.error(), FTPManager::StartError::RemotePathTooLong);
    verifyExpectedLogMessage();

    QVERIFY(ftpManager->_rgStateMachine.isEmpty());
    QVERIFY(!ftpManager->_activeJob);

    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    const FTPManager::ListDirectoryStartResult activeListResult =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"));
    QVERIFY(activeListResult.job());
    QCOMPARE(activeListResult.error(), FTPManager::StartError::None);

    const FTPManager::DeleteStartResult busyResult =
        ftpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/busy.bin"));
    QVERIFY(!busyResult.job());
    QCOMPARE(busyResult.error(), FTPManager::StartError::Busy);

    activeListResult.job()->cancel();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);

    _disconnectMockLink();
}

void FTPManagerTest::_testScopedListDirectoryJob()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();

    const FTPManager::ListDirectoryStartResult completedResult =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"));
    FTPListDirectoryJob* const completedJob = completedResult.job();
    QVERIFY(completedJob);
    QCOMPARE(completedJob->type(), FTPJob::Type::ListDirectory);
    QVERIFY(completedJob->active());

    QSignalSpy jobCompletionSpy(completedJob, &FTPListDirectoryJob::finished);
    QVERIFY_SIGNAL_WAIT(jobCompletionSpy, TestTimeout::longMs());
    QCOMPARE(jobCompletionSpy.size(), 1);
    const QList<QVariant> completedArguments = jobCompletionSpy.takeFirst();
    QCOMPARE(completedArguments[0].toStringList().size(), 6);
    QVERIFY(completedArguments[1].toString().isEmpty());
    QVERIFY(!completedArguments[2].toBool());
    QVERIFY(!completedJob->active());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(completedResult);
    QVERIFY(completedResult.succeeded());
    QCOMPARE(completedResult.error(), FTPManager::StartError::None);
    QVERIFY(!completedResult.job());

    FTPListDirectoryJob* const selfDeletingJob =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(selfDeletingJob);
    QPointer<FTPListDirectoryJob> selfDeletingGuard(selfDeletingJob);
    (void) connect(selfDeletingJob, &FTPListDirectoryJob::finished, selfDeletingJob,
                   [selfDeletingJob]() { delete selfDeletingJob; });
    QTRY_VERIFY_WITH_TIMEOUT(!selfDeletingGuard, TestTimeout::longMs());
    QVERIFY(!ftpManager->_activeJob);

    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    FTPListDirectoryJob* const canceledJob =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(canceledJob);
    QSignalSpy canceledSpy(canceledJob, &FTPListDirectoryJob::finished);

    canceledJob->cancel();

    QCOMPARE(canceledSpy.size(), 1);
    const QList<QVariant> canceledArguments = canceledSpy.takeFirst();
    QVERIFY(canceledArguments[0].toStringList().isEmpty());
    QCOMPARE(canceledArguments[1].toString(), QStringLiteral("Aborted"));
    QVERIFY(!canceledJob->active());

    FTPListDirectoryJob* const abandonedJob =
        ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/")).job();
    QVERIFY(abandonedJob);
    QPointer<FTPListDirectoryJob> abandonedGuard(abandonedJob);
    delete abandonedJob;
    QVERIFY(!abandonedGuard);
    QVERIFY(!ftpManager->_activeJob);
    QVERIFY(ftpManager->_rgStateMachine.isEmpty());

    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNone);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryWithTime()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
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
    FTPListDirectoryJob* const job = ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, "/").job();
    QVERIFY(job);
    QSignalSpy spyListDirectoryComplete(job, &FTPListDirectoryJob::finished);
    job->cancel();
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
    FTPUploadJob* const job = ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName()).job();
    QVERIFY(job);
    QSignalSpy spyUploadComplete(job, &FTPUploadJob::finished);
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

void FTPManagerTest::_testUploadCancelFromProgress()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->clearUploadedFiles();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    const QString remotePath(QStringLiteral("/mock/upload/cancel-from-progress.bin"));
    const int chunkSize = sizeof(((MavlinkFTP::Request*) nullptr)->data);
    const QByteArray payload(chunkSize * 4, 'x');
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QCOMPARE(tempFile.write(payload), static_cast<qint64>(payload.size()));
    tempFile.close();

    FTPUploadJob* const job = ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName()).job();
    QVERIFY(job);
    QSignalSpy completionSpy(job, &FTPUploadJob::finished);
    bool canceledFromProgress = false;
    connect(
        job, &FTPJob::progress, this,
        [job, &canceledFromProgress](float) {
            if (!canceledFromProgress) {
                canceledFromProgress = true;
                job->cancel();
            }
        },
        Qt::DirectConnection);

    QVERIFY_SIGNAL_WAIT(completionSpy, TestTimeout::longMs());
    QVERIFY(canceledFromProgress);
    QCOMPARE(completionSpy.size(), 1);
    const QList<QVariant> arguments = completionSpy.takeFirst();
    QCOMPARE(arguments[0].toString(), remotePath);
    QVERIFY(arguments[1].toString().contains(QStringLiteral("Aborted")));
    QVERIFY(!ftpManager->_activeJob);
    mockFtp->clearUploadedFiles();
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

        FTPUploadJob* const job =
            ftpManager->startUpload(MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName()).job();
        if (!job) {
            return false;
        }
        QSignalSpy completionSpy(job, &FTPUploadJob::finished);
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
