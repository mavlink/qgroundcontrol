#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolTestIO.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

struct ScriptedIO
{
    GPSCallbackType fault;
    int error;
    bool failed = false;
    unsigned operations = 0;

    static int callback(GPSCallbackType type, void* data, int length, void* user)
    {
        auto& io = *static_cast<ScriptedIO*>(user);
        if (type != GPSCallbackType::readDeviceData && type != GPSCallbackType::writeDeviceData &&
            type != GPSCallbackType::setBaudrate) {
            return 0;
        }
        CHECK(!io.failed);
        CHECK(++io.operations < 100);
        if (type == io.fault) {
            io.failed = true;
            return io.error;
        }
        if (type == GPSCallbackType::readDeviceData) {
            const auto request = *static_cast<const GPSReadRequest*>(data);
            const int timeout = request.timeoutMs;
            data = request.buffer;
            gps_test_time += uint64_t(timeout + 1) * 1000;
            return 0;
        }
        return type == GPSCallbackType::writeDeviceData ? length : 0;
    }
};

static std::unique_ptr<GPSBaseProtocol> createReceiver(unsigned family, ScriptedIO& io, GPSPositionReport& position,
                                                       GPSSatelliteReport& satellites)
{
    switch (family) {
        case 0: {
            GPSDriverUBX::Settings settings{};
            return std::make_unique<GPSDriverUBX>(makeGPSProtocolTestIO(ScriptedIO::callback, &io), &position,
                                                  &satellites, settings);
        }
        case 1:
            return std::make_unique<GPSDriverAshtech>(makeGPSProtocolTestIO(ScriptedIO::callback, &io), &position,
                                                      &satellites);
        case 2:
            return std::make_unique<GPSDriverSBF>(makeGPSProtocolTestIO(ScriptedIO::callback, &io), &position,
                                                  &satellites);
        default:
            return std::make_unique<GPSDriverFemto>(makeGPSProtocolTestIO(ScriptedIO::callback, &io), &position,
                                                    &satellites);
    }
}

int main()
{
    try {
        for (unsigned family = 0; family != 4; ++family) {
            for (const auto fault :
                 {GPSCallbackType::readDeviceData, GPSCallbackType::writeDeviceData, GPSCallbackType::setBaudrate}) {
                for (const int error : {GPSProtocol::ReadCancelled, -EIO}) {
                    for (const auto mode : {GPSProtocol::OutputMode::GPS, GPSProtocol::OutputMode::RTCM}) {
                        gps_test_time = 0;
                        ScriptedIO io{fault, error};
                        GPSPositionReport position{};
                        GPSSatelliteReport satellites{};
                        auto receiver = createReceiver(family, io, position, satellites);
                        GPSProtocol::GPSConfig config{};
                        config.base.surveyInAccMeters = 1;
                        config.base.surveyInDurationSecs = 60;
                        config.output_mode = mode;
                        unsigned baudrate = 115200;
                        const int result = receiver->configure(baudrate, config);
                        CHECK(io.failed);
                        CHECK(result < 0);
                        CHECK(receiver->ioError() == error);
                        CHECK(receiver->receive(10) < 0);
                        CHECK(receiver->ioError() == error);
                    }
                }
            }
        }
    } catch (const std::exception& error) {
        fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
