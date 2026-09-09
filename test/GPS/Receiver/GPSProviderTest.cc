#include "GPSProviderTest.h"

#include <QtTest/QSignalSpy>

#include <algorithm>
#include <cstring>

#include "GPSProvider.h"
#include "GPSTransport.h"

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

    bool open() override
    {
        _trace.openedOn = QThread::currentThread();
        // Stop before receiver configuration; this test exercises transport ownership only.
        if (_cancelInOpen) {
            _stop();
        }
        return _openResult;
    }

    bool fatalError() const override { return false; }

    int read(uint8_t*, int, int) override { return -1; }

    int write(const uint8_t*, int) override { return -1; }

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
    GPSProvider provider(
        [&, lifetime = std::move(lifetime)](const std::atomic_bool& requestStop) {
            return std::make_unique<TestTransport>(
                requestStop, trace, [&provider]() { provider.stop(); }, openResult, cancelInOpen);
        },
        GPSType::u_blox, GPSReceiverConfig{});
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
    GPSProvider provider(std::move(factory), GPSType::u_blox, GPSReceiverConfig{});
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
        GPSType::u_blox, GPSReceiverConfig{});
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

    bool open() override { return true; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    int write(const uint8_t* bytes, int size) override
    {
        const QByteArray command(reinterpret_cast<const char*>(bytes), size);
        _reply = '<' + command.split(' ').first().trimmed() + " OK";
        _reply.append(char(0));
        return size;
    }

    int read(uint8_t* bytes, int size, int) override
    {
        if (_reply.isEmpty()) {
            return -1;
        }
        const auto count = qMin(size, static_cast<int>(_reply.size()));
        std::memcpy(bytes, _reply.constData(), count);
        _reply.remove(0, count);
        return count;
    }

private:
    QByteArray _reply;
};
}  // namespace

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss_data()
{
    QTest::addColumn<bool>("validConfig");
    QTest::newRow("configured-then-lost") << true;
    QTest::newRow("rejected-configuration") << false;
}

void GPSProviderTest::_configuredReceiverReportsReadyThenLoss()
{
    QFETCH(bool, validConfig);
    GPSReceiverConfig config;
    config.base.surveyInAccMeters = 1.0;
    config.base.surveyInDurationSecs = 30;
    if (!validConfig) {
        config.headingOffsetDeg = 181.0f;
    }
    GPSProvider provider(
        [](const std::atomic_bool& requestStop) { return std::make_unique<FemtoAckTransport>(requestStop); },
        GPSType::femto, config);
    QSignalSpy ready(&provider, &GPSProvider::receiverReady);
    QSignalSpy reports(&provider, &GPSProvider::configurationReported);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    QCOMPARE(ready.size(), validConfig ? 1 : 0);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()),
             validConfig ? GPSConnectionError::DeviceError : GPSConnectionError::ConfigFailed);
    QCOMPARE(reports.size(), 2);
    const auto requested = reports.first().first().value<GPSConfigurationReport>();
    const auto finalized = reports.last().first().value<GPSConfigurationReport>();
    QCOMPARE(requested.settings.isEmpty(), validConfig);
    QVERIFY(requested.monotonicTimestampUs > 0);
    QVERIFY(finalized.monotonicTimestampUs >= requested.monotonicTimestampUs);
    QCOMPARE(finalized.settings.size(), requested.settings.size());
    for (const auto& setting : requested.settings) {
        QCOMPARE(setting.requestState, GPSSettingReport::RequestState::Requested);
        QCOMPARE(setting.readbackState, GPSSettingReport::ReadbackState::Unverifiable);
    }
    if (!validConfig) {
        QVERIFY(
            std::any_of(finalized.settings.cbegin(), finalized.settings.cend(), [](const GPSSettingReport& setting) {
                return setting.requestState == GPSSettingReport::RequestState::Rejected;
            }));
    }
}

void GPSProviderTest::_cancelledFactoryDoesNotOpenTransport()
{
    TransportTrace trace;
    GPSProvider provider(
        [&](const std::atomic_bool& requestStop) {
            provider.stop();
            return std::make_unique<TestTransport>(requestStop, trace, []() {}, true, false);
        },
        GPSType::u_blox, GPSReceiverConfig{});
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QVERIFY(!trace.openedOn);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(errors.isEmpty());
}
