#include "GPSProviderTest.h"

#include <cstring>

#include <QtTest/QSignalSpy>

#include "GPSProvider.h"
#include "GPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

Q_DECLARE_METATYPE(GPSReceiverConfig)

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

    OpenResult open() override
    {
        _trace.openedOn = QThread::currentThread();
        // Stop before receiver configuration; this test exercises transport ownership only.
        if (_cancelInOpen) {
            _stop();
        }
        return {_openResult ? OpenStatus::Opened : OpenStatus::Error};
    }

    bool fatalError() const override { return false; }

    ReadResult read(uint8_t*, int, int) override { return {ReadStatus::Error}; }

    WriteResult write(const uint8_t*, int) override { return {WriteStatus::Error}; }

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
        GPSReceiverType::ublox, GPSReceiverConfig{});
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
    GPSProvider provider(std::move(factory), GPSReceiverType::ublox, GPSReceiverConfig{});
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
        GPSReceiverType::ublox, GPSReceiverConfig{});
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

    OpenResult open() override { return {OpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    WriteResult write(const uint8_t* bytes, int size) override
    {
        const QByteArray command(reinterpret_cast<const char*>(bytes), size);
        _reply = '<' + command.split(' ').first().trimmed() + " OK";
        _reply.append(char(0));
        return {WriteStatus::Completed, size, size, 0};
    }

    ReadResult read(uint8_t* bytes, int size, int) override
    {
        if (_reply.isEmpty()) {
            return {ReadStatus::Error};
        }
        const auto count = qMin(size, static_cast<int>(_reply.size()));
        std::memcpy(bytes, _reply.constData(), count);
        _reply.remove(0, count);
        return {ReadStatus::Data, count};
    }

private:
    QByteArray _reply;
};
}  // namespace

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::newRow("survey") << GPSReceiverConfig{.base = GPSSurveyInConfig{2, std::chrono::seconds(180)}};
    QTest::newRow("minimum-survey") << GPSReceiverConfig{.base = GPSSurveyInConfig{0.0001, std::chrono::seconds(1)}};
    QTest::newRow("maximum-survey") << GPSReceiverConfig{
        .base = GPSSurveyInConfig{429496.7295, std::chrono::seconds(4294967295LL)}};
    QTest::newRow("fixed") << GPSReceiverConfig{.base = GPSFixedBaseConfig{QGeoCoordinate(47, 8), 500, 1}};
    QTest::newRow("fixed-wire-limits") << GPSReceiverConfig{
        .base = GPSFixedBaseConfig{QGeoCoordinate(47, 8), 21474836.0f, 429496.71875f}};
    QTest::newRow("fixed-unknown-accuracy")
        << GPSReceiverConfig{.base = GPSFixedBaseConfig{QGeoCoordinate(47, 8), 500, 0}};
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(GPSReceiverConfig, config);
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); },
        GPSReceiverType::femto, config);
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
        GPSReceiverType::ublox, GPSReceiverConfig{});
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
        GPSReceiverType::ublox, GPSReceiverConfig{});
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
