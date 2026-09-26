#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

namespace {
struct ScriptedIO
{
    enum class Operation
    {
        Read,
        Write,
        Baud
    };
    GPSTestClock& clock;
    Operation fault;
    GPSProtocolError error;
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

    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(clock);
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocolError::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                        detail};
            }
            clock.advanceTo(deadline.untilUs + 1000);
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocolError::Cancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error, 0, 0,
                        detail};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        result.setBaudrate = [this](unsigned) {
            return !fail(Operation::Baud)                 ? GPSBaudStatus::Configured
                   : error == GPSProtocolError::Cancelled ? GPSBaudStatus::Cancelled
                                                          : GPSBaudStatus::Error;
        };
        return result;
    }
};

/// Every native family, in family-table order.
constexpr std::array FAMILY_TYPES{GPSType::ublox,   GPSType::trimble, GPSType::septentrio, GPSType::femto,
                                  GPSType::unicore, GPSType::quectel, GPSType::passive};

/// Every family logs under its own child of GPS.Driver.Protocols, so a single family can be enabled.
void checkLogCategory(GPSTestClock& clock, GPSType type, const char* expected)
{
    const GPSProtocolLogCapture log;
    const auto* family = gpsReceiverFamily(type);
    CHECK(family);
    auto host = std::make_unique<GPSProtocolRuntime>(*family, makeGPSRuntimeTestIO(clock), GPSRuntimeObserver{},
                                                     GPSFamilyOptions{.satelliteInfoEnabled = false});
    // No family supports both persistent changes and receiver-managed averaging, so validation rejects this.
    const GPSConfig config{.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{}}, .allowPersistentChanges = true};
    unsigned baud = 115200;
    CHECK(!host->configure(config, baud));
    CHECK(!log.categories().empty());
    CHECK(log.categories().count(QString::fromLatin1(expected)) == log.categories().size());
}

void driverLogCategories(GPSTestClock& clock)
{
    checkLogCategory(clock, GPSType::ublox, "GPS.Driver.Protocols.UBX");
    checkLogCategory(clock, GPSType::septentrio, "GPS.Driver.Protocols.SBF");
    checkLogCategory(clock, GPSType::trimble, "GPS.Driver.Protocols.Ashtech");
    checkLogCategory(clock, GPSType::femto, "GPS.Driver.Protocols.Femto");
    checkLogCategory(clock, GPSType::quectel, "GPS.Driver.Protocols.Quectel");
    checkLogCategory(clock, GPSType::unicore, "GPS.Driver.Protocols.Unicore");
    checkLogCategory(clock, GPSType::passive, "GPS.Driver.Protocols.Passive");
}

}  // namespace

class GPSRuntimeIOContractTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSRuntimeIOContractTest::_protocol()
{
    GPSTestClock clock;
    try {
        driverLogCategories(clock);
        CHECK(GPSDeadline{}.remaining(0) == std::chrono::milliseconds(INT32_MAX));
        CHECK(GPSDeadline{0}.remaining(0) == 0ms);
        CHECK(GPSDeadline{1}.remaining(0) == 1ms);
        CHECK(GPSDeadline{1000}.remaining(0) == 1ms);
        CHECK(GPSDeadline{1001}.remaining(0) == 2ms);
        CHECK(GPSDeadline{1000}.remaining(1001) == 0ms);
        CHECK(GPSDeadline{UINT64_MAX}.remaining(UINT64_MAX - 1001) == 2ms);
        CHECK(GPSDeadline{UINT64_MAX}.remaining(UINT64_MAX) == 0ms);
        CHECK(GPSDeadline::after(1000, 2ms).untilUs == 3000);
        CHECK(GPSDeadline::after(1000, -1ms).untilUs == 1000);
        CHECK(GPSDeadline{}.toQDeadlineTimer().isForever());
        CHECK(GPSDeadline{0}.toQDeadlineTimer().hasExpired());
        const auto liveDeadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);
        const GPSDeadline deadline{static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(liveDeadline.time_since_epoch()).count())};
        const auto qtDeadline = deadline.toQDeadlineTimer();
        CHECK(!qtDeadline.isForever());
        CHECK(std::abs(qtDeadline.deadline() - QDeadlineTimer(liveDeadline, Qt::PreciseTimer).deadline()) <= 1);
        for (size_t family = 0; family != FAMILY_TYPES.size(); ++family) {
            const GPSType type = FAMILY_TYPES[family];
            const bool passive = type == GPSType::passive;
            for (const auto fault :
                 {ScriptedIO::Operation::Read, ScriptedIO::Operation::Write, ScriptedIO::Operation::Baud}) {
                if (passive && fault != ScriptedIO::Operation::Baud) {
                    continue;
                }
                for (const auto error : {GPSProtocolError::Cancelled, GPSProtocolError::Transport}) {
                    clock.reset();
                    ScriptedIO io{clock, fault, error};
                    const GPSProtocolLogCapture log;
                    const auto* descriptor = gpsReceiverFamily(type);
                    CHECK(descriptor);
                    auto receiver = std::make_unique<GPSProtocolRuntime>(*descriptor, io.io());
                    GPSConfig config{};
                    if (type == GPSType::unicore) {
                        config.base.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s};
                    } else if (!passive) {
                        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
                        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
                    }
                    unsigned baudrate = passive ? 115200 : 0;
                    const bool configured = receiver->configure(config, baudrate);
                    CHECK(io.failed);
                    CHECK(!configured);
                    CHECK(receiver->error() == error);
                    if (fault != ScriptedIO::Operation::Baud) {
                        CHECK(!receiver->errorDetail().isEmpty());
                    }
                    const auto warnings = log.warnings();
                    if (fault == ScriptedIO::Operation::Read && error != GPSProtocolError::Cancelled && family < 4) {
                        CHECK(warnings == QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                                          .arg(static_cast<int>(GPSReadStatus::Error))
                                                          .arg(io.detail)});
                    } else if (fault == ScriptedIO::Operation::Read && error != GPSProtocolError::Cancelled) {
                        CHECK(!warnings.empty());
                    } else if (family < 4) {
                        CHECK(warnings.empty());
                    }
                    CHECK(receiver->receive(10ms) == GPSReceiveUpdates{});
                    CHECK(receiver->error() == error);
                    if (fault != ScriptedIO::Operation::Baud) {
                        CHECK(!receiver->errorDetail().isEmpty());
                    }
                    CHECK(log.warnings() == warnings);
                }
            }
        }
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSRuntimeIOContractTest, TestLabel::Unit)

#include "gps-io-contract-test.moc"
