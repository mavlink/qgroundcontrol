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
#include "GPSReplayDevice.h"
#include "GPSReplayDriver.h"
#include "GPSReplayScheduler.h"
#include "GPSReplayTransport.h"
#include "NMEADecoderSession.h"
#include "NMEAPositionSource.h"
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
        const auto result = transport.read(static_cast<uint8_t*>(data), size, timeout);
        return result.status == GPSTransport::ReadStatus::Data       ? result.bytesRead
               : result.status == GPSTransport::ReadStatus::TimedOut ? 0
                                                                     : -EIO;
    }
    if (type == GPSCallbackType::writeDeviceData) {
        const auto result = transport.write(static_cast<const uint8_t*>(data), size);
        return result.status == GPSTransport::WriteStatus::Completed ? result.writtenBytes : -EIO;
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
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        sensor_gps_s position{};
        GPSDriverUBX driver(GPSHelper::Interface::UART, nativeCallback, &transport, &position, nullptr, {});
        GPSHelper::GPSConfig config{};
        config.output_mode = GPSHelper::OutputMode::GPS;
        unsigned baudrate = 115200;
        recordedStream->configurationStarted();
        const auto configured = driver.configure(baudrate, config);
        recordedStream->configurationFinished(
            int(configured == 0 ? GPSDriver::ConfigurationStatus::Ready : GPSDriver::ConfigurationStatus::Failed));
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
        QCOMPARE(roundTrip.open().status, GPSTransport::OpenStatus::Opened);
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
            QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
            ReplayInput input;
            NMEAStreamSplitter splitter(&input);
            QtNmeaDecoder decoder;
            int decoded = 0;
            QGeoPositionInfo position;
            quint64 firstReceipt = 0;
            while (transport.remainingEvents() > 0) {
                uint8_t bytes[32];
                const auto read = transport.read(bytes, sizeof(bytes), 1000);
                const int count = read.status == GPSTransport::ReadStatus::Data       ? read.bytesRead
                                  : read.status == GPSTransport::ReadStatus::TimedOut ? 0
                                                                                      : -EIO;
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
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("com"), 3).writtenBytes, 3);
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("mand"), 4).writtenBytes, 4);
        uint8_t bytes[8];
        QCOMPARE(transport.read(bytes, sizeof(bytes), 500).status, GPSTransport::ReadStatus::TimedOut);
        QCOMPARE(clock.nowUs(), quint64(500001));
        QCOMPARE(transport.read(bytes, sizeof(bytes), 100).status, GPSTransport::ReadStatus::Error);
        QVERIFY(transport.fatalError());
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("command"), 7).writtenBytes, 2);
        QVERIFY(!transport.fatalError());
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        QCOMPARE(transport.read(bytes, sizeof(bytes), 100).status, GPSTransport::ReadStatus::Cancelled);
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
        QCOMPARE(replay.open().status, GPSTransport::OpenStatus::Opened);
        QByteArray received;
        uint8_t bytes[32];
        while (!replay.complete()) {
            const auto result = replay.read(bytes, sizeof(bytes), 100);
            const int count = result.bytesRead;
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
        QCOMPARE(replayClock.nowUs() - replay.lastReadTimestampUs(), clock.nowUs() - quint64(43));
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
            QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Error);
            QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
            QVERIFY(!transport.setBaudrate(9600));
            QVERIFY(transport.setBaudrate(115200));
            QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("command"), 7).writtenBytes, 2);
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
        QCOMPARE(replay.open().status, GPSTransport::OpenStatus::Error);
        QCOMPARE(replay.open().status, GPSTransport::OpenStatus::Opened);
        QVERIFY(!replay.setBaudrate(9600));
        QVERIFY(replay.setBaudrate(115200));
        QCOMPARE(replay.write(reinterpret_cast<const uint8_t*>("command"), 7).writtenBytes, 2);
        uint8_t terminalByte;
        QCOMPARE(replay.read(&terminalByte, 1, 1000).status, GPSReadStatus::Closed);
        QCOMPARE(replay.termination()->reason, GPSReplayTermination::Reason::Closed);
        QCOMPARE(replay.terminationCount(), quint64(2));  // The first open failed before the successful attempt.
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
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
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
            QCOMPARE(tap.open().status, GPSTransport::OpenStatus::Opened);
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
        QCOMPARE(replay.open().status, GPSTransport::OpenStatus::Opened);
        const auto result = replay.writeBounded(reinterpret_cast<const uint8_t*>("abcdef"), 6, QDeadlineTimer(100));
        QCOMPARE(result.acceptedBytes, 5);
        QCOMPARE(result.writtenBytes, 2);
        QCOMPARE(result.uncertainBytes, 3);
        QCOMPARE(result.status, GPSTransport::WriteStatus::TimedOut);
        QCOMPARE(secondClock.nowUs(), quint64(501));
        uint8_t terminalByte;
        QCOMPARE(replay.read(&terminalByte, 1, 1000).status, GPSReadStatus::Closed);
        QVERIFY(replay.complete());
        // Impossible delivery counts must never enter the replay transport.
        decoded.events[2].writeResult->writtenBytes = 6;
        QVERIFY(decoded.encode(&error).isEmpty());
        QVERIFY(!error.isEmpty());
        GPSReplayClock shortClock;
        GPSReplayTransport shortDeadline(shortClock, stop, roundTrip);
        QCOMPARE(shortDeadline.open().status, GPSTransport::OpenStatus::Opened);
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
                : GPSTransport(stop), _septentrio(septentrio), _clock(clock)
            {}

            OpenResult open() override { return {.status = OpenStatus::Opened}; }

            bool fatalError() const override { return false; }

            bool setBaudrate(unsigned baud) override { return baud == 115200; }

            unsigned fixedBaudrate() const override { return 115200; }

            ReadResult read(uint8_t* data, int size, int timeout) override
            {
                const int count = qMin(size, int(_reply.size()));
                memcpy(data, _reply.constData(), count);
                _reply.remove(0, count);
                _clock.advanceBy(count ? 1 : quint64(qMax(timeout, 0)) * 1000 + 1);
                return {.status = count ? ReadStatus::Data : ReadStatus::TimedOut, .bytesRead = count};
            }

            WriteResult write(const uint8_t* data, int size) override
            {
                const QByteArray command(reinterpret_cast<const char*>(data), size);
                if (_septentrio) {
                    _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
                } else {
                    _reply = '<' + command.split(' ').first().trimmed() + " OK";
                    _reply.append(char(0));
                }
                return {.status = WriteStatus::Completed, .acceptedBytes = size, .writtenBytes = size};
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
            QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
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
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        auto driver = createGPSReplayDriver(transport, {}, error);
        QVERIFY2(driver != nullptr, qPrintable(error));
        QCOMPARE(transport.fixedBaudrate(), 115200u);
        QVERIFY2(driver->configure(), qPrintable(transport.failure()));
        uint8_t terminalByte;
        QCOMPARE(transport.read(&terminalByte, 1, 1000).status, GPSReadStatus::Closed);
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

    void schedulerRetiresCancelledAndDestroyedCallbacks()
    {
        GPSReplayScheduler scheduler;
        auto owner = std::make_unique<QObject>();
        int calls = 0;
        const auto cancelled = scheduler.schedule(owner.get(), std::chrono::milliseconds(1), [&]() { ++calls; });
        scheduler.cancel(cancelled);
        scheduler.schedule(owner.get(), std::chrono::milliseconds(2), [&]() { ++calls; });
        owner.reset();
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
        QCOMPARE(calls, 0);
        QObject alive;
        scheduler.schedule(&alive, std::chrono::microseconds::zero(), [&]() { ++calls; });
        QCOMPARE(calls, 0);
        QVERIFY(scheduler.advanceBy(std::chrono::microseconds::zero()));
        QCOMPARE(calls, 1);
        QCOMPARE(scheduler.pendingCount(), 0);
    }

    void nmeaProductionSessionReplay()
    {
        GPSReplayScheduler scheduler;
        GPSReplayDevice input(&scheduler);
        NMEADecoderSession session(nullptr, &scheduler);
        int positions = 0;
        QVector<GPSObservation> observations;
        connect(&input, &GPSReplayDevice::streamOpened, &session, [&]() {
            QVERIFY(session.start(&input));
            auto* position = qobject_cast<NMEAPositionSource*>(session.positionSource());
            QVERIFY(position);
            position->setUpdateInterval(100);
            connect(position, &QGeoPositionInfoSource::positionUpdated, &session, [&, position]() {
                ++positions;
                observations.append(position->lastObservation());
            });
            position->startUpdates();
        });
        connect(&input, &GPSReplayDevice::streamClosed, &session, &NMEADecoderSession::stop);
        auto sentence = [](const QByteArray& body) {
            uint8_t checksum = 0;
            for (const auto byte : body) {
                checksum ^= static_cast<uint8_t>(byte);
            }
            return '$' + body + '*' + QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper() + "\r\n";
        };
        const auto first = sentence("GPRMC,120000.00,A,4807.038,N,01131.000,E,0.0,0.0,010126,,,A") +
                           sentence("GPGGA,120000.00,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
        const auto second = sentence("GPRMC,120001.00,A,4807.039,N,01131.001,E,0.0,0.0,010126,,,A");
        using K = GPSRecordingEvent::Kind;
        GPSRecordingEvent delayed{.atUs = 1000, .kind = K::Rx, .bytes = first.left(12)};
        delayed.receivedAtUs = -500000;
        GPSRecordingEvent remainder{.atUs = 2000, .kind = K::Rx, .bytes = first.mid(12)};
        remainder.receivedAtUs = -490000;
        input.play({{.atUs = 1, .kind = K::Open},
                    delayed,
                    remainder,
                    {.atUs = 3000, .kind = K::Rx, .bytes = second},
                    {.atUs = 200000, .kind = K::Close},
                    {.atUs = 210000, .kind = K::Open},
                    {.atUs = 211000, .kind = K::Rx, .bytes = first},
                    {.atUs = 212000, .kind = K::Rx, .bytes = second},
                    {.atUs = 215000, .kind = K::Close}});
        const auto origin = input.timeOriginUs();
        QVERIFY(scheduler.advanceToUs(origin + 150000));
        QCOMPARE(positions, 1);
        QCOMPARE(observations.first().monotonicTimestampUs, origin - 500000);
        QVERIFY(session.health()->usable());
        QVERIFY(scheduler.advanceToUs(origin + 500000));
        // The second session retired before its scheduled publication; no previous fix leaks through.
        QCOMPARE(positions, 1);
        QVERIFY(!session.health()->usable());
    }

    void oneShotTimeoutUsesVirtualTime()
    {
        GPSReplayScheduler scheduler;
        ReplayInput input;
        NMEAPositionSource source(&input, nullptr, &scheduler);
        QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
        source.requestUpdate(100);
        source.requestUpdate(500);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(99)));
        QCOMPARE(errors.size(), 0);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(errors.size(), 1);
        QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
        source.requestUpdate(100);
        source.startUpdates();
        source.stopUpdates();
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
        QCOMPARE(errors.size(), 2);
    }

    void typedReadAndOpenReplay()
    {
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        using K = GPSRecordingEvent::Kind;
        GPSRecordingEvent failed{.atUs = 1000, .kind = K::OpenError};
        failed.openStatus = GPSOpenStatus::TimedOut;
        GPSRecordingEvent overflow{.atUs = 3000, .kind = K::ReadError};
        overflow.readStatus = GPSReadStatus::Overflow;
        GPSReplayTransport transport(clock, stop, GPSReplayTrace{{failed, {.atUs = 2000, .kind = K::Open}, overflow}});
        QCOMPARE(transport.open().status, GPSOpenStatus::TimedOut);
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        uint8_t buffer[8];
        QCOMPARE(transport.read(buffer, sizeof(buffer), 10).status, GPSReadStatus::Overflow);
        QVERIFY(transport.complete());
        GPSReplayScheduler scheduler;
        GPSReplayDevice device(&scheduler);
        QSignalSpy terminals(&device, &GPSReplayDevice::terminated);
        QSignalSpy opened(&device, &GPSReplayDevice::streamOpened);
        device.play({failed, {.atUs = 2000, .kind = K::Open}, overflow, {.atUs = 4000, .kind = K::Close}});
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(5)));
        QCOMPARE(opened.size(), 1);
        QCOMPARE(terminals.size(), 2);
        const auto first = qvariant_cast<GPSReplayTermination>(terminals.first().first());
        QCOMPARE(first.reason, GPSReplayTermination::Reason::OpenFailure);
        QCOMPARE(first.openStatus, std::optional{GPSOpenStatus::TimedOut});
        QCOMPARE(first.atUs, quint64(1000));
        QCOMPARE(qvariant_cast<GPSReplayTermination>(terminals.last().first()), *transport.termination());
    }

    void recoverableWrite_data()
    {
        QTest::addColumn<bool>("bounded");
        QTest::newRow("legacy-short-write") << false;
        QTest::newRow("bounded-nonfatal-error") << true;
    }

    void recoverableWrite()
    {
        QFETCH(bool, bounded);
        using K = GPSRecordingEvent::Kind;
        GPSRecordingEvent write{
            .atUs = 10, .kind = bounded ? K::BoundedWrite : K::WriteError, .bytes = "command", .value = 2};
        if (bounded) {
            write.writeResult = GPSWriteResult{.status = GPSWriteStatus::Error, .acceptedBytes = 2, .writtenBytes = 2};
        }
        const QVector<GPSRecordingEvent> events{{.atUs = 1, .kind = K::Open},
                                                write,
                                                {.atUs = 20, .kind = K::Rx, .bytes = "recovered"},
                                                {.atUs = 30, .kind = K::Close}};
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSReplayTransport transport(clock, stop, GPSReplayTrace{events});
        QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
        QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>("command"), 7).writtenBytes, 2);
        QVERIFY(!transport.fatalError());
        QVERIFY(!transport.termination());
        uint8_t bytes[32];
        const auto read = transport.read(bytes, sizeof(bytes), 100);
        QCOMPARE(read.status, GPSReadStatus::Data);
        const QByteArray received(reinterpret_cast<char*>(bytes), read.bytesRead);
        QCOMPARE(received, QByteArray("recovered"));
        QCOMPARE(transport.read(bytes, sizeof(bytes), 100).status, GPSReadStatus::Closed);
        QVERIFY(transport.complete());
        GPSReplayScheduler scheduler;
        GPSReplayDevice device(&scheduler);
        QByteArray passive;
        connect(&device, &QIODevice::readyRead, &device, [&]() { passive += device.readAll(); });
        QSignalSpy terminal(&device, &GPSReplayDevice::terminated);
        device.play(events);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(passive, received);
        QCOMPARE(terminal.size(), 1);
        QCOMPARE(qvariant_cast<GPSReplayTermination>(terminal.first().first()), *transport.termination());
        QCOMPARE(transport.termination()->reason, GPSReplayTermination::Reason::Closed);
        QCOMPARE(transport.termination()->atUs, quint64(30));
    }

    void recordedTermination_data()
    {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("wireVersion");
        QTest::newRow("normal-v1") << 0 << 1;
        QTest::newRow("normal-v2") << 0 << 2;
        QTest::newRow("normal-v3") << 0 << 3;
        QTest::newRow("cancel-on-close-v1") << 1 << 1;
        QTest::newRow("cancel-on-close-v2") << 1 << 2;
        QTest::newRow("cancel-on-close-v3") << 1 << 3;
        QTest::newRow("overflow-then-close") << 2 << 3;
        QTest::newRow("read-cancel-then-close") << 3 << 3;
        QTest::newRow("capture-exhaustion") << 4 << 3;
    }

    void recordedTermination()
    {
        QFETCH(int, mode);
        QFETCH(int, wireVersion);
        using K = GPSRecordingEvent::Kind;
        using R = GPSReplayTermination::Reason;
        GPSReplayClock clock;
        std::atomic_bool stop = false;
        GPSReplayTrace fixture{{{.atUs = 1, .kind = K::Open}}};
        if (mode == 2 || mode == 3) {
            GPSRecordingEvent event{.atUs = 100, .kind = mode == 2 ? K::ReadError : K::Cancel};
            event.readStatus = mode == 2 ? GPSReadStatus::Overflow : GPSReadStatus::Cancelled;
            fixture.events.append(event);
        }
        auto buffer = std::make_shared<GPSRecordingBuffer>([&clock]() { return clock.nowUs(); });
        auto stream = std::make_shared<GPSRecordingStream>(buffer, GPSRecordingMetadata{});
        QVERIFY(buffer->start());
        {
            GPSRecordingTransport tap(std::make_unique<GPSReplayTransport>(clock, stop, fixture), stop, stream);
            QCOMPARE(tap.open().status, GPSOpenStatus::Opened);
            if (mode == 2 || mode == 3) {
                uint8_t bytes[8];
                QCOMPARE(tap.read(bytes, sizeof(bytes), 1).status,
                         mode == 2 ? GPSReadStatus::Overflow : GPSReadStatus::Cancelled);
            }
            clock.advanceTo(200);
            if (mode == 1) {
                stop = true;
            }
            if (mode == 4) {
                buffer->stop();  // Capture ending is not evidence that the receiver disconnected.
            }
        }
        buffer->stop();
        auto document = QJsonDocument::fromJson(buffer->exportJson()).object();
        document.insert("version", wireVersion);
        if (wireVersion < 3) {
            QJsonArray legacyEvents;
            const auto events = document.value("events").toArray();
            for (const auto& value : events) {
                auto event = value.toObject();
                event.remove("open_status");
                if (wireVersion == 1) {
                    if (event.value("kind").toString() == QStringLiteral("session")) {
                        continue;
                    }
                    event.remove("stream");
                    event.remove("profile");
                }
                legacyEvents.append(event);
            }
            document.insert("events", legacyEvents);
        }
        GPSReplayTrace trace;
        QString error;
        QVERIFY2(GPSReplayTrace::fromJson(QJsonDocument(document).toJson(), trace, error), qPrintable(error));
        stop = false;
        GPSReplayClock replayClock;
        GPSReplayTransport native(replayClock, stop, trace);
        QCOMPARE(native.open().status, GPSOpenStatus::Opened);
        uint8_t bytes[8];
        do {
            native.read(bytes, sizeof(bytes), 100);
        } while (!native.complete());
        QVERIFY(native.termination());
        const auto first = *native.termination();
        const auto expectedReason = mode == 4                  ? R::CaptureExhausted
                                    : mode == 2                ? R::ReadFailure
                                    : (mode == 1 || mode == 3) ? R::Cancelled
                                                               : R::Closed;
        QCOMPARE(first.reason, expectedReason);
        QCOMPARE(first.atUs, mode == 4 ? quint64(1) : (mode == 2 || mode == 3) ? quint64(100) : quint64(200));
        QCOMPARE(first.readStatus, mode == 4                  ? GPSReadStatus::InvalidData
                                   : mode == 2                ? GPSReadStatus::Overflow
                                   : (mode == 1 || mode == 3) ? GPSReadStatus::Cancelled
                                                              : GPSReadStatus::Closed);
        const auto stoppedAt = replayClock.nowUs();
        for (int i = 0; i < 3; ++i) {
            QCOMPARE(native.read(bytes, sizeof(bytes), 100).status, first.readStatus);
        }
        QCOMPARE(replayClock.nowUs(), stoppedAt);
        QCOMPARE(native.terminationCount(), quint64(1));
        QVERIFY(native.fatalError());

        GPSReplayScheduler scheduler;
        GPSReplayDevice device(&scheduler);
        QSignalSpy terminal(&device, &GPSReplayDevice::terminated);
        QSignalSpy closed(&device, &GPSReplayDevice::streamClosed);
        QSignalSpy errors(&device, &GPSReplayDevice::sessionError);
        device.play(trace.recordedEvents);
        QVERIFY(scheduler.advanceToUs(device.timeOriginUs() + 1000));
        QCOMPARE(terminal.size(), 1);
        QCOMPARE(qvariant_cast<GPSReplayTermination>(terminal.first().first()), first);
        QCOMPARE(closed.size(), mode == 4 ? 0 : 1);
        QCOMPARE(errors.size(), mode == 1 || mode == 2 || mode == 3 ? 1 : 0);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
        device.stop();
        QCOMPARE(terminal.size(), 1);
        QCOMPARE(closed.size(), mode == 4 ? 0 : 1);
    }

    void correctionAndRecoveryUseVirtualTime()
    {
        GPSReplayScheduler scheduler;
        ReplayCaster* stream = nullptr;
        int attempts = 0;
        NTRIPSession session(
            [&](const NTRIPTransportConfig&, QObject* owner) {
                ++attempts;
                stream = new ReplayCaster(owner);
                return stream;
            },
            nullptr, {}, &scheduler);
        QSignalSpy corrections(&session, &NTRIPSession::correctionReceived);
        QSignalSpy retired(&session, &NTRIPSession::streamEnded);
        NTRIPTransportConfig config;
        config.host = QStringLiteral("synthetic.invalid");
        config.mountpoint = QStringLiteral("BASE");
        session.start(config);
        const auto firstAttempt = session.activeAttemptId();
        stream->frame(scheduler.nowMs());
        QCOMPARE(corrections.size(), 1);
        stream->fail();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(session.state(), NTRIPSession::State::Reconnecting);
        QCOMPARE(retired.size(), 1);
        const auto retry = session.nextRetryDelay();
        QVERIFY(scheduler.advanceBy(retry - std::chrono::milliseconds(1)));
        QCOMPARE(attempts, 1);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(attempts, 2);
        QVERIFY(session.activeAttemptId() != firstAttempt);
        stream->frame(scheduler.nowMs());
        QCOMPARE(corrections.size(), 2);
        QCOMPARE(corrections.last().at(3).toLongLong(), scheduler.nowMs());
        stream->fail();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        session.stop();
        QVERIFY(scheduler.advanceBy(std::chrono::minutes(1)));
        QCOMPARE(attempts, 2);
        QCOMPARE(session.state(), NTRIPSession::State::Disconnected);
    }
};

QTEST_GUILESS_MAIN(GPSReplayTest)
#include "GPSReplayTest.moc"
