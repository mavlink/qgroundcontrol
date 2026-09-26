#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <QtCore/QStringList>

#include "../Support/GPSRuntimeTestIO.h"
#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolRuntime.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {

/// Offers every received line to the outstanding command's matcher, and configures by running one sequence.
class SequenceProtocol final : public GPSFamilyProtocol
{
public:
    explicit SequenceProtocol(GPSCommandSequence sequence)
        : _sequence(std::move(sequence))
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig, unsigned&) override
    {
        result = co_await channel.runSequence(_sequence);
        co_return result.succeeded();
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        context.offerReply(frame.text());
        return {};
    }

    GPSCommandSequence::Result result;

private:
    GPSCommandSequence _sequence;
};

constexpr GPSReceiverFamily SEQUENCE_FAMILY{
    .type = GPSType::passive,
    .name = QLatin1StringView("sequence"),
    .stream = {.framers = GPSFrameKind::ASCIILine, .enabled = GPSFrameKind::ASCIILine}};

/// Answers each written command with its next scripted reply; an empty reply is silence.
struct ScriptedPeer
{
    explicit ScriptedPeer(GPSTestClock& testClock)
        : clock(testClock)
    {}

    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(clock);
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            const std::string command(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            commands.push_back(command);
            if (command == failingWrite) {
                return {GPSWriteStatus::Error, 0, 0, QStringLiteral("scripted write failure")};
            }
            auto& script = replies[command];
            if (!script.empty()) {
                pending += script.front();
                script.erase(script.begin());
            }
            const int size = static_cast<int>(bytes.size());
            return {GPSWriteStatus::Completed, size, size};
        };
        result.read = [this](std::span<uint8_t> buffer, GPSDeadline deadline) -> GPSReadResult {
            if (pending.empty()) {
                clock.advanceTo(deadline.untilUs);
                return {GPSReadStatus::TimedOut};
            }
            const size_t count = std::min(pending.size(), buffer.size());
            std::copy_n(pending.begin(), count, buffer.begin());
            pending.erase(0, count);
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        return result;
    }

    GPSRuntimeObserver observer()
    {
        return {.commandFinished = [this](const GPSCommandResult& command) { finished.push_back(command.evidence); }};
    }

    GPSTestClock& clock;
    std::map<std::string, std::vector<std::string>> replies;
    std::string failingWrite;
    std::string pending;
    std::vector<std::string> commands;
    std::vector<GPSConfigurationEvidence> finished;
};

GPSCommandOutcome acknowledgement(std::string_view reply)
{
    return reply == "OK"   ? GPSCommandOutcome::Acknowledged
           : reply == "NO" ? GPSCommandOutcome::Rejected
                           : GPSCommandOutcome::Pending;
}

GPSCommandSequence::Command command(const std::string& name, bool required = true, unsigned attempts = 1)
{
    return {.step = {name, std::chrono::milliseconds(100), {}, required},
            .wire = QByteArray::fromStdString(name + "\r\n"),
            .reply = GPSTextMatcher(acknowledgement),
            .attempts = attempts};
}

/// Runs @a sequence as a configuration. @return the sequence result.
GPSCommandSequence::Result runSequence(ScriptedPeer& peer, GPSCommandSequence sequence,
                                       GPSProtocolError* error = nullptr)
{
    auto protocol = std::make_unique<SequenceProtocol>(std::move(sequence));
    auto& family = *protocol;
    GPSProtocolRuntime runtime(SEQUENCE_FAMILY, std::move(protocol), peer.io(), peer.observer());
    unsigned baud = 0;
    (void) runtime.configure({}, baud);
    if (error) {
        *error = runtime.error();
    }
    return family.result;
}

}  // namespace

class GPSCommandSequenceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _attempts_data();
    void _attempts();
    void _requiredFailureNamesStep();
    void _customSteps();
    void _ioFailureEndsSequence();
};

void GPSCommandSequenceTest::_attempts_data()
{
    QTest::addColumn<bool>("raw");
    QTest::addColumn<QStringList>("script");
    QTest::addColumn<int>("writes");
    QTest::addColumn<int>("outcome");

    const auto ack = static_cast<int>(GPSCommandOutcome::Acknowledged);
    const auto rejected = static_cast<int>(GPSCommandOutcome::Rejected);
    const auto timedOut = static_cast<int>(GPSCommandOutcome::TimedOut);
    for (const bool raw : {false, true}) {
        const QString family = raw ? QStringLiteral("raw ") : QStringLiteral("line ");
        const QString ok = raw ? QStringLiteral("..OK") : QStringLiteral("OK\r\n");
        const QString no = raw ? QStringLiteral("..NO") : QStringLiteral("NO\r\n");
        QTest::newRow(qPrintable(family + "first")) << raw << QStringList{ok} << 1 << ack;
        QTest::newRow(qPrintable(family + "after rejection")) << raw << QStringList{no, ok} << 2 << ack;
        QTest::newRow(qPrintable(family + "after silence")) << raw << QStringList{{}, ok} << 2 << ack;
        QTest::newRow(qPrintable(family + "exhausted")) << raw << QStringList{no, no, no, ok} << 3 << rejected;
        QTest::newRow(qPrintable(family + "silent")) << raw << QStringList{} << 3 << timedOut;
    }
}

void GPSCommandSequenceTest::_attempts()
{
    QFETCH(bool, raw);
    QFETCH(QStringList, script);
    QFETCH(int, writes);
    QFETCH(int, outcome);

    GPSTestClock clock;
    ScriptedPeer peer(clock);
    for (const auto& reply : script) {
        peer.replies["A\r\n"].push_back(reply.toStdString());
    }
    auto step = command("A", true, 3);
    if (raw) {
        step.reply = GPSCommandSequence::RawReply{"OK", "NO"};
    }

    const auto result = runSequence(peer, {{step}});

    QCOMPARE(result.succeeded(), outcome == static_cast<int>(GPSCommandOutcome::Acknowledged));
    QCOMPARE(static_cast<int>(result.outcome),
             result.succeeded() ? static_cast<int>(GPSCommandOutcome::Pending) : outcome);
    QCOMPARE(peer.commands, std::vector<std::string>(static_cast<size_t>(writes), "A\r\n"));
    QCOMPARE(static_cast<int>(peer.finished.back().outcome), outcome);
}

void GPSCommandSequenceTest::_requiredFailureNamesStep()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.replies = {{"A\r\n", {"NO\r\n"}}, {"B\r\n", {"OK\r\n"}}, {"C\r\n", {"NO\r\n"}}, {"D\r\n", {"OK\r\n"}}};

    const auto result = runSequence(peer, {{command("A", false), command("B"), command("C"), command("D")}});

    QCOMPARE(result.failedStep, std::optional<size_t>(2));
    QCOMPARE(result.failedLabel, std::string("C"));
    QCOMPARE(result.outcome, GPSCommandOutcome::Rejected);
    QCOMPARE(peer.commands, (std::vector<std::string>{"A\r\n", "B\r\n", "C\r\n"}));
    QVERIFY(!peer.finished.front().required);
    QVERIFY(peer.finished.back().required);
}

void GPSCommandSequenceTest::_customSteps()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.replies = {{"A\r\n", {"OK\r\n"}}, {"B\r\n", {"OK\r\n"}}};
    std::vector<std::string> ran;
    const auto custom = [&ran, &peer](const std::string& label, bool succeeds, bool required) {
        return GPSCommandSequence::Custom{label,
                                          [&ran, &peer, label, succeeds]() -> GPSTask<bool> {
                                              ran.push_back(label + "@" + std::to_string(peer.commands.size()));
                                              co_return succeeds;
                                          },
                                          required};
    };

    const auto result = runSequence(peer, {{command("A"), custom("optional", false, false), command("B"),
                                            custom("quirk", false, true), command("C")}});

    QCOMPARE(ran, (std::vector<std::string>{"optional@1", "quirk@2"}));
    QCOMPARE(result.failedStep, std::optional<size_t>(3));
    QCOMPARE(result.failedLabel, std::string("quirk"));
    QCOMPARE(result.outcome, GPSCommandOutcome::Pending);
    QCOMPARE(peer.commands, (std::vector<std::string>{"A\r\n", "B\r\n"}));
}

void GPSCommandSequenceTest::_ioFailureEndsSequence()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.failingWrite = "A\r\n";
    peer.replies = {{"B\r\n", {"OK\r\n"}}};
    GPSProtocolError error = GPSProtocolError::None;

    const auto result = runSequence(peer, {{command("A", false, 3), command("B")}}, &error);

    QCOMPARE(result.failedStep, std::optional<size_t>(0));
    QCOMPARE(result.outcome, GPSCommandOutcome::TransportError);
    QCOMPARE(peer.commands, std::vector<std::string>{"A\r\n"});
    QCOMPARE(error, GPSProtocolError::Transport);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCommandSequenceTest, TestLabel::Unit)

#include "GPSCommandSequenceTest.moc"
