#include "GPSProviderTest.h"

#include <cerrno>
#include <cmath>
#include <cstring>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "GPSDriver.h"
#include "GPSProvider.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSTransport.h"
#include "NMEAUtils.h"
#include "ScriptedSBFReceiver.h"
#include "UnitTest.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

Q_DECLARE_METATYPE(GPSBaseStationConfig)
Q_DECLARE_METATYPE(GPSSurveyReport)

void GPSProviderTest::_queuedPayloadsOwnSnapshots()
{
    GPSProvider provider({}, GPSType::ublox, {});
    for (const auto name : {"GPSSatelliteReport", "GPSSatelliteUsageReport", "GPSPositionReport::FixType",
                            "GPSConnectionError", "GPSSurveyInStatus"}) {
        QVERIFY2(QMetaType::fromName(name).isValid(), name);
    }
    GPSSatelliteReport satellites;
    GPSSatelliteUsageReport usage;
    auto fixType = GPSPositionReport::FixType::Unknown;
    GPSSurveyInStatus survey;
    GPSConnectionError error = GPSConnectionError::None;
    QString configurationDetail;
    QObject receiver;
    connect(
        &provider, &GPSProvider::satelliteInfoUpdate, &receiver,
        [&](const GPSSatelliteReport& value) { satellites = value; }, Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::satelliteUsageUpdate, &receiver,
        [&](const GPSSatelliteUsageReport& value) { usage = value; }, Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::fixTypeChanged, &receiver, [&](GPSPositionReport::FixType value) { fixType = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::surveyInStatus, &receiver, [&](const GPSSurveyInStatus& value) { survey = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::connectionError, &receiver, [&](GPSConnectionError value) { error = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::configurationError, &receiver,
        [&](const QString& value) { configurationDetail = value; }, Qt::QueuedConnection);
    auto worker = std::unique_ptr<QThread>(QThread::create([&]() {
        GPSSatelliteReport snapshot;
        snapshot.count = 1;
        snapshot.satellites[0].used = true;
        emit provider.satelliteInfoUpdate(snapshot);
        snapshot.satellites[0].used = false;
        GPSSatelliteUsageReport count{.timestampUs = 123, .usedCount = 7};
        emit provider.satelliteUsageUpdate(count);
        count.usedCount.reset();
        emit provider.fixTypeChanged(GPSPositionReport::FixType::Fix3D);
        GPSSurveyReport progress;
        progress.duration = std::chrono::seconds(4294967295LL);
        progress.meanAccuracyMeters = 1.234;
        provider._handleSurveyIn(progress);
        progress.meanAccuracyMeters = 0;
        QString detail = QStringLiteral("Settings may be saved; reconnect failed");
        emit provider.configurationError(detail);
        detail.clear();
        emit provider.connectionError(GPSConnectionError::DeviceError);
    }));
    worker->start();
    const bool timely = worker->wait(TestTimeout::shortMs());
    if (!timely) {
        worker->wait();
    }
    QVERIFY(timely);
    QCOMPARE(satellites.count, 0);
    QCOMPARE(survey.duration.count(), 0);
    QCoreApplication::sendPostedEvents(&receiver, QEvent::MetaCall);
    QCOMPARE(satellites.count, 1);
    QCOMPARE(satellites.satellites[0].used, std::optional<bool>{true});
    QCOMPARE(usage.timestampUs, uint64_t{123});
    QCOMPARE(usage.usedCount, std::optional<int>{7});
    QCOMPARE(fixType, GPSPositionReport::FixType::Fix3D);
    QCOMPARE(survey.duration.count(), 4294967295LL);
    QCOMPARE(survey.meanAccuracyMeters.value(), 1.234);
    QCOMPARE(error, GPSConnectionError::DeviceError);
    QCOMPARE(configurationDetail, QStringLiteral("Settings may be saved; reconnect failed"));
}

void GPSProviderTest::_surveyReportProjection_data()
{
    QTest::addColumn<GPSSurveyReport>("report");
    QTest::addColumn<bool>("coordinateValid");

    QTest::newRow("unknown") << GPSSurveyReport{} << false;
    QTest::newRow("fixed") << GPSSurveyReport{.position = {.latitudeDegrees = -47.123,
                                                           .longitudeDegrees = 128.456,
                                                           .altitudeMeters = 512.25f},
                                              .meanAccuracyMeters = 1.234,
                                              .duration = std::chrono::seconds(4294967295LL),
                                              .valid = true,
                                              .active = false}
                           << true;
    QTest::newRow("survey-zero-accuracy")
        << GPSSurveyReport{.position = {.latitudeDegrees = 47, .longitudeDegrees = -8, .altitudeMeters = -30},
                           .meanAccuracyMeters = 0,
                           .duration = std::chrono::seconds(180),
                           .valid = false,
                           .active = true}
        << true;
    QTest::newRow("valid-active-unknown-accuracy")
        << GPSSurveyReport{.position = {.latitudeDegrees = 0, .longitudeDegrees = 0}, .valid = true, .active = true}
        << true;
    QTest::newRow("invalid-latitude") << GPSSurveyReport{.position = {.latitudeDegrees = 91, .longitudeDegrees = 8}}
                                      << false;
    QTest::newRow("invalid-longitude") << GPSSurveyReport{.position = {.latitudeDegrees = 47, .longitudeDegrees = -181}}
                                       << false;
}

void GPSProviderTest::_surveyReportProjection()
{
    QFETCH(GPSSurveyReport, report);
    QFETCH(bool, coordinateValid);
    GPSProvider provider({}, GPSType::ublox, {});
    QSignalSpy reports(&provider, &GPSProvider::surveyInStatus);

    provider._handleSurveyIn(report);

    QCOMPARE(reports.size(), 1);
    const auto status = qvariant_cast<GPSSurveyInStatus>(reports.first().first());
    QCOMPARE(status.coordinate.isValid(), coordinateValid);
    if (coordinateValid) {
        QCOMPARE(status.coordinate.latitude(), report.position.latitudeDegrees);
        QCOMPARE(status.coordinate.longitude(), report.position.longitudeDegrees);
        QCOMPARE(status.coordinate.type(), QGeoCoordinate::Coordinate2D);
    }
    QVERIFY(std::isnan(status.coordinate.altitude()));
    if (std::isnan(report.position.altitudeMeters)) {
        QVERIFY(std::isnan(status.altitudeEllipsoidMeters));
    } else {
        QCOMPARE(status.altitudeEllipsoidMeters, report.position.altitudeMeters);
    }
    QCOMPARE(status.altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(status.meanAccuracyMeters, report.meanAccuracyMeters);
    QCOMPARE(status.duration, report.duration);
    QCOMPARE(status.valid, report.valid);
    QCOMPARE(status.active, report.active);
}

namespace {
struct TransportTrace
{
    QThread* constructedOn = nullptr;
    QThread* openedOn = nullptr;
    QThread* destroyedOn = nullptr;
    std::weak_ptr<int> factoryLifetime;
    bool factoryAliveDuringDestruction = false;
};

class TestTransport : public GPSTransport
{
public:
    TestTransport(const std::atomic_bool& requestStop, TransportTrace& trace, std::function<void()> stop,
                  bool openResult, bool cancelInOpen)
        : GPSTransport(requestStop)
        , _trace(trace)
        , _stop(stop)
        , _openResult(openResult)
        , _cancelInOpen(cancelInOpen)
    {
        _trace.constructedOn = QThread::currentThread();
    }

    ~TestTransport() override
    {
        _trace.destroyedOn = QThread::currentThread();
        _trace.factoryAliveDuringDestruction = !_trace.factoryLifetime.expired();
    }

    GPSOpenResult open() override
    {
        _trace.openedOn = QThread::currentThread();
        // Stop before receiver configuration; this test exercises transport ownership only.
        if (_cancelInOpen) {
            _stop();
        }
        return {_openResult ? GPSOpenStatus::Opened : GPSOpenStatus::Error};
    }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t*, int, int) override { return {GPSReadStatus::Error}; }

    GPSWriteResult writeBounded(const uint8_t*, int, QDeadlineTimer) override { return {GPSWriteStatus::Error}; }

    bool setBaudrate(unsigned) override { return true; }

private:
    TransportTrace& _trace;
    std::function<void()> _stop;
    bool _openResult;
    bool _cancelInOpen;
};
}  // namespace

void GPSProviderTest::_transportLifetimeStaysOnWorker_data()
{
    QTest::addColumn<bool>("openResult");
    QTest::addColumn<bool>("cancelInOpen");
    QTest::newRow("opened") << true << true;
    QTest::newRow("cancelled-during-open") << false << true;
    QTest::newRow("open-failed") << false << false;
}

void GPSProviderTest::_transportLifetimeStaysOnWorker()
{
    QFETCH(bool, openResult);
    QFETCH(bool, cancelInOpen);
    TransportTrace trace;
    auto lifetime = std::make_shared<int>(0);
    trace.factoryLifetime = lifetime;
    std::function<void()> stopProvider;
    GPSProvider provider(
        [&, lifetime = std::move(lifetime)](const std::atomic_bool& requestStop) {
            return std::make_unique<TestTransport>(requestStop, trace, stopProvider, openResult, cancelInOpen);
        },
        GPSType::ublox, GPSReceiverConfig{});
    stopProvider = [&provider]() { provider.stop(); };
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QCOMPARE(trace.openedOn, &provider);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(trace.factoryAliveDuringDestruction);
    QVERIFY(trace.factoryLifetime.expired());
    QCOMPARE(errors.size(), cancelInOpen ? 0 : 1);
    if (!cancelInOpen) {
        QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
    }
}

void GPSProviderTest::_missingTransportReportsOpenFailure_data()
{
    QTest::addColumn<bool>("hasFactory");
    QTest::newRow("empty-factory") << false;
    QTest::newRow("null-transport") << true;
}

void GPSProviderTest::_missingTransportReportsOpenFailure()
{
    QFETCH(bool, hasFactory);
    GPSProvider::TransportFactory factory;
    if (hasFactory) {
        factory = [](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; };
    }
    GPSProvider provider(std::move(factory), GPSType::ublox, GPSReceiverConfig{});
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(errors.count(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
}

void GPSProviderTest::_cancelledProviderDoesNotCreateTransport()
{
    bool created = false;
    GPSProvider provider(
        [&](const std::atomic_bool&) {
            created = true;
            return std::unique_ptr<GPSTransport>{};
        },
        GPSType::ublox, GPSReceiverConfig{});
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.stop();
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!created);
    QVERIFY(errors.isEmpty());
}

UT_REGISTER_TEST(GPSProviderTest, TestLabel::Unit)

namespace {
class FemtoAckTransport : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSWriteResult writeBounded(const uint8_t* bytes, int size, QDeadlineTimer deadline) override
    {
        if (isCancelled() || deadline.hasExpired()) {
            return {isCancelled() ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut};
        }
        const QByteArray command(reinterpret_cast<const char*>(bytes), size);
        _reply = '<' + command.split(' ').first().trimmed() + " OK";
        _reply.append(char(0));
        return {GPSWriteStatus::Completed, size, size};
    }

    GPSReadResult read(uint8_t* bytes, int size, int) override
    {
        if (_reply.isEmpty()) {
            return {GPSReadStatus::Error, 0, QStringLiteral("Scripted receiver connection lost")};
        }
        const auto count = qMin(size, static_cast<int>(_reply.size()));
        std::memcpy(bytes, _reply.constData(), count);
        _reply.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

private:
    QByteArray _reply;
};

class ExpiringSatelliteTransport : public GPSTransport
{
public:
    static constexpr int POSITION_AT_MS = 2000;

    ExpiringSatelliteTransport(const std::atomic_bool& stop, std::atomic<qint64>& lastPositionAtMs)
        : GPSTransport(stop)
        , _lastPositionAtMs(lastPositionAtMs)
    {}

    GPSOpenResult open() override
    {
        _elapsed.start();
        _pending = "$GPGSV,1,1,01,01,10,20,30*79\r\n" + position();
        _lastPositionAtMs = 0;
        return {GPSOpenStatus::Opened};
    }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSReadResult read(uint8_t* bytes, int size, int timeoutMs) override
    {
        if (_pending.isEmpty() && timeoutMs > 0) {
            const qint64 untilPosition =
                _sentPosition ? timeoutMs : qMax(qint64{0}, POSITION_AT_MS - _elapsed.elapsed());
            QThread::msleep(static_cast<unsigned long>(qMin(qint64{timeoutMs}, untilPosition)));
        }
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (!_sentPosition && _elapsed.elapsed() >= POSITION_AT_MS) {
            _sentPosition = true;
            _pending.append(position());
            _lastPositionAtMs = _elapsed.elapsed();
        }
        if (_pending.isEmpty()) {
            return {GPSReadStatus::TimedOut};
        }
        const int count = qMin(size, static_cast<int>(_pending.size()));
        std::memcpy(bytes, _pending.constData(), static_cast<size_t>(count));
        _pending.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

private:
    static QByteArray position() { return "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"; }

    QElapsedTimer _elapsed;
    QByteArray _pending;
    bool _sentPosition = false;
    std::atomic<qint64>& _lastPositionAtMs;
};

class FixSequenceTransport : public GPSTransport
{
public:
    using GPSTransport::GPSTransport;

    GPSOpenResult open() override
    {
        for (const int quality : {0, 0, 1, 1, 4, 4, 0}) {
            _pending += NMEAUtils::repairChecksum("$GPGGA,123519,4807.038,N,01131.000,E," +
                                                  QByteArray::number(quality) + ",08,0.9,545.4,M,46.9,M,,");
        }
        return {GPSOpenStatus::Opened};
    }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSReadResult read(uint8_t* bytes, int size, int) override
    {
        if (_pending.isEmpty()) {
            return {GPSReadStatus::Cancelled};
        }
        const auto count = qMin(size, static_cast<int>(_pending.size()));
        std::memcpy(bytes, _pending.constData(), static_cast<size_t>(count));
        _pending.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

private:
    QByteArray _pending;
};
}  // namespace

void GPSProviderTest::_positionFixTransitions()
{
    if (!GPSDriver::supportsType(GPSType::passive)) {
        QSKIP("Passive receiver support is disabled");
    }
    for (int session = 0; session < 2; ++session) {
        GPSProvider provider([](const std::atomic_bool& stop) { return std::make_unique<FixSequenceTransport>(stop); },
                             GPSType::passive, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200});
        QSignalSpy fixes(&provider, &GPSProvider::fixTypeChanged);
        provider.start();
        const bool finished = provider.wait(TestTimeout::shortMs());
        if (!finished) {
            provider.stop();
            QVERIFY(provider.wait(TestTimeout::longMs()));
        }
        QVERIFY(finished);
        QCOMPARE(fixes.size(), 4);
        const QList<GPSPositionReport::FixType> expected{
            GPSPositionReport::FixType::NoFix, GPSPositionReport::FixType::Fix3D, GPSPositionReport::FixType::RTKFixed,
            GPSPositionReport::FixType::NoFix};
        for (qsizetype i = 0; i < expected.size(); ++i) {
            QCOMPARE(qvariant_cast<GPSPositionReport::FixType>(fixes[i].first()), expected[i]);
        }
    }
}

void GPSProviderTest::_satelliteExpiryDoesNotRenewLiveness()
{
    if (!GPSDriver::supportsType(GPSType::passive)) {
        QSKIP("Passive receiver support is disabled");
    }
    std::atomic<qint64> lastPositionAtMs = -1;
    GPSProvider provider(
        [&](const std::atomic_bool& stop) {
            return std::make_unique<ExpiringSatelliteTransport>(stop, lastPositionAtMs);
        },
        GPSType::passive, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200});
    std::atomic_bool freshView = false;
    std::atomic_bool expiredView = false;
    QElapsedTimer elapsed;
    connect(
        &provider, &GPSProvider::satelliteInfoUpdate, &provider,
        [&](const GPSSatelliteReport& report) {
            if (report.timestampUs && report.count) {
                freshView = true;
            } else if (!report.timestampUs && freshView.load()) {
                expiredView = true;
            }
        },
        Qt::DirectConnection);
    QSignalSpy fixes(&provider, &GPSProvider::fixTypeChanged);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    elapsed.start();
    provider.start();
    const bool finished = provider.wait(TestTimeout::longMs());
    if (!finished) {
        provider.stop();
        QVERIFY(provider.wait(TestTimeout::longMs()));
    }
    QVERIFY(finished);
    QVERIFY(freshView.load());
    QVERIFY(expiredView.load());
    QCOMPARE(fixes.size(), 1);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    // An expiry credited as new traffic would add another complete inactivity window.
    QVERIFY(lastPositionAtMs.load() >= ExpiringSatelliteTransport::POSITION_AT_MS);
    const qint64 expectedDeadline = lastPositionAtMs.load() + GPSProvider::kUsefulDataTimeoutMs;
    QVERIFY(elapsed.elapsed() >= expectedDeadline);
    QVERIFY(elapsed.elapsed() < expectedDeadline + GPSProvider::kGPSReceiveTimeout);
}

void GPSProviderTest::_ancillaryTraffic_data()
{
    QTest::addColumn<bool>("sendUsage");
    QTest::newRow("rapid-ancillary-then-data") << true;
    QTest::newRow("activity-only-expires") << false;
}

void GPSProviderTest::_ancillaryTraffic()
{
    QFETCH(bool, sendUsage);
    ScriptedSBFReceiver* peer = nullptr;
    QElapsedTimer streamingTime;
    GPSProvider provider(
        [&](const std::atomic_bool& stop) {
            auto transport = std::make_unique<ScriptedSBFReceiver>(stop);
            transport->sendUsage = sendUsage;
            peer = transport.get();
            return transport;
        },
        GPSType::septentrio,
        {.base = {.useFixedBase = true,
                  .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}});
    connect(
        &provider, &GPSProvider::receiverReady, &provider,
        [&] {
            streamingTime.start();
            peer->streaming = true;
        },
        Qt::DirectConnection);
    connect(
        &provider, &GPSProvider::satelliteUsageUpdate, &provider, [&](const auto&) { provider.stop(); },
        Qt::DirectConnection);
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy usage(&provider, &GPSProvider::satelliteUsageUpdate);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    const bool finished = provider.wait(TestTimeout::mediumMs());
    if (!finished) {
        provider.stop();
        provider.wait();
    }
    QVERIFY(finished);
    QCOMPARE(ready.size(), 1);
    QCOMPARE(usage.size(), sendUsage ? 1 : 0);
    QCOMPARE(errors.size(), sendUsage ? 0 : 1);
    if (sendUsage) {
        QCOMPARE(qvariant_cast<GPSSatelliteUsageReport>(usage.first().first()).usedCount, std::optional<int>{12});
    } else {
        QVERIFY(streamingTime.elapsed() >= GPSProvider::kUsefulDataTimeoutMs);
        QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    }
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::newRow("survey") << GPSBaseStationConfig{.surveyInAccMeters = 2, .surveyInDurationSecs = 180};
    QTest::newRow("minimum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 0.0001, .surveyInDurationSecs = 1};
    QTest::newRow("maximum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 429496.7295,
                                                            .surveyInDurationSecs = 4294967295LL};
    QTest::newRow("fixed") << GPSBaseStationConfig{
        .useFixedBase = true,
        .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
        .fixedBaseAccuracyMeters = 1};
    QTest::newRow("fixed-wire-limits") << GPSBaseStationConfig{
        .useFixedBase = true,
        .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 21474836.0f},
        .fixedBaseAccuracyMeters = 429496.71875f};
    QTest::newRow("fixed-unknown-accuracy")
        << GPSBaseStationConfig{.useFixedBase = true,
                                .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                                .fixedBaseAccuracyMeters = 0};
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(GPSBaseStationConfig, config);
    const QString diagnostic =
        QStringLiteral("Receiver read failed (status %1, code %2): Scripted receiver connection lost")
            .arg(static_cast<int>(GPSReadStatus::Error))
            .arg(-EIO);
    expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QRegularExpression::escape(diagnostic)));
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); },
        GPSType::femto, GPSReceiverConfig{.base = config});
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    QSignalSpy surveys(&provider, &GPSProvider::surveyInStatus);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QCOMPARE(ready.size(), 1);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
    if (config.useFixedBase) {
        QCOMPARE(surveys.size(), 1);
        const auto status = qvariant_cast<GPSSurveyInStatus>(surveys.first().first());
        QCOMPARE(status.coordinate.type(), QGeoCoordinate::Coordinate2D);
        QCOMPARE(status.coordinate.latitude(), config.fixedPosition.latitudeDegrees);
        QCOMPARE(status.coordinate.longitude(), config.fixedPosition.longitudeDegrees);
        QVERIFY(std::isnan(status.coordinate.altitude()));
        QCOMPARE(status.altitudeEllipsoidMeters, config.fixedPosition.altitudeMeters);
        QCOMPARE(status.altitudeDatum, GPSAltitudeDatum::Ellipsoid);
        QVERIFY(!status.meanAccuracyMeters.has_value());
        QCOMPARE(status.duration.count(), 0);
        QVERIFY(status.valid);
        QVERIFY(!status.active);
    }
}

void GPSProviderTest::_unsupportedPositionRoleReportsConfigFailure_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::newRow("trimble") << GPSType::trimble;
    QTest::newRow("septentrio") << GPSType::septentrio;
    QTest::newRow("femto") << GPSType::femto;
}

void GPSProviderTest::_unsupportedPositionRoleReportsConfigFailure()
{
    QFETCH(GPSType, type);
    const GPSReceiverConfig config{.role = GPSReceiverConfig::Role::Position};
    const QString error = gpsReceiverConfigError(type, config);
    QVERIFY(!error.isEmpty());
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(error)));
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); }, type,
        config);
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    QObject observer;
    QStringList errorOrder;
    connect(
        &provider, &GPSProvider::configurationError, &observer,
        [&errorOrder](const QString& detail) { errorOrder.append(detail); }, Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::connectionError, &observer,
        [&errorOrder](GPSConnectionError) { errorOrder.append(QStringLiteral("ConfigFailed")); }, Qt::QueuedConnection);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QVERIFY(ready.isEmpty());
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::ConfigFailed);
    QCoreApplication::sendPostedEvents(&observer, QEvent::MetaCall);
    QCOMPARE(errorOrder, (QStringList{error, QStringLiteral("ConfigFailed")}));
}

void GPSProviderTest::_cancelledFactoryDoesNotOpenTransport()
{
    TransportTrace trace;
    std::function<void()> stopProvider;
    GPSProvider provider(
        [&](const std::atomic_bool& requestStop) {
            stopProvider();
            return std::make_unique<TestTransport>(requestStop, trace, []() {}, true, false);
        },
        GPSType::ublox, GPSReceiverConfig{});
    stopProvider = [&provider]() { provider.stop(); };
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QVERIFY(!trace.openedOn);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(errors.isEmpty());
}

#ifndef QGC_NO_SERIAL_LINK
void GPSProviderTest::_finishedReceiverReleasesReservation_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("open-failed") << false;
    QTest::newRow("cancelled-before-start") << true;
}

void GPSProviderTest::_finishedReceiverReleasesReservation()
{
    QFETCH(bool, cancelled);
    QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/gps"), QStringLiteral("gps"), QGCSerialPortInfo::BoardTypeRTKGPS, QString()}};
    SerialPortManager ports(nullptr, [&]() { return inventory; });
    ports.setSinglePortOnly(true);
    QCOMPARE(ports.availablePorts().size(), 1);
    auto reservation = ports.reservePort(QStringLiteral("/test/gps"));
    QVERIFY(reservation);
    GPSProvider provider(
        [reservation = std::move(reservation)](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; },
        GPSType::ublox, GPSReceiverConfig{});
    if (cancelled) {
        provider.stop();
    }
    QVERIFY(!ports.canReservePort(QStringLiteral("/test/mavlink")));
    inventory.clear();
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!ports.anyPortReserved());
    QVERIFY(ports.reservePort(QStringLiteral("/test/mavlink")));
    QTRY_VERIFY_WITH_TIMEOUT(ports.availablePorts().isEmpty(), TestTimeout::mediumMs());
}
#endif
