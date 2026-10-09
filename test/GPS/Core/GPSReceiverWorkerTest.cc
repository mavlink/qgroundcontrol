#include "GPSReceiverWorkerTest.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QRegularExpression>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include "GPSCancellation.h"
#include "GPSDriver.h"
#include "GPSInputMonitor.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverWorker.h"
#include "NMEASentence.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/ScriptedReceiver.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
struct TransportTrace
{
    QThread* constructedOn = nullptr;
    QThread* openedOn = nullptr;
    QThread* destroyedOn = nullptr;
    std::weak_ptr<int> factoryLifetime;
    bool factoryAliveDuringDestruction = false;
};

class TestTransport : public ScriptedReceiver
{
public:
    TestTransport(GPSCancelToken cancelToken, TransportTrace& trace, std::function<void()> stop, bool openResult,
                  bool cancelInOpen)
        : ScriptedReceiver(std::move(cancelToken))
        , _trace(trace)
        , _stop(stop)
        , _openResult(openResult)
        , _cancelInOpen(cancelInOpen)
    {
        _trace.constructedOn = QThread::currentThread();
        setOpenHandler([this] {
            _trace.openedOn = QThread::currentThread();
            if (_cancelInOpen) {
                _stop();
            }
            return _openResult ? GPSOpenResult{GPSOpenStatus::Opened}
                               : GPSOpenResult{GPSOpenStatus::Error, QStringLiteral("test port is busy")};
        });
        setReadHandler([](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
            return GPSReadResult{GPSReadStatus::Error};
        });
        setWriteHandler([](const QByteArray&, const ScriptedReceiver::WriteContext&) {
            return std::optional<GPSWriteResult>{GPSWriteResult{GPSWriteStatus::Error}};
        });
    }

    ~TestTransport() override
    {
        _trace.destroyedOn = QThread::currentThread();
        _trace.factoryAliveDuringDestruction = !_trace.factoryLifetime.expired();
    }

private:
    TransportTrace& _trace;
    std::function<void()> _stop;
    bool _openResult;
    bool _cancelInOpen;
};
}  // namespace

void GPSReceiverWorkerTest::_transportLifetimeStaysOnWorker_data()
{
    QTest::addColumn<bool>("openResult");
    QTest::addColumn<QString>("cancel");
    QTest::newRow("opened") << true << QStringLiteral("in-open");
    QTest::newRow("cancelled-during-open") << false << QStringLiteral("in-open");
    QTest::newRow("open-failed") << false << QStringLiteral("none");
    QTest::newRow("cancelled-in-factory") << true << QStringLiteral("in-factory");
    QTest::newRow("cancelled-before-start") << true << QStringLiteral("before-start");
}

void GPSReceiverWorkerTest::_transportLifetimeStaysOnWorker()
{
    QFETCH(bool, openResult);
    QFETCH(QString, cancel);
    const bool cancelInFactory = cancel == QStringLiteral("in-factory");
    TransportTrace trace;
    auto lifetime = std::make_shared<int>(0);
    trace.factoryLifetime = lifetime;
    std::function<void()> stopWorker;
    GPSReceiverWorker worker(
        [&, lifetime = std::move(lifetime)](GPSCancelToken cancelToken) {
            if (cancelInFactory) {
                stopWorker();
            }
            return std::make_unique<TestTransport>(std::move(cancelToken), trace, stopWorker, openResult,
                                                   cancel == QStringLiteral("in-open"));
        },
        GPSType::ublox, GPSReceiverConfig{});
    stopWorker = [&worker]() { worker.stop(); };
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    if (cancel == QStringLiteral("before-start")) {
        worker.stop();
    }
    worker.start();
    QVERIFY(worker.wait(TestTimeout::shortDuration()));
    QVERIFY(trace.factoryLifetime.expired());
    if (cancel == QStringLiteral("before-start")) {
        // A cancelled worker never creates its transport.
        QVERIFY(!trace.constructedOn);
        QVERIFY(errors.isEmpty());
        return;
    }
    QVERIFY(trace.constructedOn && trace.constructedOn != QThread::currentThread());
    // A transport cancelled while it is created is never opened.
    QCOMPARE(trace.openedOn, cancelInFactory ? nullptr : trace.constructedOn);
    QCOMPARE(trace.destroyedOn, trace.constructedOn);
    QVERIFY(trace.factoryAliveDuringDestruction);
    QCOMPARE(errors.size(), cancel == QStringLiteral("none") ? 1 : 0);
    if (!errors.isEmpty()) {
        QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
        // The transport's reason reaches the session.
        QCOMPARE(errors.first().at(1).toString(), QStringLiteral("test port is busy"));
    }
}

void GPSReceiverWorkerTest::_missingTransportReportsOpenFailure_data()
{
    QTest::addColumn<bool>("hasFactory");
    QTest::newRow("empty-factory") << false;
    QTest::newRow("null-transport") << true;
}

void GPSReceiverWorkerTest::_missingTransportReportsOpenFailure()
{
    QFETCH(bool, hasFactory);
    GPSReceiverWorker::TransportFactory factory;
    if (hasFactory) {
        factory = [](GPSCancelToken) { return std::unique_ptr<GPSTransport>{}; };
    }
    GPSReceiverWorker worker(std::move(factory), GPSType::ublox, GPSReceiverConfig{});
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    worker.start();
    QVERIFY(worker.wait(TestTimeout::shortDuration()));
    QCOMPARE(errors.count(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
}

void GPSReceiverWorkerTest::_workerLifecycle()
{
    QSemaphore entered;
    QThread* sessionThread = nullptr;
    bool tokenStopped = false;
    auto worker = std::make_unique<GPSReceiverWorker>(
        [&](GPSCancelToken cancelToken) {
            sessionThread = QThread::currentThread();
            QSemaphore stopped;
            const GPSCancelCallback wake(cancelToken, [&stopped] { stopped.release(); });
            entered.release();
            tokenStopped = stopped.tryAcquire(1, TestTimeout::mediumMs()) && cancelToken.isCancelled();
            return std::unique_ptr<GPSTransport>{};
        },
        GPSType::ublox, GPSReceiverConfig{});
    QVERIFY(worker->wait(0ms));
    QVERIFY(!worker->_thread);
    QSignalSpy finished(worker.get(), &GPSReceiverWorker::finished);
    worker->start();
    worker->start();
    QVERIFY(entered.tryAcquire(1, TestTimeout::shortMs()));
    QVERIFY(worker->_thread && worker->_thread->isRunning());
    QVERIFY(sessionThread != QThread::currentThread());
    QCOMPARE(worker->thread(), QThread::currentThread());
    QVERIFY(!worker->wait(1ms));
    worker->stop();
    QVERIFY(worker->wait(TestTimeout::shortDuration()));
    QVERIFY(tokenStopped);
    QVERIFY(finished.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, TestTimeout::shortMs());
    QVERIFY(!worker->_thread->isRunning());
    worker.reset();
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverWorkerTest, TestLabel::Unit)

namespace {
uint64_t durationUs(std::chrono::milliseconds duration)
{
    return static_cast<uint64_t>(std::chrono::microseconds(duration).count());
}

/// An LG290P reporting @a identity, with @a role and saved @a base, answering on @a clock.
std::unique_ptr<GPSTransport> quectelTransport(GPSCancelToken cancelToken, GPSTestClock& clock, unsigned role,
                                               const std::string& base, const std::string& identity)
{
    auto transport = std::make_unique<ModelReceiver<QuectelReceiverModel>>(std::move(cancelToken), clock);
    auto& model = transport->model;
    model.identity = identity;
    model.role = model.activeRole = model.savedRole = role;
    model.base = model.activeBase = model.savedBase = base;
    transport->setFixedBaudrate(460800);
    return transport;
}

/// A passive receiver that sends a satellite view with its first position, then one more position POSITION_AT after
/// opening, and nothing after that.
class ExpiringSatelliteTransport : public ScriptedReceiver
{
public:
    static constexpr std::chrono::milliseconds POSITION_AT{2000};

    ExpiringSatelliteTransport(GPSCancelToken cancelToken, GPSTestClock& clock, std::atomic<uint64_t>& lastPositionAtUs)
        : ScriptedReceiver(std::move(cancelToken))
        , _clock(clock)
        , _lastPositionAtUs(lastPositionAtUs)
    {
        setClock(&clock);
        setReadHandler([this](uint8_t*, int, std::chrono::milliseconds timeout) -> std::optional<GPSReadResult> {
            // The position arrives during the read that waits past its time.
            const uint64_t positionAtUs = _openedAtUs + durationUs(POSITION_AT);
            const uint64_t readUntilUs = _clock.nowUs() + durationUs(timeout);
            if (!_sentPosition && !hasQueuedReadData() && readUntilUs >= positionAtUs) {
                _clock.advanceTo(positionAtUs);
                _sentPosition = true;
                queueReply(position());
                _lastPositionAtUs = positionAtUs;
            }
            return std::nullopt;
        });
    }

    GPSOpenResult open() override
    {
        const auto result = ScriptedReceiver::open();
        _openedAtUs = _clock.nowUs();
        _sentPosition = false;
        clearReplies();
        queueReply("$GPGSV,1,1,01,01,10,20,30*79\r\n" + position());
        _lastPositionAtUs = _openedAtUs;
        return result;
    }

private:
    static QByteArray position() { return "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"; }

    GPSTestClock& _clock;
    uint64_t _openedAtUs = 0;
    bool _sentPosition = false;
    std::atomic<uint64_t>& _lastPositionAtUs;
};

/// A passive receiver that sends @a input every 100 ms, or a GGA position once @a positions is set.
class PassiveInputTransport : public ScriptedReceiver
{
public:
    PassiveInputTransport(GPSCancelToken cancelToken, GPSTestClock& clock, QByteArray input,
                          const std::atomic_bool& positions)
        : ScriptedReceiver(std::move(cancelToken))
        , _clock(clock)
        , _input(std::move(input))
        , _positions(positions)
    {
        setClock(&clock);
        setReadHandler([this](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
            const QByteArray next =
                _positions ? QByteArrayLiteral("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n")
                           : _input;
            if (!next.isEmpty() && !hasQueuedReadData()) {
                _clock.advanceBy(100000);
                queueReply(next);
            }
            return std::nullopt;
        });
    }

private:
    GPSTestClock& _clock;
    QByteArray _input;
    const std::atomic_bool& _positions;
};

class FixSequenceTransport : public ScriptedReceiver
{
public:
    explicit FixSequenceTransport(GPSCancelToken cancelToken)
        : ScriptedReceiver(std::move(cancelToken))
    {
        setReadHandler([this](uint8_t*, int, std::chrono::milliseconds) -> std::optional<GPSReadResult> {
            if (!hasQueuedReadData()) {
                return GPSReadResult{GPSReadStatus::Cancelled};
            }
            return std::nullopt;
        });
    }

    GPSOpenResult open() override
    {
        const auto result = ScriptedReceiver::open();
        QByteArray pending;
        for (const int quality : {0, 0, 1, 1, 4, 4, 0}) {
            pending += NMEAUtils::repairChecksum("$GPGGA,123519,4807.038,N,01131.000,E," + QByteArray::number(quality) +
                                                 ",08,0.9,545.4,M,46.9,M,,");
        }
        queueReply(pending);
        return result;
    }
};
}  // namespace

void GPSReceiverWorkerTest::_positionFixTransitions()
{
    for (int session = 0; session < 2; ++session) {
        GPSReceiverWorker worker(
            [](GPSCancelToken cancelToken) { return std::make_unique<FixSequenceTransport>(std::move(cancelToken)); },
            GPSType::passive, {.baudRate = 115200});
        QSignalSpy positions(&worker, &GPSReceiverWorker::positionUpdated);
        worker.start();
        const bool finished = worker.wait(TestTimeout::shortDuration());
        if (!finished) {
            worker.stop();
            QVERIFY(worker.wait(TestTimeout::longDuration()));
        }
        QVERIFY(finished);
        using Fix = GPSPositionReport::FixType;
        const QList<Fix> expected{Fix::NoFix,    Fix::NoFix,    Fix::Fix3D, Fix::Fix3D,
                                  Fix::RTKFixed, Fix::RTKFixed, Fix::NoFix};
        QCOMPARE(positions.size(), expected.size());
        for (qsizetype i = 0; i < expected.size(); ++i) {
            const auto report = qvariant_cast<GPSPositionReport>(positions[i].first());
            QCOMPARE(report.navigation.fixType, expected[i]);
            if (expected[i] != Fix::NoFix) {
                QVERIFY(qAbs(report.navigation.latitudeDegrees - 48.1173) < 1e-6);
            }
        }
    }
}

void GPSReceiverWorkerTest::_satelliteExpiryDoesNotRenewLiveness()
{
    GPSTestClock clock(GPSTestClock::START_US);
    std::atomic<uint64_t> lastPositionAtUs = 0;
    std::atomic<uint64_t> endedAtUs = 0;
    GPSReceiverWorker worker(
        [&](GPSCancelToken cancelToken) {
            return std::make_unique<ExpiringSatelliteTransport>(std::move(cancelToken), clock, lastPositionAtUs);
        },
        GPSType::passive, {.baudRate = 115200}, nullptr, clock.source());
    std::atomic_bool freshView = false;
    std::atomic_bool expiredView = false;
    connect(
        &worker, &GPSReceiverWorker::connectionError, &worker, [&] { endedAtUs = clock.nowUs(); },
        Qt::DirectConnection);
    connect(
        &worker, &GPSReceiverWorker::satelliteInfoUpdated, &worker,
        [&](const GPSSatelliteReport& report) {
            if (report.timestampUs && report.inView.value_or(0)) {
                freshView = true;
            } else if (!report.timestampUs && freshView.load()) {
                expiredView = true;
            }
        },
        Qt::DirectConnection);
    QSignalSpy positions(&worker, &GPSReceiverWorker::positionUpdated);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    worker.start();
    const bool finished = worker.wait(TestTimeout::longDuration());
    if (!finished) {
        worker.stop();
        QVERIFY(worker.wait(TestTimeout::longDuration()));
    }
    QVERIFY(finished);
    QVERIFY(freshView.load());
    QVERIFY(expiredView.load());
    QCOMPARE(positions.size(), 2);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    QCOMPARE(lastPositionAtUs.load(), GPSTestClock::START_US + durationUs(ExpiringSatelliteTransport::POSITION_AT));
    // The session ends one inactivity window after the last position; an expiry credited as traffic would extend it.
    QCOMPARE(endedAtUs.load(), lastPositionAtUs.load() + durationUs(GPSReceiverWorker::USEFUL_DATA_TIMEOUT));
}

void GPSReceiverWorkerTest::_passiveInputProblems_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<GPSInputProblem>("problem");
    QTest::addColumn<int>("protocol");
    QTest::newRow("silent") << QByteArray() << GPSInputProblem::NoData << -1;
    QTest::newRow("not-gnss") << QByteArray("AT+OK\r\nhello\r\n") << GPSInputProblem::NotGNSS << -1;
    QTest::newRow("ubx-without-position") << ubxBytes(0x3501, QByteArray::fromHex("e803000001000000"))
                                          << GPSInputProblem::NoPositions << static_cast<int>(GPSType::ublox);
    QTest::newRow("rtcm-only") << rtcmMessage(1005) << GPSInputProblem::CorrectionsOnly << -1;
}

void GPSReceiverWorkerTest::_passiveInputProblems()
{
    QFETCH(QByteArray, input);
    QFETCH(GPSInputProblem, problem);
    QFETCH(int, protocol);
    GPSTestClock clock(GPSTestClock::START_US);
    std::atomic_bool positions = false;
    GPSReceiverWorker worker(
        [&clock, &positions, input](GPSCancelToken cancelToken) {
            return std::make_unique<PassiveInputTransport>(std::move(cancelToken), clock, input, positions);
        },
        GPSType::passive, {.baudRate = 115200}, nullptr, clock.source());
    worker.setEndsWhenIdle(false);
    uint64_t readyAtUs = 0;
    std::vector<GPSType> detected;
    std::vector<std::pair<GPSInputProblem, uint64_t>> reports;
    connect(
        &worker, &GPSReceiverWorker::receiverReady, &worker, [&] { readyAtUs = clock.nowUs(); }, Qt::DirectConnection);
    connect(
        &worker, &GPSReceiverWorker::receiverDetected, &worker, [&](GPSType type) { detected.push_back(type); },
        Qt::DirectConnection);
    connect(
        &worker, &GPSReceiverWorker::inputProblem, &worker,
        [&](GPSInputProblem reported) {
            reports.emplace_back(reported, clock.nowUs());
            // Positions clear the report; the session runs on until stopped.
            if (reported == GPSInputProblem::None) {
                worker.stop();
            }
            positions = true;
        },
        Qt::DirectConnection);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    worker.start();
    const bool finished = worker.wait(TestTimeout::longDuration());
    if (!finished) {
        worker.stop();
        QVERIFY(worker.wait(TestTimeout::longDuration()));
    }
    QVERIFY(finished);
    QCOMPARE(errors.size(), 0);
    QCOMPARE(reports.size(), 2);
    QCOMPARE(reports[0].first, problem);
    QCOMPARE(reports[1].first, GPSInputProblem::None);
    // Reported once no position arrived for the inactivity window, within one receive of it.
    const uint64_t silentUs = reports[0].second - readyAtUs;
    QVERIFY(silentUs >= durationUs(GPSReceiverWorker::USEFUL_DATA_TIMEOUT));
    QVERIFY(silentUs <= durationUs(GPSReceiverWorker::USEFUL_DATA_TIMEOUT + GPSReceiverWorker::RECEIVE_TIMEOUT));
    QVERIFY(!detected.empty());
    QCOMPARE(detected.front(), protocol < 0 ? GPSType::passive : static_cast<GPSType>(protocol));
}

void GPSReceiverWorkerTest::_ancillaryTraffic_data()
{
    QTest::addColumn<bool>("sendUsage");
    QTest::newRow("rapid-ancillary-then-data") << true;
    QTest::newRow("activity-only-expires") << false;
}

void GPSReceiverWorkerTest::_ancillaryTraffic()
{
    QFETCH(bool, sendUsage);
    GPSTestClock clock(GPSTestClock::START_US);
    auto model = std::make_shared<SBFReceiverModel>(clock);
    SBFReceiverModel* peer = model.get();
    std::atomic<uint64_t> readyAtUs = 0;
    std::atomic<uint64_t> endedAtUs = 0;
    GPSReceiverWorker worker(
        [model, sendUsage, &clock](GPSCancelToken cancelToken) {
            model->sendUsage = sendUsage;
            auto transport = std::make_unique<ScriptedReceiver>(std::move(cancelToken), model.get());
            transport->setClock(&clock);
            return transport;
        },
        GPSType::septentrio,
        {.base = {.mode =
                      GPSBaseStationConfig::Fixed{
                          .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}},
        nullptr, clock.source());
    connect(
        &worker, &GPSReceiverWorker::receiverReady, &worker,
        [&] {
            readyAtUs = clock.nowUs();
            peer->streaming = true;
        },
        Qt::DirectConnection);
    connect(
        &worker, &GPSReceiverWorker::connectionError, &worker, [&] { endedAtUs = clock.nowUs(); },
        Qt::DirectConnection);
    connect(
        &worker, &GPSReceiverWorker::satelliteInfoUpdated, &worker, [&](const auto&) { worker.stop(); },
        Qt::DirectConnection);
    QSignalSpy ready(&worker, &GPSReceiverWorker::receiverReady);
    QSignalSpy satellites(&worker, &GPSReceiverWorker::satelliteInfoUpdated);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    worker.start();
    const bool finished = worker.wait(TestTimeout::mediumDuration());
    if (!finished) {
        worker.stop();
        worker.wait();
    }
    QVERIFY(finished);
    QCOMPARE(ready.size(), 1);
    QCOMPARE(satellites.size(), sendUsage ? 1 : 0);
    QCOMPARE(errors.size(), sendUsage ? 0 : 1);
    if (sendUsage) {
        const auto report = qvariant_cast<GPSSatelliteReport>(satellites.first().first());
        QVERIFY(!report.inView);
        QCOMPARE(report.used, std::optional<int>{12});
    } else {
        // Blocks without useful data are activity, which never extends the inactivity window.
        QVERIFY(peer->streamReads > 1);
        QCOMPARE(endedAtUs.load(), readyAtUs.load() + durationUs(GPSReceiverWorker::USEFUL_DATA_TIMEOUT));
        QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    }
}

void GPSReceiverWorkerTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::newRow("survey") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::SurveyIn{2, 180s}};
    QTest::newRow("fixed-wire-limits") << GPSBaseStationConfig{
        .mode = GPSBaseStationConfig::Fixed{
            .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 21474836.0f},
            .accuracyMeters = 429496.71875f}};
}

void GPSReceiverWorkerTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(GPSBaseStationConfig, config);
    const QString diagnostic = QStringLiteral("Receiver read failed (status %1): Scripted receiver connection lost")
                                   .arg(static_cast<int>(GPSReadStatus::Error));
    expectLogMessage("GPS.Protocols.Femto", QtWarningMsg, QRegularExpression(QRegularExpression::escape(diagnostic)));
    GPSTestClock clock(GPSTestClock::START_US);
    GPSReceiverWorker worker(
        [&clock](GPSCancelToken cancelToken) {
            auto transport = std::make_unique<ModelReceiver<FemtoReceiverModel>>(std::move(cancelToken), clock);
            transport->model.failIdleReads = true;
            return transport;
        },
        GPSType::femto, GPSReceiverConfig{.base = config}, nullptr, clock.source());
    QSignalSpy ready(&worker, &GPSReceiverWorker::receiverReady);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    QSignalSpy surveys(&worker, &GPSReceiverWorker::surveyInStatusUpdated);
    worker.start();
    QVERIFY(worker.wait(TestTimeout::mediumDuration()));
    verifyExpectedLogMessage();
    QCOMPARE(ready.size(), 1);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    // The transport's diagnostic reaches the receiver's message.
    QCOMPARE(errors.first().at(1).toString(), QStringLiteral("Scripted receiver connection lost"));
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.mode)) {
        QCOMPARE(surveys.size(), 1);
        const auto status = qvariant_cast<GPSSurveyReport>(surveys.first().first());
        QCOMPARE(status.position.latitudeDegrees, fixed->position.latitudeDegrees);
        QCOMPARE(status.position.longitudeDegrees, fixed->position.longitudeDegrees);
        QCOMPARE(status.position.altitudeMeters, fixed->position.altitudeMeters);
        QVERIFY(!status.meanAccuracyMeters.has_value());
        QCOMPARE(status.duration.count(), 0);
        QVERIFY(status.valid);
        QVERIFY(!status.active);
    }
}

void GPSReceiverWorkerTest::_quectelConsentRefusal_data()
{
    QTest::addColumn<unsigned>("role");
    QTest::addColumn<QString>("base");
    QTest::addColumn<QString>("identity");
    QTest::addColumn<GPSConnectionError>("error");
    QTest::addColumn<QString>("detail");
    // The request is a 60-observation survey at 15 m.
    const QString requestedBase = QStringLiteral("1,60,15.0,0.0000,0.0000,0.0000,0.0");
    const QString qualified = QString::fromLatin1(GPSTest::QUECTEL_IDENTITY);
    QTest::newRow("rover-role") << 1U << requestedBase << qualified << GPSConnectionError::ConsentRequired
                                << QStringLiteral("LG290P role mismatch");
    QTest::newRow("base-differs") << 2U << QStringLiteral("1,120,15.0,0.0000,0.0000,0.0000,0.0") << qualified
                                  << GPSConnectionError::ConsentRequired
                                  << QStringLiteral("LG290P base settings mismatch");
    // Flash-save consent would not help a receiver that is not an LG290P(03).
    QTest::newRow("unqualified-firmware")
        << 2U << requestedBase
        << QString::fromStdString(GPSTest::nmeaSentence("PQTMVERNO,LC29HAANR11A03S,2024/04/30,10:53:07"))
        << GPSConnectionError::ConfigFailed << QStringLiteral("No verified LG290P(03) identity");
}

void GPSReceiverWorkerTest::_quectelConsentRefusal()
{
    QFETCH(unsigned, role);
    QFETCH(QString, base);
    QFETCH(QString, identity);
    QFETCH(GPSConnectionError, error);
    QFETCH(QString, detail);
    GPSTestClock clock(GPSTestClock::START_US);
    GPSReceiverWorker worker(
        [&clock, role, base = base.toStdString(), identity = identity.toStdString()](GPSCancelToken cancelToken) {
            return quectelTransport(std::move(cancelToken), clock, role, base, identity);
        },
        GPSType::quectel,
        GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 15, .duration = 60s}}},
        nullptr, clock.source());
    QSignalSpy ready(&worker, &GPSReceiverWorker::receiverReady);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    const QRegularExpression diagnostic(QRegularExpression::escape(detail));
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, diagnostic);
    worker.start();
    QVERIFY(worker.wait(TestTimeout::mediumDuration()));
    verifyExpectedLogMessage();
    QVERIFY(ready.isEmpty());
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().at(0)), error);
    QVERIFY2(errors.first().at(1).toString().startsWith(detail), qPrintable(errors.first().at(1).toString()));
}

void GPSReceiverWorkerTest::_inputMonitorClassification()
{
    constexpr uint64_t WINDOW_US = 1'000'000;
    const auto at = [](uint64_t seconds) { return seconds * WINDOW_US; };
    const GPSReceiveResult idle{};
    const GPSReceiveResult activity{.liveness = GPSReceiveLiveness::Activity};
    const GPSReceiveResult data{.liveness = GPSReceiveLiveness::Data};
    const GPSReceiveResult position{.liveness = GPSReceiveLiveness::Data, .updates = GPSReceiveUpdate::Position};

    GPSInputMonitor monitor(at(1), std::chrono::milliseconds(1000));
    QVERIFY(!monitor.update(idle, false, at(1)));
    QCOMPARE(monitor.problem(), GPSInputProblem::None);
    QVERIFY(monitor.update(idle, false, at(3)));
    QCOMPARE(monitor.problem(), GPSInputProblem::NoData);
    QVERIFY(monitor.update(activity, false, at(5)));
    QCOMPARE(monitor.problem(), GPSInputProblem::NotGNSS);
    QVERIFY(monitor.update(data, false, at(5)));
    QCOMPARE(monitor.problem(), GPSInputProblem::CorrectionsOnly);
    QVERIFY(monitor.update(data, true, at(5)));
    QCOMPARE(monitor.problem(), GPSInputProblem::NoPositions);
    QVERIFY(!monitor.update(data, true, at(5)));
    QVERIFY(monitor.update(position, true, at(6)));
    QCOMPARE(monitor.problem(), GPSInputProblem::None);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSReceiverWorkerTest::_finishedReceiverReleasesReservation_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("open-failed") << false;
    QTest::newRow("cancelled-before-start") << true;
}

void GPSReceiverWorkerTest::_finishedReceiverReleasesReservation()
{
    QFETCH(bool, cancelled);
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeRTKGPS, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    GPSReceiverWorker worker(
        [reservation = std::move(reservation)](GPSCancelToken) { return std::unique_ptr<GPSTransport>{}; },
        GPSType::ublox, GPSReceiverConfig{});
    if (cancelled) {
        worker.stop();
    }
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    worker.start();
    QVERIFY(worker.wait(TestTimeout::shortDuration()));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.reservePort(QStringLiteral("/test/mavlink")));
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
}
#endif
