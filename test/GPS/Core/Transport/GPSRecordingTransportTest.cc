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
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QTimeZone>

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
#include "Transport/Support/ReplayGPSTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
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

    bool drained() const { return _offset >= _input.size(); }

    const QByteArray& written() const { return _written; }

    unsigned baudRate() const { return _baudRate; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer) override
    {
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
    unsigned _baudRate = 0;
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

/// The records of the recording at @a path as "<kind> <value>", in file order. Whatever breaks the format stays
/// whole: a missing header, an unterminated last line, a line without a time, or a time before the one above it.
QByteArrayList records(const QString& path)
{
    QByteArrayList lines = readFile(path).value_or(QByteArray()).split('\n');
    if (lines.size() < 2 || lines.takeFirst() != GPSRecordingTransport::FILE_HEADER || !lines.takeLast().isEmpty()) {
        return {"not a recording"};
    }
    qint64 previousUs = 0;
    for (QByteArray& line : lines) {
        const qsizetype space = line.indexOf(' ');
        bool timed = false;
        const qint64 timeUs = line.left(space).toLongLong(&timed);
        if (timed && timeUs >= previousUs) {
            previousUs = timeUs;
            line.remove(0, space + 1);
        }
    }
    return lines;
}

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
    // What the link refused, a rate or the rest of a write, never crossed it.
    QCOMPARE(records(file), (QByteArrayList{
                                "BAUD 38400",
                                "RX " + input.first(40).toHex(),
                                "TX " + command.toHex(),
                                "RX " + input.sliced(40, 40).toHex(),
                                "TX " + command.first(3).toHex(),
                                "RX " + input.sliced(80).toHex(),
                            }));
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

void GPSRecordingTransportTest::_fileErrorKeepsLink_data()
{
    QTest::addColumn<QString>("failure");
    QTest::newRow("unwritable-directory") << QStringLiteral("unwritable-directory");
    QTest::newRow("write-error") << QStringLiteral("write-error");
}

void GPSRecordingTransportTest::_fileErrorKeepsLink()
{
    QFETCH(QString, failure);
    TestFixtures::TempDirFixture directory;
    QString file;
    if (failure == QStringLiteral("unwritable-directory")) {
        // A regular file where the recording folder should be.
        file = directory.createFile(QStringLiteral("GPSReceiver"), "not a folder") + QStringLiteral("/session.log");
    } else {
        if (!QFileInfo::exists(QStringLiteral("/dev/full"))) {
            QSKIP("Needs /dev/full to fail file writes");
        }
        file = QStringLiteral("/dev/full");
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
    QCOMPARE(readAvailable(recorder), input);
    verifyExpectedLogMessage();
    QVERIFY(!recorder.recording());
    QCOMPARE(stops, 1);
    QVERIFY(recorder.setBaudRate(57600));
    QCOMPARE(link->baudRate(), 57600U);
    QCOMPARE(writeBytes(recorder, QByteArrayLiteral("$PQTMVERNO*58\r\n")).status, GPSWriteStatus::Completed);
    QCOMPARE(link->written(), QByteArrayLiteral("$PQTMVERNO*58\r\n"));
    QVERIFY(!recorder.fatalError());
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
