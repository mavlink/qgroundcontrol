#include "FTPManagerTest.h"

#include <cstring>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "CompInfoParam.h"
#include "ComponentInformationManager.h"
#include "FTPManager.h"
#include "FileFixtures.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSigningKeys.h"
#include "MockLink.h"
#include "MockLinkFTP.h"
#include "MultiVehicleManager.h"
#include "RequestMetaDataTypeStateMachine.h"
#include "SettingsManager.h"
#include "UnitTest.h"
#include "Vehicle.h"
#include "VehicleCameraControl.h"
#include "VehicleSigningController.h"
const FTPManagerTest::TestCase_t FTPManagerTest::_rgTestCases[] = {
    {"/general.json"},
};

FTPManager::FileCallback FTPManagerTest::_record(Result& result, QStringList* order, const QString& name)
{
    return [&result, order, name](const QString& path, const QString& errorMsg) {
        result.calls++;
        result.path = path;
        result.error = errorMsg;
        if (order) {
            order->append(name);
        }
    };
}

FTPManager::ListCallback FTPManagerTest::_recordList(Result& result)
{
    return [&result](const QStringList& entries, const QString& errorMsg) {
        result.calls++;
        result.entries = entries;
        result.error = errorMsg;
    };
}

void FTPManagerTest::_testDownloadExplicitFileNameIsBasename()
{
    // An explicit fileName can come from the vehicle: a ListDirectory entry is surfaced to
    // QML and passed straight back as the local name. It must not be usable as a path.
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, _rgTestCases[0].file, tempDir.path(),
                                 {.fileName = QStringLiteral("../../QGC03_escaped.bin")}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));

    const QString downloaded = QDir::cleanPath(QFileInfo(result.path).absoluteFilePath());
    const QString root = QDir::cleanPath(QDir(tempDir.path()).absolutePath());
    QVERIFY2(downloaded.startsWith(root + QLatin1Char('/')), qPrintable(downloaded));
    QCOMPARE(QFileInfo(downloaded).fileName(), QStringLiteral("QGC03_escaped.bin"));
    QVERIFY(!QFile::exists(QDir(root).filePath(QStringLiteral("../../QGC03_escaped.bin"))));

    QFile::remove(downloaded);
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadRejectsMissingDestination()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString missingDir = QDir(tempDir.path()).filePath(QStringLiteral("does-not-exist"));

    const QList<QPair<QString, QString>> refusedDestinations = {
        {QString(), QStringLiteral("Destination directory is not set")},
        {missingDir, QStringLiteral("Destination directory does not exist")},
    };
    QObject owner;
    Result result;
    for (const auto& [toDir, expectedWarning] : refusedDestinations) {
        expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(expectedWarning));
        QVERIFY(
            !ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, _rgTestCases[0].file, toDir, {}, _record(result)));
        verifyExpectedLogMessage();
    }
    QCOMPARE(ftpManager->requestCount(), 0);

    // A refused download must not leave FTPManager busy.
    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
}

void FTPManagerTest::cleanup()
{
    MAVLinkSigningKeys::instance()->removeAllKeys();
    VehicleTestManualConnect::cleanup();
}

void FTPManagerTest::_testCaseWorker(const TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, testCase.file,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    _disconnectMockLink();
}

void FTPManagerTest::_testDownloadUri_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<int>("fromCompId");
    QTest::addColumn<bool>("absolutePath");
    QTest::addColumn<int>("responseDelayMs");

    QTest::newRow("plain-absolute") << "/general.json" << int(MAV_COMP_ID_AUTOPILOT1) << true << 0;
    QTest::newRow("plain-relative") << "mocklink-size-239" << int(MAV_COMP_ID_AUTOPILOT1) << false << 0;
    QTest::newRow("scheme-absolute") << "mftp:///general.json" << int(MAV_COMP_ID_AUTOPILOT1) << true << 0;
    QTest::newRow("scheme-relative") << "mftp://mocklink-size-239" << int(MAV_COMP_ID_AUTOPILOT1) << false << 0;
    QTest::newRow("component-absolute") << "mftp://[;comp=1]/general.json" << int(MAV_COMP_ID_CAMERA) << true << 0;
    QTest::newRow("component-relative") << "mftp://[;comp=1]mocklink-size-239" << int(MAV_COMP_ID_CAMERA) << false << 0;
    QTest::newRow("uppercase-scheme") << "MFTP://[;comp=1]/general.json" << int(MAV_COMP_ID_CAMERA) << true << 0;
    QTest::newRow("default-component") << "mftp:///general.json" << int(MAV_COMP_ID_ALL) << true << 0;
    // Allow for queued FTP responses being delayed during UI startup.
    QTest::newRow("delayed-metadata") << "mftp://[;comp=1]/general.json" << int(MAV_COMP_ID_CAMERA) << true << 50;
}

void FTPManagerTest::_testDownloadUri()
{
    QFETCH(QString, uri);
    QFETCH(int, fromCompId);
    QFETCH(bool, absolutePath);
    QFETCH(int, responseDelayMs);

    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);
    _mockLink->mockLinkFTP()->setResponseDelayMs(responseDelayMs);
    FTPManager* ftpManager = _vehicle->ftpManager();
    QTemporaryDir destination;
    QVERIFY(destination.isValid());
    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, fromCompId, uri, destination.path(), {}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));

    if (absolutePath) {
        QFile expectedFile(QStringLiteral(":MockLink/General.MetaData.json"));
        QVERIFY(expectedFile.open(QIODevice::ReadOnly));
        QFile downloadedFile(result.path);
        QVERIFY(downloadedFile.open(QIODevice::ReadOnly));
        QCOMPARE(downloadedFile.readAll(), expectedFile.readAll());
    } else {
        _verifyFileContentsAndDelete(result.path, 239);
    }
    _disconnectMockLink();
}

void FTPManagerTest::_sizeTestCaseWorker(int fileSize)
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY(result.error.isEmpty());
    _verifyFileSizeAndDelete(result.path, fileSize);
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
    _mockLink->mockLinkFTP()->enableRandomDrops(true);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY(result.error.isEmpty());
    _verifyFileSizeAndDelete(result.path, fileSize);
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
    QObject owner;
    Result result;
    if (!ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                              QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result))) {
        return QStringLiteral("download refused");
    }
    if (!UnitTest::waitForCondition([&result]() { return result.calls > 0; }, TestTimeout::longMs(),
                                    QStringLiteral("download result"))) {
        return QStringLiteral("download never called back");
    }
    if (downloadedPath) {
        *downloadedPath = result.path;
    }
    return result.error;
}

void FTPManagerTest::_testDownloadSizeLimit_data()
{
    QTest::addColumn<qint64>("maxBytes");
    QTest::addColumn<int>("announcedLength");  // What the vehicle announces, or -1 for the size of the file
    QTest::addColumn<bool>("refused");

    // The file has 4 KiB
    QTest::newRow("at the limit") << qint64{4 * 1024} << -1 << false;
    QTest::newRow("larger than the limit") << qint64{4 * 1024 - 1} << -1 << true;
    // Refused before any data is read
    QTest::newRow("announced larger than the limit") << qint64{4 * 1024} << 8 * 1024 << true;
    QTest::newRow("announced within the limit, larger as it arrives") << qint64{2 * 1024} << 1024 << true;
}

// A file larger than the limit of its download, as announced or as it arrives, fails the download. No part of it is
// kept, and the session of the vehicle is free for the next download
void FTPManagerTest::_testDownloadSizeLimit()
{
    QFETCH(qint64, maxBytes);
    QFETCH(int, announcedLength);
    QFETCH(bool, refused);

    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    if (announcedLength >= 0) {
        mockFtp->setAnnouncedFileLength(static_cast<uint32_t>(announcedLength));
    }
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QTemporaryDir destination;
    QVERIFY(destination.isValid());

    constexpr int fileSize = 4 * 1024;
    QObject owner;
    Result result;
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1,
                                 QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize),
                                 destination.path(), {.maxBytes = maxBytes}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    const QString error = result.error;

    if (refused) {
        QVERIFY2(error.startsWith(QStringLiteral("Download failed: the file is larger than ")), qPrintable(error));
        QVERIFY(QDir(destination.path()).isEmpty());
    } else {
        QVERIFY2(error.isEmpty(), qPrintable(error));
        _verifyFileContentsAndDelete(result.path, fileSize);
    }

    QString downloadedPath;
    QCOMPARE(_downloadSizeFile(3 * 1024, &downloadedPath), QString());
    _verifyFileSizeAndDelete(downloadedPath, 3 * 1024);
    _disconnectMockLink();
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

// A rejected URI must queue nothing, otherwise every later operation waits behind it.
void FTPManagerTest::_testInvalidUriDoesNotBlockNextOperation()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("Incorrect uri scheme or format"));
    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("_parseURI failed"));
    QObject owner;
    Result result;
    QVERIFY(!ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("http://bogus/file"),
                                  QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result)));
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
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(100 * 1024);
    QObject owner;
    Result result;
    const QPointer<FTPRequest> request =
        ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                             QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result));
    QVERIFY(request);
    QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    const int resetsBeforeCancel = spyReset.count();

    request->cancel();
    QCOMPARE(ftpManager->requestCount(), 0);
    QVERIFY(!QFile::exists(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(filename)));
    QTRY_VERIFY_WITH_TIMEOUT(spyReset.count() > resetsBeforeCancel, TestTimeout::mediumMs());
    QVERIFY(request.isNull());
    QCOMPARE(result.calls, 0);

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
    QSignalSpy spyReset(mockFtp, &MockLinkFTP::resetCommandReceived);
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(3 * 1024);
    QObject owner;
    Result result;
    const QPointer<FTPRequest> request =
        ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                             QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result));
    QVERIFY(request);
    QVERIFY_SIGNAL_COUNT_WAIT(spyReset, 1, TestTimeout::mediumMs());

    request->cancel();
    // The clear-stale reset, then the one releasing the abandoned session
    QVERIFY_SIGNAL_COUNT_WAIT(spyReset, 2, TestTimeout::mediumMs());
    // Nothing should proceed past the cancel: a live state machine would resend the unanswered reset every
    // kTestAckTimeoutMs (100 ms)
    QVERIFY_NO_SIGNAL_WAIT(spyReset, 250);
    QCOMPARE(result.calls, 0);
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
    QTest::addColumn<bool>("signedLink");
    QTest::addColumn<int>("expectedChunkSize");
    QTest::newRow("non_radio_link") << MockConfiguration::Options(MockConfiguration::OptionNoRadioStatus) << false
                                    << static_cast<int>(FTPManager::kFullReadChunkSize);
    QTest::newRow("radio_link") << MockConfiguration::Options(MockConfiguration::OptionNone) << false
                                << static_cast<int>(FTPManager::kRadioReadChunkSize);
    QTest::newRow("signed_radio_link") << MockConfiguration::Options(MockConfiguration::OptionNone) << true
                                       << static_cast<int>(FTPManager::kRadioReadChunkSizeSigned);
}

// A SiK radio splits any packet larger than its max_data_packet_length, only 118 bytes with error correction on.
// Once RADIO_STATUS (which only radios inject) has been seen on the link, reads must ask for chunks that fit, and
// smaller still when signing adds its signature. Dropping one burst packet forces the ReadFile fill phase so both
// request paths are checked.
void FTPManagerTest::_testReadChunkSizeFollowsLinkType()
{
    QFETCH(MockConfiguration::Options, options);
    QFETCH(bool, signedLink);
    QFETCH(int, expectedChunkSize);

    _connectMockLinkNoInitialConnectSequence(options);
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    if (signedLink) {
        QVERIFY(MAVLinkSigningKeys::instance()->addKey(QStringLiteral("FtpTestKey"), QStringLiteral("FtpTestPass")));
        _vehicle->signingController()->enable(QStringLiteral("FtpTestKey"));
        QTRY_VERIFY_WITH_TIMEOUT(_vehicle->signingController()->signingStatus().state == SigningStatus::State::On,
                                 TestTimeout::mediumMs());
    }
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
    const int fileSize = 64 * 1024;
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    QObject owner;
    Result result;
    FTPRequest* const request =
        ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, filename,
                             QStandardPaths::writableLocation(QStandardPaths::TempLocation), {}, _record(result));
    QVERIFY(request);
    QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), static_cast<int>(FTPManager::kFullReadChunkSize));

    mavlink_message_t msg{};
    (void) mavlink_msg_radio_status_pack(_mockLink->vehicleId(), MAV_COMP_ID_TELEMETRY_RADIO, &msg, 100, 100, 50, 10, 10, 0, 0);
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int len = mavlink_msg_to_send_buffer(buffer, &msg);
    MAVLinkProtocol::instance()->receiveBytes(_mockLink, QByteArray(reinterpret_cast<const char*>(buffer), len));
    QVERIFY(_mockLink->isRadioLink());

    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.error, QString());
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), static_cast<int>(FTPManager::kRadioReadChunkSize));
    _verifyFileContentsAndDelete(result.path, fileSize);

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
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    TEST_DEBUG(QStringLiteral("listDirectory entries: %1").arg(result.entries.join(',')));
    QCOMPARE(result.entries.count(), 6);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryWithTime()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    const QStringList entries = result.entries;
    QCOMPARE(entries.count(), 6);
    QVERIFY(result.error.isEmpty());

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

void FTPManagerTest::_testListDirectoryWithTimeFallback_data()
{
    QTest::addColumn<int>("nakError");

    QTest::newRow("UnknownCommand (PX4)") << static_cast<int>(MavlinkFTP::kErrUnknownCommand);
    QTest::newRow("Fail (ArduPilot)") << static_cast<int>(MavlinkFTP::kErrFail);
}

void FTPManagerTest::_testListDirectoryWithTimeFallback()
{
    QFETCH(int, nakError);

    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setListDirectoryWithTimeSupported(false);
    _mockLink->mockLinkFTP()->setListDirectoryWithTimeNakError(static_cast<MavlinkFTP::ErrorCode_t>(nakError));
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    const QStringList entries = result.entries;
    QCOMPARE(entries.count(), 6);
    QVERIFY(result.error.isEmpty());
    QVERIFY(ftpManager->listDirectoryWithTimeUnsupported());

    // After falling back to kCmdListDirectory the entries carry no modification-time field.
    for (const QString &entry : entries) {
        QCOMPARE(entry.mid(1).count(QLatin1Char('\t')), 1);
    }
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryWithTimeFailAfterSupported()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    QObject owner;

    // First listing establishes kCmdListDirectoryWithTime support
    Result first;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(first)));
    QVERIFY_TRUE_WAIT(first.calls == 1, TestTimeout::longMs());
    QVERIFY(first.error.isEmpty());

    // Once support is known a kErrFail Nak is a real failure, not a reason to fall back
    _mockLink->mockLinkFTP()->setListDirectoryWithTimeSupported(false);
    _mockLink->mockLinkFTP()->setListDirectoryWithTimeNakError(MavlinkFTP::kErrFail);
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    QVERIFY(!ftpManager->listDirectoryWithTimeUnsupported());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoResponse);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNakResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoSecondResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponse);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNoSecondResponseAllowRetry()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNoSecondResponseAllowRetry);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    TEST_DEBUG(QStringLiteral("listDirectory retry entries: %1").arg(result.entries.join(',')));
    QCOMPARE(result.entries.count(), 6);
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryNakSecondResponse()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakSecondResponse);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryBadSequence()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeBadSequence);
    QObject owner;
    Result result;
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.entries.count(), 0);
    QVERIFY(!result.error.isEmpty());
    _disconnectMockLink();
}

void FTPManagerTest::_testListDirectoryCancel()
{
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    FTPManager* ftpManager = vehicle->ftpManager();
    QObject owner;
    Result result;
    const QPointer<FTPRequest> request =
        ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result));
    QVERIFY(request);
    request->cancel();
    // Gone from the queue at once, and never calls back
    QCOMPARE(ftpManager->requestCount(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(request.isNull(), TestTimeout::shortMs());
    QCOMPARE(result.calls, 0);
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
    QObject owner;
    Result result;
    QVERIFY(ftpManager->upload(&owner, MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName(), _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QCOMPARE(result.path, remotePath);
    QVERIFY(result.error.isEmpty());
    QVERIFY(_mockLink->mockLinkFTP()->uploadedFiles().contains(remotePath));
    const QByteArray uploadedPayload = _mockLink->mockLinkFTP()->uploadedFileContents(remotePath);
    QCOMPARE(uploadedPayload, payload);
    _mockLink->mockLinkFTP()->clearUploadedFiles();
    _disconnectMockLink();
}

FTPRequest* FTPManagerTest::_queueSizeFile(QObject* owner, int fileSize, Result& result, QStringList* order,
                                           const QString& name)
{
    const QString filename = QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
    FTPRequest* const request = _vehicle->ftpManager()->download(
        owner, MAV_COMP_ID_AUTOPILOT1, filename, QStandardPaths::writableLocation(QStandardPaths::TempLocation), {},
        _record(result, order, QStringLiteral("done ") + name));
    if (request && order) {
        (void) connect(request, &FTPRequest::started, this,
                       [order, name]() { order->append(QStringLiteral("start ") + name); });
    }
    return request;
}

// Requests made while another runs wait for it and run one after the other, in the order they were made. Each gets
// its own result.
void FTPManagerTest::_testQueuedRequestsRunInOrder()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setSingleSessionEnforced(true);

    QObject owner;
    QStringList order;
    Result first;
    Result second;
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, first, &order, QStringLiteral("first")));
    QVERIFY(_queueSizeFile(&owner, 2 * 1024, second, &order, QStringLiteral("second")));
    QCOMPARE(_vehicle->ftpManager()->requestCount(), 2);

    QVERIFY_TRUE_WAIT(second.calls == 1, TestTimeout::longMs());
    QCOMPARE(order, QStringList({QStringLiteral("start first"), QStringLiteral("done first"),
                                 QStringLiteral("start second"), QStringLiteral("done second")}));
    QCOMPARE(first.calls, 1);
    QCOMPARE(first.error, QString());
    QCOMPARE(second.error, QString());
    _verifyFileContentsAndDelete(first.path, 3 * 1024);
    _verifyFileContentsAndDelete(second.path, 2 * 1024);
    QCOMPARE(_vehicle->ftpManager()->requestCount(), 0);
}

// A request that fails ends alone: the one after it runs as if nothing had happened, also when it is of another
// kind.
void FTPManagerTest::_testFailedRequestDoesNotDisturbNext()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    FTPManager* const ftpManager = _vehicle->ftpManager();

    QObject owner;
    QStringList order;
    Result first;
    Result missing;
    Result listing;
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, first, &order, QStringLiteral("first")));
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/no-such-file"),
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation), {},
                                 _record(missing, &order, QStringLiteral("missing"))));
    QVERIFY(ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"), _recordList(listing)));

    QVERIFY_TRUE_WAIT(listing.calls == 1, TestTimeout::longMs());
    QCOMPARE(order,
             QStringList({QStringLiteral("start first"), QStringLiteral("done first"), QStringLiteral("missing")}));
    QCOMPARE(first.error, QString());
    _verifyFileContentsAndDelete(first.path, 3 * 1024);
    QVERIFY2(missing.error.startsWith(QStringLiteral("Download failed")), qPrintable(missing.error));
    QVERIFY(!QFile::exists(missing.path));
    QCOMPARE(listing.error, QString());
    QCOMPARE(listing.entries.count(), 6);
}

// A request cancelled while it waits leaves the queue without a trace: it never reaches the vehicle and never calls
// back, and the others run.
void FTPManagerTest::_testCancelQueuedRequest()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();

    QObject owner;
    QStringList order;
    Result first;
    Result cancelled;
    Result last;
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, first, &order, QStringLiteral("first")));
    const QPointer<FTPRequest> request =
        _queueSizeFile(&owner, 2 * 1024, cancelled, &order, QStringLiteral("cancelled"));
    QVERIFY(request);
    QVERIFY(_queueSizeFile(&owner, 1024, last, &order, QStringLiteral("last")));
    QVERIFY_TRUE_WAIT(order.contains(QStringLiteral("start first")), TestTimeout::longMs());

    request->cancel();
    QCOMPARE(_vehicle->ftpManager()->requestCount(), 2);

    QVERIFY_TRUE_WAIT(last.calls == 1, TestTimeout::longMs());
    QCOMPARE(order, QStringList({QStringLiteral("start first"), QStringLiteral("done first"),
                                 QStringLiteral("start last"), QStringLiteral("done last")}));
    QCOMPARE(cancelled.calls, 0);
    QVERIFY(request.isNull());
    QCOMPARE(mockFtp->openFileROCount(), 2);
    _verifyFileContentsAndDelete(first.path, 3 * 1024);
    _verifyFileContentsAndDelete(last.path, 1024);
}

// Cancelling the request that runs ends it on the vehicle, without waiting for it: its session is released, its part
// of the file is removed, and the next request runs.
void FTPManagerTest::_testCancelRunningRequest()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    mockFtp->setBurstReadDelayMs(100);
    QSignalSpy spyReset(mockFtp, &MockLinkFTP::resetCommandReceived);

    QObject owner;
    Result cancelled;
    Result next;
    const QPointer<FTPRequest> request = _queueSizeFile(&owner, 100 * 1024, cancelled);
    QVERIFY(request);
    QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, next));
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    const int resetsBeforeCancel = spyReset.count();

    request->cancel();
    mockFtp->setBurstReadDelayMs(0);
    QVERIFY(
        !QFile::exists(cancelled.path.isEmpty()
                           ? QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(100 * 1024))
                           : cancelled.path));
    QVERIFY_SIGNAL_COUNT_WAIT(spyReset, resetsBeforeCancel + 1, TestTimeout::mediumMs());

    QVERIFY_TRUE_WAIT(next.calls == 1, TestTimeout::longMs());
    QCOMPARE(next.error, QString());
    _verifyFileContentsAndDelete(next.path, 3 * 1024);
    QCOMPARE(cancelled.calls, 0);
    QVERIFY(request.isNull());
}

// The requests of an owner that is destroyed end with it, the one that runs as by cancel(). Those of others run.
void FTPManagerTest::_testOwnerDestroyedDropsItsRequests()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    FTPManager* const ftpManager = _vehicle->ftpManager();

    Result running;
    Result waiting;
    Result other;
    QStringList order;
    auto owner = std::make_unique<QObject>();
    QObject otherOwner;
    QVERIFY(_queueSizeFile(owner.get(), 3 * 1024, running, &order, QStringLiteral("running")));
    QVERIFY(_queueSizeFile(owner.get(), 2 * 1024, waiting, &order, QStringLiteral("waiting")));
    QVERIFY(_queueSizeFile(&otherOwner, 1024, other, &order, QStringLiteral("other")));
    QVERIFY_TRUE_WAIT(order.contains(QStringLiteral("start running")), TestTimeout::longMs());

    owner.reset();
    QCOMPARE(ftpManager->requestCount(), 1);

    QVERIFY_TRUE_WAIT(other.calls == 1, TestTimeout::longMs());
    QCOMPARE(other.error, QString());
    _verifyFileContentsAndDelete(other.path, 1024);
    QCOMPARE(order, QStringList({QStringLiteral("start running"), QStringLiteral("start other"),
                                 QStringLiteral("done other")}));
    QCOMPARE(running.calls, 0);
    QCOMPARE(waiting.calls, 0);
}

// A vehicle that goes away ends the requests that are left with an error, the one that ran and those that waited,
// from the event loop. The part of the file that was downloaded is removed.
void FTPManagerTest::_testVehicleGoneEndsRequests()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(100);

    QObject owner;
    Result running;
    Result waiting;
    FTPRequest* const request = _queueSizeFile(&owner, 100 * 1024, running);
    QVERIFY(request);
    QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, waiting));
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    QVERIFY(
        QFile::exists(running.path.isEmpty()
                          ? QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                .filePath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(100 * 1024))
                          : running.path));

    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    _disconnectMockLink();

    QVERIFY_TRUE_WAIT((running.calls == 1) && (waiting.calls == 1), TestTimeout::mediumMs());
    QCOMPARE(running.error, QStringLiteral("Vehicle disconnected"));
    QCOMPARE(waiting.error, QStringLiteral("Vehicle disconnected"));
    QVERIFY(!QFile::exists(running.path));
    QVERIFY(!QFile::exists(waiting.path));
}

// At connection, the component metadata and the definition of a camera are downloaded over MAVLink FTP at the same
// time. Both are downloaded, whichever asks first.
void FTPManagerTest::_testMetadataAndCameraDefinitionTogether()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);
    ComponentInformationManager* const compInfoManager = _vehicle->compInfoManager();
    QVERIFY(compInfoManager);
    QVERIFY_TRUE_WAIT(!compInfoManager->inProgress(), TestTimeout::mediumMs());

    // The camera definition is cached under the save path: a directory of the test's own
    QTemporaryDir saveRoot;
    QVERIFY(saveRoot.isValid());
    Fact* const savePath = SettingsManager::instance()->appSettings()->savePath();
    const QVariant originalSavePath = savePath->rawValue();
    savePath->setRawValue(saveRoot.path());
    const auto restoreSavePath =
        qScopeGuard([savePath, originalSavePath]() { savePath->setRawValue(originalSavePath); });

    QTemporaryDir servedDir;
    QVERIFY(servedDir.isValid());
    const QString definitionFile = servedDir.filePath(QStringLiteral("camera.xml.gz"));
    QVERIFY(TestFixtures::writeFile(
        definitionFile,
        TestFixtures::gzipMember(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                                 R"(<mavlinkcamera><definition version="1"><model>Cam</model><vendor>Acme</vendor>)"
                                 R"(</definition><parameters><parameter name="CAM_FTP" type="uint32" default="0">)"
                                 R"(<description>Setting</description></parameter></parameters></mavlinkcamera>)")));
    _mockLink->mockLinkFTP()->setServedFile(QStringLiteral("/camera.xml.gz"), definitionFile);

    CompInfoParam* const param = compInfoManager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);
    // Random CRC so the file cache cannot satisfy the request
    param->setUriMetaData(QStringLiteral("mftp://[;comp=1]parameter.json.xz"), QRandomGenerator::global()->generate());
    RequestMetaDataTypeStateMachine metadataRequest(compInfoManager, this);
    QSignalSpy metadataComplete(&metadataRequest, &RequestMetaDataTypeStateMachine::requestComplete);

    const QByteArray uri = QByteArrayLiteral("mftp://[;comp=1]/camera.xml.gz");
    mavlink_camera_information_t info{};
    memcpy(info.vendor_name, "Acme", 5);
    memcpy(info.model_name, "Cam", 4);
    memcpy(info.cam_definition_uri, uri.constData(), static_cast<size_t>(uri.size()));

    metadataRequest.request(param);
    VehicleCameraControl camera(&info, _vehicle, MAV_COMP_ID_CAMERA6);

    QVERIFY(UnitTest::waitForSignal(metadataComplete, TestTimeout::longMs(), QStringLiteral("requestComplete")));
    QVERIFY_TRUE_WAIT(!camera.isBasic(), TestTimeout::longMs());
    QCOMPARE(camera.activeSettings(), QStringList{QStringLiteral("CAM_FTP")});
    // A metadata download that is refused or fails is reported with a warning, which fails the test
}

UT_REGISTER_TEST(FTPManagerTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Serial)
