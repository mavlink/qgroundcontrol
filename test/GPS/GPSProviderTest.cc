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
};

class TestTransport : public GPSTransport
{
public:
    TestTransport(TransportTrace& trace, std::atomic_bool& stop, bool openResult)
        : _trace(trace), _stop(stop), _openResult(openResult)
    {
        _trace.constructedOn = QThread::currentThread();
    }

    ~TestTransport() override { _trace.destroyedOn = QThread::currentThread(); }

    bool open() override
    {
        _trace.openedOn = QThread::currentThread();
        // Stop before receiver configuration; this test exercises transport ownership only.
        _stop = true;
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
};
}  // namespace

void GPSProviderTest::_transportLifetimeStaysOnWorker_data()
{
    QTest::addColumn<bool>("openResult");
    QTest::newRow("opened") << true;
    QTest::newRow("cancelled-during-open") << false;
}

void GPSProviderTest::_transportLifetimeStaysOnWorker()
{
    QFETCH(bool, openResult);
    TransportTrace trace;
    std::atomic_bool stop = false;
    GPSProvider provider(
        [&](const std::atomic_bool&) { return std::make_unique<TestTransport>(trace, stop, openResult); },
        GPSType::u_blox, GPSReceiverConfig{}, stop);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QVERIFY(provider.wait(TestTimeout::shortMs()));
    QCOMPARE(trace.constructedOn, &provider);
    QCOMPARE(trace.openedOn, &provider);
    QCOMPARE(trace.destroyedOn, &provider);
    QVERIFY(errors.isEmpty());
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
