#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolFeatures.h"
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
    enum class Operation
    {
        Read,
        Write,
        Baud
    };
    Operation fault;
    int error;
    bool failed = false;
    unsigned operations = 0;

    bool fail(Operation operation)
    {
        CHECK(!failed);
        CHECK(++operations < 100);
        failed = operation == fault;
        return failed;
    }

    GPSProtocolIO io()
    {
        auto result = makeGPSProtocolTestIO();
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSProtocolReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocol::ReadCancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error};
            }
            gps_test_time = deadline.untilUs + 1000;
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSProtocolWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocol::ReadCancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size()), 0};
        };
        result.setBaudrate = [this](unsigned) {
            return !fail(Operation::Baud)                ? GPSBaudStatus::Configured
                   : error == GPSProtocol::ReadCancelled ? GPSBaudStatus::Cancelled
                                                         : GPSBaudStatus::Error;
        };
        return result;
    }
};

static std::unique_ptr<GPSBaseProtocol> createReceiver(unsigned family, ScriptedIO& io,
                                                       GPSNativePositionReport& position,
                                                       GPSNativeSatelliteReport& satellites)
{
    switch (family) {
#if QGC_GPS_ENABLE_UBX
        case 0:
            return std::make_unique<GPSNativeUBX>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_ASHTECH
        case 1:
            return std::make_unique<GPSNativeAshtech>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_SBF
        case 2:
            return std::make_unique<GPSNativeSBF>(io.io(), &position, &satellites);
#endif
#if QGC_GPS_ENABLE_FEMTO
        case 3:
            return std::make_unique<GPSNativeFemto>(io.io(), &position, &satellites);
#endif
        default:
            return {};
    }
}

int main()
{
    try {
        CHECK(GPSDeadline{}.remainingMilliseconds(0) == INT32_MAX);
        CHECK(GPSDeadline{0}.remainingMilliseconds(0) == 0);
        CHECK(GPSDeadline{1}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1001}.remainingMilliseconds(0) == 2);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(1001) == 0);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX - 1001) == 2);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX) == 0);
        for (unsigned family = 0; family != 4; ++family) {
            for (const auto fault :
                 {ScriptedIO::Operation::Read, ScriptedIO::Operation::Write, ScriptedIO::Operation::Baud}) {
                for (const int error : {GPSProtocol::ReadCancelled, -EIO}) {
                    for (const auto mode : {GPSProtocol::OutputMode::GPS, GPSProtocol::OutputMode::RTCM}) {
                        gps_test_time = 0;
                        ScriptedIO io{fault, error};
                        GPSNativePositionReport position{};
                        GPSNativeSatelliteReport satellites{};
                        auto receiver = createReceiver(family, io, position, satellites);
                        if (!receiver) {
                            continue;
                        }
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
