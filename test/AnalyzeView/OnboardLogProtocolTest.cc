#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPersistentModelIndex>
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
#include "FTPManager.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#include "FtpDownloadSession.h"
#include "FtpListingParser.h"
#include "FtpListingSession.h"
#include "FtpTransport.h"
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

void OnboardLogDownloadTest::_duplicateLogDataTest()
{
    constexpr uint kPacketSize = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    const std::array<uint8_t, kPacketSize> packetData = {};
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    LogProtocolTransport transport;
    transport.setVehicle(nullptr);
    OnboardLogEntry* const entry = new OnboardLogEntry(40, QDateTime(), kPacketSize * 2, true, &transport);
    transport.model()->append(entry);
    (void) transport._listingSession.begin(true);
    (void) transport._downloadBatch.begin({entry}, downloadDir.path() + QDir::separator());
    LogProtocolDownloadSession* const download = transport._downloadBatch.startNext();
    QVERIFY(download);
    QCOMPARE(download->openFile(downloadDir.filePath(QStringLiteral("duplicate.bin"))),
             LogProtocolDownloadSession::FileOpenResult::Success);
    transport._setDownloading(true);
    transport._batchState.addFile(entry->size());

    transport._logData(0, 40, kPacketSize, packetData.data());
    QCOMPARE(download->bytesWritten(), kPacketSize);
    QCOMPARE(download->receivedBinCount(), 1U);
    transport._updateBatchProgress(download->bytesWritten());
    QCOMPARE(transport.batchProgress(), 0.5);

    QVERIFY(transport._downloadBatch.tryConsumeRetry(LogProtocolTransport::kMaxDownloadRetries));
    QVERIFY(transport._downloadBatch.tryConsumeRetry(LogProtocolTransport::kMaxDownloadRetries));
    QVERIFY(transport._downloadBatch.tryConsumeRetry(LogProtocolTransport::kMaxDownloadRetries));
    transport._logData(0, 40, kPacketSize, packetData.data());
    QCOMPARE(download->bytesWritten(), kPacketSize);
    QCOMPARE(download->receivedBinCount(), 1U);
    QCOMPARE(transport._downloadBatch.retryCount(), 3);
    transport._updateBatchProgress(download->bytesWritten());
    QCOMPARE(transport.batchProgress(), 0.5);
    transport._timer->stop();
    transport._removePartialDownload();
    transport._downloadBatch.clear();
    transport._setDownloading(false);
}

void OnboardLogDownloadTest::_duplicateLogEntryTest()
{
    LogProtocolTransport transport;
    transport.setVehicle(vehicle());
    transport.model()->clearAndDeleteContents();
    const FirmwarePlugin::OnboardLogPolicy policy = vehicle()->firmwarePlugin()->onboardLogPolicy(vehicle());
    (void) transport._listingSession.begin(policy.logProtocolIdsAreOneBased);
    transport._setListing(true);
    const uint16_t firstLogId = policy.logProtocolIdsAreOneBased ? 1 : 0;

    transport._logEntry(100, 100, firstLogId, 2, firstLogId + 1);
    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QVERIFY(transport._listingSession.tryConsumeRetry());
    QVERIFY(transport._listingSession.tryConsumeRetry());
    QVERIFY(!transport._listingSession.tryConsumeRetry());

    transport._logEntry(100, 100, firstLogId, 2, firstLogId + 1);
    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QCOMPARE(transport._listingSession.retryCount(), 2);
    const OnboardLogEntry* const acceptedEntry = transport.model()->value<OnboardLogEntry*>(0);
    QVERIFY(acceptedEntry);
    QCOMPARE(acceptedEntry->size(), 100U);
    QCOMPARE(acceptedEntry->time(), QDateTime::fromSecsSinceEpoch(100));

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("conflicting metadata for onboard log")));
    transport._logEntry(200, 200, firstLogId, 2, firstLogId + 1);
    verifyExpectedLogMessage();
    QCOMPARE(acceptedEntry->size(), 100U);
    QCOMPARE(acceptedEntry->time(), QDateTime::fromSecsSinceEpoch(100));
    QVERIFY(transport._listingSession.partial());
    QVERIFY(!transport.errorMessage().isEmpty());
    transport.cancel();

    LogProtocolTransport rangedTransport;
    rangedTransport._setListing(true);
    (void) rangedTransport._listingSession.begin(false);
    rangedTransport._logEntry(100, 100, 40, 2, 41);
    QCOMPARE(rangedTransport.model()->count(), 2);
    QCOMPARE(rangedTransport.model()->value<OnboardLogEntry*>(0)->id(), 40U);
    QCOMPARE(rangedTransport.model()->value<OnboardLogEntry*>(1)->id(), 41U);
    const std::optional<LogProtocolListingSession::MissingRange> rangedMissing =
        rangedTransport._listingSession.firstMissingRange();
    QVERIFY(rangedMissing.has_value());
    QCOMPARE(rangedMissing->start, 41U);
    QCOMPARE(rangedMissing->end, 41U);
    rangedTransport._logEntry(200, 100, 41, 2, 41);
    QVERIFY(!rangedTransport.requestingList());

    LogProtocolTransport reentrantTransport;
    reentrantTransport.setVehicle(vehicle());
    bool listingRestarted = false;
    const QMetaObject::Connection reentrantConnection =
        connect(reentrantTransport.model(), &QAbstractItemModel::rowsInserted, &reentrantTransport,
                [&reentrantTransport, &listingRestarted]() {
                    if (!listingRestarted) {
                        listingRestarted = true;
                        reentrantTransport.refresh();
                    }
                });

    (void) reentrantTransport._listingSession.begin(policy.logProtocolIdsAreOneBased);
    reentrantTransport._setListing(true);
    reentrantTransport._logEntry(200, 100, firstLogId, 2, firstLogId + 1);
    QVERIFY(listingRestarted);
    QVERIFY(reentrantTransport.requestingList());
    QCOMPARE(reentrantTransport._listingSession.receivedEntryCount(), 0);
    reentrantTransport.cancel();
    (void) disconnect(reentrantConnection);
}

void OnboardLogDownloadTest::_approximateLogSizeTest()
{
    constexpr uint32_t packetSize = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    const std::array<uint8_t, packetSize> packetData = {};
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());

    OnboardLogEntry underestimatedEntry(7, QDateTime(), 10, true);
    LogProtocolDownloadSession underestimatedDownload(&underestimatedEntry);
    const QString underestimatedPath = downloadDir.filePath(QStringLiteral("underestimated.bin"));
    QCOMPARE(underestimatedDownload.openFile(underestimatedPath), LogProtocolDownloadSession::FileOpenResult::Success);
    QCOMPARE(underestimatedDownload.acceptPacket(0, packetSize, packetData.data()).result,
             LogProtocolDownloadSession::PacketResult::Accepted);
    constexpr uint8_t tailSize = 17;
    QCOMPARE(underestimatedDownload.acceptPacket(packetSize, tailSize, packetData.data()).result,
             LogProtocolDownloadSession::PacketResult::Accepted);
    QVERIFY(underestimatedDownload.logComplete());
    QCOMPARE(underestimatedDownload.actualSize(), packetSize + tailSize);
    QVERIFY(underestimatedDownload.commit());
    QCOMPARE(QFileInfo(underestimatedPath).size(), static_cast<qint64>(packetSize + tailSize));

    OnboardLogEntry overestimatedEntry(8, QDateTime(), 1000, true);
    LogProtocolDownloadSession overestimatedDownload(&overestimatedEntry);
    const QString overestimatedPath = downloadDir.filePath(QStringLiteral("overestimated.bin"));
    QCOMPARE(overestimatedDownload.openFile(overestimatedPath), LogProtocolDownloadSession::FileOpenResult::Success);
    QCOMPARE(overestimatedDownload.acceptPacket(0, tailSize, packetData.data()).result,
             LogProtocolDownloadSession::PacketResult::Accepted);
    QVERIFY(overestimatedDownload.logComplete());
    QCOMPARE(overestimatedDownload.actualSize(), static_cast<uint32_t>(tailSize));
    QVERIFY(overestimatedDownload.commit());
    QCOMPARE(QFileInfo(overestimatedPath).size(), static_cast<qint64>(tailSize));

    OnboardLogEntry emptyEntry(9, QDateTime(), 1000, true);
    LogProtocolDownloadSession emptyDownload(&emptyEntry);
    const QString emptyPath = downloadDir.filePath(QStringLiteral("empty.bin"));
    QCOMPARE(emptyDownload.openFile(emptyPath), LogProtocolDownloadSession::FileOpenResult::Success);
    QCOMPARE(emptyDownload.acceptPacket(0, 0, nullptr).result, LogProtocolDownloadSession::PacketResult::Accepted);
    QVERIFY(emptyDownload.logComplete());
    QCOMPARE(emptyDownload.actualSize(), 0U);
    QVERIFY(emptyDownload.commit());
    QVERIFY(QFileInfo(emptyPath).isFile());
    QCOMPARE(QFileInfo(emptyPath).size(), 0);

    OnboardLogEntry invalidPayloadEntry(10, QDateTime(), 100, true);
    LogProtocolDownloadSession invalidPayloadDownload(&invalidPayloadEntry);
    QCOMPARE(invalidPayloadDownload.acceptPacket(0, 1, nullptr).result,
             LogProtocolDownloadSession::PacketResult::InvalidPayload);

    OnboardLogEntry sizeLimitedEntry(11, QDateTime(), 0, true);
    LogProtocolDownloadSession sizeLimitedDownload(&sizeLimitedEntry);
    constexpr uint32_t sizeLimit = LogProtocolDownloadSession::kMaximumAdvertisedSizeOverrun;
    const uint32_t limitPacketOffset = (sizeLimit / packetSize) * packetSize;
    sizeLimitedDownload._currentChunk = limitPacketOffset / LogProtocolDownloadSession::kChunkSize;
    QCOMPARE(sizeLimitedDownload.acceptPacket(limitPacketOffset, packetSize, packetData.data()).result,
             LogProtocolDownloadSession::PacketResult::SizeLimitExceeded);

    OnboardLogEntry boundaryEntry(12, QDateTime(), (std::numeric_limits<uint32_t>::max)(), true);
    LogProtocolDownloadSession boundaryDownload(&boundaryEntry);
    boundaryDownload._currentChunk = (std::numeric_limits<uint32_t>::max)() / LogProtocolDownloadSession::kChunkSize;
    boundaryDownload._resetChunk();
    const uint32_t boundaryOffset = boundaryDownload.currentChunkOffset();
    boundaryDownload._endOffset = boundaryOffset + tailSize;
    boundaryDownload._chunkTable.setBit(0);
    boundaryDownload._receivedBinCount = 1;
    const LogProtocolDownloadSession::CompletionState boundaryCompletion = boundaryDownload.completionState();
    QVERIFY(boundaryCompletion.chunkComplete);
    QVERIFY(boundaryCompletion.logComplete);
    QCOMPARE(boundaryDownload.currentChunkRequestBytes(), (std::numeric_limits<uint32_t>::max)() - boundaryOffset);
    boundaryDownload._endOffset = std::nullopt;
    boundaryDownload._chunkTable.fill(false);
    boundaryDownload._receivedBinCount = 0;
    const std::optional<LogProtocolDownloadSession::MissingRange> boundaryMissingRange =
        boundaryDownload.firstMissingRange();
    QVERIFY(boundaryMissingRange.has_value());
    QCOMPARE(boundaryMissingRange->offset, boundaryOffset);
    QCOMPARE(boundaryMissingRange->count, (std::numeric_limits<uint32_t>::max)() - boundaryOffset);

    OnboardLogEntry collisionEntry(13, QDateTime(), 100, true);
    LogProtocolDownloadSession collisionDownload(&collisionEntry);
    const QString collisionPath = downloadDir.filePath(QStringLiteral("collision.bin"));
    QCOMPARE(collisionDownload.openFile(collisionPath), LogProtocolDownloadSession::FileOpenResult::Success);
    QCOMPARE(collisionDownload.acceptPacket(0, tailSize, packetData.data()).result,
             LogProtocolDownloadSession::PacketResult::Accepted);
    QFile competingFile(collisionPath);
    QVERIFY(competingFile.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    const QByteArray competingContents("competing file");
    QCOMPARE(competingFile.write(competingContents), competingContents.size());
    competingFile.close();
    QVERIFY(!collisionDownload.commit());
    QVERIFY(competingFile.open(QIODevice::ReadOnly));
    QCOMPARE(competingFile.readAll(), competingContents);
    competingFile.close();
    collisionDownload.cancelWriting();

    const uint invalidLogId = static_cast<uint>((std::numeric_limits<uint16_t>::max)()) + 1U;
    OnboardLogEntry invalidIdEntry(invalidLogId, QDateTime(), 100, true);
    LogProtocolDownloadSession invalidIdDownload(&invalidIdEntry);
    QVERIFY(!invalidIdDownload.hasValidLogId());
    const QString invalidIdPath = downloadDir.filePath(QStringLiteral("invalid-id.bin"));
    QCOMPARE(invalidIdDownload.openFile(invalidIdPath), LogProtocolDownloadSession::FileOpenResult::InvalidLogId);
    QVERIFY(!QFileInfo::exists(invalidIdPath));
}

void OnboardLogDownloadTest::_emptyArduPilotLogEntryTest()
{
    LogProtocolTransport transport;
    transport.setVehicle(vehicle());
    transport.model()->clearAndDeleteContents();
    (void) transport._listingSession.begin(true);
    transport._setListing(true);
    QSignalSpy listingFinishedSpy(&transport, &LogProtocolTransport::listingFinished);

    transport._logEntry(100, 0, 1, 2, 2);

    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QVERIFY(transport._listingSession.firstMissingRange().has_value());
    QVERIFY(transport.requestingList());
    QCOMPARE(listingFinishedSpy.size(), 0);

    transport._logEntry(100, 0, 1, 2, 2);
    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QVERIFY(!transport._listingSession.partial());
    QVERIFY(transport.errorMessage().isEmpty());

    transport._logEntry(200, 100, 2, 2, 2);
    QVERIFY(!transport.requestingList());
    QCOMPARE(listingFinishedSpy.size(), 1);
    QCOMPARE(listingFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Success);

    QCOMPARE(transport.model()->count(), 2);
    const OnboardLogEntry* const entry = transport.model()->value<OnboardLogEntry*>(0);
    QVERIFY(entry);
    QVERIFY(!entry->received());
    QVERIFY(!entry->selected());
    QCOMPARE(entry->state(), OnboardLogEntry::State::Skipped);
    QCOMPARE(entry->status(), QStringLiteral("Empty"));
    QVERIFY(entry->errorMessage().isEmpty());
    QCOMPARE(entry->time(), QDateTime::fromSecsSinceEpoch(100));
}

void OnboardLogDownloadTest::_inconsistentLogCountTest()
{
    LogProtocolTransport emptyTransport;
    (void) emptyTransport._listingSession.begin(false);
    emptyTransport._setListing(true);
    QSignalSpy emptyListingFinishedSpy(&emptyTransport, &LogProtocolTransport::listingFinished);
    emptyTransport._logEntry(0, 0, 0, 0, 0);
    QVERIFY(!emptyTransport.requestingList());
    QCOMPARE(emptyListingFinishedSpy.size(), 1);
    QCOMPARE(emptyListingFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Success);

    LogProtocolTransport transport;
    (void) transport._listingSession.begin(false);
    transport._setListing(true);
    QSignalSpy listingFinishedSpy(&transport, &LogProtocolTransport::listingFinished);

    transport._logEntry(100, 100, 0, 2, 1);
    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QVERIFY(transport.requestingList());

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("changed the onboard log count from 2 to 0")));
    transport._logEntry(0, 0, 0, 0, 0);
    verifyExpectedLogMessage();

    QVERIFY(transport.requestingList());
    QCOMPARE(listingFinishedSpy.size(), 0);
    QCOMPARE(transport._listingSession.receivedEntryCount(), 1);
    QVERIFY(transport._listingSession.partial());

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("changed the onboard log count from 2 to 3")));
    transport._logEntry(200, 100, 1, 3, 2);
    verifyExpectedLogMessage();
    QVERIFY(!transport.requestingList());
    QCOMPARE(listingFinishedSpy.size(), 1);
    QCOMPARE(listingFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Partial);
}

void OnboardLogDownloadTest::_sessionStateTest()
{
    LogProtocolListingSession listingSession;
    const quint64 listingGeneration = listingSession.begin(true);
    const LogProtocolListingSession::Response response = listingSession.observeResponse(3, 3, 1);
    QVERIFY(response.firstResponse);
    QCOMPARE(response.entryCount, 3);
    QVERIFY(!response.truncated);
    QCOMPARE(listingSession.firstLogId(), 1);

    const LogProtocolListingSession::EntryAcceptance firstEntry = listingSession.acceptEntry(1, 100);
    QCOMPARE(firstEntry.result, LogProtocolListingSession::EntryResult::Accepted);
    QCOMPARE(firstEntry.modelIndex, 0);
    QVERIFY(!firstEntry.complete);

    const LogProtocolListingSession::EntryAcceptance duplicateEntry = listingSession.acceptEntry(1, 100);
    QCOMPARE(duplicateEntry.result, LogProtocolListingSession::EntryResult::Duplicate);
    QCOMPARE(listingSession.receivedEntryCount(), 1);

    QVERIFY(listingSession.tryConsumeRetry());
    QVERIFY(listingSession.tryConsumeRetry());
    QVERIFY(!listingSession.tryConsumeRetry());
    QCOMPARE(listingSession.retryCount(), 2);
    listingSession.resetRetries();

    const std::optional<LogProtocolListingSession::MissingRange> missingRange = listingSession.firstMissingRange();
    QVERIFY(missingRange.has_value());
    QCOMPARE(missingRange->start, 2U);
    QCOMPARE(missingRange->end, 3U);

    QVERIFY(!listingSession.acceptEntry(3, 100).complete);
    QVERIFY(listingSession.acceptEntry(2, 100).complete);
    QVERIFY(!listingSession.firstMissingRange().has_value());
    listingSession.finish();
    QVERIFY(!listingSession.active());

    LogProtocolListingSession emptyListingSession;
    (void) emptyListingSession.begin(true);
    emptyListingSession.observeResponse(1, 1, 1);
    const LogProtocolListingSession::EntryAcceptance emptyEntry = emptyListingSession.acceptEntry(1, 0);
    QCOMPARE(emptyEntry.result, LogProtocolListingSession::EntryResult::IgnoredEmptyArduPilotLog);
    QCOMPARE(emptyEntry.modelIndex, 0);
    QVERIFY(emptyEntry.complete);
    QCOMPARE(emptyListingSession.receivedEntryCount(), 1);
    QVERIFY(!emptyListingSession.firstMissingRange().has_value());
    QCOMPARE(listingSession.generation(), listingGeneration);

    (void) listingSession.begin();
    const LogProtocolListingSession::Response truncatedResponse = listingSession.observeResponse(
        LogProtocolListingSession::kMaxLogEntries + 1, LogProtocolListingSession::kMaxLogEntries, 0);
    QVERIFY(truncatedResponse.truncated);
    QVERIFY(listingSession.partial());
    const quint64 truncatedGeneration = listingSession.generation();
    QVERIFY(listingSession.cancel());
    QVERIFY(listingSession.generation() > truncatedGeneration);

    OnboardLogEntry firstEraseEntry(0);
    OnboardLogEntry secondEraseEntry(1);
    FtpEraseSession eraseSession;
    (void) eraseSession.begin({&firstEraseEntry, &secondEraseEntry});
    QCOMPARE(eraseSession.takeNext(), &firstEraseEntry);
    eraseSession.completeCurrent(true);
    QCOMPARE(eraseSession.takeNext(), &secondEraseEntry);
    QCOMPARE(eraseSession.finish(), 1U);
    QVERIFY(!eraseSession.active());

    (void) eraseSession.begin({&firstEraseEntry, &secondEraseEntry});
    QCOMPARE(eraseSession.takeNext(), &firstEraseEntry);
    const FtpEraseSession::Cancellation cancellation = eraseSession.cancel();
    QVERIFY(cancellation.wasActive);
    QCOMPARE(cancellation.currentEntry, &firstEraseEntry);
    QCOMPARE(cancellation.pendingEntries.size(), 1);
    QCOMPARE(cancellation.pendingEntries.constFirst(), &secondEraseEntry);
    QVERIFY(eraseSession.canceling());
    eraseSession.finishCancellation();
    QVERIFY(!eraseSession.active());
    QVERIFY(!eraseSession.canceling());

    FtpListingSession ftpListingSession;
    const quint64 ftpListingGeneration = ftpListingSession.begin(QStringLiteral("/logs"), 0, 3);
    QCOMPARE(ftpListingSession.state(), FtpListingSession::State::ListingRoot);
    QVERIFY(ftpListingSession.isCurrent(ftpListingGeneration));
    QVERIFY(!ftpListingSession.observeDirectoryResult(2, false));
    QCOMPARE(ftpListingSession.remainingEntryBudget(), 1);
    const FtpListingParser::ParseResult ftpListingResult =
        ftpListingSession.parse({QStringLiteral("D2026-07-14"), QStringLiteral("Fflight.ulg\t100")}, QString(), true);
    QCOMPARE(ftpListingResult.logs.size(), 1);
    QCOMPARE(ftpListingResult.directories.size(), 1);
    ftpListingSession.beginSubdirectories();
    QCOMPARE(ftpListingSession.currentDirectory(), QStringLiteral("2026-07-14"));
    QCOMPARE(ftpListingSession.currentDirectoryPath(), QStringLiteral("/logs/2026-07-14"));
    QVERIFY(ftpListingSession.canListNextDirectory());
    ftpListingSession.completeCurrentDirectory();
    QVERIFY(!ftpListingSession.hasPendingDirectories());
    ftpListingSession.finish();
    QVERIFY(!ftpListingSession.active());

    (void) ftpListingSession.begin(QStringLiteral("@MAV_LOG"), 42);
    QVERIFY(ftpListingSession.useFallbackRoot(QStringLiteral("/APM/LOGS")));
    QCOMPARE(ftpListingSession.root(), QStringLiteral("/APM/LOGS"));
    const FtpListingParser::ParseResult fallbackResult =
        ftpListingSession.parse({QStringLiteral("Ffallback.bin\t100")}, QString(), false);
    QCOMPARE(fallbackResult.logs.size(), 1);
    QCOMPARE(fallbackResult.logs.constFirst().id, 42U);
    QVERIFY(!ftpListingSession.useFallbackRoot(QStringLiteral("/another-root")));
    const quint64 fallbackGeneration = ftpListingSession.generation();
    QVERIFY(ftpListingSession.cancel());
    QVERIFY(ftpListingSession.generation() > fallbackGeneration);
    QVERIFY(ftpListingSession.canceling());
    ftpListingSession.finish();

    OnboardLogEntry firstDownloadEntry(2, QDateTime(), 100, true);
    OnboardLogEntry secondDownloadEntry(3);
    FtpDownloadSession downloadSession;
    const quint64 downloadGeneration =
        downloadSession.begin({&firstDownloadEntry, &secondDownloadEntry}, QStringLiteral("/tmp/"));
    QVERIFY(downloadSession.active());
    QVERIFY(downloadSession.isCurrent(downloadGeneration));
    QCOMPARE(downloadSession.takeNext(), &firstDownloadEntry);
    downloadSession.setInFlight(true);
    const std::optional<FtpDownloadSession::ProgressUpdate> progress =
        downloadSession.updateProgress(0.5, std::chrono::milliseconds::zero());
    QVERIFY(progress.has_value());
    QCOMPARE(progress->bytes, 50U);
    QCOMPARE(progress->progress, 0.5);
    downloadSession.requestRefresh();
    const FtpDownloadSession::Cancellation downloadCancellation = downloadSession.cancel();
    QVERIFY(downloadCancellation.wasActive);
    QVERIFY(downloadCancellation.cancelRemoteDownload);
    QCOMPARE(downloadCancellation.currentEntry, &firstDownloadEntry);
    QVERIFY(downloadSession.canceling());
    QVERIFY(downloadSession.finishCancellation());
    QVERIFY(!downloadSession.active());
    QVERIFY(!downloadSession.canceling());

    OnboardLogEntry firstLogDownloadEntry(4);
    OnboardLogEntry secondLogDownloadEntry(5);
    LogProtocolDownloadBatchSession logDownloadBatch;
    const quint64 logDownloadGeneration = logDownloadBatch.begin({&firstLogDownloadEntry, &secondLogDownloadEntry},
                                                                 QStringLiteral("/tmp/"), QStringLiteral("ulg"));
    QVERIFY(logDownloadBatch.active());
    QVERIFY(logDownloadBatch.isCurrent(logDownloadGeneration));
    QCOMPARE(logDownloadBatch.fileExtension(), QStringLiteral("ulg"));
    QCOMPARE(logDownloadBatch.pendingCount(), 2);
    LogProtocolDownloadSession* const firstLogDownload = logDownloadBatch.startNext();
    QVERIFY(firstLogDownload);
    QCOMPARE(firstLogDownload->entry(), &firstLogDownloadEntry);
    QVERIFY(logDownloadBatch.isCurrent(logDownloadGeneration, &firstLogDownloadEntry));
    QVERIFY(logDownloadBatch.tryConsumeRetry(2));
    QCOMPARE(logDownloadBatch.retryCount(), 1);
    QVERIFY(logDownloadBatch.tryConsumeRetry(2));
    QVERIFY(!logDownloadBatch.tryConsumeRetry(2));
    QCOMPARE(logDownloadBatch.retryCount(), 2);
    logDownloadBatch.resetRetries();
    QCOMPARE(logDownloadBatch.retryCount(), 0);
    logDownloadBatch.completeCurrent();
    LogProtocolDownloadSession* const secondLogDownload = logDownloadBatch.startNext();
    QVERIFY(secondLogDownload);
    QCOMPARE(secondLogDownload->entry(), &secondLogDownloadEntry);
    const LogProtocolDownloadBatchSession::Cancellation logDownloadCancellation = logDownloadBatch.cancel();
    QVERIFY(logDownloadCancellation.wasActive);
    QCOMPARE(logDownloadCancellation.currentEntry, &secondLogDownloadEntry);
    QVERIFY(logDownloadBatch.generation() > logDownloadGeneration);
    QVERIFY(!logDownloadBatch.active());
    QVERIFY(!logDownloadBatch.current());
}

void OnboardLogDownloadTest::_entryStateTest()
{
    OnboardLogEntry entry(42);
    QCOMPARE(entry.state(), OnboardLogEntry::State::Pending);
    QCOMPARE(entry.status(), QStringLiteral("Pending"));

    OnboardLogEntry availableEntry(43, QDateTime(), 1, true);
    QCOMPARE(availableEntry.state(), OnboardLogEntry::State::Available);
    QCOMPARE(availableEntry.status(), QStringLiteral("Available"));

    QSignalSpy stateSpy(&entry, &OnboardLogEntry::stateChanged);
    QSignalSpy statusSpy(&entry, &OnboardLogEntry::statusChanged);

    entry.setState(OnboardLogEntry::State::Downloading, QStringLiteral("Downloading"));
    QCOMPARE(entry.state(), OnboardLogEntry::State::Downloading);
    QCOMPARE(entry.status(), QStringLiteral("Downloading"));
    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(statusSpy.size(), 1);

    entry.setStatus(QStringLiteral("64 KiB (32 KiB/s)"));
    QCOMPARE(entry.state(), OnboardLogEntry::State::Downloading);
    QCOMPARE(entry.status(), QStringLiteral("64 KiB (32 KiB/s)"));
    QCOMPARE(stateSpy.size(), 1);
    QCOMPARE(statusSpy.size(), 2);

    entry.setState(OnboardLogEntry::State::Downloaded, QStringLiteral("Downloaded"));
    QCOMPARE(entry.state(), OnboardLogEntry::State::Downloaded);
    QCOMPARE(entry.status(), QStringLiteral("Downloaded"));
    QCOMPARE(stateSpy.size(), 2);
    QCOMPARE(statusSpy.size(), 3);

    entry.setState(OnboardLogEntry::State::Downloaded, QStringLiteral("Downloaded"));
    QCOMPARE(stateSpy.size(), 2);
    QCOMPARE(statusSpy.size(), 3);
}

void OnboardLogDownloadTest::_remoteBoundsTest()
{
    FtpTransport ftpTransport;
    (void) ftpTransport._listingSession.begin(QStringLiteral("/logs"), FtpListingParser::kMaxLogEntries - 1);

    QCOMPARE(ftpTransport._processFileEntries({QStringLiteral("Ffirst.ulg\t100"), QStringLiteral("Fsecond.ulg\t100")},
                                              QString()),
             1U);
    QCOMPARE(ftpTransport.model()->count(), 1);
    QVERIFY(!ftpTransport.errorMessage().isEmpty());

    ftpTransport._setErrorMessage(QString());
    QCOMPARE(
        ftpTransport._processFileEntries(
            {QStringLiteral("Dsubdir"), QStringLiteral("Fnotes.txt\t100"), QStringLiteral("malformed")}, QString()),
        0U);
    QVERIFY(ftpTransport.errorMessage().isEmpty());

    ftpTransport._listDirComplete({}, QString(), true);
    QVERIFY(!ftpTransport.errorMessage().isEmpty());

    const int entryCountAfterListing = ftpTransport.model()->count();
    const QString listingError = ftpTransport.errorMessage();
    ftpTransport._listDirComplete({QStringLiteral("Funexpected.ulg\t100")}, QString(), false);
    QCOMPARE(ftpTransport.model()->count(), entryCountAfterListing);
    QCOMPARE(ftpTransport.errorMessage(), listingError);

    FtpTransport malformedTransport;
    (void) malformedTransport._listingSession.begin(QStringLiteral("/logs"));
    expectLogMessage("AnalyzeView.OnboardLogs.FtpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("malformed onboard log entry")));
    QCOMPARE(malformedTransport._processFileEntries({QStringLiteral("Fbroken.ulg\tnot-a-size")}, QString()), 0U);
    verifyExpectedLogMessage();
    QVERIFY(malformedTransport._listingSession.partial());
    QVERIFY(!malformedTransport.errorMessage().isEmpty());

    FtpListingParser duplicateParser;
    duplicateParser.reset(QStringLiteral("/logs"));
    const FtpListingParser::ParseResult duplicateResult =
        duplicateParser.parse({QStringLiteral("D2026-07-14"), QStringLiteral("D2026-07-14"),
                               QStringLiteral("Fflight.ulg\t100"), QStringLiteral("Fflight.ulg\t100")},
                              QString(), true);
    QCOMPARE(duplicateResult.directories.size(), 1);
    QCOMPARE(duplicateResult.logs.size(), 1);
    QCOMPARE(duplicateResult.duplicateDirectories, 1U);
    QCOMPARE(duplicateResult.duplicateLogs, 1U);

    FtpTransport exactBudgetTransport;
    QSignalSpy exactBudgetFinishedSpy(&exactBudgetTransport, &FtpTransport::listingFinished);
    (void) exactBudgetTransport._listingSession.begin(QStringLiteral("/logs"), 0, 1);
    exactBudgetTransport._listDirComplete({QStringLiteral("Fflight.ulg\t100")}, QString(), false);
    QCOMPARE(exactBudgetTransport._listingSession.remainingEntryBudget(), 0);
    QVERIFY(!exactBudgetTransport._listingSession.active());
    QVERIFY(exactBudgetTransport.errorMessage().isEmpty());
    QCOMPARE(exactBudgetTransport.model()->count(), 1);
    QCOMPARE(exactBudgetFinishedSpy.size(), 1);
    QCOMPARE(exactBudgetFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Success);

    FtpTransport scanBudgetTransport;
    QSignalSpy listingFinishedSpy(&scanBudgetTransport, &FtpTransport::listingFinished);
    (void) scanBudgetTransport._listingSession.begin(QStringLiteral("/logs"), 0, 1);
    scanBudgetTransport._listDirComplete({QStringLiteral("D2026-07-14")}, QString(), false);
    QCOMPARE(scanBudgetTransport._listingSession.remainingEntryBudget(), 0);
    QVERIFY(!scanBudgetTransport._listingSession.active());
    QVERIFY(!scanBudgetTransport.errorMessage().isEmpty());
    QCOMPARE(listingFinishedSpy.size(), 1);
    QCOMPARE(listingFinishedSpy.takeFirst().at(0).value<OnboardLogTransport::ListingResult>(),
             OnboardLogTransport::ListingResult::Partial);

    LogProtocolTransport logTransport;
    logTransport.setVehicle(vehicle());
    logTransport.model()->clearAndDeleteContents();
    const FirmwarePlugin::OnboardLogPolicy policy = vehicle()->firmwarePlugin()->onboardLogPolicy(vehicle());
    (void) logTransport._listingSession.begin(policy.logProtocolIdsAreOneBased);
    logTransport._setListing(true);
    const uint16_t firstLogId = policy.logProtocolIdsAreOneBased ? 1 : 0;
    logTransport._logEntry(100, 100, firstLogId, LogProtocolTransport::kMaxLogEntries + 1, firstLogId);

    QCOMPARE(logTransport.model()->count(), LogProtocolTransport::kMaxLogEntries);
    QVERIFY(!logTransport.errorMessage().isEmpty());
    logTransport.cancel();
}

void OnboardLogDownloadTest::_destructorCleanupTest()
{
    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    const QString partialPath = downloadDir.filePath(QStringLiteral("partial.bin"));

    auto* transport = new LogProtocolTransport;
    transport->setVehicle(nullptr);
    OnboardLogEntry* const entry = new OnboardLogEntry(0, QDateTime(), 128, true, transport);
    (void) transport->_downloadBatch.begin({entry}, downloadDir.path() + QDir::separator());
    LogProtocolDownloadSession* const download = transport->_downloadBatch.startNext();
    QVERIFY(download);
    QCOMPARE(download->openFile(partialPath), LogProtocolDownloadSession::FileOpenResult::Success);
    QVERIFY(!QFile::exists(partialPath));

    delete transport;

    QVERIFY(!QFile::exists(partialPath));
}

void OnboardLogDownloadTest::_logProtocolFailureHandlingTest()
{
    LogProtocolTransport transport;
    transport.setVehicle(vehicle());
    OnboardLogModel* const model = transport.model();
    QVERIFY(model);
    model->clearAndDeleteContents();

    QTemporaryDir downloadDir;
    QVERIFY(downloadDir.isValid());
    const QString downloadPath = downloadDir.path() + QDir::separator();

    OnboardLogEntry* const emptyEntry = new OnboardLogEntry(0, QDateTime(), 0, true, &transport);
    emptyEntry->setSelected(true);
    model->append(emptyEntry);
    (void) transport._downloadBatch.begin({emptyEntry}, downloadPath);
    transport._batchState.addFile(0);
    transport._setDownloading(true);
    QSignalSpy batchedSelectionSpy(&transport, &LogProtocolTransport::selectionChanged);

    QVERIFY(transport._prepareLogDownload());
    QVERIFY(!emptyEntry->selected());
    QCOMPARE(batchedSelectionSpy.size(), 0);
    QCOMPARE(emptyEntry->status(), QStringLiteral("Downloading"));
    transport._logData(0, static_cast<uint16_t>(emptyEntry->id()), 0, nullptr);
    QCOMPARE(batchedSelectionSpy.size(), 1);
    QCOMPARE(emptyEntry->status(), QStringLiteral("Downloaded"));
    QCOMPARE(emptyEntry->state(), OnboardLogEntry::State::Downloaded);
    QVERIFY(emptyEntry->errorMessage().isEmpty());
    QVERIFY(transport.errorMessage().isEmpty());
    QVERIFY(!transport.downloadingLogs());
    QCOMPARE(transport.batchProgress(), 1.);
    const QFileInfo emptyLogFile(downloadDir.filePath(QStringLiteral("log_0_UnknownDate.bin")));
    QVERIFY(emptyLogFile.isFile());
    QCOMPARE(emptyLogFile.size(), 0);

    OnboardLogEntry* const writeEntry = new OnboardLogEntry(1, QDateTime(), 64, true, &transport);
    model->append(writeEntry);
    (void) transport._downloadBatch.begin({writeEntry}, downloadPath);
    LogProtocolDownloadSession* download = transport._downloadBatch.startNext();
    QVERIFY(download);

    const QString canceledPath = downloadDir.filePath(QStringLiteral("canceled-save.bin"));
    QCOMPARE(download->openFile(canceledPath), LogProtocolDownloadSession::FileOpenResult::Success);
    download->cancelWriting();
    transport._setDownloading(true);

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error writing the partial log file")));
    const std::array<uint8_t, 64> packetData = {};
    transport._logData(0, static_cast<uint16_t>(writeEntry->id()), static_cast<uint8_t>(packetData.size()),
                       packetData.data());
    verifyExpectedLogMessage();

    QVERIFY(!transport._downloadBatch.current());
    QVERIFY(!transport._downloadBatch.active());
    QVERIFY(!transport.downloadingLogs());
    QCOMPARE(writeEntry->status(), QStringLiteral("Error"));
    QVERIFY(!QFile::exists(canceledPath));

    OnboardLogEntry* const retryEntry = new OnboardLogEntry(2, QDateTime(), 64, true, &transport);
    model->append(retryEntry);
    OnboardLogEntry* const queuedAfterFailure = new OnboardLogEntry(3, QDateTime(), 64, true, &transport);
    queuedAfterFailure->setSelected(true);
    model->append(queuedAfterFailure);
    (void) transport._downloadBatch.begin({retryEntry, queuedAfterFailure}, downloadPath);
    download = transport._downloadBatch.startNext();
    QVERIFY(download);
    for (int retry = 0; retry < LogProtocolTransport::kMaxDownloadRetries; ++retry) {
        QVERIFY(transport._downloadBatch.tryConsumeRetry(LogProtocolTransport::kMaxDownloadRetries));
    }
    transport._setDownloading(true);

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Too many retries while downloading")));
    transport._findMissingData();
    verifyExpectedLogMessage();

    QVERIFY(!transport._downloadBatch.current());
    QVERIFY(!transport._downloadBatch.active());
    QVERIFY(!transport.downloadingLogs());
    QCOMPARE(retryEntry->status(), QStringLiteral("Error"));
    QCOMPARE(queuedAfterFailure->status(), QStringLiteral("Skipped"));
    QVERIFY(!queuedAfterFailure->errorMessage().isEmpty());

    constexpr uint kFullPacketSize = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    const std::array<uint8_t, kFullPacketSize> shortPacketData = {};
    const std::array<uint8_t, 1> invalidCounts = {kFullPacketSize + 1};
    uint invalidPacketId = 4;
    for (const uint8_t invalidCount : invalidCounts) {
        OnboardLogEntry* const invalidPacketEntry =
            new OnboardLogEntry(invalidPacketId++, QDateTime(), kFullPacketSize * 2, true, &transport);
        model->append(invalidPacketEntry);
        (void) transport._downloadBatch.begin({invalidPacketEntry}, downloadPath);
        download = transport._downloadBatch.startNext();
        QVERIFY(download);
        const QString partialPath = downloadDir.filePath(QStringLiteral("invalid-packet-%1.bin").arg(invalidCount));
        QCOMPARE(download->openFile(partialPath), LogProtocolDownloadSession::FileOpenResult::Success);
        transport._setDownloading(true);

        expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Unexpected onboard log data packet size")));
        transport._logData(0, static_cast<uint16_t>(invalidPacketEntry->id()), invalidCount, shortPacketData.data());
        verifyExpectedLogMessage();

        QVERIFY(!transport._downloadBatch.current());
        QVERIFY(!transport._downloadBatch.active());
        QVERIFY(!transport.downloadingLogs());
        QCOMPARE(invalidPacketEntry->status(), QStringLiteral("Error"));
        QVERIFY(!QFile::exists(partialPath));
    }

    const uint invalidLogId = static_cast<uint>((std::numeric_limits<uint16_t>::max)()) + 1U;
    OnboardLogEntry* const invalidIdEntry = new OnboardLogEntry(invalidLogId, QDateTime(), 64, true, &transport);
    invalidIdEntry->setSelected(true);
    model->append(invalidIdEntry);

    expectLogMessage("AnalyzeView.OnboardLogs.LogProtocolTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("invalid LOG protocol ID")));
    transport.download(downloadDir.path());
    verifyExpectedLogMessage();

    QVERIFY(!transport.downloadingLogs());
    QCOMPARE(invalidIdEntry->state(), OnboardLogEntry::State::Error);
    QVERIFY(!invalidIdEntry->selected());
    QVERIFY(!invalidIdEntry->errorMessage().isEmpty());
}
