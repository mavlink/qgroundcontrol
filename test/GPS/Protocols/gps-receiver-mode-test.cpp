#include <algorithm>
#include <chrono>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "Femto/FemtoFamily.h"
#include "Femto/FemtoPlan.h"
#include "GPSCommandChannel.h"
#include "GPSEllipsoidPosition.h"
#include "GPSEventSink.h"
#include "GPSProtocolRuntime.h"
#include "GPSRawAckMatcher.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "Support/ProtocolTestPackets.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
class Receiver
{
public:
    explicit Receiver(GPSTestClock& testClock)
        : _clock(testClock)
    {}

    std::string rejected_command;
    bool cancel_read = false;
    size_t read_chunk = 7;
    size_t noise_bytes = 0;
    std::string interleave_on;
    std::string interleaved;
    unsigned failed_reads = 0;
    size_t transport_calls = 0;
    std::vector<std::string> commands;

    bool sent(const std::string& prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [&](const auto& command) { return command.compare(0, prefix.size(), prefix) == 0; });
    }

private:
    std::string reply;
    bool rejected = false;
    GPSTestClock& _clock;

public:
    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(_clock);
        result.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++transport_calls;
            const auto timeout = deadline.remaining(_clock.nowUs());
            auto* data = bytes.data();
            const int size = static_cast<int>(bytes.size());
            CHECK(timeout >= 0ms);
            _clock.advanceBy(1000);
            if (rejected && cancel_read) {
                ++failed_reads;
                return {GPSReadStatus::Cancelled};
            }
            if (reply.empty()) {
                _clock.advanceBy(static_cast<uint64_t>(std::chrono::microseconds(timeout).count()) + 1);
                return {GPSReadStatus::TimedOut};
            }
            // Femtomes does not guarantee that an ACK fits in one read or includes a NUL terminator.
            const size_t count = std::min({reply.size(), size_t(size), read_chunk});
            memcpy(data, reply.data(), count);
            reply.erase(0, count);
            _clock.advanceBy(1000);
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        result.write = [this](std::span<const uint8_t> input, GPSDeadline) -> GPSWriteResult {
            ++transport_calls;
            const auto* data = input.data();
            const int size = static_cast<int>(input.size());

            const std::string command(reinterpret_cast<const char*>(data), size);
            commands.push_back(command);
            rejected = !rejected_command.empty() && command.compare(0, rejected_command.size(), rejected_command) == 0;
            if (rejected) {
                reply = "<ERROR\r\n";
            } else {
                reply = '<' + command.substr(0, command.find_first_of(" \r\n")) + " OK";
                reply.insert(0, noise_bytes, '\0');
            }
            if (!interleave_on.empty() && command.starts_with(interleave_on)) {
                reply.insert(0, interleaved);
                interleave_on.clear();
            }
            return {GPSWriteStatus::Completed, size, size};
        };
        result.setBaudrate = [this](unsigned) {
            ++transport_calls;
            return GPSBaudStatus::Configured;
        };
        return result;
    }
};

static void rawAcknowledgements()
{
    for (const auto& patterns : {std::pair{"<LOG OK", "<ERROR"}, std::pair{"$R: set", "$R?"}}) {
        for (size_t chunk : {1u, 3u, 150u}) {
            GPSRawAckMatcher matcher(patterns.first, patterns.second);
            CHECK(matcher.valid());
            const std::string bytes = std::string(600, '\0') + patterns.first;
            for (size_t offset = 0; offset < bytes.size(); offset += chunk) {
                const size_t count = std::min(chunk, bytes.size() - offset);
                matcher.append({reinterpret_cast<const uint8_t*>(bytes.data() + offset), count});
            }
            CHECK(matcher.outcome() == GPSCommandOutcome::Acknowledged);
        }
        for (bool negativeFirst : {false, true}) {
            GPSRawAckMatcher matcher(patterns.first, patterns.second);
            const std::string bytes = negativeFirst ? std::string(patterns.second) + '\0' + patterns.first
                                                    : std::string(patterns.first) + '\0' + patterns.second;
            matcher.append({reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()});
            CHECK(matcher.outcome() == GPSCommandOutcome::Rejected);
        }
    }
    GPSRawAckMatcher empty("", "");
    CHECK(!empty.valid());
    const std::string overlong(257, 'A');
    GPSRawAckMatcher oversized(overlong, "");
    CHECK(!oversized.valid());
}

static void receiverMode(GPSTestClock& clock, bool fixed, const std::string& rejected_command = {},
                         bool cancel_read = false, size_t read_chunk = 7, size_t noise_bytes = 0)
{
    clock.reset();
    const GPSProtocolLogCapture log;
    Receiver receiver(clock);
    receiver.rejected_command = rejected_command;
    receiver.cancel_read = cancel_read;
    receiver.read_chunk = read_chunk;
    receiver.noise_bytes = noise_bytes;
    GPSProtocolRuntime driver(Femto::FAMILY, receiver.io());
    GPSConfig config{};
    config.base = {
        .mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                            .accuracyMeters = 1.0f}}
                      : GPSBaseStationConfig::Mode{
                            GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.25, .duration = 60s}}};
    unsigned baudrate = 115200;
    const bool configured = driver.configure(config, baudrate);
    if (!rejected_command.empty()) {
        CHECK(!configured);
        // A receiver whose base setup was rejected must not take injected corrections.
        CHECK(!driver.receiverReady());
        CHECK(receiver.sent(rejected_command));
        CHECK(!receiver.sent("LOG UAVGPSB"));
        if (cancel_read) {
            CHECK(log.warnings().empty());
            CHECK(receiver.failed_reads == 1);
        }
        return;
    }
    CHECK(configured);
    CHECK(log.warnings().empty());
    CHECK(!receiver.sent("POSAVE OFF"));
    CHECK(!receiver.sent("FIX NONE"));
    CHECK(!receiver.sent("LOG UAVGPSB"));
    CHECK(receiver.sent("FIX POSITION 47.00000000 8.00000000") == fixed);
    CHECK(receiver.sent("POSAVE ON") == !fixed);
    config.base = {};
    const auto calls = receiver.transport_calls;
    CHECK(!driver.configure(config, baudrate));
    CHECK(!driver.receiverReady());
    CHECK(receiver.transport_calls == calls);
    CHECK(log.warnings().size() == 1 &&
          log.warnings().front().startsWith(QStringLiteral("Invalid receiver physical configuration")));
    CHECK(log.categories() == QStringList{QStringLiteral("GPS.Driver.Protocols.Femto")});
}

/// Coordinates are formatted as printf does: exact ties round to even, and negative zero keeps its sign.
void fixedPositionText()
{
    const auto plan = Femto::Plan::fixedBase(
        {.latitudeDegrees = 47.001953125, .longitudeDegrees = -0.0, .altitudeMeters = 100.015625f});
    CHECK(plan.steps.size() == 2);
    const auto& position = std::get<GPSCommandSequence::Command>(plan.steps.front());
    CHECK(position.wire == "FIX POSITION 47.00195312 -0.00000000 100.01562\r\n");
    // The label never carries the coordinates.
    CHECK(position.step.command == "FIX POSITION (fixed position)");
    CHECK(std::get<GPSCommandSequence::Command>(plan.steps.back()).wire == "LOG GPGGA 1 \r\n");
}

void baseMixedFraming(GPSTestClock& clock)
{
    Receiver peer(clock);
    std::vector<std::vector<uint8_t>> frames;
    size_t otherReports = 0;
    size_t largestBatch = 0;
    GPSRuntimeObserver observer;
    observer.decoded = [&](const GPSEventBatch& batch) {
        largestBatch = std::max(largestBatch, batch.events.size());
        for (const auto& event : batch.events) {
            if (const auto* frame = std::get_if<GPSRTCMFrame>(&event)) {
                frames.emplace_back(frame->bytes.begin(), frame->bytes.end());
            } else {
                ++otherReports;
            }
        }
    };
    GPSProtocolRuntime driver(Femto::FAMILY, peer.io(), std::move(observer), {.satelliteInfoEnabled = false});
    GPSConfig config;
    config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}};
    unsigned baud = 115200;
    CHECK(driver.configure(config, baud));
    (void) driver.consume({});
    otherReports = 0;
    const std::string body = "GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,";
    const auto text = nmeaSentence(body);
    const auto binary = rtcmPacket({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
    (void) driver.consume(binary);
    CHECK(frames == std::vector<std::vector<uint8_t>>{binary});
    CHECK(otherReports == 0);
    verifyRTCMRecovery(driver, frames);
    CHECK(otherReports == 0);
    CHECK(largestBatch <= GPSEventSink::MAX_EVENTS);
}

void configurationDecodesInterleavedTraffic(GPSTestClock& clock)
{
    {
        Receiver peer(clock);
        peer.interleave_on = "UNLOGALL";
        peer.interleaved = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,");
        size_t usageReports = 0;
        std::vector<GPSDecodedSurvey> surveys;
        GPSRuntimeObserver observer;
        observer.decoded = [&](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                usageReports += std::holds_alternative<GPSDecodedSatelliteUsage>(event);
                if (const auto* survey = std::get_if<GPSDecodedSurvey>(&event)) {
                    surveys.push_back(*survey);
                }
            }
        };
        GPSProtocolRuntime driver(Femto::FAMILY, peer.io(), std::move(observer));
        GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}};
        unsigned baud = 115200;
        CHECK(driver.configure(config, baud));
        CHECK(usageReports == 1);
        (void) driver.receive(10ms);
        // The fixed-quality GGA predates this configuration's survey, so it must not complete it.
        CHECK(!peer.sent("LOG RTCM"));
        CHECK(std::none_of(surveys.begin(), surveys.end(), [](const auto& survey) { return survey.survey.valid; }));
    }
}

}  // namespace

class GPSProtocolReceiverModesTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolReceiverModesTest::_protocol()
{
    GPSTestClock clock;
    try {
        rawAcknowledgements();
        for (bool fixed : {false, true}) {
            receiverMode(clock, fixed);
        }
        for (bool cancelRead : {false, true}) {
            receiverMode(clock, false, "POSAVE ON", cancelRead);
            receiverMode(clock, false, "LOG GPGGA", cancelRead);
            receiverMode(clock, true, "FIX POSITION", cancelRead);
            receiverMode(clock, true, "LOG RTCM", cancelRead);
        }
        fixedPositionText();
        baseMixedFraming(clock);
        configurationDecodesInterleavedTraffic(clock);
        receiverMode(clock, true, {}, false, 1);
        receiverMode(clock, true, {}, false, GPSCommandChannel::READ_CHUNK_SIZE,
                     2 * GPSCommandChannel::READ_CHUNK_SIZE - 3);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolReceiverModesTest, TestLabel::Unit)

#include "gps-receiver-mode-test.moc"
