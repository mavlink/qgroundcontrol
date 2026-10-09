#include "GPSCommandChannelTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QStringList>

#include "GPSCommand.h"
#include "GPSNMEAStream.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "Protocols/Support/GPSEventSummary.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

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
        .arg(QString::fromUtf8(result.failedLabel))
        .arg(static_cast<int>(result.outcome));
}

/// A line decoder that offers every line to the outstanding command and resolves "$DONE" itself. "$WARN" schedules a
/// DIAG command for the streaming services.
class ChannelHarness final : public GPSFamilyProtocol
{
public:
    using Script = std::function<bool(GPSCommandChannel&, ChannelHarness&, unsigned&, QStringList&)>;

    ChannelHarness(Script script, QStringList& notes)
        : _script(std::move(script))
        , _notes(notes)
    {}

    bool configure(GPSCommandChannel& channel, GPSConfig, unsigned& baud) override
    {
        return _script(channel, *this, baud, _notes);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _nmea.onFrame(frame, context, [this, &context](std::string_view line) {
            if (line == "$DONE") {
                done = true;
                context.resolveReply(GPSCommandOutcome::Acknowledged);
                return GPSReceiveUpdates{GPSReceiveUpdate::Activity};
            }
            diagnosticsPending |= line == "$WARN";
            context.offerReply(line);
            return GPSReceiveUpdates{};
        });
    }

    void flush(GPSDecodeContext& context) override { _nmea.flush(context); }

    GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        const GPSReceiveUpdates updates = channel.receiveCycle(_nmea.limitReceiveTimeout(timeout, channel.nowUs()));
        channel.serviceControls();
        return updates;
    }

    void serviceStreaming(GPSCommandChannel& channel) override
    {
        if (diagnosticsPending) {
            diagnosticsPending = false;
            _notes += summary(channel.transact({"DIAG", 100ms}, "DIAG\r\n", GPSTextMatcher(lineReply)));
        }
    }

    bool diagnosticsPending = false;
    /// Whether "$DONE" arrived, for commands that await their reply with a poll.
    bool done = false;

private:
    Script _script;
    QStringList& _notes;
    GPSNMEAStream _nmea;
};

constexpr GPSReceiverFamily HARNESS_FAMILY{.type = GPSType::passive, .stream = GPSNMEAStream::STREAM};

/// A receiver on a virtual clock. Each command write schedules its scripted replies after their latency, unsolicited
/// output arrives at scheduled times, and time only passes while a read waits for data.
class LatencyModel : public ScriptedReceiver::Model
{
public:
    struct Reply
    {
        QByteArray bytes;
        uint64_t latencyUs = 1000;
    };

    explicit LatencyModel(GPSTestClock& clock)
        : _clock(clock)
    {}

    /// Replies to successive writes of @a command; an empty reply is silence.
    void script(const QByteArray& command, QList<Reply> replies) { _script[command] = std::move(replies); }

    /// Unsolicited output delivered at @a atUs.
    void emitAt(uint64_t atUs, const QByteArray& bytes) { _schedule(atUs, bytes); }

    /// Rates at which replies are delivered; empty accepts every rate.
    QList<unsigned> answeringRates;
    /// Rates the link cannot be set to.
    QList<unsigned> unsupportedRates;
    /// Commands whose write fails with a transport error.
    QList<QByteArray> failingWrites;
    /// Commands after which the next idle read fails.
    QList<QByteArray> failingReadsAfter;
    int readChunk = 0;
    unsigned baud = 0;
    QList<unsigned> bauds;
    int reads = 0;

private:
    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        Q_UNUSED(context)
        if (failingWrites.contains(command)) {
            return {GPSWriteStatus::Error, 0, 0, QStringLiteral("scripted write failure")};
        }
        if (failingReadsAfter.contains(command)) {
            receiver.failNextRead({GPSReadStatus::Error, 0, QStringLiteral("scripted read failure")});
        }
        auto found = _script.find(command);
        if (found != _script.end() && !found->second.isEmpty() &&
            (answeringRates.isEmpty() || answeringRates.contains(baud))) {
            const Reply reply = found->second.takeFirst();
            if (!reply.bytes.isEmpty()) {
                _schedule(_clock.nowUs() + reply.latencyUs, reply.bytes);
            }
        }
        return {GPSWriteStatus::Completed, static_cast<int>(command.size()), static_cast<int>(command.size())};
    }

    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override
    {
        Q_UNUSED(receiver)
        bauds.append(baudrate);
        if (unsupportedRates.contains(baudrate)) {
            return false;
        }
        baud = baudrate;
        return true;
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        ++reads;
        if (!_pending.empty() && _pending.begin()->first <= deadline.untilUs) {
            const auto next = _pending.begin();
            _clock.advanceTo(next->first);
            receiver.queueReply(next->second);
            _pending.erase(next);
            return;
        }
        _clock.advanceTo(deadline.untilUs);
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        Q_UNUSED(receiver)
        return readChunk > 0 ? std::min({requested, available, readChunk}) : std::min(requested, available);
    }

    void _schedule(uint64_t atUs, const QByteArray& bytes)
    {
        auto [slot, inserted] = _pending.try_emplace(atUs, bytes);
        if (!inserted) {
            slot->second += bytes;
        }
    }

    GPSTestClock& _clock;
    std::map<QByteArray, QList<Reply>> _script;
    std::map<uint64_t, QByteArray> _pending;
};

using enum GPSCommandOutcome;

/// Everything observable about one run: its results, the wire, the evidence and the decoded batches.
struct Run
{
    bool configured = false;
    unsigned baud = 0;
    uint64_t clockUs = 0;
    int reads = 0;
    GPSProtocolError error = GPSProtocolError::None;
    QString detail{};
    QByteArrayList writes{};
    QList<unsigned> bauds{};
    QStringList notes{};
    QStringList evidence{};
    /// The evidence of commands during configuration; empty in an expectation when it is all of the evidence.
    QStringList configurationEvidence{};
    QStringList batches{};
    QStringList warnings{};
};

struct Scenario
{
    const char* name = nullptr;
    std::function<void(LatencyModel&)> setup;
    ChannelHarness::Script channel;
    unsigned baud = 0;
    int receives = 0;
    std::chrono::milliseconds receiveTimeout{100};
    bool cancelWaits = false;
    Run expected{};
};

/// The summary() of a command's evidence, with @a bytes accepted and written.
QString command(const char* label, GPSCommandOutcome outcome, uint64_t startedUs, int bytes, bool required = true)
{
    return summary(GPSConfigurationEvidence{.command = label,
                                            .outcome = outcome,
                                            .startedAtUs = startedUs,
                                            .acceptedBytes = bytes,
                                            .writtenBytes = bytes,
                                            .required = required});
}

void finish(Run& run, const ScriptedReceiver& receiver, const LatencyModel& model, const GPSTestClock& clock)
{
    run.writes = receiver.commands();
    run.bauds = model.bauds;
    run.clockUs = clock.nowUs();
    run.reads = model.reads;
}

GPSRuntimeIO scriptedIO(ScriptedReceiver& receiver, GPSTestClock& clock, bool cancelWaits)
{
    auto io = receiver.makeIO(makeGPSRuntimeTestIO(clock));
    if (cancelWaits) {
        io.clock.wait = [&clock](std::chrono::microseconds duration) {
            clock.advanceBy(static_cast<uint64_t>(duration.count()));
            return false;
        };
    }
    return io;
}

Run runChannel(const Scenario& scenario)
{
    GPSTestClock clock(GPSTestClock::START_US);
    LatencyModel model(clock);
    if (scenario.setup) {
        scenario.setup(model);
    }
    ScriptedReceiver receiver(GPSCancelToken{}, &model);
    Run run;
    bool configuring = true;
    GPSRuntimeObserver observer;
    observer.commandFinished = [&run, &configuring](const GPSConfigurationEvidence& result) {
        run.evidence += summary(result);
        if (configuring) {
            run.configurationEvidence += summary(result);
        }
    };
    observer.decoded = [&run](const GPSEventBatch& batch) {
        if (batch.updates || !batch.events.empty()) {
            run.batches += summary(batch);
        }
    };
    GPSProtocolRuntime runtime(HARNESS_FAMILY, std::make_unique<ChannelHarness>(scenario.channel, run.notes),
                               scriptedIO(receiver, clock, scenario.cancelWaits), std::move(observer));
    unsigned baud = scenario.baud;
    run.configured = runtime.configure({}, baud);
    configuring = false;
    for (int index = 0; index < scenario.receives && runtime.error() == GPSProtocolError::None; ++index) {
        run.notes += QStringLiteral("receive %1").arg(runtime.receive(scenario.receiveTimeout).toInt());
    }
    run.baud = baud;
    run.error = runtime.error();
    run.detail = runtime.errorDetail();
    finish(run, receiver, model, clock);
    return run;
}

QByteArray line(const char* text)
{
    return QByteArray(text) + "\r\n";
}

/// The checks were recorded from the pre-runtime command base while it ran side by side with the channel and matched
/// it exactly; change one only for a justified change.
QList<Scenario> scenarios()
{
    QList<Scenario> result;

    const QStringList replies{command("A", Acknowledged, 1000000, 3), command("B", Rejected, 1001000, 3),
                              command("C", TimedOut, 1002000, 3, false), command("D", Acknowledged, 1102000, 3),
                              command("E", Acknowledged, 1122000, 3)};
    result.append({.name = "replies",
                   .setup =
                       [](LatencyModel& model) {
                           model.script("A\r\n", {{line("$ACK")}});
                           model.script("B\r\n", {{line("$NAK")}});
                           model.script("D\r\n", {{line("$DONE"), 20000}});
                           model.script("E\r\n", {{line("$NOISE") + line("$ACK"), 30000}});
                       },
                   .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
                       notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
                       notes += summary(c.transact({"B", 100ms}, "B\r\n", GPSTextMatcher(lineReply)));
                       notes += summary(c.transact({"C", 100ms, false}, "C\r\n", GPSTextMatcher(lineReply)));
                       notes += summary(c.transact({"D", 100ms}, "D\r\n"));
                       notes += summary(c.transact({"E", 100ms}, "E\r\n", GPSTextMatcher(lineReply)));
                       return true;
                   },
                   .expected = {.configured = true,
                                .clockUs = 1152000,
                                .reads = 5,
                                .writes = {"A\r\n", "B\r\n", "C\r\n", "D\r\n", "E\r\n"},
                                .notes = replies,
                                .evidence = replies,
                                .batches = {QStringLiteral("batch updates=4")}}});

    // Raw acknowledgements split across reads, a rejection after partial acceptance, and raw sequence steps.
    result.append(
        {.name = "raw",
         .setup =
             [](LatencyModel& model) {
                 model.readChunk = 2;
                 model.script("R1\r\n", {{"xxOK!"}});
                 model.script("R2\r\n", {{"OKERR"}});
                 model.script("R3\r\n", {{"nothing"}, {"...OK!", 5000}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             const GPSCommandSequence::RawReply reply{"OK!", "ERR"};
             notes += summary(c.transact({.step = {"R1", 100ms}, .wire = "R1\r\n", .reply = reply}));
             notes += summary(c.transact({.step = {"R2", 100ms}, .wire = "R2\r\n", .reply = reply}));
             GPSCommandSequence sequence;
             sequence.steps.emplace_back(
                 GPSCommandSequence::Command{.step = {"R3", 50ms},
                                             .wire = "R3\r\n",
                                             .reply = GPSCommandSequence::RawReply{"OK!", "ERR"},
                                             .attempts = 2});
             notes += note(c.runSequence(sequence));
             return true;
         },
         .expected = {.configured = true,
                      .clockUs = 1057000,
                      .reads = 5,
                      .writes = {"R1\r\n", "R2\r\n", "R3\r\n", "R3\r\n"},
                      .notes = {command("R1", Acknowledged, 1000000, 4), command("R2", Rejected, 1001000, 4),
                                QStringLiteral("sequence step=- label= outcome=0")},
                      .evidence = {command("R1", Acknowledged, 1000000, 4), command("R2", Rejected, 1001000, 4),
                                   command("R3", TimedOut, 1002000, 4), command("R3", Acknowledged, 1052000, 4)}}});

    // Nested deadlines bound commands, waits and write-only commands; a pending command retires as Written.
    result.append(
        {.name = "deadlines",
         .setup = [](LatencyModel& model) { model.script("A\r\n", {{line("$ACK"), 5000}, {line("$ACK"), 5000}}); },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             {
                 const auto outer = c.deadlineScope(300ms);
                 notes += QString::number(c.writeCommand({"W0", 50ms}, "W0"));
                 notes += summary(c.transact({"SLOW", 100ms}, "SLOW\r\n", GPSTextMatcher(lineReply)));
                 {
                     const auto inner = c.deadlineScope(20ms);
                     c.wait(200ms);
                     notes += QString::number(c.nowUs());
                 }
                 notes += QString::number(c.writeCommand({"W1", 1000ms}, "W1"));
                 notes += summary(c.awaitReply([] { return GPSCommandOutcome::Pending; }));
             }
             {
                 const auto limited = c.deadlineScope(GPSDeadline{c.nowUs() + 3000});
                 notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             }
             notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += QString::number(c.writeCommand({"W2", 50ms}, "W2"));
             return true;
         },
         .expected = {.configured = true,
                      .clockUs = 1305000,
                      .reads = 4,
                      .writes = {"W0", "SLOW\r\n", "W1", "A\r\n", "A\r\n", "W2"},
                      .notes = {QStringLiteral("1"), command("SLOW", TimedOut, 1000000, 6), QStringLiteral("1120000"),
                                QStringLiteral("1"), command("W1", TimedOut, 1120000, 2),
                                command("A", TimedOut, 1300000, 3), command("A", Acknowledged, 1303000, 3),
                                QStringLiteral("1")},
                      .evidence = {command("W0", Written, 1000000, 2), command("SLOW", TimedOut, 1000000, 6),
                                   command("W1", TimedOut, 1120000, 2), command("A", TimedOut, 1300000, 3),
                                   command("A", Acknowledged, 1303000, 3), command("W2", Written, 1305000, 2)}}});

    // A failed write is sticky: every later operation fails at once without I/O.
    result.append(
        {.name = "sticky",
         .setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.failingWrites = {"B\r\n"};
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += summary(c.transact({"B", 100ms}, "B\r\n", GPSTextMatcher(lineReply)));
             notes += summary(c.transact({"C", 100ms}, "C\r\n", GPSTextMatcher(lineReply)));
             notes += QString::number(c.setBaudrate(9600));
             c.wait(1s);
             std::array<uint8_t, 1> byte{};
             notes += QString::number(c.read(byte, 100ms));
             notes += QString::number(c.writeCommand({"W", 100ms}, byte));
             notes += note(c.runSequence({{GPSCommandSequence::Command{
                 .step = {"S", 100ms}, .wire = "S\r\n", .reply = GPSTextMatcher(lineReply)}}}));
             constexpr unsigned RATES[] = {9600, 115200};
             notes += note(c.detectBaud(RATES, 0, []() -> GPSBaudProbe { return GPSBaudProbe::Found; }));
             notes += summary(c.awaitReply([] { return GPSCommandOutcome::Acknowledged; }));
             return false;
         },
         .expected = {.clockUs = 1001000,
                      .reads = 1,
                      .error = GPSProtocolError::Transport,
                      .detail = QStringLiteral("scripted write failure"),
                      .writes = {"A\r\n", "B\r\n"},
                      .notes = {command("A", Acknowledged, 1000000, 3), command("B", TransportError, 1001000, 0),
                                command("C", TransportError, 0, 0), QStringLiteral("0"), QStringLiteral("-1"),
                                QStringLiteral("0"), QStringLiteral("sequence step=0 label=S outcome=7"),
                                QStringLiteral("baud found=0 rate=9600 linkFailed=1"),
                                command("W", TransportError, 0, 0)},
                      .evidence = {command("A", Acknowledged, 1000000, 3), command("B", TransportError, 1001000, 0)}}});

    // Attempts and optional failures in a sequence; the required failure names its step.
    result.append(
        {.name = "sequence",
         .setup =
             [](LatencyModel& model) {
                 model.script("S1\r\n", {{line("$NAK")}, {}, {line("$ACK")}});
                 model.script("S2\r\n", {{line("$NAK")}});
                 model.script("S4\r\n", {{line("$NAK")}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             GPSCommandSequence sequence{{
                 GPSCommandSequence::Command{
                     .step = {"S1", 60ms}, .wire = "S1\r\n", .reply = GPSTextMatcher(lineReply), .attempts = 3},
                 GPSCommandSequence::Command{
                     .step = {"S2", 60ms, false}, .wire = "S2\r\n", .reply = GPSTextMatcher(lineReply)},
                 GPSCommandSequence::Command{
                     .step = {"S4", 60ms}, .wire = "S4\r\n", .reply = GPSTextMatcher(lineReply)},
                 GPSCommandSequence::Command{
                     .step = {"S5", 60ms}, .wire = "S5\r\n", .reply = GPSTextMatcher(lineReply)},
             }};
             notes += note(c.runSequence(sequence));
             return true;
         },
         .expected = {.configured = true,
                      .clockUs = 1064000,
                      .reads = 5,
                      .writes = {"S1\r\n", "S1\r\n", "S1\r\n", "S2\r\n", "S4\r\n"},
                      .notes = {QStringLiteral("sequence step=2 label=S4 outcome=4")},
                      .evidence = {command("S1", Rejected, 1000000, 4), command("S1", TimedOut, 1001000, 4),
                                   command("S1", Acknowledged, 1061000, 4), command("S2", Rejected, 1062000, 4, false),
                                   command("S4", Rejected, 1063000, 4)}}});

    // Baud detection through a probing command, a fixed rate, and a rate the link rejects: skipped among
    // candidates, a link failure when it was the one requested.
    result.append(
        {.name = "baud",
         .setup =
             [](LatencyModel& model) {
                 model.answeringRates = {115200};
                 model.unsupportedRates = {57600};
                 model.script(
                     "PING\r\n",
                     {{line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}, {line("$ACK")}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned& baud, QStringList& notes) -> bool {
             const auto probe = [&c]() -> GPSBaudProbe {
                 const auto reply = c.transact({"PING", 50ms}, "PING\r\n", GPSTextMatcher(lineReply));
                 return reply.succeeded() ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
             };
             constexpr unsigned RATES[] = {9600, 38400, 115200, 230400};
             const auto found = c.detectBaud(RATES, 0, probe);
             notes += note(found);
             notes += note(c.detectBaud(RATES, 38400, probe));
             constexpr unsigned LINK[] = {57600, 115200};
             notes += note(c.detectBaud(LINK, 0, probe));
             notes += note(c.detectBaud(LINK, 57600, probe));
             baud = found.baud;
             return found.found;
         },
         .expected = {.configured = true,
                      .baud = 115200,
                      .clockUs = 1152000,
                      .reads = 5,
                      .writes = {"PING\r\n", "PING\r\n", "PING\r\n", "PING\r\n", "PING\r\n"},
                      .bauds = {9600, 38400, 115200, 38400, 57600, 115200, 57600},
                      .notes = {QStringLiteral("baud found=1 rate=115200 linkFailed=0"),
                                QStringLiteral("baud found=0 rate=38400 linkFailed=0"),
                                QStringLiteral("baud found=1 rate=115200 linkFailed=0"),
                                QStringLiteral("baud found=0 rate=57600 linkFailed=1")},
                      .evidence = {command("PING", TimedOut, 1000000, 6), command("PING", TimedOut, 1050000, 6),
                                   command("PING", Acknowledged, 1100000, 6), command("PING", TimedOut, 1101000, 6),
                                   command("PING", Acknowledged, 1151000, 6)}}});

    // Streaming receives with NMEA, RTCM3 and a warning whose streaming service sends a command.
    const QString position = QStringLiteral(
        "position t=%1 utc=0 fix=3 lat=48.117299999999993 lon=11.516666666666667 msl=545.39999999999998 "
        "ell=592.29999999999995 hacc=nan vacc=nan hdop=0.89999997615814209 vdop=nan speed=nan course=nan used=8 "
        "velocity=0");
    const QString correctionFrame = QStringLiteral("rtcm d300063ed0010203047d7045");
    QStringList receives{QStringLiteral("receive 7"), QStringLiteral("receive 0"), QStringLiteral("receive 4")};
    result.append(
        {.name = "streaming",
         .setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.script("DIAG\r\n", {{line("$ACK"), 30000}});
                 const auto gga = QByteArray::fromStdString(
                     nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
                 const auto rtcm = rtcmPacket(std::array<uint8_t, 6>{0x3e, 0xd0, 1, 2, 3, 4});
                 const QByteArray correction(reinterpret_cast<const char*>(rtcm.data()), qsizetype(rtcm.size()));
                 model.emitAt(GPSTestClock::START_US + 50000, gga + correction);
                 model.emitAt(GPSTestClock::START_US + 180000,
                              QByteArray::fromStdString(nmeaSentence("GPGSV,2,1,05,01,40,083,46,02,17,308,41")));
                 model.emitAt(GPSTestClock::START_US + 260000, line("$WARN") + gga);
                 model.emitAt(GPSTestClock::START_US + 900000, correction + correction);
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             return true;
         },
         .receives = 12,
         .expected = {.configured = true,
                      .clockUs = 2000000,
                      .reads = 14,
                      .writes = {"A\r\n", "DIAG\r\n"},
                      .notes = QStringList{command("A", Acknowledged, 1000000, 3)} + receives +
                               QStringList{command("DIAG", Acknowledged, 1260000, 6), QStringLiteral("receive 7")} +
                               QStringList(6, QStringLiteral("receive 0")) +
                               QStringList{QStringLiteral("receive 4"), QStringLiteral("receive 0")},
                      .evidence = {command("A", Acknowledged, 1000000, 3), command("DIAG", Acknowledged, 1260000, 6)},
                      .configurationEvidence = {command("A", Acknowledged, 1000000, 3)},
                      .batches = {QStringLiteral("batch updates=7"), QStringLiteral("usage t=1050000 used=8"),
                                  position.arg(1050000), correctionFrame, QStringLiteral("batch updates=4"),
                                  QStringLiteral("batch updates=7"), QStringLiteral("usage t=1260000 used=8"),
                                  position.arg(1260000), QStringLiteral("batch updates=4"), correctionFrame,
                                  correctionFrame}}});

    // A cancelled wait stops the session.
    // One command written in parts is one attempt that accounts for the bytes of every part.
    const QString parts = command("PART", Acknowledged, GPSTestClock::START_US, 6);
    result.append({.name = "parts",
                   .setup = [](LatencyModel& model) { model.script("RT\r\n", {{line("$DONE")}}); },
                   .channel = [](GPSCommandChannel& c, ChannelHarness& h, unsigned&, QStringList& notes) -> bool {
                       c.beginCommand({"PART", 100ms});
                       notes += QString::number(c.write("PA") && c.write("RT\r\n"));
                       notes +=
                           summary(c.awaitReply([&h] { return h.done ? Acknowledged : GPSCommandOutcome::Pending; }));
                       return true;
                   },
                   .expected = {.configured = true,
                                .clockUs = GPSTestClock::START_US + 1000,
                                .reads = 1,
                                .writes = {"PA", "RT\r\n"},
                                .notes = {QStringLiteral("1"), parts},
                                .evidence = {parts},
                                .batches = {QStringLiteral("batch updates=4")}}});

    result.append({.name = "cancelled",
                   .setup = [](LatencyModel& model) { model.script("A\r\n", {{line("$ACK")}}); },
                   .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
                       c.wait(10ms);
                       notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
                       return false;
                   },
                   .cancelWaits = true,
                   .expected = {.clockUs = 1010000,
                                .error = GPSProtocolError::Cancelled,
                                .notes = {command("A", Cancelled, 0, 0)}}});

    // A transport read failure ends the pending command and the session.
    const QStringList readFailure{command("A", Acknowledged, 1000000, 3), command("B", TransportError, 1001000, 3)};
    result.append(
        {.name = "readFailure",
         .setup =
             [](LatencyModel& model) {
                 model.script("A\r\n", {{line("$ACK")}});
                 model.failingReadsAfter = {"B\r\n"};
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             notes += summary(c.transact({"A", 100ms}, "A\r\n", GPSTextMatcher(lineReply)));
             notes += summary(c.transact({"B", 100ms}, "B\r\n", GPSTextMatcher(lineReply)));
             return false;
         },
         .expected = {.clockUs = 1001000,
                      .reads = 1,
                      .error = GPSProtocolError::Transport,
                      .detail = QStringLiteral("scripted read failure"),
                      .writes = {"A\r\n", "B\r\n"},
                      .notes = readFailure,
                      .evidence = readFailure,
                      .warnings = {QStringLiteral("Receiver read failed (status 4): scripted read failure")}}});
    return result;
}

/// Compares every observable of @a run with @a expected.
void compareRun(const Run& run, const Run& expected)
{
    QCOMPARE(run.configured, expected.configured);
    QCOMPARE(run.baud, expected.baud);
    QCOMPARE(run.clockUs, expected.clockUs);
    QCOMPARE(run.reads, expected.reads);
    QCOMPARE(run.error, expected.error);
    QCOMPARE(run.detail, expected.detail);
    QCOMPARE(run.writes, expected.writes);
    QCOMPARE(run.bauds, expected.bauds);
    QCOMPARE(run.notes, expected.notes);
    QCOMPARE(run.evidence, expected.evidence);
    QCOMPARE(run.configurationEvidence,
             expected.configurationEvidence.isEmpty() ? expected.evidence : expected.configurationEvidence);
    QCOMPARE(run.batches, expected.batches);
    QCOMPARE(run.warnings, expected.warnings);
}

}  // namespace

void GPSCommandChannelTest::_channel_data()
{
    QTest::addColumn<int>("scenario");
    const auto rows = scenarios();
    for (qsizetype index = 0; index < rows.size(); ++index) {
        QTest::newRow(rows[index].name) << static_cast<int>(index);
    }
}

void GPSCommandChannelTest::_channel()
{
    QFETCH(int, scenario);
    const QTest::ThrowOnFailEnabler endRowOnFailure;
    const Scenario definition = scenarios().at(scenario);
    const GPSProtocolLogCapture log;
    Run run = runChannel(definition);
    run.warnings = log.warnings();
    compareRun(run, definition.expected);
}

void GPSCommandChannelTest::_sequenceAttempts_data()
{
    QTest::addColumn<bool>("raw");
    QTest::addColumn<QByteArrayList>("replies");
    QTest::addColumn<int>("writes");
    QTest::addColumn<GPSCommandOutcome>("outcome");
    for (const bool raw : {false, true}) {
        const char* kind = raw ? "raw" : "line";
        const QByteArray ok = raw ? "..OK!" : line("$ACK");
        const QByteArray no = raw ? "..ERR" : line("$NAK");
        QTest::addRow("%s-first", kind) << raw << QByteArrayList{ok} << 1 << GPSCommandOutcome::Acknowledged;
        QTest::addRow("%s-after-rejection", kind)
            << raw << QByteArrayList{no, ok} << 2 << GPSCommandOutcome::Acknowledged;
        QTest::addRow("%s-after-silence", kind)
            << raw << QByteArrayList{{}, ok} << 2 << GPSCommandOutcome::Acknowledged;
        QTest::addRow("%s-exhausted", kind)
            << raw << QByteArrayList{no, no, no, ok} << 3 << GPSCommandOutcome::Rejected;
        QTest::addRow("%s-silent", kind) << raw << QByteArrayList{} << 3 << GPSCommandOutcome::TimedOut;
    }
}

void GPSCommandChannelTest::_sequenceAttempts()
{
    QFETCH(bool, raw);
    QFETCH(QByteArrayList, replies);
    QFETCH(int, writes);
    QFETCH(GPSCommandOutcome, outcome);
    const Run run =
        runChannel({.setup =
                        [&replies](LatencyModel& model) {
                            QList<LatencyModel::Reply> script;
                            for (const auto& reply : replies) {
                                script.append({reply});
                            }
                            model.script("A\r\n", script);
                        },
                    .channel = [raw](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
                        GPSCommandSequence::Command step{
                            .step = {"A", 100ms}, .wire = "A\r\n", .reply = GPSTextMatcher(lineReply), .attempts = 3};
                        if (raw) {
                            step.reply = GPSCommandSequence::RawReply{"OK!", "ERR"};
                        }
                        const auto result = c.runSequence({{step}});
                        notes += note(result);
                        return result.succeeded();
                    }});
    const bool succeeded = outcome == GPSCommandOutcome::Acknowledged;
    QCOMPARE(run.configured, succeeded);
    QCOMPARE(run.notes,
             QStringList{succeeded ? QStringLiteral("sequence step=- label= outcome=0")
                                   : QStringLiteral("sequence step=0 label=A outcome=%1").arg(int(outcome))});
    QCOMPARE(run.writes, QByteArrayList(writes, "A\r\n"));
    QVERIFY(run.evidence.last().startsWith(QStringLiteral("A outcome=%1 ").arg(int(outcome))));
}

void GPSCommandChannelTest::_sequenceEndsOnWriteFailure()
{
    // Even an optional step with attempts left ends the sequence when its write fails.
    const Run run = runChannel(
        {.setup =
             [](LatencyModel& model) {
                 model.failingWrites = {"A\r\n"};
                 model.script("B\r\n", {{line("$ACK")}});
             },
         .channel = [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& notes) -> bool {
             const GPSCommandSequence sequence{{
                 GPSCommandSequence::Command{
                     .step = {"A", 100ms, false}, .wire = "A\r\n", .reply = GPSTextMatcher(lineReply), .attempts = 3},
                 GPSCommandSequence::Command{.step = {"B", 100ms}, .wire = "B\r\n", .reply = GPSTextMatcher(lineReply)},
             }};
             notes += note(c.runSequence(sequence));
             return false;
         }});
    QCOMPARE(
        run.notes,
        QStringList{QStringLiteral("sequence step=0 label=A outcome=%1").arg(int(GPSCommandOutcome::TransportError))});
    QCOMPARE(run.writes, QByteArrayList{"A\r\n"});
    QCOMPARE(run.error, GPSProtocolError::Transport);
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
    // The transport found the command late before taking a byte: nothing is torn, so the connection stays usable.
    QTest::newRow("late before any byte")
        << GPSWriteResult{GPSWriteStatus::TimedOut} << GPSCommandOutcome::TimedOut << GPSProtocolError::None;
    QTest::newRow("cancelled") << GPSWriteResult{GPSWriteStatus::Cancelled, 6, 1, detail}
                               << GPSCommandOutcome::Cancelled << GPSProtocolError::Cancelled;
    QTest::newRow("error") << GPSWriteResult{GPSWriteStatus::Error, 6, 1, detail} << transport
                           << GPSProtocolError::Transport;
    // Without a transport detail, the channel names the command it could not write.
    QTest::newRow("error without detail")
        << GPSWriteResult{GPSWriteStatus::Error} << transport << GPSProtocolError::Transport;
    // Unsupported without progress rejects the attempt without poisoning the connection.
    QTest::newRow("unsupported") << GPSWriteResult{GPSWriteStatus::Unsupported} << transport << GPSProtocolError::None;
    QTest::newRow("unsupported progress")
        << GPSWriteResult{GPSWriteStatus::Unsupported, 6, 6, detail} << transport << GPSProtocolError::Transport;
    QTest::newRow("short write") << GPSWriteResult{GPSWriteStatus::Completed, 6, 2, detail} << transport
                                 << GPSProtocolError::Transport;
    QTest::newRow("short accepted") << GPSWriteResult{GPSWriteStatus::Completed, 2, 2, detail} << transport
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
    GPSTestClock clock(GPSTestClock::START_US);
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
    QList<GPSConfigurationEvidence> finished;
    GPSRuntimeObserver observer;
    observer.commandFinished = [&finished](const GPSConfigurationEvidence& command) { finished += command; };
    QStringList notes;
    GPSProtocolRuntime runtime(HARNESS_FAMILY,
                               std::make_unique<ChannelHarness>(
                                   [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList&) -> bool {
                                       (void) c.transact({"probe", 100ms}, "PROBE\n", GPSTextMatcher(lineReply));
                                       (void) c.transact({"again", 100ms}, "PROBE\n", GPSTextMatcher(lineReply));
                                       return true;
                                   },
                                   notes),
                               std::move(io), std::move(observer));
    unsigned baud = 0;
    (void) runtime.configure({}, baud);
    QVERIFY(!finished.isEmpty());
    const auto& evidence = finished.front();
    QCOMPARE(evidence.outcome, outcome);
    QCOMPARE(evidence.acceptedBytes, result.acceptedBytes);
    QCOMPARE(evidence.writtenBytes, result.writtenBytes);
    QCOMPARE(runtime.error(), error);
    // A sticky failure skips the second command's write; a rejected attempt does not.
    QCOMPARE(writes, error == GPSProtocolError::None ? 2 : 1);
    if (error != GPSProtocolError::None && error != GPSProtocolError::Cancelled) {
        QCOMPARE(
            runtime.errorDetail(),
            result.detail.isEmpty()
                ? QStringLiteral("Could not write 'probe' to the %1 receiver").arg(gpsReceiverName(HARNESS_FAMILY.type))
                : result.detail);
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
    GPSTestClock clock(GPSTestClock::START_US);
    int reads = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [&](std::span<uint8_t>, GPSDeadline) {
        ++reads;
        return result;
    };
    QStringList notes;
    GPSProtocolRuntime runtime(HARNESS_FAMILY,
                               std::make_unique<ChannelHarness>(
                                   [](GPSCommandChannel& c, ChannelHarness&, unsigned&, QStringList& stops) -> bool {
                                       stops += c.receiveUntil([] { return false; }, 50ms) ? QStringLiteral("done")
                                                                                           : QStringLiteral("failed");
                                       c.receiveFor(50ms);
                                       return true;
                                   },
                                   notes),
                               std::move(io));
    unsigned baud = 0;
    (void) runtime.configure({}, baud);
    QCOMPARE(notes, QStringList{QStringLiteral("failed")});
    QCOMPARE(runtime.error(), GPSProtocolError::Transport);
    QCOMPARE(runtime.errorDetail(), result.detail);
    QCOMPARE(reads, 1);
    QCOMPARE(log.warnings(), QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                             .arg(static_cast<int>(result.status))
                                             .arg(result.detail)});
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCommandChannelTest, TestLabel::Unit)
