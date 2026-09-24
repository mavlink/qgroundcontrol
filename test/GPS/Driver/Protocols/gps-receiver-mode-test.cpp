#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolTestIO.h"
#include "GPSRawAckMatcher.h"
#include "LittleEndian.h"
#include "ProtocolTestPackets.h"
#include "RTCMFramer.h"
#include "SBF/GPSDriverSBF.h"
#include "UnitTest.h"

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
int surveyFlags(const GPSNativeSurveyReport& report)
{
    return (report.survey.valid ? 1 : 0) | (report.survey.active ? 2 : 0);
}

uint32_t surveyDuration(const GPSNativeSurveyReport& report)
{
    return static_cast<uint32_t>(report.survey.duration.count());
}

class Receiver
{
public:
    bool septentrio = false;
    std::string port = "USB1";
    std::string rejected_command;
    unsigned reject_after = 0;
    unsigned reject_attempts = UINT_MAX;
    unsigned matched_commands = 0;
    bool unterminated_reply = false;
    bool conflicting_reply = false;
    bool cancel_read = false;
    bool silence_rejected = false;
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

public:
    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO();
        result.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++transport_calls;
            const int timeout = deadline.remainingMilliseconds(gps_test_time);
            auto* data = bytes.data();
            const int size = static_cast<int>(bytes.size());
            CHECK(timeout >= 0);
            gps_test_time += 1000;
            if (rejected && cancel_read) {
                ++failed_reads;
                return {GPSReadStatus::Cancelled};
            }
            if (reply.empty()) {
                gps_test_time += uint64_t(timeout) * 1000 + 1;
                return {GPSReadStatus::TimedOut};
            }
            // Neither protocol guarantees that an ACK fits in one read or includes a NUL terminator.
            const size_t count = std::min({reply.size(), size_t(size), read_chunk});
            memcpy(data, reply.data(), count);
            reply.erase(0, count);
            gps_test_time += 1000;
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        result.write = [this](std::span<const uint8_t> input, GPSDeadline) -> GPSWriteResult {
            ++transport_calls;
            const auto* data = input.data();
            const int size = static_cast<int>(input.size());

            const std::string command(reinterpret_cast<const char*>(data), size);
            commands.push_back(command);
            const bool matches =
                !rejected_command.empty() && command.compare(0, rejected_command.size(), rejected_command) == 0;
            if (matches) {
                ++matched_commands;
            }
            rejected = matches && matched_commands > reject_after && matched_commands - reject_after <= reject_attempts;
            if (rejected) {
                reply = silence_rejected ? std::string{} : septentrio ? "$R? rejected\n" : "<ERROR\r\n";
                if (conflicting_reply && !silence_rejected) {
                    reply += septentrio ? "$R: " + command : '<' + command.substr(0, command.find(' ')) + " OK";
                }
            } else if (septentrio) {
                reply = command == "\n\r" ? port + ">" : "$R: " + command;
            } else {
                reply = '<' + command.substr(0, command.find_first_of(" \r\n")) + " OK";
                reply.insert(0, noise_bytes, '\0');
            }
            if (unterminated_reply) {
                while (reply.ends_with('\r') || reply.ends_with('\n')) {
                    reply.pop_back();
                }
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

static void receiverMode(bool septentrio, bool fixed, const std::string& rejected_command = {},
                         bool cancel_read = false, size_t read_chunk = 7, size_t noise_bytes = 0)
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    Receiver receiver;
    receiver.septentrio = septentrio;
    receiver.rejected_command = rejected_command;
    receiver.cancel_read = cancel_read;
    receiver.read_chunk = read_chunk;
    receiver.noise_bytes = noise_bytes;
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    std::unique_ptr<GPSProtocol> driver;
    if (septentrio) {
        driver = std::make_unique<GPSNativeSBF>(captureGPSReports(receiver.io(), position, &satellites));
    } else {
        driver = std::make_unique<GPSNativeFemto>(captureGPSReports(receiver.io(), position, &satellites));
    }
    CHECK(driver);
    GPSProtocol::GPSConfig config{};
    config.base = {
        .mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                            .accuracyMeters = 1.0f}}
                      : GPSBaseStationConfig::Mode{
                            GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.25, .durationSecs = 60}}};
    unsigned baudrate = 115200;
    const bool configured = driver->configure(baudrate, config);
    if (!rejected_command.empty()) {
        CHECK(!configured);
        CHECK(receiver.sent(rejected_command));
        CHECK(!receiver.sent(septentrio ? "setSBFOutput, Stream1, USB1, PVTGeodetic" : "LOG UAVGPSB"));
        if (cancel_read) {
            CHECK(gps_test_warnings.empty());
            CHECK(receiver.failed_reads == 1);
        }
        return;
    }
    CHECK(configured);
    CHECK(gps_test_warnings.empty());
    if (septentrio) {
        CHECK(!receiver.sent("setAttitudeOffset, 0.000, 0.000"));
        CHECK(!receiver.sent("setPVTMode, Rover, All, auto"));
        CHECK(receiver.sent("setPVTMode, Static"));
        CHECK(receiver.sent("setDataInOut, USB1, Auto, RTCMv3+SBF"));
        CHECK(receiver.sent("setStaticPosGeodetic") == fixed);
    } else {
        CHECK(!receiver.sent("POSAVE OFF"));
        CHECK(!receiver.sent("FIX NONE"));
        CHECK(!receiver.sent("LOG UAVGPSB"));
        CHECK(receiver.sent("FIX POSITION 47.00000000 8.00000000") == fixed);
        CHECK(receiver.sent("POSAVE ON") == !fixed);
    }
    config.base = {};
    const auto calls = receiver.transport_calls;
    CHECK(!driver->configure(baudrate, config));
    CHECK(!driver->receiverReady());
    CHECK(receiver.transport_calls == calls);
}

void sbfConfirmationPolicy()
{
    for (unsigned failures : {0u, 1u, 4u, 5u}) {
        for (bool firstFails : {false, true}) {
            Receiver receiver;
            receiver.septentrio = true;
            receiver.unterminated_reply = true;
            receiver.reject_after = firstFails ? 0 : 1;
            receiver.reject_attempts = failures;
            const std::string dataIO = "setDataInOut, USB1, Auto, SBF\n";
            receiver.rejected_command = dataIO;
            GPSNativeSBF driver(receiver.io(), false);
            GPSProtocol::GPSConfig config;
            config.base.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60};
            unsigned baud = 115200;
            const bool success = driver.configure(baud, config);
            CHECK(success == (firstFails ? failures == 0 : failures < 5));
            std::vector<std::string> expected{"SSSSSSSSSS\n",
                                              "setDataInOut,COM1,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "setDataInOut,COM2,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "setDataInOut,USB1,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "setDataInOut,USB2,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "setDataInOut,USB3,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "setDataInOut,USB4,,-RTCMv3-RTCMv2-CMRv2\n",
                                              "\n\r",
                                              "setSBFOutput, Stream1, USB1, none, off\n",
                                              dataIO};
            if (!(firstFails && failures)) {
                expected.emplace_back("setGeodeticDatum, WGS84\n");
                expected.insert(expected.end(), std::min(failures + 1, 5u), dataIO);
                if (success) {
                    expected.emplace_back("setDataInOut, USB1, Auto, RTCMv3+SBF\n");
                    expected.emplace_back("setPVTMode, Static, All, auto\n");
                    expected.emplace_back("setSBFOutput, Stream1, USB1, +PVTGeodetic, msec500\n");
                }
            }
            CHECK(receiver.commands == expected);
        }
    }
    Receiver receiver;
    receiver.septentrio = true;
    receiver.rejected_command = "setGeodeticDatum";
    receiver.conflicting_reply = true;
    receiver.read_chunk = GPS_READ_BUFFER_SIZE;
    GPSNativeSBF driver(receiver.io());
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config;
    config.base.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60};
    CHECK(!driver.configure(baud, config));
    CHECK(receiver.commands.back() == "setGeodeticDatum, WGS84\n");
}

void sbfRequiredBaseCommands()
{
    const std::vector<std::string> fixedCommands = {
        "setGeodeticDatum, WGS84",
        "setDataInOut, USB1, Auto, RTCMv3+SBF",
        "setStaticPosGeodetic",
        "setAntennaOffset",
        "setReceiverDynamics, Low, Static",
        "setPVTMode, Static",
        "setSBFOutput, Stream1, USB1, +PVTGeodetic",
    };
    for (bool fixed : {false, true}) {
        const auto commands =
            fixed ? fixedCommands
                  : std::vector<std::string>{"setGeodeticDatum, WGS84", "setDataInOut, USB1, Auto, RTCMv3+SBF",
                                             "setPVTMode, Static", "setSBFOutput, Stream1, USB1, +PVTGeodetic"};
        for (const auto& command : commands) {
            for (bool silent : {false, true}) {
                gps_test_time = 0;
                Receiver receiver;
                receiver.septentrio = true;
                receiver.rejected_command = command;
                receiver.silence_rejected = silent;
                GPSNativePositionReport position;
                GPSNativeSatelliteReport satellites;
                std::vector<GPSCommandResult> results;
                auto io = receiver.io();
                io.commandFinished = [&](const GPSCommandResult& result) { results.push_back(result); };
                GPSNativeSBF driver(captureGPSReports(io, position, &satellites));
                GPSProtocol::GPSConfig config{};
                config.base = {
                    .mode =
                        fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                                    .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                              : GPSBaseStationConfig::Mode{
                                    GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}}};
                unsigned baudrate = 115200;
                CHECK(!driver.configure(baudrate, config));
                CHECK(!driver.receiverReady());
                CHECK(receiver.sent(command));
                CHECK(receiver.commands.back().starts_with(command));
                CHECK(!results.empty());
                CHECK(results.back().evidence.command.starts_with(command));
                CHECK(results.back().evidence.required);
                CHECK(results.back().evidence.outcome ==
                      (silent ? GPSCommandOutcome::TimedOut : GPSCommandOutcome::Rejected));
                CHECK(results.back().evidence.acceptedBytes == int(receiver.commands.back().size()));
                CHECK(results.back().evidence.writtenBytes == int(receiver.commands.back().size()));
            }
        }
    }
}

void sbfSelectedPortAndPrecision()
{
    for (const auto* port : {"COM1", "USB2", "IP10", "IPS1"}) {
        Receiver peer;
        peer.septentrio = true;
        peer.port = port;
        GPSNativePositionReport position;
        GPSNativeSBF driver(captureGPSReports(peer.io(), position), false);
        GPSProtocol::GPSConfig config;
        config.base.mode = GPSBaseStationConfig::Fixed{};
        std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position = {47.397742491, -8.545593291, 500.125f};
        unsigned baud = 115200;
        CHECK(driver.configure(baud, config));
        CHECK(peer.sent(std::string("setDataInOut, ") + port + ", Auto, RTCMv3+SBF"));
        CHECK(peer.sent(std::string("setSBFOutput, Stream1, ") + port + ", +PVTGeodetic"));
        CHECK(peer.sent("setStaticPosGeodetic, Geodetic1, 47.397742491, -8.545593291, 500.1250, WGS84"));
        CHECK(peer.sent("setCOMSettings") == std::string_view(port).starts_with("COM"));
    }
}

void sbfDatumRejection()
{
    for (uint8_t datum : {19, 31, 250}) {
        Receiver peer;
        peer.septentrio = true;
        GPSNativePositionReport position;
        std::vector<GPSNativeSurveyReport> surveys;
        auto io = peer.io();
        io.decoded = [&](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*report);
                }
            }
        };
        GPSNativeSBF driver(captureGPSReports(io, position), false);
        GPSProtocol::GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
        unsigned baud = 115200;
        CHECK(driver.configure(baud, config));
        std::vector<uint8_t> frame(96);
        (void) LittleEndian::write<uint16_t>(frame, 0, 0x4024);
        (void) LittleEndian::write<uint16_t>(frame, 4, SBF_ID_PVTGeodetic);
        (void) LittleEndian::write<uint16_t>(frame, 6, uint16_t(frame.size()));
        (void) LittleEndian::write<uint16_t>(frame, 12, 2435);
        frame[14] = 3;
        frame[73] = datum;
        (void) LittleEndian::write<double>(frame, 16, 0.5);
        (void) LittleEndian::write<double>(frame, 24, 1);
        (void) LittleEndian::write<double>(frame, 32, 500);
        (void) LittleEndian::write<uint16_t>(frame, 2, crc16(frame.data() + 4, frame.size() - 4));
        driver.consume(frame);
        CHECK(driver.ioError() == GPSProtocolError::Protocol);
        CHECK(driver.ioErrorDetail().contains("datum"));
        CHECK(!driver.receiverReady());
        CHECK(surveys.size() == 1 && surveyFlags(surveys.back()) == 0);
        CHECK(std::isnan(surveys.back().survey.position.latitudeDegrees));
    }
}

void sbfFrameOwnership()
{
    Receiver receiver;
    receiver.septentrio = true;
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    size_t correctionCount = 0;
    size_t fixCount = 0;
    auto io = receiver.io();
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            correctionCount += std::holds_alternative<GPSRTCMReport>(event);
            fixCount += std::holds_alternative<GPSNativePositionReport>(event);
        }
    };
    GPSNativeSBF driver(captureGPSReports(io, position, &satellites));
    GPSProtocol::GPSConfig config{};
    config.base = {
        .mode = GPSBaseStationConfig::Fixed{
            .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}, .accuracyMeters = 1}};
    unsigned baudrate = 115200;
    CHECK(driver.configure(baudrate, config));
    std::vector<uint8_t> correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto checksum = RTCMFramer::crc24q(correction);
    correction.push_back(checksum >> 16);
    correction.push_back(checksum >> 8);
    correction.push_back(checksum);
    CHECK(RTCMFramer::isValidFrame(std::span<const uint8_t>(correction)));
    std::vector<uint8_t> native(94);
    (void) LittleEndian::write<uint16_t>(native, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(native, 4, SBF_ID_PVTGeodetic);
    (void) LittleEndian::write<uint16_t>(native, 6, native.size());
    (void) LittleEndian::write<uint16_t>(native, 12, 2435);
    native[14] = 1;
    (void) LittleEndian::write<double>(native, 16, 0.5);
    (void) LittleEndian::write<double>(native, 24, 1);
    // Both checksums are valid; only the outer native frame may own these bytes.
    std::copy(correction.begin(), correction.end(), native.begin() + 60);
    (void) LittleEndian::write<uint16_t>(native, 2, crc16(native.data() + 4, native.size() - 4));
    driver.consume(native);
    CHECK(correctionCount == 0);
    gps_test_time += 200000;
    driver.consume({});
    CHECK(fixCount == 1);
    CHECK(std::abs(position.navigation.latitudeDegrees - 0.5 * GPS_RAD_TO_DEG) < 1e-6);
    driver.consume(correction);
    CHECK(correctionCount == 1);
    CHECK(fixCount == 1);
}

void sbfSurveyEvidence()
{
    gps_test_time = 1000000;
    Receiver receiver;
    receiver.septentrio = true;
    GPSNativePositionReport position;
    std::vector<GPSNativeSurveyReport> surveys;
    auto io = receiver.io();
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                surveys.push_back(*survey);
            }
        }
    };
    GPSNativeSBF driver(captureGPSReports(io, position), false);
    GPSProtocol::GPSConfig config{};
    config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
    unsigned baudrate = 115200;
    uint32_t tow = 0;
    std::vector<uint8_t> frame(94);
    (void) LittleEndian::write<uint16_t>(frame, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(frame, 4, SBF_ID_PVTGeodetic);
    (void) LittleEndian::write<uint16_t>(frame, 6, uint16_t(frame.size()));
    (void) LittleEndian::write<uint16_t>(frame, 12, 2435);
    (void) LittleEndian::write<double>(frame, 16, 0.5);
    (void) LittleEndian::write<double>(frame, 24, 1);
    (void) LittleEndian::write<double>(frame, 32, 500);
    auto publish = [&](uint8_t mode, uint8_t flags, uint16_t horizontal = 200, uint16_t vertical = 200) {
        gps_test_time += 1000000;
        tow += 1000;
        frame[14] = mode;
        (void) LittleEndian::write<uint32_t>(frame, 8, tow);
        (void) LittleEndian::write<uint16_t>(frame, 90, horizontal);
        (void) LittleEndian::write<uint16_t>(frame, 92, vertical);
        (void) LittleEndian::write<uint16_t>(frame, 2, crc16(frame.data() + 4, frame.size() - 4));
        surveys.clear();
        driver.consume(frame);
        CHECK(surveys.size() == 1);
        CHECK(surveyFlags(surveys.back()) == flags);
        CHECK(!surveys.back().survey.meanAccuracyMeters.has_value());
        gps_test_time += 200000;
        driver.consume({});
        if (horizontal == UINT16_MAX) {
            CHECK(std::isnan(position.navigation.horizontalAccuracyMeters));
        } else {
            CHECK(position.navigation.horizontalAccuracyMeters == horizontal / 200.0f);
        }
        if (vertical == UINT16_MAX) {
            CHECK(std::isnan(position.navigation.verticalAccuracyMeters));
        } else {
            CHECK(position.navigation.verticalAccuracyMeters == vertical / 200.0f);
        }
        return surveyDuration(surveys.back());
    };

    for (bool fixed : {false, true, false}) {
        config.base.mode =
            fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                        .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                  : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
        CHECK(driver.configure(baudrate, config));
        CHECK(driver.receiverReady());
        // A standalone solution without the determination flag is not a completed fixed base.
        CHECK(publish(1, 0) == 0);
        const auto progress = publish(0x41, fixed ? 0 : 2);
        CHECK(fixed ? progress == 0 : progress > 0);
        const auto completed = publish(3, 1);
        CHECK(fixed ? completed == 0 : completed >= progress);
        CHECK(std::abs(surveys.back().survey.position.latitudeDegrees - 0.5 * GPS_RAD_TO_DEG) < 1e-6);
        CHECK(surveys.back().survey.position.altitudeMeters == 500);
        CHECK(publish(3, 1, 0, 0) == completed);
        CHECK(publish(3, 1, UINT16_MAX, UINT16_MAX) == completed);
        CHECK(publish(3, 1, UINT16_MAX, 200) == completed);
        CHECK(publish(3, 1, 200, UINT16_MAX) == completed);

        for (uint8_t mode : {0, 1, 2, 4, 9, 15, 0x83}) {
            CHECK(publish(mode, 0) == completed);
        }
        frame[15] = 9;
        CHECK(publish(3, 0) == completed);
        CHECK(std::isnan(surveys.back().survey.position.latitudeDegrees));
        const auto failedProgress = publish(0x41, fixed ? 0 : 2);
        CHECK(fixed ? failedProgress == 0 : failedProgress > completed);
        frame[15] = 0;
        publish(3, 1);

        for (size_t offset : {16, 24, 32}) {
            const double original = LittleEndian::read<double>(frame, offset).value();
            for (double invalid : {double(NAN), double(INFINITY), -2e10}) {
                (void) LittleEndian::write<double>(frame, offset, invalid);
                publish(3, 0);
                CHECK(std::isnan(surveys.back().survey.position.latitudeDegrees));
                CHECK(std::isnan(surveys.back().survey.position.longitudeDegrees));
                CHECK(std::isnan(surveys.back().survey.position.altitudeMeters));
            }
            (void) LittleEndian::write<double>(frame, offset, original);
        }
        publish(3, 1);
    }
}

void baseMixedFraming(bool septentrio)
{
    Receiver peer;
    peer.septentrio = septentrio;
    auto io = peer.io();
    GPSNativePositionReport position;
    std::vector<std::vector<uint8_t>> frames;
    size_t otherReports = 0;
    io.decoded = [&](const GPSDecodedBatch& batch) {
        CHECK(batch.events.size() <= GPSDecodedBatch::MAX_EVENTS);
        for (const auto& event : batch.events) {
            if (const auto* frame = std::get_if<GPSRTCMReport>(&event)) {
                frames.emplace_back(frame->bytes.begin(), frame->bytes.begin() + frame->size);
            } else {
                ++otherReports;
            }
        }
    };
    std::unique_ptr<GPSProtocol> driver;
    if (septentrio) {
        driver = std::make_unique<GPSNativeSBF>(captureGPSReports(io, position), false);
    } else {
        driver = std::make_unique<GPSNativeFemto>(captureGPSReports(io, position), false);
    }
    GPSProtocol::GPSConfig config;
    config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
    unsigned baud = 115200;
    CHECK(driver->configure(baud, config));
    driver->consume({});
    otherReports = 0;
    const std::string body = "GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,";
    const auto text = nmeaSentence(body);
    const auto binary = rtcmPacket({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
    driver->consume(binary);
    CHECK(frames == std::vector<std::vector<uint8_t>>{binary});
    CHECK(otherReports == 0);
    verifyRTCMRecovery(*driver, frames);
    CHECK(otherReports == 0);
}

void configurationDecodesInterleavedTraffic()
{
    {
        Receiver peer;
        peer.interleave_on = "UNLOGALL";
        peer.interleaved = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,");
        size_t usageReports = 0;
        std::vector<GPSNativeSurveyReport> surveys;
        auto io = peer.io();
        io.decoded = [&](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                usageReports += std::holds_alternative<GPSNativeSatelliteUsageReport>(event);
                if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*survey);
                }
            }
        };
        GPSNativePositionReport position;
        GPSNativeFemto driver(captureGPSReports(io, position));
        GPSProtocol::GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
        unsigned baud = 115200;
        CHECK(driver.configure(baud, config));
        CHECK(usageReports == 1);
        (void) driver.receive(10);
        // The fixed-quality GGA predates this configuration's survey, so it must not complete it.
        CHECK(!peer.sent("LOG RTCM"));
        CHECK(std::none_of(surveys.begin(), surveys.end(), [](const auto& survey) { return survey.survey.valid; }));
    }
    {
        Receiver peer;
        peer.septentrio = true;
        std::vector<uint8_t> frame(94);
        (void) LittleEndian::write<uint16_t>(frame, 0, 0x4024);
        (void) LittleEndian::write<uint16_t>(frame, 4, SBF_ID_PVTGeodetic);
        (void) LittleEndian::write<uint16_t>(frame, 6, uint16_t(frame.size()));
        (void) LittleEndian::write<uint16_t>(frame, 12, 2435);
        frame[14] = 1;
        (void) LittleEndian::write<double>(frame, 16, 0.5);
        (void) LittleEndian::write<double>(frame, 24, 1);
        (void) LittleEndian::write<double>(frame, 32, 500);
        (void) LittleEndian::write<uint16_t>(frame, 2, crc16(frame.data() + 4, frame.size() - 4));
        peer.interleave_on = "setDataInOut";
        peer.interleaved.assign(frame.begin(), frame.end());
        size_t fixCount = 0;
        auto io = peer.io();
        io.decoded = [&](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                fixCount += std::holds_alternative<GPSNativePositionReport>(event);
            }
        };
        GPSNativePositionReport position;
        GPSNativeSBF driver(captureGPSReports(io, position), false);
        GPSProtocol::GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}};
        unsigned baud = 115200;
        CHECK(driver.configure(baud, config));
        gps_test_time += 200000;
        driver.consume({});
        CHECK(fixCount == 1);
        CHECK(std::abs(position.navigation.latitudeDegrees - 0.5 * GPS_RAD_TO_DEG) < 1e-6);
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
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        rawAcknowledgements();
        for (bool septentrio : {false, true}) {
            for (bool fixed : {false, true}) {
                receiverMode(septentrio, fixed);
            }
            baseMixedFraming(septentrio);
        }
        sbfConfirmationPolicy();
        sbfRequiredBaseCommands();
        sbfSelectedPortAndPrecision();
        sbfDatumRejection();
        sbfFrameOwnership();
        sbfSurveyEvidence();
        configurationDecodesInterleavedTraffic();
        receiverMode(false, true, {}, false, 1);
        receiverMode(false, true, {}, false, GPS_READ_BUFFER_SIZE, 2 * GPS_READ_BUFFER_SIZE - 3);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolReceiverModesTest, TestLabel::Unit)

#include "gps-receiver-mode-test.moc"
