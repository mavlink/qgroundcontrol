#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "Ashtech/GPSDriverAshtech.h"
#include "CRC32.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolTestIO.h"
#include "NMEAFields.h"
#include "SBF/GPSDriverSBF.h"

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

int noDevice(GPSCallbackType type, void*, int, void*)
{
    CHECK(type != GPSCallbackType::readDeviceData && type != GPSCallbackType::writeDeviceData &&
          type != GPSCallbackType::setBaudrate);
    return 0;
}

std::vector<uint8_t> sbfPacket(uint16_t id, std::span<const uint8_t> payload)
{
    sbf_buf_t header{};
    header.sync = 0x4024;
    header.msg_id = id;
    header.length = 14 + payload.size();
    header.WNc = 2435;
    std::vector<uint8_t> packet(header.length);
    std::memcpy(packet.data(), &header, 14);
    std::memcpy(packet.data() + 14, payload.data(), payload.size());
    const auto crc = crc16(packet.data() + 4, packet.size() - 4);
    packet[2] = crc & 0xff;
    packet[3] = crc >> 8;
    return packet;
}

std::vector<uint8_t> femtoPacket(uint16_t id, std::span<const uint8_t> payload)
{
    femto_msg_header_t header{};
    header.preamble[0] = 0xaa;
    header.preamble[1] = 0x44;
    header.preamble[2] = 0x12;
    header.headerlength = sizeof(header);
    header.messageid = id;
    header.messagelength = payload.size();
    std::vector<uint8_t> packet(sizeof(header) + payload.size() + 4);
    std::memcpy(packet.data(), &header, sizeof(header));
    std::memcpy(packet.data() + sizeof(header), payload.data(), payload.size());
    const auto crc = QGC::crc32Update({packet.data(), packet.size() - 4});
    for (size_t i = 0; i < 4; ++i) {
        packet[packet.size() - 4 + i] = crc >> (8 * i);
    }
    return packet;
}

template <class T>
std::span<const uint8_t> bytes(const T& value)
{
    return {reinterpret_cast<const uint8_t*>(&value), sizeof(value)};
}

void malformedMessages()
{
    GPSPositionReport position{};
    GPSSatelliteReport satellites{};
    GPSDriverSBF sbf(makeGPSProtocolTestIO(noDevice, nullptr), &position, &satellites);
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.latitude = 0.5;
    fix.longitude = 1.0;
    fix.nr_sv = 12;
    sbf_payload_dop_t dop{};
    sbf_payload_vel_cov_geodetic_t covariance{};
    sbf.consume(sbfPacket(SBF_ID_DOP, bytes(dop)));
    sbf.consume(sbfPacket(SBF_ID_VelCovGeodetic, bytes(covariance)));
    const auto good = sbfPacket(SBF_ID_PVTGeodetic, bytes(fix));
    const std::array<uint8_t, 2> shortPayload{};
    CHECK(sbf.consume(good) & 1);
    CHECK(std::abs(position.latitude_deg - 0.5 * M_RAD_TO_DEG) < 0.00001);
    gps_test_time += 1000;
    const auto received = position.timestamp;
    CHECK(sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, shortPayload)) == 0);
    CHECK(position.timestamp == received);
    CHECK(sbf.consume(good) & 1);

    fix.cog = -2.0e10f;
    CHECK(sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix))) & 1);
    CHECK(std::isnan(position.cog_rad));
    fix.cog = 90.0f;
    CHECK(sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix))) & 1);
    CHECK(std::abs(position.cog_rad - M_PI_F / 2) < 0.00001);

    GPSDriverFemto femto(makeGPSProtocolTestIO(noDevice, nullptr), &position, &satellites);
    femto_uav_gps_t gps{};
    gps.lat = 470000000;
    gps.lon = 80000000;
    gps.fix_type = 3;
    gps.satellites_used = 14;
    const auto valid = femtoPacket(FEMTO_MSG_ID_UAVGPS, bytes(gps));
    CHECK(femto.consume(valid) & 1);
    CHECK(position.latitude_deg == 47.0);
    gps_test_time += 1000;
    const auto femtoReceived = position.timestamp;
    CHECK(femto.consume(femtoPacket(FEMTO_MSG_ID_UAVGPS, shortPayload)) == 0);
    CHECK(position.timestamp == femtoReceived);
    CHECK(femto.consume(valid) & 1);
    for (uint8_t length : {uint8_t{0}, uint8_t{3}, uint8_t{255}}) {
        auto invalid = valid;
        invalid[3] = length;
        CHECK(femto.consume(invalid) == 0);
        CHECK(femto.consume(valid) & 1);
    }
    CHECK(femto.consume(femtoPacket(FEMTO_MSG_ID_UAVSTATUS, shortPayload)) == 0);
}

class ReadProbe : public GPSProtocol
{
public:
    using GPSProtocol::GPSProtocol;
    using GPSProtocol::read;

    int configure(unsigned&, const GPSConfig&) override { return 0; }

    int receive(unsigned) override { return 0; }

    int consume(std::span<const uint8_t>) override { return 0; }
};

void tinyReads()
{
    const auto callback = [](GPSCallbackType type, void* data, int, void*) {
        CHECK(type == GPSCallbackType::readDeviceData);
        const auto& request = *static_cast<GPSReadRequest*>(data);
        CHECK(request.timeoutMs == 1000);
        CHECK(request.capacity > 0 && request.capacity <= 3);
        std::memset(request.buffer, 0x12, request.capacity);
        return request.capacity;
    };
    ReadProbe probe(makeGPSProtocolTestIO(callback, nullptr));
    for (int capacity = 0; capacity <= 3; ++capacity) {
        std::array<uint8_t, 5> guarded{0xab, 0xab, 0xab, 0xab, 0xab};
        CHECK(probe.read(guarded.data() + 1, capacity, 1000) == capacity);
        CHECK(guarded.front() == 0xab);
        CHECK(guarded[capacity + 1] == 0xab);
    }
}

void boundedFields()
{
    CHECK(NMEAFields::number<double>("+47.5").value() == 47.5);
    CHECK(!NMEAFields::number<double>("+-1"));
    CHECK(!NMEAFields::number<double>("12.5garbage"));
    CHECK(!NMEAFields::number<double>("NaN"));
    CHECK(!NMEAFields::number<int>("999999999999999999999"));
    NMEAFields::Cursor fields("1,,3*00");
    int value = 0;
    CHECK(fields.read(value) && value == 1);
    CHECK(!fields.read(value) && fields.valid());
    CHECK(fields.read(value) && value == 3);
}

class DeadlineProbe : public ReadProbe
{
public:
    using ReadProbe::ReadProbe;

    int transaction()
    {
        const Operation outer(*this, 10);
        const Operation inner(*this, 1000);
        uint8_t byte{};
        if (read(&byte, 1, 1000) != 1)
            return -1;
        return read(&byte, 1, 1000);
    }
};

void absoluteDeadline()
{
    uint64_t now = 1000000;
    GPSProtocolIO io;
    io.nowUs = [&] { return now; };
    io.read = [&](auto buffer, GPSDeadline deadline) {
        CHECK(deadline.untilUs == 1010000);
        if (now == 1000000) {
            CHECK(deadline.remainingMilliseconds(now) == 10);
            now += 9000;
            buffer[0] = 0;
            return GPSProtocolReadResult{GPSReadStatus::Data, 1};
        }
        CHECK(deadline.remainingMilliseconds(now) == 1);
        now = deadline.untilUs;
        return GPSProtocolReadResult{GPSReadStatus::TimedOut};
    };
    DeadlineProbe probe(io);
    CHECK(probe.transaction() == 0);
    CHECK(now == 1010000);
}

std::vector<uint8_t> nmeaPacket(std::string_view body)
{
    uint8_t checksum = 0;
    std::vector<uint8_t> result{'$'};
    for (const auto byte : body) {
        result.push_back(byte);
        checksum ^= byte;
    }
    result.push_back('*');
    result.push_back(NMEAFields::hexDigit(checksum >> 4));
    result.push_back(NMEAFields::hexDigit(checksum));
    result.push_back('\r');
    result.push_back('\n');
    return result;
}

void ashtechMetadata()
{
    GPSPositionReport position{};
    GPSSatelliteReport satellites{};
    auto io = makeGPSProtocolTestIO(noDevice, nullptr);
    GPSSatelliteReport gpsSatellites;
    io.decoded = [&](GPSDecodedBatch batch) {
        for (const auto& event : batch.events)
            if (const auto* report = std::get_if<GPSSatelliteReport>(&event);
                report && report->constellation == GPSConstellation::GPS)
                gpsSatellites = *report;
    };
    GPSDriverAshtech driver(std::move(io), &position, &satellites);
    const auto gga = nmeaPacket("GPGGA,123519,4700.0,N,00800.0,E,1,08,0.9,500.0,M,0,M,,");
    CHECK(driver.consume(gga) & 1);
    CHECK(position.latitude_deg == 47.0);
    const auto received = position.timestamp;
    CHECK(driver.consume(nmeaPacket("GPGGA,123519,,N,,E,1,08,0.9,,M,,M,,")) == 0);
    CHECK(position.timestamp == received);
    CHECK(driver.consume(nmeaPacket("PASHR,POS,bad,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1")) == 0);
    CHECK(driver.consume(gga) & 1);
    CHECK(driver.consume(nmeaPacket("GPGSV,1,1,01,01,,,")) & 2);
    CHECK(gpsSatellites.count == 1);
    CHECK(!gpsSatellites.entries[0].signal);
    CHECK(!gpsSatellites.entries[0].azimuth);
    CHECK(!gpsSatellites.entries[0].elevation);
    CHECK(!gpsSatellites.entries[0].used);
    CHECK(driver.consume(nmeaPacket("GPGSV,1,1,01,01,0,0,0")) & 2);
    CHECK(gpsSatellites.entries[0].signal == 0);
    CHECK(gpsSatellites.entries[0].azimuth == 0);
    CHECK(gpsSatellites.entries[0].elevation == 0);
    driver.consume(nmeaPacket("GPZDA,172809.456,12,07,2026,00,00"));
    CHECK(position.time_utc_usec % 1000000 >= 455999 && position.time_utc_usec % 1000000 <= 456001);
}

int main()
{
    try {
        tinyReads();
        boundedFields();
        absoluteDeadline();
        malformedMessages();
        ashtechMetadata();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
