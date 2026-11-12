#include "GPSRecordingTransportTest.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <variant>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QTimeZone>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Fixtures/RAIIFixtures.h"
#include "GPSCancellation.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "GPSRecordingTransport.h"
#include "GPSTransport.h"
#include "Protocols/Support/GPSEventSummary.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Transport/Support/GPSRecordingDialogue.h"
#include "Transport/Support/GPSRecordingReader.h"
#include "Transport/Support/ReplayGPSTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
#ifdef Q_OS_LINUX
/// Points this process's open descriptors of @a file at /dev/full, so that what is written to the open file from now
/// on fails as on a full disk. Only that file is affected. @return whether a descriptor was found.
bool failWritesTo(const QString& file)
{
    constexpr int DESCRIPTORS = 4096;
    const QString path = QFileInfo(file).canonicalFilePath();
    const int full = ::open("/dev/full", O_WRONLY | O_CLOEXEC);
    if (path.isEmpty() || full < 0) {
        return false;
    }
    bool found = false;
    for (int descriptor = 0; descriptor < DESCRIPTORS; ++descriptor) {
        if (descriptor != full && QFile::symLinkTarget(QStringLiteral("/proc/self/fd/%1").arg(descriptor)) == path) {
            found = ::dup2(full, descriptor) == descriptor || found;
        }
    }
    (void) ::close(full);
    return found;
}
#endif

/// Serves its input in fixed chunks; once drained, every read times out at once rather than waiting out its timeout.
/// Writes are kept.
class MemoryGPSTransport final : public GPSTransport
{
public:
    static constexpr unsigned REFUSED_BAUD_RATE = 1200;

    explicit MemoryGPSTransport(QByteArray input = {}, int chunkBytes = 64, GPSCancelToken cancelToken = {})
        : GPSTransport(std::move(cancelToken))
        , _input(std::move(input))
        , _chunkBytes(chunkBytes)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    unsigned fixedBaudRate() const override { return _fixedBaudRate; }

    /// The link is a network bridge, which runs at this rate alone.
    void setFixedBaudRate(unsigned baudRate) { _fixedBaudRate = baudRate; }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds) override
    {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (drained()) {
            return {GPSReadStatus::TimedOut};
        }
        const int count = static_cast<int>(
            std::min<qsizetype>({static_cast<qsizetype>(buffer.size()), _chunkBytes, _input.size() - _offset}));
        std::memcpy(buffer.data(), _input.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        return {GPSReadStatus::Data, count};
    }

    bool setBaudRate(unsigned baudRate) override
    {
        if (baudRate == REFUSED_BAUD_RATE) {
            return false;
        }
        _baudRate = baudRate;
        return true;
    }

    /// Writes from now on take the first @a bytes of what they are given and then time out.
    void acceptBytes(int bytes) { _acceptedBytes = bytes; }

    /// Writes from now on are refused whole, with @a answer.
    void refuseWrites(GPSWriteStatus answer) { _refusal = answer; }

    bool drained() const { return _offset >= _input.size(); }

    const QByteArray& written() const { return _written; }

    unsigned baudRate() const { return _baudRate; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer) override
    {
        if (_refusal) {
            return {*_refusal};
        }
        const auto length = static_cast<int>(bytes.size());
        const int accepted = (std::min) (_acceptedBytes.value_or(length), length);
        _written.append(bytes.first(accepted));
        return {accepted == length ? GPSWriteStatus::Completed : GPSWriteStatus::TimedOut, accepted, accepted};
    }

private:
    QByteArray _input;
    QByteArray _written;
    qsizetype _offset = 0;
    int _chunkBytes;
    std::optional<int> _acceptedBytes;
    std::optional<GPSWriteStatus> _refusal;
    unsigned _baudRate = 0;
    unsigned _fixedBaudRate = 0;
};

constexpr const char* RECORDING_LOG = "GPS.Transport.GPSRecordingTransport";

QByteArray bytePattern(int size, int seed = 0)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>((index * 31 + seed) & 0xff);
    }
    return bytes;
}

/// Reads until the transport has nothing left and returns what it delivered.
QByteArray readAvailable(GPSTransport& transport)
{
    QByteArray received;
    uint8_t buffer[512];
    for (;;) {
        const auto result = transport.read(buffer, 0ms);
        if (result.status != GPSReadStatus::Data) {
            return received;
        }
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
    }
}

GPSWriteResult writeBytes(GPSTransport& transport, const QByteArray& bytes)
{
    return transport.write(bytes, QDeadlineTimer(TestTimeout::shortDuration()));
}

/// The records of the recording at @a path as "<kind> <value>", in file order, followed by what the reader rejects
/// the recording for: a missing header, an unterminated last line, or a line that is not a record with a time at or
/// after the one above it.
QByteArrayList records(const QString& path)
{
    QString error;
    QByteArrayList lines;
    for (const GPSRecordingRecord& record : GPSRecordingReader::readFile(path, &error)) {
        switch (record.kind) {
            case GPSRecordingRecord::Kind::Received:
                lines.append("RX " + record.bytes.toHex());
                break;
            case GPSRecordingRecord::Kind::Sent:
                lines.append("TX " + record.bytes.toHex());
                break;
            case GPSRecordingRecord::Kind::BaudRate:
                lines.append("BAUD " + QByteArray::number(record.baudRate));
                break;
            case GPSRecordingRecord::Kind::RefusedWrite:
                lines.append("TX-REFUSED " + QByteArray(GPSRecordingTransport::refusalName(record.answer)) + ' ' +
                             record.bytes.toHex());
                break;
            case GPSRecordingRecord::Kind::RefusedBaudRate:
                lines.append("BAUD-REFUSED " + QByteArray::number(record.baudRate));
                break;
        }
    }
    if (!error.isEmpty()) {
        lines.append(error.toUtf8());
    }
    return lines;
}

/// The first line of every recording, spelled out so that the format cannot change with the recorder's constant.
constexpr const char* HEADER_LINE = "QGC GPS receiver recording 1\n";

/// Reads @a transport in @a bufferBytes pieces until it has nothing left, calling @a afterRead with the number of
/// each read, and returns one line per event the passive decoder publishes for what was read. The decoder's clock
/// stands still until the input ends, so the same bytes give the same lines however they were divided.
template <typename AfterRead>
QStringList decodedEvents(GPSTransport& transport, size_t bufferBytes, AfterRead afterRead)
{
    GPSTestClock clock(GPSTestClock::START_US);
    QStringList lines;
    GPSRuntimeObserver observer;
    // Without the batches, whose number follows the reads.
    observer.decoded = [&lines](const GPSEventBatch& batch) {
        for (const auto& event : batch.events) {
            lines.append(std::visit([](const auto& report) { return summary(report); }, event));
        }
    };
    GPSProtocolRuntime runtime(*gpsReceiverFamily(GPSType::Passive), makeDecoderOnlyIO(clock), std::move(observer));
    runtime.stream().setEnabled(GPSFrameKind::RTCM3, true);
    (void) runtime.armNavigationDecode({.corrections = true});
    QByteArray buffer(static_cast<qsizetype>(bufferBytes), Qt::Uninitialized);
    const std::span<uint8_t> bytes(reinterpret_cast<uint8_t*>(buffer.data()), bufferBytes);
    for (int read = 0;; ++read) {
        const GPSReadResult result = transport.read(bytes, 0ms);
        if (result.status != GPSReadStatus::Data) {
            break;
        }
        (void) runtime.consume(bytes.first(static_cast<size_t>(result.bytesRead)));
        afterRead(read);
    }
    // The last epoch is published once nothing follows it.
    clock.advanceBy(10s);
    (void) runtime.consume({});
    return lines;
}
}  // namespace

void GPSRecordingTransportTest::_recordsLinkTrafficInOrder()
{
    TestFixtures::TempDirFixture directory;
    // Named in UTC whatever the zone of the start time, in a folder that does not exist yet.
    const QString file = GPSRecordingTransport::sessionFile(
        QDir(directory.path()).filePath(QStringLiteral("GPSReceiver")), GPSType::UBlox,
        QDateTime(QDate(2026, 1, 2), QTime(5, 4, 5, 6), QTimeZone::fromSecondsAheadOfUtc(2 * 60 * 60)));
    QCOMPARE(file, QDir(directory.path()).filePath(QStringLiteral("GPSReceiver/20260102T030405.006Z-ublox.log")));

    const QByteArray input = bytePattern(100, 3);
    const QByteArray command = QByteArray::fromHex("b5620a0400000e34");
    auto inner = std::make_unique<MemoryGPSTransport>(input, 40);
    MemoryGPSTransport* const link = inner.get();
    {
        GPSRecordingTransport recorder(std::move(inner), file);
        // A link that never opens leaves no file behind.
        QVERIFY(!QFileInfo::exists(file));
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        QVERIFY(recorder.recording());

        QVERIFY(recorder.setBaudRate(38400));
        QVERIFY(!recorder.setBaudRate(MemoryGPSTransport::REFUSED_BAUD_RATE));
        QCOMPARE(link->baudRate(), 38400U);
        uint8_t buffer[512];
        QCOMPARE(recorder.read(buffer, 0ms).bytesRead, 40);
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(buffer), 40), input.first(40));
        QCOMPARE(writeBytes(recorder, command).status, GPSWriteStatus::Completed);
        QCOMPARE(recorder.read(buffer, 0ms).bytesRead, 40);
        // The link takes only the start of the next write.
        link->acceptBytes(3);
        const GPSWriteResult partial = writeBytes(recorder, command);
        QCOMPARE(partial.status, GPSWriteStatus::TimedOut);
        QCOMPARE(partial.acceptedBytes, 3);
        QCOMPARE(recorder.read(buffer, 0ms).bytesRead, 20);
        QCOMPARE(recorder.read(buffer, 0ms).status, GPSReadStatus::TimedOut);
        QCOMPARE(link->written(), command + command.first(3));
    }
    // The rest of a write that the link accepted the start of is not recorded.
    QCOMPARE(records(file), (QByteArrayList{
                                "BAUD 38400",
                                "BAUD-REFUSED 1200",
                                "RX " + input.first(40).toHex(),
                                "TX " + command.toHex(),
                                "RX " + input.sliced(40, 40).toHex(),
                                "TX " + command.first(3).toHex(),
                                "RX " + input.sliced(80).toHex(),
                            }));
}

void GPSRecordingTransportTest::_recordsWhatLinkRefused_data()
{
    QTest::addColumn<GPSWriteStatus>("answer");
    QTest::addColumn<QByteArray>("record");
    // UDP carries no writes.
    QTest::newRow("unsupported") << GPSWriteStatus::Unsupported << QByteArray("TX-REFUSED unsupported b5620a04");
    QTest::newRow("timed-out") << GPSWriteStatus::TimedOut << QByteArray("TX-REFUSED timed-out b5620a04");
    QTest::newRow("error") << GPSWriteStatus::Error << QByteArray("TX-REFUSED error b5620a04");
    QTest::newRow("completed") << GPSWriteStatus::Completed << QByteArray("TX-REFUSED completed b5620a04");
    // The session was stopped; no link refused it.
    QTest::newRow("cancelled") << GPSWriteStatus::Cancelled << QByteArray();
}

void GPSRecordingTransportTest::_recordsWhatLinkRefused()
{
    QFETCH(GPSWriteStatus, answer);
    QFETCH(QByteArray, record);
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
    GPSTestClock clock(GPSTestClock::START_US);
    GPSCancelSource stop;
    {
        auto inner = std::make_unique<MemoryGPSTransport>(QByteArray(), 64, stop.token());
        MemoryGPSTransport* const link = inner.get();
        GPSRecordingTransport recorder(std::move(inner), file, GPSRecordingTransport::MAX_FILE_BYTES, {}, {},
                                       clock.source());
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        clock.advanceBy(1500us);
        QVERIFY(!recorder.setBaudRate(MemoryGPSTransport::REFUSED_BAUD_RATE));
        clock.advanceBy(1s);
        link->refuseWrites(answer);
        const GPSWriteResult refused = writeBytes(recorder, QByteArray::fromHex("b5620a04"));
        QCOMPARE(refused.status, answer);
        QCOMPARE(refused.acceptedBytes, 0);
        QVERIFY(link->written().isEmpty());
        // Nor does a link refuse the rate of a session that was stopped.
        QVERIFY(stop.cancel());
        QVERIFY(!recorder.setBaudRate(MemoryGPSTransport::REFUSED_BAUD_RATE));
    }
    QFile recording(file);
    QVERIFY(recording.open(QIODevice::ReadOnly));
    QCOMPARE(recording.readAll(), QByteArray(HEADER_LINE) + "1500 BAUD-REFUSED 1200\n" +
                                      (record.isEmpty() ? QByteArray() : "1001500 " + record + '\n'));

    QList<GPSRecordingRecord> expected{{.time = 1500us,
                                        .kind = GPSRecordingRecord::Kind::RefusedBaudRate,
                                        .bytes = {},
                                        .baudRate = MemoryGPSTransport::REFUSED_BAUD_RATE}};
    if (!record.isEmpty()) {
        expected.append({.time = 1001500us,
                         .kind = GPSRecordingRecord::Kind::RefusedWrite,
                         .bytes = QByteArray::fromHex("b5620a04"),
                         .baudRate = 0,
                         .answer = answer});
    }
    QString error;
    QCOMPARE(GPSRecordingReader::readFile(file, &error), expected);
    QCOMPARE(error, QString());
}

void GPSRecordingTransportTest::_recordsWriteGivenUpAtItsDeadline()
{
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
    const QByteArray command = QByteArray::fromHex("b5620a0400000e34");
    GPSTestClock clock(GPSTestClock::START_US);
    {
        auto inner = std::make_unique<MemoryGPSTransport>();
        MemoryGPSTransport* const link = inner.get();
        GPSRecordingTransport recorder(std::move(inner), file, GPSRecordingTransport::MAX_FILE_BYTES, {}, {},
                                       clock.source());
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        clock.advanceBy(2ms);
        // The time the session gave the command had run out when the write was made, so the link was not asked.
        const GPSWriteResult late = recorder.write(command, QDeadlineTimer());
        QCOMPARE(late.status, GPSWriteStatus::TimedOut);
        QCOMPARE(late.acceptedBytes, 0);
        QVERIFY(link->written().isEmpty());
        clock.advanceBy(1ms);
        QCOMPARE(writeBytes(recorder, command).status, GPSWriteStatus::Completed);
    }
    QCOMPARE(records(file), (QByteArrayList{"TX-REFUSED timed-out " + command.toHex(), "TX " + command.toHex()}));

    // A driver that repeats the session makes the write in time, on a clock that only the recording moves, and gets
    // the answer the recorded session got.
    GPSTestClock replayClock(GPSTestClock::START_US);
    GPSDialogueTransport replay(file, replayClock);
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    const GPSWriteResult answered = writeBytes(replay, command);
    QCOMPARE(answered.status, GPSWriteStatus::TimedOut);
    QCOMPARE(answered.acceptedBytes, 0);
    QCOMPARE(replayClock.nowUs(), GPSTestClock::START_US + 2000);
    QCOMPARE(writeBytes(replay, command).status, GPSWriteStatus::Completed);
    QVERIFY(replay.finished());
    QVERIFY(!replay.divergence());
}

void GPSRecordingTransportTest::_recordsSessionSettings_data()
{
    using Session = GPSRecordingTransport::Session;
    QTest::addColumn<std::optional<Session>>("session");
    QTest::addColumn<unsigned>("fixedBaudRate");
    QTest::addColumn<QByteArray>("settings");
    QTest::newRow("survey") << std::optional(
                                   Session{GPSType::UBlox,
                                           {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2.5,
                                                                                            .duration = 180s}}}})
                            << 0U
                            << QByteArray(
                                   "type=ublox,baud=0,fixed-baud=0,save=0,compact=0,"
                                   "base=survey,duration=180,accuracy=2.5");
    // Coordinates with every digit a double holds, on a network bridge.
    QTest::newRow("fixed") << std::optional(Session{
                                  GPSType::Automatic,
                                  {.base = {.mode =
                                                GPSBaseStationConfig::Fixed{
                                                    .position = {47.397741900000014, -122.54559380000002, 488.25},
                                                    .accuracyMeters = 0.5f},
                                            .compactObservations = true},
                                   .baudRate = 230400,
                                   .allowPersistentChanges = true}})
                           << GPSTransport::BRIDGE_BAUD_RATE
                           << QByteArray(
                                  "type=automatic,baud=230400,fixed-baud=115200,save=1,compact=1,base=fixed,"
                                  "latitude=47.397741900000014,longitude=-122.54559380000002,altitude=488.25,"
                                  "accuracy=0.5");
    QTest::newRow("averaging")
        << std::optional(Session{GPSType::Unicore, {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{90s}}}})
        << 0U
        << QByteArray(
               "type=unicore,baud=0,fixed-baud=0,save=0,compact=0,"
               "base=averaging,duration=90");
    // A base that leaves constellations out of its corrections names the ones it sends besides GPS.
    QTest::newRow("constellations") << std::optional(Session{
                                           GPSType::Unicore,
                                           {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{90s},
                                                     .correctionConstellations = GPSCorrectionConstellation::Galileo}}})
                                    << 0U
                                    << QByteArray(
                                           "type=unicore,baud=0,fixed-baud=0,save=0,compact=0,constellations=2,"
                                           "base=averaging,duration=90");
    QTest::newRow("passive") << std::optional(Session{GPSType::Passive, {.baudRate = 115200}}) << 0U
                             << QByteArray(
                                    "type=passive,baud=115200,fixed-baud=0,save=0,compact=0,"
                                    "base=survey,duration=0,accuracy=0");
    // A recorder that was not told its session writes the recording as before.
    QTest::newRow("not-given") << std::optional<Session>() << 0U << QByteArray();
}

void GPSRecordingTransportTest::_recordsSessionSettings()
{
    QFETCH(std::optional<GPSRecordingTransport::Session>, session);
    QFETCH(unsigned, fixedBaudRate);
    QFETCH(QByteArray, settings);
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
    GPSTestClock clock(GPSTestClock::START_US);
    {
        auto link = std::make_unique<MemoryGPSTransport>();
        link->setFixedBaudRate(fixedBaudRate);
        GPSRecordingTransport recorder(std::move(link), file, GPSRecordingTransport::MAX_FILE_BYTES, {}, session,
                                       clock.source());
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        // The records carry the time of the session's clock, which stands still unless the session waits.
        clock.advanceBy(1500us);
        QVERIFY(recorder.setBaudRate(38400));
    }
    QFile recording(file);
    QVERIFY(recording.open(QIODevice::ReadOnly));
    QCOMPARE(recording.readAll(), QByteArray(HEADER_LINE) +
                                      (settings.isEmpty() ? QByteArray() : "0 SETTINGS " + settings + '\n') +
                                      "1500 BAUD 38400\n");

    QString error;
    std::optional<GPSRecordingSettings> read;
    QCOMPARE(GPSRecordingReader::readFile(file, &error, &read),
             (QList<GPSRecordingRecord>{
                 {.time = 1500us, .kind = GPSRecordingRecord::Kind::BaudRate, .bytes = {}, .baudRate = 38400}}));
    QCOMPARE(error, QString());
    QCOMPARE(read.has_value(), session.has_value());
    if (session) {
        QCOMPARE(read->type, session->type);
        QCOMPARE(read->fixedBaudRate, fixedBaudRate);
        QCOMPARE(read->config.baudRate, session->config.baudRate);
        QCOMPARE(read->config.allowPersistentChanges, session->config.allowPersistentChanges);
        QVERIFY(read->config.base == session->config.base);
    }
}

void GPSRecordingTransportTest::_recordingReplaysToSameEvents()
{
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
    // A receiver's NMEA, UBX and RTCM3 output, then NMEA fixes.
    const QByteArray input = dataFile(FIXTURE_DIR, QStringLiteral("mixed.gps")) +
                             dataFile(FIXTURE_DIR, QStringLiteral("synthetic-gga.nmea"));
    QStringList live;
    {
        GPSRecordingTransport recorder(std::make_unique<MemoryGPSTransport>(input, 100), file);
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        // The session's own traffic lies between the reads.
        live = decodedEvents(recorder, 512, [&recorder](int read) {
            (void) recorder.setBaudRate(read % 2 ? 115200 : 230400);
            (void) writeBytes(recorder, QByteArrayLiteral("$PQTMVERNO*58\r\n"));
        });
    }
    QVERIFY2(!live.filter(QStringLiteral("position ")).isEmpty() && !live.filter(QStringLiteral("rtcm ")).isEmpty(),
             qPrintable(live.join(u'\n')));

    GPSCancelSource stop;
    ReplayGPSTransport replay(file, stop.token());
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    // Reads that cut the recorded ones.
    const QStringList replayed = decodedEvents(replay, 64, [](int) {});
    QVERIFY(replay.finished());
    QCOMPARE(replay.bytesDelivered(), input.size());
    QCOMPARE(replayed, live);
}

void GPSRecordingTransportTest::_readerReturnsBothDirectionsWithTimes()
{
    using Kind = GPSRecordingRecord::Kind;
    TestFixtures::TempDirFixture directory;
    // As the recorder writes it, and after a transfer that converted the line endings.
    const QByteArray text = QByteArray(HEADER_LINE) +
                            "0 BAUD 115200\n"
                            "1500 TX b5620a0400000e34\n"
                            "1500 RX b562\n"
                            "2250000 RX 0a04\n"
                            "2250000 BAUD-REFUSED 921600\n"
                            "2750000 TX-REFUSED timed-out b5620a04\n";
    const QList<GPSRecordingRecord> expected{
        {.time = 0us, .kind = Kind::BaudRate, .bytes = {}, .baudRate = 115200},
        {.time = 1500us, .kind = Kind::Sent, .bytes = QByteArray::fromHex("b5620a0400000e34"), .baudRate = 0},
        {.time = 1500us, .kind = Kind::Received, .bytes = QByteArray::fromHex("b562"), .baudRate = 0},
        {.time = 2250ms, .kind = Kind::Received, .bytes = QByteArray::fromHex("0a04"), .baudRate = 0},
        {.time = 2250ms, .kind = Kind::RefusedBaudRate, .bytes = {}, .baudRate = 921600},
        {.time = 2750ms,
         .kind = Kind::RefusedWrite,
         .bytes = QByteArray::fromHex("b5620a04"),
         .baudRate = 0,
         .answer = GPSWriteStatus::TimedOut},
    };
    for (const QByteArray& lineEnding : {QByteArray("\n"), QByteArray("\r\n")}) {
        QByteArray converted = text;
        converted.replace("\n", lineEnding);
        const QString file = directory.createFile(QStringLiteral("session-%1.log").arg(lineEnding.size()), converted);
        QString error;
        QCOMPARE(GPSRecordingReader::readFile(file, &error), expected);
        QCOMPARE(error, QString());
    }
    // A session that crossed nothing is a recording without records.
    QString error;
    QVERIFY(
        GPSRecordingReader::readFile(directory.createFile(QStringLiteral("empty.log"), HEADER_LINE), &error).isEmpty());
    QCOMPARE(error, QString());
}

void GPSRecordingTransportTest::_readerStopsAtDamage_data()
{
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<int>("records");
    QTest::addColumn<QString>("error");
    const QByteArray header(HEADER_LINE);
    QTest::newRow("no-header") << QByteArray("0 RX aa\n") << 0 << QStringLiteral("not a receiver recording");
    QTest::newRow("other-version") << QByteArray("QGC GPS receiver recording 2\n0 RX aa\n") << 0
                                   << QStringLiteral("not a receiver recording");
    // What a session leaves that was killed between two flushes of its file.
    QTest::newRow("cut-in-a-record") << header + "0 RX aa\n10 RX bb" << 1
                                     << QStringLiteral("line 3: the recording ends within a record");
    QTest::newRow("time-goes-back") << header + "10 RX aa\n10 TX bb\n9 RX cc\n20 RX dd\n"
                                    << 2
                                    << QStringLiteral(
                                           "line 4: not a time at or after the time of the record before it");
    QTest::newRow("no-time") << header + "soon RX aa\n"
                             << 0 << QStringLiteral("line 2: not a time at or after the time of the record before it");
    QTest::newRow("missing-field") << header + "0 RX aa\n5 RX\n"
                                   << 1 << QStringLiteral("line 3: not a time, a kind and a value");
    QTest::newRow("blank-line") << header + "\n0 RX aa\n"
                                << 0 << QStringLiteral("line 2: not a time, a kind and a value");
    QTest::newRow("unknown-kind") << header + "0 ERR aa\n" << 0 << QStringLiteral("line 2: unknown record kind");
    QTest::newRow("half-a-byte") << header + "0 TX b56\n"
                                 << 0 << QStringLiteral("line 2: not whole bytes in lower-case hexadecimal");
    QTest::newRow("not-hexadecimal") << header + "0 RX b5g2\n"
                                     << 0 << QStringLiteral("line 2: not whole bytes in lower-case hexadecimal");
    QTest::newRow("upper-case") << header + "0 RX B562\n"
                                << 0 << QStringLiteral("line 2: not whole bytes in lower-case hexadecimal");
    QTest::newRow("no-bytes") << header + "0 RX \n"
                              << 0 << QStringLiteral("line 2: not whole bytes in lower-case hexadecimal");
    QTest::newRow("no-baud-rate") << header + "0 BAUD fast\n" << 0 << QStringLiteral("line 2: not a baud rate");
    QTest::newRow("no-refused-baud-rate") << header + "0 BAUD-REFUSED\n"
                                          << 0 << QStringLiteral("line 2: not a time, a kind and a value");
    QTest::newRow("refused-baud-rate-with-an-answer") << header + "0 BAUD-REFUSED unsupported 9600\n"
                                                      << 0 << QStringLiteral("line 2: not a time, a kind and a value");
    QTest::newRow("refused-write-without-answer")
        << header + "0 RX aa\n5 TX-REFUSED b562\n"
        << 1 << QStringLiteral("line 3: not a time, a kind, the answer of the link and a value");
    QTest::newRow("refused-write-with-another-answer")
        << header + "0 TX-REFUSED refused b562\n"
        << 0 << QStringLiteral("line 2: not completed, timed-out, error or unsupported");
    // A cancelled write was not refused by its link.
    QTest::newRow("refused-write-cancelled")
        << header + "0 TX-REFUSED cancelled b562\n"
        << 0 << QStringLiteral("line 2: not completed, timed-out, error or unsupported");
    QTest::newRow("refused-write-of-half-a-byte")
        << header + "0 TX-REFUSED unsupported b56\n"
        << 0 << QStringLiteral("line 2: not whole bytes in lower-case hexadecimal");
    const QByteArray settings("0 SETTINGS type=ublox,baud=0,fixed-baud=0,save=0,compact=0,base=survey,duration=60");
    QTest::newRow("settings-after-a-record") << header + "0 RX aa\n" + settings + ",accuracy=2\n"
                                             << 1 << QStringLiteral("line 3: the settings are not the first record");
    QTest::newRow("settings-of-no-receiver")
        << header + QByteArray(settings).replace("ublox", "sextant") + ",accuracy=2\n0 RX aa\n"
        << 0 << QStringLiteral("line 2: not the settings of a session");
    QTest::newRow("setting-missing") << header + settings + "\n0 RX aa\n"
                                     << 0 << QStringLiteral("line 2: not the settings of a session");
    QTest::newRow("setting-of-another-base-mode") << header + settings + ",accuracy=2,latitude=47\n0 RX aa\n"
                                                  << 0 << QStringLiteral("line 2: not the settings of a session");
    QTest::newRow("setting-not-a-number") << header + settings + ",accuracy=two\n0 RX aa\n"
                                          << 0 << QStringLiteral("line 2: not the settings of a session");
}

void GPSRecordingTransportTest::_readerStopsAtDamage()
{
    QFETCH(QByteArray, text);
    QFETCH(int, records);
    QFETCH(QString, error);
    TestFixtures::TempDirFixture directory;
    const QString file = directory.createFile(QStringLiteral("session.log"), text);
    QString reported;
    QCOMPARE(GPSRecordingReader::readFile(file, &reported).size(), records);
    QCOMPARE(reported, error);
}

void GPSRecordingTransportTest::_replayEndsAtDamage()
{
    TestFixtures::TempDirFixture directory;
    const QString file = directory.createFile(QStringLiteral("session.log"),
                                              QByteArray(HEADER_LINE) + "0 RX aabb\n5 TX cc\n9 RX dd\n12 RX e");
    GPSCancelSource stop;
    ReplayGPSTransport replay(file, stop.token());
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    // The records before the damage are delivered, and then the reads fail with what the reader found.
    expectLogMessage("Test.GPS.Transport.ReplayGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("line 5: the recording ends within a record")));
    QCOMPARE(readAvailable(replay), QByteArray::fromHex("aabbdd"));
    verifyExpectedLogMessage();
    uint8_t buffer[8];
    const GPSReadResult failed = replay.read(buffer, 0ms);
    QCOMPARE(failed.status, GPSReadStatus::Error);
    QCOMPARE(failed.detail, QStringLiteral("line 5: the recording ends within a record"));
    QVERIFY(replay.fatalError());
    QVERIFY(!replay.finished());
    QCOMPARE(replay.sentBytes(), 1);
}

void GPSRecordingTransportTest::_replayKeepsRecordedTimes()
{
    TestFixtures::TempDirFixture directory;
    // The receiver's second answer came 60 ms into the session; what the session sent takes no time to replay.
    constexpr std::chrono::milliseconds SECOND_ANSWER{60};
    const QString file = directory.createFile(
        QStringLiteral("session.log"),
        QByteArray(HEADER_LINE) +
            "0 RX aa\n1000 TX cc\n1000 TX-REFUSED unsupported dd\n60000 RX bb\n60000 BAUD-REFUSED 4800\n"
            "60000 BAUD 9600\n");
    GPSCancelSource stop;
    ReplayGPSTransport replay(file, stop.token());
    replay.setRecordedTiming(true);
    QElapsedTimer elapsed;
    elapsed.start();
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    QVERIFY(replay.recording());
    QByteArray received;
    uint8_t buffer[8];
    while (!replay.finished()) {
        const GPSReadResult result = replay.read(buffer, TestTimeout::shortDuration());
        QCOMPARE(result.status, GPSReadStatus::Data);
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
    }
    // Only a lower bound: the replay never runs ahead of the recording.
    QVERIFY2(elapsed.durationElapsed() >= SECOND_ANSWER, qPrintable(QString::number(elapsed.elapsed())));
    QCOMPARE(received, QByteArray::fromHex("aabb"));
    // Without what the link refused.
    QCOMPARE(replay.sentBytes(), 1);
    QCOMPARE(replay.baudRateChanges(), 1);
}

void GPSRecordingTransportTest::_fileErrorKeepsLink_data()
{
    QTest::addColumn<QString>("failure");
    QTest::newRow("unwritable-directory") << QStringLiteral("unwritable-directory");
    QTest::newRow("write-error") << QStringLiteral("write-error");
    QTest::newRow("existing-file") << QStringLiteral("existing-file");
}

void GPSRecordingTransportTest::_fileErrorKeepsLink()
{
    QFETCH(QString, failure);
    TestFixtures::TempDirFixture directory;
    QString file;
    if (failure == QStringLiteral("unwritable-directory")) {
        // A regular file where the recording folder should be.
        file = directory.createFile(QStringLiteral("GPSReceiver"), "not a folder") + QStringLiteral("/session.log");
    } else if (failure == QStringLiteral("existing-file")) {
        file = directory.createFile(QStringLiteral("session.log"), "an earlier recording");
    } else {
#ifdef Q_OS_LINUX
        file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
#else
        QSKIP("Needs /proc/self/fd and /dev/full to fail the writes to an open file");
#endif
    }
    // More than QFile's write buffer, so a failing device reports the error during the session.
    const QByteArray input = bytePattern(64 * 1024, 5);
    auto inner = std::make_unique<MemoryGPSTransport>(input, 4096);
    MemoryGPSTransport* const link = inner.get();
    int stops = 0;
    GPSRecordingTransport recorder(std::move(inner), file, GPSRecordingTransport::MAX_FILE_BYTES,
                                   [&stops]() { ++stops; });

    expectLogMessage(RECORDING_LOG, QtWarningMsg, QRegularExpression(QStringLiteral("file error")));
    QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
#ifdef Q_OS_LINUX
    // The file opens, and then its disk is full.
    if (failure == QStringLiteral("write-error")) {
        QVERIFY(recorder.recording());
        QVERIFY(failWritesTo(file));
    }
#endif
    QCOMPARE(readAvailable(recorder), input);
    verifyExpectedLogMessage();
    QVERIFY(!recorder.recording());
    QCOMPARE(stops, 1);
    QVERIFY(recorder.setBaudRate(57600));
    QCOMPARE(link->baudRate(), 57600U);
    QCOMPARE(writeBytes(recorder, QByteArrayLiteral("$PQTMVERNO*58\r\n")).status, GPSWriteStatus::Completed);
    QCOMPARE(link->written(), QByteArrayLiteral("$PQTMVERNO*58\r\n"));
    QVERIFY(!recorder.fatalError());
    if (failure == QStringLiteral("existing-file")) {
        QFile earlier(file);
        QVERIFY(earlier.open(QIODevice::ReadOnly));
        QCOMPARE(earlier.readAll(), QByteArrayLiteral("an earlier recording"));
    }
}

void GPSRecordingTransportTest::_fileStopsAtSizeCap()
{
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.log"));
    const QByteArray input = bytePattern(250, 1);
    constexpr qint64 maxFileBytes = 250;
    int stops = 0;
    {
        // Room for the header and three of the nine reads, whose records take about 70 bytes each.
        GPSRecordingTransport recorder(std::make_unique<MemoryGPSTransport>(input, 30), file, maxFileBytes,
                                       [&stops]() { ++stops; });
        QCOMPARE(recorder.open().status, GPSOpenStatus::Opened);
        expectLogMessage(RECORDING_LOG, QtWarningMsg, QRegularExpression(QStringLiteral("size limit")));
        QCOMPARE(readAvailable(recorder), input);
        verifyExpectedLogMessage();
        QVERIFY(!recorder.recording());
        QCOMPARE(writeBytes(recorder, input).status, GPSWriteStatus::Completed);
    }
    // Reported once, and not again by the traffic and the destruction that follow.
    QCOMPARE(stops, 1);
    QVERIFY(QFileInfo(file).size() <= maxFileBytes);
    // The start of the session, in whole records.
    QCOMPARE(records(file), (QByteArrayList{
                                "RX " + input.first(30).toHex(),
                                "RX " + input.sliced(30, 30).toHex(),
                                "RX " + input.sliced(60, 30).toHex(),
                            }));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSRecordingTransportTest, TestLabel::Unit)
