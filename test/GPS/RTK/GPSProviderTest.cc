#include "GPSProviderTest.h"

#include <cstring>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtTest/QSignalSpy>

#include "GPSProvider.h"
#include "GPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

Q_DECLARE_METATYPE(GPSBaseStationConfig)

void GPSProviderTest::_queuedPayloadsOwnSnapshots()
{
    GPSProvider provider({}, GPSType::ublox, {});
    for (const auto name : {"satellite_info_s", "sensor_gps_s", "GPSConnectionError", "GPSSurveyInStatus"}) {
        QVERIFY2(QMetaType::fromName(name).isValid(), name);
    }
    satellite_info_s satellites{};
    sensor_gps_s position{};
    GPSSurveyInStatus survey;
    GPSConnectionError error = GPSConnectionError::None;
    QObject receiver;
    connect(
        &provider, &GPSProvider::satelliteInfoUpdate, &receiver,
        [&](const satellite_info_s& value) { satellites = value; }, Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::sensorGpsUpdate, &receiver, [&](const sensor_gps_s& value) { position = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::surveyInStatus, &receiver, [&](const GPSSurveyInStatus& value) { survey = value; },
        Qt::QueuedConnection);
    connect(
        &provider, &GPSProvider::connectionError, &receiver, [&](GPSConnectionError value) { error = value; },
        Qt::QueuedConnection);
    auto worker = std::unique_ptr<QThread>(QThread::create([&]() {
        satellite_info_s snapshot{};
        snapshot.count = 1;
        snapshot.used[0] = 1;
        emit provider.satelliteInfoUpdate(snapshot);
        snapshot.used[0] = 0;
        sensor_gps_s fix{};
        fix.latitude_deg = 47;
        emit provider.sensorGpsUpdate(fix);
        fix.latitude_deg = 0;
        GPSSurveyInStatus progress;
        progress.duration = std::chrono::seconds(4294967295LL);
        progress.meanAccuracyMeters = 1.234;
        emit provider.surveyInStatus(progress);
        progress.meanAccuracyMeters = 0;
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
    QCOMPARE(satellites.used[0], 1);
    QCOMPARE(position.latitude_deg, 47);
    QCOMPARE(survey.duration.count(), 4294967295LL);
    QCOMPARE(survey.meanAccuracyMeters.value(), 1.234);
    QCOMPARE(error, GPSConnectionError::DeviceError);
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

    GPSWriteResult write(const uint8_t*, int) override { return {GPSWriteStatus::Error}; }

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
        GPSType::ublox, GPSBaseStationConfig{});
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
    GPSProvider provider(std::move(factory), GPSType::ublox, GPSBaseStationConfig{});
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
        GPSType::ublox, GPSBaseStationConfig{});
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

    GPSWriteResult write(const uint8_t* bytes, int size) override
    {
        const QByteArray command(reinterpret_cast<const char*>(bytes), size);
        _reply = '<' + command.split(' ').first().trimmed() + " OK";
        _reply.append(char(0));
        return {GPSWriteStatus::Completed, size, size};
    }

    GPSReadResult read(uint8_t* bytes, int size, int) override
    {
        if (_reply.isEmpty()) {
            return {GPSReadStatus::Error};
        }
        const auto count = qMin(size, static_cast<int>(_reply.size()));
        std::memcpy(bytes, _reply.constData(), count);
        _reply.remove(0, count);
        return {GPSReadStatus::Data, count};
    }

private:
    QByteArray _reply;
};
}  // namespace

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::newRow("survey") << GPSBaseStationConfig{.surveyInAccMeters = 2, .surveyInDurationSecs = 180};
    QTest::newRow("minimum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 0.0001, .surveyInDurationSecs = 1};
    QTest::newRow("maximum-survey") << GPSBaseStationConfig{.surveyInAccMeters = 429496.7295,
                                                            .surveyInDurationSecs = 4294967295LL};
    QTest::newRow("fixed") << GPSBaseStationConfig{.useFixedBase = true,
                                                   .fixedBaseLatitude = 47,
                                                   .fixedBaseLongitude = 8,
                                                   .fixedBaseAltitudeMeters = 500,
                                                   .fixedBaseAccuracyMeters = 1};
    QTest::newRow("fixed-wire-limits") << GPSBaseStationConfig{.useFixedBase = true,
                                                               .fixedBaseLatitude = 47,
                                                               .fixedBaseLongitude = 8,
                                                               .fixedBaseAltitudeMeters = 21474836.0f,
                                                               .fixedBaseAccuracyMeters = 429496.71875f};
    QTest::newRow("fixed-unknown-accuracy") << GPSBaseStationConfig{.useFixedBase = true,
                                                                    .fixedBaseLatitude = 47,
                                                                    .fixedBaseLongitude = 8,
                                                                    .fixedBaseAltitudeMeters = 500,
                                                                    .fixedBaseAccuracyMeters = 0};
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(GPSBaseStationConfig, config);
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); },
        GPSType::femto, config);
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    QCOMPARE(ready.size(), 1);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::DeviceError);
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
        GPSType::ublox, GPSBaseStationConfig{});
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
        GPSType::ublox, GPSBaseStationConfig{});
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
