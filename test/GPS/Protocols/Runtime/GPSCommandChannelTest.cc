#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStringList>

#include "../Support/GPSProtocolLogCapture.h"
#include "../Support/GPSRuntimeTestIO.h"
#include "../Support/ProtocolTestPackets.h"
#include "GPSNMEAStream.h"
#include "GPSProtocolRuntime.h"
#include "GPSRawAckMatcher.h"
#include "GPSRuntimeTestSupport.h"
#include "UBX/UBXFrameDecoder.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {

using GPSRuntimeTest::LatencyModel;
using GPSRuntimeTest::summary;

constexpr uint64_t START_US = 1000000;

GPSCommandOutcome lineReply(std::string_view reply)
{
    return reply == "$ACK"   ? GPSCommandOutcome::Acknowledged
           : reply == "$NAK" ? GPSCommandOutcome::Rejected
                             : GPSCommandOutcome::Pending;
}

QString note(const GPSBaudDetection& detection)
{
    return QStringLiteral("baud found=%1 rate=%2 linkFailed=%3")
        .arg(int(detection.found))
        .arg(detection.baud)
        .arg(int(detection.linkFailed));
}

QString note(const GPSCommandSequence::Result& result)
{
    return QStringLiteral("sequence step=%1 label=%2 outcome=%3")
        .arg(result.failedStep ? QString::number(*result.failedStep) : QStringLiteral("-"))
        .arg(QString::fromStdString(result.failedLabel))
        .arg(static_cast<int>(result.outcome));
}

/// A line decoder that offers every line to the outstanding command and resolves "$DONE" itself. "$WARN" schedules a
/// DIAG command for the streaming services.
class ChannelHarness final : public GPSFamilyProtocol
{
public:
    using Script = std::function<GPSTask<bool>(GPSCommandChannel&, ChannelHarness&, unsigned&, QStringList&)>;

    ChannelHarness(Script script, QStringList& notes)
        : _script(std::move(script))
        , _notes(notes)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig, unsigned& baud) override
    {
        co_return co_await _script(channel, *this, baud, _notes);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            return _nmea.decodeRTCM(frame, context);
        }
        const auto line = frame.text();
        auto updates = _nmea.decodeStandard(line, context);
        if (line == "$DONE") {
            context.resolveReply(GPSCommandOutcome::Acknowledged);
            updates |= GPSReceiveUpdate::Activity;
        } else {
            ready |= line == "$READY";
            diagnosticsPending |= line == "$WARN";
            context.offerReply(line);
        }
        return _nmea.finishLine(updates, context);
    }

    void flush(GPSDecodeContext& context) override { _nmea.flush(context); }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        const GPSReceiveUpdates updates =
            co_await channel.receiveCycle(_nmea.limitReceiveTimeout(timeout, channel.nowUs()));
        co_await channel.serviceControls();
        co_return updates;
    }

    GPSTask<void> serviceStreaming(GPSCommandChannel& channel) override
    {
        if (diagnosticsPending) {
            diagnosticsPending = false;
            _notes += summary(co_await channel.transact({"DIAG", 100ms}, "DIAG\r\n", GPSTextMatcher(lineReply)));
        }
    }

    bool ready = false;
    bool diagnosticsPending = false;

private:
    Script _script;
    QStringList& _notes;
    GPSNMEAStream _nmea{GPSNMEAStream::Navigation::StandardNMEA, false};
};

constexpr GPSReceiverFamily HARNESS_FAMILY{
    .type = GPSType::passive, .name = QLatin1StringView("harness"), .stream = GPSNMEAStream::STREAM};

constexpr uint16_t ACK_NAK = 0x0005;
constexpr uint16_t ACK_ACK = 0x0105;
constexpr std::chrono::milliseconds UBX_WRITE_CAP{1000};

QByteArray ubx(uint16_t message, std::initializer_list<uint8_t> payload)
{
    const auto frame = ubxFrame(message, payload);
    return QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size()));
}

QByteArray acknowledgement(uint16_t message, bool accepted)
{
    return ubx(accepted ? ACK_ACK : ACK_NAK, {uint8_t(message), uint8_t(message >> 8)});
}

/// Acknowledgement state as UBX::ReceiverController keeps it: a NAK overrides an ACK decoded in the same chunk.
struct AcknowledgementState
{
    void accept(uint16_t ack, std::span<const uint8_t> payload)
    {
        if ((ack != ACK_ACK && ack != ACK_NAK) || payload.size() != 2) {
            return;
        }
        const uint16_t message = payload[0] | (payload[1] << 8);
        if (awaited == message && outcome != GPSCommandOutcome::Rejected) {
            outcome = ack == ACK_ACK ? GPSCommandOutcome::Acknowledged : GPSCommandOutcome::Rejected;
        }
    }

    std::optional<uint16_t> awaited;
    GPSCommandOutcome outcome = GPSCommandOutcome::Pending;
};

class ChannelBinaryHarness final : public GPSFamilyProtocol
{
public:
    using Script = std::function<GPSTask<bool>(GPSCommandChannel&, ChannelBinaryHarness&, QStringList&)>;

    ChannelBinaryHarness(Script script, QStringList& notes)
        : _script(std::move(script))
        , _notes(notes)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig, unsigned&) override
    {
        co_return co_await _script(channel, *this, _notes);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext&) override
    {
        _acknowledgement.accept(static_cast<uint16_t>(frame.messageId), frame.payload);
        return GPSReceiveUpdate::Activity;
    }

    GPSTask<bool> sendMessage(GPSCommandChannel& channel, uint16_t message, std::span<const uint8_t> payload,
                              GPSConfigurationStep step)
    {
        channel.beginCommand(std::move(step));
        auto scope = channel.deadlineScope(UBX_WRITE_CAP);
        scope.limitUntil(GPSDeadline::after(channel.currentCommand().evidence.startedAtUs, UBX_WRITE_CAP).untilUs);
        const auto frame = ubxFrame(message, payload);
        const std::span<const uint8_t> bytes(frame);
        if (!co_await channel.write(bytes.first(6))) {
            co_return false;
        }
        if (!payload.empty()) {
            if (!co_await channel.write(bytes.subspan(6, payload.size()))) {
                co_return false;
            }
        }
        co_return co_await channel.write(bytes.last(2));
    }

    GPSTask<GPSCommandResult> waitForAck(GPSCommandChannel& channel, uint16_t message)
    {
        auto scope = channel.deadlineScope(channel.remainingUntil(channel.commandDeadline().untilUs));
        scope.limitUntil(channel.commandDeadline().untilUs);
        _acknowledgement = {.awaited = message};
        const auto result = co_await channel.awaitReply([this] { return _acknowledgement.outcome; });
        _acknowledgement.awaited.reset();
        co_return result;
    }

private:
    Script _script;
    QStringList& _notes;
    AcknowledgementState _acknowledgement;
};

constexpr GPSReceiverFamily BINARY_FAMILY{.type = GPSType::ublox,
                                          .name = QLatin1StringView("binary"),
                                          .stream = {.framers = GPSFrameKind::UBX, .enabled = GPSFrameKind::UBX}};

struct Scenario
{
    std::function<void(LatencyModel&)> setup;
    ChannelHarness::Script channel;
    unsigned baud = 0;
    int receives = 0;
    std::chrono::milliseconds receiveTimeout{100};
    bool cancelWaits = false;
};

/// Everything observable about one run: its results, the wire, the evidence and the decoded batches.
struct Outcome
{
    bool configured = false;
    unsigned baud = 0;
    QStringList notes;
    QStringList writes;
    QStringList evidence;
    QStringList configurationEvidence;
    QStringList batches;
    QList<unsigned> bauds;
    uint64_t clockUs = 0;
    int error = 0;
    QString detail;
    int reads = 0;
};

void finish(Outcome& outcome, const ScriptedReceiver& receiver, const LatencyModel& model, const GPSTestClock& clock)
{
    for (const auto& command : receiver.commands()) {
        outcome.writes += QString::fromLatin1(command.toHex());
    }
    outcome.bauds = model.bauds;
    outcome.clockUs = clock.nowUs();
    outcome.reads = model.reads;
}

GPSRuntimeIO scriptedIO(ScriptedReceiver& receiver, GPSTestClock& clock, bool cancelWaits)
{
    auto io = receiver.makeIO(makeGPSRuntimeTestIO(clock));
    if (cancelWaits) {
        io.wait = [&clock](std::chrono::microseconds duration) {
            clock.advanceBy(static_cast<uint64_t>(duration.count()));
            return false;
        };
    }
    return io;
}

Outcome runChannel(const Scenario& scenario)
{
    GPSTestClock clock(START_US);
    LatencyModel model(clock);
    if (scenario.setup) {
        scenario.setup(model);
    }
    ScriptedReceiver receiver(std::stop_token{}, &model);
    Outcome outcome;
    GPSRuntimeObserver observer;
    observer.commandFinished = [&outcome](const GPSCommandResult& result) { outcome.evidence += summary(result); };
    observer.decoded = [&outcome](const GPSEventBatch& batch) { outcome.batches += summary(batch); };
    GPSProtocolRuntime runtime(HARNESS_FAMILY, std::make_unique<ChannelHarness>(scenario.channel, outcome.notes),
                               scriptedIO(receiver, clock, scenario.cancelWaits), std::move(observer));
    unsigned baud = scenario.baud;
    outcome.configured = runtime.configure({}, baud);
    for (const auto& evidence : runtime.configurationEvidence()) {
        outcome.configurationEvidence += summary(evidence);
    }
    for (int index = 0; index < scenario.receives && runtime.error() == GPSProtocolError::None; ++index) {
        outcome.notes += QStringLiteral("receive %1").arg(runtime.receive(scenario.receiveTimeout).toInt());
    }
    outcome.baud = baud;
    outcome.error = static_cast<int>(runtime.error());
    outcome.detail = runtime.errorDetail();
    finish(outcome, receiver, model, clock);
    return outcome;
}

QString escaped(const QByteArray& bytes)
{
    QString text;
    for (const char value : bytes) {
        const auto byte = static_cast<uint8_t>(value);
        if (byte == '\r') {
            text += QStringLiteral("\\r");
        } else if (byte == '\n') {
            text += QStringLiteral("\\n");
        } else if (byte == '\\') {
            text += QStringLiteral("\\\\");
        } else if (byte >= 0x20 && byte < 0x7f) {
            text += QChar(byte);
        } else {
            text += QStringLiteral("\\x%1").arg(byte, 2, 16, QLatin1Char('0'));
        }
    }
    return text;
}

QStringList render(const Outcome& outcome, const QStringList& warnings)
{
    QStringList lines{QStringLiteral("result configured=%1 baud=%2 clock=%3 error=%4 reads=%5 detail=\"%6\"")
                          .arg(int(outcome.configured))
                          .arg(outcome.baud)
                          .arg(outcome.clockUs)
                          .arg(outcome.error)
                          .arg(outcome.reads)
                          .arg(outcome.detail)};
    for (const auto rate : outcome.bauds) {
        lines += QStringLiteral("baud %1").arg(rate);
    }
    for (const auto& write : outcome.writes) {
        lines += QStringLiteral("write ") + escaped(QByteArray::fromHex(write.toLatin1()));
    }
    for (const auto& note : outcome.notes) {
        lines += QStringLiteral("note ") + note;
    }
    for (const auto& evidence : outcome.evidence) {
        lines += QStringLiteral("evidence ") + evidence;
    }
    for (const auto& evidence : outcome.configurationEvidence) {
        lines += QStringLiteral("configuration ") + evidence;
    }
    for (const auto& batch : outcome.batches) {
        lines += QStringLiteral("decoded ") + batch;
    }
    for (const auto& warning : warnings) {
        lines += QStringLiteral("warning ") + warning;
    }
    return lines;
}

QByteArray line(const char* text)
{
    return QByteArray(text) + "\r\n";
}

QList<Scenario> scenarios()
{
    QList<Scenario> result;

    // 0: acknowledgement, rejection, silence, a decoder-resolved reply and a reply behind unrelated traffic.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.script("B\r\n", {{line("$NAK")}});
                 model.script("D\r\n", {{line("$DONE"), 20000}});
                 model.script("E\r\n", {{line("$NOISE") + line("$ACK"), 30000}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"B", 100ms, {GPSReceiverSetting::OutputRateHz}}, "B\r\n",
                                                  GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"C", 100ms, {}, false}, "C\r\n", GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"D", 100ms}, "D\r\n"));
             notes += summary(co_await c.transact({"E", 100ms}, "E\r\n", GPSTextMatcher(lineReply)));
             co_return true;
         }});

    // 1: raw acknowledgements split across reads, a rejection after partial acceptance, and raw sequence steps.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.readChunk = 2;
                 model.script("R1\r\n", {{"xxOK!"}});
                 model.script("R2\r\n", {{"OKERR"}});
                 model.script("R3\r\n", {{"nothing"}, {"...OK!", 5000}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             GPSRawAckMatcher first("OK!", "ERR");
             notes += summary(co_await c.transact({"R1", 100ms}, "R1\r\n", first));
             GPSRawAckMatcher second("OK!", "ERR");
             notes += summary(co_await c.transact({"R2", 100ms}, "R2\r\n", second));
             GPSCommandSequence sequence;
             sequence.steps.emplace_back(
                 GPSCommandSequence::Command{.step = {"R3", 50ms},
                                             .wire = "R3\r\n",
                                             .reply = GPSCommandSequence::RawReply{"OK!", "ERR"},
                                             .attempts = 2});
             notes += note(co_await c.runSequence(sequence));
             co_return true;
         }});

    // 2: nested deadlines bound commands, waits and write-only commands; a pending command retires as Written.
    result.append(
        {.setup = [](LatencyModel& model) { model.script("A\r\n", {{line("$ACK"), 5000}, {line("$ACK"), 5000}}); },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             {
                 const auto outer = c.deadlineScope(300ms);
                 notes += QString::number(co_await c.writeCommand({"W0", 50ms}, "W0"));
                 notes += summary(co_await c.transact({"SLOW", 100ms}, "SLOW\r\n", GPSTextMatcher(lineReply)));
                 {
                     const auto inner = c.deadlineScope(20ms);
                     co_await c.wait(200ms);
                     notes += QString::number(c.nowUs());
                 }
                 notes += QString::number(co_await c.writeCommand({"W1", 1000ms}, "W1"));
                 notes += summary(co_await c.awaitReply([] { return GPSCommandOutcome::Pending; }));
             }
             {
                 auto limited = c.deadlineScope(500ms);
                 limited.limitUntil(c.nowUs() + 3000);
                 notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             }
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += QString::number(co_await c.writeCommand({"W2", 50ms}, "W2"));
             co_return true;
         }});

    // 3: a failed write is sticky: every later operation fails at once without I/O.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.failingWrites = {"B\r\n"};
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"B", 100ms}, "B\r\n", GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"C", 100ms}, "C\r\n", GPSTextMatcher(lineReply)));
             notes += QString::number(co_await c.setBaudrate(9600));
             co_await c.wait(1s);
             std::array<uint8_t, 1> byte{};
             notes += QString::number(co_await c.read(byte, 100ms));
             notes += QString::number(co_await c.writeCommand({"W", 100ms}, byte));
             notes += note(co_await c.runSequence({{GPSCommandSequence::Command{
                 .step = {"S", 100ms}, .wire = "S\r\n", .reply = GPSTextMatcher(lineReply)}}}));
             constexpr unsigned RATES[] = {9600, 115200};
             notes += note(co_await c.detectBaud(
                 RATES, 0, [](unsigned) -> GPSTask<GPSBaudProbe> { co_return GPSBaudProbe::Found; }));
             notes += summary(co_await c.awaitReply([] { return GPSCommandOutcome::Acknowledged; }));
             co_return false;
         }});

    // 4: attempts, optional failures and custom steps in a sequence; the required failure names its step.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.script("S1\r\n", {{line("$NAK")}, {}, {line("$ACK")}});
                 model.script("S2\r\n", {{line("$NAK")}});
                 model.script("S4\r\n", {{line("$NAK")}});
                 model.emitAt(START_US + 220000, line("$READY"));
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness& h, unsigned&, QStringList& notes) -> GPSTask<bool> {
             GPSCommandSequence sequence{{
                 GPSCommandSequence::Command{
                     .step = {"S1", 60ms}, .wire = "S1\r\n", .reply = GPSTextMatcher(lineReply), .attempts = 3},
                 GPSCommandSequence::Command{
                     .step = {"S2", 60ms, {}, false}, .wire = "S2\r\n", .reply = GPSTextMatcher(lineReply)},
                 GPSCommandSequence::Custom{"ready",
                                            [&c, &h]() -> GPSTask<bool> {
                                                co_await c.wait(30ms);
                                                co_return co_await c.receiveUntil([&h] { return h.ready; }, 200ms);
                                            }},
                 GPSCommandSequence::Custom{"optional", []() -> GPSTask<bool> { co_return false; }, false},
                 GPSCommandSequence::Command{
                     .step = {"S4", 60ms}, .wire = "S4\r\n", .reply = GPSTextMatcher(lineReply)},
                 GPSCommandSequence::Command{
                     .step = {"S5", 60ms}, .wire = "S5\r\n", .reply = GPSTextMatcher(lineReply)},
             }};
             notes += note(co_await c.runSequence(sequence));
             co_return true;
         }});

    // 5: baud detection through a probing command, a fixed rate, and a rate the link rejects.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.answeringRates = {115200};
                 model.unsupportedRates = {57600};
                 model.script(
                     "PING\r\n",
                     {{line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned& baud, QStringList& notes) -> GPSTask<bool> {
             const auto probe = [&c](unsigned) -> GPSTask<GPSBaudProbe> {
                 const auto reply = co_await c.transact({"PING", 50ms}, "PING\r\n", GPSTextMatcher(lineReply));
                 co_return reply.succeeded() ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
             };
             constexpr unsigned RATES[] = {9600, 38400, 115200, 230400};
             const auto found = co_await c.detectBaud(RATES, 0, probe);
             notes += note(found);
             notes += note(co_await c.detectBaud(RATES, 38400, probe));
             constexpr unsigned LINK[] = {57600, 115200};
             notes += note(co_await c.detectBaud(LINK, 0, probe));
             baud = found.baud;
             co_return found.found;
         }});

    // 6: streaming receives with NMEA, RTCM3 and a warning whose streaming service sends a command.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.script("DIAG\r\n", {{line("$ACK"), 30000}});
                 const auto gga = QByteArray::fromStdString(
                     nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
                 const auto rtcm = rtcmPacket(std::array<uint8_t, 6>{0x3e, 0xd0, 1, 2, 3, 4});
                 const QByteArray correction(reinterpret_cast<const char*>(rtcm.data()), qsizetype(rtcm.size()));
                 model.emitAt(START_US + 50000, gga + correction);
                 model.emitAt(START_US + 180000,
                              QByteArray::fromStdString(nmeaSentence("GPGSV,2,1,05,01,40,083,46,02,17,308,41")));
                 model.emitAt(START_US + 260000, line("$WARN") + gga);
                 model.emitAt(START_US + 900000, correction + correction);
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             co_return true;
         },
         .receives = 12});

    // 7: a cancelled wait stops the session.
    result.append(
        {.setup = [](LatencyModel& model) { model.script("A\r\n", {{line("$ACK")}}); },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             co_await c.wait(10ms);
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             co_return false;
         },
         .cancelWaits = true});

    // 8: a transport read failure ends the pending command and the session.
    result.append(
        {.setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.failingReadsAfter = {"B\r\n"};
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> GPSTask<bool> {
             notes += summary(co_await c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += summary(co_await c.transact({"B", 100ms}, "B\r\n", GPSTextMatcher(lineReply)));
             co_return false;
         }});
    return result;
}

/// Compares @a lines with the checked-in transcript @a name, or rewrites it when QGC_GPS_GOLDEN_UPDATE=1.
void compareTranscript(const QString& name, const QStringList& lines)
{
    const QString path = QDir(QStringLiteral(GPS_CHANNEL_TRANSCRIPT_DIR)).filePath(name + QStringLiteral(".txt"));
    QFile file(path);
    if (qEnvironmentVariable("QGC_GPS_GOLDEN_UPDATE") == QStringLiteral("1")) {
        QStringList header;
        if (file.open(QIODevice::ReadOnly)) {
            for (const auto& existing : QString::fromUtf8(file.readAll()).split(u'\n')) {
                if (existing.startsWith(u'#')) {
                    header += existing;
                }
            }
            file.close();
        }
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate), qPrintable(path));
        file.write(((header + lines).join(u'\n') + u'\n').toUtf8());
        return;
    }
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
    QStringList expected;
    for (const auto& text : QString::fromUtf8(file.readAll()).split(u'\n', Qt::SkipEmptyParts)) {
        if (!text.startsWith(u'#')) {
            expected += text;
        }
    }
    QCOMPARE(lines, expected);
}

}  // namespace

class GPSCommandChannelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _transcripts_data();
    void _transcripts();
    void _binaryCommands();
    void _failedChannelSkipsIO();
    void _writeResults_data();
    void _writeResults();
    void _invalidReadResults_data();
    void _invalidReadResults();
};

void GPSCommandChannelTest::_transcripts_data()
{
    QTest::addColumn<int>("scenario");
    const QStringList names{QStringLiteral("replies"),   QStringLiteral("raw"),       QStringLiteral("deadlines"),
                            QStringLiteral("sticky"),    QStringLiteral("sequence"),  QStringLiteral("baud"),
                            QStringLiteral("streaming"), QStringLiteral("cancelled"), QStringLiteral("readFailure")};
    QCOMPARE(names.size(), scenarios().size());
    for (int index = 0; index < names.size(); ++index) {
        QTest::newRow(qPrintable(names[index])) << index;
    }
}

void GPSCommandChannelTest::_transcripts()
{
    QFETCH(int, scenario);
    const GPSProtocolLogCapture log;
    const Outcome outcome = runChannel(scenarios().at(scenario));
    compareTranscript(QString::fromLatin1(QTest::currentDataTag()), render(outcome, log.warnings()));
}

void GPSCommandChannelTest::_binaryCommands()
{
    GPSTestClock clock(START_US);
    LatencyModel model(clock);
    model.ubxCommands = true;
    model.script(ubx(0x8a06, {1, 2, 3}), {{acknowledgement(0x8a06, true), 4000}});
    model.script(ubx(0x8a06, {4}), {{acknowledgement(0x8a06, false), 4000}});
    // An ACK and a late NAK decoded together resolve as a rejection, as UBX::ReceiverController does.
    model.script(ubx(0x0806, {}), {{acknowledgement(0x0806, true) + acknowledgement(0x0806, false), 4000}});
    model.script(ubx(0x0406, {9}), {{acknowledgement(0x0406, true), 80000}});
    ScriptedReceiver receiver(std::stop_token{}, &model);
    Outcome outcome;
    GPSRuntimeObserver observer;
    observer.commandFinished = [&outcome](const GPSCommandResult& result) { outcome.evidence += summary(result); };
    GPSProtocolRuntime runtime(
        BINARY_FAMILY,
        std::make_unique<ChannelBinaryHarness>(
            [](GPSCommandChannel& c, ChannelBinaryHarness& h, QStringList& notes) -> GPSTask<bool> {
                const std::array<uint8_t, 3> first{1, 2, 3};
                const std::array<uint8_t, 1> second{4};
                const std::array<uint8_t, 1> late{9};
                notes += QString::number(co_await h.sendMessage(c, 0x8a06, first, {"VALSET 1", 200ms}));
                notes += summary(co_await h.waitForAck(c, 0x8a06));
                notes += QString::number(co_await h.sendMessage(c, 0x8a06, second, {"VALSET 2", 200ms}));
                notes += summary(co_await h.waitForAck(c, 0x8a06));
                notes += QString::number(
                    co_await h.sendMessage(c, 0x0806, {}, {"RATE", 200ms, {GPSReceiverSetting::OutputRateHz}}));
                notes += summary(co_await h.waitForAck(c, 0x0806));
                notes += QString::number(co_await h.sendMessage(c, 0x0406, late, {"LATE", 50ms}));
                notes += summary(co_await h.waitForAck(c, 0x0406));
                co_return true;
            },
            outcome.notes),
        scriptedIO(receiver, clock, false), std::move(observer));
    unsigned baud = 0;
    outcome.configured = runtime.configure({}, baud);
    finish(outcome, receiver, model, clock);
    QCOMPARE(outcome.writes.size(), 4);
    compareTranscript(QStringLiteral("binary"), render(outcome, {}));
}

void GPSCommandChannelTest::_failedChannelSkipsIO()
{
    const Outcome outcome = runChannel(scenarios().at(3));
    QCOMPARE(outcome.writes, (QStringList{QString::fromLatin1(QByteArray("A\r\n").toHex()),
                                          QString::fromLatin1(QByteArray("B\r\n").toHex())}));
    QVERIFY(outcome.bauds.isEmpty());
    QCOMPARE(outcome.error, static_cast<int>(GPSProtocolError::Transport));
    QCOMPARE(outcome.detail, QStringLiteral("scripted write failure"));
    // One read answered the acknowledged command; nothing was read after the failed write.
    QCOMPARE(outcome.reads, 1);
}

void GPSCommandChannelTest::_writeResults_data()
{
    QTest::addColumn<GPSWriteResult>("result");
    QTest::addColumn<GPSCommandOutcome>("outcome");
    QTest::addColumn<GPSProtocolError>("error");
    const QString detail = QStringLiteral("Invalid receiver progress: Gerät");
    const auto transport = GPSCommandOutcome::TransportError;
    QTest::newRow("timed out") << GPSWriteResult{GPSWriteStatus::TimedOut, 6, 6, detail} << transport
                               << GPSProtocolError::Transport;
    QTest::newRow("cancelled") << GPSWriteResult{GPSWriteStatus::Cancelled, 6, 1, detail}
                               << GPSCommandOutcome::Cancelled << GPSProtocolError::Cancelled;
    QTest::newRow("error") << GPSWriteResult{GPSWriteStatus::Error, 6, 1, detail} << transport
                           << GPSProtocolError::Transport;
    // Unsupported without progress rejects the attempt without poisoning the connection.
    QTest::newRow("unsupported") << GPSWriteResult{GPSWriteStatus::Unsupported} << transport << GPSProtocolError::None;
    QTest::newRow("unsupported progress")
        << GPSWriteResult{GPSWriteStatus::Unsupported, 6, 6, detail} << transport << GPSProtocolError::Transport;
    QTest::newRow("short write") << GPSWriteResult{GPSWriteStatus::Completed, 6, 2, detail} << transport
                                 << GPSProtocolError::Transport;
    QTest::newRow("nothing written") << GPSWriteResult{GPSWriteStatus::Completed, 0, 0, detail} << transport
                                     << GPSProtocolError::Transport;
    QTest::newRow("negative accepted") << GPSWriteResult{GPSWriteStatus::Completed, -1, 6, detail} << transport
                                       << GPSProtocolError::Transport;
    QTest::newRow("negative written") << GPSWriteResult{GPSWriteStatus::Completed, 6, -1, detail} << transport
                                      << GPSProtocolError::Transport;
    QTest::newRow("overwritten") << GPSWriteResult{GPSWriteStatus::Completed, 6, 7, detail} << transport
                                 << GPSProtocolError::Transport;
    QTest::newRow("overaccepted") << GPSWriteResult{GPSWriteStatus::Completed, 7, 6, detail} << transport
                                  << GPSProtocolError::Transport;
}

void GPSCommandChannelTest::_writeResults()
{
    QFETCH(GPSWriteResult, result);
    QFETCH(GPSCommandOutcome, outcome);
    QFETCH(GPSProtocolError, error);
    const GPSProtocolLogCapture log;
    GPSTestClock clock(START_US);
    int writes = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.write = [&](std::span<const uint8_t>, GPSDeadline) {
        ++writes;
        return result;
    };
    io.read = [&clock](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
        clock.advanceTo(deadline.untilUs);
        return {GPSReadStatus::TimedOut};
    };
    QList<GPSCommandResult> finished;
    GPSRuntimeObserver observer;
    observer.commandFinished = [&finished](const GPSCommandResult& command) { finished += command; };
    QStringList notes;
    GPSProtocolRuntime runtime(
        HARNESS_FAMILY,
        std::make_unique<ChannelHarness>(
            [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList&) -> GPSTask<bool> {
                (void) co_await c.transact({"probe", 100ms}, "PROBE\n", GPSTextMatcher(lineReply));
                (void) co_await c.transact({"again", 100ms}, "PROBE\n", GPSTextMatcher(lineReply));
                co_return true;
            },
            notes),
        std::move(io), std::move(observer));
    unsigned baud = 0;
    (void) runtime.configure({}, baud);
    QVERIFY(!finished.isEmpty());
    const auto& evidence = finished.front().evidence;
    QCOMPARE(evidence.outcome, outcome);
    QCOMPARE(evidence.acceptedBytes, result.acceptedBytes);
    QCOMPARE(evidence.writtenBytes, result.writtenBytes);
    QCOMPARE(evidence.uncertainBytes, result.uncertainBytes());
    QCOMPARE(runtime.error(), error);
    // A sticky failure skips the second command's write; a rejected attempt does not.
    QCOMPARE(writes, error == GPSProtocolError::None ? 2 : 1);
    if (error != GPSProtocolError::None && error != GPSProtocolError::Cancelled) {
        QCOMPARE(runtime.errorDetail(), result.detail);
    }
}

void GPSCommandChannelTest::_invalidReadResults_data()
{
    QTest::addColumn<GPSReadResult>("result");
    const QString detail = QStringLiteral("Invalid receiver progress: Gerät");
    QTest::newRow("negative") << GPSReadResult{GPSReadStatus::Data, -1, detail};
    QTest::newRow("overlong") << GPSReadResult{GPSReadStatus::Data,
                                               static_cast<int>(GPSCommandChannel::READ_CHUNK_SIZE) + 1, detail};
    QTest::newRow("timed out with data") << GPSReadResult{GPSReadStatus::TimedOut, 1, detail};
}

void GPSCommandChannelTest::_invalidReadResults()
{
    QFETCH(GPSReadResult, result);
    const GPSProtocolLogCapture log;
    GPSTestClock clock(START_US);
    int reads = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [&](std::span<uint8_t>, GPSDeadline) {
        ++reads;
        return result;
    };
    QStringList notes;
    GPSProtocolRuntime runtime(HARNESS_FAMILY,
                               std::make_unique<ChannelHarness>(
                                   [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList&) -> GPSTask<bool> {
                                       co_await c.receiveFor(50ms);
                                       co_await c.receiveFor(50ms);
                                       co_return true;
                                   },
                                   notes),
                               std::move(io));
    unsigned baud = 0;
    (void) runtime.configure({}, baud);
    QCOMPARE(runtime.error(), GPSProtocolError::Transport);
    QCOMPARE(runtime.errorDetail(), result.detail);
    QCOMPARE(reads, 1);
    QCOMPARE(log.warnings(), QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                             .arg(static_cast<int>(result.status))
                                             .arg(result.detail)});
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCommandChannelTest, TestLabel::Unit)

#include "GPSCommandChannelTest.moc"
