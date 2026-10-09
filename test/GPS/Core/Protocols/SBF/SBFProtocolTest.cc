#include "SBFProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <numbers>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "GPSCommandChannel.h"
#include "GPSProtocolMath.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "LittleEndian.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/ScriptedFaults.h"
#include "Protocols/fixtures/GPSFixtureExpectations.h"
#include "RTCMFramer.h"
#include "SBF/SBFBlocks.h"
#include "WireFields.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

// SBF Reference Guide: every block starts with "$@"; PVTGeodetic is block 4007 and QGC reads it up to VAccuracy.
static_assert(SBF::SYNC1 == '$' && SBF::SYNC2 == '@');
static_assert(SBF::BlockId::PVT_GEODETIC == 4007);
static_assert(Wire::SIZE<SBF::PVTGeodetic> == 94);

constexpr double RAD_TO_DEG = GPSProtocolMath::RAD_TO_DEG;

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

/// The golden Septentrio model: "$R: <command>" for an accepted command and "USB1>" for the prompt, behind fault rules.
using Receiver = ModelReceiver<SBFReceiverModel>;

const QByteArray REJECTED = "$R? rejected\n";

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
    LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
    const auto& position = sbf.position;
    PVTBody fix{};
    fix.mode = 1;
    fix.latitude = 0.5;
    fix.longitude = 1.0;
    fix.nrSV = 12;
    const auto good = sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix));
    QVERIFY(sbf->consume(good).testFlag(GPSReceiveUpdate::Position));
    QVERIFY(std::abs(position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 0.00001);
    const auto received = position.navigation.timestampUs;
    QCOMPARE(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, shortPayload, 1000)), GPSReceiveUpdates{});
    QCOMPARE(position.navigation.timestampUs, received);
    QVERIFY(!sbf->consume(good).testFlag(GPSReceiveUpdate::Position));  // Duplicate receiver epoch does not republish.

    fix.cog = -2.0e10f;
    QVERIFY(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 1000)).testFlag(GPSReceiveUpdate::Position));
    QVERIFY(std::isnan(position.navigation.courseRadians));
    fix.cog = 90.0f;
    QVERIFY(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 2000)).testFlag(GPSReceiveUpdate::Position));
    QVERIFY(std::abs(position.navigation.courseRadians - std::numbers::pi / 2) < 0.00001);
}

void epochMetadata(GPSTestClock& clock)
{
    LoggedRuntime sbf(SBF::FAMILY, makeGPSRuntimeTestIO(clock));
    PVTBody fix{};
    fix.mode = 0x81;  // Stand-alone PVT in 2D mode.
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    fix.nrSV = UINT8_MAX;
    fix.hAccuracy = UINT16_MAX;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 1000));
    auto fixes = sbf.reports<GPSDecodedPosition>();
    const auto usage = sbf.reports<GPSDecodedSatelliteUsage>();
    QCOMPARE(fixes.size(), 1);
    QCOMPARE(fixes[0].navigation.fixType, GPSPositionReport::FixType::Fix2D);
    QVERIFY(!fixes[0].navigation.satellitesUsed);
    QVERIFY(usage.size() == 1 && !usage[0].usedCount);
    QVERIFY(std::isnan(fixes[0].navigation.horizontalDop));
    QVERIFY(std::isnan(fixes[0].navigation.horizontalAccuracyMeters));
    QCOMPARE(fixes[0].navigation.utcTimeUs, 0);  // GNSS time cannot be labeled UTC without a receiver UTC offset.
    fix.mode = 1;
    fix.nrSV = 0;
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 3000));
    fixes = sbf.reports<GPSDecodedPosition>();
    QCOMPARE(fixes.size(), 2);
    QCOMPARE(fixes.back().navigation.satellitesUsed, 0);
    QCOMPARE(fixes.back().navigation.fixType, GPSPositionReport::FixType::Fix3D);
    QCOMPARE(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), UINT32_MAX)), GPSReceiveUpdates{});
    QCOMPARE(sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes(fix), 4000, UINT16_MAX)), GPSReceiveUpdates{});
    QCOMPARE(sbf.reports<GPSDecodedPosition>().size(), 2);
}

void invalidCoordinates(GPSTestClock& clock)
{
    LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
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
        const auto reports = sbf.reports<GPSDecodedPosition>();
        QCOMPARE(reports.size(), previous + 1);
        QCOMPARE(reports.back().navigation.fixType, GPSPositionReport::FixType::NoFix);
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
        QVERIFY(LittleEndian::write(payload, invalid.offset, invalid.value));
        check(payload);
    }
    for (const float invalid : {NAN, 1e11f}) {
        auto payload = bytes(fix);
        QVERIFY(LittleEndian::write(payload, 26, invalid));
        check(payload);
    }
}

// Captured and synthetic fixtures

void independentValidity(GPSTestClock& clock)
{
    for (const auto& expected : GPSFixture::sbfEpochs) {
        for (const size_t chunkSize : {1u, 11u, 512u}) {
            clock.advanceBy(1000000);
            LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
            const auto& position = sbf.position;
            const auto blocks = fixtureBytes(expected.filename);
            QVERIFY(blocks);
            auto remaining = std::span(*blocks);
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                (void) sbf->consume(remaining.first(count));
                remaining = remaining.subspan(count);
            }
            QCOMPARE(sbf.reports<GPSDecodedPosition>().size(), 1);
            QCOMPARE(position.navigation.utcTimeUs, 0);
            QVERIFY(matches(position.navigation.latitudeDegrees, expected.latitude, 1e-9));
            QVERIFY(matches(position.navigation.longitudeDegrees, expected.longitude, 1e-9));
            QVERIFY(matches(position.navigation.altitudeEllipsoidMeters, expected.ellipsoid));
            QVERIFY(matches(position.navigation.altitudeMslMeters, expected.msl));
            QVERIFY(matches(position.navigation.horizontalAccuracyMeters, expected.horizontalAccuracy));
            QVERIFY(matches(position.navigation.verticalAccuracyMeters, expected.verticalAccuracy));
            if (expected.velocityAvailable) {
                QVERIFY(matches(position.navigation.speedMetersPerSecond,
                                std::hypot(static_cast<float>(expected.north), static_cast<float>(expected.east))));
            }
            QVERIFY(matches(position.navigation.courseRadians, expected.course));
            // Base stations output only PVTGeodetic; DOP and attitude blocks in the capture are ignored.
            QVERIFY(std::isnan(position.navigation.horizontalDop));
            QVERIFY(std::isnan(position.navigation.verticalDop));
            QCOMPARE(position.navigation.satellitesUsed, gpsSatellitesUsed(expected.satellites));
            QCOMPARE(position.velocityValid, expected.velocityAvailable);
            using Fix = GPSPositionReport::FixType;
            const auto fix = expected.error            ? Fix::NoFix
                             : expected.twoDimensional ? Fix::Fix2D
                             : expected.mode == 4      ? Fix::RTKFixed
                             : expected.mode == 5      ? Fix::RTKFloat
                                                       : Fix::Fix3D;
            QCOMPARE(position.navigation.fixType, fix);
        }
    }
    LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
    const auto invalidTimes = fixtureBytes("synthetic-invalid-time.sbf");
    QVERIFY(invalidTimes);
    for (const auto& expected : GPSFixture::invalidSbfTimes) {
        QVERIFY(expected.week == UINT16_MAX || expected.tow >= 604800000);
        (void) sbf->consume(std::span(*invalidTimes).subspan(expected.offset, expected.size));
        QVERIFY(sbf.events.empty());
    }
    const auto valid = fixtureBytes("synthetic-valid.sbf");
    QVERIFY(valid);
    auto corrupt = *valid;
    corrupt[2] ^= 1;  // Reject the PVT CRC; remaining metadata must not manufacture a fix.
    (void) sbf->consume(corrupt);
    QVERIFY(sbf.events.empty());

    LoggedRuntime recovered(SBF::FAMILY, makeDecoderOnlyIO(clock));
    (void) recovered->consume(*invalidTimes);
    (void) recovered->consume(*valid);
    QCOMPARE_NE(recovered.position.navigation.timestampUs, 0);
    QVERIFY(matches(recovered.position.navigation.latitudeDegrees, GPSFixture::sbfEpochs[0].latitude, 1e-9));
}

void capturedBlocks(GPSTestClock& clock)
{
    {
        LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
        const auto& position = sbf.position;
        const auto geodetic = fixtureBytes("geodetic.sbf");
        const auto attitude = fixtureBytes("attitude.sbf");
        QVERIFY(geodetic && attitude);
        for (auto byte : *geodetic) {
            (void) sbf->consume({&byte, 1});
        }
        QVERIFY(position.velocityValid);
        // Attitude blocks for a different epoch must not create another position.
        const auto previousTimestamp = position.navigation.timestampUs;
        (void) sbf->consume(*attitude);
        QCOMPARE(position.navigation.timestampUs, previousTimestamp);
    }
    LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
    const auto& position = sbf.position;
    const auto pvtGeodetic = fixtureBytes(PVT_GEODETIC);
    QVERIFY(pvtGeodetic);
    for (auto byte : *pvtGeodetic) {
        (void) sbf->consume({&byte, 1});
    }
    QVERIFY(std::abs(position.navigation.latitudeDegrees - 0.9310293523340808 * RAD_TO_DEG) < 1e-8);
    QVERIFY(std::abs(position.navigation.longitudeDegrees + 0.03921206770879602 * RAD_TO_DEG) < 1e-8);
    QVERIFY(std::abs(position.navigation.altitudeEllipsoidMeters - 131.18596542546626) < 1e-5);
    QCOMPARE(position.navigation.satellitesUsed, 36);
    QVERIFY(std::isnan(position.navigation.courseRadians));
}

// Configuration

void confirmationPolicy(GPSTestClock& clock)
{
    for (const bool rejected : {false, true}) {
        Receiver receiver(clock);
        // Replies without line endings, in reads too short for a whole reply.
        receiver.model.unterminatedReply = true;
        receiver.model.readChunk = 7;
        const QByteArray dataIO = "setDataInOut, USB1, Auto, SBF\n";
        if (rejected) {
            receiver.faults.rules.push_back(latestReply(dataIO, REJECTED.chopped(1)));
        }
        LoggedRuntime sbf(SBF::FAMILY, receiver.io());
        unsigned baud = 115200;
        QCOMPARE(sbf->configure(surveyIn(), baud), !rejected);
        if (!rejected) {
            continue;
        }
        QCOMPARE(sbf->errorDetail(), QStringLiteral("Septentrio receiver rejected 'setDataInOut, USB1, Auto, SBF'"));
        QCOMPARE(receiver.commands().back(), dataIO);
    }
    // A rejection followed by an acceptance in the same read is a rejection.
    Receiver receiver(clock);
    receiver.faults.rules.push_back(latestReply("setGeodeticDatum", REJECTED + "$R: setGeodeticDatum, WGS84\n"));
    receiver.model.readChunk = GPSCommandChannel::READ_CHUNK_SIZE;
    LoggedRuntime sbf(SBF::FAMILY, receiver.io());
    unsigned baud = 115200;
    QVERIFY(!sbf->configure(surveyIn(), baud));
    QCOMPARE(receiver.commands().back(), "setGeodeticDatum, WGS84\n");
}

void selectedPortAndPrecision(GPSTestClock& clock)
{
    for (const auto* port : {"COM1", "USB2", "IP10", "IPS1"}) {
        Receiver peer(clock);
        peer.faults.rules.push_back(latestReply("\n\r", QByteArray(port) + ">"));
        LoggedRuntime sbf(SBF::FAMILY, peer.io());
        unsigned baud = 115200;
        QVERIFY(sbf->configure(fixedBase(47.397742491, -8.545593291, 500.125f), baud));
        QVERIFY(peer.sent(QByteArray("setDataInOut, ") + port + ", Auto, RTCMv3+SBF"));
        QVERIFY(peer.sent(QByteArray("setSBFOutput, Stream1, ") + port + ", +PVTGeodetic"));
        QVERIFY(peer.sent("setStaticPosGeodetic, Geodetic1, 47.397742491, -8.545593291, 500.1250, WGS84"));
        QCOMPARE(peer.sent("setCOMSettings"), std::string_view(port).starts_with("COM"));
    }
    // Coordinates keep printf's bytes: 500.03125 is an exact binary tie that rounds to even, and -0 keeps its sign.
    Receiver peer(clock);
    LoggedRuntime sbf(SBF::FAMILY, peer.io());
    unsigned baud = 115200;
    QVERIFY(sbf->configure(fixedBase(-0.0, 8.0, 500.03125f), baud));
    QVERIFY(peer.sent("setStaticPosGeodetic, Geodetic1, -0.000000000, 8.000000000, 500.0312, WGS84\n"));
}

void datumRejection(GPSTestClock& clock)
{
    for (uint8_t datum : {19, 31, 250}) {
        const GPSProtocolLogCapture log;
        Receiver peer(clock);
        LoggedRuntime sbf(SBF::FAMILY, peer.io());
        unsigned baud = 115200;
        QVERIFY(sbf->configure(surveyIn(), baud));
        auto payload = bytes({.mode = 3, .latitude = 0.5, .longitude = 1, .height = 500});
        payload.resize(82);
        payload[59] = datum;
        (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, payload));
        QCOMPARE(sbf->error(), GPSProtocolError::Protocol);
        QVERIFY(sbf->errorDetail().contains("datum"));
        QVERIFY(!sbf->receiverReady());
        const auto surveys = sbf.reports<GPSSurveyReport>();
        QVERIFY(surveys.size() == 1 && surveyFlags(surveys.back()) == 0);
        QVERIFY(std::isnan(surveys.back().position.latitudeDegrees));
        QCOMPARE(log.warnings(),
                 QStringList{QStringLiteral("Unsupported Septentrio position datum: %1").arg(unsigned(datum))});
        QVERIFY(log.categories().contains(QStringLiteral("GPS.Protocols.SBF")));
    }
}

void datumWithoutBase(GPSTestClock& clock)
{
    // A rover reports RTK in its correction provider's datum (19) or a datum it was set to; only a base must use WGS84.
    uint32_t tow = 1000;
    for (uint8_t datum : {19, 30}) {
        LoggedRuntime sbf(SBF::FAMILY, makeDecoderOnlyIO(clock));
        PVTBody fix{};
        fix.mode = 4;
        fix.latitude = 0.5;
        fix.longitude = 1;
        fix.height = 500;
        auto payload = bytes(fix);
        payload[59] = datum;
        (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, payload, tow += 1000));
        QCOMPARE(sbf->error(), GPSProtocolError::None);
        const auto fixes = sbf.reports<GPSDecodedPosition>();
        QCOMPARE(fixes.size(), 1);
        QCOMPARE(fixes[0].navigation.fixType, GPSPositionReport::FixType::RTKFixed);
        QVERIFY(std::abs(fixes[0].navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
    }
}

void frameOwnership(GPSTestClock& clock)
{
    Receiver receiver(clock);
    LoggedRuntime sbf(SBF::FAMILY, receiver.io());
    unsigned baudrate = 115200;
    QVERIFY(sbf->configure(fixedBase(47, 8, 500, 1), baudrate));
    const auto corrections = [&sbf] { return sbf.reports<GPSRTCMFrame>().size(); };
    const auto fixes = [&sbf] { return sbf.reports<GPSDecodedPosition>().size(); };
    const auto correction = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    QVERIFY(RTCMFramer::isValidFrame(std::span<const uint8_t>(correction)));
    auto payload = bytes({.mode = 1, .latitude = 0.5, .longitude = 1});
    // Both checksums are valid; only the outer native frame may own these bytes.
    std::copy(correction.begin(), correction.end(), payload.begin() + 46);
    (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, payload));
    QCOMPARE(corrections(), 0);
    QCOMPARE(fixes(), 1);
    QVERIFY(std::abs(sbf.position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
    (void) sbf->consume(correction);
    QCOMPARE(corrections(), 1);
    QCOMPARE(fixes(), 1);
}

void surveyEvidence(GPSTestClock& clock)
{
    clock.reset(GPSTestClock::START_US);
    Receiver receiver(clock);
    LoggedRuntime sbf(SBF::FAMILY, receiver.io());
    const auto& position = sbf.position;
    unsigned baudrate = 115200;
    uint32_t tow = 0;
    auto payload = bytes({.latitude = 0.5, .longitude = 1, .height = 500});
    std::vector<GPSSurveyReport> surveys;
    // Publishes one epoch and checks the single survey report and accuracies it carries.
    auto publish = [&](uint8_t mode, uint8_t flags, uint16_t horizontal = 200, uint16_t vertical = 200) {
        clock.advanceBy(1000000);
        tow += 1000;
        payload[0] = mode;
        (void) LittleEndian::write<uint16_t>(payload, 76, horizontal);
        (void) LittleEndian::write<uint16_t>(payload, 78, vertical);
        sbf.events.clear();
        (void) sbf->consume(sbfBlock(SBF::BlockId::PVT_GEODETIC, payload, tow));
        surveys = sbf.reports<GPSSurveyReport>();
        QCOMPARE(surveys.size(), 1);
        QCOMPARE(surveyFlags(surveys.back()), flags);
        QVERIFY(!surveys.back().meanAccuracyMeters.has_value());
        if (horizontal == UINT16_MAX) {
            QVERIFY(std::isnan(position.navigation.horizontalAccuracyMeters));
        } else {
            QCOMPARE_EQ(position.navigation.horizontalAccuracyMeters, horizontal / 200.0f);
        }
        if (vertical == UINT16_MAX) {
            QVERIFY(std::isnan(position.navigation.verticalAccuracyMeters));
        } else {
            QCOMPARE_EQ(position.navigation.verticalAccuracyMeters, vertical / 200.0f);
        }
    };
    const auto duration = [&surveys] { return surveyDuration(surveys.back()); };

    for (bool fixed : {false, true, false}) {
        const GPSConfig config = fixed ? fixedBase(47, 8, 500) : surveyIn();
        QVERIFY(sbf->configure(config, baudrate));
        QVERIFY(sbf->receiverReady());
        // A standalone solution without the determination flag is not a completed fixed base.
        publish(1, 0);
        QCOMPARE(duration(), 0u);
        publish(0x41, fixed ? 0 : 2);
        const auto progress = duration();
        QVERIFY(fixed ? progress == 0 : progress > 0);
        publish(3, 1);
        const auto completed = duration();
        QVERIFY(fixed ? completed == 0 : completed >= progress);
        QVERIFY(std::abs(surveys.back().position.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
        QCOMPARE(surveys.back().position.altitudeMeters, 500);
        for (const auto& [horizontal, vertical] :
             {std::pair<uint16_t, uint16_t>{0, 0}, {UINT16_MAX, UINT16_MAX}, {UINT16_MAX, 200}, {200, UINT16_MAX}}) {
            publish(3, 1, horizontal, vertical);
            QCOMPARE(duration(), completed);
        }

        for (uint8_t mode : {0, 1, 2, 4, 9, 15, 0x83}) {
            publish(mode, 0);
            QCOMPARE(duration(), completed);
        }
        payload[1] = 9;
        publish(3, 0);
        QCOMPARE(duration(), completed);
        QVERIFY(std::isnan(surveys.back().position.latitudeDegrees));
        publish(0x41, fixed ? 0 : 2);
        const auto failedProgress = duration();
        QVERIFY(fixed ? failedProgress == 0 : failedProgress > completed);
        payload[1] = 0;
        publish(3, 1);

        for (size_t offset : {2, 10, 18}) {
            const double original = LittleEndian::read<double>(payload, offset).value();
            for (double invalid : {double(NAN), double(INFINITY), -2e10}) {
                (void) LittleEndian::write<double>(payload, offset, invalid);
                publish(3, 0);
                QVERIFY(std::isnan(surveys.back().position.latitudeDegrees));
                QVERIFY(std::isnan(surveys.back().position.longitudeDegrees));
                QVERIFY(std::isnan(surveys.back().position.altitudeMeters));
            }
            (void) LittleEndian::write<double>(payload, offset, original);
        }
        publish(3, 1);
    }
}

void configurationDecodesInterleavedTraffic(GPSTestClock& clock)
{
    Receiver peer(clock);
    const auto frame =
        sbfBlock(SBF::BlockId::PVT_GEODETIC, bytes({.mode = 1, .latitude = 0.5, .longitude = 1, .height = 500}));
    // The block arrives ahead of the reply to the first port command.
    peer.faults.rules.push_back(
        latestReply("setDataInOut", toByteArray(frame) + "$R: setDataInOut,COM1,,-RTCMv3-RTCMv2-CMRv2\n", 1, 1));
    LoggedRuntime sbf(SBF::FAMILY, peer.io());
    unsigned baud = 115200;
    QVERIFY(sbf->configure(surveyIn(), baud));
    QCOMPARE(sbf.reports<GPSDecodedPosition>().size(), 1);
    QVERIFY(std::abs(sbf.position.navigation.latitudeDegrees - 0.5 * RAD_TO_DEG) < 1e-6);
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"malformed-messages", malformedMessages},
    {"epoch-metadata", epochMetadata},
    {"invalid-coordinates", invalidCoordinates},
    {"independent-validity", independentValidity},
    {"captured-blocks", capturedBlocks},
    {"confirmation-policy", confirmationPolicy},
    {"selected-port-and-precision", selectedPortAndPrecision},
    {"datum-rejection", datumRejection},
    {"datum-without-base", datumWithoutBase},
    {"frame-ownership", frameOwnership},
    {"survey-evidence", surveyEvidence},
    {"configuration-decodes-interleaved-traffic", configurationDecodesInterleavedTraffic},
};
}  // namespace

void SBFProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void SBFProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

UT_REGISTER_TEST_LIGHTWEIGHT(SBFProtocolTest, TestLabel::Unit)
