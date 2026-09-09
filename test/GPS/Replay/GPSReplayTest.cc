#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>

#include "GPSCorrectionRouter.h"
#include "GPSReadTimestamp.h"
#include "GPSReplayTransport.h"
#include "NMEAStreamSplitter.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPSession.h"
#include "RTCMParser.h"
#include "ubx.h"

namespace {
class ReplayInput : public QIODevice, public GPSReadTimestamp
{
public:
    ReplayInput() { open(QIODevice::ReadOnly | QIODevice::Unbuffered); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return _bytes.size() + QIODevice::bytesAvailable(); }

    quint64 lastReadTimestampUs() const override { return _timestamp; }

    void feed(const QByteArray& bytes, quint64 timestamp)
    {
        _timestamp = timestamp;
        _bytes += bytes;
        emit readyRead();
    }

protected:
    qint64 readData(char* bytes, qint64 size) override
    {
        const auto count = std::min(size, qint64(_bytes.size()));
        std::memcpy(bytes, _bytes.constData(), static_cast<size_t>(count));
        _bytes.remove(0, count);
        return count;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _bytes;
    quint64 _timestamp = 0;
};

class QtNmeaDecoder : public QNmeaPositionInfoSource
{
public:
    QtNmeaDecoder() : QNmeaPositionInfoSource(RealTimeMode) {}

    bool decode(const QByteArray& bytes, QGeoPositionInfo& position, bool& fix)
    {
        return parsePosInfoFromNmeaData(bytes.constData(), bytes.size(), &position, &fix);
    }
};

int nativeCallback(GPSCallbackType type, void* data, int size, void* user)
{
    auto& transport = *static_cast<GPSReplayTransport*>(user);
    if (type == GPSCallbackType::readDeviceData) {
        int timeout = 0;
        std::memcpy(&timeout, data, sizeof(timeout));
        return transport.read(static_cast<uint8_t*>(data), size, timeout);
    }
    if (type == GPSCallbackType::writeDeviceData) {
        return transport.write(static_cast<const uint8_t*>(data), size);
    }
    if (type == GPSCallbackType::setBaudrate) {
        return transport.setBaudrate(size) ? 0 : -EIO;
    }
    return 0;
}

class ReplayCaster : public NTRIPStream
{
public:
    using NTRIPStream::NTRIPStream;

    void start() override { emit connected(); }

    void stop() override { emit finished(); }

    void sendNMEA(const QByteArray& bytes) override { written += bytes; }

    void fail() { emit error(NTRIPError::SocketError, QStringLiteral("replayed disconnect")); }

    void frame() { emit RTCMDataUpdate(QByteArrayLiteral("valid correction"), 1005); }

    QByteArray written;
};
}  // namespace

class GPSReplayTest : public QObject
{
    Q_OBJECT
private slots:

    void nativePosition_data()
    {
        QTest::addColumn<int>("fragment");
        QTest::newRow("one-byte") << 1;
        QTest::newRow("seven-bytes") << 7;
        QTest::newRow("whole-read") << 4096;
    }

    void nativePosition()
    {
        QFETCH(int, fragment);
        GPSReplayTrace trace;
        QString error;
        QVERIFY2(GPSReplayTrace::load(QStringLiteral(QGC_GPS_REPLAY_FIXTURES "/ubx-native.json"), trace, error),
                 qPrintable(error));
        gps_test_time = 1;
        GPSReplayClock clock(&gps_test_time);
        std::atomic_bool stop = false;
        GPSReplayTransport transport(clock, stop, std::move(trace), fragment);
        QVERIFY(transport.open());
        sensor_gps_s position{};
        GPSDriverUBX driver(GPSHelper::Interface::UART, nativeCallback, &transport, &position, nullptr, {});
        GPSHelper::GPSConfig config{};
        config.output_mode = GPSHelper::OutputMode::GPS;
        unsigned baudrate = 115200;
        const auto configured = driver.configure(baudrate, config);
        QVERIFY2(configured == 0, qPrintable(transport.failure()));
        QVERIFY(driver.receiverReady());
        QVERIFY2(driver.receive(500) > 0, qPrintable(transport.failure()));
        QVERIFY2(transport.complete(), qPrintable(transport.failure()));
        QCOMPARE(position.latitude_deg, 47.3977);
        QCOMPARE(position.longitude_deg, 8.5456);
        QCOMPARE(position.altitude_msl_m, 450.0);
        QCOMPARE(position.satellites_used, uint8_t(14));
        QCOMPARE(position.fix_type, uint8_t(3));
        QCOMPARE(clock.nowUs(), quint64(84002));
        stop = true;
        const auto before = clock.nowUs();
        QVERIFY(driver.receive(500) < 0);
        QCOMPARE(clock.nowUs(), before);
    }

    void nmeaReconnect()
    {
        GPSReplayTrace trace;
        QString error;
        QVERIFY2(GPSReplayTrace::load(QStringLiteral(QGC_GPS_REPLAY_FIXTURES "/nmea-reconnect.json"), trace, error),
                 qPrintable(error));
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSReplayTransport transport(clock, stop, std::move(trace), 3);
        for (int attempt = 0; attempt < 2; ++attempt) {
            QVERIFY(transport.open());
            ReplayInput input;
            NMEAStreamSplitter splitter(&input);
            QtNmeaDecoder decoder;
            int decoded = 0;
            QGeoPositionInfo position;
            quint64 firstReceipt = 0;
            while (transport.remainingEvents() > 0) {
                uint8_t bytes[32];
                const int count = transport.read(bytes, sizeof(bytes), 1000);
                if (count < 0) {
                    input.close();
                    break;
                }
                input.feed(QByteArray(reinterpret_cast<char*>(bytes), count), transport.lastReadTimestampUs());
                auto* output = splitter.positionDevice();
                while (output->canReadLine()) {
                    bool fix = false;
                    const auto line = output->readLine();
                    if (decoder.decode(line, position, fix) && fix) {
                        ++decoded;
                        if (firstReceipt == 0) {
                            firstReceipt = GPSReadTimestamp::from(output);
                        }
                    }
                }
            }
            QCOMPARE(decoded, 2);
            QVERIFY(qAbs(position.coordinate().latitude() - 53.3613366667) < 1e-8);
            QVERIFY(qAbs(position.coordinate().longitude() + 6.50562) < 1e-8);
            QCOMPARE(position.coordinate().altitude(), 61.7);
            QCOMPARE(firstReceipt, attempt == 0 ? quint64(1000000) : quint64(3100000));
        }
        QVERIFY2(transport.complete(), qPrintable(transport.failure()));
    }

    void faultsAndExactWrites()
    {
        using K = GPSReplayEvent::Kind;
        GPSReplayTrace trace{{{1, K::Open},
                              {2, K::Tx, QByteArrayLiteral("command")},
                              {500001, K::Timeout},
                              {600000, K::ReadError, {}, -EIO},
                              {700000, K::Open},
                              {800000, K::WriteError, {}, 2},
                              {900000, K::Open},
                              {1000000, K::Cancel}}};
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSReplayTransport transport(clock, stop, std::move(trace));
        QVERIFY(transport.open());
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("com"), 3), 3);
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("mand"), 4), 4);
        uint8_t bytes[8];
        QCOMPARE(transport.read(bytes, sizeof(bytes), 500), 0);
        QCOMPARE(clock.nowUs(), quint64(500001));
        QCOMPARE(transport.read(bytes, sizeof(bytes), 100), -EIO);
        QVERIFY(transport.fatalError());
        QVERIFY(transport.open());
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("command"), 7), 2);
        QVERIFY(transport.fatalError());
        QVERIFY(transport.open());
        QCOMPARE(transport.read(bytes, sizeof(bytes), 100), -ECANCELED);
        QVERIFY(stop);
        QVERIFY(transport.complete());
    }

    void correctionAndRecoveryUseVirtualTime()
    {
        GPSReplayClock clock;
        clock.advanceTo(1000000);
        ReplayCaster* stream = nullptr;
        NTRIPSession session(
            [&](const NTRIPTransportConfig&, QObject* owner) {
                stream = new ReplayCaster(owner);
                return stream;
            },
            nullptr, [&]() { return clock.nowMs(); });
        GPSCorrectionRouter router(nullptr, [&]() { return clock.nowMs(); });
        int delivered = 0;
        router.setSink(QStringLiteral("recording"), [&](const GPSCorrectionFrame& frame) {
            ++delivered;
            return quint64(frame.data.size());
        });
        connect(&session, &NTRIPSession::streamStarted, &router,
                [&]() { router.beginSourceSession(GPSCorrectionSource::Ntrip, session.sourceId()); });
        connect(&session, &NTRIPSession::streamEnded, &router,
                [&]() { router.endSourceSession(GPSCorrectionSource::Ntrip); });
        GPSCorrectionFrame retired;
        connect(&session, &NTRIPSession::correctionReceived, &router,
                [&](const QByteArray& data, int id, bool filtered, qint64 receivedAtMs) {
                    retired = {GPSCorrectionSource::Ntrip,
                               router.sourceSession(GPSCorrectionSource::Ntrip),
                               receivedAtMs,
                               data,
                               id,
                               true,
                               filtered,
                               session.sourceId()};
                    router.acceptFrame(retired);
                });
        NTRIPTransportConfig config;
        config.host = QStringLiteral("synthetic.invalid");
        config.mountpoint = QStringLiteral("BASE");
        session.start(config);
        stream->frame();
        QCOMPARE(delivered, 1);
        clock.advanceBy(6000000);
        QVERIFY(!router.acceptFrame(retired));
        stream->fail();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(session.state(), NTRIPSession::State::Reconnecting);
        QCOMPARE(session.failedAttempts(), 1);
        session.stop();
        session.start(config);
        QVERIFY(!router.acceptFrame(retired));
        stream->frame();
        QCOMPARE(delivered, 2);
        session.stop();
    }
};

QTEST_GUILESS_MAIN(GPSReplayTest)
#include "GPSReplayTest.moc"
