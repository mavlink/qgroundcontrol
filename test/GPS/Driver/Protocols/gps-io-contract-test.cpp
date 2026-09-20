#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <QtCore/QScopeGuard>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolFeatures.h"
#include "GPSProtocolTestIO.h"
#include "NMEAUtils.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"
#include "UnitTest.h"

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

namespace {
struct ScriptedIO
{
    enum class Operation
    {
        Read,
        Write,
        Baud
    };
    Operation fault;
    int error;
    bool failed = false;
    unsigned operations = 0;
    QString detail = QStringLiteral("Receiver connection lost: Gerät disconnected");

    bool fail(Operation operation)
    {
        CHECK(!failed);
        CHECK(++operations < 100);
        failed = operation == fault;
        return failed;
    }

    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO();
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocol::ReadCancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                        detail};
            }
            gps_test_time = deadline.untilUs + 1000;
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocol::ReadCancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error, 0, 0,
                        detail};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        result.setBaudrate = [this](unsigned) {
            return !fail(Operation::Baud)                ? GPSBaudStatus::Configured
                   : error == GPSProtocol::ReadCancelled ? GPSBaudStatus::Cancelled
                                                         : GPSBaudStatus::Error;
        };
        return result;
    }
};

class IOProbe : public GPSProtocol
{
public:
    using GPSProtocol::beginCommandWrite;
    using GPSProtocol::GPSProtocol;
    using GPSProtocol::log;
    using GPSProtocol::read;
    using GPSProtocol::resetIOError;
    using GPSProtocol::write;

    int configure(unsigned&, const GPSConfig&) override { return 0; }

    int receive(unsigned timeout) override { return receiveDecoded(timeout); }
};

class CommandProbe : public IOProbe
{
public:
    using IOProbe::IOProbe;

    GPSCommandResult attempt(bool required = true)
    {
        _reply = GPSCommandOutcome::Pending;
        _awaiting = true;
        const auto clearReply = qScopeGuard([this] { _awaiting = false; });
        const GPSConfigurationStep step{
            "SET", std::chrono::milliseconds(100), {GPSReceiverSetting::OutputRateHz}, required};
        static constexpr std::array<uint8_t, 3> bytes{'S', 'E', 'T'};
        if (!writeCommand(step, bytes)) {
            return completeCommand(ioError() == ReadCancelled ? GPSCommandOutcome::Cancelled
                                                              : GPSCommandOutcome::TransportError);
        }
        return awaitCommand(step, [this] { return _reply; });
    }

    GPSCommandResult attemptWithin(unsigned timeout)
    {
        const Operation operation(*this, timeout);
        return attempt();
    }

    GPSCommandResult awaitAgain()
    {
        return awaitCommand({"must not replace completed evidence", std::chrono::milliseconds(100)},
                            [this] { return _reply; });
    }

    bool awaiting() const { return _awaiting; }

    GPSCommandOutcome reply() const { return _reply; }

private:
    int decodeByte(uint8_t byte) override
    {
        if (byte != '\n') {
            _line += static_cast<char>(byte);
            return 0;
        }
        if (_line == "N") {
            publishIntegrity();
        } else if (_awaiting && _line == "ACK") {
            _reply = GPSCommandOutcome::Acknowledged;
        } else if (_awaiting && _line == "NAK") {
            _reply = GPSCommandOutcome::Rejected;
        }
        _line.clear();
        return 0;  // ACK-only input need not contain a navigation update.
    }

    std::string _line;
    GPSCommandOutcome _reply = GPSCommandOutcome::Pending;
    bool _awaiting = false;
};

struct CommandIO
{
    std::deque<std::string> chunks;
    std::vector<std::string> transcript;
    std::vector<GPSCommandResult> completions;
    GPSWriteResult writeResult{GPSWriteStatus::Completed, 3, 3};
    GPSReadStatus readStatus = GPSReadStatus::Data;
    uint64_t writeDurationUs = 40000;
    size_t decodedReports = 0;
    std::function<void()> onCompletion;

    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO();
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
            CHECK(!bytes.empty());
            CHECK(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()) == "SET");
            transcript.push_back("write/" + std::to_string(deadline.untilUs));
            gps_test_time += writeDurationUs;
            return writeResult;
        };
        result.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            transcript.push_back("read/" + std::to_string(deadline.untilUs));
            CHECK(gps_test_time < deadline.untilUs);
            if (readStatus != GPSReadStatus::Data) {
                return {readStatus};
            }
            if (chunks.empty()) {
                gps_test_time = deadline.untilUs;
                return {GPSReadStatus::TimedOut};
            }
            const auto chunk = std::move(chunks.front());
            chunks.pop_front();
            CHECK(chunk.size() <= bytes.size());
            std::memcpy(bytes.data(), chunk.data(), chunk.size());
            gps_test_time += 1000;
            return {GPSReadStatus::Data, static_cast<int>(chunk.size())};
        };
        result.decoded = [this](const auto& batch) { decodedReports += batch.events.size(); };
        result.commandFinished = [this](const auto& command) {
            completions.push_back(command);
            if (onCompletion) {
                onCompletion();
            }
        };
        return result;
    }
};

static void commandAttempts()
{
    gps_test_time = 1000000;
    CommandIO io;
    io.chunks = {"N\nA", "CK\nN\n"};
    CommandProbe probe(io.io());
    io.onCompletion = [&] { probe.finishConfigurationEvidence(); };
    const auto result = probe.attempt(false);
    CHECK(result.evidence.outcome == GPSCommandOutcome::Acknowledged);
    CHECK(result.evidence.startedAtUs == 1000000);
    CHECK(result.evidence.finishedAtUs == 1042000);
    CHECK(result.evidence.acceptedBytes == 3 && result.evidence.writtenBytes == 3 &&
          result.evidence.uncertainBytes == 0);
    CHECK(result.affectedSettings.contains(GPSReceiverSetting::OutputRateHz));
    CHECK(!result.evidence.required);
    CHECK(io.completions.size() == 1 && !io.completions.front().evidence.required);
    CHECK(io.transcript == (std::vector<std::string>{"write/1100000", "read/1100000", "read/1100000"}));
    CHECK(io.decodedReports == 2);
    CHECK(!probe.awaiting());
    const auto operations = io.transcript.size();
    const std::array<uint8_t, 4> lateReply{'N', 'A', 'K', '\n'};
    probe.consume(lateReply);
    CHECK(probe.reply() == GPSCommandOutcome::Acknowledged);
    CHECK(io.transcript.size() == operations);  // Decoding never performs device I/O.
    probe.finishConfigurationEvidence();
    CHECK(io.completions.size() == 1);
    CHECK(probe.awaitAgain().evidence.command == "SET");
    CHECK(io.transcript.size() == operations);

    io.chunks = {"NAK\n"};
    CHECK(probe.attempt().evidence.outcome == GPSCommandOutcome::Rejected);
    CHECK(io.completions.size() == 2);
    CHECK(io.completions.back().evidence.required);
    CHECK(!probe.awaiting());

    for (const auto readStatus : {GPSReadStatus::Data, GPSReadStatus::Cancelled, GPSReadStatus::Error}) {
        gps_test_time = 1000000;
        CommandIO failed;
        failed.readStatus = readStatus;
        CommandProbe receiver(failed.io());
        const auto failure = receiver.attempt();
        CHECK(failure.evidence.outcome == (readStatus == GPSReadStatus::Data ? GPSCommandOutcome::TimedOut
                                           : readStatus == GPSReadStatus::Cancelled
                                               ? GPSCommandOutcome::Cancelled
                                               : GPSCommandOutcome::TransportError));
        CHECK(failed.transcript == (std::vector<std::string>{"write/1100000", "read/1100000"}));
        CHECK(failed.completions.size() == 1);
        CHECK(!receiver.awaiting());
        receiver.finishConfigurationEvidence();
        if (readStatus != GPSReadStatus::Data) {
            CHECK(receiver.receive(100) < 0);
        } else {
            CHECK(gps_test_time == 1100000);  // Writing did not buy a second read deadline.
        }
        CHECK(failed.completions.size() == 1);
        CHECK(failed.transcript.size() == 2);
    }

    for (const GPSWriteResult& failure :
         {GPSWriteResult{GPSWriteStatus::TimedOut, 3, 3}, GPSWriteResult{GPSWriteStatus::Cancelled, 3, 1},
          GPSWriteResult{GPSWriteStatus::Error, 3, 1}, GPSWriteResult{GPSWriteStatus::Unsupported}}) {
        gps_test_time = 1000000;
        CommandIO failed;
        failed.writeResult = failure;
        CommandProbe receiver(failed.io());
        const auto attemptResult = receiver.attempt(false);
        CHECK(attemptResult.evidence.outcome == (failure.status == GPSWriteStatus::Cancelled
                                                     ? GPSCommandOutcome::Cancelled
                                                     : GPSCommandOutcome::TransportError));
        CHECK(attemptResult.evidence.acceptedBytes == failure.acceptedBytes);
        CHECK(attemptResult.evidence.writtenBytes == failure.writtenBytes);
        CHECK(attemptResult.evidence.uncertainBytes == failure.uncertainBytes());
        CHECK(!attemptResult.evidence.required);
        CHECK(failed.completions.size() == 1);
        CHECK(!failed.completions.front().evidence.required);
        CHECK(!receiver.awaiting());
        CHECK(receiver.awaitAgain().evidence.outcome == attemptResult.evidence.outcome);
        receiver.finishConfigurationEvidence();
        if (failure.status != GPSWriteStatus::Unsupported) {
            CHECK(receiver.receive(100) < 0);
        } else {
            CHECK(receiver.ioError() == 0);  // Unsupported is a rejected attempt, not a poisoned connection.
        }
        CHECK(failed.completions.size() == 1);
        CHECK(failed.transcript == std::vector<std::string>{"write/1100000"});
    }

    gps_test_time = 1000000;
    CommandIO exhausted;
    exhausted.writeDurationUs = 100000;
    CommandProbe receiver(exhausted.io());
    CHECK(receiver.attempt().evidence.outcome == GPSCommandOutcome::TimedOut);
    CHECK(exhausted.transcript == std::vector<std::string>{"write/1100000"});
    CHECK(exhausted.completions.size() == 1);
    CHECK(!receiver.awaiting());

    gps_test_time = 1000000;
    CommandIO outerDeadline;
    CommandProbe bounded(outerDeadline.io());
    CHECK(bounded.attemptWithin(50).evidence.outcome == GPSCommandOutcome::TimedOut);
    CHECK(outerDeadline.transcript == (std::vector<std::string>{"write/1050000", "read/1050000"}));
    CHECK(gps_test_time == 1050000);

    gps_test_time = 1000000;
    CommandIO nested;
    nested.chunks = {"ACK\n"};
    CommandProbe nestedReceiver(nested.io());
    nested.onCompletion = [&] {
        if (nested.completions.size() == 1) {
            nestedReceiver.beginCommandWrite("next");
        }
    };
    CHECK(nestedReceiver.attempt().evidence.outcome == GPSCommandOutcome::Acknowledged);
    CHECK(nested.completions.size() == 1);
    nestedReceiver.finishConfigurationEvidence();
    CHECK(nested.completions.size() == 2);
    CHECK(nested.completions.back().evidence.command == "next");
    CHECK(nested.completions.back().evidence.outcome == GPSCommandOutcome::Written);

    gps_test_time = 1000000;
    CommandIO throwing;
    throwing.chunks = {"ACK\n"};
    CommandProbe throwingReceiver(throwing.io());
    throwing.onCompletion = [] { throw std::runtime_error("completion"); };
    bool threw = false;
    try {
        (void) throwingReceiver.attempt();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw && !throwingReceiver.awaiting());
    throwingReceiver.finishConfigurationEvidence();
    CHECK(throwing.completions.size() == 1);
}

static void ashtechAcknowledgementReturnsImmediately()
{
#if QGC_GPS_ENABLE_ASHTECH
    gps_test_time = 1000000;
    std::string reply = NMEAUtils::repairChecksum("$PASHR,PRT,A,115200").toStdString();
    std::vector<std::string> writes;
    std::vector<GPSCommandResult> completions;
    int reads = 0;
    auto io = makeGPSProtocolTestIO();
    io.write = [&](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
        writes.emplace_back(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        gps_test_time += 1000;
        return writes.size() == 1 ? GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())}
                                  : GPSWriteResult{GPSWriteStatus::Cancelled};
    };
    io.read = [&](std::span<uint8_t> bytes, GPSDeadline) -> GPSReadResult {
        CHECK(!reply.empty());  // A decoded PRT response must not trigger a trailing timeout read.
        ++reads;
        const auto size = std::min<size_t>({bytes.size(), reply.size(), 4});
        std::memcpy(bytes.data(), reply.data(), size);
        reply.erase(0, size);
        gps_test_time += 1000;
        return {GPSReadStatus::Data, static_cast<int>(size)};
    };
    io.commandFinished = [&](const auto& result) { completions.push_back(result); };
    GPSNativePositionReport position;
    GPSNativeAshtech receiver(std::move(io), &position, nullptr);
    unsigned baud = 115200;
    CHECK(receiver.configure(baud, {}) < 0);
    CHECK(writes == (std::vector<std::string>{"$PASHQ,PRT\r\n", "$PASHQ,RID\r\n"}));
    CHECK(reads > 1);
    CHECK(completions.size() == 2);
    CHECK(completions[0].evidence.outcome == GPSCommandOutcome::Acknowledged);
    CHECK(completions[0].evidence.finishedAtUs == 1001000 + uint64_t(reads) * 1000);
    CHECK(completions[1].evidence.outcome == GPSCommandOutcome::Cancelled);
    CHECK(receiver.ioError() == GPSProtocol::ReadCancelled);
#endif
}

static void sharedResults()
{
    const uint8_t payload[6] = {};
    const QString detail = QStringLiteral("Invalid receiver progress: Gerät");
    const std::array<GPSWriteResult, 7> invalidWrites{{
        {GPSWriteStatus::Completed, 6, 2, detail},
        {GPSWriteStatus::Completed, 0, 0, detail},
        {GPSWriteStatus::Completed, -1, 6, detail},
        {GPSWriteStatus::Completed, 6, -1, detail},
        {GPSWriteStatus::Completed, 6, 7, detail},
        {GPSWriteStatus::Completed, 7, 6, detail},
        {GPSWriteStatus::Unsupported, 6, 6, detail},
    }};
    for (const auto& result : invalidWrites) {
        int writes = 0;
        GPSCommandResult completion;
        auto io = makeGPSProtocolTestIO();
        io.write = [&](std::span<const uint8_t>, GPSDeadline) {
            ++writes;
            return result;
        };
        io.commandFinished = [&](const auto& command) { completion = command; };
        IOProbe probe(std::move(io));
        probe.beginCommandWrite("probe");
        CHECK(probe.write(payload, sizeof(payload)) < 0);
        CHECK(probe.ioError() == -EIO);
        CHECK(probe.ioErrorDetail() == detail);
        CHECK(completion.evidence.acceptedBytes == result.acceptedBytes);
        CHECK(completion.evidence.writtenBytes == result.writtenBytes);
        CHECK(completion.evidence.uncertainBytes == result.uncertainBytes());
        CHECK(probe.write(payload, sizeof(payload)) < 0);
        CHECK(writes == 1);
        probe.resetIOError();
        CHECK(probe.ioErrorDetail().isEmpty());
    }
    const std::array<GPSReadResult, 3> invalidReads{{
        {GPSReadStatus::Data, -1, detail},
        {GPSReadStatus::Data, 2, detail},
        {GPSReadStatus::TimedOut, 1, detail},
    }};
    for (const auto& result : invalidReads) {
        auto io = makeGPSProtocolTestIO();
        io.read = [&](std::span<uint8_t>, GPSDeadline) { return result; };
        IOProbe probe(std::move(io));
        uint8_t byte;
        CHECK(probe.read(&byte, 1, 0) == -EIO);
        CHECK(probe.ioErrorDetail() == detail);
    }
    QStringList messages;
    GPSProtocolIO io;
    io.log = [&](GPSProtocolLogLevel, QStringView message) { messages.push_back(message.toString()); };
    IOProbe probe(std::move(io));
    probe.log(GPSProtocolLogLevel::Warning, "Gerät");
    probe.log(GPSProtocolLogLevel::Warning, "%s %d", "Gerät", 2);
    CHECK(messages == (QStringList{QStringLiteral("Gerät"), QStringLiteral("Gerät 2")}));
}

static std::unique_ptr<GPSBaseProtocol> createReceiver(unsigned family, ScriptedIO& io,
                                                       GPSNativePositionReport& position,
                                                       GPSNativeSatelliteReport& satellites)
{
    switch (family) {
#if QGC_GPS_ENABLE_UBX
        case 0:
            return std::make_unique<GPSNativeUBX>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_ASHTECH
        case 1:
            return std::make_unique<GPSNativeAshtech>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_SBF
        case 2:
            return std::make_unique<GPSNativeSBF>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_FEMTO
        case 3:
            return std::make_unique<GPSNativeFemto>(io.io(), &position, &satellites);
#endif
        default:
            return {};
    }
}

}  // namespace

class GPSProtocolIOContractTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolIOContractTest::_protocol()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        commandAttempts();
        ashtechAcknowledgementReturnsImmediately();
        sharedResults();
        CHECK(GPSDeadline{}.remainingMilliseconds(0) == INT32_MAX);
        CHECK(GPSDeadline{0}.remainingMilliseconds(0) == 0);
        CHECK(GPSDeadline{1}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1001}.remainingMilliseconds(0) == 2);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(1001) == 0);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX - 1001) == 2);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX) == 0);
        CHECK(GPSDeadline{}.toQDeadlineTimer().isForever());
        CHECK(GPSDeadline{0}.toQDeadlineTimer().hasExpired());
        const auto liveDeadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);
        const GPSDeadline deadline{static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(liveDeadline.time_since_epoch()).count())};
        const auto qtDeadline = deadline.toQDeadlineTimer();
        CHECK(!qtDeadline.isForever());
        CHECK(std::abs(qtDeadline.deadline() - QDeadlineTimer(liveDeadline, Qt::PreciseTimer).deadline()) <= 1);
        for (unsigned family = 0; family != 4; ++family) {
            for (const auto fault :
                 {ScriptedIO::Operation::Read, ScriptedIO::Operation::Write, ScriptedIO::Operation::Baud}) {
                for (const int error : {GPSProtocol::ReadCancelled, -EIO}) {
                    for (const auto mode : {GPSProtocol::OutputMode::GPS, GPSProtocol::OutputMode::RTCM}) {
                        gps_test_time = 0;
                        gps_test_warnings.clear();
                        ScriptedIO io{fault, error};
                        GPSNativePositionReport position{};
                        GPSNativeSatelliteReport satellites{};
                        auto receiver = createReceiver(family, io, position, satellites);
                        if (!receiver) {
                            continue;
                        }
                        GPSProtocol::GPSConfig config{};
                        config.base.surveyInAccMeters = 1;
                        config.base.surveyInDurationSecs = 60;
                        config.output_mode = mode;
                        unsigned baudrate = 115200;
                        const int result = receiver->configure(baudrate, config);
                        CHECK(io.failed);
                        CHECK(result < 0);
                        CHECK(receiver->ioError() == error);
                        CHECK(receiver->ioErrorDetail() ==
                              (fault == ScriptedIO::Operation::Baud ? QString() : io.detail));
                        const auto warnings = gps_test_warnings;
                        if (fault == ScriptedIO::Operation::Read && error != GPSProtocol::ReadCancelled) {
                            CHECK(warnings ==
                                  QStringList{QStringLiteral("Receiver read failed (status %1, code %2): %3")
                                                  .arg(static_cast<int>(GPSReadStatus::Error))
                                                  .arg(error)
                                                  .arg(io.detail)});
                        } else {
                            CHECK(warnings.empty());
                        }
                        CHECK(receiver->receive(10) < 0);
                        CHECK(receiver->ioError() == error);
                        CHECK(receiver->ioErrorDetail() ==
                              (fault == ScriptedIO::Operation::Baud ? QString() : io.detail));
                        CHECK(gps_test_warnings == warnings);
                    }
                }
            }
        }
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolIOContractTest, TestLabel::Unit)

#include "gps-io-contract-test.moc"
