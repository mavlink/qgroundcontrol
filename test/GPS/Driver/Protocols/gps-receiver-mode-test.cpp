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
#include "GPSProtocolFeatures.h"
#include "GPSProtocolTestIO.h"
#include "LittleEndian.h"
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
class Receiver
{
public:
    bool septentrio = false;
    std::string rejected_command;
    bool cancel_read = false;
    bool silence_rejected = false;
    size_t read_chunk = 7;
    size_t noise_bytes = 0;
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
            rejected = !rejected_command.empty() && command.compare(0, rejected_command.size(), rejected_command) == 0;
            if (rejected) {
                reply = silence_rejected ? std::string{} : septentrio ? "$R? rejected\n" : "<ERROR\r\n";
            } else if (septentrio) {
                reply = command == "\n\r" ? "USB1>" : "$R: " + command;
            } else {
                reply = '<' + command.substr(0, command.find_first_of(" \r\n")) + " OK";
                reply.insert(0, noise_bytes, '\0');
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

static void receiverMode(bool septentrio, GPSProtocol::OutputMode mode, bool fixed,
                         const std::string& rejected_command = {}, bool cancel_read = false, size_t read_chunk = 7,
                         size_t noise_bytes = 0)
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
    std::unique_ptr<GPSBaseProtocol> driver;
    if (septentrio) {
#if QGC_GPS_ENABLE_SBF
        driver = std::make_unique<GPSNativeSBF>(receiver.io(), &position, &satellites);
#endif
    } else {
#if QGC_GPS_ENABLE_FEMTO
        driver = std::make_unique<GPSNativeFemto>(receiver.io(), &position, &satellites);
#endif
    }
    CHECK(driver);
    GPSProtocol::GPSConfig config{};
    config.base = {.useFixedBase = fixed,
                   .surveyInAccMeters = 1.25,
                   .surveyInDurationSecs = 60,
                   .fixedPosition = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                   .fixedBaseAccuracyMeters = 1.0f};
    config.output_mode = mode;
    unsigned baudrate = 115200;
    const int result = driver->configure(baudrate, config);
    if (!rejected_command.empty()) {
        CHECK(result < 0);
        CHECK(receiver.sent(rejected_command));
        CHECK(!receiver.sent(septentrio ? "setSBFOutput, Stream1, USB1, PVTGeodetic" : "LOG UAVGPSB"));
        if (cancel_read) {
            CHECK(gps_test_warnings.empty());
            CHECK(receiver.failed_reads == 1);
        }
        return;
    }
    CHECK(result == 0);
    CHECK(gps_test_warnings.empty());
    const bool base = mode == GPSProtocol::OutputMode::RTCM;
    if (septentrio) {
        CHECK(receiver.sent("setPVTMode, Rover, All, auto") == !base);
        CHECK(receiver.sent("setPVTMode, Static") == base);
        CHECK(receiver.sent("setDataInOut, USB1, Auto, RTCMv3+SBF") == (mode != GPSProtocol::OutputMode::GPS));
        CHECK(receiver.sent("setStaticPosGeodetic") == (base && fixed));
    } else {
        CHECK(receiver.sent("POSAVE OFF") == !base);
        CHECK(receiver.sent("FIX NONE") == !base);
        CHECK(receiver.sent("LOG UAVGPSB") == !base);
        CHECK(receiver.sent("FIX POSITION 47.00000000 8.00000000") == (base && fixed));
        CHECK(receiver.sent("POSAVE ON") == (base && !fixed));
    }
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    config.base = {};
    const auto calls = receiver.transport_calls;
    CHECK(driver->configure(baudrate, config) < 0);
    CHECK(!driver->receiverReady());
    CHECK(receiver.transport_calls == calls);
}

#if QGC_GPS_ENABLE_SBF
void sbfRequiredBaseCommands()
{
    const std::vector<std::string> fixedCommands = {
        "setDataInOut, USB1, Auto, RTCMv3+SBF", "setStaticPosGeodetic", "setAntennaOffset",
        "setReceiverDynamics, Low, Static",     "setPVTMode, Static",   "setSBFOutput, Stream1, USB1, +PVTGeodetic",
    };
    for (bool fixed : {false, true}) {
        const auto commands =
            fixed ? fixedCommands
                  : std::vector<std::string>{"setDataInOut, USB1, Auto, RTCMv3+SBF", "setPVTMode, Static",
                                             "setSBFOutput, Stream1, USB1, +PVTGeodetic"};
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
                GPSNativeSBF driver(io, &position, &satellites);
                GPSProtocol::GPSConfig config{};
                config.output_mode = GPSProtocol::OutputMode::RTCM;
                config.base = {.useFixedBase = fixed,
                               .surveyInAccMeters = 1,
                               .surveyInDurationSecs = 60,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}};
                unsigned baudrate = 115200;
                CHECK(driver.configure(baudrate, config) < 0);
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
#endif

#if QGC_GPS_ENABLE_SBF
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
    GPSNativeSBF driver(io, &position, &satellites);
    GPSProtocol::GPSConfig config{};
    config.base = {.useFixedBase = true,
                   .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                   .fixedBaseAccuracyMeters = 1};
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    unsigned baudrate = 115200;
    CHECK(driver.configure(baudrate, config) == 0);
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
    CHECK(std::abs(position.latitude_deg - 0.5 * GPS_RAD_TO_DEG) < 1e-6);
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
    GPSNativeSBF driver(io, &position);
    GPSProtocol::GPSConfig config{};
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    config.base = {.surveyInAccMeters = 1,
                   .surveyInDurationSecs = 60,
                   .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}};
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
        CHECK(surveys.back().flags == flags);
        CHECK(!surveys.back().accuracyKnown);
        CHECK(surveys.back().altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid);
        gps_test_time += 200000;
        driver.consume({});
        if (horizontal == UINT16_MAX) {
            CHECK(std::isnan(position.eph));
        } else {
            CHECK(position.eph == horizontal / 200.0f);
        }
        if (vertical == UINT16_MAX) {
            CHECK(std::isnan(position.epv));
        } else {
            CHECK(position.epv == vertical / 200.0f);
        }
        return surveys.back().duration;
    };

    for (bool fixed : {false, true, false}) {
        config.base.useFixedBase = fixed;
        CHECK(driver.configure(baudrate, config) == 0);
        CHECK(driver.receiverReady());
        // A standalone solution without the determination flag is not a completed fixed base.
        CHECK(publish(1, 0) == 0);
        const auto progress = publish(0x41, fixed ? 0 : 2);
        CHECK(fixed ? progress == 0 : progress > 0);
        const auto completed = publish(3, 1);
        CHECK(fixed ? completed == 0 : completed >= progress);
        CHECK(std::abs(surveys.back().latitude - 0.5 * GPS_RAD_TO_DEG) < 1e-6);
        CHECK(surveys.back().altitude == 500);
        CHECK(publish(3, 1, 0, 0) == completed);
        CHECK(publish(3, 1, UINT16_MAX, UINT16_MAX) == completed);
        CHECK(publish(3, 1, UINT16_MAX, 200) == completed);
        CHECK(publish(3, 1, 200, UINT16_MAX) == completed);

        for (uint8_t mode : {0, 1, 2, 4, 9, 15, 0x83}) {
            CHECK(publish(mode, 0) == completed);
        }
        frame[15] = 9;
        CHECK(publish(3, 0) == completed);
        CHECK(std::isnan(surveys.back().latitude));
        const auto failedProgress = publish(0x41, fixed ? 0 : 2);
        CHECK(fixed ? failedProgress == 0 : failedProgress > completed);
        frame[15] = 0;
        publish(3, 1);

        for (size_t offset : {16, 24, 32}) {
            const double original = LittleEndian::read<double>(frame, offset).value();
            for (double invalid : {double(NAN), double(INFINITY), -2e10}) {
                (void) LittleEndian::write<double>(frame, offset, invalid);
                publish(3, 0);
                CHECK(std::isnan(surveys.back().latitude));
                CHECK(std::isnan(surveys.back().longitude));
                CHECK(std::isnan(surveys.back().altitude));
            }
            (void) LittleEndian::write<double>(frame, offset, original);
        }
        publish(3, 1);
    }
    config.output_mode = GPSProtocol::OutputMode::GPS;
    CHECK(driver.configure(baudrate, config) == 0);
    surveys.clear();
    driver.consume(frame);
    CHECK(surveys.empty());
}
#endif

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
        for (bool septentrio : {false, true}) {
            if ((septentrio && !QGC_GPS_ENABLE_SBF) || (!septentrio && !QGC_GPS_ENABLE_FEMTO)) {
                continue;
            }
            for (bool fixed : {false, true}) {
                receiverMode(septentrio, GPSProtocol::OutputMode::GPS, fixed);
                receiverMode(septentrio, GPSProtocol::OutputMode::RTCM, fixed);
            }
            const std::vector<std::string> stop_commands = septentrio
                                                               ? std::vector<std::string>{"setPVTMode, Rover"}
                                                               : std::vector<std::string>{"POSAVE OFF", "FIX NONE"};
            for (const auto& command : stop_commands) {
                for (bool cancel : {false, true}) {
                    receiverMode(septentrio, GPSProtocol::OutputMode::GPS, true, command, cancel);
                }
            }
        }
#if QGC_GPS_ENABLE_SBF
        sbfRequiredBaseCommands();
        sbfFrameOwnership();
        sbfSurveyEvidence();
#endif
#if QGC_GPS_ENABLE_FEMTO
        receiverMode(false, GPSProtocol::OutputMode::GPS, true, {}, false, 1);
        receiverMode(false, GPSProtocol::OutputMode::GPS, true, {}, false, GPS_READ_BUFFER_SIZE,
                     2 * GPS_READ_BUFFER_SIZE - 3);
#endif
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolReceiverModesTest, TestLabel::Unit)

#include "gps-receiver-mode-test.moc"
