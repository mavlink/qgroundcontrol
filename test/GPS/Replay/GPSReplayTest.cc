#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <thread>

#include "GPSCorrectionRouter.h"
#include "GPSReadTimestamp.h"
#include "GPSRecordingController.h"
#include "GPSRecordingDevice.h"
#include "GPSRecordingTransport.h"
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
    auto& transport = *static_cast<GPSTransport*>(user);
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
        auto original = std::make_unique<GPSReplayTransport>(clock, stop, std::move(trace), fragment);
        auto* originalTrace = original.get();
        auto recording = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        GPSReceiverConfig intent{.role = GPSReceiverConfig::Role::Position, .base = {}};
        auto recordedStream =
            std::make_shared<GPSRecordingStream>(recording, GPSRecordingMetadata::forReceiver(intent, GPSType::u_blox));
        QVERIFY(recording->start());
        GPSRecordingTransport transport(std::move(original), stop, recordedStream);
        QVERIFY(transport.open());
        sensor_gps_s position{};
        GPSDriverUBX driver(GPSHelper::Interface::UART, nativeCallback, &transport, &position, nullptr, {});
        GPSHelper::GPSConfig config{};
        config.output_mode = GPSHelper::OutputMode::GPS;
        unsigned baudrate = 115200;
        recordedStream->configurationStarted();
        const auto configured = driver.configure(baudrate, config);
        recordedStream->configurationFinished(configured);
        QVERIFY2(configured == 0, qPrintable(originalTrace->failure()));
        QVERIFY(driver.receiverReady());
        QVERIFY2(driver.receive(500) > 0, qPrintable(originalTrace->failure()));
        QVERIFY2(originalTrace->complete(), qPrintable(originalTrace->failure()));
        QCOMPARE(position.latitude_deg, 47.3977);
        QCOMPARE(position.longitude_deg, 8.5456);
        QCOMPARE(position.altitude_msl_m, 450.0);
        QCOMPARE(position.satellites_used, uint8_t(14));
        QCOMPARE(position.fix_type, uint8_t(3));
        QCOMPARE(clock.nowUs(), quint64(84002));
        recording->stop();
        const QByteArray captured = recording->exportJson();
        GPSReplayTrace exported;
        QVERIFY2(GPSReplayTrace::fromJson(captured, exported, error), qPrintable(error));
        QCOMPARE(exported.profile.value("configured").toBool(), true);
        gps_test_time = 1;
        GPSReplayTransport roundTrip(clock, stop, std::move(exported), fragment);
        QVERIFY(roundTrip.open());
        sensor_gps_s decoded{};
        GPSDriverUBX replayed(GPSHelper::Interface::UART, nativeCallback, &roundTrip, &decoded, nullptr, {});
        baudrate = 115200;
        QVERIFY2(replayed.configure(baudrate, config) == 0, qPrintable(roundTrip.failure()));
        QVERIFY2(replayed.receive(500) > 0, qPrintable(roundTrip.failure()));
        QVERIFY2(roundTrip.complete(), qPrintable(roundTrip.failure()));
        QCOMPARE(decoded.latitude_deg, position.latitude_deg);
        QCOMPARE(decoded.longitude_deg, position.longitude_deg);
        QCOMPARE(decoded.satellites_used, position.satellites_used);
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

    void recordingNmeaExportAndResume()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        GPSReplayClock clock;
        clock.advanceTo(1000000);
        auto buffer = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        GPSRecordingController controller(nullptr, buffer);
        GPSRecordingMetadata metadata;
        metadata.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        metadata.transport = GPSRecordingMetadata::Transport::Serial;
        metadata.initialBaud = 9600;
        auto stream = std::make_shared<GPSRecordingStream>(buffer, metadata);
        stream->opened(true);
        ReplayInput input;
        GPSRecordingDevice tap(&input, stream);
        NMEAStreamSplitter splitter(&tap);
        const QByteArray sentence = "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
        QVERIFY(controller.start());
        QVERIFY(!controller.exportRecording(QUrl::fromLocalFile(directory.filePath("active.json"))));
        clock.advanceBy(1000);
        input.feed(sentence.first(10), 42);
        clock.advanceBy(1000);
        input.feed(sentence.sliced(10), 43);
        const auto live = splitter.positionDevice()->readLine();
        QCOMPARE(live, sentence);
        QCOMPARE(GPSReadTimestamp::from(splitter.positionDevice()), quint64(42));
        stream->record(GPSRecordingBuffer::Kind::Disconnect, {}, -1);
        tap.close();
        controller.stop();
        const QString path = directory.filePath("nmea.json");
        QVERIFY(controller.exportRecording(QUrl::fromLocalFile(path)));
        QCOMPARE(controller.lastExportPath(), path);
        QVERIFY(controller.errorString().isEmpty());
        GPSReplayTrace trace;
        QString error;
        QVERIFY2(GPSReplayTrace::load(path, trace, error), qPrintable(error));
        QCOMPARE(trace.profile.value("baud").toInt(), 9600);
        QFile saved(path);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const auto events = QJsonDocument::fromJson(saved.readAll()).object().value("events").toArray();
        QCOMPARE(events.at(1).toObject().value("resumed").toBool(), true);
        std::atomic_bool stop = false;
        GPSReplayClock replayClock;
        GPSReplayTransport replay(replayClock, stop, std::move(trace), 5);
        QVERIFY(replay.open());
        QByteArray received;
        uint8_t bytes[32];
        while (!replay.complete()) {
            const auto count = replay.read(bytes, sizeof(bytes), 100);
            if (count > 0) {
                received.append(reinterpret_cast<char*>(bytes), count);
            }
        }
        QCOMPARE(received, live);
        QGeoPositionInfo position;
        bool fix = false;
        QtNmeaDecoder decoder;
        QVERIFY(decoder.decode(received, position, fix));
        QVERIFY(fix);
        QCOMPARE(position.coordinate().altitude(), 61.7);
        QCOMPARE(replayClock.nowUs(), quint64(2001));
    }

    void recordingFaultsAndMetadata()
    {
        using K = GPSReplayEvent::Kind;
        GPSReplayTrace fixture{{{1, K::OpenError},
                                {2, K::Open},
                                {3, K::BaudError, {}, 9600},
                                {4, K::Baud, {}, 115200},
                                {5, K::WriteError, "command", 2}}};
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        auto buffer = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        GPSReceiverConfig config;
        config.constellationMask = 17;
        config.dynamicModel = 4;
        config.outputRateHz = 5;
        config.headingOffsetDeg = 12.5f;
        config.base.useFixedBase = true;
        config.base.fixedBaseLatitude = 47.3977;
        config.base.fixedBaseLongitude = 8.5456;
        config.base.surveyInDurationSecs = 60;
        auto stream =
            std::make_shared<GPSRecordingStream>(buffer, GPSRecordingMetadata::forReceiver(config, GPSType::u_blox));
        QVERIFY(buffer->start());
        {
            GPSRecordingTransport transport(std::make_unique<GPSReplayTransport>(clock, stop, fixture), stop, stream);
            QVERIFY(!transport.open());
            QVERIFY(transport.open());
            QVERIFY(!transport.setBaudrate(9600));
            QVERIFY(transport.setBaudrate(115200));
            QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("command"), 7), 2);
        }
        buffer->stop();
        GPSReplayTrace exported;
        QString error;
        QVERIFY2(GPSReplayTrace::fromJson(buffer->exportJson(), exported, error), qPrintable(error));
        QCOMPARE(exported.profile.value("dynamic_model").toInt(), config.dynamicModel);
        QCOMPARE(exported.profile.value("constellation_mask").toInt(), config.constellationMask);
        QCOMPARE(exported.profile.value("output_rate_hz").toInt(), config.outputRateHz);
        QCOMPARE(exported.profile.value("heading_offset_deg").toDouble(), double(config.headingOffsetDeg));
        QCOMPARE(exported.profile.value("base").toObject().value("latitude").toDouble(), config.base.fixedBaseLatitude);
        GPSReplayTransport replay(clock, stop, std::move(exported));
        QVERIFY(!replay.open());
        QVERIFY(replay.open());
        QVERIFY(!replay.setBaudrate(9600));
        QVERIFY(replay.setBaudrate(115200));
        QCOMPARE(replay.write(reinterpret_cast<const uint8_t*>("command"), 7), 2);
        QVERIFY(replay.complete());
    }

    void recordingBoundsAndThreadRetirement()
    {
        auto buffer = std::make_shared<GPSRecordingBuffer>();
        auto controller = std::make_unique<GPSRecordingController>(nullptr, buffer);
        QVERIFY(!controller->hasRecording());
        QVERIFY(controller->start());
        std::vector<std::thread> writers;
        for (int writerIndex = 0; writerIndex < 4; ++writerIndex) {
            writers.emplace_back([buffer]() {
                GPSRecordingStream stream(buffer, {});
                stream.opened(true);
                for (int chunkIndex = 0; chunkIndex < 500; ++chunkIndex) {
                    stream.record(GPSRecordingBuffer::Kind::Rx, QByteArray(4096, 'x'));
                }
            });
        }
        for (auto& writer : writers) {
            writer.join();
        }
        QVERIFY(!controller->recording());
        QVERIFY(controller->limitReached());
        QVERIFY(controller->eventCount() <= GPSRecordingBuffer::MAX_EVENTS);
        QVERIFY(controller->bytesRecorded() <= GPSRecordingBuffer::MAX_STORAGE_BYTES / 2);
        const auto frozen = buffer->exportJson();
        QVERIFY(frozen.size() < 4 * 1024 * 1024);
        GPSReplayTrace selected;
        QString error;
        QVERIFY2(GPSReplayTrace::fromJson(frozen, selected, error), qPrintable(error));
        QVERIFY(selected.streamId > 0);
        QVERIFY(!GPSReplayTrace::fromJson(frozen, selected, error, 9999));
        controller.reset();
        // Retired worker tokens may outlive the controller, but cannot append after its destruction.
        GPSRecordingStream retired(buffer, {});
        retired.opened(true);
        retired.record(GPSRecordingBuffer::Kind::Rx, "late");
        QCOMPARE(buffer->exportJson(), frozen);
        QVERIFY(buffer->start());
        retired.record(GPSRecordingBuffer::Kind::Rx, "new capture");
        buffer->stop();
        QVERIFY2(GPSReplayTrace::fromJson(buffer->exportJson(), selected, error), qPrintable(error));
        QCOMPARE(selected.events.first().kind, GPSReplayEvent::Kind::Open);
        QCOMPARE(selected.events.last().bytes, QByteArray("new capture"));
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
