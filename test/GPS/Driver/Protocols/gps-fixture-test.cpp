#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "GPSProtocolFeatures.h"
#include "GPSProtocolTestIO.h"
#include "LittleEndian.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"
#include "UBX/UBXMessageSchema.h"
#include "UnitTest.h"
#include "fixtures/GPSFixtureExpectations.h"

#define CHECK(value)                          \
    do {                                      \
        if (!(value)) {                       \
            throw std::runtime_error(#value); \
        }                                     \
    } while (0)

namespace {
std::vector<uint8_t> fixture(const char* name)
{
    std::ifstream file(std::string(GPS_FIXTURE_DIR) + "/" + name, std::ios::binary);
    CHECK(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

GPSProtocolIO noDevice()
{
    auto io = makeGPSProtocolTestIO();
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

void checksum(std::vector<uint8_t>& frame)
{
    uint8_t a = 0, b = 0;
    for (size_t index = 2; index + 2 < frame.size(); ++index) {
        a += frame[index];
        b += a;
    }
    frame[frame.size() - 2] = a;
    frame.back() = b;
}

std::vector<uint8_t> timed(std::vector<uint8_t> frame, uint32_t tow, size_t offset = 0)
{
    for (size_t byte = 0; byte < 4; ++byte) {
        frame[6 + offset + byte] = tow >> (8 * byte);
    }
    checksum(frame);
    return frame;
}

std::vector<uint8_t> endEpoch(uint32_t tow)
{
    return timed({0xb5, 0x62, 1, 0x61, 4, 0, 0, 0, 0, 0, 0, 0}, tow);
}

bool matches(double actual, double expected, double tolerance = 1e-6)
{
    return std::isnan(expected) ? std::isnan(actual) : std::abs(actual - expected) < tolerance;
}

void independentNmeaFields()
{
    const auto ggaBytes = fixture("synthetic-gga.nmea");
    const std::string_view ggaText(reinterpret_cast<const char*>(ggaBytes.data()), ggaBytes.size());
    for (const auto& expected : GPSFixture::ggas) {
        const auto text = ggaText.substr(expected.offset, expected.size);
        const auto sentence = NMEA::sentence(text);
        CHECK(sentence && sentence->talker() == expected.talker);
        const auto actual = NMEA::gga(*sentence);
        if (std::isnan(expected.latitude) || std::isnan(expected.longitude)) {
            CHECK(!actual);
        } else {
            CHECK(actual);
            CHECK(matches(actual->latitude, expected.latitude, 1e-9));
            CHECK(matches(actual->longitude, expected.longitude, 1e-9));
            CHECK(matches(actual->altitude, expected.altitude));
            CHECK(matches(actual->geoidSeparation, expected.geoid));
            CHECK(matches(actual->hdop, expected.hdop));
            CHECK(actual->quality == expected.quality);
            CHECK(actual->satellitesUsed.has_value() == (expected.satellites >= 0));
            if (expected.satellites >= 0) {
                CHECK(*actual->satellitesUsed == static_cast<unsigned>(expected.satellites));
            }
        }
        CHECK(!NMEA::sentence(text.substr(0, text.size() - 3)));
        std::string corrupt(text);
        corrupt[corrupt.size() - 3] = corrupt[corrupt.size() - 3] == '0' ? '1' : '0';
        CHECK(!NMEA::sentence(corrupt));
    }
    const auto gstBytes = fixture("synthetic-gst.nmea");
    const std::string_view gstText(reinterpret_cast<const char*>(gstBytes.data()), gstBytes.size());
    for (const auto& expected : GPSFixture::gsts) {
        const auto sentence = NMEA::sentence(gstText.substr(expected.offset, expected.size));
        CHECK(sentence && sentence->talker() == expected.talker);
        const auto actual = NMEA::gst(*sentence);
        CHECK(actual);
        CHECK(matches(actual->horizontalAccuracy, expected.horizontal));
        CHECK(matches(actual->verticalAccuracy, expected.vertical));
    }
}

#if QGC_GPS_ENABLE_UBX
void independentIntegrityAndUtc()
{
    const auto integrityBytes = fixture("synthetic-integrity.ubx");
    const auto utcBytes = fixture("synthetic-timeutc.ubx");
    for (const size_t chunkSize : {1u, 7u, 256u}) {
        gps_test_time += 1000000;
        GPSNativePositionReport position{};
        GPSNativeUBX ubx(noDevice(), &position, nullptr);
        ubx.setDecodeContext({.navigation = true, .useNavPvt = false});
        for (const auto& expected : GPSFixture::integrity) {
            auto remaining = std::span(integrityBytes).subspan(expected.offset, expected.size);
            auto corrupt = std::vector<uint8_t>(remaining.begin(), remaining.end());
            corrupt.back() ^= 1;
            CHECK(ubx.decode(corrupt).batch.events.empty());
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                const auto decoded = ubx.decode(remaining.first(count));
                CHECK(decoded.bytesConsumed == count);
                remaining = remaining.subspan(count);
                if (!remaining.empty()) {
                    CHECK(decoded.batch.events.empty());
                    continue;
                }
                CHECK(decoded.batch.events.size() == 1);
                const auto& actual = std::get<GPSNativeIntegrityReport>(decoded.batch.events.front());
                CHECK(static_cast<unsigned>(actual.spoofing_state) == expected.spoofing);
                CHECK(static_cast<unsigned>(actual.jamming_state) == expected.jamming);
                CHECK(actual.noise_per_ms.value_or(-1) == expected.noise);
                CHECK(actual.automatic_gain_control.has_value() == (expected.agc >= 0));
                if (expected.agc >= 0) {
                    CHECK(*actual.automatic_gain_control == expected.agc);
                }
                CHECK(actual.jamming_indicator.value_or(-1) == expected.indicator);
                CHECK(actual.spoofing_state_timestamp != 0);
                if (expected.noise >= 0) {
                    CHECK(actual.rf_timestamp != 0 && actual.jamming_state_timestamp != 0);
                }
                CHECK(actual.corrections_crc_failed.has_value() == (expected.crcFailed >= 0));
                if (expected.crcFailed >= 0) {
                    CHECK(*actual.corrections_crc_failed == static_cast<bool>(expected.crcFailed));
                    CHECK(static_cast<int>(actual.corrections_msg_used) == expected.correctionUse);
                    CHECK(actual.corrections_protocol == GPSNativeIntegrityReport::CORRECTIONS_PROTOCOL_RTCM3);
                    CHECK(actual.corrections_timestamp != 0);
                }
            }
        }
        for (const auto& expected : GPSFixture::utc) {
            auto remaining = std::span(utcBytes).subspan(expected.offset, expected.size);
            const auto previousUtc = position.time_utc_usec;
            auto corrupt = std::vector<uint8_t>(remaining.begin(), remaining.end());
            corrupt.back() ^= 1;
            CHECK(ubx.consume(corrupt) == 0);
            CHECK(position.time_utc_usec == previousUtc);
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                ubx.consume(remaining.first(count));
                remaining = remaining.subspan(count);
                CHECK(position.time_utc_usec == (remaining.empty() ? expected.microseconds : previousUtc));
            }
        }
    }
}

void relativeUtcUnavailable()
{
    GPSNativePositionReport position{};
    GPSNativeUBX ubx(noDevice(), &position, nullptr);
    ubx.setDecodeContext({.navigation = true});
    ubx.consume(fixture("nav-pvt.ubx"));
    CHECK(position.time_utc_usec != 0);
    const auto original = fixture("relative.ubx");
    for (const uint32_t tow : {UBXNavigationEpoch::WEEK_MS - 1, 0u, 1000u}) {
        const auto frame = timed(original, tow, 4);
        CHECK(ubx.decode(std::span(frame).first(frame.size() - 1)).batch.events.empty());
        const auto decoded = ubx.decode(std::span(frame).last(1));
        CHECK(decoded.batch.events.size() == 1);
        const auto& relative = std::get<GPSNativeRelativeReport>(decoded.batch.events.front());
        CHECK(relative.time_utc_usec == 0);
        CHECK(relative.relative_position_valid == GPSFixture::relativeValid);
    }
}

void navigationEpochs()
{
    for (bool before : {false, true}) {
        GPSNativePositionReport position{};
        GPSNativeSatelliteReport satellites{};
        std::vector<GPSNativePositionReport> observations;
        auto io = noDevice();
        io.decoded = [&](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* fix = std::get_if<GPSNativePositionReport>(&event)) {
                    observations.push_back(*fix);
                }
            }
        };
        GPSNativeUBX ubx(std::move(io), &position, &satellites);
        ubx.setDecodeContext({.navigation = true, .assembleEpochs = true});
        const auto pvt = timed(fixture("nav-pvt.ubx"), UBXNavigationEpoch::WEEK_MS - 1000);
        const auto dop = timed(fixture("nav-dop.ubx"), UBXNavigationEpoch::WEEK_MS - 1000);
        const auto hp = timed(fixture("nav-hpposllh.ubx"), UBXNavigationEpoch::WEEK_MS - 1000, 4);
        ubx.consume(before ? dop : pvt);
        ubx.consume(hp);
        ubx.consume(before ? pvt : dop);
        CHECK(observations.empty());
        ubx.consume(endEpoch(UBXNavigationEpoch::WEEK_MS - 1000));
        CHECK(observations.size() == 1);
        CHECK(std::abs(observations.back().latitude_deg - 53.337816927) < 1e-9);
        CHECK(std::abs(observations.back().hdop - 0.58) < 1e-6);
        ubx.consume(pvt);
        ubx.consume(endEpoch(UBXNavigationEpoch::WEEK_MS - 1000));
        CHECK(observations.size() == 1);
        ubx.consume(timed(pvt, 0));
        gps_test_time += UBXNavigationEpoch::MAX_AGE_US;
        ubx.consume({});
        CHECK(observations.size() == 2);
        CHECK(observations.back().dop_timestamp == 0);
        CHECK(std::isnan(observations.back().hdop));
        CHECK(std::isnan(observations.back().heading));
        CHECK(std::isnan(observations.back().heading_accuracy));
        CHECK(std::abs(observations.back().latitude_deg - 53.4507228) < 1e-8);
        // Adjacent epochs can interleave without donating DOP to one another.
        ubx.consume(timed(pvt, 1000));
        ubx.consume(timed(dop, 2000));
        ubx.consume(timed(dop, 1000));
        ubx.consume(endEpoch(1000));
        CHECK(observations.size() == 3);
        ubx.consume(timed(pvt, 2000));
        ubx.consume(endEpoch(2000));
        CHECK(observations.size() == 4);
        CHECK(observations.back().dop_timestamp != 0);
        // Metadata-only epochs never manufacture a position.
        ubx.consume(timed(dop, 3000));
        gps_test_time += UBXNavigationEpoch::MAX_AGE_US;
        ubx.consume({});
        CHECK(observations.size() == 4);
        auto fixed = timed(pvt, 4000);
        fixed[6 + 21] = 0x81;
        checksum(fixed);
        ubx.consume(fixed);
        gps_test_time += UBXNavigationEpoch::MAX_AGE_US;
        ubx.consume({});
        CHECK(observations.size() == 5);
        CHECK(observations.back().fix_type == GPSPositionReport::FixType::RTKFixed);
        CHECK(std::abs(observations.back().latitude_deg - 53.4507228) < 1e-8);
    }
}
#endif

#if QGC_GPS_ENABLE_SBF
void independentSbfValidity()
{
    for (const auto& expected : GPSFixture::sbfEpochs) {
        for (const size_t chunkSize : {1u, 11u, 512u}) {
            gps_test_time += 1000000;
            GPSNativePositionReport position{};
            unsigned observations = 0;
            auto io = noDevice();
            io.decoded = [&](const GPSDecodedBatch& batch) {
                for (const auto& event : batch.events) {
                    if (std::holds_alternative<GPSNativePositionReport>(event)) {
                        ++observations;
                    }
                }
            };
            GPSNativeSBF sbf(std::move(io), &position, nullptr);
            const auto bytes = fixture(expected.filename);
            auto remaining = std::span(bytes);
            while (!remaining.empty()) {
                const auto count = std::min(chunkSize, remaining.size());
                sbf.consume(remaining.first(count));
                remaining = remaining.subspan(count);
            }
            CHECK(observations == 0);
            gps_test_time += 200000;
            sbf.consume({});
            CHECK(observations == 1);
            CHECK(position.time_utc_usec == 0);
            CHECK(matches(position.latitude_deg, expected.latitude, 1e-9));
            CHECK(matches(position.longitude_deg, expected.longitude, 1e-9));
            CHECK(matches(position.altitude_ellipsoid_m, expected.ellipsoid));
            CHECK(matches(position.altitude_msl_m, expected.msl));
            CHECK(matches(position.eph, expected.horizontalAccuracy));
            CHECK(matches(position.epv, expected.verticalAccuracy));
            CHECK(matches(position.vel_n_m_s, expected.north));
            CHECK(matches(position.vel_e_m_s, expected.east));
            CHECK(matches(position.vel_d_m_s, expected.down));
            CHECK(matches(position.cog_rad, expected.course));
            CHECK(matches(position.hdop, expected.hdop));
            CHECK(matches(position.vdop, expected.vdop));
            CHECK(matches(position.speedAccuracyMetersPerSecond, expected.speedAccuracy));
            CHECK(matches(position.heading, expected.heading));
            CHECK(matches(position.heading_accuracy, expected.headingAccuracy));
            CHECK(position.satellites_used == expected.satellites);
            CHECK(position.vel_ned_valid == expected.velocityAvailable);
            using Fix = GPSPositionReport::FixType;
            const auto fix = expected.error            ? Fix::NoFix
                             : expected.twoDimensional ? Fix::Fix2D
                             : expected.mode == 4      ? Fix::RTKFixed
                             : expected.mode == 5      ? Fix::RTKFloat
                                                       : Fix::Fix3D;
            CHECK(position.fix_type == fix);
        }
    }
    GPSNativePositionReport position{};
    GPSNativeSBF sbf(noDevice(), &position, nullptr);
    const auto invalidTimes = fixture("synthetic-invalid-time.sbf");
    for (const auto& expected : GPSFixture::invalidSbfTimes) {
        CHECK(expected.week == UINT16_MAX || expected.tow >= 604800000);
        sbf.consume(std::span(invalidTimes).subspan(expected.offset, expected.size));
        gps_test_time += 200000;
        CHECK(sbf.decode({}).batch.events.empty());
        CHECK(position.timestamp == 0);
    }
    const auto valid = fixture("synthetic-valid.sbf");
    auto corrupt = valid;
    corrupt[2] ^= 1;  // Reject the PVT CRC; remaining metadata must not manufacture a fix.
    sbf.consume(corrupt);
    gps_test_time += 200000;
    CHECK(sbf.decode({}).batch.events.empty());
    CHECK(position.timestamp == 0);

    GPSNativeSBF recovered(noDevice(), &position, nullptr);
    recovered.consume(invalidTimes);
    recovered.consume(valid);
    gps_test_time += 200000;
    recovered.consume({});
    CHECK(position.timestamp != 0);
    CHECK(matches(position.latitude_deg, GPSFixture::sbfEpochs[0].latitude, 1e-9));
}
#endif

#if QGC_GPS_ENABLE_UBX || QGC_GPS_ENABLE_SBF
void independentSequences()
{
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
#if QGC_GPS_ENABLE_UBX
    GPSNativeUBX ubx(noDevice(), &position, &satellites);
    ubx.setDecodeContext({.navigation = true});
    const auto navigation = fixture("navigation.ubx");
    size_t offset = 0;
    for (const auto& expected : GPSFixture::positions) {
        const auto end = expected.offset + expected.size;
        while (offset < end) {
            ubx.consume({navigation.data() + offset, 1});
            ++offset;
        }
        CHECK(std::abs(position.latitude_deg - expected.latitude) < 1e-8);
        CHECK(std::abs(position.longitude_deg - expected.longitude) < 1e-8);
        CHECK(std::abs(position.altitude_msl_m - expected.altitude) < 1e-6);
    }
    ubx.consume(std::span(navigation).subspan(offset));
    CHECK(satellites.count == std::size(GPSFixture::satellites));
    constexpr GPSConstellation systems[] = {
        GPSConstellation::GPS,     GPSConstellation::SBAS, GPSConstellation::Galileo, GPSConstellation::BeiDou,
        GPSConstellation::Unknown, GPSConstellation::QZSS, GPSConstellation::GLONASS};
    for (size_t i = 0; i < satellites.count; ++i) {
        const auto& expected = GPSFixture::satellites[i];
        const auto& actual = satellites.entries[i];
        CHECK(actual.id == expected.id && actual.signal == expected.signal);
        CHECK(actual.elevation == expected.elevation && actual.azimuth == expected.azimuth);
        CHECK(actual.used == expected.used);
        CHECK(expected.gnss < std::size(systems) && actual.constellation == systems[expected.gnss]);
    }
    const auto relative = ubx.decode(fixture("relative.ubx")).batch;
    CHECK(relative.events.size() == 1);
    CHECK(std::get<GPSNativeRelativeReport>(relative.events.front()).relative_position_valid ==
          GPSFixture::relativeValid);

    const auto mixed = fixture("mixed.gps");
    for (const auto& expected : GPSFixture::corrections) {
        RTCMFramer frame;
        for (unsigned i = 0; i < expected.size; ++i) {
            CHECK(frame.addByte(mixed[expected.offset + i]) == (i + 1 == expected.size));
        }
        CHECK(frame.valid() && frame.messageId() == expected.id);
        CHECK(frame.frame().size() == expected.size);
        std::vector<uint8_t> corrupt(frame.frame().begin(), frame.frame().end());
        corrupt.back() ^= 1;
        CHECK(!RTCMFramer::isValidFrame(std::span<const uint8_t>(corrupt)));
    }
    ubx.consume(mixed);
    CHECK(std::abs(position.latitude_deg - 32.0658325) < 1e-8);
    const auto nmea = fixture("gga.nmea");
    const auto sentence = NMEA::sentence({reinterpret_cast<const char*>(nmea.data()), nmea.size()});
    CHECK(sentence);
    const auto gga = NMEA::gga(*sentence);
    CHECK(gga);
    CHECK(std::abs(gga->latitude - GPSFixture::ggaLatitude) < 1e-8);
    CHECK(std::abs(gga->longitude - GPSFixture::ggaLongitude) < 1e-8);
    CHECK(std::abs(gga->altitude - GPSFixture::ggaAltitude) < 1e-6);
    CHECK(gga->satellitesUsed == GPSFixture::ggaSatellites);

#endif
#if QGC_GPS_ENABLE_SBF
    GPSNativeSBF sbf(noDevice(), &position, &satellites);
    for (auto byte : fixture("geodetic.sbf")) {
        sbf.consume({&byte, 1});
    }
    gps_test_time += 200000;
    sbf.consume({});
    CHECK(std::abs(position.speedAccuracyMetersPerSecond - std::sqrt(GPSFixture::speedVariance)) < 1e-7);
    // Attitude blocks for a different epoch must not create another position.
    const auto previousTimestamp = position.timestamp;
    sbf.consume(fixture("attitude.sbf"));
    gps_test_time += 200000;
    sbf.consume({});
    CHECK(position.timestamp == previousTimestamp);
    CHECK(std::isnan(position.heading));
#endif
}
#endif

void scalarWireValues()
{
    const std::array<uint8_t, 9> data{0, 0xfe, 0xff, 0xff, 0xff, 0, 0, 0x80, 0xbf};
    CHECK(LittleEndian::read<int32_t>(data, 1) == -2);
    CHECK(LittleEndian::read<float>(data, 5) == -1.0f);
    CHECK(!LittleEndian::read<double>(data, 2));
    CHECK(!LittleEndian::read<uint8_t>(data, SIZE_MAX));
    std::array<uint8_t, 9> output{};
    CHECK(LittleEndian::write(output, 1, int32_t(-2)));
    CHECK(LittleEndian::write(output, 5, -1.0f));
    CHECK(output == data);
    CHECK(!LittleEndian::write(output, 2, double(1.0)));
    CHECK(output == data);
    CHECK(LittleEndian::write(output, 1, std::numeric_limits<uint64_t>::max()));
    CHECK(LittleEndian::read<uint64_t>(output, 1) == std::numeric_limits<uint64_t>::max());
    CHECK(LittleEndian::write(output, 1, std::bit_cast<double>(uint64_t(0x7ff8000000000001))));
    CHECK(std::bit_cast<uint64_t>(*LittleEndian::read<double>(output, 1)) == 0x7ff8000000000001);
}

}  // namespace

class GPSProtocolFixtureTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolFixtureTest::_protocol()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        scalarWireValues();
        independentNmeaFields();
#if QGC_GPS_ENABLE_SBF
        independentSbfValidity();
#endif
#if QGC_GPS_ENABLE_UBX || QGC_GPS_ENABLE_SBF
        independentSequences();
        GPSNativePositionReport position{};
        GPSNativeSatelliteReport satellites{};
#endif
#if QGC_GPS_ENABLE_UBX
        independentIntegrityAndUtc();
        relativeUtcUnavailable();
        navigationEpochs();
        CHECK(!UBX::receiverProfile(UBX::Board::u_blox9).rtcmOutput);
        CHECK(UBX::receiverProfile(UBX::Board::u_blox9_F9P_L1L2).rtcmOutput);
        CHECK(!UBX::receiverProfile(UBX::Board::u_blox10).usb);
        CHECK(UBX::configurationValueBytes(0x50000001) == 0);
        CHECK(UBX::configurationValueBytes(0x30000001) == 2);
        for (const auto& schema : UBX::MESSAGE_SCHEMAS) {
            std::vector<uint8_t> shortPayload(schema.minimum - 1);
            CHECK(!UBX::validPayload(schema.message, shortPayload));
            std::vector<uint8_t> longPayload(schema.maximum + 1);
            CHECK(!UBX::validPayload(schema.message, longPayload));
        }
        GPSNativeUBX ubx(noDevice(), &position, &satellites);
        ubx.setDecodeContext({.navigation = true});
        std::vector<uint8_t> relative(72);
        relative[0] = 0xb5;
        relative[1] = 0x62;
        relative[2] = 1;
        relative[3] = 0x3c;
        relative[4] = 64;
        relative[6] = 1;
        relative = timed(relative, UBXNavigationEpoch::WEEK_MS - 1000, 4);
        const auto relativeBatch = ubx.decode(relative).batch;
        CHECK(relativeBatch.events.size() == 1);
        CHECK(std::get<GPSNativeRelativeReport>(relativeBatch.events.front()).time_utc_usec == 0);
        const auto pvt = fixture("nav-pvt.ubx");
        for (auto byte : pvt) {
            ubx.consume({&byte, 1});
        }
        CHECK(std::abs(position.latitude_deg - 53.4507228) < 1e-8);
        CHECK(std::abs(position.longitude_deg + 2.2402855) < 1e-8);
        CHECK(std::abs(position.altitude_msl_m - 42.701) < 1e-6);
        CHECK(std::abs(position.vel_n_m_s + 0.007) < 1e-6);
        CHECK(position.satellites_used == 26);
        ubx.consume(fixture("nav-dop.ubx"));
        CHECK(std::abs(position.hdop - 0.58) < 1e-6);
        ubx.consume(fixture("nav-sat.ubx"));
        CHECK(satellites.count == 43);
        const auto timestamp = position.timestamp;
        auto corrupt = pvt;
        corrupt[30] ^= 1;
        CHECK(ubx.consume(corrupt) == 0);
        CHECK(position.timestamp == timestamp);
        position.fix_type = GPSPositionReport::FixType::RTKFixed;
        ubx.consume(fixture("nav-hpposllh.ubx"));
        CHECK(std::abs(position.latitude_deg - 53.337816927) < 1e-9);
        CHECK(std::abs(position.longitude_deg + 2.056673696) < 1e-9);
        CHECK(std::abs(position.altitude_msl_m - 233.5227) < 1e-6);
        CHECK(std::abs(position.eph - 0.335) < 1e-6);
#endif
#if QGC_GPS_ENABLE_SBF
        GPSNativeSBF sbf(noDevice(), &position, &satellites);
        const auto geodetic = fixture("pvt-geodetic.sbf");
        for (auto byte : geodetic) {
            sbf.consume({&byte, 1});
        }
        gps_test_time += 200000;
        sbf.consume({});
        CHECK(std::abs(position.latitude_deg - 0.9310293523340808 * 180 / M_PI) < 1e-8);
        CHECK(std::abs(position.longitude_deg + 0.03921206770879602 * 180 / M_PI) < 1e-8);
        CHECK(std::abs(position.altitude_ellipsoid_m - 131.18596542546626) < 1e-5);
        CHECK(position.satellites_used == 36);
        CHECK(std::isnan(position.cog_rad));
#endif
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolFixtureTest, TestLabel::Unit)

#include "gps-fixture-test.moc"
