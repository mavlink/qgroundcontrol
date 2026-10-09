#include "UnicoreProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

std::string native(std::string_view name, std::string_view body, uint32_t milliseconds = 378238000)
{
    return GPSTest::unicoreNative(name, body, milliseconds);
}

std::string position(std::string_view type, const std::array<double, 3>& coordinates, uint32_t milliseconds = 378238000)
{
    return GPSTest::unicorePosition(type, coordinates, milliseconds);
}

std::vector<uint8_t> correction(std::string_view payload = "\x43\x20")
{
    return rtcmPacket(payload);
}

void consume(GPSProtocolRuntime& driver, std::string_view line, size_t chunk = 5)
{
    while (!line.empty()) {
        const auto count = std::min(chunk, line.size());
        driver.consume({reinterpret_cast<const uint8_t*>(line.data()), count});
        line.remove_prefix(count);
    }
}

using Receiver = ModelReceiver<UnicoreReceiverModel>;

/// Reads and baud changes: every transport call besides writes.
int transportCalls(const Receiver& receiver)
{
    return receiver.reads() + receiver.baudChanges();
}

/// A required command's fault: the receiver's answer, or the link's.
enum class CommandFault
{
    Silence,
    Reject,
    WrongAck,
    Corrupt,
    Cancel,
    WriteError,
    ShortWrite,
};

GPSConfig baseConfig(bool fixed)
{
    GPSConfig config{};
    config.base.mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                                   .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                             : GPSBaseStationConfig::Mode{GPSBaseStationConfig::ReceiverAveraging{}};
    return config;
}

void identityAndRole(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    for (const auto* model : {"UM980", "UM982"}) {
        for (const unsigned baud : {0U, 115200U}) {
            clock.reset();
            Receiver receiver(clock);
            receiver.model.chunk = 1;
            if (std::string_view(model) == "UM980") {
                receiver.model.version =
                    native("VERSIONA", "\"UM980\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"");
            }
            receiver.model.role = "MODE BASE TIME";
            if (baud == 0) {
                receiver.model.availableBaud = 460800;
            }
            GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
            unsigned rate = baud;
            QVERIFY(driver.configure(baseConfig(false), rate));
            QVERIFY(driver.receiverReady());
            QCOMPARE(rate, receiver.model.availableBaud);
            QCOMPARE(driver.identity(), QLatin1StringView(model) + QStringLiteral(" R4.10Build15434"));
            const auto& commands = receiver.commands();
            const auto mutation = std::find(commands.begin(), commands.end(), "UNLOG\r\n");
            QVERIFY(mutation != commands.end());
            QVERIFY(
                std::all_of(commands.begin(), mutation, [](const auto& command) { return command == "VERSIONA\r\n"; }));
            QVERIFY(receiver.sent("MODE ROVER"));
            QCOMPARE(receiver.model.role, "MODE BASE TIME");
            QVERIFY(receiver.sent("GPGGA 1") && receiver.sent("GPGST 1") && receiver.sent("GPGSV 1"));
            QVERIFY(!receiver.sent("SAVECONFIG") && !receiver.sent("FRESET") && receiver.sent("RTCM1074 1"));
            QCOMPARE(receiver.log.commands.back().outcome, GPSCommandOutcome::Acknowledged);
            QVERIFY(std::any_of(receiver.log.commands.begin(), receiver.log.commands.end(), [](const auto& result) {
                return result.command == "MODE" && result.outcome == GPSCommandOutcome::ReadbackVerified;
            }));
            driver.consume(correction());
            QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
        }
    }
    // Rates the baud search tried without an answer are not failures.
    QVERIFY(log.warnings().isEmpty());
}

void rejectBeforeMutation(GPSTestClock& clock)
{
    for (const auto* body : {
             "\"UM960\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982 PRO\",\"R4.10Build15434\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM980\",\"R4.10Build7922\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"R4.10Build7649\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"R5.00Build20000\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
             "\"UM982\",\"\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"",
         }) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.version = native("VERSIONA", body);
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        unsigned rate = 115200;
        QVERIFY(!driver.configure(baseConfig(false), rate));
        QVERIFY(!driver.receiverReady());
        QCOMPARE(receiver.commands(), QByteArrayList{"VERSIONA\r\n"});
        if (std::string_view(body).find("R5.00") != std::string_view::npos) {
            QVERIFY(driver.errorDetail().contains("Unsupported Unicore receiver"));
            QVERIFY(driver.errorDetail().contains("R5.00Build20000"));
            QVERIFY(driver.errorDetail().contains("R4.10Build7650"));
        }
    }
    // Unicore receivers have no accuracy-controlled survey-in.
    clock.reset();
    Receiver receiver(clock);
    GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
    auto config = baseConfig(false);
    config.base.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s};
    unsigned rate = 115200;
    QVERIFY(!driver.configure(config, rate));
    QCOMPARE(transportCalls(receiver), 0);
    QVERIFY(!driver.receiverReady());
}

void fixedBaseAndTransition(GPSTestClock& clock)
{
    clock.reset();
    Receiver receiver(clock);
    GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
    unsigned rate = 115200;
    QVERIFY(driver.configure(baseConfig(true), rate));
    QVERIFY(driver.receiverReady());
    QVERIFY(receiver.sent("MODE BASE 4315616."));
    QVERIFY(receiver.sent("BESTNAVXYZA") && receiver.sent("RTCM1005 1") && receiver.sent("RTCM1124 1"));
    QVERIFY(receiver.log.count<GPSSurveyReport>());
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
    QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 0);
    QVERIFY(!std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees - 47) < 1e-7);
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees - 8) < 1e-7);
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters - 500) < 0.01);
    QVERIFY(std::any_of(receiver.log.commands.begin(), receiver.log.commands.end(), [](const auto& result) {
        return result.command == "BESTNAVXYZA" && result.outcome == GPSCommandOutcome::ReadbackVerified;
    }));
    const auto calls = transportCalls(receiver);
    const auto frame = correction();
    for (const auto byte : frame) {
        driver.consume({&byte, 1});
    }
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QCOMPARE(transportCalls(receiver), calls);
    auto corrupt = frame;
    corrupt.back() ^= 1;
    driver.consume(corrupt);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QVERIFY(driver.configure(baseConfig(false), rate));
    QVERIFY(driver.receiverReady() && receiver.model.role == "MODE BASE TIME");
    driver.consume(frame);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
}

void averagingEvidenceAndRestart(GPSTestClock& clock)
{
    clock.reset();
    Receiver receiver(clock);
    GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
    unsigned rate = 115200;
    QVERIFY(driver.configure(baseConfig(false), rate));
    QVERIFY(receiver.sent("MODE BASE TIME 60 0"));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value() &&
            surveyDuration(receiver.log.latest<GPSSurveyReport>()) == 0);
    clock.advanceBy(7200000000ULL);
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    driver.consume(correction(position("FIXEDPOS", receiver.model.coordinates)));
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);

    // BASEPOS is a real-time monitoring solution, not evidence of a frozen average.
    consume(driver, native("BASEPOSA", "SOL_COMPUTED,SINGLE,47,8,450,50,WGS84,0.001,0.001,0.001"));
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    const auto calls = transportCalls(receiver);
    consume(driver, position("FIXEDPOS", receiver.model.coordinates));
    QCOMPARE(transportCalls(receiver), calls);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value() &&
            surveyDuration(receiver.log.latest<GPSSurveyReport>()) == 0);
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);

    // Loss of the receiver's fixed solution cannot silently resume with a previous average.
    consume(driver, position("SINGLE", receiver.model.coordinates, 378239000));
    QVERIFY(!driver.receiverReady());
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    consume(driver, position("FIXEDPOS", receiver.model.coordinates));
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QVERIFY(driver.configure(baseConfig(false), rate));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    QCOMPARE(receiver.commands().count("MODE BASE TIME 60 0\r\n"), 2);
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    consume(driver, position("FIXEDPOS", receiver.model.coordinates));
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 2);
}

void readbackFailures(GPSTestClock& clock)
{
    // The last variant reads back another role ahead of receiver averaging rather than a fixed base.
    for (unsigned variant = 0; variant < 7; ++variant) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.modeMismatch = variant == 0 || variant == 6;
        receiver.model.positionMismatch = variant == 1;
        if (variant == 2) {
            receiver.model.version[receiver.model.version.find("*") + 1] = 'Z';
        } else if (variant == 3) {
            receiver.model.version.clear();
        } else if (variant == 4) {
            receiver.model.omitPositionReadback = true;
        } else if (variant == 5) {
            receiver.model.omitModeReadback = true;
        }
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        unsigned rate = 115200;
        QVERIFY(!driver.configure(baseConfig(variant != 6), rate));
        QVERIFY(!driver.receiverReady());
        QCOMPARE(receiver.log.commands.back().outcome,
                 (variant < 2 || variant == 6 ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut));
        QVERIFY(!receiver.sent("RTCM"));
        if (variant == 4) {
            QCOMPARE(receiver.commands().back(), "BESTNAVXYZA\r\n");
        } else if (variant == 5) {
            QCOMPARE(receiver.commands().back(), "MODE\r\n");
        }
    }
}

void corruptStatusAndExpiry(GPSTestClock& clock)
{
    for (unsigned variant = 0; variant < 6; ++variant) {
        clock.reset();
        Receiver receiver(clock);
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        unsigned rate = 115200;
        QVERIFY(driver.configure(baseConfig(false), rate));
        std::string data = position("FIXEDPOS", receiver.model.coordinates);
        if (variant == 0) {
            data[data.find('*') + 1] = 'Z';
        } else if (variant == 1) {
            data[data.find("FIXEDPOS")] = 'X';
        } else if (variant == 2) {
            data = native("BESTNAVXYZA", "SOL_COMPUTED,FIXEDPOS,nan,1,2");
        } else if (variant == 3) {
            data = '#' + std::string(5000, 'A') + "\r\n";
        } else if (variant == 4) {
            data = native("BESTNAVXYZA", "SOL_COMPUTED,FIXEDPOS,6378137,0,0");
        } else {
            data = position("FIXEDPOS", receiver.model.coordinates, 604800000);
        }
        const auto calls = transportCalls(receiver);
        consume(driver, data);
        driver.consume(correction());
        QCOMPARE(transportCalls(receiver), calls);
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
        consume(driver, position("FIXEDPOS", receiver.model.coordinates, 378239000));
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
        clock.advanceBy(5000001);
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
        QVERIFY(!driver.receiverReady());
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    }
}

void restartAndReadErrors(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    const std::array<QString, 4> details{
        QStringLiteral("Unicore receiver reported its identity unasked; it may have restarted"),
        QStringLiteral("Unicore receiver left the configured base mode"),
        QStringLiteral("Unicore base status went back in time; the receiver may have restarted"),
        QStringLiteral("Unicore test disconnect"),
    };
    for (unsigned variant = 0; variant < 4; ++variant) {
        clock.reset();
        Receiver receiver(clock);
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        unsigned rate = 115200;
        QVERIFY(driver.configure(baseConfig(true), rate));
        if (variant == 0) {
            consume(driver, GPSTest::UNICORE_VERSION);
        } else if (variant == 1) {
            consume(driver, native("MODE", "MODE ROVER SURVEY,"));
        } else if (variant == 2) {
            consume(driver, position("FIXEDPOS", receiver.model.coordinates, 1000));
        } else {
            receiver.model.readError = true;
            QCOMPARE(driver.receive(100ms), GPSReceiveUpdates{});
            QCOMPARE(driver.error(), GPSProtocolError::Transport);
        }
        QCOMPARE(driver.errorDetail(), details[variant]);
        QVERIFY(!driver.receiverReady());
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    }
    QVERIFY(!log.warnings().isEmpty());
}

void measurementFreshnessAndRollover(GPSTestClock& clock)
{
    for (const bool rollover : {false, true}) {
        clock.reset();
        Receiver receiver(clock);
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        unsigned baud = 115200;
        QVERIFY(driver.configure(baseConfig(false), baud));
        const auto complete = GPSTest::unicorePosition("FIXEDPOS", receiver.model.coordinates, 604799000, 2326);
        consume(driver, complete, 1);
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
        QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
        QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 0);
        const auto reports = receiver.log.count<GPSSurveyReport>();
        const auto receipt = receiver.log.latest<GPSSurveyReport>().timestampUs;
        clock.advanceBy(4000000);
        consume(driver, complete);
        QCOMPARE(receiver.log.count<GPSSurveyReport>(), reports);
        QCOMPARE(receiver.log.latest<GPSSurveyReport>().timestampUs, receipt);
        if (rollover) {
            consume(driver, GPSTest::unicorePosition("FIXEDPOS", receiver.model.coordinates, 0, 2327));
            QVERIFY(driver.receiverReady());
            QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
            QVERIFY(receiver.log.latest<GPSSurveyReport>().timestampUs > receipt);
            driver.consume(correction());
            QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
            consume(driver, complete);  // Previous week cannot revive/replace the new week's evidence.
        } else {
            clock.advanceBy(1000001);
            driver.consume({});
        }
        QVERIFY(!driver.receiverReady());
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
        QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
        QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 0);
        QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees));
        QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees));
        QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
        consume(driver, complete);
        const auto count = receiver.log.count<GPSRTCMFrame>();
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), count);
    }
}

void scheduledAveragingAndBoot(GPSTestClock& clock)
{
    for (const size_t chunk : {size_t(1), size_t(150)}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.chunk = chunk;
        receiver.model.initialTow = 604798000;
        GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
        auto config = baseConfig(false);
        std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDuration = 1s;
        unsigned baud = 115200;
        QVERIFY(driver.configure(config, baud));
        const auto commands = receiver.commands().size();
        for (unsigned slices = 0; clock.nowUs() < 2500000; ++slices) {
            QVERIFY(slices < 100);
            driver.receive(1000ms);
            QVERIFY(driver.receiverReady());
        }
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
        QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 0);
        QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
        QCOMPARE(receiver.commands().size(), commands);
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
        receiver.model.events.schedule(500000, [&receiver] { receiver.model.boot(); });
        for (unsigned slices = 0; driver.receiverReady(); ++slices) {
            QVERIFY(slices < 100);
            driver.receive(1000ms);
        }
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
        QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
        driver.consume(correction());
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    }
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"identity-and-role", identityAndRole},
    {"reject-before-mutation", rejectBeforeMutation},
    {"fixed-base-and-transition", fixedBaseAndTransition},
    {"averaging-evidence-and-restart", averagingEvidenceAndRestart},
    {"readback-failures", readbackFailures},
    {"corrupt-status-and-expiry", corruptStatusAndExpiry},
    {"restart-and-read-errors", restartAndReadErrors},
    {"measurement-freshness-and-rollover", measurementFreshnessAndRollover},
    {"scheduled-averaging-and-boot", scheduledAveragingAndBoot},
};
}  // namespace

void UnicoreProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void UnicoreProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

void UnicoreProtocolTest::_commandFailures_data()
{
    QTest::addColumn<QByteArray>("command");
    QTest::addColumn<int>("fault");
    QTest::addColumn<GPSCommandOutcome>("outcome");

    const struct
    {
        const char* name;
        CommandFault fault;
        GPSCommandOutcome outcome;
    } faults[] = {
        {"silence", CommandFault::Silence, GPSCommandOutcome::TimedOut},
        {"reject", CommandFault::Reject, GPSCommandOutcome::Rejected},
        {"wrong-ack", CommandFault::WrongAck, GPSCommandOutcome::TimedOut},
        {"corrupt", CommandFault::Corrupt, GPSCommandOutcome::TimedOut},
        {"cancel", CommandFault::Cancel, GPSCommandOutcome::Cancelled},
        {"write-error", CommandFault::WriteError, GPSCommandOutcome::TransportError},
        {"short-write", CommandFault::ShortWrite, GPSCommandOutcome::TransportError},
    };

    for (const char* command : {"VERSIONA", "MODE BASE TIME"}) {
        for (const auto& [name, fault, outcome] : faults) {
            QTest::addRow("%s-%s", command, name) << QByteArray(command) << static_cast<int>(fault) << outcome;
        }
    }
    // Base output arrives while the acknowledgement of a later output command is awaited.
    QTest::newRow("RTCM1005 1-silence") << QByteArray("RTCM1005 1") << static_cast<int>(CommandFault::Silence)
                                        << GPSCommandOutcome::TimedOut;
}

void UnicoreProtocolTest::_commandFailures()
{
    QFETCH(QByteArray, command);
    QFETCH(int, fault);
    QFETCH(GPSCommandOutcome, outcome);
    GPSTestClock clock;
    Receiver receiver(clock);
    switch (static_cast<CommandFault>(fault)) {
        case CommandFault::Silence:
            receiver.faults.rules.push_back(silence(command));
            break;
        case CommandFault::Reject:
            receiver.model.fault = UnicoreReceiverModel::Fault::Reject;
            break;
        case CommandFault::WrongAck:
            receiver.model.fault = UnicoreReceiverModel::Fault::WrongAck;
            break;
        case CommandFault::Corrupt:
            receiver.model.fault = UnicoreReceiverModel::Fault::Corrupt;
            break;
        case CommandFault::Cancel: {
            auto rule = silence(command);
            rule.after = [&receiver](ScriptedReceiver&) { receiver.model.cancel = true; };
            receiver.faults.rules.push_back(std::move(rule));
            break;
        }
        case CommandFault::WriteError:
            receiver.faults.rules.push_back(writeFails(command, [](const QByteArray&) {
                return GPSWriteResult{GPSWriteStatus::Error, 0, 0, QStringLiteral("Unicore test write failure")};
            }));
            break;
        case CommandFault::ShortWrite:
            receiver.faults.rules.push_back(writeFails(command, [](const QByteArray& written) {
                const int length = static_cast<int>(written.size()) - 1;
                return GPSWriteResult{GPSWriteStatus::Completed, length, length};
            }));
            break;
    }
    receiver.model.faultCommand = command.toStdString();
    const GPSProtocolLogCapture log;
    GPSProtocolRuntime driver = receiver.runtime(Unicore::FAMILY);
    unsigned rate = 115200;
    QVERIFY(!driver.configure(baseConfig(false), rate));
    QVERIFY(!driver.receiverReady());
    QVERIFY(receiver.commands().back().startsWith(command));
    QVERIFY(!receiver.log.commands.empty());
    QCOMPARE(receiver.log.commands.back().outcome, outcome);
    QVERIFY(receiver.log.commands.back().required);
    if (fault == static_cast<int>(CommandFault::Cancel)) {
        QCOMPARE(driver.error(), GPSProtocolError::Cancelled);
        QVERIFY(log.warnings().isEmpty());
    }
    if (fault == static_cast<int>(CommandFault::WriteError)) {
        QCOMPARE(driver.errorDetail(), QStringLiteral("Unicore test write failure"));
    }
    if (fault == static_cast<int>(CommandFault::ShortWrite)) {
        QVERIFY(receiver.log.commands.back().writtenBytes > 0);
        QCOMPARE(receiver.log.commands.back().acceptedBytes, receiver.log.commands.back().writtenBytes);
    }
    QVERIFY(clock.nowUs() < 45000000);
    driver.consume(correction());
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
}

UT_REGISTER_TEST_LIGHTWEIGHT(UnicoreProtocolTest, TestLabel::Unit)
