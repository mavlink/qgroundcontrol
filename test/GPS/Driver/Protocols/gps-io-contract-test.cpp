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
    QString detail = QStringLiteral("Receiver connection lost: Gerät disconnected");

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
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocol::ReadCancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                        detail};
            }
            gps_test_time = deadline.untilUs + 1000;
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocol::ReadCancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error, 0, 0,
                        detail};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        result.setBaudrate = [this](unsigned) {
            return !fail(Operation::Baud)                ? GPSBaudStatus::Configured
                   : error == GPSProtocol::ReadCancelled ? GPSBaudStatus::Cancelled
                                                         : GPSBaudStatus::Error;
        };
        return result;
    }
};

class IOProbe : public GPSProtocol
{
public:
    using GPSProtocol::beginCommandWrite;
    using GPSProtocol::GPSProtocol;
    using GPSProtocol::log;
    using GPSProtocol::read;
    using GPSProtocol::resetIOError;
    using GPSProtocol::write;

    int configure(unsigned&, const GPSConfig&) override { return 0; }

    int receive(unsigned timeout) override { return receiveDecoded(timeout); }
};

static void sharedResults()
{
    const uint8_t payload[6] = {};
    const QString detail = QStringLiteral("Invalid receiver progress: Gerät");
    const std::array<GPSWriteResult, 7> invalidWrites{{
        {GPSWriteStatus::Completed, 6, 2, detail},
        {GPSWriteStatus::Completed, 0, 0, detail},
        {GPSWriteStatus::Completed, -1, 6, detail},
        {GPSWriteStatus::Completed, 6, -1, detail},
        {GPSWriteStatus::Completed, 6, 7, detail},
        {GPSWriteStatus::Completed, 7, 6, detail},
        {GPSWriteStatus::Unsupported, 6, 6, detail},
    }};
    for (const auto& result : invalidWrites) {
        int writes = 0;
        GPSCommandResult evidence;
        auto io = makeGPSProtocolTestIO();
        io.write = [&](std::span<const uint8_t>, GPSDeadline) {
            ++writes;
            return result;
        };
        io.commandFinished = [&](const auto& command) { evidence = command; };
        IOProbe probe(std::move(io));
        probe.beginCommandWrite("probe");
        CHECK(probe.write(payload, sizeof(payload)) < 0);
        CHECK(probe.ioError() == -EIO);
        CHECK(probe.ioErrorDetail() == detail);
        CHECK(evidence.acceptedBytes == result.acceptedBytes);
        CHECK(evidence.writtenBytes == result.writtenBytes);
        CHECK(evidence.uncertainBytes == result.uncertainBytes());
        CHECK(probe.write(payload, sizeof(payload)) < 0);
        CHECK(writes == 1);
        probe.resetIOError();
        CHECK(probe.ioErrorDetail().isEmpty());
    }
    const std::array<GPSReadResult, 3> invalidReads{{
        {GPSReadStatus::Data, -1, detail},
        {GPSReadStatus::Data, 2, detail},
        {GPSReadStatus::TimedOut, 1, detail},
    }};
    for (const auto& result : invalidReads) {
        auto io = makeGPSProtocolTestIO();
        io.read = [&](std::span<uint8_t>, GPSDeadline) { return result; };
        IOProbe probe(std::move(io));
        uint8_t byte;
        CHECK(probe.read(&byte, 1, 0) == -EIO);
        CHECK(probe.ioErrorDetail() == detail);
    }
    QStringList messages;
    GPSProtocolIO io;
    io.log = [&](GPSProtocolLogLevel, QStringView message) { messages.push_back(message.toString()); };
    IOProbe probe(std::move(io));
    probe.log(GPSProtocolLogLevel::Warning, "Gerät");
    probe.log(GPSProtocolLogLevel::Warning, "%s %d", "Gerät", 2);
    CHECK(messages == (QStringList{QStringLiteral("Gerät"), QStringLiteral("Gerät 2")}));
}

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
        sharedResults();
        CHECK(GPSDeadline{}.remainingMilliseconds(0) == INT32_MAX);
        CHECK(GPSDeadline{0}.remainingMilliseconds(0) == 0);
        CHECK(GPSDeadline{1}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(0) == 1);
        CHECK(GPSDeadline{1001}.remainingMilliseconds(0) == 2);
        CHECK(GPSDeadline{1000}.remainingMilliseconds(1001) == 0);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX - 1001) == 2);
        CHECK(GPSDeadline{UINT64_MAX}.remainingMilliseconds(UINT64_MAX) == 0);
        CHECK(GPSDeadline{}.toQDeadlineTimer().isForever());
        CHECK(GPSDeadline{0}.toQDeadlineTimer().hasExpired());
        const auto liveDeadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);
        const GPSDeadline deadline{static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(liveDeadline.time_since_epoch()).count())};
        const auto qtDeadline = deadline.toQDeadlineTimer();
        CHECK(!qtDeadline.isForever());
        CHECK(std::abs(qtDeadline.deadline() - QDeadlineTimer(liveDeadline, Qt::PreciseTimer).deadline()) <= 1);
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
                        CHECK(receiver->ioErrorDetail() ==
                              (fault == ScriptedIO::Operation::Baud ? QString() : io.detail));
                        CHECK(receiver->receive(10) < 0);
                        CHECK(receiver->ioError() == error);
                        CHECK(receiver->ioErrorDetail() ==
                              (fault == ScriptedIO::Operation::Baud ? QString() : io.detail));
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
