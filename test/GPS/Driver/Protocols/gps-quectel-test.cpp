#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "Quectel/GPSDriverQuectel.h"
#include "RTCMFramer.h"
#include "Support/QuectelReceiverModel.h"
#include "UnitTest.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
// Fixed manufacturer examples: GNSS Protocol Specification V1.1 §2.3.23
// and Base Station Mode Application Note V1.1 §3.2.2. These are not emitted by a driver encoder.
constexpr std::string_view BOOT = "$PQTMVER,1,MODULE,LG290P03AANR01A03S,2024/04/30,10:53:07*32\r\n";
constexpr std::string_view PROGRESS =
    "$PQTMSVINSTATUS,1,291264000,1,,11,1,60,-2005560.2218,5411825.5447,2706139.7061,1.8691*0C\r\n";
constexpr std::string_view COMPLETE =
    "$PQTMSVINSTATUS,1,291323000,2,,11,60,60,-2005559.8481,5411823.1873,2706139.3995,1.8075*3F\r\n";
constexpr std::array<uint8_t, 25> CORRECTION{0xD3, 0x00, 0x13, 0x3E, 0xD1, 0x22, 0x03, 0x3A, 0x22,
                                             0x66, 0xA8, 0xEE, 0x8B, 0x4A, 0x8C, 0x4D, 0x35, 0x07,
                                             0xA1, 0xBD, 0xDF, 0xBF, 0xBB, 0x0A, 0x12};

std::string sentence(std::string_view body)
{
    unsigned checksum = 0;
    for (const char value : body) {
        checksum ^= static_cast<unsigned char>(value);
    }
    std::array<char, 8> tail{};
    std::snprintf(tail.data(), tail.size(), "*%02X\r\n", checksum);
    return '$' + std::string(body) + tail.data();
}

using Receiver = GPSTest::QuectelReceiver;

GPSProtocol::GPSConfig surveyConfig()
{
    GPSProtocol::GPSConfig config;
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    config.base.surveyInDurationSecs = 60;
    config.base.surveyInAccMeters = 15;
    return config;
}

GPSProtocol::GPSConfig fixedConfig()
{
    auto config = surveyConfig();
    config.base.useFixedBase = true;
    config.base.fixedPosition = {.latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    return config;
}

void feed(GPSNativeQuectel& driver, std::string_view line, size_t chunk = 3)
{
    while (!line.empty()) {
        const auto size = std::min(line.size(), chunk);
        driver.consume({reinterpret_cast<const uint8_t*>(line.data()), size});
        line.remove_prefix(size);
    }
}

void revoked(const GPSNativeSurveyReport& report)
{
    CHECK(report.flags == 0);
    CHECK(!report.accuracyKnown);
    CHECK(report.duration == 0);
    CHECK(std::isnan(report.latitude));
    CHECK(std::isnan(report.longitude));
    CHECK(std::isnan(report.altitude));
    CHECK(report.altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Unknown);
}

std::string surveyStatus(unsigned tow, unsigned validity, unsigned observations, unsigned count = 60)
{
    return sentence("PQTMSVINSTATUS,1," + std::to_string(tow) + ',' + std::to_string(validity) + ",,11," +
                    std::to_string(observations) + ',' + std::to_string(count) +
                    ",-2005559.8481,5411823.1873,2706139.3995,1.8075");
}

void receiveUntil(GPSNativeQuectel& driver, uint64_t deadline)
{
    unsigned slices = 0;
    while (gps_test_time < deadline) {
        CHECK(++slices < 1000);
        driver.receive(1000);
        CHECK(driver.receiverReady());
    }
}

void noPersistence(const Receiver& receiver)
{
    CHECK(!receiver.sent("PQTMSAVEPAR"));
    CHECK(!receiver.sent("PQTMRESTOREPAR"));
    CHECK(!receiver.sent("PQTMCOLD"));
    CHECK(!receiver.sent("PQTMCFGRCVRMODE,W"));
}

void positionAndEvidence()
{
    gps_test_time = 0;
    Receiver receiver;
    GPSNativePositionReport position{};
    GPSNativeQuectel driver(receiver.io(), &position);
    unsigned baud = 0;
    CHECK(!driver.receiverReady());
    CHECK(driver.configure(baud, {}) == 0);
    CHECK(baud == 460800);
    CHECK(driver.receiverReady());
    CHECK(receiver.commands[0] == "PQTMVERNO");
    CHECK(receiver.commands[1] == "PQTMCFGRCVRMODE,R");
    for (const auto name : {"GGA", "GST", "GSA", "GSV"}) {
        CHECK(receiver.sent("PQTMCFGMSGRATE,W," + std::string(name) + ",1"));
        CHECK(receiver.sent("PQTMCFGMSGRATE,R," + std::string(name)));
    }
    CHECK(receiver.sent("PQTMSRR"));
    CHECK(receiver.outcomes[0].evidence.outcome == GPSCommandOutcome::ReadbackVerified);
    CHECK(receiver.outcomes[2].evidence.outcome == GPSCommandOutcome::Written);
    CHECK(receiver.outcomes[5].evidence.outcome == GPSCommandOutcome::Acknowledged);
    CHECK(receiver.outcomes[6].evidence.outcome == GPSCommandOutcome::ReadbackVerified);
    feed(driver, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n", 1);
    CHECK(std::abs(position.latitude_deg - 48.1173) < 1e-7);
    CHECK(std::abs(position.altitude_ellipsoid_m - 592.3) < 0.01);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);
    noPersistence(receiver);
}

void identityAndRoleSafety()
{
    for (const std::string& identity : {sentence("PQTMVERNO,LG580P03AANR01A03S,2024/04/30,10:53:07"),
                                        sentence("PQTMVERNO,LG290P99AANR01A03S,2024/04/30,10:53:07"),
                                        std::string("$PQTMVERNO,LG290P03AANR01A03S,2024/04/30,10:53:07*00\r\n")}) {
        gps_test_time = 0;
        Receiver receiver;
        receiver.identity = identity;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, {}) < 0);
        CHECK(receiver.commands.size() == 1);
        CHECK(!driver.receiverReady());
    }
    for (const bool requestBase : {false, true}) {
        Receiver receiver;
        receiver.role = requestBase ? 1 : 2;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, requestBase ? surveyConfig() : GPSProtocol::GPSConfig{}) < 0);
        CHECK(receiver.commands.size() == 2);
        CHECK(!driver.receiverReady());
        noPersistence(receiver);
    }
}

void fixedECEF()
{
    Receiver receiver;
    receiver.role = 2;
    receiver.base = "2,0,0.0,0.0000,6378237.0000,0.0000,0.0";
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, fixedConfig()) == 0);
    CHECK(receiver.sent("PQTMSRR"));
    CHECK(!receiver.sent("PQTMCFGSVIN,W"));
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);
    feed(driver, sentence("PQTMSVINSTATUS,1,288282000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
    CHECK(receiver.surveys.size() == 1);
    CHECK(receiver.surveys.back().flags == 1);
    CHECK(!receiver.surveys.back().accuracyKnown);
    CHECK(std::abs(receiver.surveys.back().latitude) < 0.000001);
    CHECK(std::abs(receiver.surveys.back().longitude - 90) < 0.000001);
    CHECK(std::abs(receiver.surveys.back().altitude - 100) < 0.001);
    CHECK(receiver.surveys.back().altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    feed(driver, sentence("PQTMSVINSTATUS,1,288283000,2,,00,0,0,0.0000,6378238.0000,0.0000,0.0000"));
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    CHECK(receiver.surveys.size() == 2);
    revoked(receiver.surveys.back());
    feed(driver, sentence("PQTMSVINSTATUS,1,288284000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
    CHECK(receiver.surveys.back().flags == 1);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 2);
    CHECK(std::any_of(receiver.outcomes.begin(), receiver.outcomes.end(), [](const auto& result) {
        return result.evidence.command == "PQTMSRR" && result.evidence.outcome == GPSCommandOutcome::Written;
    }));
    noPersistence(receiver);
}

void surveyLifecycle()
{
    Receiver receiver;
    receiver.role = 2;
    receiver.queued = surveyStatus(291263000, 2, 60);
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, surveyConfig()) == 0);
    CHECK(receiver.sent("PQTMCFGSVIN,W,1,60,15.0,0.0000,0.0000,0.0000,0.0"));
    CHECK(receiver.sent("PQTMCFGMSGRATE,W,PQTMSVINSTATUS,1,1"));
    CHECK(receiver.sent("PQTMCFGMSGRATE,R,RTCM3-107X,0"));
    driver.consume(CORRECTION);
    feed(driver, surveyStatus(291263000, 2, 60));
    CHECK(receiver.surveys.empty());
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);
    feed(driver, sentence("PQTMSVINSTATUS,1,291263500,1,,11,0,60,0,0,0,0"));
    CHECK(receiver.surveys.back().flags == 2);
    CHECK(std::isnan(receiver.surveys.back().latitude));
    CHECK(std::isnan(receiver.surveys.back().longitude));
    CHECK(std::isnan(receiver.surveys.back().altitude));
    CHECK(!receiver.surveys.back().accuracyKnown);
    receiver.chunk = 1;
    receiver.queued = PROGRESS;
    CHECK(driver.receive(1000) & GPSDecodedBatch::PROTOCOL_ACTIVITY);
    CHECK(receiver.surveys.back().flags == 2);
    CHECK(receiver.surveys.back().duration == 1);
    CHECK(receiver.surveys.back().accuracyKnown);
    CHECK(receiver.surveys.back().mean_accuracy == 1869);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);
    feed(driver, COMPLETE, 2);
    CHECK(receiver.surveys.back().flags == 1);
    CHECK(receiver.surveys.back().duration == 60);
    CHECK(receiver.surveys.back().mean_accuracy == 1808);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    gps_test_time += 5000001;
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    revoked(receiver.surveys.back());
    const auto expiredReports = receiver.surveys.size();
    feed(driver, COMPLETE);
    CHECK(receiver.surveys.size() == expiredReports);
    feed(driver, sentence("PQTMSVINSTATUS,1,291324000,0,,11,0,60,0,0,0,0"));
    CHECK(receiver.surveys.back().flags == 0);
    CHECK(!receiver.surveys.back().accuracyKnown);
    CHECK(std::isnan(receiver.surveys.back().latitude));
    CHECK(std::isnan(receiver.surveys.back().longitude));
    CHECK(std::isnan(receiver.surveys.back().altitude));
    feed(driver, COMPLETE);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    feed(driver, PROGRESS);
    feed(driver, COMPLETE);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    CHECK(receiver.surveys.back().flags == 0);
    feed(driver, surveyStatus(291325000, 1, 1));
    feed(driver, surveyStatus(291326000, 2, 60));
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 2);
    feed(driver, sentence("PQTMSVINSTATUS,1,291327000,2,,11,60,60,nan,0,0,1"));
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 2);
    revoked(receiver.surveys.back());
    feed(driver, COMPLETE);
    CHECK(receiver.surveys.back().flags == 0);
    feed(driver, surveyStatus(291328000, 2, 60));
    CHECK(receiver.surveys.back().flags == 1);
    feed(driver, BOOT);
    CHECK(!driver.receiverReady());
    revoked(receiver.surveys.back());
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 2);
    noPersistence(receiver);
}

void numericStatusFields()
{
    for (const std::string_view invalid :
         {"", "nan", "inf", "1e9999", "6378137junk", " 6378137", "6378137 ", "+6378137"}) {
        gps_test_time = 0;
        Receiver receiver;
        receiver.role = 2;
        receiver.periodicStatus = false;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, surveyConfig()) == 0);
        feed(driver, sentence("PQTMSVINSTATUS,1,291324000,2,,11,60,60,6.378137e6,-0.0,0.0000,1.25e-1"));
        CHECK(!receiver.surveys.empty());
        CHECK(receiver.surveys.back().flags == 1);
        CHECK(receiver.surveys.back().mean_accuracy == 125);
        CHECK(std::abs(receiver.surveys.back().latitude) < 1e-7);
        CHECK(std::abs(receiver.surveys.back().longitude) < 1e-7);
        feed(driver, sentence("PQTMSVINSTATUS,1,291325000,2,,11,60,60," + std::string(invalid) + ",0,0,1"));
        revoked(receiver.surveys.back());
        driver.consume(CORRECTION);
        CHECK(receiver.corrections == 0);
    }
}

void malformedAndMixedFraming()
{
    Receiver receiver;
    receiver.role = 2;
    GPSNativePositionReport position{};
    GPSNativeQuectel driver(receiver.io(), &position);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, surveyConfig()) == 0);
    const auto writes = receiver.commands.size();
    for (const auto& line : {std::string("$PQTMSVINSTATUS,1,1000,2,,11,60,60,6378137,0,0,1*00\r\n"),
                             sentence("PQTMSVINSTATUS,2,1000,2,,11,60,60,6378137,0,0,1"),
                             sentence("PQTMSVINSTATUS,1,1001,2,,11,60,60,nan,0,0,1"),
                             sentence("PQTMSVINSTATUS,1,1002,2,,11,60,60,6378137,0,0,-1"),
                             sentence("PQTMSVINSTATUS,1,1003,2,,11,60,60,6378137,0,0,1,extra"),
                             sentence("PQTMSVINSTATUS,1,1004,2,,11,60,60,6378137,0,0"),
                             sentence("PQTMSVINSTATUS,1,1005,9,,11,60,60,6378137,0,0,1"),
                             sentence("PQTMSVINSTATUS,1,604800000,2,,11,60,60,6378137,0,0,1"),
                             sentence("PQTMSVINSTATUS,1,1006,2,,11,4294967296,60,6378137,0,0,1")}) {
        feed(driver, line);
    }
    CHECK(receiver.surveys.empty());
    // A checksum-valid vendor sentence in a binary payload belongs exclusively to RTCM.
    std::vector<uint8_t> nested(3 + PROGRESS.size());
    nested[0] = 0xd3;
    nested[2] = static_cast<uint8_t>(PROGRESS.size());
    std::copy(PROGRESS.begin(), PROGRESS.end(), nested.begin() + 3);
    const auto crc = RTCMFramer::crc24q(nested);
    nested.push_back(crc >> 16);
    nested.push_back(crc >> 8);
    nested.push_back(crc);
    CHECK(RTCMFramer::isValidFrame(std::span<const uint8_t>(nested)));
    driver.consume(nested);
    CHECK(receiver.surveys.empty());
    feed(driver, std::string(5000, 'x') + "\r\n");
    feed(driver, PROGRESS);
    feed(driver, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    feed(driver, COMPLETE);
    CHECK(receiver.surveys.size() == 2);
    CHECK(std::abs(position.latitude_deg - 48.1173) < 1e-7);
    CHECK(RTCMFramer::isValidFrame(std::span<const uint8_t>(CORRECTION)));
    for (const auto byte : CORRECTION) {
        driver.consume({&byte, 1});
    }
    CHECK(receiver.corrections == 1);
    CHECK(receiver.commands.size() == writes);
}

void scheduledShortSurvey()
{
    for (const bool previouslyEnabled : {false, true}) {
        for (const size_t chunk : {size_t(1), size_t(150)}) {
            gps_test_time = 0;
            Receiver receiver;
            receiver.role = 2;
            receiver.base = "1,1,15.0,0,0,0,0";
            receiver.responseDelayUs = 400000;
            receiver.chunk = chunk;
            if (previouslyEnabled) {
                receiver.rates["PQTMSVINSTATUS"] = "PQTMSVINSTATUS,1,1";
            }
            bool savedBaseVerified = false;
            uint64_t savedBaseVerifiedAt = 0;
            auto io = receiver.io();
            const auto captureCommand = io.commandFinished;
            io.commandFinished = [&](const GPSCommandResult& result) {
                captureCommand(result);
                if (receiver.sent("PQTMSRR") && result.evidence.command == "PQTMCFGSVIN,R" &&
                    result.evidence.outcome == GPSCommandOutcome::ReadbackVerified) {
                    savedBaseVerified = true;
                    savedBaseVerifiedAt = gps_test_time;
                }
            };
            const auto captureDecoded = io.decoded;
            io.decoded = [&](const GPSDecodedBatch& batch) {
                for (const auto& event : batch.events) {
                    if (const auto* report = std::get_if<GPSNativeSurveyReport>(&event); report && report->flags == 1) {
                        CHECK(savedBaseVerified);
                    }
                }
                captureDecoded(batch);
            };
            GPSNativeQuectel driver(std::move(io), nullptr);
            auto config = surveyConfig();
            config.base.surveyInDurationSecs = 1;
            unsigned baud = 460800;
            CHECK(driver.configure(baud, config) == 0);
            CHECK(savedBaseVerified);
            CHECK(receiver.nativeStoredSurvey);
            CHECK(receiver.surveys.back().flags == 1);
            CHECK(receiver.surveys.back().duration == 1);
            CHECK(receiver.surveys.back().mean_accuracy == 1250);
            CHECK(std::none_of(receiver.surveys.begin(), receiver.surveys.end(),
                               [](const auto& report) { return report.flags == 2; }));
            if (previouslyEnabled) {
                CHECK(receiver.surveys.front().timestamp < savedBaseVerifiedAt);
            }
            driver.consume(CORRECTION);
            CHECK(receiver.corrections == 1);
            noPersistence(receiver);
            CHECK(receiver.saves == 0);

            // Continuing navigation must not hide missing base-status telemetry.
            receiver.periodicStatus = false;
            receiveUntil(driver, gps_test_time + 7000000);
            CHECK(receiver.positions > 0);
            revoked(receiver.surveys.back());
            driver.consume(CORRECTION);
            CHECK(receiver.corrections == 1);
        }
    }
}

void measurementOrderAndRollover()
{
    gps_test_time = 0;
    Receiver receiver;
    receiver.role = 2;
    receiver.periodicStatus = false;
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, surveyConfig()) == 0);
    const auto complete = surveyStatus(604799000, 2, 60);
    feed(driver, complete);
    CHECK(receiver.surveys.back().flags == 1);
    const size_t reports = receiver.surveys.size();
    const auto receivedAt = receiver.surveys.back().timestamp;
    gps_test_time += 4000000;
    feed(driver, complete, 1);
    CHECK(receiver.surveys.size() == reports);
    CHECK(receiver.surveys.back().timestamp == receivedAt);
    gps_test_time += 1000001;
    feed(driver, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    revoked(receiver.surveys.back());
    feed(driver, complete);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);

    feed(driver, surveyStatus(0, 1, 1));
    CHECK(receiver.surveys.back().flags == 2);
    feed(driver, surveyStatus(604798000, 2, 60));
    CHECK(receiver.surveys.back().flags == 2);
    feed(driver, surveyStatus(0, 2, 60));  // Same measurement epoch cannot change an accepted observation.
    CHECK(receiver.surveys.back().flags == 2);
    feed(driver, surveyStatus(1000, 2, 60));
    CHECK(receiver.surveys.back().flags == 1);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    feed(driver, surveyStatus(2000, 0, 0));
    CHECK(receiver.surveys.back().flags == 0);
    feed(driver, surveyStatus(0, 1, 1));
    feed(driver, surveyStatus(1000, 2, 60));
    CHECK(receiver.surveys.back().flags == 0);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
}

void cancellationRevokesPublishedState()
{
    gps_test_time = 0;
    Receiver receiver;
    receiver.role = 2;
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, surveyConfig()) == 0);
    feed(driver, COMPLETE);
    CHECK(receiver.surveys.back().flags == 1);
    receiver.failed = true;
    receiver.fault = Receiver::Fault::Cancel;
    CHECK(driver.receive(1000) == GPSProtocol::ReadCancelled);
    CHECK(!driver.receiverReady());
    revoked(receiver.surveys.back());
    feed(driver, surveyStatus(291324000, 2, 60));
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 0);
    CHECK(receiver.surveys.back().flags == 0);
}

void transactionFailures()
{
    for (const auto fault : {Receiver::Fault::Reject, Receiver::Fault::Silent, Receiver::Fault::Cancel,
                             Receiver::Fault::Partial, Receiver::Fault::Readback, Receiver::Fault::Checksum}) {
        gps_test_time = 0;
        gps_test_warnings.clear();
        Receiver receiver;
        receiver.failure = fault == Receiver::Fault::Readback ? "PQTMCFGMSGRATE,R,GGA" : "PQTMCFGMSGRATE,W,GGA";
        receiver.fault = fault;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, {}) < 0);
        CHECK(!driver.receiverReady());
        CHECK(receiver.sent(receiver.failure));
        CHECK(!receiver.sent("PQTMCFGMSGRATE,W,GST"));
        CHECK(receiver.outcomes.back().evidence.outcome ==
              (fault == Receiver::Fault::Cancel    ? GPSCommandOutcome::Cancelled
               : fault == Receiver::Fault::Partial ? GPSCommandOutcome::TransportError
               : fault == Receiver::Fault::Silent || fault == Receiver::Fault::Checksum ? GPSCommandOutcome::TimedOut
                                                                                        : GPSCommandOutcome::Rejected));
        if (fault == Receiver::Fault::Cancel) {
            CHECK(gps_test_warnings.empty());
        }
        if (fault == Receiver::Fault::Partial) {
            CHECK(receiver.outcomes.back().evidence.writtenBytes == 4);
            CHECK(receiver.outcomes.back().evidence.uncertainBytes > 0);
        }
        CHECK(gps_test_time < 5000000);
        noPersistence(receiver);
    }
}

void restartAndConfigurationSafety()
{
    for (const bool silence : {false, true}) {
        gps_test_time = 0;
        Receiver receiver;
        receiver.role = 2;
        receiver.silentAfterReset = silence;
        receiver.roleAfterReset = silence ? 0 : 1;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, surveyConfig()) < 0);
        CHECK(receiver.sent("PQTMSRR"));
        CHECK(!receiver.sent("PQTMCFGMSGRATE,W"));
        CHECK(!driver.receiverReady());
        CHECK(gps_test_time < 12000000);
    }
    for (const auto fault : {Receiver::Fault::Reject, Receiver::Fault::Silent, Receiver::Fault::Cancel,
                             Receiver::Fault::Partial, Receiver::Fault::Checksum}) {
        gps_test_time = 0;
        Receiver receiver;
        receiver.failure = "PQTMSRR";
        receiver.fault = fault;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, {}) < 0);
        CHECK(!driver.receiverReady());
        CHECK(!receiver.sent("PQTMCFGMSGRATE,W"));
        CHECK(gps_test_time < 12000000);
    }
    for (const auto base : {"1,59,15.0,0,0,0,0", "1,60,14.0,0,0,0,0", "1,60,15.0,0,0,0,1", "2,0,0.0,6378137,0,0,0"}) {
        Receiver receiver;
        receiver.role = 2;
        receiver.base = base;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, surveyConfig()) < 0);
        CHECK(!receiver.sent("PQTMCFGSVIN,W"));
        CHECK(!receiver.sent("PQTMSRR"));
    }
    {
        Receiver receiver;
        receiver.roleAfterReset = 2;  // An unsaved rover selection must not pass for an active rover.
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, {}) < 0);
        CHECK(!driver.receiverReady());
        CHECK(!receiver.sent("PQTMCFGMSGRATE,W"));
    }
    Receiver receiver;
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    for (unsigned variation = 0; variation < 3; ++variation) {
        auto config = surveyConfig();
        if (variation == 0) {
            config.base.surveyMode = GPSBaseStationConfig::SurveyMode::ReceiverManaged;
        } else if (variation == 1) {
            config.base.surveyInDurationSecs = 86401;
        } else {
            config.base.surveyInAccMeters = 1000.01;
        }
        CHECK(driver.configure(baud, config) < 0);
        CHECK(receiver.commands.empty());
    }
}

void requiredBaseCommands()
{
    for (const auto* command :
         {"PQTMCFGFIXRATE,R", "PQTMCFGSVIN,R", "PQTMCFGSVIN,W", "PQTMCFGMSGRATE,W,PQTMSVINSTATUS",
          "PQTMCFGMSGRATE,R,PQTMSVINSTATUS", "PQTMCFGMSGRATE,W,RTCM3-1005", "PQTMCFGMSGRATE,R,RTCM3-1005",
          "PQTMCFGMSGRATE,W,RTCM3-107X", "PQTMCFGMSGRATE,R,RTCM3-107X"}) {
        for (const auto fault : {Receiver::Fault::Reject, Receiver::Fault::Silent, Receiver::Fault::Cancel}) {
            gps_test_time = 0;
            gps_test_warnings.clear();
            Receiver receiver;
            receiver.role = 2;
            receiver.failure = command;
            receiver.fault = fault;
            GPSNativeQuectel driver(receiver.io(), nullptr);
            unsigned baud = 460800;
            CHECK(driver.configure(baud, surveyConfig()) < 0);
            CHECK(receiver.commands.back().starts_with(command));
            CHECK(!driver.receiverReady());
            CHECK(receiver.outcomes.back().evidence.outcome ==
                  (fault == Receiver::Fault::Reject   ? GPSCommandOutcome::Rejected
                   : fault == Receiver::Fault::Silent ? GPSCommandOutcome::TimedOut
                                                      : GPSCommandOutcome::Cancelled));
            if (fault == Receiver::Fault::Cancel) {
                CHECK(gps_test_warnings.empty());
            }
            noPersistence(receiver);
        }
    }
}

void roleTransition()
{
    Receiver receiver;
    receiver.role = 2;
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, surveyConfig()) == 0);
    feed(driver, PROGRESS);
    feed(driver, COMPLETE);
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    receiver.commands.clear();
    CHECK(driver.configure(baud, {}) < 0);
    CHECK(receiver.commands.size() == 2);
    CHECK(!driver.receiverReady());
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    // An externally saved/rebooted role can be used on the next explicit retry.
    receiver.role = 1;
    receiver.savedRole = 1;
    CHECK(driver.configure(baud, {}) == 0);
    CHECK(driver.receiverReady());
    driver.consume(CORRECTION);
    CHECK(receiver.corrections == 1);
    noPersistence(receiver);
}

void managedChanges()
{
    for (const bool fixed : {false, true}) {
        gps_test_time = 0;
        Receiver receiver;
        receiver.base = "0,0,0,0,0,0,0";
        GPSNativeQuectel driver(receiver.io(), nullptr);
        auto config = fixed ? fixedConfig() : surveyConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(driver.receiverReady());
        CHECK(receiver.commands[0] == "PQTMVERNO");
        CHECK(receiver.commands[1] == "PQTMCFGRCVRMODE,R");
        CHECK(receiver.commands[2] == "PQTMSRR");
        CHECK(receiver.commands[3] == "PQTMVERNO");
        CHECK(receiver.commands[4] == "PQTMCFGRCVRMODE,R");
        CHECK(receiver.commands[5] == "PQTMCFGRCVRMODE,W,2");
        CHECK(receiver.commands[6] == "PQTMCFGRCVRMODE,R");
        CHECK(receiver.commands[7] == "PQTMSAVEPAR");
        CHECK(receiver.commands[8] == "PQTMSRR");
        CHECK(receiver.savedRole == 2);
        CHECK(receiver.savedBase == (fixed ? "2,0,0,0.0000,6378237.0000,0.0000,0" : "1,60,15.000000000,0,0,0,0"));
        CHECK(receiver.saves == 2);  // Role must be active before configuring base parameters.
        CHECK(std::count(receiver.commands.begin(), receiver.commands.end(), "PQTMSRR") == 3);
        CHECK(std::count_if(receiver.outcomes.begin(), receiver.outcomes.end(), [](const auto& outcome) {
                  return outcome.evidence.command == "PQTMSAVEPAR" &&
                         outcome.evidence.outcome == GPSCommandOutcome::Acknowledged;
              }) == 2);
        CHECK(!receiver.sent("PQTMRESTOREPAR"));
        CHECK(!receiver.sent("PQTMCOLD"));
        driver.consume(CORRECTION);
        CHECK(receiver.corrections == 0);
        if (fixed) {
            feed(driver, sentence("PQTMSVINSTATUS,1,1000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
        } else {
            feed(driver, PROGRESS);
            feed(driver, COMPLETE);
        }
        driver.consume(CORRECTION);
        CHECK(receiver.corrections == 1);
        GPSProtocol::GPSConfig rover;
        CHECK(driver.configure(baud, rover) < 0);
        CHECK(receiver.saves == 2);
        CHECK(!driver.receiverReady());
        rover.allowPersistentChanges = true;
        CHECK(driver.configure(baud, rover) == 0);
        CHECK(receiver.savedRole == 1);
        CHECK(receiver.saves == 3);
        driver.consume(CORRECTION);
        CHECK(receiver.corrections == 1);
    }
}

void managedBaseValuesAndNoUnnecessarySaves()
{
    for (const bool distanceSupported : {false, true}) {
        Receiver receiver;
        receiver.role = 2;
        receiver.base = distanceSupported ? "1,60,15,0,0,0,2.5" : "0,0,0,0,0,0";
        GPSNativeQuectel driver(receiver.io(), nullptr);
        auto config = surveyConfig();
        config.allowPersistentChanges = true;
        config.base.surveyInDurationSecs = 3600;
        config.base.surveyInAccMeters = 1.25;
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.saves == 1);
        CHECK(!receiver.sent("PQTMCFGRCVRMODE,W"));
        CHECK(receiver.savedBase == (distanceSupported ? "1,3600,1.250000000,0,0,0,0" : "1,3600,1.250000000,0,0,0"));
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.saves == 1);
    }
    for (const auto configType : {0, 1, 2}) {
        Receiver receiver;
        receiver.role = configType == 0 ? 1 : 2;
        if (configType == 2) {
            receiver.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
        }
        auto config = configType == 0 ? GPSProtocol::GPSConfig{} : configType == 1 ? surveyConfig() : fixedConfig();
        config.allowPersistentChanges = true;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.saves == 0);
        noPersistence(receiver);
    }
    Receiver receiver;
    receiver.identity = sentence("PQTMVERNO,LG580P03AANR01A03S,2024/04/30,10:53:07");
    auto config = fixedConfig();
    config.allowPersistentChanges = true;
    GPSNativeQuectel driver(receiver.io(), nullptr);
    unsigned baud = 460800;
    CHECK(driver.configure(baud, config) < 0);
    CHECK(receiver.commands == std::vector<std::string>{"PQTMVERNO"});
}

void managedFailures()
{
    for (const auto* command : {"PQTMCFGRCVRMODE,W", "PQTMCFGSVIN,W", "PQTMSAVEPAR", "PQTMSRR"}) {
        for (const auto fault : {Receiver::Fault::Reject, Receiver::Fault::Silent, Receiver::Fault::Cancel,
                                 Receiver::Fault::Partial, Receiver::Fault::Checksum}) {
            gps_test_time = 0;
            gps_test_warnings.clear();
            Receiver receiver;
            receiver.role = std::string_view(command) == "PQTMCFGSVIN,W" ? 2 : 1;
            receiver.base = "0,0,0,0,0,0,0";
            receiver.failure = command;
            receiver.failAfterSaves = std::string_view(command) == "PQTMSRR" ? 1 : 0;
            receiver.fault = fault;
            GPSNativeQuectel driver(receiver.io(), nullptr);
            auto config = fixedConfig();
            config.allowPersistentChanges = true;
            unsigned baud = 460800;
            CHECK(driver.configure(baud, config) < 0);
            CHECK(!driver.receiverReady());
            CHECK(!receiver.sent("PQTMCFGMSGRATE,W"));
            CHECK(receiver.sent(command));
            CHECK(!receiver.sent("PQTMRESTOREPAR"));
            CHECK(!receiver.sent("PQTMCOLD"));
            CHECK(gps_test_time < 15000000);
            if (std::string_view(command) == "PQTMSRR") {
                CHECK(receiver.savedRole == 2);
                CHECK(driver.ioErrorDetail().contains("flash save was acknowledged"));
                CHECK(driver.ioErrorDetail().contains("no rollback"));
                CHECK(!gps_test_warnings.empty());
                if (fault == Receiver::Fault::Reject) {
                    CHECK(driver.ioErrorDetail().contains("rejected PQTMSRR"));
                }
                CHECK(std::any_of(receiver.outcomes.begin(), receiver.outcomes.end(), [](const auto& result) {
                    return result.evidence.command == "PQTMSAVEPAR" &&
                           result.evidence.outcome == GPSCommandOutcome::Acknowledged;
                }));
            } else if (std::string_view(command) == "PQTMSAVEPAR" && fault != Receiver::Fault::Reject) {
                CHECK(driver.ioErrorDetail().contains("flash save may"));
                CHECK(driver.ioErrorDetail().contains("no rollback"));
                CHECK(!gps_test_warnings.empty());
                CHECK(std::count(receiver.commands.begin(), receiver.commands.end(), "PQTMSRR") == 1);
                CHECK(receiver.commands.back() == "PQTMSAVEPAR");
            } else {
                CHECK(receiver.saves == 0);
                if (fault == Receiver::Fault::Cancel) {
                    CHECK(gps_test_warnings.empty());
                }
            }
        }
    }
}

void managedReadbackFailures()
{
    for (const bool roleReadback : {false, true}) {
        for (const bool afterSave : {false, true}) {
            Receiver receiver;
            receiver.role = roleReadback ? 1 : 2;
            receiver.base = "0,0,0,0,0,0,0";
            receiver.failure = roleReadback ? "PQTMCFGRCVRMODE,R" : "PQTMCFGSVIN,R";
            receiver.failureOccurrence = afterSave ? 1 : roleReadback ? 3 : 2;
            receiver.failAfterSaves = afterSave ? 1 : 0;
            receiver.fault = Receiver::Fault::Readback;
            receiver.wrongReadback =
                roleReadback ? sentence("PQTMCFGRCVRMODE,OK,1") : sentence("PQTMCFGSVIN,OK,2,0,0,1,6378237,0,0");
            GPSNativeQuectel driver(receiver.io(), nullptr);
            auto config = fixedConfig();
            config.allowPersistentChanges = true;
            unsigned baud = 460800;
            CHECK(driver.configure(baud, config) < 0);
            CHECK(!driver.receiverReady());
            CHECK(receiver.outcomes.back().evidence.outcome == GPSCommandOutcome::Rejected);
            CHECK(receiver.saves == (afterSave ? 1 : 0));
            CHECK(!receiver.sent("PQTMCFGMSGRATE,W"));
            if (afterSave) {
                CHECK(driver.ioErrorDetail().contains("flash save was acknowledged"));
                CHECK(driver.ioErrorDetail().contains("no rollback"));
            }
        }
    }
}

void managedPersistenceScope()
{
    {
        Receiver receiver;
        receiver.role = 2;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        receiver.savedRole = 1;  // A pending role readback must not trigger an unnecessary save.
        GPSProtocol::GPSConfig config;
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.savedRole == 1);
        noPersistence(receiver);
    }
    {
        Receiver receiver;
        receiver.role = 2;
        receiver.base = "0,0,0,0,0,0,0";
        receiver.rates["RMC"] = "RMC,7";
        receiver.rates["GGA"] = "GGA,2";
        GPSNativeQuectel driver(receiver.io(), nullptr);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.saves == 1);
        CHECK(receiver.savedRates.at("RMC") == "RMC,7");
        CHECK(receiver.rates.at("RMC") == "RMC,7");
        CHECK(receiver.savedRates.at("GGA") == "GGA,2");
        CHECK(receiver.rates.at("GGA") == "GGA,1");
    }
    {
        Receiver receiver;
        receiver.failure = "PQTMCFGSVIN,W";
        receiver.failAfterSaves = 1;
        GPSNativeQuectel driver(receiver.io(), nullptr);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) < 0);
        CHECK(receiver.savedRole == 2);
        CHECK(receiver.savedBase == "1,60,15.0,0.0000,0.0000,0.0000,0.0");
        CHECK(receiver.saves == 1);
        CHECK(driver.ioErrorDetail().contains("flash save was acknowledged"));
        CHECK(driver.ioErrorDetail().contains("no rollback"));
    }
    for (const auto& [latitude, expected] : {std::pair{90.0, "2,0,0,0.0000,0.0000,6356752.3142,0"},
                                             std::pair{45.0, "2,0,0,3194419.1451,3194419.1451,4487348.4089,0"}}) {
        Receiver receiver;
        receiver.role = 2;
        receiver.base = "0,0,0,0,0,0,0";
        GPSNativeQuectel driver(receiver.io(), nullptr);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        config.base.fixedPosition = {.latitudeDegrees = latitude, .longitudeDegrees = 45, .altitudeMeters = 0};
        unsigned baud = 460800;
        CHECK(driver.configure(baud, config) == 0);
        CHECK(receiver.savedBase == expected);
    }
}
}  // namespace

class GPSProtocolQuectelTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolQuectelTest::_protocol()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        positionAndEvidence();
        identityAndRoleSafety();
        fixedECEF();
        surveyLifecycle();
        numericStatusFields();
        malformedAndMixedFraming();
        scheduledShortSurvey();
        measurementOrderAndRollover();
        cancellationRevokesPublishedState();
        transactionFailures();
        restartAndConfigurationSafety();
        requiredBaseCommands();
        roleTransition();
        managedChanges();
        managedBaseValuesAndNoUnnecessarySaves();
        managedFailures();
        managedReadbackFailures();
        managedPersistenceScope();
    } catch (const std::exception& exception) {
        QFAIL(exception.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolQuectelTest, TestLabel::Unit)

#include "gps-quectel-test.moc"
