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
#include "GPSReceiverProfile.h"
#include "GPSRecordingController.h"
#include "GPSRecordingDevice.h"
#include "GPSRecordingTransport.h"
#include "GPSReplayDriver.h"
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

    void frame(qint64 receiptMs)
    {
        emit correctionReceivedAt(QByteArrayLiteral("valid correction"), 1005, false, receiptMs);
    }

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
        recordedStream->configurationFinished(int(configured == 0 ? GPSDriver::ConfigurationStatus::Ready
                                                                 : GPSDriver::ConfigurationStatus::Failed));
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
        QCOMPARE(exported.profile->configured, true);
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
        QCOMPARE(trace.profile->initialBaud, 9600);
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
        QCOMPARE(exported.profile->receiver.dynamicModel, config.dynamicModel);
        QCOMPARE(exported.profile->receiver.constellationMask, config.constellationMask);
        QCOMPARE(exported.profile->receiver.outputRateHz, config.outputRateHz);
        QCOMPARE(double(exported.profile->receiver.headingOffsetDeg), double(config.headingOffsetDeg));
        QCOMPARE(exported.profile->receiver.base.fixedBaseLatitude, config.base.fixedBaseLatitude);
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

    void recordingCodecRejectsMalformed_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("fractional-version") << QByteArray(R"({"version":1.5,"events":[]})");
        QTest::newRow("future-version") << QByteArray(R"({"version":3,"events":[]})");
        QTest::newRow("string-version") << QByteArray(R"({"version":"1","events":[]})");
        QTest::newRow("missing-filetype") << QByteArray(R"({"version":2,"events":[]})");
        QTest::newRow("wrong-limit-type") << QByteArray(R"({"version":1,"limit_reached":0,"events":[]})");
        QTest::newRow("event-array") << QByteArray(R"({"version":1,"events":[[]]})");
        QTest::newRow("string-time") << QByteArray(R"({"version":1,"events":[{"at_us":"2","kind":"open"}]})");
        QTest::newRow("fractional-stream")
            << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"open","stream":1.5}]})");
        QTest::newRow("string-error") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"read_error","value":"-1"}]})");
        QTest::newRow("invalid-hex") << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"rx","hex":"gg"}]})");
        QTest::newRow("nonascii-hex")
            << QStringLiteral("{\"version\":1,\"events\":[{\"at_us\":1,\"kind\":\"rx\",\"hex\":\"\u0131\"}]}").toUtf8();
        QTest::newRow("wrong-hex-type") << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"rx","hex":1234}]})");
        QTest::newRow("future-start") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"started_us":2,"kind":"open"}]})");
        QTest::newRow("wrong-resumed") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"open","resumed":"yes"}]})");
        QTest::newRow("metadata-type") << QByteArray(
            R"({"version":1,"events":[{"at_us":1,"kind":"session","profile":[]}]})");
        QTest::newRow("metadata-incomplete")
            << QByteArray(R"({"version":1,"events":[{"at_us":1,"kind":"session","profile":{}}]})");
    }

    void recordingCodecRejectsMalformed()
    {
        QFETCH(QByteArray, json);
        GPSRecordingDocument unchanged;
        unchanged.limitReached = true;
        QString error;
        QVERIFY(!GPSRecordingDocument::decode(json, unchanged, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(unchanged.limitReached);
    }

    void recordingCodecBoundsAndStreamValidation()
    {
        GPSRecordingDocument document;
        QString error;
        QVERIFY(!GPSRecordingDocument::decode(QByteArray(GPSRecordingDocument::MAX_BYTES + 1, ' '), document, error));
        document.events.append({.atUs = 1, .kind = GPSRecordingEvent::Kind::Rx,
                                .bytes = QByteArray(GPSRecordingDocument::MAX_BYTES, 'x')});
        QVERIFY(document.encode(&error).isEmpty());
        const QByteArray badOtherStream = R"({"version":1,"events":[
            {"at_us":1,"stream":7,"kind":"open"},
            {"at_us":2,"stream":8,"kind":"rx","hex":false}]})";
        GPSReplayTrace trace;
        QVERIFY(!GPSReplayTrace::fromJson(badOtherStream, trace, error, 7));
        QVERIFY(!error.isEmpty());
    }

    void recordingCodecMetadataAndTiming()
    {
        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpPeer;
        profile.endpoint.host = QStringLiteral("private.example");
        profile.endpoint.device = QStringLiteral("private-device");
        profile.receiverName = QStringLiteral("private-name");
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
        profile.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
        profile.receiver.base.useFixedBase = true;
        profile.receiver.base.fixedBaseLatitude = 47.4;
        const auto metadata = GPSRecordingMetadata::fromProfile(profile);
        GPSRecordingDocument document;
        document.limitReached = true;
        GPSRecordingEvent session{.atUs = 1, .kind = GPSRecordingEvent::Kind::Session};
        session.stream = 7;
        session.metadata = metadata;
        document.events.append(session);
        GPSRecordingEvent open{.atUs = 25, .kind = GPSRecordingEvent::Kind::Open};
        open.startedAtUs = 2;
        open.stream = 7;
        open.resumed = true;
        document.events.append(open);
        QString error;
        const auto bytes = document.encode(&error);
        QVERIFY2(!bytes.isEmpty(), qPrintable(error));
        QVERIFY(!bytes.contains("private"));
        QVERIFY(bytes.contains("udp_peer"));
        GPSRecordingDocument decoded;
        QVERIFY2(GPSRecordingDocument::decode(bytes, decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[0].metadata, metadata);
        QCOMPARE(decoded.events[0].metadata.fixedBaud, 115200u);
        QCOMPARE(decoded.events[1].startedAtUs, quint64(2));
        QVERIFY(decoded.events[1].resumed);
        QCOMPARE(decoded.encode(), bytes);
        GPSReplayTrace trace;
        QVERIFY(GPSReplayTrace::fromJson(bytes, trace, error, 7));
        QVERIFY(trace.limitReached);
        QCOMPARE(trace.recordedEvents.size(), 2);
        QCOMPARE(trace.events.first().startedAtUs, quint64(2));
        QVERIFY(!GPSReplayTrace::fromJson(bytes, trace, error, 8));
        auto malformed = QJsonDocument::fromJson(bytes).object();
        auto events = malformed["events"].toArray();
        auto event = events[0].toObject();
        auto config = event["profile"].toObject();
        config.insert("host", "credential-bearing-endpoint");
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        config.remove("host");
        config.insert("driver", 0);
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY(!GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error));
        // Version 1 transport/driver/role/protocol ordinals have a frozen compatibility mapping.
        malformed.insert("version", 1);
        malformed.remove("fileType");
        config.insert("transport", 2);
        config.insert("driver", 0);
        config.insert("role", 1);
        config.insert("protocol", 0);
        config.remove("fixed_baud");
        event.insert("profile", config);
        events[0] = event;
        malformed.insert("events", events);
        QVERIFY2(GPSRecordingDocument::decode(QJsonDocument(malformed).toJson(), decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[0].metadata.transport, GPSRecordingMetadata::Transport::Tcp);
        QCOMPARE(decoded.events[0].metadata.fixedBaud, 115200u);
        QCOMPARE(decoded.events[0].metadata.driverType, int(GPSType::u_blox));
        QCOMPARE(decoded.events[0].metadata.receiver.role, GPSReceiverConfig::Role::Position);
    }

    void replaySerialDeadlineFollowsCapturedBaud()
    {
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSReplayTrace trace{{{1, GPSReplayEvent::Kind::Open}, {2, GPSReplayEvent::Kind::Baud, {}, 9600}}};
        GPSRecordingMetadata metadata;
        metadata.transport = GPSRecordingMetadata::Transport::Serial;
        metadata.initialBaud = 115200;
        trace.profile = metadata;
        GPSReplayTransport transport(clock, stop, trace);
        QCOMPARE(transport.correctionWriteTimeout(1029), GPSTransport::serialCorrectionWriteTimeout(1029, 115200));
        QVERIFY(transport.open());
        QVERIFY(transport.setBaudrate(9600));
        QCOMPARE(transport.correctionWriteTimeout(1029), GPSTransport::serialCorrectionWriteTimeout(1029, 9600));
        QVERIFY(transport.correctionWriteTimeout(1029) > std::chrono::milliseconds(1000));
        QVERIFY(transport.complete());
    }

    void recordingPartialWriteEvidence()
    {
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSRecordingEvent write{.atUs = 501, .kind = GPSRecordingEvent::Kind::BoundedWrite, .bytes = "abcdef"};
        write.startedAtUs = 1;
        write.writeResult = GPSTransport::WriteResult{GPSTransport::WriteStatus::TimedOut, 5, 2, 3};
        GPSReplayTrace fixture{{{1, GPSReplayEvent::Kind::Open}, write}};
        auto buffer = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        auto stream = std::make_shared<GPSRecordingStream>(buffer, GPSRecordingMetadata{});
        QVERIFY(buffer->start());
        {
            GPSRecordingTransport tap(std::make_unique<GPSReplayTransport>(clock, stop, fixture), stop, stream);
            QVERIFY(tap.open());
            const auto result = tap.writeBounded(reinterpret_cast<const uint8_t*>("abcdef"), 6, QDeadlineTimer(100));
            QCOMPARE(result.status, GPSTransport::WriteStatus::TimedOut);
            QCOMPARE(result.acceptedBytes, 5);
            QCOMPARE(result.writtenBytes, 2);
            QCOMPARE(result.uncertainBytes, 3);
            QVERIFY(!tap.fatalError());
        }
        buffer->stop();
        GPSRecordingDocument decoded;
        QString error;
        const auto json = buffer->exportJson();
        QVERIFY2(GPSRecordingDocument::decode(json, decoded, error), qPrintable(error));
        QCOMPARE(decoded.events[2].startedAtUs, quint64(1));
        QCOMPARE(decoded.events[2].atUs, quint64(501));
        QVERIFY(decoded.events[2].writeResult.has_value());
        GPSReplayTrace roundTrip;
        QVERIFY(GPSReplayTrace::fromJson(json, roundTrip, error));
        GPSReplayClock secondClock;
        GPSReplayTransport replay(secondClock, stop, roundTrip);
        QVERIFY(replay.open());
        const auto result = replay.writeBounded(reinterpret_cast<const uint8_t*>("abcdef"), 6, QDeadlineTimer(100));
        QCOMPARE(result.acceptedBytes, 5);
        QCOMPARE(result.writtenBytes, 2);
        QCOMPARE(result.uncertainBytes, 3);
        QCOMPARE(result.status, GPSTransport::WriteStatus::TimedOut);
        QCOMPARE(secondClock.nowUs(), quint64(501));
        QVERIFY(replay.complete());
        // Impossible delivery counts must never enter the replay transport.
        decoded.events[2].writeResult->writtenBytes = 6;
        QVERIFY(decoded.encode(&error).isEmpty());
        QVERIFY(!error.isEmpty());
        GPSReplayClock shortClock;
        GPSReplayTransport shortDeadline(shortClock, stop, roundTrip);
        QVERIFY(shortDeadline.open());
        QCOMPARE(shortDeadline.writeBounded(reinterpret_cast<const uint8_t*>("abcdef"), 6, QDeadlineTimer(0)).status,
                 GPSTransport::WriteStatus::Error);
        QVERIFY(!shortDeadline.failure().isEmpty());
        QCOMPARE(shortClock.nowUs(), quint64(1));
    }

    void driverFromCapturedProfile_data()
    {
        QTest::addColumn<bool>("septentrio");
        QTest::addColumn<bool>("base");
        QTest::newRow("septentrio-position") << true << false;
        QTest::newRow("septentrio-base") << true << true;
        QTest::newRow("femto-position") << false << false;
        QTest::newRow("femto-base") << false << true;
    }

    void driverFromCapturedProfile()
    {
        QFETCH(bool, septentrio);
        QFETCH(bool, base);
        gps_test_time = 1;
        GPSReplayClock clock(&gps_test_time);
        std::atomic_bool stop = false;

        class Receiver : public GPSTransport
        {
        public:
            Receiver(std::atomic_bool& stop, bool septentrio, GPSReplayClock& clock)
                : GPSTransport(stop)
                , _septentrio(septentrio)
                , _clock(clock)
            {}

            bool open() override { return true; }

            bool fatalError() const override { return false; }

            bool setBaudrate(unsigned baud) override { return baud == 115200; }

            unsigned fixedBaudrate() const override { return 115200; }

            int read(uint8_t* data, int size, int timeout) override
            {
                const int count = qMin(size, int(_reply.size()));
                memcpy(data, _reply.constData(), count);
                _reply.remove(0, count);
                _clock.advanceBy(count ? 1 : quint64(qMax(timeout, 0)) * 1000 + 1);
                return count;
            }

            int write(const uint8_t* data, int size) override
            {
                const QByteArray command(reinterpret_cast<const char*>(data), size);
                if (_septentrio) {
                    _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
                } else {
                    _reply = '<' + command.split(' ').first().trimmed() + " OK";
                    _reply.append(char(0));
                }
                return size;
            }

        private:
            bool _septentrio;
            GPSReplayClock& _clock;
            QByteArray _reply;
        };

        GPSReceiverProfile profile;
        profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
        profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
        profile.driverType = septentrio ? GPSType::septentrio : GPSType::femto;
        profile.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
        profile.receiver.role = base ? GPSReceiverConfig::Role::RTKBase : GPSReceiverConfig::Role::Position;
        profile.receiver.base.useFixedBase = true;
        profile.receiver.base.fixedBaseLatitude = 10;
        profile.receiver.base.fixedBaseLongitude = 20;
        auto buffer = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        auto stream = std::make_shared<GPSRecordingStream>(buffer, GPSRecordingMetadata::fromProfile(profile));
        QVERIFY(buffer->start());
        {
            GPSRecordingTransport transport(std::make_unique<Receiver>(stop, septentrio, clock), stop, stream);
            QVERIFY(transport.open());
            stream->configurationStarted();
            GPSDriver original(profile.driverType, transport, profile.receiver, {});
            QVERIFY(original.configure());
            stream->configurationFinished(int(original.configurationResult().status));
        }
        buffer->stop();
        GPSReplayTrace trace;
        QString error;
        QVERIFY2(GPSReplayTrace::fromJson(buffer->exportJson(), trace, error), qPrintable(error));
        QCOMPARE(trace.profile->receiver, profile.receiver);
        gps_test_time = 1;
        GPSReplayTransport transport(clock, stop, trace);
        QVERIFY(transport.open());
        auto driver = createGPSReplayDriver(transport, {}, error);
        QVERIFY2(driver != nullptr, qPrintable(error));
        QCOMPARE(transport.fixedBaudrate(), 115200u);
        QVERIFY2(driver->configure(), qPrintable(transport.failure()));
        QVERIFY2(transport.complete(), qPrintable(transport.failure()));
        auto incomplete = trace;
        incomplete.recordedEvents[1].resumed = true;
        GPSReplayTransport resumed(clock, stop, incomplete);
        QVERIFY(!createGPSReplayDriver(resumed, {}, error));
        incomplete = trace;
        incomplete.profile->configured = false;
        GPSReplayTransport passive(clock, stop, incomplete);
        QVERIFY(!createGPSReplayDriver(passive, {}, error));
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
        stream->frame(clock.nowMs());
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
        stream->frame(clock.nowMs());
        QCOMPARE(delivered, 2);
        session.stop();
    }
};

QTEST_GUILESS_MAIN(GPSReplayTest)
#include "GPSReplayTest.moc"
