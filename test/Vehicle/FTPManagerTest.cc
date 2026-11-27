#include "FTPManagerTest.h"

#include <cstring>
#include <functional>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
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

void FTPManagerTest::_testDownloadFileName_data()
{
    QTest::addColumn<QString>("pathOnVehicle");
    QTest::addColumn<QString>("fileName");
    // Name of the downloaded file in the destination, empty if the download is refused
    QTest::addColumn<QString>("localName");

    QTest::newRow("backslashes on the vehicle") << "/dir\\..\\..\\escaped.json" << "" << "escaped.json";
    // An explicit fileName can come from the vehicle too: a ListDirectory entry is surfaced to QML and passed straight
    // back as the local name
    QTest::newRow("slashes in the name") << "/general.json" << "../../escaped.json" << "escaped.json";
    QTest::newRow("backslashes in the name") << "/general.json" << "..\\..\\escaped.json" << "escaped.json";
    QTest::newRow("no name on the vehicle") << "/dir/" << "" << "";
    QTest::newRow("dot dot on the vehicle") << "/dir/.." << "" << "";
    QTest::newRow("a device on the vehicle") << "/dir/COM1.json" << "" << "";
    QTest::newRow("relative to a drive in the name") << "/general.json" << "C:escaped.json" << "";
}

// The vehicle names the file in the destination, never where it goes, and the same way on every system
void FTPManagerTest::_testDownloadFileName()
{
    QFETCH(QString, pathOnVehicle);
    QFETCH(QString, fileName);
    QFETCH(QString, localName);

    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setServedFile(pathOnVehicle, QStringLiteral(":MockLink/General.MetaData.json"));
    FTPManager* ftpManager = _vehicle->ftpManager();

    QTemporaryDir root;
    QVERIFY(root.isValid());
    // Two levels down, so that a name that climbs two levels would still land in the temporary directory
    const QString destination = root.filePath(QStringLiteral("a/b"));
    QVERIFY(QDir().mkpath(destination));

    QObject owner;
    Result result;
    if (localName.isEmpty()) {
        expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("Not a file name"));
        QVERIFY(!ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, pathOnVehicle, destination,
                                      {.fileName = fileName}, _record(result)));
        verifyExpectedLogMessage();
        QCOMPARE(ftpManager->requestCount(), 0);
    } else {
        QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, pathOnVehicle, destination, {.fileName = fileName},
                                     _record(result)));
        QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.path, QDir(destination).absoluteFilePath(localName));
    }

    QStringList written;
    QDirIterator files(root.path(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        written.append(QDir(root.path()).relativeFilePath(files.next()));
    }
    QCOMPARE(written, localName.isEmpty() ? QStringList() : QStringList{QStringLiteral("a/b/") + localName});
    _disconnectMockLink();
}

// Arguments that cannot be used are refused with a warning that says why. Nothing is queued, otherwise every later
// operation would wait behind it, and nothing is called back.
void FTPManagerTest::_testRefusedRequestQueuesNothing()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString missingDir = tempDir.filePath(QStringLiteral("does-not-exist"));
    const QString missingFile = tempDir.filePath(QStringLiteral("does-not-exist.bin"));
    const QString localFile = tempDir.filePath(QStringLiteral("upload.bin"));
    QVERIFY(TestFixtures::writeFile(localFile, "data"));
    const QString sizeFile = _sizeFileName(1024);
    const QString otherScheme = QStringLiteral("http://bogus/file");
    const QStringList otherSchemeWarnings{QStringLiteral("Incorrect uri scheme or format"),
                                          QStringLiteral("_parseURI failed")};

    QObject owner;
    Result result;

    struct Refused
    {
        const char* name;
        std::function<FTPRequest*()> request;
        QStringList warnings;
    };

    const QList<Refused> refused = {
        {"download, no destination",
         [&]() {
             return ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, sizeFile, QString(), {}, _record(result));
         },
         {QStringLiteral("Destination directory is not set")}},
        {"download, missing destination",
         [&]() {
             return ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, sizeFile, missingDir, {}, _record(result));
         },
         {QStringLiteral("Destination directory does not exist")}},
        {"download, other scheme",
         [&]() {
             return ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, otherScheme, tempDir.path(), {},
                                         _record(result));
         },
         otherSchemeWarnings},
        {"upload, missing source",
         [&]() {
             return ftpManager->upload(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/upload.bin"), missingFile,
                                       _record(result));
         },
         {QStringLiteral("Cannot upload. Source file missing")}},
        {"upload, other scheme",
         [&]() { return ftpManager->upload(&owner, MAV_COMP_ID_AUTOPILOT1, otherScheme, localFile, _record(result)); },
         otherSchemeWarnings},
        {"listing, other scheme",
         [&]() { return ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, otherScheme, _recordList(result)); },
         otherSchemeWarnings},
        {"delete, other scheme",
         [&]() { return ftpManager->deleteFile(&owner, MAV_COMP_ID_AUTOPILOT1, otherScheme, _record(result)); },
         otherSchemeWarnings},
        {"listing, no context",
         [&]() {
             return ftpManager->listDirectory(nullptr, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/"),
                                              _recordList(result));
         },
         {QStringLiteral("Request without a context")}},
    };
    QStringList notRefused;
    for (const Refused& entry : refused) {
        for (const QString& warning : entry.warnings) {
            expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression(warning));
        }
        if (entry.request()) {
            notRefused.append(QString::fromLatin1(entry.name));
        }
        for (qsizetype i = 0; i < entry.warnings.size(); i++) {
            verifyExpectedLogMessage();
        }
    }
    QVERIFY2(notRefused.isEmpty(), qPrintable(notRefused.join(QStringLiteral("; "))));
    QCOMPARE(ftpManager->requestCount(), 0);
    QCOMPARE(result.calls, 0);

    // A refused request must not leave FTPManager busy.
    QCOMPARE(_downloadAndVerifySizeFile(3 * 1024), QString());
    _disconnectMockLink();
}

void FTPManagerTest::cleanup()
{
    MAVLinkSigningKeys::instance()->removeAllKeys();
    VehicleTestManualConnect::cleanup();
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

    _connectMockLinkNoInitialConnectSequence();
    QCOMPARE(_downloadAndVerifySizeFile(fileSize), QString());
    _disconnectMockLink();
}

void FTPManagerTest::_testLostPackets()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->enableRandomDrops(true);
    QCOMPARE(_downloadAndVerifySizeFile(4 * 1024), QString());
    _disconnectMockLink();
}

QString FTPManagerTest::_sizeFileName(int fileSize)
{
    return QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(fileSize);
}

QString FTPManagerTest::_sizeFilePath(int fileSize)
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(_sizeFileName(fileSize));
}

QString FTPManagerTest::_downloadSizeFile(int fileSize, QString* downloadedPath)
{
    QObject owner;
    Result result;
    if (!_queueSizeFile(&owner, fileSize, result)) {
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

QString FTPManagerTest::_downloadAndVerifySizeFile(int fileSize)
{
    QString downloadedPath;
    const QString error = _downloadSizeFile(fileSize, &downloadedPath);
    return error.isEmpty() ? _sizeFileMismatch(downloadedPath, fileSize) : error;
}

void FTPManagerTest::_testDownloadSizeLimit_data()
{
    QTest::addColumn<qint64>("maxBytes");
    QTest::addColumn<int>("announcedLength");  // What the vehicle announces, or -1 for the size of the file
    QTest::addColumn<bool>("checksize");
    QTest::addColumn<bool>("refused");

    // The file has 4 KiB
    QTest::newRow("at the limit") << qint64{4 * 1024} << -1 << true << false;
    QTest::newRow("larger than the limit") << qint64{4 * 1024 - 1} << -1 << true << true;
    // Refused before any data is read
    QTest::newRow("announced larger than the limit") << qint64{4 * 1024} << 8 * 1024 << true << true;
    QTest::newRow("announced within the limit, larger as it arrives") << qint64{2 * 1024} << 1024 << true << true;
    // Without the size check what the vehicle announces says nothing, as for the ArduPilot parameter file: only the
    // data counts
    QTest::newRow("checksize off, announced larger than the limit") << qint64{4 * 1024} << 8 * 1024 << false << false;
    QTest::newRow("checksize off, larger as it arrives") << qint64{2 * 1024} << 1024 << false << true;
}

// A file larger than the limit of its download, as announced or as it arrives, fails the download. No part of it is
// kept, and the session of the vehicle is free for the next download
void FTPManagerTest::_testDownloadSizeLimit()
{
    QFETCH(qint64, maxBytes);
    QFETCH(int, announcedLength);
    QFETCH(bool, checksize);
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
    QVERIFY(ftpManager->download(&owner, MAV_COMP_ID_AUTOPILOT1, _sizeFileName(fileSize), destination.path(),
                                 {.checksize = checksize, .maxBytes = maxBytes}, _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    const QString error = result.error;

    if (refused) {
        QVERIFY2(error.startsWith(QStringLiteral("Download failed: the file is larger than ")), qPrintable(error));
        QVERIFY(QDir(destination.path()).isEmpty());
    } else {
        QVERIFY2(error.isEmpty(), qPrintable(error));
        _verifyFileContentsAndDelete(result.path, fileSize);
    }

    QCOMPARE(_downloadAndVerifySizeFile(3 * 1024), QString());
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
    QCOMPARE(_downloadAndVerifySizeFile(3 * 1024), QString());
    _disconnectMockLink();
}

// A session left open by someone else (crashed GCS, other GCS) must not block our download.
void FTPManagerTest::_testDownloadClearsStaleSession()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    mockFtp->openStaleSessionForTest();

    QCOMPARE(_downloadAndVerifySizeFile(3 * 1024), QString());
    _disconnectMockLink();
}

// The leading ResetSessions is best effort: losing its ack must not fail the operation.
void FTPManagerTest::_testDownloadProceedsWhenResetUnanswered()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setIgnoreResetSessions(true);

    QCOMPARE(_downloadAndVerifySizeFile(3 * 1024), QString());
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
    QCOMPARE(_downloadAndVerifySizeFile(fileSize), QString());
    QCOMPARE(mockFtp->openFileROCount(), 2);
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

    QCOMPARE(_downloadAndVerifySizeFile(fileSize), QString());
    QCOMPARE(mockFtp->openFileROCount(), 2);
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
    QCOMPARE(_downloadAndVerifySizeFile(fileSize), QString());
    QCOMPARE(mockFtp->readFileCount(), 0);
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
    QCOMPARE(_downloadAndVerifySizeFile(fileSize), QString());
    QCOMPARE(mockFtp->lastBurstReadRequestSize(), expectedChunkSize);
    QCOMPARE(mockFtp->readFileCount(), 1);
    QCOMPARE(mockFtp->lastReadFileRequestSize(), expectedChunkSize);
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

QString FTPManagerTest::_sizeFileMismatch(const QString& filename, int expectedSize)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringLiteral("%1: %2").arg(filename, file.errorString());
    }
    const QByteArray contents = file.readAll();
    file.close();
    if (!QFile::remove(filename)) {
        return QStringLiteral("%1 could not be removed").arg(filename);
    }
    if (contents.size() != expectedSize) {
        return QStringLiteral("Size %1, expected %2").arg(contents.size()).arg(expectedSize);
    }
    for (int i = 0; i < expectedSize; i++) {
        if (contents[i] != static_cast<char>(i % 255)) {
            return QStringLiteral("Byte %1 mismatch: got %2 expected %3")
                .arg(i)
                .arg(static_cast<int>(contents[i]))
                .arg(i % 255);
        }
    }
    return QString();
}

void FTPManagerTest::_verifyFileContentsAndDelete(const QString& filename, int expectedSize)
{
    const QString mismatch = _sizeFileMismatch(filename, expectedSize);
    QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
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

void FTPManagerTest::_testListDirectoryFails_data()
{
    QTest::addColumn<int>("errorMode");

    QTest::newRow("NoResponse") << static_cast<int>(MockLinkFTP::errModeNoResponse);
    QTest::newRow("NakResponse") << static_cast<int>(MockLinkFTP::errModeNakResponse);
    QTest::newRow("NoSecondResponse") << static_cast<int>(MockLinkFTP::errModeNoSecondResponse);
    QTest::newRow("NakSecondResponse") << static_cast<int>(MockLinkFTP::errModeNakSecondResponse);
    QTest::newRow("BadSequence") << static_cast<int>(MockLinkFTP::errModeBadSequence);
}

void FTPManagerTest::_testListDirectoryFails()
{
    QFETCH(int, errorMode);

    _connectMockLinkNoInitialConnectSequence();
    FTPManager* ftpManager = _vehicle->ftpManager();
    _mockLink->mockLinkFTP()->setErrorMode(static_cast<MockLinkFTP::ErrorMode_t>(errorMode));
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

// A listing is cancelled under way, and before it has started. Either way it never calls back and the queue goes on:
// neither what is left of the listing nor a start that was scheduled for it may keep the next request from running.
void FTPManagerTest::_testListDirectoryCancel()
{
    _connectMockLinkNoInitialConnectSequence();
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QObject owner;
    Result result;

    const QPointer<FTPRequest> underWay =
        ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result));
    QVERIFY(underWay);
    QSignalSpy spyStarted(underWay, &FTPRequest::started);
    QVERIFY(UnitTest::waitForSignal(spyStarted, TestTimeout::mediumMs(), QStringLiteral("started")));
    underWay->cancel();
    QCOMPARE(ftpManager->requestCount(), 0);

    const QPointer<FTPRequest> notStarted =
        ftpManager->listDirectory(&owner, MAV_COMP_ID_AUTOPILOT1, "/", _recordList(result));
    QVERIFY(notStarted);
    notStarted->cancel();
    // Gone from the queue at once. Its start then finds the queue empty.
    QCOMPARE(ftpManager->requestCount(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(notStarted.isNull(), TestTimeout::shortMs());
    QVERIFY(underWay.isNull());

    QCOMPARE(_downloadAndVerifySizeFile(1024), QString());
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

void FTPManagerTest::_testUploadNakNamesPath()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setErrorMode(MockLinkFTP::errModeNakResponse);
    // A path on the vehicle may hold what looks like a placeholder of the message
    const QString remotePath(QStringLiteral("/mock/upload/a%1b%2.bin"));
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QCOMPARE(tempFile.write("data"), 4);
    tempFile.close();
    QObject owner;
    Result result;
    QVERIFY(_vehicle->ftpManager()->upload(&owner, MAV_COMP_ID_AUTOPILOT1, remotePath, tempFile.fileName(),
                                           _record(result)));
    QVERIFY_TRUE_WAIT(result.calls == 1, TestTimeout::longMs());
    QVERIFY2(result.error.startsWith(QStringLiteral("Upload failed for: %1 - error: ").arg(remotePath)),
             qPrintable(result.error));
    _disconnectMockLink();
}

// The file of an upload is opened when its turn comes. One that cannot be opened by then fails the upload alone: a
// request that could not begin and never ended would hold up every request behind it.
void FTPManagerTest::_testUploadFileGoneWhenItsTurnComes()
{
    _connectMockLinkNoInitialConnectSequence();
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString localFile = directory.filePath(QStringLiteral("upload.bin"));
    QVERIFY(TestFixtures::writeFile(localFile, "data"));
    const QString remotePath(QStringLiteral("/mock/upload/gone.bin"));

    QObject owner;
    Result upload;
    Result next;
    QVERIFY(_vehicle->ftpManager()->upload(&owner, MAV_COMP_ID_AUTOPILOT1, remotePath, localFile, _record(upload)));
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, next));
    // No request begins within the call that makes it
    QVERIFY(QFile::remove(localFile));

    expectLogMessage("Vehicle.FTPManager", QtWarningMsg, QRegularExpression("Cannot upload. Failed to open file"));
    QVERIFY_TRUE_WAIT(next.calls == 1, TestTimeout::longMs());
    verifyExpectedLogMessage();
    QCOMPARE(upload.calls, 1);
    QCOMPARE(upload.error, QStringLiteral("Upload failed for: %1 - error reading file").arg(remotePath));
    QCOMPARE(next.error, QString());
    _verifyFileContentsAndDelete(next.path, 3 * 1024);
}

FTPRequest* FTPManagerTest::_queueSizeFile(QObject* owner, int fileSize, Result& result, QStringList* order,
                                           const QString& name)
{
    FTPRequest* const request =
        _vehicle->ftpManager()->download(owner, MAV_COMP_ID_AUTOPILOT1, _sizeFileName(fileSize),
                                         QStandardPaths::writableLocation(QStandardPaths::TempLocation), {},
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

// A request talks to the component it was made for, whichever the requests around it address: it is sent there, and
// only an answer from there is taken. MockLink answers as the autopilot whichever component is asked, so a request to
// another component is not answered.
void FTPManagerTest::_testRequestsOfDifferentComponents()
{
    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();

    QObject owner;
    Result autopilot;
    Result camera;
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, autopilot));
    QVERIFY(
        _vehicle->ftpManager()->listDirectory(&owner, MAV_COMP_ID_CAMERA, QStringLiteral("/"), _recordList(camera)));

    QVERIFY_TRUE_WAIT(camera.calls == 1, TestTimeout::longMs());
    QCOMPARE(autopilot.calls, 1);
    QCOMPARE(autopilot.error, QString());
    _verifyFileContentsAndDelete(autopilot.path, 3 * 1024);
    QCOMPARE(camera.error, QStringLiteral("List directory failed"));
    QTRY_COMPARE_WITH_TIMEOUT(mockFtp->lastRequestTargetComponent(), static_cast<int>(MAV_COMP_ID_CAMERA),
                              TestTimeout::mediumMs());

    QCOMPARE(_downloadAndVerifySizeFile(2 * 1024), QString());
    QCOMPARE(mockFtp->lastRequestTargetComponent(), static_cast<int>(MAV_COMP_ID_AUTOPILOT1));
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

void FTPManagerTest::_testCancelRunningRequest_data()
{
    QTest::addColumn<QString>("operation");
    // Whether the next request is made before the cancel, and so has to start by itself
    QTest::addColumn<bool>("nextQueued");
    QTest::addColumn<bool>("fromProgressSignal");

    QTest::newRow("download") << "download" << true << false;
    // Nothing follows, whose leading ResetSessions would stand for the one that releases the session
    QTest::newRow("download, nothing queued behind") << "download" << false << false;
    QTest::newRow("upload, nothing queued behind") << "upload" << false << false;
    QTest::newRow("upload, cancelled from its progress signal") << "upload" << true << true;
    QTest::newRow("delete") << "delete" << true << false;
}

// Cancelling the request that runs ends it at once, without waiting for the vehicle: cancels happen on saturated
// links, where an ack is the packet most likely to be lost. The session of a download or an upload is released with
// a reset that expects no ack, the part of the file that was downloaded is removed, nothing is called back, and the
// next request runs.
void FTPManagerTest::_testCancelRunningRequest()
{
    QFETCH(QString, operation);
    QFETCH(bool, nextQueued);
    QFETCH(bool, fromProgressSignal);

    _connectMockLinkNoInitialConnectSequence();
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setSingleSessionEnforced(true);
    FTPManager* const ftpManager = _vehicle->ftpManager();
    QSignalSpy spyReset(mockFtp, &MockLinkFTP::resetCommandReceived);

    constexpr int fileSize = 100 * 1024;
    const bool download = (operation == QStringLiteral("download"));
    const bool holdsSession = (operation != QStringLiteral("delete"));
    QTemporaryFile uploadFile;
    QObject owner;
    Result cancelled;
    Result next;
    QPointer<FTPRequest> request;
    if (download) {
        mockFtp->setBurstReadDelayMs(100);
        request = _queueSizeFile(&owner, fileSize, cancelled);
    } else if (holdsSession) {
        QVERIFY(uploadFile.open());
        QCOMPARE(uploadFile.write(QByteArray(fileSize, 'u')), fileSize);
        uploadFile.close();
        request = ftpManager->upload(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/mock/upload/cancelled.bin"),
                                     uploadFile.fileName(), _record(cancelled));
    } else {
        request =
            ftpManager->deleteFile(&owner, MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/no-such-file"), _record(cancelled));
    }
    QVERIFY(request);
    if (nextQueued) {
        QVERIFY(_queueSizeFile(&owner, 3 * 1024, next));
    }

    int resetsBeforeCancel = 0;
    if (fromProgressSignal) {
        (void) connect(request, &FTPRequest::progressChanged, &owner, [request]() { request->cancel(); });
        QVERIFY_TRUE_WAIT(request.isNull(), TestTimeout::longMs());
    } else {
        // A download or an upload that reports progress has its session on the vehicle
        if (holdsSession) {
            QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
            QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
        } else {
            QSignalSpy spyStarted(request, &FTPRequest::started);
            QVERIFY(UnitTest::waitForSignal(spyStarted, TestTimeout::longMs(), QStringLiteral("started")));
        }
        resetsBeforeCancel = spyReset.count();
        request->cancel();
        QCOMPARE(ftpManager->requestCount(), nextQueued ? 1 : 0);
    }
    mockFtp->setBurstReadDelayMs(0);
    if (download) {
        QVERIFY(!QFile::exists(_sizeFilePath(fileSize)));
    }

    if (!nextQueued) {
        QTRY_VERIFY_WITH_TIMEOUT(spyReset.count() > resetsBeforeCancel, TestTimeout::mediumMs());
        QVERIFY(_queueSizeFile(&owner, 3 * 1024, next));
    }
    QVERIFY_TRUE_WAIT(next.calls == 1, TestTimeout::longMs());
    QCOMPARE(next.error, QString());
    _verifyFileContentsAndDelete(next.path, 3 * 1024);
    QCOMPARE(cancelled.calls, 0);
    QVERIFY(request.isNull());
}

// A receiver of started() may cancel the request: it is not begun, and the next one is
void FTPManagerTest::_testCancelFromStarted()
{
    _connectMockLinkNoInitialConnectSequence();

    QObject owner;
    Result cancelled;
    Result next;
    const QPointer<FTPRequest> request = _queueSizeFile(&owner, 3 * 1024, cancelled);
    QVERIFY(request);
    (void) connect(request, &FTPRequest::started, &owner, [request]() { request->cancel(); });
    QVERIFY(_queueSizeFile(&owner, 2 * 1024, next));

    QVERIFY_TRUE_WAIT(next.calls == 1, TestTimeout::longMs());
    QCOMPARE(next.error, QString());
    _verifyFileContentsAndDelete(next.path, 2 * 1024);
    QCOMPARE(cancelled.calls, 0);
    QCOMPARE(_mockLink->mockLinkFTP()->openFileROCount(), 1);
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
    QVERIFY(QFile::exists(_sizeFilePath(100 * 1024)));

    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    _disconnectMockLink();

    QVERIFY_TRUE_WAIT((running.calls == 1) && (waiting.calls == 1), TestTimeout::mediumMs());
    QCOMPARE(running.error, QStringLiteral("Vehicle disconnected"));
    QCOMPARE(waiting.error, QStringLiteral("Vehicle disconnected"));
    QVERIFY(!QFile::exists(running.path));
    QVERIFY(!QFile::exists(waiting.path));
}

// A request whose owner belongs to the vehicle ends with the vehicle, without a callback. An owner made before the
// FTPManager, as the component metadata is, goes first: its request that runs is ended as by cancel() while the vehicle
// is being destroyed. The request of an owner that stays is called back after it.
void FTPManagerTest::_testVehicleGoneWithRequestOfItsOwnChild()
{
    _connectMockLinkNoInitialConnectSequence();
    _mockLink->mockLinkFTP()->setBurstReadDelayMs(100);

    QObject* const vehicleOwner = new QObject(_vehicle->compInfoManager());
    QObject owner;
    Result running;
    Result waiting;
    FTPRequest* const request = _queueSizeFile(vehicleOwner, 100 * 1024, running);
    QVERIFY(request);
    QSignalSpy spyProgress(request, &FTPRequest::progressChanged);
    QVERIFY(_queueSizeFile(&owner, 3 * 1024, waiting));
    QVERIFY(UnitTest::waitForSignal(spyProgress, TestTimeout::longMs(), QStringLiteral("progressChanged")));
    QVERIFY(QFile::exists(_sizeFilePath(100 * 1024)));

    _mockLink->mockLinkFTP()->setBurstReadDelayMs(0);
    _disconnectMockLink();

    QVERIFY_TRUE_WAIT(waiting.calls == 1, TestTimeout::mediumMs());
    QCOMPARE(waiting.error, QStringLiteral("Vehicle disconnected"));
    QCOMPARE(running.calls, 0);
    QVERIFY(!QFile::exists(_sizeFilePath(100 * 1024)));
}

// cancel() ends a request without a callback also once its manager is gone, when the callback with the error is
// already on its way from the event loop
void FTPManagerTest::_testCancelAfterVehicleGone()
{
    _connectMockLinkNoInitialConnectSequence();

    QObject owner;
    Result cancelled;
    Result other;
    const QPointer<FTPRequest> request = _queueSizeFile(&owner, 3 * 1024, cancelled);
    QVERIFY(request);
    QVERIFY(_queueSizeFile(&owner, 2 * 1024, other));
    (void) connect(_vehicle->ftpManager(), &QObject::destroyed, &owner, [request]() {
        if (request) {
            request->cancel();
        }
    });

    _disconnectMockLink();

    // The requests are called back in their order: by now the cancelled one would have been
    QVERIFY_TRUE_WAIT(other.calls == 1, TestTimeout::mediumMs());
    QCOMPARE(other.error, QStringLiteral("Vehicle disconnected"));
    QCOMPARE(cancelled.calls, 0);
    QVERIFY(request.isNull());
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
