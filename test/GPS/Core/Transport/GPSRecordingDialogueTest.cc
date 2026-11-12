#include "GPSRecordingDialogueTest.h"

#include <array>
#include <chrono>
#include <memory>
#include <span>
#include <variant>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>

#include "Fixtures/RAIIFixtures.h"
#include "GPSDriver.h"
#include "GPSReceiverWorker.h"
#include "GPSRecordingTransport.h"
#include "GPSTransport.h"
#include "Protocols/Scenarios/GoldenChecks.h"
#include "Protocols/Support/GoldenFormat.h"
#include "Transport/Support/GPSRecordingDialogue.h"
#include "Transport/Support/GPSRecordingReader.h"

using namespace std::chrono_literals;
using namespace GPSTest;
using namespace GPSTest::GoldenScenario;

namespace {

constexpr const char* HEADER_LINE = "QGC GPS receiver recording 1\n";
/// A serial session that surveys its position, which no test here configures.
constexpr const char* SETTINGS_LINE =
    "0 SETTINGS type=ublox,baud=0,fixed-baud=0,save=0,compact=0,base=survey,duration=60,accuracy=2\n";
/// Passive input, which is ready once its rate is set.
constexpr const char* PASSIVE_SESSION =
    "0 SETTINGS type=passive,baud=115200,fixed-baud=0,save=0,compact=0,base=survey,duration=0,accuracy=0\n"
    "0 BAUD 115200\n";

/// The receiver of a scenario as the link a recorder owns.
class BenchLink final : public GPSTransport
{
public:
    explicit BenchLink(ReceiverBench& bench)
        : GPSTransport(bench.cancelToken())
        , _bench(bench)
    {}

    // The scenario prepared its receiver, which opening it would reset.
    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return _bench.fatalError(); }

    unsigned fixedBaudRate() const override { return _bench.fixedBaudRate(); }

    std::chrono::milliseconds configurationWriteTimeout() const override { return _bench.configurationWriteTimeout(); }

    bool setBaudRate(unsigned baudRate) override { return _bench.setBaudRate(baudRate); }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override
    {
        return _bench.read(buffer, timeout);
    }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override
    {
        return _bench.write(bytes, deadline);
    }

private:
    ReceiverBench& _bench;
};

/// Runs @a scenario as the golden transcripts do, GPSDriver against the scenario's receiver model on a virtual clock,
/// through the recorder, which writes the session to @a file with the times of that clock.
void recordScenario(const ScenarioDef& scenario, const QString& file)
{
    GPSTestClock clock(GOLDEN_START_US);
    const auto bench = scenario.bench(clock);
    bench->startSession();
    GPSClock sessionClock = clock.source();
    if (bench->wait) {
        sessionClock.wait = bench->wait;
    }
    GPSRecordingTransport recorder(std::make_unique<BenchLink>(*bench), file, GPSRecordingTransport::MAX_FILE_BYTES, {},
                                   GPSRecordingTransport::Session{scenario.type, scenario.config}, sessionClock);
    (void) recorder.open();
    GPSDriver driver(scenario.type, recorder, scenario.config, {}, sessionClock);
    if (!driver.configure()) {
        return;
    }
    int receives = 0;
    for (const auto& step : scenario.stream) {
        if (!step.bytes.isEmpty()) {
            bench->queueReply(step.bytes);
        }
        const uint64_t until =
            clock.nowUs() +
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(step.duration).count());
        GPSReceiveResult outcome;
        do {
            outcome = driver.receiveOutcome(Golden::RECEIVE_TIMEOUT);
            ++receives;
        } while (outcome.error == GPSProtocolError::None && clock.nowUs() < until &&
                 receives < Golden::MAX_RECEIVE_CALLS);
        if (outcome.error != GPSProtocolError::None || receives >= Golden::MAX_RECEIVE_CALLS) {
            return;
        }
    }
}

/// What a session sent and the rates it set, in order: "TX <bytes>" and "BAUD <rate>", with the bytes between two
/// rate changes as one entry however the writes divided them.
struct Wire
{
    QByteArrayList entries;

    void sent(const QByteArray& bytes)
    {
        if (entries.isEmpty() || !entries.last().startsWith("TX ")) {
            entries.append("TX ");
        }
        entries.last() += bytes.toHex();
    }

    void baudRate(unsigned rate) { entries.append("BAUD " + QByteArray::number(rate)); }
};

/// The wire of the session a recording holds.
QByteArrayList recordedWire(const QList<GPSRecordingRecord>& records)
{
    Wire wire;
    for (const GPSRecordingRecord& record : records) {
        if (record.kind == GPSRecordingRecord::Kind::Sent) {
            wire.sent(record.bytes);
        } else if (record.kind == GPSRecordingRecord::Kind::BaudRate) {
            wire.baudRate(record.baudRate);
        }
    }
    return wire.entries;
}

/// The wire of @a scenario as its golden transcript records it: what the link accepted.
QByteArrayList goldenWire(const ScenarioDef& scenario)
{
    GPSTestClock clock(GOLDEN_START_US);
    const auto bench = scenario.bench(clock);
    Wire wire;
    for (const Golden::Record& record :
         Golden::runGolden({scenario.type, scenario.config, scenario.stream}, link(*bench)).records) {
        if (const auto* write = std::get_if<Golden::Write>(&record.value)) {
            if (write->acceptedBytes > 0) {
                wire.sent(write->bytes.first(write->acceptedBytes));
            }
        } else if (const auto* baud = std::get_if<Golden::Baud>(&record.value); baud && baud->accepted) {
            wire.baudRate(baud->rate);
        }
    }
    return wire.entries;
}

/// The lines of the recording at @a file: the header, the settings, then one record each.
QByteArrayList recordingLines(const QString& file)
{
    QByteArrayList lines = readFile(file).value_or(QByteArray()).split('\n');
    lines.removeLast();
    return lines;
}

QString writeRecording(TestFixtures::TempDirFixture& directory, const QString& name, const QByteArrayList& lines)
{
    QFile::remove(QDir(directory.path()).filePath(name));
    return directory.createFile(name, lines.join('\n') + '\n');
}

/// A recording that differs from the session it was made of in one record, and where its replay has to fail.
struct ChangedRecording
{
    const char* change;
    QByteArrayList lines;
    /// The line of the record the driver departs from, and the start of what the divergence says the recording has
    /// there and the driver did.
    qint64 line;
    QString recorded;
    QString driver;
};

/// Every recording that differs from @a lines in one TX byte, in one baud rate, or by one TX record that the session
/// wrote as soon as it had read the receiver's bytes before it and that the receiver answered.
QList<ChangedRecording> changedRecordings(const QByteArrayList& lines)
{
    QList<ChangedRecording> changed;
    const auto fields = [&lines](qsizetype index) {
        return index < lines.size() ? lines[index].split(' ') : QByteArrayList{{}, {}, {}};
    };
    for (qsizetype index = 2; index < lines.size(); ++index) {
        const QByteArrayList record = fields(index);
        const QByteArray& time = record[0];
        const qint64 line = index + 1;
        if (record[1] == GPSRecordingTransport::BAUD) {
            const QByteArray rate = record[2] == "9600" ? "19200" : "9600";
            QByteArrayList other = lines;
            other[index] = time + " BAUD " + rate;
            changed.append({"changed baud rate", other, line, QStringLiteral("BAUD %1 at").arg(QLatin1StringView(rate)),
                            QStringLiteral("set the link to %1 baud at").arg(QLatin1StringView(record[2]))});
        } else if (record[1] == GPSRecordingTransport::SENT) {
            QByteArray bytes = QByteArray::fromHex(record[2]);
            const qsizetype byte = bytes.size() / 2;
            bytes[byte] ^= 0x01;
            QByteArrayList other = lines;
            other[index] = time + " TX " + bytes.toHex();
            changed.append({"changed TX byte", other, line,
                            QStringLiteral("TX %1 bytes %2 at").arg(bytes.size()).arg(QLatin1StringView(bytes.toHex())),
                            QStringLiteral("which differ from byte %1 of the record").arg(byte + 1)});
            const QByteArrayList before = fields(index - 1);
            const QByteArrayList after = fields(index + 1);
            if (before[1] == GPSRecordingTransport::RECEIVED && before[0] == time &&
                after[1] == GPSRecordingTransport::RECEIVED) {
                other = lines;
                other.removeAt(index);
                changed.append(
                    {"dropped TX record", other, line, QStringLiteral("RX "),
                     QStringLiteral("wrote %1 bytes %2").arg(bytes.size()).arg(QLatin1StringView(record[2]))});
            }
        }
    }
    return changed;
}

/// Reads up to @a size bytes from @a link within @a timeout.
struct Read
{
    Read(GPSTransport& link, std::chrono::milliseconds timeout, size_t size = 16)
    {
        std::array<uint8_t, 16> buffer{};
        const GPSReadResult result = link.read(std::span(buffer).first(size), timeout);
        status = result.status;
        bytes = QByteArray(reinterpret_cast<const char*>(buffer.data()), result.bytesRead).toHex();
    }

    GPSReadStatus status;
    QByteArray bytes;
};

GPSWriteStatus writeHex(GPSTransport& link, const char* hex)
{
    return link.write(QByteArray::fromHex(hex), QDeadlineTimer(TestTimeout::shortDuration())).status;
}

}  // namespace

void GPSRecordingDialogueTest::init()
{
    GPSProtocolTestBase::init();
    // The scenarios include receivers that fail, and a replay ends its driver's session with its recording: the
    // protocols and GPSDriver warn about both by design.
    const GPSProtocolLogCapture protocolWarnings;
    ignoreLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression());
}

void GPSRecordingDialogueTest::_modelSessionReplays_data()
{
    QTest::addColumn<int>("index");
    const auto& rows = scenarios();
    for (qsizetype index = 0; index < std::ssize(rows); ++index) {
        QTest::newRow(qPrintable(goldenName(rows[static_cast<size_t>(index)]))) << static_cast<int>(index);
    }
}

void GPSRecordingDialogueTest::_modelSessionReplays()
{
    QFETCH(int, index);
    const ScenarioDef& scenario = scenarios()[static_cast<size_t>(index)];
    TestFixtures::TempDirFixture directory;
    const QString file = QDir(directory.path()).filePath(QStringLiteral("session.recording"));
    recordScenario(scenario, file);
    QString damage;
    const QList<GPSRecordingRecord> records = GPSRecordingReader::readFile(file, &damage);
    QCOMPARE(damage, QString());
    // The session that was recorded is the one the scenario's golden transcript pins.
    QCOMPARE(recordedWire(records), goldenWire(scenario));

    const GPSDialogueResult replayed = replayDialogue(file);
    QVERIFY2(replayed.passed(), qPrintable(replayed.failure()));
    QCOMPARE(replayed.records, records.size());
    QVERIFY(replayed.settings);
    QCOMPARE(replayed.settings->type, scenario.type);
}

void GPSRecordingDialogueTest::_changedRecordingFails_data()
{
    QTest::addColumn<GPSType>("family");
    // The families that have a receiver model, and receiver detection, which runs against the same models.
    for (const GPSType family : {GPSType::UBlox, GPSType::Septentrio, GPSType::Trimble, GPSType::Femto,
                                 GPSType::Unicore, GPSType::Quectel, GPSType::Automatic}) {
        QTest::newRow(qPrintable(Golden::typeName(family))) << family;
    }
}

void GPSRecordingDialogueTest::_changedRecordingFails()
{
    QFETCH(GPSType, family);
    TestFixtures::TempDirFixture directory;
    QHash<QString, int> changes;
    for (const ScenarioDef& scenario : scenarios()) {
        if (scenario.type != family) {
            continue;
        }
        const QString file = QDir(directory.path()).filePath(QStringLiteral("session.recording"));
        QFile::remove(file);
        recordScenario(scenario, file);
        for (const ChangedRecording& changed : changedRecordings(recordingLines(file))) {
            const GPSDialogueResult replayed =
                replayDialogue(writeRecording(directory, QStringLiteral("changed.recording"), changed.lines));
            const QString found = QStringLiteral("%1, %2 on line %3: %4")
                                      .arg(goldenName(scenario), QLatin1StringView(changed.change))
                                      .arg(changed.line)
                                      .arg(replayed.passed() ? QStringLiteral("passed") : replayed.failure());
            QVERIFY2(replayed.divergence, qPrintable(found));
            QVERIFY2(replayed.divergence->line == changed.line, qPrintable(found));
            QVERIFY2(replayed.divergence->recorded.startsWith(changed.recorded), qPrintable(found));
            QVERIFY2(replayed.divergence->driver.contains(changed.driver), qPrintable(found));
            ++changes[QLatin1StringView(changed.change)];
        }
    }
    QVERIFY(changes.value(QStringLiteral("changed TX byte")) > 0);
    QVERIFY(changes.value(QStringLiteral("changed baud rate")) > 0);
    QVERIFY(changes.value(QStringLiteral("dropped TX record")) > 0);
}

void GPSRecordingDialogueTest::_receivedBytesArriveAtRecordedTimes()
{
    TestFixtures::TempDirFixture directory;
    const QString file = directory.createFile(QStringLiteral("session.recording"),
                                              QByteArray(HEADER_LINE) + "5000 RX aabbcc\n5000 RX dd\n60000 RX ee\n");
    constexpr uint64_t START_US = GPSTestClock::START_US;
    GPSTestClock clock(START_US);
    GPSDialogueTransport link(file, clock);
    QVERIFY(link.fatalError());
    QCOMPARE(link.open().status, GPSOpenStatus::Opened);
    QVERIFY(!link.fatalError());
    QVERIFY(!link.settings());

    // A read that ends before the bytes arrived finds nothing and takes its timeout.
    QCOMPARE(Read(link, 1ms).status, GPSReadStatus::TimedOut);
    QCOMPARE(clock.nowUs(), START_US + 1000);
    // The read that reaches them returns when they arrived, one recorded read at a time, in the buffer it has.
    QCOMPARE(Read(link, 10ms, 2).bytes, "aabb");
    QCOMPARE(clock.nowUs(), START_US + 5000);
    QCOMPARE(Read(link, 10ms).bytes, "cc");
    QCOMPARE(Read(link, 10ms).bytes, "dd");
    QCOMPARE(clock.nowUs(), START_US + 5000);

    QCOMPARE(Read(link, 50ms).status, GPSReadStatus::TimedOut);
    QCOMPARE(clock.nowUs(), START_US + 55000);
    QCOMPARE(Read(link, 2ms).status, GPSReadStatus::TimedOut);
    // Bytes recorded within READ_LATENESS of a read's timeout are those the read returned late.
    QCOMPARE(Read(link, 1ms).bytes, "ee");
    QCOMPARE(clock.nowUs(), START_US + 60000);

    // The recording says nothing about what followed its last record, so the session ends there.
    QVERIFY(link.finished());
    QCOMPARE(Read(link, 10ms).status, GPSReadStatus::Cancelled);
    QVERIFY(link.isCancelled());
    QVERIFY(!link.divergence());
    QCOMPARE(link.records(), 3);
    QCOMPARE(link.receivedBytes(), 5);
}

void GPSRecordingDialogueTest::_writesAreComparedAsOneStream()
{
    TestFixtures::TempDirFixture directory;
    const QString file = directory.createFile(
        QStringLiteral("session.recording"),
        QByteArray(HEADER_LINE) + QByteArray(SETTINGS_LINE).replace("fixed-baud=0", "fixed-baud=115200") +
            "0 BAUD 115200\n0 TX aabb\n0 TX cc\n10 RX 01\n20 TX ddee\n");
    GPSTestClock clock(GPSTestClock::START_US);
    GPSDialogueTransport link(file, clock);
    QCOMPARE(link.open().status, GPSOpenStatus::Opened);
    QCOMPARE(link.fixedBaudRate(), 115200U);

    // The recorded link ran at one rate: it refused any other, which a recording does not hold.
    QVERIFY(!link.setBaudRate(9600));
    QVERIFY(link.setBaudRate(115200));
    // The recorded session wrote before it read more, so the receiver's next bytes wait for the driver.
    QCOMPARE(Read(link, 100ms).status, GPSReadStatus::TimedOut);
    // Two recorded writes as other calls, and one as two.
    QCOMPARE(writeHex(link, "aa"), GPSWriteStatus::Completed);
    QCOMPARE(writeHex(link, "bbcc"), GPSWriteStatus::Completed);
    QCOMPARE(Read(link, 100ms).bytes, "01");
    QCOMPARE(writeHex(link, "dd"), GPSWriteStatus::Completed);
    QVERIFY(!link.finished());
    QCOMPARE(writeHex(link, "ee"), GPSWriteStatus::Completed);

    QVERIFY(link.finished());
    QVERIFY(!link.divergence());
    QCOMPARE(link.records(), 5);
    QCOMPARE(link.sentBytes(), 5);
    QCOMPARE(link.baudRateChanges(), 1);
}

void GPSRecordingDialogueTest::_departureNamesTheRecord_data()
{
    QTest::addColumn<QByteArray>("records");
    QTest::addColumn<QStringList>("calls");
    QTest::addColumn<int>("line");
    QTest::addColumn<QString>("recorded");
    QTest::addColumn<QString>("driver");
    QTest::newRow("another-byte") << QByteArray("1000 TX aabbcc\n") << QStringList{"write aab0cc"} << 3
                                  << "TX 3 bytes aabbcc at 0.001000 s"
                                  << "wrote 3 bytes aab0cc there, which differ from byte 2 of the record at 0.000000 s";
    QTest::newRow("another-byte-in-a-later-write")
        << QByteArray("1000 TX aabbcc\n") << QStringList{"write aa", "write bb00"} << 3
        << "TX 3 bytes aabbcc at 0.001000 s"
        << "wrote 2 bytes bb00 there, which differ from byte 3 of the record at 0.000000 s";
    QTest::newRow("more-than-recorded") << QByteArray("0 TX aa\n1000 RX 01\n") << QStringList{"write aabb"} << 4
                                        << "RX 1 bytes 01 at 0.001000 s"
                                        << "wrote 1 bytes bb at 0.000000 s";
    QTest::newRow("before-the-receiver-answered")
        << QByteArray("0 TX aa\n500000 RX 01\n600000 TX bb\n") << QStringList{"write aa", "read 200", "write bb"} << 4
        << "RX 1 bytes 01 at 0.500000 s"
        << "wrote 1 bytes bb at 0.200000 s";
    QTest::newRow("another-rate") << QByteArray("0 BAUD 9600\n") << QStringList{"baud 115200"} << 3
                                  << "BAUD 9600 at 0.000000 s"
                                  << "set the link to 115200 baud at 0.000000 s";
    QTest::newRow("a-rate-for-a-write") << QByteArray("0 BAUD 9600\n0 TX aabb\n")
                                        << QStringList{"baud 9600", "write aa", "baud 115200"} << 4
                                        << "TX 2 bytes aabb at 0.000000 s"
                                        << "set the link to 115200 baud at 0.000000 s";
    // The driver waits for an answer to what it never wrote.
    QTest::newRow("nothing-written") << QByteArray("1000000 TX aa\n")
                                     << QStringList{"read 20000", "read 20000", "read 20000"} << 3
                                     << "TX 1 bytes aa at 1.000000 s"
                                     << "wrote nothing at 40.000000 s";
    QTest::newRow("part-written") << QByteArray("0 TX aabb\n") << QStringList{"write aa", "read 31000", "read 1"} << 3
                                  << "TX 2 bytes aabb at 0.000000 s"
                                  << "wrote only its first 1 bytes at 31.000000 s";
    QTest::newRow("no-rate-set") << QByteArray("0 BAUD 9600\n") << QStringList{"read 31000", "read 1"} << 3
                                 << "BAUD 9600 at 0.000000 s"
                                 << "has not set that rate at 31.000000 s";
    QTest::newRow("session-ended") << QByteArray("0 TX aa\n1000 RX 01\n") << QStringList{"write aa", "end gave up"} << 4
                                   << "RX 1 bytes 01 at 0.001000 s"
                                   << "ended its session (gave up) at 0.000000 s";
}

void GPSRecordingDialogueTest::_departureNamesTheRecord()
{
    QFETCH(QByteArray, records);
    QFETCH(QStringList, calls);
    QFETCH(int, line);
    QFETCH(QString, recorded);
    QFETCH(QString, driver);
    TestFixtures::TempDirFixture directory;
    const QString file =
        directory.createFile(QStringLiteral("session.recording"), QByteArray(HEADER_LINE) + SETTINGS_LINE + records);
    GPSTestClock clock(GPSTestClock::START_US);
    GPSDialogueTransport link(file, clock);
    QCOMPARE(link.open().status, GPSOpenStatus::Opened);
    for (const QString& call : calls) {
        // Only the last call departs from the recording.
        QVERIFY2(!link.divergence(), qPrintable(call));
        const QString argument = call.section(u' ', 1);
        if (call.startsWith(QStringLiteral("write"))) {
            (void) writeHex(link, argument.toLatin1().constData());
        } else if (call.startsWith(QStringLiteral("baud"))) {
            (void) link.setBaudRate(argument.toUInt());
        } else if (call.startsWith(QStringLiteral("read"))) {
            (void) Read(link, std::chrono::milliseconds(argument.toInt()));
        } else {
            link.sessionEnded(argument);
        }
    }
    QVERIFY(link.divergence());
    QCOMPARE(link.divergence()->line, line);
    QCOMPARE(link.divergence()->recorded, recorded);
    QCOMPARE(link.divergence()->driver, driver);
    QCOMPARE(link.divergence()->text(),
             QStringLiteral("line %1: the recording has %2; the driver %3").arg(line).arg(recorded, driver));
    // The departure ends the driver's session, and nothing it does afterwards replaces it.
    QVERIFY(link.isCancelled());
    QVERIFY(!link.finished());
    QCOMPARE(Read(link, 10ms).status, GPSReadStatus::Cancelled);
    QCOMPARE(writeHex(link, "ff"), GPSWriteStatus::Cancelled);
    QVERIFY(!link.setBaudRate(4800));
    QCOMPARE(link.divergence()->line, line);
}

void GPSRecordingDialogueTest::_unusableRecordingIsAnError_data()
{
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<QString>("error");
    const QByteArray header(HEADER_LINE);
    QTest::newRow("raw-receiver-output") << QByteArray("$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03\r\n")
                                         << "not a receiver recording";
    // A recording does not say what its session was asked to do unless its recorder was told.
    QTest::newRow("no-settings") << header + "0 BAUD 115200\n"
                                 << "the recording does not hold the settings of its session";
    QTest::newRow("settings-of-no-session") << header + "0 SETTINGS type=ublox\n0 BAUD 115200\n"
                                            << "line 2: not the settings of a session";
    QTest::newRow("damaged-first-record") << header + SETTINGS_LINE + "0 BAUD fast\n" << "line 3: not a baud rate";
    // Found once the driver has reproduced the record before it.
    QTest::newRow("damaged-later-record") << header + PASSIVE_SESSION + "10 RX b56\n"
                                          << "line 4: not whole bytes in lower-case hexadecimal";
}

void GPSRecordingDialogueTest::_unusableRecordingIsAnError()
{
    QFETCH(QByteArray, text);
    QFETCH(QString, error);
    TestFixtures::TempDirFixture directory;
    const GPSDialogueResult replayed = replayDialogue(directory.createFile(QStringLiteral("session.recording"), text));
    QVERIFY(!replayed.passed());
    QVERIFY(!replayed.divergence);
    QCOMPARE(replayed.error, error);
    QCOMPARE(replayed.failure(), error);
}

void GPSRecordingDialogueTest::_interruptionEndsTheReplay()
{
    TestFixtures::TempDirFixture directory;
    // The receiver's bytes come an hour after the session is ready.
    const QString file = directory.createFile(QStringLiteral("session.recording"),
                                              QByteArray(HEADER_LINE) + PASSIVE_SESSION + "3600000000 RX 0d0a\n");
    int asked = 0;
    const GPSDialogueResult replayed = replayDialogue(file, [&asked] { return ++asked == 3; });
    QCOMPARE(asked, 3);
    QVERIFY(!replayed.passed());
    QCOMPARE(replayed.error, QStringLiteral("interrupted"));
    QVERIFY(replayed.configured);
    QCOMPARE(replayed.records, 1);
    // Two receives of the worker's slice went by on the replay's clock, not on the test's.
    QCOMPARE(replayed.duration, 2 * GPSReceiverWorker::RECEIVE_TIMEOUT);

    QVERIFY(!replayDialogue(QDir(directory.path()).filePath(QStringLiteral("missing.recording"))).error.isEmpty());
}

void GPSRecordingDialogueTest::_recordedDialoguesReplay()
{
    const QDir directory(QString::fromUtf8(FIXTURE_DIR) + QStringLiteral("/dialogues"));
    const QStringList recordings = directory.entryList({QStringLiteral("*.recording")}, QDir::Files, QDir::Name);
    if (recordings.isEmpty()) {
        QSKIP("No recording of a receiver's dialogue has been added to fixtures/dialogues yet");
    }
    for (const QString& recording : recordings) {
        const GPSDialogueResult replayed = replayDialogue(directory.filePath(recording));
        QVERIFY2(replayed.passed(), qPrintable(recording + QStringLiteral(": ") + replayed.failure()));
        // A recording without a write checks nothing the driver sends.
        QVERIFY2(replayed.sentBytes > 0, qPrintable(recording));
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSRecordingDialogueTest, TestLabel::Unit)
