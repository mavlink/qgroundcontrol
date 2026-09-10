#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "GPSProtocolTestIO.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"
#include "UBX/UBXMessageSchema.h"

#define CHECK(value)                          \
    do {                                      \
        if (!(value))                         \
            throw std::runtime_error(#value); \
    } while (0)

std::vector<uint8_t> fixture(const char* name)
{
    std::ifstream file(std::string(GPS_FIXTURE_DIR) + "/" + name, std::ios::binary);
    CHECK(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int noDevice(GPSCallbackType type, void*, int, void*)
{
    CHECK(type != GPSCallbackType::readDeviceData && type != GPSCallbackType::writeDeviceData);
    return 0;
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
    for (size_t byte = 0; byte < 4; ++byte)
        frame[6 + offset + byte] = tow >> (8 * byte);
    checksum(frame);
    return frame;
}

std::vector<uint8_t> endEpoch(uint32_t tow)
{
    return timed({0xb5, 0x62, 1, 0x61, 4, 0, 0, 0, 0, 0, 0, 0}, tow);
}

void navigationEpochs()
{
    for (bool before : {false, true}) {
        GPSPositionReport position{};
        GPSSatelliteReport satellites{};
        std::vector<GPSPositionReport> observations;
        auto io = makeGPSProtocolTestIO(noDevice, nullptr);
        io.decoded = [&](GPSDecodedBatch batch) {
            for (const auto& event : batch.events)
                if (const auto* fix = std::get_if<GPSPositionReport>(&event))
                    observations.push_back(*fix);
        };
        GPSDriverUBX ubx(std::move(io), &position, &satellites);
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
        CHECK(observations.back().fix_type == 6);
        CHECK(std::abs(observations.back().latitude_deg - 53.4507228) < 1e-8);
    }
}

int main()
{
    try {
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
        GPSPositionReport position{};
        GPSSatelliteReport satellites{};
        GPSDriverUBX ubx(makeGPSProtocolTestIO(noDevice, nullptr), &position, &satellites);
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
        CHECK(std::get<GPSRelativeReport>(relativeBatch.events.front()).time_utc_usec ==
              uint64_t(UBXNavigationEpoch::WEEK_MS - 1000) * 1000);
        const auto pvt = fixture("nav-pvt.ubx");
        for (auto byte : pvt)
            ubx.consume({&byte, 1});
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
        position.fix_type = 6;
        ubx.consume(fixture("nav-hpposllh.ubx"));
        CHECK(std::abs(position.latitude_deg - 53.337816927) < 1e-9);
        CHECK(std::abs(position.longitude_deg + 2.056673696) < 1e-9);
        CHECK(std::abs(position.altitude_msl_m - 233.5227) < 1e-6);
        CHECK(std::abs(position.eph - 0.335) < 1e-6);
        GPSDriverSBF sbf(makeGPSProtocolTestIO(noDevice, nullptr), &position, &satellites);
        const auto geodetic = fixture("pvt-geodetic.sbf");
        for (auto byte : geodetic)
            sbf.consume({&byte, 1});
        CHECK(std::abs(position.latitude_deg - 0.9310293523340808 * 180 / M_PI) < 1e-8);
        CHECK(std::abs(position.longitude_deg + 0.03921206770879602 * 180 / M_PI) < 1e-8);
        CHECK(std::abs(position.altitude_ellipsoid_m - 131.18596542546626) < 1e-5);
        CHECK(position.satellites_used == 36);
        CHECK(std::isnan(position.cog_rad));
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
