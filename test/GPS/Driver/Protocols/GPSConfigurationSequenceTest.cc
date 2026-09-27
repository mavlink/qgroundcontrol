#include <algorithm>
#include <chrono>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "GPSAsciiProtocol.h"
#include "GPSProtocolTestIO.h"
#include "UnitTest.h"

namespace {

/// Offers every received line to the outstanding command's matcher.
class SequenceProtocol final : public GPSAsciiProtocol
{
public:
    using GPSAsciiProtocol::GPSAsciiProtocol;
    using GPSProtocol::runSequence;

    bool configure(unsigned&, const GPSConfig&) override { return true; }

protected:
    int handleReceiverLine(std::string_view line) override
    {
        offerReply(line);
        return 0;
    }
};

/// Answers each written command with its next scripted reply; an empty reply is silence.
struct ScriptedPeer
{
    explicit ScriptedPeer(GPSTestClock& testClock)
        : clock(testClock)
    {}

    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO(clock);
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
        result.commandFinished = [this](const GPSCommandResult& command) { finished.push_back(command.evidence); };
        return result;
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

GPSConfigurationSequence::Command command(const std::string& name, bool required = true, unsigned attempts = 1)
{
    return {.step = {name, std::chrono::milliseconds(100), {}, required},
            .wire = name + "\r\n",
            .reply = acknowledgement,
            .attempts = attempts};
}

}  // namespace

class GPSConfigurationSequenceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _attempts_data();
    void _attempts();
    void _requiredFailureNamesStep();
    void _customSteps();
    void _ioFailureEndsSequence();
};

void GPSConfigurationSequenceTest::_attempts_data()
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

void GPSConfigurationSequenceTest::_attempts()
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
    SequenceProtocol protocol(peer.io(), false);
    auto step = command("A", true, 3);
    if (raw) {
        step.reply = GPSConfigurationSequence::RawReply{"OK", "NO"};
    }

    const auto result = protocol.runSequence({{step}});

    QCOMPARE(result.succeeded(), outcome == static_cast<int>(GPSCommandOutcome::Acknowledged));
    QCOMPARE(static_cast<int>(result.outcome),
             result.succeeded() ? static_cast<int>(GPSCommandOutcome::Pending) : outcome);
    QCOMPARE(peer.commands, std::vector<std::string>(static_cast<size_t>(writes), "A\r\n"));
    QCOMPARE(static_cast<int>(peer.finished.back().outcome), outcome);
}

void GPSConfigurationSequenceTest::_requiredFailureNamesStep()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.replies = {{"A\r\n", {"NO\r\n"}}, {"B\r\n", {"OK\r\n"}}, {"C\r\n", {"NO\r\n"}}, {"D\r\n", {"OK\r\n"}}};
    SequenceProtocol protocol(peer.io(), false);

    const auto result = protocol.runSequence({{command("A", false), command("B"), command("C"), command("D")}});

    QCOMPARE(result.failedStep, std::optional<size_t>(2));
    QCOMPARE(result.failedLabel, std::string("C"));
    QCOMPARE(result.outcome, GPSCommandOutcome::Rejected);
    QCOMPARE(peer.commands, (std::vector<std::string>{"A\r\n", "B\r\n", "C\r\n"}));
    QVERIFY(!peer.finished.front().required);
    QVERIFY(peer.finished.back().required);
}

void GPSConfigurationSequenceTest::_customSteps()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.replies = {{"A\r\n", {"OK\r\n"}}, {"B\r\n", {"OK\r\n"}}};
    SequenceProtocol protocol(peer.io(), false);
    std::vector<std::string> ran;
    const auto custom = [&ran, &peer](const std::string& label, bool succeeds, bool required) {
        return GPSConfigurationSequence::Custom{label,
                                                [&ran, &peer, label, succeeds] {
                                                    ran.push_back(label + "@" + std::to_string(peer.commands.size()));
                                                    return succeeds;
                                                },
                                                required};
    };

    const auto result = protocol.runSequence(
        {{command("A"), custom("optional", false, false), command("B"), custom("quirk", false, true), command("C")}});

    QCOMPARE(ran, (std::vector<std::string>{"optional@1", "quirk@2"}));
    QCOMPARE(result.failedStep, std::optional<size_t>(3));
    QCOMPARE(result.failedLabel, std::string("quirk"));
    QCOMPARE(result.outcome, GPSCommandOutcome::Pending);
    QCOMPARE(peer.commands, (std::vector<std::string>{"A\r\n", "B\r\n"}));
}

void GPSConfigurationSequenceTest::_ioFailureEndsSequence()
{
    GPSTestClock clock;
    ScriptedPeer peer(clock);
    peer.failingWrite = "A\r\n";
    peer.replies = {{"B\r\n", {"OK\r\n"}}};
    SequenceProtocol protocol(peer.io(), false);

    const auto result = protocol.runSequence({{command("A", false, 3), command("B")}});

    QCOMPARE(result.failedStep, std::optional<size_t>(0));
    QCOMPARE(result.outcome, GPSCommandOutcome::TransportError);
    QCOMPARE(peer.commands, std::vector<std::string>{"A\r\n"});
    QCOMPARE(protocol.ioError(), GPSProtocolError::Transport);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSConfigurationSequenceTest, TestLabel::Unit)

#include "GPSConfigurationSequenceTest.moc"
