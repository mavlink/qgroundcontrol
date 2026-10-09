#include "QuectelProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "NMEASentence.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Quectel/QuectelCodec.h"
#include "Quectel/QuectelPlan.h"
#include "RTCMFramer.h"

using namespace std::chrono_literals;
using namespace GPSTest;

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

using Receiver = ModelReceiver<QuectelReceiverModel>;

GPSConfig surveyConfig()
{
    GPSConfig config;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 15;
    return config;
}

GPSConfig fixedConfig()
{
    auto config = surveyConfig();
    config.base.mode = GPSBaseStationConfig::Fixed{};
    std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position = {
        .latitudeDegrees = 0, .longitudeDegrees = 90, .altitudeMeters = 100};
    return config;
}

void feed(GPSProtocolRuntime& driver, std::string_view line, size_t chunk = 3)
{
    while (!line.empty()) {
        const auto size = std::min(line.size(), chunk);
        driver.consume({reinterpret_cast<const uint8_t*>(line.data()), size});
        line.remove_prefix(size);
    }
}

void revoked(const GPSSurveyReport& report)
{
    QVERIFY(!report.valid);
    QVERIFY(!report.active);
    QVERIFY(!report.meanAccuracyMeters);
    QCOMPARE(report.duration, std::chrono::seconds(0));
    QVERIFY(std::isnan(report.position.latitudeDegrees));
    QVERIFY(std::isnan(report.position.longitudeDegrees));
    QVERIFY(std::isnan(report.position.altitudeMeters));
}

uint32_t surveyMeanAccuracyMillimeters(const GPSSurveyReport& report)
{
    return report.meanAccuracyMeters ? static_cast<uint32_t>(std::llround(*report.meanAccuracyMeters * 1000)) : 0;
}

std::string surveyStatus(unsigned tow, unsigned validity, unsigned observations, unsigned count = 60)
{
    return nmeaSentence("PQTMSVINSTATUS,1," + std::to_string(tow) + ',' + std::to_string(validity) + ",,11," +
                        std::to_string(observations) + ',' + std::to_string(count) +
                        ",-2005559.8481,5411823.1873,2706139.3995,1.8075");
}

void receiveUntil(GPSProtocolRuntime& driver, const GPSTestClock& clock, uint64_t deadline)
{
    unsigned slices = 0;
    while (clock.nowUs() < deadline) {
        QVERIFY(++slices < 1000);
        driver.receive(1000ms);
        QVERIFY(driver.receiverReady());
    }
}

void noPersistence(const Receiver& receiver)
{
    QVERIFY(!receiver.model.sent("PQTMSAVEPAR"));
    QVERIFY(!receiver.model.sent("PQTMRESTOREPAR"));
    QVERIFY(!receiver.model.sent("PQTMCOLD"));
    QVERIFY(!receiver.model.sent("PQTMCFGRCVRMODE,W"));
}

void identityAndRoleSafety(GPSTestClock& clock)
{
    for (const std::string& identity : {nmeaSentence("PQTMVERNO,LG290P99AANR01A03S,2024/04/30,10:53:07"),
                                        std::string("$PQTMVERNO,LG290P03AANR01A03S,2024/04/30,10:53:07*00\r\n")}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.identity = identity;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(!driver.configure(surveyConfig(), baud));
        QCOMPARE(receiver.model.commands.size(), 1);
        QVERIFY(!driver.receiverReady());
    }
    Receiver receiver(clock);
    receiver.model.role = 1;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(!driver.configure(surveyConfig(), baud));
    QCOMPARE(receiver.model.commands.size(), 2);
    QVERIFY(!driver.receiverReady());
    noPersistence(receiver);
}

void fixedECEF(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.model.role = 2;
    receiver.model.base = "2,0,0.0,0.0000,6378237.0000,0.0000,0.0";
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(driver.configure(fixedConfig(), baud));
    QVERIFY(receiver.model.sent("PQTMSRR"));
    QVERIFY(!receiver.model.sent("PQTMCFGSVIN,W"));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,288282000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
    QCOMPARE(receiver.log.count<GPSSurveyReport>(), 1);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees) < 0.000001);
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees - 90) < 0.000001);
    QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters - 100) < 0.001);
    QVERIFY(!std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,288283000,2,,00,0,0,0.0000,6378238.0000,0.0000,0.0000"));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QCOMPARE(receiver.log.count<GPSSurveyReport>(), 2);
    revoked(receiver.log.latest<GPSSurveyReport>());
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,288284000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 2);
    QVERIFY(std::any_of(receiver.log.commands.begin(), receiver.log.commands.end(), [](const auto& result) {
        return result.command == "PQTMSRR" && result.outcome == GPSCommandOutcome::Written;
    }));
    noPersistence(receiver);
}

void surveyLifecycle(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.model.role = 2;
    receiver.model.queued = surveyStatus(291263000, 2, 60);
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(driver.configure(surveyConfig(), baud));
    QVERIFY(receiver.model.sent("PQTMCFGSVIN,W,1,60,15.0,0.0000,0.0000,0.0000,0.0"));
    QVERIFY(receiver.model.sent("PQTMCFGMSGRATE,W,PQTMSVINSTATUS,1,1"));
    QVERIFY(receiver.model.sent("PQTMCFGMSGRATE,R,RTCM3-107X,0"));
    driver.consume(CORRECTION);
    feed(driver, surveyStatus(291263000, 2, 60));
    QVERIFY(receiver.log.reports<GPSSurveyReport>().empty());
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,291263500,1,,11,0,60,0,0,0,0"));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
    receiver.model.chunk = 1;
    receiver.model.queued = PROGRESS;
    QVERIFY(driver.receive(1000ms).testFlag(GPSReceiveUpdate::Activity));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 1);
    QVERIFY(receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
    QCOMPARE(surveyMeanAccuracyMillimeters(receiver.log.latest<GPSSurveyReport>()), 1869);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    feed(driver, COMPLETE, 2);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 60);
    QCOMPARE(surveyMeanAccuracyMillimeters(receiver.log.latest<GPSSurveyReport>()), 1808);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    clock.advanceBy(5000001);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    revoked(receiver.log.latest<GPSSurveyReport>());
    const auto expiredReports = receiver.log.count<GPSSurveyReport>();
    feed(driver, COMPLETE);
    QCOMPARE(receiver.log.count<GPSSurveyReport>(), expiredReports);
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,291324000,0,,11,0,60,0,0,0,0"));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    QVERIFY(!receiver.log.latest<GPSSurveyReport>().meanAccuracyMeters.has_value());
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees));
    QVERIFY(std::isnan(receiver.log.latest<GPSSurveyReport>().position.altitudeMeters));
    feed(driver, COMPLETE);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    feed(driver, PROGRESS);
    feed(driver, COMPLETE);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    feed(driver, surveyStatus(291325000, 1, 1));
    feed(driver, surveyStatus(291326000, 2, 60));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 2);
    feed(driver, nmeaSentence("PQTMSVINSTATUS,1,291327000,2,,11,60,60,nan,0,0,1"));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 2);
    revoked(receiver.log.latest<GPSSurveyReport>());
    feed(driver, COMPLETE);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    feed(driver, surveyStatus(291328000, 2, 60));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    feed(driver, BOOT);
    QVERIFY(!driver.receiverReady());
    QCOMPARE(driver.errorDetail(), QStringLiteral("LG290P restarted; its base configuration is no longer verified"));
    revoked(receiver.log.latest<GPSSurveyReport>());
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 2);
    noPersistence(receiver);
}

void numericStatusFields(GPSTestClock& clock)
{
    for (const std::string_view invalid :
         {"", "nan", "inf", "1e9999", "6378137junk", " 6378137", "6378137 ", "+6378137"}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.periodicStatus = false;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(driver.configure(surveyConfig(), baud));
        feed(driver, nmeaSentence("PQTMSVINSTATUS,1,291324000,2,,11,60,60,6.378137e6,-0.0,0.0000,1.25e-1"));
        QVERIFY(!receiver.log.reports<GPSSurveyReport>().empty());
        QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
        QCOMPARE(surveyMeanAccuracyMillimeters(receiver.log.latest<GPSSurveyReport>()), 125);
        QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.latitudeDegrees) < 1e-7);
        QVERIFY(std::abs(receiver.log.latest<GPSSurveyReport>().position.longitudeDegrees) < 1e-7);
        feed(driver, nmeaSentence("PQTMSVINSTATUS,1,291325000,2,,11,60,60," + std::string(invalid) + ",0,0,1"));
        revoked(receiver.log.latest<GPSSurveyReport>());
        driver.consume(CORRECTION);
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    }
}

void malformedAndMixedFraming(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.model.role = 2;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(driver.configure(surveyConfig(), baud));
    const auto writes = receiver.model.commands.size();
    for (const auto& line : {std::string("$PQTMSVINSTATUS,1,1000,2,,11,60,60,6378137,0,0,1*00\r\n"),
                             nmeaSentence("PQTMSVINSTATUS,2,1000,2,,11,60,60,6378137,0,0,1"),
                             nmeaSentence("PQTMSVINSTATUS,1,1001,2,,11,60,60,nan,0,0,1"),
                             nmeaSentence("PQTMSVINSTATUS,1,1002,2,,11,60,60,6378137,0,0,-1"),
                             nmeaSentence("PQTMSVINSTATUS,1,1003,2,,11,60,60,6378137,0,0,1,extra"),
                             nmeaSentence("PQTMSVINSTATUS,1,1004,2,,11,60,60,6378137,0,0"),
                             nmeaSentence("PQTMSVINSTATUS,1,1005,9,,11,60,60,6378137,0,0,1"),
                             nmeaSentence("PQTMSVINSTATUS,1,604800000,2,,11,60,60,6378137,0,0,1"),
                             nmeaSentence("PQTMSVINSTATUS,1,1006,2,,11,4294967296,60,6378137,0,0,1")}) {
        feed(driver, line);
    }
    QVERIFY(receiver.log.reports<GPSSurveyReport>().empty());
    // A checksum-valid vendor sentence in a binary payload belongs exclusively to RTCM.
    const auto nested = rtcmPacket(PROGRESS);
    QVERIFY(RTCMFramer::isValidFrame(std::span<const uint8_t>(nested)));
    driver.consume(nested);
    QVERIFY(receiver.log.reports<GPSSurveyReport>().empty());
    feed(driver, std::string(5000, 'x') + "\r\n");
    feed(driver, PROGRESS);
    feed(driver, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    feed(driver, COMPLETE);
    QCOMPARE(receiver.log.count<GPSSurveyReport>(), 2);
    QVERIFY(std::abs(receiver.log.position.navigation.latitudeDegrees - 48.1173) < 1e-7);
    QVERIFY(RTCMFramer::isValidFrame(std::span<const uint8_t>(CORRECTION)));
    for (const auto byte : CORRECTION) {
        driver.consume({&byte, 1});
    }
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    QCOMPARE(receiver.model.commands.size(), writes);
}

void scheduledShortSurvey(GPSTestClock& clock)
{
    for (const bool previouslyEnabled : {false, true}) {
        for (const size_t chunk : {size_t(1), size_t(150)}) {
            clock.reset();
            Receiver receiver(clock);
            receiver.model.role = 2;
            receiver.model.base = "1,1,15.0,0,0,0,0";
            receiver.model.responseDelayUs = 400000;
            receiver.model.chunk = chunk;
            if (previouslyEnabled) {
                receiver.model.rates["PQTMSVINSTATUS"] = "PQTMSVINSTATUS,1,1";
            }
            bool savedBaseVerified = false;
            uint64_t savedBaseVerifiedAt = 0;
            auto observer = receiver.observer();
            const auto captureCommand = observer.commandFinished;
            observer.commandFinished = [&](const GPSConfigurationEvidence& result) {
                captureCommand(result);
                if (receiver.model.sent("PQTMSRR") && result.command == "PQTMCFGSVIN,R" &&
                    result.outcome == GPSCommandOutcome::ReadbackVerified) {
                    savedBaseVerified = true;
                    savedBaseVerifiedAt = clock.nowUs();
                }
            };
            const auto captureDecoded = observer.decoded;
            observer.decoded = [&](const GPSEventBatch& batch) {
                for (const auto& event : batch.events) {
                    if (const auto* report = std::get_if<GPSSurveyReport>(&event);
                        report && surveyFlags(*report) == 1) {
                        QVERIFY(savedBaseVerified);
                    }
                }
                captureDecoded(batch);
            };
            GPSProtocolRuntime driver(Quectel::FAMILY, receiver.io(), std::move(observer));
            auto config = surveyConfig();
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 1s;
            unsigned baud = 460800;
            QVERIFY(driver.configure(config, baud));
            QVERIFY(savedBaseVerified);
            QVERIFY(receiver.model.nativeStoredSurvey);
            QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
            QCOMPARE(surveyDuration(receiver.log.latest<GPSSurveyReport>()), 1);
            QCOMPARE(surveyMeanAccuracyMillimeters(receiver.log.latest<GPSSurveyReport>()), 1250);
            QVERIFY(std::ranges::none_of(receiver.log.reports<GPSSurveyReport>(),
                                         [](const auto& report) { return surveyFlags(report) == 2; }));
            if (previouslyEnabled) {
                QVERIFY(receiver.log.reports<GPSSurveyReport>().front().timestampUs < savedBaseVerifiedAt);
            }
            driver.consume(CORRECTION);
            QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
            noPersistence(receiver);
            QCOMPARE(receiver.model.saves, 0);

            // Continuing navigation must not hide missing base-status telemetry.
            receiver.model.periodicStatus = false;
            receiveUntil(driver, clock, clock.nowUs() + 7000000);
            QVERIFY(receiver.log.count<GPSDecodedPosition>() > 0);
            revoked(receiver.log.latest<GPSSurveyReport>());
            driver.consume(CORRECTION);
            QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
        }
    }
}

void measurementOrderAndRollover(GPSTestClock& clock)
{
    clock.reset();
    Receiver receiver(clock);
    receiver.model.role = 2;
    receiver.model.periodicStatus = false;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(driver.configure(surveyConfig(), baud));
    const auto complete = surveyStatus(604799000, 2, 60);
    feed(driver, complete);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    const size_t reports = receiver.log.count<GPSSurveyReport>();
    const auto receivedAt = receiver.log.latest<GPSSurveyReport>().timestampUs;
    clock.advanceBy(4000000);
    feed(driver, complete, 1);
    QCOMPARE(receiver.log.count<GPSSurveyReport>(), reports);
    QCOMPARE(receiver.log.latest<GPSSurveyReport>().timestampUs, receivedAt);
    clock.advanceBy(1000001);
    feed(driver, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
    revoked(receiver.log.latest<GPSSurveyReport>());
    feed(driver, complete);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);

    feed(driver, surveyStatus(0, 1, 1));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    feed(driver, surveyStatus(604798000, 2, 60));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    feed(driver, surveyStatus(0, 2, 60));  // Same measurement epoch cannot change an accepted observation.
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 2);
    feed(driver, surveyStatus(1000, 2, 60));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    feed(driver, surveyStatus(2000, 0, 0));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    feed(driver, surveyStatus(0, 1, 1));
    feed(driver, surveyStatus(1000, 2, 60));
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
}

void cancellationRevokesPublishedState(GPSTestClock& clock)
{
    clock.reset();
    Receiver receiver(clock);
    receiver.model.role = 2;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(driver.configure(surveyConfig(), baud));
    feed(driver, COMPLETE);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 1);
    receiver.model.failed = true;
    receiver.model.fault = QuectelReceiverModel::Fault::Cancel;
    QCOMPARE(driver.receive(1000ms), GPSReceiveUpdates{});
    QCOMPARE(driver.error(), GPSProtocolError::Cancelled);
    QVERIFY(!driver.receiverReady());
    revoked(receiver.log.latest<GPSSurveyReport>());
    feed(driver, surveyStatus(291324000, 2, 60));
    driver.consume(CORRECTION);
    QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
    QCOMPARE(surveyFlags(receiver.log.latest<GPSSurveyReport>()), 0);
}

void restartAndConfigurationSafety(GPSTestClock& clock)
{
    for (const bool silence : {false, true}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.silentAfterReset = silence;
        receiver.model.roleAfterReset = silence ? 0 : 1;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(!driver.configure(surveyConfig(), baud));
        QVERIFY(receiver.model.sent("PQTMSRR"));
        QVERIFY(!receiver.model.sent("PQTMCFGMSGRATE,W"));
        QVERIFY(!driver.receiverReady());
        QVERIFY(clock.nowUs() < 12000000);
    }
    for (const auto fault :
         {QuectelReceiverModel::Fault::Reject, QuectelReceiverModel::Fault::Silent, QuectelReceiverModel::Fault::Cancel,
          QuectelReceiverModel::Fault::Partial, QuectelReceiverModel::Fault::Checksum}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.failure = "PQTMSRR";
        receiver.model.fault = fault;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(!driver.configure({}, baud));
        QVERIFY(!driver.receiverReady());
        QVERIFY(!receiver.model.sent("PQTMCFGMSGRATE,W"));
        QVERIFY(clock.nowUs() < 12000000);
    }
    // Saved base settings that differ from the request, without consent to rewrite them.
    for (const auto& [base, fixed] : {std::pair{"1,59,15.0,0,0,0,0", false}, std::pair{"1,60,14.0,0,0,0,0", false},
                                      std::pair{"1,60,15.0,0,0,0,1", false}, std::pair{"2,0,0.0,6378137,0,0,0", false},
                                      std::pair{"0,0,0,0,0,0,0", true}}) {
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.base = base;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(!driver.configure(fixed ? fixedConfig() : surveyConfig(), baud));
        QCOMPARE(driver.error(), GPSProtocolError::ConsentRequired);
        QVERIFY(!receiver.model.sent("PQTMCFGSVIN,W"));
        QVERIFY(!receiver.model.sent("PQTMSRR"));
    }
    {
        Receiver receiver(clock);
        receiver.model.roleAfterReset = 2;  // An unsaved rover selection must not pass for an active rover.
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(!driver.configure({}, baud));
        QVERIFY(!driver.receiverReady());
        QVERIFY(!receiver.model.sent("PQTMCFGMSGRATE,W"));
    }
    Receiver receiver(clock);
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    for (unsigned variation = 0; variation < 3; ++variation) {
        auto config = surveyConfig();
        if (variation == 0) {
            config.base.mode = GPSBaseStationConfig::ReceiverAveraging{};
        } else if (variation == 1) {
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 86401s;
        } else {
            std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1000.01;
        }
        QVERIFY(!driver.configure(config, baud));
        QVERIFY(receiver.model.commands.empty());
    }
}

void managedChanges(GPSTestClock& clock)
{
    for (const bool fixed : {false, true}) {
        clock.reset();
        Receiver receiver(clock);
        receiver.model.base = "0,0,0,0,0,0,0";
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        auto config = fixed ? fixedConfig() : surveyConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        QVERIFY(driver.configure(config, baud));
        QVERIFY(driver.receiverReady());
        QCOMPARE(receiver.model.savedRole, 2);
        QCOMPARE(receiver.model.savedBase,
                 (fixed ? "2,0,0,0.0000,6378237.0000,0.0000,0" : "1,60,15.000000000,0,0,0,0"));
        QCOMPARE(receiver.model.saves, 2);  // Role must be active before configuring base parameters.
        QCOMPARE(std::count(receiver.model.commands.begin(), receiver.model.commands.end(), "PQTMSRR"), 3);
        QCOMPARE(std::count_if(receiver.log.commands.begin(), receiver.log.commands.end(),
                               [](const auto& outcome) {
                                   return outcome.command == "PQTMSAVEPAR" &&
                                          outcome.outcome == GPSCommandOutcome::Acknowledged;
                               }),
                 2);
        QVERIFY(!receiver.model.sent("PQTMRESTOREPAR"));
        QVERIFY(!receiver.model.sent("PQTMCOLD"));
        driver.consume(CORRECTION);
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 0);
        if (fixed) {
            feed(driver, nmeaSentence("PQTMSVINSTATUS,1,1000,2,,00,0,0,0.0000,6378237.0000,0.0000,0.0000"));
        } else {
            feed(driver, PROGRESS);
            feed(driver, COMPLETE);
        }
        driver.consume(CORRECTION);
        QCOMPARE(receiver.log.count<GPSRTCMFrame>(), 1);
    }
}

void managedBaseValuesAndNoUnnecessarySaves(GPSTestClock& clock)
{
    for (const bool distanceSupported : {false, true}) {
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.base = distanceSupported ? "1,60,15,0,0,0,2.5" : "0,0,0,0,0,0";
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        auto config = surveyConfig();
        config.allowPersistentChanges = true;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 3600s;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1.25;
        unsigned baud = 460800;
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(receiver.model.saves, 1);
        QVERIFY(!receiver.model.sent("PQTMCFGRCVRMODE,W"));
        QCOMPARE(receiver.model.savedBase,
                 (distanceSupported ? "1,3600,1.250000000,0,0,0,0" : "1,3600,1.250000000,0,0,0"));
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(receiver.model.saves, 1);
    }
    for (const auto configType : {1, 2}) {
        Receiver receiver(clock);
        receiver.model.role = 2;
        if (configType == 2) {
            receiver.model.base = "2,0,0,0.0000,6378237.0000,0.0000,0";
        }
        auto config = configType == 1 ? surveyConfig() : fixedConfig();
        config.allowPersistentChanges = true;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        unsigned baud = 460800;
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(receiver.model.saves, 0);
        noPersistence(receiver);
    }
    Receiver receiver(clock);
    receiver.model.identity = nmeaSentence("PQTMVERNO,LG580P03AANR01A03S,2024/04/30,10:53:07");
    auto config = fixedConfig();
    config.allowPersistentChanges = true;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(!driver.configure(config, baud));
    QCOMPARE(receiver.model.commands, std::vector<std::string>{"PQTMVERNO"});
}

void managedReadbackFailures(GPSTestClock& clock)
{
    for (const bool roleReadback : {false, true}) {
        for (const bool afterSave : {false, true}) {
            Receiver receiver(clock);
            receiver.model.role = roleReadback ? 1 : 2;
            receiver.model.base = "0,0,0,0,0,0,0";
            receiver.model.failure = roleReadback ? "PQTMCFGRCVRMODE,R" : "PQTMCFGSVIN,R";
            receiver.model.failureOccurrence = afterSave ? 1 : roleReadback ? 3 : 2;
            receiver.model.failAfterSaves = afterSave ? 1 : 0;
            receiver.model.fault = QuectelReceiverModel::Fault::Readback;
            receiver.model.wrongReadback = roleReadback ? nmeaSentence("PQTMCFGRCVRMODE,OK,1")
                                                        : nmeaSentence("PQTMCFGSVIN,OK,2,0,0,1,6378237,0,0");
            GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
            auto config = fixedConfig();
            config.allowPersistentChanges = true;
            unsigned baud = 460800;
            QVERIFY(!driver.configure(config, baud));
            QVERIFY(!driver.receiverReady());
            QCOMPARE(receiver.log.commands.back().outcome, GPSCommandOutcome::Rejected);
            QCOMPARE(receiver.model.saves, (afterSave ? 1 : 0));
            QVERIFY(!receiver.model.sent("PQTMCFGMSGRATE,W"));
            if (afterSave) {
                QVERIFY(driver.errorDetail().contains("flash save was acknowledged"));
                QVERIFY(driver.errorDetail().contains("no rollback"));
            }
        }
    }
}

void managedPersistenceScope(GPSTestClock& clock)
{
    {
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.base = "0,0,0,0,0,0,0";
        receiver.model.rates["RMC"] = "RMC,7";
        receiver.model.rates["GGA"] = "GGA,2";
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(receiver.model.saves, 1);
        QCOMPARE(receiver.model.savedRates.at("RMC"), "RMC,7");
        QCOMPARE(receiver.model.rates.at("RMC"), "RMC,7");
        QCOMPARE(receiver.model.savedRates.at("GGA"), "GGA,2");
        QCOMPARE(receiver.model.rates.at("GGA"), "GGA,1");
    }
    {
        Receiver receiver(clock);
        receiver.model.failure = "PQTMCFGSVIN,W";
        receiver.model.failAfterSaves = 1;
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        unsigned baud = 460800;
        QVERIFY(!driver.configure(config, baud));
        QCOMPARE(receiver.model.savedRole, 2);
        QCOMPARE(receiver.model.savedBase, "1,60,15.0,0.0000,0.0000,0.0000,0.0");
        QCOMPARE(receiver.model.saves, 1);
        QVERIFY(driver.errorDetail().contains("flash save was acknowledged"));
        QVERIFY(driver.errorDetail().contains("no rollback"));
    }
    for (const auto& [latitude, expected] : {std::pair{90.0, "2,0,0,0.0000,0.0000,6356752.3142,0"},
                                             std::pair{45.0, "2,0,0,3194419.1451,3194419.1451,4487348.4089,0"}}) {
        Receiver receiver(clock);
        receiver.model.role = 2;
        receiver.model.base = "0,0,0,0,0,0,0";
        GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
        auto config = fixedConfig();
        config.allowPersistentChanges = true;
        std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position = {
            .latitudeDegrees = latitude, .longitudeDegrees = 45, .altitudeMeters = 0};
        unsigned baud = 460800;
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(receiver.model.savedBase, expected);
    }
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"identity-and-role-safety", identityAndRoleSafety},
    {"fixed-ecef", fixedECEF},
    {"survey-lifecycle", surveyLifecycle},
    {"numeric-status-fields", numericStatusFields},
    {"malformed-and-mixed-framing", malformedAndMixedFraming},
    {"scheduled-short-survey", scheduledShortSurvey},
    {"measurement-order-and-rollover", measurementOrderAndRollover},
    {"cancellation-revokes-published-state", cancellationRevokesPublishedState},
    {"restart-and-configuration-safety", restartAndConfigurationSafety},
    {"managed-changes", managedChanges},
    {"managed-base-values-and-no-unnecessary-saves", managedBaseValuesAndNoUnnecessarySaves},
    {"managed-readback-failures", managedReadbackFailures},
    {"managed-persistence-scope", managedPersistenceScope},
};
}  // namespace

void QuectelProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void QuectelProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

void QuectelProtocolTest::_commandFaults_data()
{
    using Fault = QuectelReceiverModel::Fault;
    QTest::addColumn<QByteArray>("command");
    QTest::addColumn<int>("fault");
    QTest::addColumn<GPSCommandOutcome>("outcome");
    QTest::addColumn<QString>("detail");
    const auto row = [](const char* command, Fault fault, const char* name, GPSCommandOutcome outcome,
                        const char* detail = "") {
        QTest::addRow("%s-%s", command, name) << QByteArray(command) << int(fault) << outcome << QString(detail);
    };
    // A failed transaction ends the configuration at once.
    row("PQTMCFGMSGRATE,W,GGA", Fault::Reject, "reject", GPSCommandOutcome::Rejected);
    row("PQTMCFGMSGRATE,W,GGA", Fault::Silent, "silent", GPSCommandOutcome::TimedOut);
    row("PQTMCFGMSGRATE,W,GGA", Fault::Cancel, "cancel", GPSCommandOutcome::Cancelled);
    row("PQTMCFGMSGRATE,W,GGA", Fault::Partial, "partial", GPSCommandOutcome::TransportError);
    row("PQTMCFGMSGRATE,W,GSV", Fault::Partial, "partial", GPSCommandOutcome::TransportError);
    row("PQTMCFGMSGRATE,R,GGA", Fault::Readback, "readback", GPSCommandOutcome::Rejected);
    row("PQTMCFGMSGRATE,W,GGA", Fault::Checksum, "checksum", GPSCommandOutcome::TimedOut);
    // Each base stage names its failure.
    row("PQTMCFGFIXRATE,R", Fault::Reject, "reject", GPSCommandOutcome::Rejected,
        "LG290P base settings query failed; no base change was attempted");
    row("PQTMCFGSVIN,W", Fault::Reject, "reject", GPSCommandOutcome::Rejected,
        "LG290P rejected restarting the unchanged, externally saved survey");
    row("PQTMCFGMSGRATE,W,PQTMSVINSTATUS", Fault::Reject, "reject", GPSCommandOutcome::Rejected,
        "LG290P base message output configuration/readback failed");
}

void QuectelProtocolTest::_commandFaults()
{
    QFETCH(QByteArray, command);
    QFETCH(int, fault);
    QFETCH(GPSCommandOutcome, outcome);
    QFETCH(QString, detail);
    const QTest::ThrowOnFailEnabler endRowOnFailure;
    GPSTestClock clock;
    Receiver receiver(clock);
    receiver.model.role = 2;
    receiver.model.failure = command.toStdString();
    receiver.model.fault = static_cast<QuectelReceiverModel::Fault>(fault);
    const GPSProtocolLogCapture log;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    unsigned baud = 460800;
    QVERIFY(!driver.configure(surveyConfig(), baud));
    QVERIFY(!driver.receiverReady());
    // Nothing follows the failed command.
    QVERIFY(receiver.model.commands.back().starts_with(receiver.model.failure));
    QCOMPARE(receiver.log.commands.back().outcome, outcome);
    if (!detail.isEmpty()) {
        QCOMPARE(driver.errorDetail(), detail);
    }
    if (outcome == GPSCommandOutcome::Cancelled) {
        QVERIFY(log.warnings().isEmpty());
    }
    if (static_cast<QuectelReceiverModel::Fault>(fault) == QuectelReceiverModel::Fault::Partial) {
        QCOMPARE(receiver.log.commands.back().writtenBytes, 4);
        QCOMPARE_GT(receiver.log.commands.back().acceptedBytes, 4);
    }
    QVERIFY(clock.nowUs() < 5000000);
    noPersistence(receiver);
}

void QuectelProtocolTest::_managedFaults_data()
{
    using Fault = QuectelReceiverModel::Fault;
    QTest::addColumn<QByteArray>("command");
    QTest::addColumn<int>("fault");
    for (const auto* command : {"PQTMCFGRCVRMODE,W", "PQTMCFGSVIN,W", "PQTMSAVEPAR", "PQTMSRR"}) {
        for (const auto& [fault, name] : {std::pair{Fault::Reject, "reject"}, std::pair{Fault::Silent, "silent"},
                                          std::pair{Fault::Cancel, "cancel"}, std::pair{Fault::Partial, "partial"},
                                          std::pair{Fault::Checksum, "checksum"}}) {
            QTest::addRow("%s-%s", command, name) << QByteArray(command) << int(fault);
        }
    }
}

void QuectelProtocolTest::_managedFaults()
{
    QFETCH(QByteArray, command);
    QFETCH(int, fault);
    const QTest::ThrowOnFailEnabler endRowOnFailure;
    GPSTestClock clock;
    Receiver receiver(clock);
    receiver.model.role = command == "PQTMCFGSVIN,W" ? 2 : 1;
    receiver.model.base = "0,0,0,0,0,0,0";
    receiver.model.failure = command.toStdString();
    receiver.model.failAfterSaves = command == "PQTMSRR" ? 1 : 0;
    receiver.model.fault = static_cast<QuectelReceiverModel::Fault>(fault);
    const GPSProtocolLogCapture log;
    GPSProtocolRuntime driver = receiver.runtime(Quectel::FAMILY);
    auto config = fixedConfig();
    config.allowPersistentChanges = true;
    unsigned baud = 460800;
    QVERIFY(!driver.configure(config, baud));
    QVERIFY(!driver.receiverReady());
    QVERIFY(!receiver.model.sent("PQTMCFGMSGRATE,W"));
    QVERIFY(receiver.model.sent(command.toStdString()));
    QVERIFY(!receiver.model.sent("PQTMRESTOREPAR"));
    QVERIFY(!receiver.model.sent("PQTMCOLD"));
    QVERIFY(clock.nowUs() < 15000000);
    const bool rejected = static_cast<QuectelReceiverModel::Fault>(fault) == QuectelReceiverModel::Fault::Reject;
    if (command == "PQTMSRR") {
        QCOMPARE(receiver.model.savedRole, 2);
        QVERIFY(driver.errorDetail().contains("flash save was acknowledged"));
        QVERIFY(driver.errorDetail().contains("no rollback"));
        if (rejected) {
            QVERIFY(driver.errorDetail().contains("rejected PQTMSRR"));
        }
        QVERIFY(std::ranges::any_of(receiver.log.commands, [](const auto& result) {
            return result.command == "PQTMSAVEPAR" && result.outcome == GPSCommandOutcome::Acknowledged;
        }));
    } else if (command == "PQTMSAVEPAR" && !rejected) {
        QVERIFY(driver.errorDetail().contains("flash save may"));
        QVERIFY(driver.errorDetail().contains("no rollback"));
        QCOMPARE(std::ranges::count(receiver.model.commands, "PQTMSRR"), 1);
        QCOMPARE(receiver.model.commands.back(), "PQTMSAVEPAR");
    } else {
        QCOMPARE(receiver.model.saves, 0);
        if (static_cast<QuectelReceiverModel::Fault>(fault) == QuectelReceiverModel::Fault::Cancel) {
            QVERIFY(log.warnings().isEmpty());
        }
    }
}

void QuectelProtocolTest::_codec()
{
    const QByteArray body("PQTMCFGMSGRATE,OK,GGA,1,");
    const QByteArray wire =
        QByteArray::fromStdString(nmeaSentence({body.constData(), static_cast<size_t>(body.size())}));
    const std::string_view bodyView(body.constData(), body.size());
    QCOMPARE(NMEAUtils::frame(body), wire);
    const std::string_view line(wire.constData(), wire.size() - 2);
    QCOMPARE(QuectelCodec::checkedBody(line), bodyView);
    auto corrupted = wire;
    corrupted[3] ^= 1;
    QVERIFY(QuectelCodec::checkedBody({corrupted.constData(), static_cast<size_t>(corrupted.size())}).empty());
    QVERIFY(QuectelCodec::checkedBody("$PQTMCFGMSGRATE,OK,GGA,1,*+1").empty());
    QuectelCodec::Fields fields(bodyView);
    QCOMPARE(fields.size(), size_t(5));
    QVERIFY(fields.back().empty());
    fields.pop_back();
    QCOMPARE(fields.size(), size_t(4));
    QCOMPARE(fields[2], std::string_view("GGA"));
    QCOMPARE(QuectelCodec::readback(fields, "PQTMCFGMSGRATE", true), GPSCommandOutcome::ReadbackVerified);
    const QuectelCodec::Fields overflow("PQTMCFGMSGRATE,OK,GGA,1,2,3,4,5,6,7,8,9,10");
    QVERIFY(overflow.overflowed());
    QCOMPARE(QuectelCodec::readback(overflow, "PQTMCFGMSGRATE", true), GPSCommandOutcome::Rejected);
    QCOMPARE(QuectelCodec::readback(overflow, "PQTMCFGSVIN", true), GPSCommandOutcome::Pending);
    double value = 42;
    QVERIFY(QuectelCodec::number("1.25e-1", value));
    QCOMPARE(value, 0.125);
    for (const auto invalid : {"+1", " 1", "1 ", "1junk", "nan", "inf", "1e9999"}) {
        QVERIFY(!QuectelCodec::number(invalid, value));
        QCOMPARE(value, 0.125);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(QuectelProtocolTest, TestLabel::Unit)
