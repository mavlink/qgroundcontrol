#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolTestIO.h"
#include "SBF/GPSDriverSBF.h"

// Keep checks active in Release, too.
#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

class Receiver
{
public:
    bool septentrio = false;
    std::string rejected_command;
    bool cancel_read = false;
    size_t read_chunk = 7;
    size_t noise_bytes = 0;
    unsigned failed_reads = 0;
    std::vector<std::string> commands;

    static int callback(GPSCallbackType type, void* data, int size, void* user)
    {
        return static_cast<Receiver*>(user)->handle(type, data, size);
    }

    bool sent(const std::string& prefix) const
    {
        return std::any_of(commands.begin(), commands.end(),
                           [&](const auto& command) { return command.compare(0, prefix.size(), prefix) == 0; });
    }

private:
    std::string reply;
    bool rejected = false;

    int handle(GPSCallbackType type, void* data, int size)
    {
        if (type == GPSCallbackType::writeDeviceData) {
            const std::string command(static_cast<const char*>(data), size);
            commands.push_back(command);
            rejected = !rejected_command.empty() && command.compare(0, rejected_command.size(), rejected_command) == 0;
            if (rejected) {
                reply = septentrio ? "$R? rejected\n" : "<ERROR\r\n";
            } else if (septentrio) {
                reply = command == "\n\r" ? "USB1>" : "$R: " + command;
            } else {
                reply = '<' + command.substr(0, command.find_first_of(" \r\n")) + " OK";
                reply.insert(0, noise_bytes, '\0');
            }
            return size;
        }

        if (type == GPSCallbackType::readDeviceData) {
            const auto request = *static_cast<const GPSReadRequest*>(data);
            const int timeout = request.timeoutMs;
            data = request.buffer;
            CHECK(timeout >= 0);
            gps_test_time += 1000;
            if (rejected && cancel_read) {
                ++failed_reads;
                return GPSProtocol::ReadCancelled;
            }
            if (reply.empty()) {
                gps_test_time += uint64_t(timeout) * 1000 + 1;
                return 0;
            }
            // Neither protocol guarantees that an ACK fits in one read or includes a NUL terminator.
            const size_t count = std::min({reply.size(), size_t(size), read_chunk});
            memcpy(data, reply.data(), count);
            reply.erase(0, count);
            gps_test_time += 1000;
            return static_cast<int>(count);
        }
        return 0;
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
    GPSPositionReport position{};
    GPSSatelliteReport satellites{};
    std::unique_ptr<GPSBaseProtocol> driver;
    if (septentrio) {
        driver = std::make_unique<GPSDriverSBF>(makeGPSProtocolTestIO(Receiver::callback, &receiver), &position,
                                                &satellites);
    } else {
        driver = std::make_unique<GPSDriverFemto>(makeGPSProtocolTestIO(Receiver::callback, &receiver), &position,
                                                  &satellites);
    }
    GPSProtocol::GPSConfig config{};
    config.base = {.useFixedBase = fixed,
                   .surveyInAccMeters = 1.25,
                   .surveyInDurationSecs = 60,
                   .fixedBaseLatitude = 47.0,
                   .fixedBaseLongitude = 8.0,
                   .fixedBaseAltitudeMeters = 500.0f,
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
}

void sbfFrameOwnership()
{
    Receiver receiver;
    receiver.septentrio = true;
    GPSPositionReport position;
    GPSSatelliteReport satellites;
    size_t correctionCount = 0;
    size_t fixCount = 0;
    auto io = makeGPSProtocolTestIO(Receiver::callback, &receiver);
    io.decoded = [&](GPSDecodedBatch batch) {
        for (const auto& event : batch.events) {
            correctionCount += std::holds_alternative<GPSRTCMReport>(event);
            fixCount += std::holds_alternative<GPSPositionReport>(event);
        }
    };
    GPSDriverSBF driver(io, &position, &satellites);
    GPSProtocol::GPSConfig config{};
    config.base = {.useFixedBase = true,
                   .fixedBaseLatitude = 47,
                   .fixedBaseLongitude = 8,
                   .fixedBaseAltitudeMeters = 500,
                   .fixedBaseAccuracyMeters = 1};
    config.output_mode = GPSProtocol::OutputMode::RTCM;
    unsigned baudrate = 115200;
    CHECK(driver.configure(baudrate, config) == 0);
    std::vector<uint8_t> correction{0xd3, 0, 2, 0x3e, 0xd0};
    const auto checksum = RTCMFramer::crc24q(correction);
    correction.push_back(checksum >> 16);
    correction.push_back(checksum >> 8);
    correction.push_back(checksum);
    CHECK(RTCMFramer::isValidFrame(correction));
    sbf_buf_t native{};
    native.sync = 0x4024;
    native.msg_id = SBF_ID_PVTGeodetic;
    native.length = offsetof(sbf_buf_t, payload_pvt_geodetic) + sizeof(native.payload_pvt_geodetic);
    native.WNc = 2435;
    native.payload_pvt_geodetic.mode_type = 1;
    native.payload_pvt_geodetic.latitude = 0.5;
    native.payload_pvt_geodetic.longitude = 1;
    // Both checksums are valid; only the outer native frame may own these bytes.
    std::memcpy(&native.payload_pvt_geodetic.rx_clk_bias, correction.data(), correction.size());
    native.crc16 = crc16(reinterpret_cast<uint8_t*>(&native) + 4, native.length - 4);
    driver.consume({reinterpret_cast<uint8_t*>(&native), native.length});
    CHECK(correctionCount == 0);
    gps_test_time += 200000;
    driver.consume({});
    CHECK(fixCount == 1);
    CHECK(std::abs(position.latitude_deg - 0.5 * M_RAD_TO_DEG) < 1e-6);
    driver.consume(correction);
    CHECK(correctionCount == 1);
    CHECK(fixCount == 1);
}

int main()
{
    try {
        for (bool septentrio : {false, true}) {
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
        sbfFrameOwnership();
        receiverMode(true, GPSProtocol::OutputMode::GPSAndRTCM, true);
        receiverMode(false, GPSProtocol::OutputMode::GPS, true, {}, false, 1);
        receiverMode(false, GPSProtocol::OutputMode::GPS, true, {}, false, GPS_READ_BUFFER_SIZE,
                     2 * GPS_READ_BUFFER_SIZE - 3);
        std::puts("PASS receiver position/base modes and failed mode switches");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        return 1;
    }
    return 0;
}
