#include <QtCore/QEvent>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <atomic>
#include <cstring>
#include <thread>

#include "GPSReceiverSession.h"
#include "GPSReceiverTestProfile.h"
#include "GPSTransport.h"
#include "GpsTestHelpers.h"

namespace {
struct WorkerGate
{
    QSemaphore entered;
    QSemaphore release;
};

GPSProvider::TransportFactory blockedFactory(const std::shared_ptr<WorkerGate>& gate)
{
    return [gate](const std::atomic_bool&) {
        gate->entered.release();
        gate->release.acquire();
        return std::unique_ptr<GPSTransport>();
    };
}
}  // namespace

class GPSReceiverContractTest : public QObject
{
    Q_OBJECT

private slots:
    void _correctionGenerationSurvivesRestart();
    void _stalledConsumerBoundsUpdates();
    void _retirementDiscardsPendingData();
    void _correctionQueueLifecycle();
    void _staleDataIsNotRejuvenated();
    void _correctionsWrittenOnlyOnWorker_data();
    void _correctionsWrittenOnlyOnWorker();
    void _deliveryReportsBoundAdmission();
    void _uncertainDeliveryPreservesCounts();
    void _deliveryOutcomesPreserveProvenance();
    void _configurationReportLifecycle();
    void _configurationReportResetCanRestart();
    void _clearFlushesKnownResults_data();
    void _clearFlushesKnownResults();
};

void GPSReceiverContractTest::_stalledConsumerBoundsUpdates()
{
    GPSReceiverSession session;
    const auto gate = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(gate));
    // Standalone Qt test: no application harness timeout helpers are linked.
    QVERIFY(gate->entered.tryAcquire(1, 5000));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    QSignalSpy positions(&session, &GPSReceiverSession::positionReceived);
    QSignalSpy corrections(&session, &GPSReceiverSession::rtcmFrameReceived);
    std::atomic_int wakeups = 0;
    connect(worker, &GPSProvider::dataReady, worker, [&]() { ++wakeups; }, Qt::DirectConnection);
    constexpr int count = 10000;
    const auto frame = GpsTestHelpers::buildRtcmFrame(1077);
    const qint64 receivedAtMs = GPSObservation::monotonicNowUs() / 1000;
    std::jthread producer([&]() {
        for (int index = 0; index < count; ++index) {
            GPSObservation observation;
            observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
            observation.satellitesUsed = index;
            worker->sensorGpsUpdate(observation);
            worker->RTCMFrameUpdate(frame, receivedAtMs);
        }
    });
    producer.join();
    QCOMPARE(wakeups.load(), 1);
    QCOMPARE(session.deliveryStats().pendingCorrections, GPSReceiverMailbox::MAX_CORRECTIONS);
    QCOMPARE(session.deliveryStats().droppedCorrections, quint64(count - GPSReceiverMailbox::MAX_CORRECTIONS));
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(positions.size(), 1);
    QCOMPARE(positions.first().first().value<GPSObservation>().satellitesUsed.value(), count - 1);
    QCOMPARE(positions.first().first().value<GPSObservation>().sessionId, session.sessionId());
    QCOMPARE(corrections.size(), GPSReceiverMailbox::FRAMES_PER_DRAIN);
    QTRY_COMPARE_WITH_TIMEOUT(corrections.size(), GPSReceiverMailbox::MAX_CORRECTIONS, 5000);
    for (const auto& correction : corrections) {
        QCOMPARE(correction[0].toByteArray(), frame);
        QCOMPARE(correction[1].toLongLong(), receivedAtMs);
        QCOMPARE(correction[2].toULongLong(), session.sessionId());
    }
    worker->sensorGpsUpdate(GPSObservation{});
    QCOMPARE(wakeups.load(), 2);
}

void GPSReceiverContractTest::_retirementDiscardsPendingData()
{
    GPSReceiverSession session;
    const auto first = std::make_shared<WorkerGate>();
    const auto second = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        first->release.release();
        second->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(first));
    QVERIFY(first->entered.tryAcquire(1, 5000));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto mailbox = worker->mailbox();
    GPSObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    worker->sensorGpsUpdate(observation);
    worker->RTCMFrameUpdate(GpsTestHelpers::buildRtcmFrame(1077), observation.monotonicTimestampUs / 1000);
    const quint64 oldSession = session.sessionId();
    GPSCorrectionFrame incoming;
    incoming.source = GPSCorrectionSource::Ntrip;
    incoming.sourceInstance = QStringLiteral("old-caster");
    incoming.session = 5;
    incoming.deliveryId = 40;
    incoming.data = GpsTestHelpers::buildRtcmFrame(1087);
    incoming.receivedAtMs = GPSObservation::monotonicNowUs() / 1000;
    mailbox->setCorrectionsEnabled(true);
    QVERIFY(mailbox->submitCorrection(incoming, oldSession, incoming.receivedAtMs).accepted);
    const auto inFlight = mailbox->takeCommand(incoming.receivedAtMs);
    QVERIFY(inFlight);
    incoming.deliveryId = 41;
    QVERIFY(mailbox->submitCorrection(incoming, oldSession, incoming.receivedAtMs).accepted);
    QSignalSpy deliveryResults(&session, &GPSReceiverSession::correctionDeliveriesReady);
    QSignalSpy positions(&session, &GPSReceiverSession::positionReceived);
    QSignalSpy corrections(&session, &GPSReceiverSession::rtcmFrameReceived);
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(second));
    QVERIFY(second->entered.tryAcquire(1, 5000));
    QVERIFY(!mailbox->completeCommand(*inFlight, GPSCorrectionOutcome::Written, incoming.data.size()));
    emit worker->dataReady();
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(deliveryResults.size(), 1);
    const auto cancelled = deliveryResults.first().first().value<QList<GPSCorrectionDelivery>>();
    QCOMPARE(cancelled.size(), 1);
    QCOMPARE(cancelled.first().deliveryId, quint64(41));
    QCOMPARE(cancelled.first().destinationSession, oldSession);
    QCOMPARE(cancelled.first().outcome, GPSCorrectionOutcome::Cancelled);
    QVERIFY(positions.isEmpty());
    QVERIFY(corrections.isEmpty());
    QVERIFY(!mailbox->publish(observation));
    QCOMPARE(mailbox->stats().pendingCorrections, 0);
    QVERIFY(!session.submitCorrections(GpsTestHelpers::buildRtcmFrame(1077), GPSObservation::monotonicNowUs() / 1000,
                                       oldSession));
}

void GPSReceiverContractTest::_correctionQueueLifecycle()
{
    GPSReceiverMailbox mailbox;
    constexpr qint64 now = 100000;
    const auto frame = GpsTestHelpers::buildRtcmFrame(1077);
    QVERIFY(!mailbox.submitCorrection(frame, now, now));
    mailbox.setCorrectionsEnabled(true);
    QVERIFY(!mailbox.submitCorrection(frame, now + 1, now));
    QVERIFY(!mailbox.submitCorrection(frame, now - GPSReceiverMailbox::MAX_AGE_MS, now));
    QVERIFY(!mailbox.submitCorrection(frame.first(frame.size() - 1), now, now));
    auto invalid = frame;
    invalid.back() = char(invalid.back() ^ 1);
    QVERIFY(!mailbox.submitCorrection(invalid, now, now));
    for (int index = 0; index < GPSReceiverMailbox::MAX_COMMANDS; ++index) {
        QVERIFY(mailbox.submitCorrection(frame, now, now));
    }
    QVERIFY(!mailbox.submitCorrection(frame, now, now));
    QCOMPARE(mailbox.stats().pendingCommands, GPSReceiverMailbox::MAX_COMMANDS);
    QVERIFY(!mailbox.takeCommand(now + GPSReceiverMailbox::MAX_AGE_MS));
    QCOMPARE(mailbox.stats().expiredCommands, quint64(GPSReceiverMailbox::MAX_COMMANDS));
    QVERIFY(mailbox.submitCorrection(frame, now, now));
    const auto command = mailbox.takeCommand(now + 10);
    QVERIFY(command);
    QCOMPARE(command->data, frame);
    QCOMPARE(command->receivedAtMs, now);
    QVERIFY(mailbox.submitCorrection(frame, now, now));
    mailbox.clearCommands();
    QVERIFY(!mailbox.takeCommand(now));
    QVERIFY(mailbox.submitCorrection(frame, now, now));
    mailbox.setCorrectionsEnabled(false);
    QVERIFY(!mailbox.takeCommand(now));
    mailbox.setCorrectionsEnabled(true);
    QVERIFY(mailbox.submitCorrection(frame, now, now));
    mailbox.close();
    mailbox.setCorrectionsEnabled(true);
    QVERIFY(!mailbox.submitCorrection(frame, now, now));
    QVERIFY(!mailbox.takeCommand(now));
}

void GPSReceiverContractTest::_staleDataIsNotRejuvenated()
{
    GPSReceiverMailbox mailbox;
    constexpr qint64 now = 100000;
    GPSObservation position;
    position.monotonicTimestampUs = (now - GPSReceiverMailbox::MAX_AGE_MS) * 1000;
    QVERIFY(mailbox.publish(position));
    QVERIFY(!mailbox.publishCorrection(GpsTestHelpers::buildRtcmFrame(1077), now - GPSReceiverMailbox::MAX_AGE_MS));
    QVERIFY(!mailbox.publishCorrection(GpsTestHelpers::buildRtcmFrame(1087), now + 1));
    auto batch = mailbox.take(now);
    QVERIFY(!batch.position);
    QVERIFY(batch.corrections.empty());
    QVERIFY(!batch.more);
    QCOMPARE(mailbox.stats().droppedCorrections, quint64(2));
    position.monotonicTimestampUs = (now - 1000) * 1000;
    QVERIFY(mailbox.publish(position));
    batch = mailbox.take(now);
    QVERIFY(batch.position);
    QCOMPARE(batch.position->monotonicTimestampUs, position.monotonicTimestampUs);
}

void GPSReceiverContractTest::_correctionsWrittenOnlyOnWorker_data()
{
    QTest::addColumn<bool>("partialWrite");
    QTest::newRow("written") << false;
    QTest::newRow("partial-write-failed") << true;
}

void GPSReceiverContractTest::_correctionsWrittenOnlyOnWorker()
{
    QFETCH(bool, partialWrite);

    struct Trace
    {
        QSemaphore correctionWritten;
        QSemaphore heldRead;
        QSemaphore releaseRead;
        std::atomic_bool holdRead = false;
        std::atomic<QThread*> writtenOn = nullptr;
        QByteArray written;
        bool partialWrite = false;
        std::atomic<qint64> writeBudgetMs = 0;
        std::atomic_int correctionCount = 0;
        std::atomic_bool readAfterFirstCorrection = false;
        std::atomic_bool receivedBetweenCorrections = false;
    };

    class ReceiverTransport : public GPSTransport
    {
    public:
        ReceiverTransport(const std::atomic_bool& stop, Trace& trace)
            : GPSTransport(stop)
            , _trace(trace)
        {}

        OpenResult open() override { return {OpenStatus::Opened}; }

        bool fatalError() const override { return false; }

        bool setBaudrate(unsigned) override { return !isCancelled(); }

        unsigned fixedBaudrate() const override { return 115200; }

        ReadResult read(uint8_t* data, int size, int) override
        {
            if (_trace.correctionCount == 1) {
                _trace.readAfterFirstCorrection = true;
            }
            if (_trace.holdRead.exchange(false)) {
                _trace.heldRead.release();
                _trace.releaseRead.acquire();
            }
            if (isCancelled()) {
                return {ReadStatus::Cancelled};
            }
            const int count = qMin(size, int(_reply.size()));
            std::memcpy(data, _reply.constData(), count);
            _reply.remove(0, count);
            // Emulate an idle blocking device between ACK chunks.
            if (count == 0) {
                QSemaphore idle;
                idle.tryAcquire(1, 1);
            }
            return {count > 0 ? ReadStatus::Data : ReadStatus::TimedOut, count};
        }

        WriteResult write(const uint8_t* data, int size) override
        {
            const QByteArray command(reinterpret_cast<const char*>(data), size);
            if (command.startsWith(char(0xd3))) {
                const int written = _trace.partialWrite ? size / 2 : size;
                if (_trace.correctionCount == 1) {
                    _trace.written = command.first(written);
                    _trace.writtenOn = QThread::currentThread();
                    _trace.correctionWritten.release();
                }
                return {WriteStatus::Completed, size, written, size - written};
            } else {
                _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
            }
            return {WriteStatus::Completed, size, size};
        }

        WriteResult writeBounded(const uint8_t* data, int size, QDeadlineTimer deadline) override
        {
            if (size <= 0 || data[0] != 0xd3) {
                return write(data, size);
            }
            _trace.writeBudgetMs = deadline.remainingTime();
            if (++_trace.correctionCount == 2) {
                _trace.receivedBetweenCorrections = _trace.readAfterFirstCorrection.load();
            }
            if (!_trace.partialWrite) {
                // Model a successful slow drain consuming the caller's full shared budget.
                QSemaphore drain;
                drain.tryAcquire(1, static_cast<int>(deadline.remainingTime()));
            }
            const int count = write(data, size).writtenBytes;
            return {_trace.partialWrite ? WriteStatus::Error : WriteStatus::Completed, size, count, size - count};
        }

    private:
        Trace& _trace;
        QByteArray _reply;
    };

    Trace trace;
    trace.partialWrite = partialWrite;
    GPSReceiverSession session;
    QSignalSpy deliveries(&session, &GPSReceiverSession::correctionDeliveriesReady);
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        trace.releaseRead.release();
        session.shutdown();
    });
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    session.start(gpsReceiverTestProfile(config, GPSType::septentrio),
                  [&](const std::atomic_bool& stop) { return std::make_unique<ReceiverTransport>(stop, trace); });
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    GPSCorrectionFrame frame;
    frame.data = GpsTestHelpers::buildRtcmFrame(1077);
    frame.receivedAtMs = GPSObservation::monotonicNowUs() / 1000;
    frame.source = GPSCorrectionSource::Ntrip;
    frame.sourceInstance = QStringLiteral("caster/mount");
    frame.session = 23;
    frame.deliveryId = 101;
    QCOMPARE(session.submitCorrections(frame, session.sessionId()).outcome, GPSCorrectionOutcome::NotReady);
    // The SBF backend currently waits its full ACK deadlines, even with immediate replies.
    QTRY_VERIFY_WITH_TIMEOUT(session.readyForCorrections(), 30000);
    frame.receivedAtMs = GPSObservation::monotonicNowUs() / 1000;
    trace.holdRead = true;
    QVERIFY(trace.heldRead.tryAcquire(1, 5000));
    QVERIFY(session.submitCorrections(frame, session.sessionId()).accepted);
    session.clearPendingCorrections();
    frame.data = GpsTestHelpers::buildRtcmFrame(1087);
    frame.deliveryId = 102;
    QVERIFY(session.submitCorrections(frame, session.sessionId()).accepted);
    if (!partialWrite) {
        auto next = frame;
        next.deliveryId = 103;
        QVERIFY(session.submitCorrections(next, session.sessionId()).accepted);
    }
    QCOMPARE(session.deliveryStats().writtenCommandBytes, quint64(0));
    const quint64 destinationSession = session.sessionId();
    trace.releaseRead.release();
    QVERIFY(trace.correctionWritten.tryAcquire(1, 5000));
    const auto writtenBytes = partialWrite ? frame.data.size() / 2 : frame.data.size();
    QCOMPARE(trace.written, frame.data.first(writtenBytes));
    QCOMPARE(trace.writtenOn.load(), worker);
    QList<GPSCorrectionDelivery> reports;
    const auto collect = [&]() {
        for (const auto& emission : deliveries) {
            reports.append(emission.first().value<QList<GPSCorrectionDelivery>>());
        }
        deliveries.clear();
        return reports.size();
    };
    QTRY_COMPARE_WITH_TIMEOUT(collect(), partialWrite ? 2 : 3, 5000);
    QCOMPARE(reports[0].deliveryId, quint64(101));
    QCOMPARE(reports[0].outcome, GPSCorrectionOutcome::Cleared);
    QCOMPARE(reports[0].writtenBytes, quint64(0));
    QCOMPARE(reports[1].deliveryId, quint64(102));
    QCOMPARE(reports[1].source, GPSCorrectionSource::Ntrip);
    QCOMPARE(reports[1].sourceInstance, frame.sourceInstance);
    QCOMPARE(reports[1].sourceSession, quint64(23));
    QCOMPARE(reports[1].destinationId, QStringLiteral("localReceiver"));
    QCOMPARE(reports[1].destinationSession, destinationSession);
    QCOMPARE(reports[1].requestedBytes, quint64(frame.data.size()));
    QCOMPARE(reports[1].writtenBytes, quint64(writtenBytes));
    QCOMPARE(reports[1].acceptedBytes, quint64(frame.data.size()));
    QCOMPARE(reports[1].uncertainBytes, quint64(frame.data.size() - writtenBytes));
    QVERIFY(trace.writeBudgetMs > 0);
    QVERIFY(trace.writeBudgetMs <= 200);
    if (!partialWrite) {
        QVERIFY(trace.receivedBetweenCorrections.load());
        QCOMPARE(reports[2].deliveryId, quint64(103));
        QCOMPARE(reports[2].outcome, GPSCorrectionOutcome::Written);
    }
    QCOMPARE(reports[1].outcome, partialWrite ? GPSCorrectionOutcome::WriteFailed : GPSCorrectionOutcome::Written);
    session.stop();
    QVERIFY(!session.submitCorrections(frame, destinationSession).accepted);
}

void GPSReceiverContractTest::_deliveryReportsBoundAdmission()
{
    GPSReceiverMailbox mailbox;
    mailbox.setCorrectionsEnabled(true);
    GPSCorrectionFrame frame;
    frame.source = GPSCorrectionSource::Ntrip;
    frame.sourceInstance = QStringLiteral("caster/mount");
    frame.session = 3;
    frame.data = GpsTestHelpers::buildRtcmFrame(1077);
    frame.receivedAtMs = 100000;
    int notifications = 0;
    for (int index = 0; index < GPSReceiverMailbox::MAX_DELIVERIES; ++index) {
        frame.deliveryId = index + 1;
        QVERIFY(mailbox.submitCorrection(frame, 7, frame.receivedAtMs).accepted);
        const auto command = mailbox.takeCommand(frame.receivedAtMs);
        QVERIFY(command);
        notifications += mailbox.completeCommand(*command, GPSCorrectionOutcome::Written, frame.data.size());
    }
    QCOMPARE(notifications, 1);
    QCOMPARE(mailbox.stats().pendingCommands, 0);
    QCOMPARE(mailbox.stats().pendingDeliveries, GPSReceiverMailbox::MAX_DELIVERIES);
    QCOMPARE(mailbox.submitCorrection(frame, 7, frame.receivedAtMs).outcome, GPSCorrectionOutcome::Overflow);
    QList<GPSCorrectionDelivery> reports;
    bool more = false;
    do {
        auto batch = mailbox.take(frame.receivedAtMs);
        QVERIFY(batch.deliveries.size() <= GPSReceiverMailbox::FRAMES_PER_DRAIN);
        reports.append(batch.deliveries);
        more = batch.more;
    } while (more);
    QCOMPARE(reports.size(), GPSReceiverMailbox::MAX_DELIVERIES);
    for (int index = 0; index < reports.size(); ++index) {
        QCOMPARE(reports[index].deliveryId, quint64(index + 1));
        QCOMPARE(reports[index].writtenBytes, quint64(frame.data.size()));
    }
    QVERIFY(mailbox.submitCorrection(frame, 7, frame.receivedAtMs).accepted);
}

void GPSReceiverContractTest::_deliveryOutcomesPreserveProvenance()
{
    GPSReceiverMailbox mailbox;
    mailbox.setCorrectionsEnabled(true);
    GPSCorrectionFrame frame;
    frame.source = GPSCorrectionSource::Udp;
    frame.sourceInstance = QStringLiteral("127.0.0.1:2101");
    frame.session = 13;
    frame.deliveryId = 1;
    frame.data = GpsTestHelpers::buildRtcmFrame(1077);
    frame.receivedAtMs = 100000;
    QVERIFY(mailbox.submitCorrection(frame, 29, frame.receivedAtMs).accepted);
    QVERIFY(!mailbox.takeCommand(frame.receivedAtMs + GPSReceiverMailbox::MAX_AGE_MS));
    frame.deliveryId = 2;
    QVERIFY(mailbox.submitCorrection(frame, 29, frame.receivedAtMs).accepted);
    mailbox.close();
    const auto reports = mailbox.takeDeliveries();
    QCOMPARE(reports.size(), 2);
    QCOMPARE(reports[0].outcome, GPSCorrectionOutcome::Expired);
    QCOMPARE(reports[1].outcome, GPSCorrectionOutcome::Cancelled);
    for (const auto& report : reports) {
        QCOMPARE(report.source, frame.source);
        QCOMPARE(report.sourceInstance, frame.sourceInstance);
        QCOMPARE(report.sourceSession, frame.session);
        QCOMPARE(report.destinationSession, quint64(29));
        QCOMPARE(report.requestedBytes, quint64(frame.data.size()));
        QCOMPARE(report.writtenBytes, quint64(0));
    }
    QCOMPARE(mailbox.stats().queuedCommandBytes, quint64(2 * frame.data.size()));
    QCOMPARE(mailbox.stats().writtenCommandBytes, quint64(0));
    QCOMPARE(mailbox.stats().droppedCommandBytes, quint64(2 * frame.data.size()));
}

void GPSReceiverContractTest::_clearFlushesKnownResults_data()
{
    QTest::addColumn<bool>("stopFromResult");
    QTest::newRow("keep-session") << false;
    QTest::newRow("stop-from-result") << true;
}

void GPSReceiverContractTest::_clearFlushesKnownResults()
{
    QFETCH(bool, stopFromResult);
    GPSReceiverSession session;
    const auto gate = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        gate->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(gate));
    QVERIFY(gate->entered.tryAcquire(1, 5000));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto mailbox = worker->mailbox();
    mailbox->setCorrectionsEnabled(true);
    GPSCorrectionFrame frame;
    frame.source = GPSCorrectionSource::Ntrip;
    frame.sourceInstance = QStringLiteral("caster/mount");
    frame.session = 7;
    frame.data = GpsTestHelpers::buildRtcmFrame(1077);
    frame.receivedAtMs = GPSObservation::monotonicNowUs() / 1000;
    frame.deliveryId = 1;
    QVERIFY(mailbox->submitCorrection(frame, session.sessionId(), frame.receivedAtMs).accepted);
    const auto command = mailbox->takeCommand(frame.receivedAtMs);
    QVERIFY(command);
    if (mailbox->completeCommand(*command, GPSCorrectionOutcome::Written, frame.data.size())) {
        emit worker->dataReady();
    }
    frame.deliveryId = 2;
    QVERIFY(mailbox->submitCorrection(frame, session.sessionId(), frame.receivedAtMs).accepted);
    QSignalSpy results(&session, &GPSReceiverSession::correctionDeliveriesReady);
    if (stopFromResult) {
        connect(&session, &GPSReceiverSession::correctionDeliveriesReady, &session, &GPSReceiverSession::stop);
    }
    session.clearPendingCorrections();
    QCOMPARE(results.size(), 1);
    const auto known = results.first().first().value<QList<GPSCorrectionDelivery>>();
    QCOMPARE(known.size(), 2);
    QCOMPARE(known[0].outcome, GPSCorrectionOutcome::Written);
    QCOMPARE(known[0].writtenBytes, quint64(frame.data.size()));
    QCOMPARE(known[1].outcome, GPSCorrectionOutcome::Cleared);
    QCOMPARE(known[1].writtenBytes, quint64(0));
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(results.size(), 1);
    if (!stopFromResult) {
        frame.deliveryId = 3;
        QVERIFY(mailbox->submitCorrection(frame, session.sessionId(), frame.receivedAtMs).accepted);
        session.clearPendingCorrections();
        QCOMPARE(results.size(), 2);
        QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
        // A synchronous result flush must not leave the mailbox notification permanently reserved.
        GPSObservation observation;
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        QVERIFY(mailbox->publish(observation));
    }
}

void GPSReceiverContractTest::_configurationReportLifecycle()
{
    GPSReceiverSession session;
    const auto first = std::make_shared<WorkerGate>();
    const auto second = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        first->release.release();
        second->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(first));
    QVERIFY(first->entered.tryAcquire(1, 5000));
    auto* retired = session.findChild<GPSProvider*>();
    QVERIFY(retired);
    const quint64 oldSession = session.sessionId();
    QSignalSpy reports(&session, &GPSReceiverSession::configurationReported);
    GPSConfigurationReport report;
    GPSSettingReport setting;
    setting.id = GPSReceiverSetting::OutputRateHz;
    setting.requestedValue = 5;
    report.settings.append(setting);
    report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    emit retired->configurationReported(report);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(reports.size(), 1);
    QCOMPARE(session.configurationReport().sessionId, oldSession);
    QCOMPARE(session.configurationReport().monotonicTimestampUs, report.monotonicTimestampUs);
    QVERIFY(session.configurationReport().active);

    report.settings.first().requestState = GPSSettingReport::RequestState::Acknowledged;
    report.settings.first().readbackState = GPSSettingReport::ReadbackState::Reported;
    report.settings.first().reportedValue = 5;
    emit retired->configurationReported(report);
    emit retired->connectionError(GPSConnectionError::DeviceError);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(reports.size(), 3);
    QVERIFY(!session.configurationReport().active);
    QCOMPARE(session.configurationReport().settings.first().reportedValue.toInt(), 5);
    emit retired->configurationReported(report);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(reports.size(), 3);

    emit retired->configurationReported(report);
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(second));
    QVERIFY(second->entered.tryAcquire(1, 5000));
    QVERIFY(session.sessionId() != oldSession);
    QVERIFY(session.configurationReport().settings.isEmpty());
    auto* current = session.findChild<GPSProvider*>();
    QVERIFY(current);
    QVERIFY(current != retired);
    report.monotonicTimestampUs += 10;
    emit current->configurationReported(report);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(reports.size(), 5);
    QCOMPARE(session.configurationReport().sessionId, session.sessionId());
    QCOMPARE(session.configurationReport().monotonicTimestampUs, report.monotonicTimestampUs);
    QVERIFY(session.configurationReport().active);
    session.stop();
    QCOMPARE(reports.size(), 6);
    QVERIFY(session.configurationReport().settings.isEmpty());
    QVERIFY(!session.configurationReport().active);
}

void GPSReceiverContractTest::_configurationReportResetCanRestart()
{
    GPSReceiverSession session;
    const auto first = std::make_shared<WorkerGate>();
    const auto second = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        first->release.release();
        second->release.release();
        session.shutdown();
    });
    session.start(gpsReceiverTestProfile({}, GPSType::u_blox), blockedFactory(first));
    QVERIFY(first->entered.tryAcquire(1, 5000));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    GPSConfigurationReport report;
    report.settings.append(GPSSettingReport{});
    emit worker->configurationReported(report);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    bool restarted = false;
    connect(&session, &GPSReceiverSession::configurationReported, &session, [&](const GPSConfigurationReport& updated) {
        if (updated.settings.isEmpty() && !restarted) {
            restarted = true;
            GPSReceiverConfig config;
            config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
            session.start(gpsReceiverTestProfile(config, GPSType::u_blox), blockedFactory(second));
        }
    });
    session.stop();
    QVERIFY(restarted);
    QVERIFY(second->entered.tryAcquire(1, 5000));
    QVERIFY(session.hasReceiver());
    QVERIFY(session.nmeaDevice());
}

void GPSReceiverContractTest::_uncertainDeliveryPreservesCounts()
{
    GPSReceiverMailbox mailbox;
    mailbox.setCorrectionsEnabled(true);
    GPSCorrectionFrame frame;
    frame.data = GpsTestHelpers::buildRtcmFrame(1077);
    frame.receivedAtMs = 1000;
    frame.deliveryId = 12;
    QVERIFY(mailbox.submitCorrection(frame, 7, 1000).accepted);
    const auto command = mailbox.takeCommand(1000);
    QVERIFY(command);
    QVERIFY(mailbox.completeCommand(*command, GPSCorrectionOutcome::WriteFailed, 2, 5, 3));
    const auto reports = mailbox.takeDeliveries();
    QCOMPARE(reports.size(), 1);
    QCOMPARE(reports.first().writtenBytes, quint64(2));
    QCOMPARE(reports.first().acceptedBytes, quint64(5));
    QCOMPARE(reports.first().uncertainBytes, quint64(3));
    QCOMPARE(mailbox.stats().writtenCommandBytes, quint64(2));
    QCOMPARE(mailbox.stats().uncertainCommandBytes, quint64(3));
    QCOMPARE(mailbox.stats().failedCommandBytes, quint64(frame.data.size() - 5));
}

void GPSReceiverContractTest::_correctionGenerationSurvivesRestart()
{
    GPSReceiverSession session;
    const auto first = std::make_shared<WorkerGate>();
    const auto second = std::make_shared<WorkerGate>();
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        first->release.release();
        second->release.release();
        session.shutdown();
    });
    const auto profile = gpsReceiverTestProfile();
    session.start(profile, blockedFactory(first));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto originalGeneration = session.sessionId();
    connect(&session, &GPSReceiverSession::rtcmFrameReceived, &session,
            [&]() { session.start(profile, blockedFactory(second)); });
    quint64 deliveredGeneration = 0;
    connect(&session, &GPSReceiverSession::rtcmFrameReceived, &session,
            [&](const QByteArray&, qint64, quint64 generation) { deliveredGeneration = generation; });
    worker->RTCMFrameUpdate(GpsTestHelpers::buildRtcmFrame(1077), GPSObservation::monotonicNowUs() / 1000);
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
    QCOMPARE(deliveredGeneration, originalGeneration);
    QVERIFY(session.sessionId() > originalGeneration);
}

QTEST_GUILESS_MAIN(GPSReceiverContractTest)

#include "GPSReceiverContractTest.moc"
