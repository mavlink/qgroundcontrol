#include "GPSProviderTest.h"

#include <QtTest/QSignalSpy>

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
    TestTransport(TransportTrace& trace, std::atomic_bool& stop, bool openResult, bool cancelInOpen)
        : _trace(trace), _stop(stop), _openResult(openResult), _cancelInOpen(cancelInOpen)
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
        _stop = _cancelInOpen;
        return _openResult;
    }

    bool fatalError() const override { return false; }

    int read(uint8_t*, int, int) override { return -1; }

    int write(const uint8_t*, int) override { return -1; }

    bool setBaudrate(unsigned) override { return true; }

private:
    TransportTrace& _trace;
    std::atomic_bool& _stop;
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
    std::atomic_bool stop = false;
    GPSProvider provider(
        [&, lifetime = std::move(lifetime)](const std::atomic_bool&) {
            return std::make_unique<TestTransport>(trace, stop, openResult, cancelInOpen);
        },
        GPSType::u_blox, GPSReceiverConfig{}, stop);
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
    std::atomic_bool stop = false;
    GPSProvider::TransportFactory factory;
    if (hasFactory) {
        factory = [](const std::atomic_bool&) { return std::unique_ptr<GPSTransport>{}; };
    }
    GPSProvider provider(std::move(factory), GPSType::u_blox, GPSReceiverConfig{}, stop);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(errors.count(), 1);
    QCOMPARE(qvariant_cast<GPSConnectionError>(errors.first().first()), GPSConnectionError::OpenFailed);
}

void GPSProviderTest::_cancelledProviderDoesNotCreateTransport()
{
    std::atomic_bool stop = true;
    bool created = false;
    GPSProvider provider(
        [&](const std::atomic_bool&) {
            created = true;
            return std::unique_ptr<GPSTransport>{};
        },
        GPSType::u_blox, GPSReceiverConfig{}, stop);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QVERIFY(!created);
    QVERIFY(errors.isEmpty());
}

UT_REGISTER_TEST(GPSProviderTest, TestLabel::Unit)
