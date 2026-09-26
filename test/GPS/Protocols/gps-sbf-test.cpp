#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Checksums.h"
#include "GPSCommandChannel.h"
#include "GPSEventSink.h"
#include "GPSProtocolMath.h"
#include "GPSProtocolRuntime.h"
#include "LittleEndian.h"
#include "RTCMFramer.h"
#include "SBF/Generated/SBFBlocks.h"
#include "SBF/SBFFamily.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "Support/ProtocolTestPackets.h"
#include "UnitTest.h"
#include "fixtures/GPSFixtureExpectations.h"

using namespace std::chrono_literals;

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {

constexpr double RAD_TO_DEG = GPSProtocolMath::RAD_TO_DEG;

std::vector<uint8_t> fixture(const char* name)
{
    std::ifstream file(std::string(GPS_FIXTURE_DIR) + "/" + name, std::ios::binary);
    CHECK(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

bool matches(double actual, double expected, double tolerance = 1e-6)
{
    return std::isnan(expected) ? std::isnan(actual) : std::abs(actual - expected) < tolerance;
}

GPSRuntimeIO noDevice(GPSTestClock& clock)
{
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

/// The PVTGeodetic fields the decode cases set, encoded at their documented offsets in the block body.
struct PVTBody
{
    uint8_t mode = 0;
    double latitude = 0;
    double longitude = 0;
    double height = 0;
    float cog = 0;
    uint8_t nrSV = 0;
    uint16_t hAccuracy = 0;
};

std::vector<uint8_t> bytes(const PVTBody& v)
{
    std::vector<uint8_t> b(80);
    b[0] = v.mode;
    (void) LittleEndian::write<double>(b, 2, v.latitude);
    (void) LittleEndian::write<double>(b, 10, v.longitude);
    (void) LittleEndian::write<double>(b, 18, v.height);
    (void) LittleEndian::write<float>(b, 42, v.cog);
    b[60] = v.nrSV;
    (void) LittleEndian::write<uint16_t>(b, 76, v.hAccuracy);
    return b;
}

/// A whole PVTGeodetic block of @a size bytes for week 2435, with the CRC left to sealed().
std::vector<uint8_t> pvtBlock(size_t size)
{
    std::vector<uint8_t> frame(size);
    (void) LittleEndian::write<uint16_t>(frame, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(frame, 4, SBF::BlockId::PVT_GEODETIC);
    (void) LittleEndian::write<uint16_t>(frame, 6, uint16_t(frame.size()));
    (void) LittleEndian::write<uint16_t>(frame, 12, 2435);
    return frame;
}

void seal(std::vector<uint8_t>& frame)
{
    (void) LittleEndian::write<uint16_t>(frame, 2, QGC::crc16Ccitt(std::span<const uint8_t>(frame).subspan(4)));
}

int surveyFlags(const GPSDecodedSurvey& report)
{
    return (report.survey.valid ? 1 : 0) | (report.survey.active ? 2 : 0);
}

uint32_t surveyDuration(const GPSDecodedSurvey& report)
{
    return static_cast<uint32_t>(report.survey.duration.count());
}

/// The SBF family on the runtime. Keeps every delivered event, the RTCM3 frames among them, the latest position and
/// the finished commands.
class SBFRuntime
{
public:
    explicit SBFRuntime(GPSRuntimeIO io, bool satelliteInfoEnabled = true)
    {
        GPSRuntimeObserver observer;
        observer.decoded = [this](const GPSEventBatch& batch) {
            // Observers run inside the runtime's coroutines, so they record failures instead of throwing.
            oversizedBatches += batch.events.size() > GPSEventSink::MAX_EVENTS;
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                    position = *report;
                } else if (const auto* frame = std::get_if<GPSRTCMFrame>(&event)) {
                    rtcm.emplace_back(frame->bytes.begin(), frame->bytes.end());
                }
            }
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        observer.commandFinished = [this](const GPSCommandResult& result) { commands.push_back(result); };
        _runtime = std::make_unique<GPSProtocolRuntime>(SBF::FAMILY, std::move(io), std::move(observer),
                                                        GPSFamilyOptions{.satelliteInfoEnabled = satelliteInfoEnabled});
    }

    SBFRuntime(const SBFRuntime&) = delete;
    SBFRuntime& operator=(const SBFRuntime&) = delete;

    GPSProtocolRuntime& operator*() { return *_runtime; }

    GPSProtocolRuntime* operator->() { return _runtime.get(); }

    template <typename T>
    std::vector<T> reports() const
    {
        std::vector<T> result;
        for (const auto& event : events) {
            if (const auto* value = std::get_if<T>(&event)) {
                result.push_back(*value);
            }
        }
        return result;
    }

    std::vector<GPSProtocolEvent> events;
    std::vector<std::vector<uint8_t>> rtcm;
    std::vector<GPSCommandResult> commands;
    GPSDecodedPosition position;
    size_t oversizedBatches = 0;

private:
    std::unique_ptr<GPSProtocolRuntime> _runtime;
};

/// Answers like a Septentrio receiver: "$R: <command>" for an accepted command, "$R?" for a rejected one, and its
/// connection descriptor followed by '>' for the prompt.
class Receiver
{
public:
    explicit Receiver(GPSTestClock& testClock)
        : _clock(testClock)
    {}

    std::string port = "USB1";
    std::string rejectedCommand;
    unsigned rejectAfter = 0;
    unsigned rejectAttempts = UINT_MAX;
    unsigned matchedCommands = 0;
    bool unterminatedReply = false;
    bool conflictingReply = false;
    bool silenceRejected = false;
    size_t readChunk = 7;
    std::string interleaveOn;
    std::string interleaved;
    size_t transportCalls = 0;
    std::vector<std::string> commands;

    bool sent(const std::string& prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [&](const auto& command) { return command.compare(0, prefix.size(), prefix) == 0; });
    }

    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(_clock);
        result.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
            ++transportCalls;
            const auto timeout = deadline.remaining(_clock.nowUs());
            CHECK(timeout >= 0ms);
            _clock.advanceBy(1000);
            if (_reply.empty()) {
                _clock.advanceBy(static_cast<uint64_t>(std::chrono::microseconds(timeout).count()) + 1);
                return {GPSReadStatus::TimedOut};
            }
            // Septentrio does not guarantee that an ACK fits in one read or includes a NUL terminator.
            const size_t count = std::min({_reply.size(), bytes.size(), readChunk});
            memcpy(bytes.data(), _reply.data(), count);
            _reply.erase(0, count);
            _clock.advanceBy(1000);
            return {GPSReadStatus::Data, static_cast<int>(count)};
        };
        result.write = [this](std::span<const uint8_t> input, GPSDeadline) -> GPSWriteResult {
            ++transportCalls;
            const int size = static_cast<int>(input.size());
            const std::string command(reinterpret_cast<const char*>(input.data()), input.size());
            commands.push_back(command);
            const bool matching =
                !rejectedCommand.empty() && command.compare(0, rejectedCommand.size(), rejectedCommand) == 0;
            if (matching) {
                ++matchedCommands;
            }
            const bool rejected =
                matching && matchedCommands > rejectAfter && matchedCommands - rejectAfter <= rejectAttempts;
            if (rejected) {
                _reply = silenceRejected ? std::string{} : "$R? rejected\n";
                if (conflictingReply && !silenceRejected) {
                    _reply += "$R: " + command;
                }
            } else {
                _reply = command == "\n\r" ? port + ">" : "$R: " + command;
            }
            if (unterminatedReply) {
                while (_reply.ends_with('\r') || _reply.ends_with('\n')) {
                    _reply.pop_back();
                }
            }
            if (!interleaveOn.empty() && command.starts_with(interleaveOn)) {
                _reply.insert(0, interleaved);
                interleaveOn.clear();
            }
            return {GPSWriteStatus::Completed, size, size};
        };
        result.setBaudrate = [this](unsigned) {
            ++transportCalls;
            return GPSBaudStatus::Configured;
        };
        return result;
    }

private:
    std::string _reply;
    GPSTestClock& _clock;
};

GPSConfig surveyIn()
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
}

GPSConfig fixedBase(double latitude, double longitude, float altitude, float accuracy = 0.0f)
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = latitude,
                                                                      .longitudeDegrees = longitude,
                                                                      .altitudeMeters = altitude},
                                                         .accuracyMeters = accuracy}}};
}

// Decoding

void malformedMessages(GPSTestClock& clock)
{
    const std::array<uint8_t, 2> shortPayload{};
    SBFRuntime sbf(noDevice(clock));
    const auto& position = sbf.position;
    PVTBody fix{};
    fix.mode = 1;
    fix.latitude = 0.5;
    fix.longitude = 1.0;
    fix.nrSV = 12;
    const auto good = sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix));
    (void) sbf->consume(good);
    clock.advanceBy(200000);
    CHECK(sbf->consume({}).testFlag(GPSReceiveUpdate::Position));
    CHECK(std::abs(position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 0.00001);
    const auto received = position.navigation.timestampUs;
    CHECK(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, shortPayload, 1000)) == GPSReceiveUpdates{});
    CHECK(position.navigation.timestampUs == received);
    CHECK(!sbf->consume(good).testFlag(GPSReceiveUpdate::Position));  // Duplicate receiver epoch does not republish.

    fix.cog = -2.0e10f;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 1000));
    clock.advanceBy(200000);
    CHECK(sbf->consume({}).testFlag(GPSReceiveUpdate::Position));
    CHECK(std::isnan(position.navigation.courseRadians));
    fix.cog = 90.0f;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 2000));
    clock.advanceBy(200000);
    CHECK(sbf->consume({}).testFlag(GPSReceiveUpdate::Position));
    CHECK(std::abs(position.navigation.courseRadians - GPSProtocolMath::PI / 2) < 0.00001);
}

void epochMetadata(GPSTestClock& clock)
{
    SBFRuntime sbf(makeGPSRuntimeTestIO(clock));
    PVTBody fix{};
    fix.mode = 0x81;  // Stand-alone PVT in 2D mode.
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    fix.nrSV = UINT8_MAX;
    fix.hAccuracy = UINT16_MAX;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 1000));
    CHECK(sbf.reports<GPSDecodedPosition>().empty());
    clock.advanceBy(200000);
    (void) sbf->consume({});
    auto fixes = sbf.reports<GPSDecodedPosition>();
    const auto usage = sbf.reports<GPSDecodedSatelliteUsage>();
    CHECK(fixes.size() == 1);
    CHECK(fixes[0].navigation.fixType == GPSPositionReport::FixType::Fix2D);
    CHECK(fixes[0].navigation.satellitesUsed == UINT8_MAX);
    CHECK(usage.size() == 1 && !usage[0].usedCount);
    CHECK(std::isnan(fixes[0].navigation.horizontalDop));
    CHECK(std::isnan(fixes[0].navigation.horizontalAccuracyMeters));
    CHECK(fixes[0].navigation.utcTimeUs == 0);  // GNSS time cannot be labeled UTC without a receiver UTC offset.
    fix.mode = 1;
    fix.nrSV = 0;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 3000));
    clock.advanceBy(200000);
    (void) sbf->consume({});
    fixes = sbf.reports<GPSDecodedPosition>();
    CHECK(fixes.size() == 2);
    CHECK(fixes.back().navigation.satellitesUsed == 0);
    CHECK(fixes.back().navigation.fixType == GPSPositionReport::FixType::Fix3D);
    CHECK(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), UINT32_MAX)) == GPSReceiveUpdates{});
    CHECK(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 4000, UINT16_MAX)) == GPSReceiveUpdates{});
    clock.advanceBy(200000);
    (void) sbf->consume({});
    CHECK(sbf.reports<GPSDecodedPosition>().size() == 2);
}

void invalidCoordinates(GPSTestClock& clock)
{
    SBFRuntime sbf(noDevice(clock));
    PVTBody fix{};
    fix.mode = 1;
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    uint32_t tow = 1000;
    auto check = [&](const auto& payload) {
        const auto previous = sbf.reports<GPSDecodedPosition>().size();
        (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, payload, tow));
        tow += 1000;
        clock.advanceBy(200000);
        (void) sbf->consume({});
        const auto reports = sbf.reports<GPSDecodedPosition>();
        CHECK(reports.size() == previous + 1);
        CHECK(reports.back().navigation.fixType == GPSPositionReport::FixType::NoFix);
    };

    struct InvalidCoordinate
    {
        size_t offset;
        double value;
    };

    for (const auto& invalid : std::array{
             InvalidCoordinate{2, NAN},
             InvalidCoordinate{2, 4},
             InvalidCoordinate{10, NAN},
             InvalidCoordinate{10, 4},
             InvalidCoordinate{18, NAN},
             InvalidCoordinate{18, 1e11},
         }) {
        auto payload = bytes(fix);
        CHECK(LittleEndian::write(payload, invalid.offset, invalid.value));
        check(payload);
    }
    for (const float invalid : {NAN, 1e11f}) {
        auto payload = bytes(fix);
        CHECK(LittleEndian::write(payload, 26, invalid));
        check(payload);
    }
}

// Captured and synthetic fixtures

void independentValidity(GPSTestClock& clock)
{
    for (const auto& expected : GPSFixture::sbfEpochs) {
        for (const size_t chunkSize : {1u, 11u, 512u}) {
            clock.advanceBy(1000000);
            SBFRuntime sbf(noDevice(clock), false);
            const auto& position = sbf.position;
            const auto blocks = fixture(expected.filename);
            auto remaining = std::span(blocks);
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                (void) sbf->consume(remaining.first(count));
                remaining = remaining.subspan(count);
            }
            CHECK(sbf.reports<GPSDecodedPosition>().empty());
            clock.advanceBy(200000);
            (void) sbf->consume({});
            CHECK(sbf.reports<GPSDecodedPosition>().size() == 1);
            CHECK(position.navigation.utcTimeUs == 0);
            CHECK(matches(position.navigation.latitudeDegrees, expected.latitude, 1e-9));
            CHECK(matches(position.navigation.longitudeDegrees, expected.longitude, 1e-9));
            CHECK(matches(position.navigation.altitudeEllipsoidMeters, expected.ellipsoid));
            CHECK(matches(position.navigation.altitudeMslMeters, expected.msl));
            CHECK(matches(position.navigation.horizontalAccuracyMeters, expected.horizontalAccuracy));
            CHECK(matches(position.navigation.verticalAccuracyMeters, expected.verticalAccuracy));
            if (expected.velocityAvailable) {
                CHECK(matches(position.navigation.speedMetersPerSecond,
                              std::hypot(static_cast<float>(expected.north), static_cast<float>(expected.east))));
            }
            CHECK(matches(position.navigation.courseRadians, expected.course));
            // Base stations output only PVTGeodetic; DOP and attitude blocks in the capture are ignored.
            CHECK(std::isnan(position.navigation.horizontalDop));
            CHECK(std::isnan(position.navigation.verticalDop));
            CHECK(std::isnan(position.navigation.headingRadians));
            CHECK(std::isnan(position.navigation.headingAccuracyRadians));
            CHECK(position.navigation.satellitesUsed == expected.satellites);
            CHECK(position.velocityValid == expected.velocityAvailable);
            using Fix = GPSPositionReport::FixType;
            const auto fix = expected.error            ? Fix::NoFix
                             : expected.twoDimensional ? Fix::Fix2D
                             : expected.mode == 4      ? Fix::RTKFixed
                             : expected.mode == 5      ? Fix::RTKFloat
                                                       : Fix::Fix3D;
            CHECK(position.navigation.fixType == fix);
        }
    }
    SBFRuntime sbf(noDevice(clock), false);
    const auto invalidTimes = fixture("synthetic-invalid-time.sbf");
    for (const auto& expected : GPSFixture::invalidSbfTimes) {
        CHECK(expected.week == UINT16_MAX || expected.tow >= 604800000);
        (void) sbf->consume(std::span(invalidTimes).subspan(expected.offset, expected.size));
        clock.advanceBy(200000);
        CHECK(sbf->decode({}).batch.events.empty());
        CHECK(sbf.position.navigation.timestampUs == 0);
    }
    const auto valid = fixture("synthetic-valid.sbf");
    auto corrupt = valid;
    corrupt[2] ^= 1;  // Reject the PVT CRC; remaining metadata must not manufacture a fix.
    (void) sbf->consume(corrupt);
    clock.advanceBy(200000);
    CHECK(sbf->decode({}).batch.events.empty());
    CHECK(sbf.position.navigation.timestampUs == 0);

    SBFRuntime recovered(noDevice(clock), false);
    (void) recovered->consume(invalidTimes);
    (void) recovered->consume(valid);
    clock.advanceBy(200000);
    (void) recovered->consume({});
    CHECK(recovered.position.navigation.timestampUs != 0);
    CHECK(matches(recovered.position.navigation.latitudeDegrees, GPSFixture::sbfEpochs[0].latitude, 1e-9));
}

void capturedBlocks(GPSTestClock& clock)
{
    {
        SBFRuntime sbf(noDevice(clock));
        const auto& position = sbf.position;
        for (auto byte : fixture("geodetic.sbf")) {
            (void) sbf->consume({&byte, 1});
        }
        clock.advanceBy(200000);
        (void) sbf->consume({});
        CHECK(position.velocityValid);
        // Attitude blocks for a different epoch must not create another position.
        const auto previousTimestamp = position.navigation.timestampUs;
        (void) sbf->consume(fixture("attitude.sbf"));
        clock.advanceBy(200000);
        (void) sbf->consume({});
        CHECK(position.navigation.timestampUs == previousTimestamp);
        CHECK(std::isnan(position.navigation.headingRadians));
    }
    SBFRuntime sbf(noDevice(clock));
    const auto& position = sbf.position;
    for (auto byte : fixture("pvt-geodetic.sbf")) {
        (void) sbf->consume({&byte, 1});
    }
    clock.advanceBy(200000);
    (void) sbf->consume({});
    CHECK(std::abs(position.navigation.latitudeDegrees - 0.9310293523340808 * RAD_TO_DEG) < 1e-8);
    CHECK(std::abs(position.navigation.longitudeDegrees + 0.03921206770879602 * RAD_TO_DEG) < 1e-8);
    CHECK(std::abs(position.navigation.altitudeEllipsoidMeters - 131.18596542546626) < 1e-5);
    CHECK(position.navigation.satellitesUsed == 36);
    CHECK(std::isnan(position.navigation.courseRadians));
}

// Configuration

void receiverMode(GPSTestClock& clock, bool fixed)
{
    clock.reset();
    const GPSProtocolLogCapture log;
    Receiver receiver(clock);
    SBFRuntime sbf(receiver.io());
    GPSConfig config =
        fixed ? fixedBase(47.0, 8.0, 500.0f, 1.0f)
              : GPSConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.25, .duration = 60s}}};
    unsigned baudrate = 115200;
    CHECK(sbf->configure(config, baudrate));
    CHECK(log.warnings().isEmpty());
    CHECK(!receiver.sent("setAttitudeOffset, 0.000, 0.000"));
    CHECK(!receiver.sent("setPVTMode, Rover, All, auto"));
    CHECK(receiver.sent("setPVTMode, Static"));
    CHECK(receiver.sent("setDataInOut, USB1, Auto, RTCMv3+SBF"));
    CHECK(receiver.sent("setStaticPosGeodetic") == fixed);
    config.base = {};
    const auto calls = receiver.transportCalls;
    CHECK(!sbf->configure(config, baudrate));
    CHECK(!sbf->receiverReady());
    CHECK(receiver.transportCalls == calls);
}

void baseMixedFraming(GPSTestClock& clock)
{
    Receiver peer(clock);
    SBFRuntime sbf(peer.io(), false);
    unsigned baud = 115200;
    CHECK(sbf->configure(surveyIn(), baud));
    (void) sbf->consume({});
    sbf.events.clear();
    const auto otherReports = [&sbf] {
        return std::ranges::count_if(sbf.events,
                                     [](const auto& event) { return !std::holds_alternative<GPSRTCMFrame>(event); });
    };
    const std::string body = "GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,";
    const auto text = nmeaSentence(body);
    const auto binary = rtcmPacket({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
    sbf.rtcm.clear();
    (void) sbf->consume(binary);
    CHECK(sbf.rtcm == std::vector<std::vector<uint8_t>>{binary});
    CHECK(otherReports() == 0);
    verifyRTCMRecovery(*sbf, sbf.rtcm);
    CHECK(otherReports() == 0);
    CHECK(sbf.oversizedBatches == 0);
}

void confirmationPolicy(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    for (unsigned failures : {0u, 1u, 4u, 5u}) {
        for (bool firstFails : {false, true}) {
            Receiver receiver(clock);
            receiver.unterminatedReply = true;
            receiver.rejectAfter = firstFails ? 0 : 1;
            receiver.rejectAttempts = failures;
            const std::string dataIO = "setDataInOut, USB1, Auto, SBF\n";
            receiver.rejectedCommand = dataIO;
            SBFRuntime sbf(receiver.io(), false);
            unsigned baud = 115200;
            const bool success = sbf->configure(surveyIn(), baud);
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
    Receiver receiver(clock);
    receiver.rejectedCommand = "setGeodeticDatum";
    receiver.conflictingReply = true;
    receiver.readChunk = GPSCommandChannel::READ_CHUNK_SIZE;
    SBFRuntime sbf(receiver.io());
    unsigned baud = 115200;
    CHECK(!sbf->configure(surveyIn(), baud));
    CHECK(receiver.commands.back() == "setGeodeticDatum, WGS84\n");
}

void requiredBaseCommands(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
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
                clock.reset();
                Receiver receiver(clock);
                receiver.rejectedCommand = command;
                receiver.silenceRejected = silent;
                SBFRuntime sbf(receiver.io());
                GPSConfig config{};
                config.base = {
                    .mode =
                        fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                                    .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                              : GPSBaseStationConfig::Mode{
                                    GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
                unsigned baudrate = 115200;
                CHECK(!sbf->configure(config, baudrate));
                CHECK(!sbf->receiverReady());
                CHECK(receiver.sent(command));
                CHECK(receiver.commands.back().starts_with(command));
                const auto& results = sbf.commands;
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

void selectedPortAndPrecision(GPSTestClock& clock)
{
    for (const auto* port : {"COM1", "USB2", "IP10", "IPS1"}) {
        Receiver peer(clock);
        peer.port = port;
        SBFRuntime sbf(peer.io(), false);
        unsigned baud = 115200;
        CHECK(sbf->configure(fixedBase(47.397742491, -8.545593291, 500.125f), baud));
        CHECK(peer.sent(std::string("setDataInOut, ") + port + ", Auto, RTCMv3+SBF"));
        CHECK(peer.sent(std::string("setSBFOutput, Stream1, ") + port + ", +PVTGeodetic"));
        CHECK(peer.sent("setStaticPosGeodetic, Geodetic1, 47.397742491, -8.545593291, 500.1250, WGS84"));
        CHECK(peer.sent("setCOMSettings") == std::string_view(port).starts_with("COM"));
    }
    // Coordinates keep printf's bytes: 500.03125 is an exact binary tie that rounds to even, and -0 keeps its sign.
    Receiver peer(clock);
    SBFRuntime sbf(peer.io(), false);
    unsigned baud = 115200;
    CHECK(sbf->configure(fixedBase(-0.0, 8.0, 500.03125f), baud));
    CHECK(peer.sent("setStaticPosGeodetic, Geodetic1, -0.000000000, 8.000000000, 500.0312, WGS84\n"));
}

void datumRejection(GPSTestClock& clock)
{
    for (uint8_t datum : {19, 31, 250}) {
        const GPSProtocolLogCapture log;
        Receiver peer(clock);
        SBFRuntime sbf(peer.io(), false);
        unsigned baud = 115200;
        CHECK(sbf->configure(surveyIn(), baud));
        auto frame = pvtBlock(96);
        frame[14] = 3;
        frame[73] = datum;
        (void) LittleEndian::write<double>(frame, 16, 0.5);
        (void) LittleEndian::write<double>(frame, 24, 1);
        (void) LittleEndian::write<double>(frame, 32, 500);
        seal(frame);
        (void) sbf->consume(frame);
        CHECK(sbf->error() == GPSProtocolError::Protocol);
        CHECK(sbf->errorDetail().contains("datum"));
        CHECK(!sbf->receiverReady());
        const auto surveys = sbf.reports<GPSDecodedSurvey>();
        CHECK(surveys.size() == 1 && surveyFlags(surveys.back()) == 0);
        CHECK(std::isnan(surveys.back().survey.position.latitudeDegrees));
        CHECK(log.warnings() ==
              QStringList{QStringLiteral("Unsupported Septentrio position datum: %1").arg(unsigned(datum))});
        CHECK(log.categories().contains(QStringLiteral("GPS.Driver.Protocols.SBF")));
    }
}

void frameOwnership(GPSTestClock& clock)
{
    Receiver receiver(clock);
    SBFRuntime sbf(receiver.io());
    unsigned baudrate = 115200;
    CHECK(sbf->configure(fixedBase(47, 8, 500, 1), baudrate));
    const auto corrections = [&sbf] { return sbf.reports<GPSRTCMFrame>().size(); };
    const auto fixes = [&sbf] { return sbf.reports<GPSDecodedPosition>().size(); };
    std::vector<uint8_t> correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto checksum = QGC::crc24q(correction);
    correction.push_back(checksum >> 16);
    correction.push_back(checksum >> 8);
    correction.push_back(checksum);
    CHECK(RTCMFramer::isValidFrame(std::span<const uint8_t>(correction)));
    auto native = pvtBlock(94);
    native[14] = 1;
    (void) LittleEndian::write<double>(native, 16, 0.5);
    (void) LittleEndian::write<double>(native, 24, 1);
    // Both checksums are valid; only the outer native frame may own these bytes.
    std::copy(correction.begin(), correction.end(), native.begin() + 60);
    seal(native);
    (void) sbf->consume(native);
    CHECK(corrections() == 0);
    clock.advanceBy(200000);
    (void) sbf->consume({});
    CHECK(fixes() == 1);
    CHECK(std::abs(sbf.position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
    (void) sbf->consume(correction);
    CHECK(corrections() == 1);
    CHECK(fixes() == 1);
}

void surveyEvidence(GPSTestClock& clock)
{
    clock.reset(1000000);
    Receiver receiver(clock);
    SBFRuntime sbf(receiver.io(), false);
    const auto& position = sbf.position;
    unsigned baudrate = 115200;
    uint32_t tow = 0;
    auto frame = pvtBlock(94);
    (void) LittleEndian::write<double>(frame, 16, 0.5);
    (void) LittleEndian::write<double>(frame, 24, 1);
    (void) LittleEndian::write<double>(frame, 32, 500);
    std::vector<GPSDecodedSurvey> surveys;
    auto publish = [&](uint8_t mode, uint8_t flags, uint16_t horizontal = 200, uint16_t vertical = 200) {
        clock.advanceBy(1000000);
        tow += 1000;
        frame[14] = mode;
        (void) LittleEndian::write<uint32_t>(frame, 8, tow);
        (void) LittleEndian::write<uint16_t>(frame, 90, horizontal);
        (void) LittleEndian::write<uint16_t>(frame, 92, vertical);
        seal(frame);
        sbf.events.clear();
        (void) sbf->consume(frame);
        surveys = sbf.reports<GPSDecodedSurvey>();
        CHECK(surveys.size() == 1);
        CHECK(surveyFlags(surveys.back()) == flags);
        CHECK(!surveys.back().survey.meanAccuracyMeters.has_value());
        clock.advanceBy(200000);
        (void) sbf->consume({});
        surveys = sbf.reports<GPSDecodedSurvey>();
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
        const GPSConfig config = fixed ? fixedBase(47, 8, 500) : surveyIn();
        CHECK(sbf->configure(config, baudrate));
        CHECK(sbf->receiverReady());
        // A standalone solution without the determination flag is not a completed fixed base.
        CHECK(publish(1, 0) == 0);
        const auto progress = publish(0x41, fixed ? 0 : 2);
        CHECK(fixed ? progress == 0 : progress > 0);
        const auto completed = publish(3, 1);
        CHECK(fixed ? completed == 0 : completed >= progress);
        CHECK(std::abs(surveys.back().survey.position.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
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

void configurationDecodesInterleavedTraffic(GPSTestClock& clock)
{
    Receiver peer(clock);
    auto frame = pvtBlock(94);
    frame[14] = 1;
    (void) LittleEndian::write<double>(frame, 16, 0.5);
    (void) LittleEndian::write<double>(frame, 24, 1);
    (void) LittleEndian::write<double>(frame, 32, 500);
    seal(frame);
    peer.interleaveOn = "setDataInOut";
    peer.interleaved.assign(frame.begin(), frame.end());
    SBFRuntime sbf(peer.io(), false);
    unsigned baud = 115200;
    CHECK(sbf->configure(surveyIn(), baud));
    clock.advanceBy(200000);
    (void) sbf->consume({});
    CHECK(sbf.reports<GPSDecodedPosition>().size() == 1);
    CHECK(std::abs(sbf.position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
}

}  // namespace

class GPSProtocolSBFTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _decode();
    void _fixtures();
    void _configuration();
};

void GPSProtocolSBFTest::_decode()
{
    GPSTestClock clock;
    try {
        malformedMessages(clock);
        epochMetadata(clock);
        invalidCoordinates(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

void GPSProtocolSBFTest::_fixtures()
{
    GPSTestClock clock;
    try {
        independentValidity(clock);
        capturedBlocks(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

void GPSProtocolSBFTest::_configuration()
{
    GPSTestClock clock;
    try {
        for (bool fixed : {false, true}) {
            receiverMode(clock, fixed);
        }
        baseMixedFraming(clock);
        confirmationPolicy(clock);
        requiredBaseCommands(clock);
        selectedPortAndPrecision(clock);
        datumRejection(clock);
        frameOwnership(clock);
        surveyEvidence(clock);
        configurationDecodesInterleavedTraffic(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolSBFTest, TestLabel::Unit)

#include "gps-sbf-test.moc"
