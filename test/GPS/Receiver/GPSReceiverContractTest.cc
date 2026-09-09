#include <QtCore/QEvent>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <atomic>
#include <cstring>
#include <thread>

#include "GPSReceiverSession.h"
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
    void _stalledConsumerBoundsUpdates();
    void _retirementDiscardsPendingData();
    void _correctionQueueLifecycle();
    void _staleDataIsNotRejuvenated();
    void _correctionsWrittenOnlyOnWorker();
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
    session.start(GPSType::u_blox, blockedFactory(gate), {});
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
    session.start(GPSType::u_blox, blockedFactory(first), {});
    QVERIFY(first->entered.tryAcquire(1, 5000));
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto mailbox = worker->mailbox();
    GPSObservation observation;
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    worker->sensorGpsUpdate(observation);
    worker->RTCMFrameUpdate(GpsTestHelpers::buildRtcmFrame(1077), observation.monotonicTimestampUs / 1000);
    const quint64 oldSession = session.sessionId();
    QSignalSpy positions(&session, &GPSReceiverSession::positionReceived);
    QSignalSpy corrections(&session, &GPSReceiverSession::rtcmFrameReceived);
    session.start(GPSType::u_blox, blockedFactory(second), {});
    QVERIFY(second->entered.tryAcquire(1, 5000));
    QCoreApplication::sendPostedEvents(&session, QEvent::MetaCall);
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

void GPSReceiverContractTest::_correctionsWrittenOnlyOnWorker()
{
    struct Trace
    {
        QSemaphore correctionWritten;
        QSemaphore heldRead;
        QSemaphore releaseRead;
        std::atomic_bool holdRead = false;
        std::atomic<QThread*> writtenOn = nullptr;
        QByteArray written;
    };

    class ReceiverTransport : public GPSTransport
    {
    public:
        ReceiverTransport(const std::atomic_bool& stop, Trace& trace)
            : GPSTransport(stop)
            , _trace(trace)
        {}

        bool open() override { return true; }

        bool fatalError() const override { return false; }

        bool setBaudrate(unsigned) override { return !isCancelled(); }

        unsigned fixedBaudrate() const override { return 115200; }

        int read(uint8_t* data, int size, int) override
        {
            if (_trace.holdRead.exchange(false)) {
                _trace.heldRead.release();
                _trace.releaseRead.acquire();
            }
            if (isCancelled()) {
                return -1;
            }
            const int count = qMin(size, int(_reply.size()));
            std::memcpy(data, _reply.constData(), count);
            _reply.remove(0, count);
            // Emulate an idle blocking device between ACK chunks.
            if (count == 0) {
                QSemaphore idle;
                idle.tryAcquire(1, 1);
            }
            return count;
        }

        int write(const uint8_t* data, int size) override
        {
            const QByteArray command(reinterpret_cast<const char*>(data), size);
            if (command.startsWith(char(0xd3))) {
                _trace.written = command;
                _trace.writtenOn = QThread::currentThread();
                _trace.correctionWritten.release();
            } else {
                _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
            }
            return size;
        }

    private:
        Trace& _trace;
        QByteArray _reply;
    };

    Trace trace;
    GPSReceiverSession session;
    const auto cleanup = qScopeGuard([&]() {
        session.stop();
        trace.releaseRead.release();
        session.shutdown();
    });
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    session.start(
        GPSType::septentrio,
        [&](const std::atomic_bool& stop) { return std::make_unique<ReceiverTransport>(stop, trace); }, config);
    auto* worker = session.findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1077);
    QVERIFY(!session.submitCorrections(frame, GPSObservation::monotonicNowUs() / 1000, session.sessionId()));
    // The SBF backend currently waits its full ACK deadlines, even with immediate replies.
    QTRY_VERIFY_WITH_TIMEOUT(session.readyForCorrections(), 30000);
    trace.holdRead = true;
    QVERIFY(trace.heldRead.tryAcquire(1, 5000));
    QVERIFY(session.submitCorrections(frame, GPSObservation::monotonicNowUs() / 1000, session.sessionId()));
    session.clearPendingCorrections();
    const auto replacement = GpsTestHelpers::buildRtcmFrame(1087);
    QVERIFY(session.submitCorrections(replacement, GPSObservation::monotonicNowUs() / 1000, session.sessionId()));
    trace.releaseRead.release();
    QVERIFY(trace.correctionWritten.tryAcquire(1, 5000));
    QCOMPARE(trace.written, replacement);
    QCOMPARE(trace.writtenOn.load(), worker);
    session.stop();
    QVERIFY(!session.submitCorrections(frame, GPSObservation::monotonicNowUs() / 1000, session.sessionId()));
}

QTEST_GUILESS_MAIN(GPSReceiverContractTest)

#include "GPSReceiverContractTest.moc"
