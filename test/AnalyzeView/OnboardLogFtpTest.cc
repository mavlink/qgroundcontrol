#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <array>
#include <chrono>
#include <limits>

#include "ArduCopterFirmwarePlugin.h"
#include "FTP/FtpDownloadSession.h"
#include "FTP/FtpListingParser.h"
#include "FTP/FtpListingSession.h"
#include "FTP/FtpTransport.h"
#include "FTPManager.h"
#include "FTPManagerJob.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "LogProtocolDownloadBatchSession.h"
#include "LogProtocolDownloadSession.h"
#include "LogProtocolTransport.h"
#include "MAVLinkProtocol.h"
#include "MavlinkSettings.h"
#include "MockLinkFTP.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "OnboardLogController.h"
#include "OnboardLogDownloadTest.h"
#include "OnboardLogEntry.h"
#include "OnboardLogFileName.h"
#include "OnboardLogModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {

QStringList makeFtpLogEntries(qsizetype entryCount)
{
    QStringList entries;
    entries.reserve(entryCount);
    for (qsizetype index = 0; index < entryCount; ++index) {
        entries.append(QStringLiteral("Flog_%1.ulg\t128").arg(index));
    }
    return entries;
}

}  // namespace

void OnboardLogDownloadTest::_ftpEraseCancelTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());
    OnboardLogModel* const model = transport.model();
    QVERIFY(model);

    for (uint id = 0; id < 2; ++id) {
        OnboardLogEntry* const entry = new OnboardLogEntry(id, QDateTime::currentDateTimeUtc(), 1024, true, &transport);
        entry->setFtpPath(QStringLiteral("/logs/log%1.ulg").arg(id));
        model->append(entry);
    }

    FTPManager* const ftpManager = vehicle()->ftpManager();
    QVERIFY(ftpManager);
    QSignalSpy requestingListSpy(&transport, &FtpTransport::requestingListChanged);
    QSignalSpy busySpy(&transport, &FtpTransport::busyChanged);

    transport.eraseAll();
    FTPDeleteJob* const eraseJob = transport._deleteJob;
    QVERIFY(eraseJob);
    QSignalSpy deleteCompleteSpy(eraseJob, &FTPDeleteJob::finished);
    QVERIFY(transport._eraseSession.active());
    QVERIFY(transport._eraseSession.currentEntry());
    QCOMPARE(transport._eraseSession.currentEntry()->id(), 1U);
    QCOMPARE(transport._eraseSession.pendingCount(), 1);
    QCOMPARE(model->value<OnboardLogEntry*>(0)->state(), OnboardLogEntry::State::Available);
    QCOMPARE(model->value<OnboardLogEntry*>(1)->state(), OnboardLogEntry::State::Erasing);
    QVERIFY(transport.busy());
    QVERIFY(!transport.requestingList());

    transport.cancel();

    QVERIFY(!transport._eraseSession.active());
    QVERIFY(!transport.busy());
    QVERIFY(!transport.requestingList());
    QVERIFY(!transport._eraseSession.currentEntry());
    QCOMPARE(transport._eraseSession.pendingCount(), 0);
    QCOMPARE(model->count(), 2);
    QCOMPARE(model->value<OnboardLogEntry*>(0)->state(), OnboardLogEntry::State::Available);
    QCOMPARE(model->value<OnboardLogEntry*>(1)->state(), OnboardLogEntry::State::Canceled);
    QCOMPARE(deleteCompleteSpy.size(), 1);
    QCOMPARE(requestingListSpy.size(), 0);
    QCOMPARE(busySpy.size(), 2);

    // A new delete can start only if cancellation did not re-enter and start the second queued erase.
    FTPDeleteJob* const probeJob =
        ftpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/logs/probe.ulg")).job();
    QVERIFY(probeJob);
    QSignalSpy probeCompleteSpy(probeJob, &FTPDeleteJob::finished);
    probeJob->cancel();
    QCOMPARE(probeCompleteSpy.size(), 1);
}

void OnboardLogDownloadTest::_ftpDownloadRefreshCancelTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());
    OnboardLogModel* const model = transport.model();
    QVERIFY(model);

    constexpr uint kFileSize = 4 * 1024 * 1024;
    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime::currentDateTimeUtc(), kFileSize, true, &transport);
    entry->setFtpPath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(kFileSize));
    entry->setSelected(true);
    model->append(entry);

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setErrorMode(MockLinkFTP::errModeNone);

    transport.download(downloadDir.path());
    QVERIFY(transport.downloadingLogs());
    FTPDownloadJob* const downloadJob = transport._downloadJob;
    QVERIFY(downloadJob);
    QSignalSpy progressSpy(downloadJob, &FTPJob::progress);
    if (progressSpy.isEmpty()) {
        QVERIFY(progressSpy.wait(TestTimeout::longMs()));
    }

    bool listingStarted = false;
    const QMetaObject::Connection listingConnection =
        connect(&transport, &FtpTransport::requestingListChanged, this,
                [&transport, &listingStarted]() { listingStarted = listingStarted || transport.requestingList(); });

    transport.refresh();

    QTRY_VERIFY_WITH_TIMEOUT(listingStarted, TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(!transport.downloadingLogs(), TestTimeout::longMs());
    QCOMPARE(transport.batchProgress(), 0.);
    QVERIFY(!transport._downloadSession.canceling());
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    transport.cancel();
    (void) disconnect(listingConnection);
}

void OnboardLogDownloadTest::_ftpCancellationPreservesFinalPathTest()
{
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    FtpTransport transport;
    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime(), 128, true, &transport);
    transport.model()->append(entry);
    (void) transport._downloadSession.begin({entry}, downloadDir.path());
    QCOMPARE(transport._downloadSession.takeNext(), entry);
    transport._downloadSession.setInFlight(true);
    transport._setDownloading(true);

    const FtpDownloadSession::Cancellation cancellation = transport._downloadSession.cancel();
    QVERIFY(cancellation.wasActive);
    QVERIFY(cancellation.cancelRemoteDownload);

    const QString finalPath = downloadDir.filePath(QStringLiteral("created-during-cancel.ulg"));
    QFile finalFile(finalPath);
    QVERIFY(finalFile.open(QIODevice::WriteOnly));
    QCOMPARE(finalFile.write("unrelated"), qint64(9));
    finalFile.close();

    transport._downloadComplete(finalPath, QStringLiteral("Aborted"));
    QVERIFY(QFile::exists(finalPath));
    QVERIFY(!transport._downloadSession.canceling());
    QVERIFY(!transport.downloadingLogs());
}

void OnboardLogDownloadTest::_ftpPreOpenRefreshCancelTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());
    OnboardLogModel* const model = transport.model();
    QVERIFY(model);

    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime::currentDateTimeUtc(), 4096, true, &transport);
    entry->setFtpPath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(4096));
    entry->setSelected(true);
    model->append(entry);

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);

    bool listingStarted = false;
    const QMetaObject::Connection listingConnection =
        connect(&transport, &FtpTransport::requestingListChanged, this,
                [&transport, &listingStarted]() { listingStarted = listingStarted || transport.requestingList(); });

    transport.download(downloadDir.path());
    QVERIFY(transport.downloadingLogs());
    transport.refresh();

    QVERIFY(listingStarted);
    QVERIFY(!transport.downloadingLogs());
    QVERIFY(!transport._downloadSession.canceling());
    QVERIFY(transport.requestingList());
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());

    transport.cancel();
    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
    (void) disconnect(listingConnection);
}

void OnboardLogDownloadTest::_ftpReentrantRefreshIsCoalescedTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());

    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    QVERIFY(mockFtp);
    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);
    const auto restoreFtpMode = qScopeGuard([mockFtp]() { mockFtp->setErrorMode(MockLinkFTP::errModeNone); });

    transport.refresh();
    QVERIFY(transport.requestingList());
    const quint64 firstGeneration = transport._listingSession.generation();

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    bool reentered = false;
    bool replacementDownloadStarted = false;
    const QMetaObject::Connection reentrantConnection = connect(
        &transport, &FtpTransport::listingFinished, &transport,
        [&transport, &downloadDir, &reentered, &replacementDownloadStarted](OnboardLogTransport::ListingResult result) {
            if (!reentered && (result == OnboardLogTransport::ListingResult::Canceled)) {
                reentered = true;
                transport.download(downloadDir.path());
                replacementDownloadStarted = transport.downloadingLogs();
                transport.refresh();
            }
        });

    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Ignoring download request while onboard log cancellation")));
    transport.cancel();
    verifyExpectedLogMessage();

    QVERIFY(reentered);
    QVERIFY(!replacementDownloadStarted);
    QVERIFY(transport.requestingList());
    QVERIFY(transport._listingSession.active());
    QCOMPARE(transport._listingSession.generation(), firstGeneration + 2);
    QVERIFY(transport._listingJob);
    QVERIFY(!transport.errorMessage().contains(QStringLiteral("busy"), Qt::CaseInsensitive));

    (void) disconnect(reentrantConnection);
    transport.cancel();
}

void OnboardLogDownloadTest::_ftpRemoteSizeGrowthTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());

    constexpr uint kListedSize = 32;
    constexpr uint kRemoteSize = 64;
    OnboardLogEntry* const entry =
        new OnboardLogEntry(0, QDateTime::currentDateTimeUtc(), kListedSize, true, &transport);
    entry->setFtpPath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(kRemoteSize));
    entry->setSelected(true);
    transport.model()->append(entry);

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral(".*exceeds expected maximum.*")));

    transport.download(downloadDir.path());
    QTRY_VERIFY_WITH_TIMEOUT(!transport.downloadingLogs(), TestTimeout::longMs());
    verifyExpectedLogMessage();

    QCOMPARE(entry->state(), OnboardLogEntry::State::Error);
    QVERIFY(entry->errorMessage().contains(QStringLiteral("exceeds expected maximum")));
    QVERIFY(QDir(downloadDir.path()).entryList(QDir::Files).isEmpty());
}

void OnboardLogDownloadTest::_ftpUnsafePathTest()
{
    QVERIFY(FtpListingParser::isSafePathComponent(QStringLiteral("log.ulg")));
    QVERIFY(FtpListingParser::isSafePathComponent(QStringLiteral("2026-07-14")));
    QVERIFY(!FtpListingParser::isSafePathComponent(QStringLiteral(".")));
    QVERIFY(!FtpListingParser::isSafePathComponent(QStringLiteral("..")));
    QVERIFY(!FtpListingParser::isSafePathComponent(QStringLiteral("../escape.ulg")));
    QVERIFY(!FtpListingParser::isSafePathComponent(QStringLiteral("..\\escape.ulg")));
    QVERIFY(!FtpListingParser::isSafePathComponent(QStringLiteral("bad\nname.ulg")));

    FtpTransport transport;
    (void) transport._listingSession.begin(QStringLiteral("/logs"));
    const QStringList entries = {
        QStringLiteral("Fsafe.ulg\t128"),
        QStringLiteral("F../escape.ulg\t128"),
        QStringLiteral("F..\\escape.bin\t128"),
    };

    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("ignoring unsafe FTP path component")));
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("ignoring unsafe FTP path component")));
    const std::optional<uint> processedEntries = transport._processFileEntries(entries, QString());
    QVERIFY(processedEntries.has_value());
    QCOMPARE(*processedEntries, 1U);
    transport._appendNextLogEntryBatch();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(transport.model()->count(), 1);
    QCOMPARE(transport.model()->value<OnboardLogEntry*>(0)->ftpPath(), QStringLiteral("/logs/safe.ulg"));

    OnboardLogEntry binEntry(42, QDateTime(), 128, true);
    binEntry.setFtpPath(QStringLiteral("/logs/flight.BIN"));
    QCOMPARE(FtpTransport::_localFilenameForEntry(binEntry), QStringLiteral("log_42.bin"));

    OnboardLogEntry ulgEntry(43, QDateTime(), 128, true);
    ulgEntry.setFtpPath(QStringLiteral("/logs/flight.ulg"));
    QCOMPARE(FtpTransport::_localFilenameForEntry(ulgEntry), QStringLiteral("log_43.ulg"));

    QTemporaryDir filenameDir;
    QVERIFY(filenameDir.isValid());
    QFile existingFile(filenameDir.filePath(QStringLiteral("flight.ulg")));
    QVERIFY(existingFile.open(QIODevice::WriteOnly));
    existingFile.close();

    const OnboardLogFileName::UniquePath firstDuplicate =
        OnboardLogFileName::uniquePath(filenameDir.path(), QStringLiteral("flight.ulg"));
    QCOMPARE(firstDuplicate.fileName, QStringLiteral("flight_1.ulg"));
    QCOMPARE(firstDuplicate.filePath, filenameDir.filePath(QStringLiteral("flight_1.ulg")));

    QFile firstDuplicateFile(firstDuplicate.filePath);
    QVERIFY(firstDuplicateFile.open(QIODevice::WriteOnly));
    firstDuplicateFile.close();
    QCOMPARE(OnboardLogFileName::uniquePath(filenameDir.path(), QStringLiteral("flight.ulg")).fileName,
             QStringLiteral("flight_2.ulg"));

    OnboardLogEntry fallbackEntry(44, QDateTime(), 128, true);
    fallbackEntry.setFtpPath(QStringLiteral("mocklink-size-128"));
    QCOMPARE(FtpTransport::_localFilenameForEntry(fallbackEntry), QStringLiteral("log_44.ulg"));
}

void OnboardLogDownloadTest::_ftpListingBatchLifecycleTest()
{
    const qsizetype entryCount = (FtpTransport::kLogInsertionBatchSize * 2) + 1;
    const QStringList entries = makeFtpLogEntries(entryCount);

    {
        FtpTransport transport;
        (void) transport._listingSession.begin(QStringLiteral("/logs"));
        transport._setListing(true);
        QSignalSpy listingFinishedSpy(&transport, &FtpTransport::listingFinished);

        transport._listDirComplete(entries, QString(), false);

        QCOMPARE(transport.model()->count(), FtpTransport::kLogInsertionBatchSize);
        QVERIFY(transport.requestingList());
        QCOMPARE(listingFinishedSpy.size(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(transport.model()->count(), entryCount, TestTimeout::longMs());
        QTRY_COMPARE_WITH_TIMEOUT(listingFinishedSpy.size(), 1, TestTimeout::longMs());
        QCOMPARE(listingFinishedSpy.constFirst().at(0).value<OnboardLogTransport::ListingResult>(),
                 OnboardLogTransport::ListingResult::Success);
        QVERIFY(!transport.requestingList());
    }

    {
        FtpTransport transport;
        (void) transport._listingSession.begin(QStringLiteral("/logs"));
        transport._setListing(true);
        QSignalSpy listingFinishedSpy(&transport, &FtpTransport::listingFinished);

        transport._listDirComplete(entries, QString(), false);
        QCOMPARE(transport.model()->count(), FtpTransport::kLogInsertionBatchSize);
        transport.cancel();

        QCOMPARE(listingFinishedSpy.size(), 1);
        QCOMPARE(listingFinishedSpy.constFirst().at(0).value<OnboardLogTransport::ListingResult>(),
                 OnboardLogTransport::ListingResult::Canceled);
        const int countAfterCancel = transport.model()->count();
        bool eventLoopTurnComplete = false;
        QTimer::singleShot(0, &transport, [&eventLoopTurnComplete]() { eventLoopTurnComplete = true; });
        QTRY_VERIFY_WITH_TIMEOUT(eventLoopTurnComplete, TestTimeout::shortMs());
        QCOMPARE(transport.model()->count(), countAfterCancel);
        QCOMPARE(listingFinishedSpy.size(), 1);
        QVERIFY(!transport.requestingList());
    }

    {
        FtpTransport transport;
        (void) transport._listingSession.begin(QStringLiteral("/logs"));
        transport._setListing(true);
        QSignalSpy listingFinishedSpy(&transport, &FtpTransport::listingFinished);

        transport._listDirComplete(entries, QString(), false);
        QCOMPARE(transport.model()->count(), FtpTransport::kLogInsertionBatchSize);
        transport.setVehicle(nullptr);

        QCOMPARE(transport.model()->count(), 0);
        QCOMPARE(listingFinishedSpy.size(), 1);
        QCOMPARE(listingFinishedSpy.constFirst().at(0).value<OnboardLogTransport::ListingResult>(),
                 OnboardLogTransport::ListingResult::Canceled);
        bool eventLoopTurnComplete = false;
        QTimer::singleShot(0, &transport, [&eventLoopTurnComplete]() { eventLoopTurnComplete = true; });
        QTRY_VERIFY_WITH_TIMEOUT(eventLoopTurnComplete, TestTimeout::shortMs());
        QCOMPARE(transport.model()->count(), 0);
        QCOMPARE(listingFinishedSpy.size(), 1);
        QVERIFY(!transport.requestingList());
    }
}

void OnboardLogDownloadTest::_ftpListingJobOwnershipTest()
{
    const qsizetype entryCount = (FtpTransport::kLogInsertionBatchSize * 2) + 1;
    QStringList entries = makeFtpLogEntries(entryCount);
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    FTPManager* const ftpManager = vehicle()->ftpManager();
    QVERIFY(mockFtp);
    QVERIFY(ftpManager);

    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);

    {
        FtpTransport transport;
        transport.setVehicle(vehicle());
        (void) transport._listingSession.begin(QStringLiteral("/logs"));
        transport._setListing(true);
        entries.append(QStringLiteral("D2026-07-15"));

        transport._listDirComplete(entries, QString(), false);

        QCOMPARE(transport.model()->count(), 0);
        QVERIFY(transport._pendingLogInsertion.has_value());
        QCOMPARE(transport._pendingLogInsertion->descriptors.size(), entryCount);
        QVERIFY(transport._listingJob);
        QVERIFY(transport._listingJob->active());

        transport.cancel();
        QVERIFY(!transport._listingJob);
    }

    entries.removeLast();
    {
        FtpTransport transport;
        transport.setVehicle(vehicle());
        (void) transport._listingSession.begin(QStringLiteral("/logs"));
        transport._setListing(true);

        transport._listDirComplete(entries, QString(), false);
        QCOMPARE(transport.model()->count(), FtpTransport::kLogInsertionBatchSize);
        QVERIFY(!transport._listingJob);

        FTPListDirectoryJob* const unrelatedJob =
            ftpManager->startListDirectory(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/unrelated")).job();
        QVERIFY(unrelatedJob);
        QVERIFY(unrelatedJob->active());

        transport.cancel();

        QVERIFY(unrelatedJob->active());
        unrelatedJob->cancel();
        QVERIFY(!unrelatedJob->active());
    }

    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
}

void OnboardLogDownloadTest::_ftpSubdirectoryErrorContinuesTest()
{
    FtpTransport transport;
    (void) transport._listingSession.begin(QStringLiteral("/logs"));
    const FtpListingParser::ParseResult result =
        transport._listingSession.parse({QStringLiteral("Dfirst"), QStringLiteral("Dsecond")}, QString(), true);
    QCOMPARE(result.directories.size(), 2);
    transport._listingSession.beginSubdirectories();
    transport._setListing(true);

    QSignalSpy listingFinishedSpy(&transport, &FtpTransport::listingFinished);
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to list onboard log directory first: simulated")));
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to list onboard log directory second")));

    transport._listDirComplete({}, QStringLiteral("simulated failure"), false);

    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(listingFinishedSpy.size(), 1);
    QCOMPARE(listingFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Partial);
    QVERIFY(!transport._listingSession.active());
    QVERIFY(!transport.requestingList());
}

void OnboardLogDownloadTest::_downloadPreflightTest()
{
    QTemporaryDir downloadDirectory;
    QVERIFY(downloadDirectory.isValid());

    FtpTransport ftpTransport;
    OnboardLogEntry* const ftpEntry = new OnboardLogEntry(0, QDateTime(), 128, true, &ftpTransport);
    ftpEntry->setFtpPath(QStringLiteral("/logs/flight.ulg"));
    ftpEntry->setSelected(true);
    ftpTransport.model()->append(ftpEntry);

    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No active vehicle")));
    ftpTransport.download(downloadDirectory.path());
    verifyExpectedLogMessage();
    QVERIFY(!ftpTransport.busy());
    QVERIFY(!ftpTransport.errorMessage().isEmpty());

    LogProtocolTransport logTransport;
    OnboardLogEntry* const logEntry = new OnboardLogEntry(0, QDateTime(), 128, true, &logTransport);
    logEntry->setSelected(true);
    logTransport.model()->append(logEntry);

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No active vehicle")));
    logTransport.download(downloadDirectory.path());
    verifyExpectedLogMessage();
    QVERIFY(!logTransport.busy());
    QVERIFY(!logTransport.errorMessage().isEmpty());
}

void OnboardLogDownloadTest::_ftpVehicleChangeDuringEraseTest()
{
    FtpTransport transport;
    transport.setVehicle(vehicle());
    OnboardLogModel* const model = transport.model();
    QVERIFY(model);

    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime::currentDateTimeUtc(), 1024, true, &transport);
    entry->setFtpPath(QStringLiteral("/logs/log0.ulg"));
    model->append(entry);

    FTPManager* const oldFtpManager = vehicle()->ftpManager();
    QVERIFY(oldFtpManager);

    transport.eraseAll();
    QVERIFY(transport._eraseSession.active());

    transport.setVehicle(nullptr);

    QVERIFY(!transport._vehicle);
    QVERIFY(!transport._eraseSession.active());
    QVERIFY(!transport.requestingList());
    QVERIFY(!transport._eraseSession.currentEntry());
    QCOMPARE(transport._eraseSession.pendingCount(), 0);
    QCOMPARE(model->count(), 0);

    FTPDeleteJob* const probeJob =
        oldFtpManager->startDeleteFile(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/logs/probe.ulg")).job();
    QVERIFY(probeJob);
    probeJob->cancel();

    FtpTransport reentrantTransport;
    reentrantTransport.setVehicle(vehicle());
    OnboardLogEntry* const reentrantEntry =
        new OnboardLogEntry(1, QDateTime::currentDateTimeUtc(), 1024, true, &reentrantTransport);
    reentrantEntry->setFtpPath(QStringLiteral("/logs/reentrant.ulg"));
    reentrantTransport.model()->append(reentrantEntry);

    bool vehicleChangedDuringStartup = false;
    const QMetaObject::Connection reentrantConnection =
        connect(reentrantEntry, &OnboardLogEntry::stateChanged, &reentrantTransport,
                [&reentrantTransport, &vehicleChangedDuringStartup, reentrantEntry]() {
                    if (!vehicleChangedDuringStartup && (reentrantEntry->state() == OnboardLogEntry::State::Erasing)) {
                        vehicleChangedDuringStartup = true;
                        reentrantTransport.setVehicle(nullptr);
                    }
                });

    reentrantTransport.eraseAll();
    QVERIFY(vehicleChangedDuringStartup);
    QVERIFY(!reentrantTransport._vehicle);
    QVERIFY(!reentrantTransport._eraseSession.active());
    QVERIFY(!reentrantTransport._eraseSession.currentEntry());
    QCOMPARE(reentrantTransport._eraseSession.pendingCount(), 0);
    (void) disconnect(reentrantConnection);
}

void OnboardLogDownloadTest::_downloadReentrancyTest()
{
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    LogProtocolTransport logTransport;
    logTransport.setVehicle(vehicle());
    OnboardLogEntry* const logEntry = new OnboardLogEntry(0, QDateTime(), 128, true, &logTransport);
    logTransport.model()->append(logEntry);
    (void) logTransport._downloadBatch.begin({logEntry}, QStringLiteral("original-log-path"));
    LogProtocolDownloadSession* const originalDownloadSession = logTransport._downloadBatch.startNext();
    logTransport._setDownloading(true);

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("another onboard log operation is in progress")));
    logTransport.download(downloadDir.path());
    verifyExpectedLogMessage();

    QCOMPARE(logTransport._downloadBatch.current(), originalDownloadSession);
    QCOMPARE(logTransport._downloadBatch.path(), QStringLiteral("original-log-path"));
    QVERIFY(logTransport.downloadingLogs());
    logTransport._downloadBatch.clear();
    logTransport._setDownloading(false);

    (void) logTransport._downloadBatch.begin({logEntry}, QStringLiteral("original-log-path"));
    QVERIFY(logTransport._downloadBatch.startNext());
    logTransport._setDownloading(true);
    const QMetaObject::Connection logCancelConnection =
        connect(logEntry, &OnboardLogEntry::statusChanged, &logTransport, [&logTransport]() { logTransport.cancel(); });

    QVERIFY(!logTransport._updateDataRate(0));
    QVERIFY(!logTransport._downloadBatch.current());
    QVERIFY(!logTransport.downloadingLogs());
    (void) disconnect(logCancelConnection);

    FtpTransport ftpTransport;
    OnboardLogEntry* const activeEntry = new OnboardLogEntry(1, QDateTime(), 128, true, &ftpTransport);
    OnboardLogEntry* const queuedEntry = new OnboardLogEntry(2, QDateTime(), 128, true, &ftpTransport);
    ftpTransport.model()->append({activeEntry, queuedEntry});
    (void) ftpTransport._downloadSession.begin({activeEntry, queuedEntry}, QStringLiteral("original-ftp-path"));
    QCOMPARE(ftpTransport._downloadSession.takeNext(), activeEntry);
    ftpTransport._setDownloading(true);

    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("another onboard log operation is in progress")));
    ftpTransport.download(downloadDir.path());
    verifyExpectedLogMessage();

    QCOMPARE(ftpTransport._downloadSession.currentEntry(), activeEntry);
    QCOMPARE(ftpTransport._downloadSession.pendingCount(), 1);
    QCOMPARE(ftpTransport._downloadSession.path(), QStringLiteral("original-ftp-path"));
    QVERIFY(ftpTransport.downloadingLogs());

    ftpTransport._batchState.addFile(activeEntry->size());
    ftpTransport._batchState.addFile(queuedEntry->size());
    ftpTransport._downloadSession.setInFlight(true);
    const QMetaObject::Connection ftpCancelConnection = connect(
        activeEntry, &OnboardLogEntry::statusChanged, &ftpTransport, [&ftpTransport]() { ftpTransport.cancel(); });

    ftpTransport._downloadComplete(QStringLiteral("completed.ulg"), QString());
    QVERIFY(!ftpTransport.downloadingLogs());
    QVERIFY(!ftpTransport._downloadSession.currentEntry());
    QCOMPARE(ftpTransport._downloadSession.pendingCount(), 0);
    (void) disconnect(ftpCancelConnection);

    LogProtocolTransport logStartupTransport;
    logStartupTransport.setVehicle(vehicle());
    OnboardLogEntry* const selectedLogEntry = new OnboardLogEntry(3, QDateTime(), 128, true, &logStartupTransport);
    OnboardLogEntry* const replacementLogEntry = new OnboardLogEntry(4, QDateTime(), 128, true, &logStartupTransport);
    selectedLogEntry->setSelected(true);
    logStartupTransport.model()->append({selectedLogEntry, replacementLogEntry});

    bool logBatchReplaced = false;
    const QMetaObject::Connection logStartupConnection =
        connect(&logStartupTransport, &LogProtocolTransport::downloadingLogsChanged, &logStartupTransport,
                [&logStartupTransport, replacementLogEntry, &logBatchReplaced]() {
                    if (!logBatchReplaced && logStartupTransport.downloadingLogs()) {
                        logBatchReplaced = true;
                        (void) logStartupTransport._downloadBatch.begin({replacementLogEntry},
                                                                        QStringLiteral("replacement-log-path"));
                    }
                });

    logStartupTransport.download(downloadDir.path());
    QVERIFY(logBatchReplaced);
    QCOMPARE(logStartupTransport._downloadBatch.path(), QStringLiteral("replacement-log-path"));
    QCOMPARE(logStartupTransport._downloadBatch.pendingCount(), 1);
    QVERIFY(!logStartupTransport._downloadBatch.current());
    QVERIFY(logStartupTransport.downloadingLogs());
    logStartupTransport.cancel();
    (void) disconnect(logStartupConnection);

    FtpTransport ftpStartupTransport;
    ftpStartupTransport.setVehicle(vehicle());
    OnboardLogEntry* const selectedFtpEntry = new OnboardLogEntry(5, QDateTime(), 128, true, &ftpStartupTransport);
    OnboardLogEntry* const replacementFtpEntry = new OnboardLogEntry(6, QDateTime(), 128, true, &ftpStartupTransport);
    selectedFtpEntry->setFtpPath(QStringLiteral("/logs/selected.ulg"));
    replacementFtpEntry->setFtpPath(QStringLiteral("/logs/replacement.ulg"));
    selectedFtpEntry->setSelected(true);
    ftpStartupTransport.model()->append({selectedFtpEntry, replacementFtpEntry});

    bool ftpBatchReplaced = false;
    const QMetaObject::Connection ftpStartupConnection =
        connect(&ftpStartupTransport, &FtpTransport::downloadingLogsChanged, &ftpStartupTransport,
                [&ftpStartupTransport, replacementFtpEntry, &ftpBatchReplaced]() {
                    if (!ftpBatchReplaced && ftpStartupTransport.downloadingLogs()) {
                        ftpBatchReplaced = true;
                        (void) ftpStartupTransport._downloadSession.begin({replacementFtpEntry},
                                                                          QStringLiteral("replacement-ftp-path"));
                    }
                });

    ftpStartupTransport.download(downloadDir.path());
    QVERIFY(ftpBatchReplaced);
    QCOMPARE(ftpStartupTransport._downloadSession.path(), QStringLiteral("replacement-ftp-path"));
    QCOMPARE(ftpStartupTransport._downloadSession.pendingCount(), 1);
    QVERIFY(!ftpStartupTransport._downloadSession.currentEntry());
    QVERIFY(ftpStartupTransport.downloadingLogs());
    ftpStartupTransport.cancel();
    (void) disconnect(ftpStartupConnection);
}

void OnboardLogDownloadTest::_ftpTransportDestructionReentrancyTest()
{
    {
        QPointer<FtpTransport> transport = new FtpTransport;
        transport->setVehicle(vehicle());
        transport->_setErrorMessage(QStringLiteral("stale error"));
        (void) connect(transport, &FtpTransport::errorMessageChanged, this, [transport]() { delete transport.data(); });

        transport->_downloadToDirectory(QString());
        QVERIFY(!transport);
    }

    {
        QPointer<FtpTransport> transport = new FtpTransport;
        transport->setVehicle(vehicle());
        OnboardLogEntry* const entry = new OnboardLogEntry(10, QDateTime(), 128, true, transport);
        entry->setFtpPath(QStringLiteral("/logs/startup.ulg"));
        entry->setSelected(true);
        transport->model()->append(entry);
        QTemporaryDir downloadDir;
        QVERIFY(downloadDir.isValid());
        (void) connect(transport, &FtpTransport::downloadingLogsChanged, this, [transport]() {
            if (transport && transport->downloadingLogs()) {
                delete transport.data();
            }
        });

        transport->download(downloadDir.path());
        QVERIFY(!transport);
    }

    {
        QPointer<FtpTransport> transport = new FtpTransport;
        transport->setVehicle(vehicle());
        OnboardLogEntry* const entry = new OnboardLogEntry(11, QDateTime(), 128, true, transport);
        entry->setFtpPath(QStringLiteral("/logs/erase.ulg"));
        transport->model()->append(entry);
        (void) connect(entry, &OnboardLogEntry::stateChanged, this, [transport, entry]() {
            if (transport && (entry->state() == OnboardLogEntry::State::Erasing)) {
                delete transport.data();
            }
        });

        transport->eraseAll();
        QVERIFY(!transport);
    }

    {
        QPointer<FtpTransport> transport = new FtpTransport;
        (void) transport->_listingSession.begin(QStringLiteral("/logs"));
        (void) connect(transport, &FtpTransport::errorMessageChanged, this, [transport]() { delete transport.data(); });
        expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("ignoring unsafe FTP path component")));

        const std::optional<uint> result =
            transport->_processFileEntries({QStringLiteral("F../escape.ulg\t128")}, QString());
        QVERIFY(!result.has_value());
        QVERIFY(!transport);
        verifyExpectedLogMessage();
    }
}

void OnboardLogDownloadTest::_transportFeedbackTest()
{
    Fact* const transportFact = SettingsManager::instance()->mavlinkSettings()->onboardLogTransport();
    const QVariant originalOverride = transportFact->rawValue();
    const auto restoreOverride =
        qScopeGuard([transportFact, originalOverride]() { transportFact->setRawValue(originalOverride); });
    transportFact->setRawValue(1U);

    OnboardLogController controller;
    QCOMPARE(controller.transportKind(), OnboardLogController::TransportKind::LogProtocol);

    LogProtocolTransport* const logTransport = controller._logTransport;
    QVERIFY(logTransport);
    OnboardLogEntry* const logEntry = new OnboardLogEntry(0, QDateTime(), 100, true, logTransport);
    logTransport->model()->append(logEntry);

    QSignalSpy controllerBusySpy(&controller, &OnboardLogController::busyChanged);
    QVERIFY(!controller.busy());
    logTransport->_setListing(true);
    QVERIFY(controller.busy());
    logTransport->_setListing(false);
    QVERIFY(!controller.busy());
    QCOMPARE(controllerBusySpy.size(), 2);

    QSignalSpy controllerErrorSpy(&controller, &OnboardLogController::errorMessageChanged);
    logTransport->_setEntryError(logEntry, QStringLiteral("Detailed transfer failure"));
    QCOMPARE(logEntry->status(), QStringLiteral("Error"));
    QCOMPARE(logEntry->errorMessage(), QStringLiteral("Detailed transfer failure"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Detailed transfer failure"));
    QCOMPARE(controllerErrorSpy.size(), 1);

    logTransport->_batchState.totalBytes = 200;
    logTransport->_batchState.completedBytes = 50;

    QSignalSpy controllerProgressSpy(&controller, &OnboardLogController::batchProgressChanged);
    logTransport->_updateBatchProgress(50);
    QCOMPARE(controller.batchProgress(), 0.5);
    QCOMPARE(controllerProgressSpy.size(), 1);

    logTransport->setVehicle(nullptr);
    QVERIFY(controller.errorMessage().isEmpty());
    logTransport->_setEntryError(nullptr, QStringLiteral("No vehicle"));
    logTransport->setVehicle(vehicle());
    QVERIFY(controller.errorMessage().isEmpty());

    FtpTransport ftpTransport;
    ftpTransport.setVehicle(vehicle());
    OnboardLogEntry* const ftpEntry = new OnboardLogEntry(1, QDateTime(), 200, true, &ftpTransport);
    ftpEntry->setFtpPath(QStringLiteral("/logs/1.ulg"));
    ftpTransport.model()->append(ftpEntry);
    ftpTransport._batchState.totalBytes = 400;
    ftpTransport._batchState.completedBytes = 100;
    ftpTransport._updateBatchProgress(100, 0.5);
    QCOMPARE(ftpTransport.batchProgress(), 0.5);

    ftpEntry->setErrorMessage(QStringLiteral("Old download failure"));
    ftpTransport.eraseAll();
    QVERIFY(ftpEntry->errorMessage().isEmpty());
    ftpTransport.cancel();
    ftpTransport.setVehicle(nullptr);
    ftpTransport._setEntryError(nullptr, QStringLiteral("No vehicle"));
    ftpTransport.setVehicle(vehicle());
    QVERIFY(ftpTransport.errorMessage().isEmpty());
}

void OnboardLogDownloadTest::_ftpDeferredContinuationTest()
{
    FtpTransport transport;
    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime(), 100, true, &transport);
    transport.model()->append(entry);
    (void) transport._downloadSession.begin({entry}, QStringLiteral("/tmp"));
    QCOMPARE(transport._downloadSession.takeNext(), entry);
    transport._batchState.addFile(entry->size());
    transport._setDownloading(true);
    transport._downloadSession.setInFlight(true);

    const QString cleanupWarning = QStringLiteral("remote session cleanup failed");
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QRegularExpression::escape(cleanupWarning)));
    transport._downloadComplete(QStringLiteral("completed.ulg"), QString(), cleanupWarning);
    verifyExpectedLogMessage();

    QVERIFY(transport.downloadingLogs());
    QVERIFY(!transport._downloadSession.currentEntry());
    QCOMPARE(entry->status(), QStringLiteral("Downloaded"));
    QCOMPARE(entry->state(), OnboardLogEntry::State::Downloaded);
    QCOMPARE(entry->errorMessage(), cleanupWarning);
    QCOMPARE(transport.errorMessage(), cleanupWarning);
    QCOMPARE(transport.batchProgress(), 1.);

    transport.cancel();
    QVERIFY(!transport.downloadingLogs());
    QVERIFY(!transport._downloadSession.canceling());
    QCoreApplication::processEvents();
    QVERIFY(!transport.downloadingLogs());

    FtpTransport disappearingEntryTransport;
    disappearingEntryTransport.setVehicle(vehicle());
    OnboardLogEntry* const disappearingEntry =
        new OnboardLogEntry(1, QDateTime(), 100, true, &disappearingEntryTransport);
    disappearingEntryTransport.model()->append(disappearingEntry);
    (void) disappearingEntryTransport._downloadSession.begin({disappearingEntry}, QStringLiteral("/tmp"));
    QCOMPARE(disappearingEntryTransport._downloadSession.takeNext(), disappearingEntry);
    QCOMPARE(disappearingEntryTransport._downloadSession.currentEntrySize(), disappearingEntry->size());
    disappearingEntryTransport._batchState.addFile(disappearingEntry->size());
    disappearingEntryTransport._setDownloading(true);
    disappearingEntryTransport._downloadSession.setInFlight(true);

    QCOMPARE(disappearingEntryTransport.model()->removeOne(disappearingEntry), disappearingEntry);
    delete disappearingEntry;
    QVERIFY(!disappearingEntryTransport._downloadSession.currentEntry());

    disappearingEntryTransport._downloadComplete(QStringLiteral("completed.ulg"),
                                                 QStringLiteral("finalization failed"));

    QCOMPARE(disappearingEntryTransport._batchState.completedBytes, 100U);
    QTRY_VERIFY_WITH_TIMEOUT(!disappearingEntryTransport.downloadingLogs(), TestTimeout::shortMs());
    QCOMPARE(disappearingEntryTransport.batchProgress(), 1.);
    QVERIFY(disappearingEntryTransport.errorMessage().contains(QStringLiteral("finalization failed")));

    FtpTransport startupRemovalTransport;
    startupRemovalTransport.setVehicle(vehicle());
    auto* const startupRemovalEntry = new OnboardLogEntry(2, QDateTime(), 100, true, &startupRemovalTransport);
    startupRemovalEntry->setFtpPath(QStringLiteral("/logs/startup-removal.ulg"));
    startupRemovalTransport.model()->append(startupRemovalEntry);
    (void) startupRemovalTransport._downloadSession.begin({startupRemovalEntry}, QStringLiteral("/tmp"));
    QCOMPARE(startupRemovalTransport._downloadSession.takeNext(), startupRemovalEntry);
    startupRemovalTransport._batchState.addFile(startupRemovalEntry->size());
    startupRemovalTransport._setDownloading(true);
    QPointer<OnboardLogEntry> startupRemovalGuard(startupRemovalEntry);
    bool removedDuringDownloadStartup = false;
    (void) connect(startupRemovalEntry, &OnboardLogEntry::stateChanged, &startupRemovalTransport,
                   [&startupRemovalTransport, startupRemovalEntry, &removedDuringDownloadStartup]() {
                       if (startupRemovalEntry->state() == OnboardLogEntry::State::Downloading) {
                           removedDuringDownloadStartup =
                               startupRemovalTransport.model()->removeOne(startupRemovalEntry) == startupRemovalEntry;
                       }
                   });

    startupRemovalTransport._downloadEntry(startupRemovalEntry);

    QVERIFY(removedDuringDownloadStartup);
    QVERIFY(startupRemovalGuard);
    delete startupRemovalEntry;
    QVERIFY(!startupRemovalGuard);
    QCOMPARE(startupRemovalTransport._batchState.completedBytes, 100U);
    QTRY_VERIFY_WITH_TIMEOUT(!startupRemovalTransport.downloadingLogs(), TestTimeout::shortMs());
    QCOMPARE(startupRemovalTransport.batchProgress(), 1.);

    FtpTransport completionRemovalTransport;
    completionRemovalTransport.setVehicle(vehicle());
    auto* const completionRemovalEntry = new OnboardLogEntry(3, QDateTime(), 100, true, &completionRemovalTransport);
    completionRemovalTransport.model()->append(completionRemovalEntry);
    (void) completionRemovalTransport._downloadSession.begin({completionRemovalEntry}, QStringLiteral("/tmp"));
    QCOMPARE(completionRemovalTransport._downloadSession.takeNext(), completionRemovalEntry);
    completionRemovalTransport._batchState.addFile(completionRemovalEntry->size());
    completionRemovalTransport._setDownloading(true);
    completionRemovalTransport._downloadSession.setInFlight(true);
    QPointer<OnboardLogEntry> completionRemovalGuard(completionRemovalEntry);
    bool removedDuringDownloadCompletion = false;
    (void) connect(completionRemovalEntry, &OnboardLogEntry::stateChanged, &completionRemovalTransport,
                   [&completionRemovalTransport, completionRemovalEntry, &removedDuringDownloadCompletion]() {
                       if (completionRemovalEntry->state() == OnboardLogEntry::State::Downloaded) {
                           removedDuringDownloadCompletion = completionRemovalTransport.model()->removeOne(
                                                                 completionRemovalEntry) == completionRemovalEntry;
                       }
                   });

    completionRemovalTransport._downloadComplete(QStringLiteral("completed.ulg"), QString());

    QVERIFY(removedDuringDownloadCompletion);
    QVERIFY(completionRemovalGuard);
    delete completionRemovalEntry;
    QVERIFY(!completionRemovalGuard);
    QCOMPARE(completionRemovalTransport._batchState.completedBytes, 100U);
    QTRY_VERIFY_WITH_TIMEOUT(!completionRemovalTransport.downloadingLogs(), TestTimeout::shortMs());
    QCOMPARE(completionRemovalTransport.batchProgress(), 1.);
}

void OnboardLogDownloadTest::_ftpDeferredEraseGenerationTest()
{
    FtpTransport transport;
    OnboardLogEntry* const activeEntry = new OnboardLogEntry(0, QDateTime(), 100, true, &transport);
    OnboardLogEntry* const queuedEntry = new OnboardLogEntry(1, QDateTime(), 100, true, &transport);
    transport.model()->append({activeEntry, queuedEntry});

    (void) transport._eraseSession.begin({queuedEntry});
    transport._scheduleEraseNext();

    transport._cancelErase();
    (void) transport._eraseSession.begin({activeEntry, queuedEntry});
    QCOMPARE(transport._eraseSession.takeNext(), activeEntry);
    transport._scheduleEraseNext();

    QCoreApplication::processEvents();
    QCOMPARE(transport._eraseSession.currentEntry(), activeEntry);
    QCOMPARE(transport._eraseSession.pendingCount(), 1);

    transport._eraseSession.clear();

    FtpTransport disappearingEntryTransport;
    disappearingEntryTransport.setVehicle(vehicle());
    OnboardLogEntry* const disappearingEntry =
        new OnboardLogEntry(2, QDateTime(), 100, true, &disappearingEntryTransport);
    disappearingEntry->setFtpPath(QStringLiteral("/logs/disappearing.ulg"));
    disappearingEntryTransport.model()->append(disappearingEntry);
    (void) disappearingEntryTransport._eraseSession.begin({disappearingEntry});
    QCOMPARE(disappearingEntryTransport._eraseSession.takeNext(), disappearingEntry);
    QVERIFY(disappearingEntryTransport._eraseSession.hasCurrent());

    QCOMPARE(disappearingEntryTransport.model()->removeOne(disappearingEntry), disappearingEntry);
    delete disappearingEntry;
    QVERIFY(!disappearingEntryTransport._eraseSession.currentEntry());
    QVERIFY(disappearingEntryTransport._eraseSession.hasCurrent());

    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("outside the active model")));
    disappearingEntryTransport._deleteComplete(QStringLiteral("/logs/disappearing.ulg"), QString());
    verifyExpectedLogMessage();
    QTRY_VERIFY_WITH_TIMEOUT(!disappearingEntryTransport._eraseSession.active(), TestTimeout::shortMs());
    QVERIFY(!disappearingEntryTransport._eraseSession.hasCurrent());

    FtpTransport startupRemovalTransport;
    startupRemovalTransport.setVehicle(vehicle());
    OnboardLogEntry* const startupRemovalEntry =
        new OnboardLogEntry(3, QDateTime(), 100, true, &startupRemovalTransport);
    startupRemovalEntry->setFtpPath(QStringLiteral("/logs/startup-removal.ulg"));
    startupRemovalTransport.model()->append(startupRemovalEntry);
    (void) startupRemovalTransport._eraseSession.begin({startupRemovalEntry});

    bool removedDuringStartup = false;
    const QMetaObject::Connection removalConnection =
        connect(startupRemovalEntry, &OnboardLogEntry::stateChanged, &startupRemovalTransport, [&]() {
            if (startupRemovalEntry->state() == OnboardLogEntry::State::Erasing) {
                removedDuringStartup =
                    startupRemovalTransport.model()->removeOne(startupRemovalEntry) == startupRemovalEntry;
            }
        });
    startupRemovalTransport._eraseNext();
    (void) disconnect(removalConnection);

    QVERIFY(removedDuringStartup);
    QTRY_VERIFY_WITH_TIMEOUT(!startupRemovalTransport._eraseSession.active(), TestTimeout::shortMs());
    QVERIFY(!startupRemovalTransport._eraseSession.hasCurrent());
    delete startupRemovalEntry;

    FtpTransport resetTransport;
    QList<OnboardLogEntry*> removedEntries;
    QList<QPointer<OnboardLogEntry>> survivingEntries;
    for (uint id = 0; id < 70; ++id) {
        auto* const entry = new OnboardLogEntry(id, QDateTime(), 100, true, &resetTransport);
        resetTransport.model()->append(entry);
        if ((id % 2U) == 0U) {
            removedEntries.append(entry);
        } else {
            survivingEntries.append(entry);
        }
    }
    (void) resetTransport._eraseSession.begin(survivingEntries);
    QSignalSpy modelResetSpy(resetTransport.model(), &QAbstractItemModel::modelReset);
    const QMetaObject::Connection resetConnection = connect(
        resetTransport.model(), &QAbstractItemModel::rowsAboutToBeInserted, &resetTransport, [&removedEntries]() {
            for (OnboardLogEntry* const entry : std::as_const(removedEntries)) {
                delete entry;
            }
        });
    resetTransport.model()->append(new OnboardLogEntry(70, QDateTime(), 100, true, &resetTransport));
    (void) disconnect(resetConnection);

    QCOMPARE(modelResetSpy.size(), 1);
    for (const QPointer<OnboardLogEntry>& entry : std::as_const(survivingEntries)) {
        QVERIFY(resetTransport._eraseEntryIsCurrent(entry));
    }
}

void OnboardLogDownloadTest::_ftpStaleDownloadCompletionTest()
{
    MockLinkFTP* const mockFtp = _mockLink->mockLinkFTP();
    mockFtp->setErrorMode(MockLinkFTP::errModeNoResponse);

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    FtpTransport transport;
    transport.setVehicle(vehicle());
    auto* const oldEntry = new OnboardLogEntry(0, QDateTime(), 4096, true, &transport);
    oldEntry->setFtpPath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(4096));
    oldEntry->setSelected(true);
    transport.model()->append(oldEntry);
    transport.download(downloadDir.path());
    QPointer<FTPDownloadJob> oldJob(transport._downloadJob);
    QVERIFY(oldJob);

    transport.setVehicle(nullptr);
    QVERIFY(oldJob);
    transport.setVehicle(vehicle());

    auto* const currentEntry = new OnboardLogEntry(1, QDateTime(), 4096, true, &transport);
    currentEntry->setFtpPath(QStringLiteral("%1%2").arg(MockLinkFTP::sizeFilenamePrefix).arg(4096));
    currentEntry->setSelected(true);
    transport.model()->append(currentEntry);
    transport.download(downloadDir.path());
    QPointer<FTPDownloadJob> currentJob(transport._downloadJob);
    QVERIFY(currentJob);
    QVERIFY(currentJob != oldJob);

    oldJob->finished(QStringLiteral("/tmp/stale.ulg"), QString(), QStringLiteral("stale warning"));
    QCOMPARE(transport._downloadJob, currentJob);
    QCOMPARE(transport._downloadSession.currentEntry(), currentEntry);
    QVERIFY(transport._downloadSession.inFlight());
    QVERIFY(transport.downloadingLogs());
    QVERIFY(transport.errorMessage().isEmpty());

    transport.cancel();
    QVERIFY(!transport.downloadingLogs());
    mockFtp->setErrorMode(MockLinkFTP::errModeNone);
}
