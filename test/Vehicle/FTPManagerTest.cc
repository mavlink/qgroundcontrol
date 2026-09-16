#include "FTPManagerTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "FTPManager.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
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

QString FTPManagerTest::_downloadSizeFile(int fileSize, QString* downloadedPath)
{
    FTPManager* ftpManager = _vehicle->ftpManager();
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QSignalSpy spyDownloadComplete(ftpManager, &FTPManager::downloadComplete);
    ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                         QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    if (!UnitTest::waitForSignal(spyDownloadComplete, TestTimeout::longMs(), QStringLiteral("downloadComplete"))) {
        return QStringLiteral("downloadComplete never emitted");
    }
    const QList<QVariant> arguments = spyDownloadComplete.takeFirst();
    if (downloadedPath) {
        *downloadedPath = arguments[0].toString();
    }
    return arguments[1].toString();
}

// A download that dies mid-burst must release the vehicle-side session, otherwise a single-session server
// (PX4) NAKs every later OpenFileRO with "No Sessions Available".
void FTPManagerTest::_testFailedDownloadResetsSession()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    QSignalSpy spyReset(mockFtp, &MockLinkFTP::resetCommandReceived);

    mockFtp->setErrorMode(MockLinkFTP::errModeNoSecondResponse);
    QCOMPARE(_downloadSizeFile(3 * 1024), QStringLiteral("Download failed"));
    // One reset from the leading clear-stale step, one from the failure path
    QTRY_COMPARE_WITH_TIMEOUT(spyReset.count(), 2, TestTimeout::mediumMs());

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
}

// A rejected URI must leave no state machine installed, otherwise every later operation is refused as
// "Already in another operation".
void FTPManagerTest::_testInvalidUriDoesNotBlockNextOperation()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("Incorrect uri scheme or format"));
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("_parseURI failed"));
    QVERIFY(!ftpManager->download(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("http://bogus/file"),
                                  QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
}

// A session left open by someone else (crashed GCS, other GCS) must not block our download.
void FTPManagerTest::_testDownloadClearsStaleSession()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    mockFtp->openStaleSessionForTest();

    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
}

// The leading ResetSessions is best effort: losing its ack must not fail the operation.
void FTPManagerTest::_testDownloadProceedsWhenResetUnanswered()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setIgnoreResetSessions(true);

    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
}

// PX4 expires an FTP session after a fixed time without a request, even while it is streaming a burst to us.
// The next burst request then NAKs InvalidSession; the download must re-open the file and resume, not fail.
void FTPManagerTest::_testDownloadResumesAfterSessionExpiredInBurst()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setExpireSessionAfterBursts(1);

    const int fileSize = 4 * 1024;
    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(fileSize, &downloadedPath), QString());
    QCOMPARE(mockFtp->openFileROCount(), 2);
    _verifyFileContentsAndDelete(downloadedPath, fileSize);
    _disconnectMockLink();
}

// Same expiry, but hitting the hole-filling ReadFile phase instead of a burst request.
void FTPManagerTest::_testDownloadResumesAfterSessionExpiredInFill()
{
    _connectMockLinkNoInitialConnectSequence(MockConfiguration::OptionNoRadioStatus);
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    // File fits in one burst (mock bursts are 9 chunks); dropping one packet forces a fill phase after EOF
    const int fileSize = FTPManager::kFullReadChunkSize * 9;
    mockFtp->setDropBurstPacketOnce(FTPManager::kFullReadChunkSize * 3);
    mockFtp->setExpireSessionAfterBursts(1);

    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(fileSize, &downloadedPath), QString());
    QCOMPARE(mockFtp->openFileROCount(), 2);
    _verifyFileContentsAndDelete(downloadedPath, fileSize);
    _disconnectMockLink();
}

// Abandoning a download must not wait on the vehicle. Cancels happen on saturated links, where the Terminate
// ack is exactly the packet most likely to be lost; the session is released with a fire-and-forget reset instead.
void FTPManagerTest::_testCancelDownloadIsImmediate()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setBurstReadDelayMs(100);
    QSignalSpy spyReset(mockFtp, &MockLinkFTP::resetCommandReceived);

    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyProgress(ftpManager, &FTPManager::commandProgress);
    QSignalSpy spyComplete(ftpManager, &FTPManager::downloadComplete);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(100 * 1024);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("commandProgress")));
    const int resetsBeforeCancel = spyReset.count();

    ftpManager->cancelDownload();
    QCOMPARE(spyComplete.count(), 1);
    QCOMPARE(spyComplete.takeFirst()[1].toString(), QStringLiteral("Aborted"));
    QTRY_VERIFY_WITH_TIMEOUT(spyReset.count() > resetsBeforeCancel, TestTimeout::mediumMs());

    mockFtp->setBurstReadDelayMs(0);
    _disconnectMockLink();
}

// A cancel during the leading ResetSessions / OpenFileRO phases, before any file size is known, must still abort
// rather than be ignored and let the download run to completion.
void FTPManagerTest::_testCancelDownloadBeforeOpen()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    // Park the state machine in the clear-stale step
    mockFtp->setIgnoreResetSessions(true);

    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyComplete(ftpManager, &FTPManager::downloadComplete);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(3 * 1024);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));

    ftpManager->cancelDownload();
    QCOMPARE(spyComplete.count(), 1);
    QCOMPARE(spyComplete.takeFirst()[1].toString(), QStringLiteral("Aborted"));
    // Nothing should proceed past the cancel
    QVERIFY_NO_SIGNAL_WAIT(spyComplete, TestTimeout::shortMs());
    QCOMPARE(mockFtp->openFileROCount(), 0);

    mockFtp->setIgnoreResetSessions(false);
    _disconnectMockLink();
}

// Burst data is identified by offset, not sequence number. A packet arriving after later ones (a retry burst
// interleaved with the tail of the previous one, or plain reordering) must be kept, not discarded and then
// re-fetched one block at a time in the fill phase.
void FTPManagerTest::_testLateBurstPacketIsKept()
{
    _connectMockLinkNoInitialConnectSequence(MockConfiguration::OptionNoRadioStatus);
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setReorderBurstPacketOnce(FTPManager::kFullReadChunkSize * 3);

    const int fileSize = 4 * 1024;
    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(fileSize, &downloadedPath), QString());
    QCOMPARE(mockFtp->readFileCount(), 0);
    _verifyFileContentsAndDelete(downloadedPath, fileSize);
    _disconnectMockLink();
}

void FTPManagerTest::_testReadChunkSizeFollowsLinkType_data()
{
    QTest::addColumn<MockConfiguration::Options>("options");
    QTest::addColumn<int>("expectedChunkSize");
    QTest::newRow("non_radio_link") << MockConfiguration::Options(MockConfiguration::OptionNoRadioStatus)
                                     << static_cast<int>(FTPManager::kFullReadChunkSize);
    QTest::newRow("radio_link") << MockConfiguration::Options(MockConfiguration::OptionNone)
                                 << static_cast<int>(FTPManager::kRadioReadChunkSize);
}

// SiK/RFD radios have a ~252 byte air frame; a full 239 byte FTP payload spans two frames and is lost if either
// is. Once RADIO_STATUS (which only radios inject) has been seen on the link, reads must ask for smaller chunks.
// Dropping one burst packet forces the ReadFile fill phase so both request paths are checked.
void FTPManagerTest::_testReadChunkSizeFollowsLinkType()
{
    QFETCH(MockConfiguration::Options, options);
    QFETCH(int, expectedChunkSize);

    _connectMockLinkNoInitialConnectSequence(options);
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    const bool radioExpected = !options.testFlag(MockConfiguration::OptionNoRadioStatus);
    if (radioExpected) {
        QTRY_VERIFY_WITH_TIMEOUT(_mockLink->isRadioLink(), TestTimeout::mediumMs());
    } else {
        QVERIFY(!_mockLink->isRadioLink());
    }
    mockFtp->setDropBurstPacketOnce(expectedChunkSize * 3);

    const int fileSize = 3 * 1024;
    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(fileSize, &downloadedPath), QString());
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), expectedChunkSize);
    QCOMPARE(mockFtp->readFileCount(), 1);
    QCOMPARE(mockFtp->lastReadFileRequestSize(), expectedChunkSize);
    _verifyFileContentsAndDelete(downloadedPath, fileSize);
    _disconnectMockLink();
}

// RADIO_STATUS may first arrive after a download has started (cold connect races the radio's 1Hz report). The
// chunk size is re-evaluated per burst request, so the download must switch to small chunks without restarting.
void FTPManagerTest::_testReadChunkSizeShrinksWhenRadioDetectedMidDownload()
{
    _connectMockLinkNoInitialConnectSequence(MockConfiguration::OptionNoRadioStatus);
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setBurstReadDelayMs(20);

    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyProgress(ftpManager, &FTPManager::commandProgress);
    QSignalSpy spyComplete(ftpManager, &FTPManager::downloadComplete);
    const int fileSize = 64 * 1024;
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QVERIFY(ftpManager->download(MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("commandProgress")));
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), static_cast<int>(FTPManager::kFullReadChunkSize));

    mavlink_message_t msg{};
    (void) mavlink_msg_radio_status_pack(_mockLink->vehicleId(), MAV_COMP_ID_TELEMETRY_RADIO, &msg, 100, 100, 50, 10, 10, 0, 0);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int len = mavlink_msg_to_send_buffer(buffer, &msg);
    MAVLinkProtocol::instance()->receiveBytes(_mockLink, QByteArray(reinterpret_cast<const char*>(buffer), len));
    QVERIFY(_mockLink->isRadioLink());

    QVERIFY(UnitTest::waitForSignal(spyComplete, TestTimeout::longMs(), QStringLiteral("downloadComplete")));
    const QList<QVariant> arguments = spyComplete.takeFirst();
    QCOMPARE(arguments[1].toString(), QString());
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), static_cast<int>(FTPManager::kRadioReadChunkSize));
    _verifyFileContentsAndDelete(arguments[0].toString(), fileSize);

    mockFtp->setBurstReadDelayMs(0);
    _disconnectMockLink();
}

void FTPManagerTest::_verifyFileContentsAndDelete(const QString& filename, int expectedSize)
{
    QFile file(filename);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QByteArray contents = file.readAll();
    file.close();
    QCOMPARE(contents.size(), expectedSize);
    for (int i = 0; i < expectedSize; i++) {
        if (contents[i] != static_cast<char>(i % 255)) {
            QFAIL(qPrintable(QStringLiteral("Byte %1 mismatch: got %2 expected %3")
                                 .arg(i)
                                 .arg(static_cast<int>(contents[i]))
                                 .arg(i % 255)));
        }
    }
    QVERIFY(QFile::remove(filename));
}

void FTPManagerTest::_testListDirectory()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
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

void FTPManagerTest::_testListDirectoryWithTime()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QSignalSpy spyListDirectoryComplete(ftpManager, &FTPManager::listDirectoryComplete);
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
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
    ftpManager->listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY_SIGNAL_WAIT(spyListDirectoryComplete, TestTimeout::longMs());
    QCOMPARE(spyListDirectoryComplete.count(), 1);
    QList<QVariant> arguments = spyListDirectoryComplete.takeFirst();
    const QStringList entries = arguments[0].toStringList();
    QCOMPARE(entries.count(), 6);
    QVERIFY(arguments[1].toString().isEmpty());

    // After falling back to kCmdListDirectory the entries carry no modification-time field.
    for (const QString &entry : entries) {
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
    FTPManager* ftpManager = _vehicle->ftpManager();
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
    FTPManager* ftpManager = _vehicle->ftpManager();
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
    FTPManager* ftpManager = _vehicle->ftpManager();
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
